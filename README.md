# UsagiInit

> Compact init/service manager in C. Container-first; designed as PID 1.

## What is it?

UsagiInit is a lightweight init system and service manager for containers. It interprets a simple script language, launches services in the background, and reaps their children. It is built as a statically-linked single binary so it can serve as the sole PID 1 inside minimal container images.

## Features

- **Shell-like script interpreter** — supports `cd`, `export VAR=value`, `$VAR` expansion, pipe `|`, and redirection (`<`, `>`, `2>`).
- **Background services** — any line ending with `&` becomes a tracked service.
- **Signal forwarding** — `SIGINT`, `SIGTERM`, `SIGHUP` are forwarded to the service process group.
- **Service restarts** — optionally restart any terminated service or only failed (non-zero / signal-killed) services, up to a configurable limit.
- **Full reinitialization** — when all services have exited, optionally `exec` yourself to restart from the top.
- **Debug & release logging** — debug builds emit `LOG_DEBUG` and `LOG_TRACE` with file/line info; release builds strip them for a clean look.

## Quick start

Write an init script as `./UsagiInit.sh` in the same directory as the binary, or pass a custom script as the first argument:

```bash
./UsagiInit ./my-init.sh
```

A simple script might look like:

```sh
export LOG_LEVEL=info
cd /app
./web-server &
./background-worker &
```

Lines ending with `&` become services; all other commands are executed in the foreground during the shell phase. Once the script ends, UsagiInit enters the guardian phase and waits for services to finish.

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

A multi-stage `Dockerfile` is included. It builds the static binary, compresses it with UPX, and copies it into a `busybox:stable-musl` image.

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

## Supported Features

| Feature | Syntax | Description |
|---------|--------|-------------|
| Export | `export VAR=value` | Sets an environment variable visible to all subsequent commands |
| Expansion | `echo $VAR` | Expands a previously exported variable into a command argument |
| Redirection | `>`, `<`, `2>` | Redirect stdin, stdout, or stderr |
| Pipes | `cmd1 | cmd2` | Chain commands together |
| Background | `cmd &` | Run the command as a persistent service |
| Comment | `# comment` | Ignored lines in scripts |

## Testing

Run the full integration test suite:

```bash
./tst/run.sh
```

Individual test cases live under `tst/integration/<name>/`. Each test builds the binary, runs a `UsagiInit.sh` script, and diffs normalized output against an `expected.txt` snapshot.

## License

MIT License. See `LICENSE` for details.
