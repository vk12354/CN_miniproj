#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PROXY_PORT=18081
ORIGIN_PORT=18080
COUNTER="$(mktemp)"
trap 'kill "${PROXY_PID:-}" 2>/dev/null || true; kill "${ORIGIN_PID:-}" 2>/dev/null || true; rm -f "$COUNTER"' EXIT

make >/dev/null
python3 tests/origin_server.py --port "$ORIGIN_PORT" --counter "$COUNTER" >/dev/null 2>&1 &
ORIGIN_PID=$!
./proxy_server "$PROXY_PORT" >/dev/null 2>&1 &
PROXY_PID=$!
sleep 1

export NO_PROXY=""; export no_proxy=""; export HTTP_PROXY=""; export HTTPS_PROXY=""; export ALL_PROXY=""

printf '%-30s %12s\n' "Test" "time(s)"
printf '%-30s %12s\n' "------------------------------" "------------"

direct=$(curl -s -o /dev/null -w '%{time_total}' "http://127.0.0.1:${ORIGIN_PORT}/perf-direct")
proxy_miss=$(curl -s -o /dev/null -w '%{time_total}' -x "http://127.0.0.1:${PROXY_PORT}" "http://127.0.0.1:${ORIGIN_PORT}/perf-miss")
proxy_hit=$(curl -s -o /dev/null -w '%{time_total}' -x "http://127.0.0.1:${PROXY_PORT}" "http://127.0.0.1:${ORIGIN_PORT}/perf-miss")

printf '%-30s %12s\n' "Direct" "$direct"
printf '%-30s %12s\n' "Proxy (cache MISS)" "$proxy_miss"
printf '%-30s %12s\n' "Proxy (cache HIT)" "$proxy_hit"
