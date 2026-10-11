# Step-by-Step Development Phases

This repository is intentionally organized as an incremental four-member team project.

## Phase 1 — Foundation

**Agent 1:** TCP listener, socket utilities, pthread worker model.

**Agent 2:** HTTP request data structures and parser.

**Agent 3:** Cache and blocklist interfaces.

**Agent 4:** Logging format, test harness, and documentation skeleton.

## Phase 2 — Basic Proxy

Integrate Agent 1 and Agent 2:

```text
Client -> Proxy -> Destination -> Proxy -> Client
```

Do not add advanced features until basic forwarding works.

## Phase 3 — Access Control

Agent 3 adds the blocklist check before any destination connection.

## Phase 4 — Cache

Agent 3 adds cache HIT/MISS and TTL handling. Shared cache operations are protected against concurrent access.

## Phase 5 — Logging

Agent 4 integrates logging across the request lifecycle.

## Phase 6 — Robustness

All agents test malformed input, destination failures, timeouts, disconnects, and simultaneous clients.

## Phase 7 — Performance

Agent 4 records direct, proxy MISS, and proxy HIT timings. Only measured values should be used in the final report.

## Phase 8 — Final Integration

All agents review the complete system, update documentation, rehearse the demo, and prepare for viva questions.
