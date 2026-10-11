# Viva Questions and Answers

## What is a forward proxy?
A forward proxy is an intermediary chosen by clients. The client sends the request to the proxy, and the proxy contacts the destination server on the client's behalf.

## Why are threads used?
Each client gets an independent worker thread so a slow request does not block all other clients.

## Why TCP?
HTTP/1.x relies on a reliable byte-stream transport, and TCP sockets give direct practice with connection-oriented networking.

## How does caching work?
The proxy builds a cache key from host, port and path. For a valid GET cache entry it serves the stored raw HTTP response. Otherwise it fetches from the origin and stores a successful 200 response for 60 seconds.

## Why is the cache protected by a mutex?
Multiple worker threads may access the cache simultaneously. The mutex prevents races during cache file operations.

## How does access control work?
The destination host is compared against entries in `config/blocklist.txt` before an outbound connection is created.

## What happens when the server is unreachable?
The proxy returns `502 Bad Gateway` when it cannot connect to the destination.

## What is cache MISS vs HIT?
MISS means the proxy must contact the destination. HIT means a still-valid response is already available locally.

## Why are responses read until connection close?
The proxy normalizes outbound requests to `Connection: close`, so the response can be delimited by the destination closing the connection. This keeps the implementation simple for the project.

## What are the main limitations?
The proxy focuses on HTTP and does not decrypt arbitrary HTTPS traffic. It does not implement connection reuse or sophisticated cache eviction.
