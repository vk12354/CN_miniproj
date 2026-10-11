# Live Demo Script

## 1. Start destination server

Terminal 1:

```bash
python3 -m http.server 18080
```

## 2. Start proxy

Terminal 2:

```bash
./proxy_server 8080
```

## 3. Normal forwarding

Terminal 3:

```bash
NO_PROXY= HTTP_PROXY= HTTPS_PROXY= ALL_PROXY= \
curl -i -x http://127.0.0.1:8080 http://127.0.0.1:18080/
```

Explain:

`Client -> Proxy -> Destination -> Proxy -> Client`

## 4. Cache MISS vs HIT

Run the same GET twice.

```bash
NO_PROXY= HTTP_PROXY= HTTPS_PROXY= ALL_PROXY= \
curl -s -x http://127.0.0.1:8080 http://127.0.0.1:18080/README.md >/dev/null

NO_PROXY= HTTP_PROXY= HTTPS_PROXY= ALL_PROXY= \
curl -s -x http://127.0.0.1:8080 http://127.0.0.1:18080/README.md >/dev/null
```

Show:

```bash
tail -n 5 logs/proxy.log
```

The first request should show `MISS`; the second should show `HIT`.

## 5. Access control

Put a domain in `config/blocklist.txt`, restart the proxy, and request it through the proxy.

Expected:

`403 Forbidden`

## 6. Concurrency

Use several curl processes at the same time. The server should keep processing all client connections.

## 7. Error handling

Try an unreachable destination and a malformed request. Show that the server returns a controlled error and stays alive.

## 8. Performance

Run:

```bash
bash tests/performance.sh
```

Explain direct vs proxy latency and the effect of a cache HIT.
