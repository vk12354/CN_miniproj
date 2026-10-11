# Testing

## Build

```bash
make clean
make
```

## Automated tests

```bash
make test
```

The test script starts a local origin server and exercises forwarding, cache HIT/MISS, access control, malformed input, unreachable destination handling, and concurrent clients.

## Manual test

Start the proxy:

```bash
./proxy_server 8080
```

Start a destination server in another terminal:

```bash
python3 -m http.server 18080
```

Then use curl through the proxy:

```bash
NO_PROXY= HTTP_PROXY= HTTPS_PROXY= ALL_PROXY= \
curl -v -x http://127.0.0.1:8080 http://127.0.0.1:18080/
```

Inspect:

```bash
tail -f logs/proxy.log
```

## Access control demo

Add:

```text
blocked.test
```

to `config/blocklist.txt`, start/restart the proxy, and run:

```bash
NO_PROXY= HTTP_PROXY= HTTPS_PROXY= ALL_PROXY= \
curl -i -x http://127.0.0.1:8080 http://blocked.test/
```

Expected status:

```text
403 Forbidden
```
