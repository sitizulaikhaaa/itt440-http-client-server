# ITT440 Individual Assignment - HTTP Client & Server in C

## Overview
A custom HTTP/1.1 client and server written in C using POSIX TCP sockets. The server logs incoming requests with a counter, serves static files from www/, and supports GET, HEAD, and POST /echo.

## Compile & Run
gcc -Wall -Wextra -o http_server http_server.c
gcc -Wall -Wextra -o http_client http_client.c

Start the server:
./http_server

Run the client in another terminal:
./http_client 127.0.0.1 8080 / GET
./http_client 127.0.0.1 8080 / HEAD
./http_client 127.0.0.1 8080 /echo POST "Hello from client"

## Sources & References
- Linux manual pages (socket, bind, listen, accept, connect, getaddrinfo, send, recv)
- HTTP/1.1 message format and status code specifications (RFC 9110 / RFC 7230)

