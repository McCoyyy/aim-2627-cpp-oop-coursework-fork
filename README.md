# AIM 2627 C++ OOP Coursework 02

仓库拣货机器人任务分配系统。全部任务规格见 [Problem.pdf](Problem.pdf)，本 README 只负责本地启动。

```bash
cmake -S . -B build
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./build/debug_task
```

## 修改范围

- 可以修改：`include/`、`src/`、`README.md`、`docs/`。
- 不要修改：`tests/`、`tools/`、`.github/`、`CMakeLists.txt`、`Problem.pdf`。
- `src/debug/main.cpp` 是 Q4 的独立调试任务；不要通过绕开缺陷的输出方式得分。

## ROS 2 演示

ROS 2 部分在 `ros2_ws/` 中独立构建，复用 `include/` 和 `src/` 的核心实现。课程使用 ROS 2 Jazzy，演示不依赖硬件。命令见题面 Q2。

## 文档交付

在 `docs/README.md` 和 `docs/DESIGN.md` 中完成 Q5。初始文件只是提示骨架，正式要求以题面为准。

## Agent 会话归档

允许并鼓励合理使用 AI 辅助开发，但必须保留可核对的过程记录。参照 Python CW 与 C++ Assessment 01，本作业使用 Agent Session Commit（AgentLedger）v0.1.5。

在个人学生仓库根目录配置，不要在教师完整目录或其他项目中安装：

```bash
python -m venv .venv-agent-session-commit
source .venv-agent-session-commit/bin/activate
python -m pip install --index-url https://pypi.org/simple 'agent-session-commit[pre-commit]==0.1.5'
agent-session-commit install --pre-commit
agent-session-commit status
```

每次正常 `git commit` 后，post-commit hook 会把新增 AI 会话打包到 `.agent-sessions/bundles/` 并 amend 到同一提交。`.agent-sessions/bundles/` 必须提交；`.agent-sessions/source/` 与 `.venv-agent-session-commit/` 已忽略。没有使用 AI 时，在 README 末尾明确声明并提供开发记录。

提交前检查：

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
