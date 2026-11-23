# 设计与调试记录

## 1. 命名空间与类职责

| 命名空间 | 主要文件 | 职责 | 依赖 |
| --- | --- | --- | --- |
| `warehouse` | `include/warehouse/domain.hpp`、`src/warehouse/domain.cpp` | `Order`、`PickStation`、`RobotState` 与 `Inventory`。负责站点注册、查询、候选筛选、原子扣减和完成校验 | 只依赖 C++ 标准库 |
| `dispatch` | `include/dispatch/policy.hpp`、`include/dispatch/scheduler.hpp`、对应 `.cpp` | `DispatchPolicy` 抽象接口、`priority`/`stock` 两种实现，`Scheduler` 串起派单与完成流程 | 依赖 `warehouse`、`audit` |
| `audit` | `include/audit/trace.hpp`、`src/audit/trace.cpp` | `Trace` 按插入顺序保存成功事件，`append`/`events`/`clear` | 只依赖 C++ 标准库 |
| `app` | `include/app/cli.hpp`、`include/app/application.hpp`、对应 `.cpp` | SKU 规范化、ID 合法性校验、命令行入口与输出格式化 | 只依赖 C++ 标准库 |

依赖方向是单向的：`dispatch` 知道 `warehouse` 和 `audit`，反过来不成立。`warehouse/domain.hpp` 不包含任何 `dispatch/` 头文件，也不包含 ROS 头文件。`app` 不与 `dispatch` 互相包含，避免输入清洗规则和调度规则纠缠在一起。

`Inventory` 是唯一能改站点字段的类，`PickStation` 通过值拷贝对外返回，外部拿不到内部容器的引用。`DispatchPolicy::select` 的签名只有 `const`，策略无法修改库存或机器人状态，策略切换不需要动 `Scheduler`。

`Scheduler::submit` 的顺序是：校验订单 → 取候选站 → 调策略 → `reserve` 原子扣减 → 写 `Trace`。前四步中任何一步失败都直接返回，`Trace` 只在成功后追加。

## 2. 策略差异对比

| 策略 | 选择规则 | 平局处理 |
| --- | --- | --- |
| `priority` | 候选列表第一项 | 候选已按站点 ID 字典序升序排列，第一项就是 ID 最小的合格站 |
| `stock` | 当前库存最多的候选站 | 库存相同时取站点 ID 字典序较小者 |

同一初始状态下两种策略会选出不同站点：

```text
S1 = 2, S2 = 5，两台机器人均 idle，提交 ORDER O1 SKU-A 1
priority -> S1
stock    -> S2
```

`priority` 取 ID 最小的 S1，`stock` 取库存更多的 S2。这段结果由两份实现分别实例化核心库后运行得到，不是推导值。

`stock` 的平局分支依赖显式比较，不依赖输入顺序：

```cpp
if (candidate.stock > best->stock) {
    best = &candidate;
} else if (candidate.stock == best->stock && candidate.id < best->id) {
    best = &candidate;
}
```

## 3. 状态转换表

| 操作 | 库存 | 机器人状态 | 当前订单 ID | 审计轨迹 |
| --- | --- | --- | --- | --- |
| 派单成功 | 选中站点减少订单数量，结果不小于 0 | 选中站点 idle 变 busy | 选中站点记录订单 ID | 追加一条 `DISPATCH O S` |
| 完成成功 | 目标站点不变，库存不恢复 | 目标站点 busy 变 idle | 目标站点清空 | 追加一条 `COMPLETE O S` |
| 派单失败 | 全部不变 | 全部不变 | 全部不变 | 不追加 |
| 重复完成 | 全部不变 | 全部不变 | 全部不变 | 不追加 |

失败路径的实现方式是把校验全部前移到写入之前。`Inventory::reserve` 先判断数量、站点存在、机器人空闲、库存足够，四个条件都通过才执行 `stock -= quantity` 和三处状态赋值。`Inventory::finish` 先判断站点存在、状态 busy、订单 ID 完全一致，再改状态。

