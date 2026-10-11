# Proxy Server Architecture

## Purpose

This project implements a forward proxy for HTTP traffic. The proxy accepts requests from clients, checks access-control rules, checks a local response cache, forwards cache misses to a destination server, relays the response, and logs the transaction.

## Module ownership

- **Agent 1:** TCP server, socket utilities, pthread concurrency
- **Agent 2:** HTTP parsing, request normalization, destination forwarding
- **Agent 3:** blocklist access control, cache storage and TTL policy
- **Agent 4:** logging, tests, performance, documentation and demo

## Runtime flow

```text
Client
  |
  | HTTP request
  v
accept()
  |
  v
pthread worker
  |
  v
HTTP parser
  |
  +--> malformed? --> 400
  |
  v
Access control
  |
  +--> blocked? --> 403
  |
  v
Cache lookup (GET)
  |
  +--> HIT --> cached response --> Client
  |
  +--> MISS
         |
         v
   connect() to origin
         |
         +--> fail --> 502
         |
         v
   forward HTTP request
         |
         v
   receive response
         |
         +--> client
         |
         +--> cache eligible 200 GET response
         |
         v
       logger
```

## Concurrency

Each accepted client is handled by a detached pthread. The cache module protects cache file operations with a mutex, and the logger protects the shared log file with a mutex.

## HTTP policy

The implementation supports `GET`. Other methods return `501 Not Implemented`. Requests are normalized to HTTP/1.0 with `Connection: close`, which makes response framing straightforward for this educational proxy.

## Cache policy

Only successful `GET` responses with status `200` are stored, and each entry is valid for 60 seconds. A cache file stores a header, the exact cache key, and the raw HTTP response bytes. Writes use a temporary file followed by `rename()` so an incomplete file is not presented as a valid cache entry.

## Access control

`config/blocklist.txt` contains one domain per line. Exact domain matches and subdomains of blocked domains are rejected before the proxy connects to the destination.

## Error handling

Malformed/oversized requests receive `400`. Unsupported methods receive `501`. Blocked hosts receive `403`. Destination connection failure receives `502`. Socket read/write problems are logged and connections are closed without terminating the whole server.

## Limitations

- HTTP is the primary supported protocol; this project intentionally does not implement HTTPS MITM.
- The response cache is bounded by 10 MiB per object.
- Cache eviction beyond TTL is not implemented.
- Full HTTP/1.1 connection reuse is not implemented; the proxy uses connection-close semantics.
