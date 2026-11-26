#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <host> <port> <path> [METHOD] [BODY]\n", argv[0]);
        printf("Examples:\n");
        printf("  %s 127.0.0.1 8080 / GET\n", argv[0]);
        printf("  %s 127.0.0.1 8080 / HEAD\n", argv[0]);
        printf("  %s 127.0.0.1 8080 /echo POST \"Hello ITT440\"\n", argv[0]);
        return 1;
    }

    const char *host = argv[1];
    const char *port = argv[2];
    const char *path = argv[3];
    const char *method = (argc >= 5) ? argv[4] : "GET";
    const char *body = (argc >= 6) ? argv[5] : "";

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo failed");
        return 1;
    }

    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd < 0) {
        perror("socket failed");
        freeaddrinfo(res);
        return 1;
    }

    if (connect(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect failed");
        close(sock_fd);
        freeaddrinfo(res);
        return 1;
    }
    freeaddrinfo(res);

    char req[BUF_SIZE];
    int req_len;
    if (strcmp(method, "POST") == 0) {
        req_len = snprintf(req, sizeof(req),
            "%s %s HTTP/1.1\r\n"
            "Host: %s:%s\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            method, path, host, port, strlen(body), body);
    } else {
        req_len = snprintf(req, sizeof(req),
            "%s %s HTTP/1.1\r\n"
            "Host: %s:%s\r\n"
            "Connection: close\r\n"
            "\r\n",
            method, path, host, port);
    }

    send(sock_fd, req, (size_t)req_len, 0);

    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = recv(sock_fd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }

    close(sock_fd);
    return 0;
}
