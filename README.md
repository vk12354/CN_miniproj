# Designing a Proxy Server

A beginner-friendly C forward HTTP proxy for a Computer Networks mini-project.

## Features

- Concurrent client handling using pthreads
- Real HTTP request parsing
- HTTP request forwarding
- Domain/URL access control
- File-based response cache with 60-second TTL
- Thread-safe logging
- Graceful error handling and socket timeouts
- Performance measurement

## Requirements

Linux/Ubuntu/WSL2 with:

```bash
sudo apt update
sudo apt install gcc make git curl python3
```

## Build

```bash
make clean
make
```

## Run

```bash
./proxy_server 8080
```

Default port is 8080.

## Test

Automated test suite:

```bash
make test
```

Manual request through proxy:

```bash
NO_PROXY= HTTP_PROXY= HTTPS_PROXY= ALL_PROXY= \
curl -v -x http://127.0.0.1:8080 http://127.0.0.1:18080/
```

For a local destination server:

```bash
python3 -m http.server 18080
```

## Cache demonstration

Request the same URL twice. The first request should be a `MISS`; the next valid request should be a `HIT`.

Inspect:

```bash
tail -f logs/proxy.log
```

## Access control demonstration

Edit:

`config/blocklist.txt`

Add a domain, restart the proxy, and request that domain through the proxy. The proxy should return `403 Forbidden` and log `BLOCKED`.

## Performance

```bash
bash tests/performance.sh
```

The script compares a direct request with proxy MISS and proxy HIT timings.

## Project structure

```text
src/       C implementation
include/   shared constants
config/    access-control configuration
cache/     generated cache files
logs/      generated log files
tests/     automated/manual test helpers
docs/      architecture, testing, performance, demo and viva notes
```

## Team contributions

- Member 1 — Networking & concurrency
- Member 2 — HTTP parsing & forwarding
- Member 3 — Caching & access control
- Member 4 — Logging, testing, performance & documentation

Replace these role labels with your team's actual names.
