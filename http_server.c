#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUF_SIZE 4096

static void send_response(int client_fd, const char *status, const char *content_type,
                          const char *body, size_t body_len, int send_body) {
    char header[1024];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, content_type, body_len);

    send(client_fd, header, (size_t)hlen, 0);
    if (send_body && body && body_len > 0) {
        send(client_fd, body, body_len, 0);
    }
}

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    printf("Server running at http://127.0.0.1:%d (GET, HEAD, POST /echo)\n", PORT);

    int req_count = 0;

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        char req[BUF_SIZE] = {0};
        ssize_t n = recv(client_fd, req, sizeof(req) - 1, 0);
        if (n <= 0) {
            close(client_fd);
            continue;
        }

        req_count++;
        char method[16] = {0};
        char path[256] = {0};
        char version[16] = {0};
        sscanf(req, "%15s %255s %15s", method, path, version);
        printf("[#%d] %s %s %s\n", req_count, method, path, version);

        if (strcmp(method, "POST") == 0 && strcmp(path, "/echo") == 0) {
            char *body_start = strstr(req, "\r\n\r\n");
            const char *payload = body_start ? (body_start + 4) : "";
            char resp_body[BUF_SIZE];
            int rlen = snprintf(resp_body, sizeof(resp_body),
                "Request #%d echoed POST data:\n%s\n", req_count, payload);
            send_response(client_fd, "200 OK", "text/plain", resp_body, (size_t)rlen, 1);
            close(client_fd);
            continue;
        }

        int is_get = (strcmp(method, "GET") == 0);
        int is_head = (strcmp(method, "HEAD") == 0);
        if (!is_get && !is_head) {
            const char *msg = "405 Method Not Allowed\n";
            send_response(client_fd, "405 Method Not Allowed", "text/plain", msg, strlen(msg), 1);
            close(client_fd);
            continue;
        }

        if (strstr(path, "..") != NULL) {
            const char *msg = "403 Forbidden\n";
            send_response(client_fd, "403 Forbidden", "text/plain", msg, strlen(msg), !is_head);
            close(client_fd);
            continue;
        }

        char filepath[512];
        if (strcmp(path, "/") == 0) {
            snprintf(filepath, sizeof(filepath), "www/index.html");
        } else {
            snprintf(filepath, sizeof(filepath), "www%s", path);
        }

        FILE *fp = fopen(filepath, "rb");
        if (!fp) {
            const char *msg = "<h1>404 Not Found</h1><p>File does not exist.</p>";
            send_response(client_fd, "404 Not Found", "text/html", msg, strlen(msg), !is_head);
            close(client_fd);
            continue;
        }

        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        rewind(fp);

        char *body = malloc((size_t)fsize + 1);
        size_t read_bytes = fread(body, 1, (size_t)fsize, fp);
        body[read_bytes] = '\0';
        fclose(fp);

        const char *ctype = "text/html";
        if (strstr(filepath, ".css")) ctype = "text/css";
        else if (strstr(filepath, ".json")) ctype = "application/json";
        else if (strstr(filepath, ".txt")) ctype = "text/plain";

        send_response(client_fd, "200 OK", ctype, body, read_bytes, is_get);
        free(body);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
