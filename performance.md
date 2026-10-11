# Performance Measurement

The assignment asks for performance comparison with and without the proxy.

Run:

```bash
bash tests/performance.sh
```

This prints three timings:

1. direct request to the origin server
2. proxy request that causes a cache MISS
3. repeated proxy request that causes a cache HIT

Record the actual measured numbers in your report. Do not replace them with made-up values.

Suggested calculation:

```text
Proxy overhead (MISS) = proxy_miss - direct
Cache improvement      = proxy_miss / proxy_hit
```

The exact values depend on the computer, network stack, origin server, and workload.
