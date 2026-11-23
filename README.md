# AIM 2627 C++ OOP Coursework 02

仓库拣货机器人任务分配系统。调度核心是仅依赖 C++ 标准库的 `aim_oop_core`，同一份实现被本地演示、公开测试与 ROS 2 调度节点复用。完整规格见 [Problem.pdf](Problem.pdf)。

## 快速开始

```bash
cmake -S . -B build
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./build/public_tests
./build/debug_task
```

预期输出、ROS 2 构建运行命令与故障排查见 [`docs/README.md`](docs/README.md)。

## 仓库结构

| 路径 | 内容 |
| --- | --- |
| `include/` | `warehouse`、`dispatch`、`audit`、`app` 四个命名空间的公开头文件 |
| `src/` | 核心库实现、命令行入口、Q4 独立调试程序 |
| `ros2_ws/` | ROS 2 Jazzy 包 `warehouse_dispatch`，编译同一批核心源码 |
| `docs/` | 运行说明与设计记录 |
| `tests/`、`tools/` | 公开测试与结构校验脚本，保持模板原样 |

## ROS 2 演示

ROS 2 部分在 `ros2_ws/` 中独立构建，两个节点只通过核心库的公开接口完成调度，不复制库存扣减或候选筛选逻辑。构建、运行与六条预期输出的对照见 `docs/README.md` 第 3、4 节。

## 文档

- [`docs/README.md`](docs/README.md)：环境要求、本地构建与验收命令、Q4 调试程序、ROS 2 运行步骤、故障排查。
- [`docs/DESIGN.md`](docs/DESIGN.md)：四个命名空间的职责与依赖方向、两种调度策略的差异、状态转换表、Q4 调试证据与边界验证记录。

## Agent 会话归档

本仓库使用 Agent Session Commit（AgentLedger）v0.1.5 记录 AI 辅助开发的过程，归档文件位于 `.agent-sessions/bundles/`。每次提交后由 post-commit hook 把新增会话打包并 amend 到同一提交。

提交前可以用以下命令核对归档是否随提交一起进入版本库：

```bash
git rev-parse HEAD
git show --stat HEAD -- .agent-sessions/
```

## AI 使用与开发过程

本次作业使用了 AI 编程助手，工具为 Tencent WorkBuddy（CLI 形态的编码代理）。

帮助范围：

- Q1 的 `warehouse/domain.cpp`、`dispatch/policy.cpp`、`dispatch/scheduler.cpp`、`audit/trace.cpp`、`app/cli.cpp` 五个实现文件的初稿。
- Q4 `src/debug/main.cpp` 的修复草稿。
- Q5 的 `docs/README.md` 与 `docs/DESIGN.md` 初稿。

未采纳的建议：

- 建议在 `Scheduler::submit` 中调用 `app::valid_order_id` 和 `app::valid_station_id` 做输入校验。这样会让 `dispatch` 反向依赖 `app`，与本作业的单向依赖要求冲突，改为在 `dispatch` 内部保留一份本地校验。
- 建议把 `Inventory::available_for` 的返回类型改为 `std::optional<std::vector<PickStation>>`，用 `nullopt` 表示无候选。公开方法签名由题面固定，未改动。

验证方式：以下命令均为实际运行结果，不是推断值。

```text
./build/public_tests                  -> PUBLIC 2/2
./build/debug_task                    -> S1 no 5 yes 0
python3 tools/validate_structure.py   -> Structure OK
ros2 launch warehouse_dispatch ...    -> ACCEPT O1 S1 / ACCEPT O2 S2 / REJECT O3 /
                                         COMPLETE O1 / REJECT O4 / WAITED 1
```

ROS 2 演示在 Ubuntu 24.04 + ROS 2 Jazzy 环境中构建运行，`colcon build` 在 2 核环境耗时 9.27 秒，两个节点在输出第六行后自行退出。逐条边界验证记录见 `docs/DESIGN.md` 第 6 节。

归档位置：`.agent-sessions/bundles/`。归档由 Agent Session Commit（AgentLedger）v0.1.5 在本仓库以原生 hook 方式安装后生成，导出源目录为 `.agent-sessions/source/`（该目录按仓库约定不入版本库）。归档内容为本次开发会话的导出记录，与本仓库无关的本机环境信息没有纳入。