## 4. Q4 调试证据

复现输入固定在 `src/debug/main.cpp` 的 `main` 中：初始 `S1 = 3`、`S2 = 5`，两次提交 `O1`（数量 2）与 `O2`（数量 6），完成 `O1`，再提交 `O3`（数量 1）。

修复前输出：

```text
S1 no -1 yes 0
```

根因在 `LegacyDispatcher::submit`：循环里先执行 `item.second -= quantity`，之后才判断 `item.second < 0` 并返回失败。拒绝 O2 时 S2 的库存已经被扣成 `5 - 6 = -1`，函数虽然返回了拒绝，状态却留下了副作用。

修复前后差异：

```cpp
// 修复前
if (!busy_[station_id]) {
    item.second -= quantity;          // 先扣
    if (item.second < 0) {            // 后判
        return {false, "", "insufficient stock"};
    }
    busy_[station_id] = true;
    orders_[station_id] = order_id;
    return {true, station_id, "accepted"};
}

// 修复后
if (busy_[station_id]) {
    continue;
}
if (item.second < quantity) {         // 先判
    return {false, "", "insufficient stock"};
}
item.second -= quantity;              // 后扣
busy_[station_id] = true;
orders_[station_id] = order_id;
return {true, station_id, "accepted"};
```

同时在函数入口加了 `quantity <= 0` 的拦截。原控制流保持不变：仍按站点 ID 顺序取第一个空闲站点，该站库存不足就立即拒绝，不继续扫描后续站点。修复后输出 `S1 no 5 yes 0`。

## 5. 修复前输出记录

模板初始提交 `9ca9389` 中的 `src/debug/main.cpp` 就是缺陷版本，修复前在同一台机器上编译运行得到：

```text
$ ./build/debug_task
S1 no -1 yes 0
```

该版本可用 `git show 9ca9389:src/debug/main.cpp` 取回后单独编译复核。本次开发先把模板提交拉下来跑出上述错误输出，再做修改，因此错误现象和根因是对着实跑结果写的。

## 6. 边界验证记录

三条必测边界都用 `./build/debug_task` 的五字段输出核对，命令均为 `./build/debug_task`。该程序把四个事件串在同一个序列里，一次运行同时覆盖三个场景，下表按输出字段拆开对应。

| 场景 | 输入 | 预期 | 实际 |
| --- | --- | --- | --- |
| 库存不足 | `O2` 数量 6，此时 S1 为 busy，只能考察 S2（库存 5） | 第 2 字段 `no`，第 3 字段保持 `5` | `S1 no 5 yes 0`，第 2、3 字段为 `no 5` |
| 库存恰好等于数量 | `O3` 数量 1，`O1` 完成后 S1 库存为 1 | 第 4 字段 `yes`，第 5 字段 `0` | `S1 no 5 yes 0`，第 4、5 字段为 `yes 0` |
| 忙碌站点被跳过 | 提交 `O2` 时 S1 处于 busy | 不派给 S1，第 1 字段仍是 O1 的站点 `S1` | `S1 no 5 yes 0`，第 1 字段 `S1` |

核心库层另有两条对照记录，取自 ROS 2 演示的实际运行结果：

| 场景 | 输入 | 预期 | 实际 |
| --- | --- | --- | --- |
| 候选站为空 | `ORDER O3 SKU-A 2`，S1、S2 均 busy | 拒绝且不改状态 | `REJECT O3` |
| 库存不足 | `ORDER O4 SKU-A 3`，S1 库存 2、S2 仍 busy | 拒绝且不改状态 | `REJECT O4` |

以上五条覆盖了库存不足、库存恰好等于数量、忙碌站点被跳过、候选站为空四种失败与边界情形，输出与题面 2.3.1 和 4.2.1 的对照表一致。
