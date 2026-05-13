# UsagiInit — Agent Quick-Start

> Compact init/service manager in C. Container-first; designed as PID 1.

## Repository Layout

| Directory | Purpose |
|-----------|---------|
| `src/`   | Source (shell/, logger.c, services.c, guardian.c, signals.c, globals.c, main.c) |
| `include/` | Public headers mirroring src/ layout — all headers live here |
| `tst/integration/` | Black-box integration tests that build the binary and compare stdout against expected snapshots |
| `scripts/` | `build.sh` (dev) and `release_static.sh` (release) |

## Build Commands

Dev build with default flags (debug-style logging, no restart):  
```bash
./scripts/build.sh
```
This just runs `cmake . -DCMAKE_BUILD_TYPE=Build -DTERMINATE_ALL_PROCESSES=OFF` then `cmake --build .`. Extra `-D` flags are forwarded via `$@`.

CI / release build (default on Docker):  
```bash
./scripts/release_static.sh
```
Runs dev build + integration tests, removes binary, then reconfigures with all feature flags enabled:
- `-DTERMINATE_ALL_PROCESSES=ON`
- `-DREINITIALIZE_ON_ALL_SERVICE_TERMINATION=ON`
- `-DRESTART_TERMINATED_SERVICES=ON`

## Testing

Integration tests assume a built `./UsagiInit` binary at the repo root.

- Run all integration tests: `./tst/run.sh`
- Run a single test: `cd tst/integration/<name>` then run its `./run.sh`
- Tests rebuild the binary if needed (e.g. env-var-expansion rebuilds with `TERMINATE_ALL_PROCESSES=OFF`), but others need correct flags already set before launching.

Snapshot tests normalize ANSI colors, timestamps, and PIDs before diffing.

## Feature Flags

Set at compile-time via CMake:
- `TERMINATE_ALL_PROCESSES`: On SIGINT/SIGTERM/SIGHUP, kill(-1, signal) instead of only exiting shell phase.
- `REINITIALIZE_ON_ALL_SERVICE_TERMINATION`: When every service has exited, execvp() yourself to restart from the top.
- `RESTART_TERMINATED_SERVICES`: Restart any terminated service (up to 999 in release mode, 2 in dev). Also implicitly enables `RESTART_FAILED_SERVICES`.
- `RESTART_FAILED_SERVICES`: Restart services only when they exit non-zero or are killed by signal.

## Architecture Notes

- Two-phase execution: shell phase (interprets a script or stdin) → guardian phase (reaps children and restarts services).
- `&` on a line makes that command a service (tracked in `services.c`.
- Default script path: `./UsagiInit.sh`; argv[1] provides an alternate script.
- `export VAR=value` and `$VAR` expansion are supported; persists into spawned processes.
- Logging behavior changes between Debug and Release: `CMAKE_BUILD_TYPE=Release` neuters `LOG_DEBUG`/`LOG_TRACE` and strips file/line info.

## Code Style

- `.clang-format` at repo root (LLVM-ish, 2-space indent, 80-column limit).
- All headers use `#pragma once`.
- Format before committing: run `clang-format -i` on changed files.

## Docker

`Dockerfile` uses a multi-stage build:
1. Compile statically linked `UsagiInit` (Alpine stage).
2. Compress with UPX.
3. Copy into a minimal `busybox:stable-musl` image.

