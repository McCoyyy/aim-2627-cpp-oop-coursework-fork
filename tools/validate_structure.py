from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    required = [
        "include/warehouse/domain.hpp",
        "include/dispatch/policy.hpp",
        "include/dispatch/scheduler.hpp",
        "include/audit/trace.hpp",
        "include/app/cli.hpp",
        "src/warehouse/domain.cpp",
        "src/dispatch/policy.cpp",
        "src/dispatch/scheduler.cpp",
        "src/audit/trace.cpp",
        "src/app/cli.cpp",
        "ros2_ws/src/warehouse_dispatch/CMakeLists.txt",
    ]
    missing = [path for path in required if not (ROOT / path).is_file()]
    if missing:
        print("Missing required files:")
        print("\n".join(missing))
        return 1
    print("Structure OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
