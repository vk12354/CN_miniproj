#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PROXY_PORT=18081
ORIGIN_PORT=18080
COUNTER="$(mktemp)"
ORIGIN_LOG="$(mktemp)"
PROXY_LOG_PID=""

cleanup() {
  [[ -n "$PROXY_LOG_PID" ]] && kill "$PROXY_LOG_PID" 2>/dev/null || true
  [[ -n "${ORIGIN_PID:-}" ]] && kill "$ORIGIN_PID" 2>/dev/null || true
  rm -f "$COUNTER" "$ORIGIN_LOG"
}
trap cleanup EXIT

make clean >/dev/null
make >/dev/null

python3 tests/origin_server.py --port "$ORIGIN_PORT" --counter "$COUNTER" >"$ORIGIN_LOG" 2>&1 &
ORIGIN_PID=$!
./proxy_server "$PROXY_PORT" > /tmp/proxy_test_stdout.log 2>&1 &
PROXY_LOG_PID=$!
sleep 1

export NO_PROXY=""
export no_proxy=""
export HTTP_PROXY=""
export HTTPS_PROXY=""
export ALL_PROXY=""

BASE="http://127.0.0.1:${ORIGIN_PORT}"
URL="$BASE/hello"

echo "[1/7] normal forwarding"
curl --fail --silent --show-error -x "http://127.0.0.1:${PROXY_PORT}" "$URL" | grep -q 'origin response for /hello'

echo "[2/7] cache hit after first miss"
curl --fail --silent --show-error -x "http://127.0.0.1:${PROXY_PORT}" "$URL" | grep -q 'origin response for /hello'
[[ "$(cat "$COUNTER")" == "1" ]]
grep -q 'MISS' logs/proxy.log
grep -q 'HIT' logs/proxy.log

echo "[3/7] access control"
curl --silent --show-error -x "http://127.0.0.1:${PROXY_PORT}" -o /tmp/blocked.out -w '%{http_code}' http://blocked.test/ | grep -q '^403$'
grep -q 'BLOCKED' logs/proxy.log

echo "blocked response: $(cat /tmp/blocked.out)"


echo "[4/7] malformed request"
python3 - <<PY
import socket
s=socket.create_connection(('127.0.0.1', $PROXY_PORT), 2)
s.sendall(b'NOT HTTP\\r\\n\\r\\n')
data=s.recv(4096)
s.close()
assert b'400 Bad Request' in data, data
PY

echo "malformed request handled"

echo "[5/7] unreachable destination"
status=$(curl --silent --show-error -x "http://127.0.0.1:${PROXY_PORT}" -o /tmp/unreachable.out -w '%{http_code}' http://127.0.0.1:1/ || true)
[[ "$status" == "502" ]]
echo "unreachable destination returned $status"

echo "[6/7] concurrent clients"
pids=()
for i in {1..8}; do
  curl --fail --silent --show-error -x "http://127.0.0.1:${PROXY_PORT}" "$BASE/concurrent-$i" >/dev/null &
  pids+=("$!")
done
for pid in "${pids[@]}"; do wait "$pid"; done

echo "[7/7] source tree and logs present"
test -f README.md
test -f docs/architecture.md
test -f docs/performance.md
test -f docs/demo.md
test -s logs/proxy.log

echo "ALL TESTS PASSED"
