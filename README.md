# CN_miniproj
# Designing a Proxy Server
### Computer Networks Mini Project

1. Project Overview

A proxy server acts as an intermediary between a client and a destination server. It receives requests from clients, processes them according to predefined rules, forwards permitted requests to the destination, and returns the responses to the clients.

This project implements a **forward HTTP proxy server using C**. It demonstrates important networking concepts such as TCP socket programming, concurrent client handling, HTTP request processing, response caching, access control, and traffic logging.

The proxy is designed to improve request handling, control access to selected domains, and provide useful information about network traffic.

 2. Why We Chose This Project

We chose the Proxy Server project to gain practical knowledge of how communication between clients and web servers works in real-world networks. Rather than studying networking concepts only in theory, this project allows us to implement them using C and TCP sockets.

The project also gives us an opportunity to work with HTTP protocols, multithreading, caching mechanisms, and access-control techniques. Its combination of networking fundamentals and practical software development makes it a valuable project for understanding how network intermediaries operate.

 3. Importance of the Project

Proxy servers are widely used in networks for traffic management, access control, performance optimization, and monitoring.

The main importance of this project includes:

- **Performance optimization:** Caching eligible responses can reduce repeated requests to destination servers and improve response times.
- **Access control:** A configurable blocklist allows the proxy to restrict access to selected domains.
- **Concurrent communication:** Multithreading enables the server to handle multiple clients without making each client wait for the previous request to finish.
- **Traffic monitoring:** Logging helps administrators understand requests, response statuses, cache activity, and errors.
- **Network reliability:** Error handling and connection timeouts help the proxy respond appropriately when requests are invalid or destination servers are unavailable.

Through this project, we gain practical experience in building a network application that combines communication, performance, control, and reliability.

 4. Key Features

- TCP-based forward proxy implemented in C.
- HTTP request parsing and forwarding.
- Concurrent client handling using POSIX threads (`pthreads`).
- Domain-based access control using a configurable blocklist.
- Response caching with a time-to-live (TTL) policy.
- Cache HIT and MISS detection.
- Traffic logging for request and response analysis.
- Error handling for malformed requests and unreachable destinations.
- Performance comparison between direct and proxied requests.

**Protocol scope:** The implementation focuses on HTTP GET requests and does not perform HTTPS traffic decryption.

 5. Technologies Used

| Technology | Purpose |
|---|---|
| C | Core implementation |
| POSIX TCP sockets | Client and server communication |
| pthreads | Concurrent client handling |
| GCC | Compilation |
| Makefile | Build automation |
| curl | Sending HTTP requests and testing |
| Git and GitHub | Version control and collaboration |
| Linux / Ubuntu | Development and execution environment |

 6. Team Members and Responsibilities

The project is divided among four members, with each member responsible for a distinct component of the system.

# 1. Vivek Raj — Networking and Concurrency

- Implementing the TCP server and socket management.
- Handling client connections using `socket()`, `bind()`, `listen()`, and `accept()`.
- Implementing concurrent client handling using `pthreads`.
- Managing connection cleanup and supporting integration with the other modules.

# 2. Rajat Jha — HTTP Parsing and Request Forwarding

- Implementing HTTP request parsing and validation.
- Extracting request methods, destination hosts, ports, and paths.
- Establishing connections to destination servers.
- Forwarding HTTP requests and relaying responses to clients.
- Handling protocol-related errors and invalid requests.

# 3. Hrishika Biswas — Caching and Access Control

- Implementing the response caching mechanism.
- Managing cache entries, TTL, and cache HIT/MISS decisions.
- Developing the configurable domain blocklist.
- Implementing access-control checks and blocked-request responses.
- Ensuring safe access to shared cache resources during concurrent requests.

# 4. Sanchali Ray — Logging, Testing, and Documentation

- Implementing and integrating traffic logging.
- Testing normal forwarding, caching, access control, and error handling.
- Testing the proxy with multiple simultaneous clients.
- Measuring and documenting latency and throughput.
- Preparing the README, design documentation, testing instructions, and live demonstration procedure.

All four members contribute to integration, debugging, verification, and preparation for the final project demonstration. Each member is also expected to understand the overall architecture and the major design decisions.

7. General Working Flow
          Client / Browser
                 |
                 v
          Proxy Server
                 |
                 v
        Parse HTTP Request
                 |
                 v
        Validate the Request
                 |
                 v
         Access-Control Check
            /          \
        Blocked        Allowed
           |              |
           v              v
       Return 403     Check Cache
                      /         \
                    HIT         MISS
                     |            |
                     v            v
                 Return       Contact
                 Cached       Destination
                 Response     Server
                                  |
                                  v
                           Receive Response
                                  |
                                  v
                           Cache if Eligible
                                  |
                                  v
                           Return to Client


Traffic events, cache activity, and errors are logged throughout the request-processing lifecycle.

8. Expected Outcomes

By completing this project, we aim to demonstrate:

- Successful HTTP request forwarding through a proxy.
- Concurrent handling of multiple client connections.
- Correct blocking of configured domains.
- Cache HIT, MISS, and expiration behavior.
- Graceful handling of invalid requests and connection failures.
- Useful traffic logs.
- Measured performance differences between direct connections and proxied requests.
9. Conclusion

The Proxy Server mini-project provides practical insight into the operation of application-layer network services. It combines TCP communication, HTTP processing, concurrency, caching, access control, and performance analysis in a single system.

Working collaboratively on this project helps us strengthen our C programming skills, understand computer networking concepts, and gain experience in modular software development, testing, debugging, and version control.

The project demonstrates how a relatively simple intermediary can be extended to provide useful network-management capabilities while highlighting the trade-offs between functionality, performance, and reliability.

