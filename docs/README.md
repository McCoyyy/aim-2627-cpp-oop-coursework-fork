# 运行与验收说明

面向接手维护者：以下命令均在仓库根目录执行，复制即可运行。

## 0. 环境要求

| 用途 | 要求 |
| --- | --- |
| 本地构建（Q1/Q3/Q4） | 支持 C++17 的编译器、CMake 3.16 或更高、Python 3（结构检查脚本） |
| ROS 2 演示（Q2） | Ubuntu 24.04、ROS 2 Jazzy（Base 即可） |

核心库只使用 C++ 标准库，没有第三方依赖。

## 1. 本地构建与测试

```bash
cmake -S . -B build
cmake --build build --parallel 4
./build/public_tests
./build/core_demo
python3 tools/validate_structure.py
```

本次实测输出：

```text
$ ./build/public_tests
PUBLIC 2/2

$ ./build/core_demo
AIM OOP CW2 1.0          # 退出码 0

$ python3 tools/validate_structure.py
Structure OK
```

`tools/check.sh` 把上面三步串起来执行，等价于：

```bash
bash tools/check.sh
```

## 2. Q4 调试程序运行说明

`src/debug/main.cpp` 是独立于核心库的旧控制器，编译目标为 `debug_task`。

```bash
./build/debug_task
```

修复前（模板原始版本）输出：

```text
S1 no -1 yes 0
```

修复后输出：

```text
S1 no 5 yes 0
```

五个字段依次是：订单 O1 被派到的站点、O2 是否被接受、拒绝 O2 后 S2 的库存、O3 是否被接受、接受 O3 后 S1 的库存。第三个字段从 `-1` 变成 `5` 是本次修复的唯一目标，其余四个字段必须保持不变。

## 3. ROS 2 构建与运行

在 Ubuntu 24.04 上安装 ROS 2 Jazzy 后执行：

```bash
source /opt/ros/jazzy/setup.bash
cd ros2_ws
colcon build --symlink-install
source install/setup.bash
ros2 launch warehouse_dispatch warehouse_demo.launch.py
```

本次实测构建输出：

```text
Starting >>> warehouse_dispatch
Finished <<< warehouse_dispatch [9.27s]

Summary: 1 package finished [9.27s]
```

启动后 `warehouse_simulator` 每 300 ms 发布一条事件，`warehouse_dispatcher` 处理完 `WAIT 1` 后两个节点分别退出，`ros2 launch` 自行返回命令行提示符，不留后台进程。

## 4. ROS 2 预期输出对照

节点用 `RCLCPP_INFO` 打印到终端，下面只列日志前缀之后的消息正文。

| 输入事件（`/warehouse_events`） | 预期输出（`/dispatch_results`） | 状态变化 |
| --- | --- | --- |
| `ORDER O1 SKU-A 1` | `ACCEPT O1 S1` | S1 库存 3 变 2，busy，持有 O1 |
| `ORDER O2 SKU-A 1` | `ACCEPT O2 S2` | S2 库存 2 变 1，busy，持有 O2 |
| `ORDER O3 SKU-A 2` | `REJECT O3` | 两个站点均 busy，无候选站 |
| `COMPLETE O1` | `COMPLETE O1` | S1 回到 idle，清空订单，库存保持 2 |
| `ORDER O4 SKU-A 3` | `REJECT O4` | S1 库存 2 小于 3，S2 仍 busy |
| `WAIT 1` | `WAITED 1` | 不改变状态，随后两节点退出 |

六行必须按顺序出现且只出现一次。拒绝行不带尾随空格。

## 5. 故障排查

1. `ros2: command not found` 或 `colcon: command not found`
   `/opt/ros/jazzy/setup.bash` 只对当前终端生效，新开终端必须重新执行 `source`。`colcon build` 之前没有 source 时会报找不到 `rclcpp` 或 `std_msgs`，报错形如 `Could not find a package configuration file provided by "rclcpp"`。

2. `ros2 launch` 结束后仍有节点残留
   先执行 `ros2 node list`，如果还能看到 `/warehouse_dispatcher` 或 `/warehouse_simulator`，说明 `WAIT 1` 没有到达调度节点。用 `ros2 topic echo /warehouse_events` 确认模拟节点是否在发布，用 `ros2 topic hz /warehouse_events` 确认频率约为 3.3 Hz。残留进程可以用 `pkill -f warehouse_dispatcher` 清掉后重跑。

3. 终端只出现前几条结果就停住
   `warehouse_dispatcher` 在收到 `COMPLETE O1` 时会用派单阶段记录的真实站点去调用 `Scheduler::complete`。若核心库的 `submit` 返回的 `station_id` 为空，完成请求会被判为未知站点，后续事件不再产生预期输出。先跑 `./build/public_tests` 确认核心库本身正常。

4. `./build/debug_task` 仍然输出 `S1 no -1 yes 0`
   说明二进制没有重新生成，或者改的位置不对。先 `cmake --build build --clean-first` 再跑；如果依旧，检查 `src/debug/main.cpp` 的 `submit` 里库存判断是否位于 `item.second -= quantity` 之前，以及 `quantity <= 0` 的拦截是否加在循环外。
