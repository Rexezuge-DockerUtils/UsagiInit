# UsagiInit

> Compact init/service manager in C. Container-first; designed as PID 1.

## What is it?

UsagiInit is a lightweight init system and service manager for containers. It runs your init script through `/bin/sh` (full POSIX shell syntax), tracks background services, and reaps their children as PID 1. It ships as a statically-linked pair of binaries (`UsagiInit` + `usagi-reg`) for minimal container images.

## Features

- **Full sh syntax** — the init script is executed by `/bin/sh`, so `if/for/while`, functions, `&&`/`||`, command substitution, and all POSIX shell constructs work out of the box.
- **Transparent service registration** — any line ending with `&` is automatically converted to a `usagi-reg` call; existing scripts need no changes.
- **Signal forwarding** — `SIGINT`, `SIGTERM`, `SIGHUP` are forwarded to the service process group.
- **Service restarts** — optionally restart any terminated service or only failed (non-zero / signal-killed) services, up to a configurable limit with exponential backoff.
- **Full reinitialization** — when all services have exited, optionally `exec` yourself to restart from the top.
- **Debug & release logging** — debug builds emit `LOG_DEBUG` and `LOG_TRACE` with file/line info; release builds strip them. Colors are suppressed when stdout is not a terminal.

## Quick start

Write an init script as `./UsagiInit.sh` in the same directory as the binary, or pass a custom script as the first argument:

```bash
./UsagiInit ./my-init.sh
```

A simple script might look like:

```sh
export LOG_LEVEL=info
cd /app

if [ "$ENV" = "prod" ]; then
  ./web-server --tls &
else
  ./web-server &
fi

./background-worker &
```

Lines ending with `&` become managed services. Full POSIX shell syntax (`if`, `for`, functions, pipes, etc.) is available. Once the script ends, UsagiInit enters the guardian phase and watches those services.

## Building

Requires a C11 compiler, `cmake`, and optionally `bash`.

### Development build

```bash
./scripts/build.sh
```

This compiles the binary with debug logging and no restart behavior.

### Release build

```bash
./scripts/release_static.sh
```

This runs the full integration test suite, then rebuilds a statically-linked binary with all feature flags enabled and produces the compressed artifact used by the Docker image.

## Docker

A multi-stage `Dockerfile` is included. It builds both `UsagiInit` and `usagi-reg` as static binaries, compresses them with UPX, and copies them into a `busybox:stable-musl` image. Both binaries must be present in the same directory at runtime.

Build the image locally:

```bash
docker build -t usagi-init .
```

Use it as your container's entry point:

```dockerfile
FROM rexezugedockerutils/usagi-init:latest
COPY UsagiInit.sh /UsagiInit.sh
ENTRYPOINT ["/UsagiInit"]
```

The base image ships both `/UsagiInit` and `/usagi-reg`. `usagi-reg` is an internal helper that registers background services with the guardian — it is not intended to be called directly from init scripts.

## Supported Features

Because the init script runs through `/bin/sh`, all standard POSIX shell constructs are available. The table below highlights the UsagiInit-specific behaviours on top of that:

| Feature | Syntax | Description |
|---------|--------|-------------|
| Full sh syntax | `if`, `for`, `while`, functions, … | Any valid POSIX shell construct works in the init script |
| Service registration | `cmd &` | Run the command as a tracked service (preprocessed to `usagi-reg cmd`) |
| Export | `export VAR=value` | Sets an environment variable inherited by services |
| Redirection | `>`, `<`, `2>`, `2>&1` | Redirect stdin, stdout, or stderr of any command |
| Pipes | `cmd1 \| cmd2` | Chain commands together |
| Comment | `# comment` | Ignored lines in scripts |

### Known preprocessor limitations

Lines with complex background constructs (grouped commands `{ ... } &` or subshells `( ... ) &`) are not auto-detected as services. Run them with the service flag commented if guardian restart is not needed, or restructure them as a wrapper script.

## Testing

Run the full integration test suite:

```bash
./tst/run.sh
```

Individual test cases live under `tst/integration/<name>/`. Each test builds the binary, runs a `UsagiInit.sh` script, and diffs normalized output against an `expected.txt` snapshot.

## License

MIT License. See `LICENSE` for details.
