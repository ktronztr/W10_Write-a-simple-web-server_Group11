#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("HTTP Server running at http://127.0.0.1:%d\n",
           PORT);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        ssize_t n = recv(client_fd, buffer,
                         sizeof(buffer) - 1, 0);

        if (n <= 0) {
            close(client_fd);
            continue;
        }

        char method[16] = {0};
        char path[256] = {0};
        char version[16] = {0};

        sscanf(buffer, "%15s %255s %15s",
               method, path, version);

        printf("Request: %s %s %s\n",
               method, path, version);

        if (strcmp(version, "HTTP/1.0") != 0 &&
            strcmp(version, "HTTP/1.1") != 0) {
            const char *response =
                "HTTP/1.0 400 Bad Request\r\n"
                "Content-Length: 0\r\n\r\n";
            send(client_fd, response, strlen(response), 0);
        } else if (strcmp(method, "GET") != 0 &&
                   strcmp(method, "HEAD") != 0) {
            const char *response =
                "HTTP/1.0 405 Method Not Allowed\r\n"
                "Allow: GET, HEAD\r\n"
                "Content-Length: 0\r\n\r\n";
            send(client_fd, response, strlen(response), 0);
        } else if (strcmp(path, "/") != 0 &&
                   strcmp(path, "/index.html") != 0) {
            const char *response =
                "HTTP/1.0 404 Not Found\r\n"
                "Content-Length: 0\r\n\r\n";
            send(client_fd, response, strlen(response), 0);
        } else {
            FILE *file = fopen("www/index.html", "rb");

            if (file == NULL) {
                const char *response =
                    "HTTP/1.0 404 Not Found\r\n"
                    "Content-Length: 0\r\n\r\n";
                send(client_fd, response, strlen(response), 0);
            } else {
                struct stat st;
                stat("www/index.html", &st);

                char date[100];
                char modified[100];
                time_t now = time(NULL);
                struct tm tm_now, tm_modified;

                gmtime_r(&now, &tm_now);
                gmtime_r(&st.st_mtime, &tm_modified);

                strftime(date, sizeof(date),
                         "%a, %d %b %Y %H:%M:%S GMT",
                         &tm_now);
                strftime(modified, sizeof(modified),
                         "%a, %d %b %Y %H:%M:%S GMT",
                         &tm_modified);

                char headers[1024];
                int header_len = snprintf(
                    headers, sizeof(headers),
                    "HTTP/1.0 200 OK\r\n"
                    "Date: %s\r\n"
                    "Server: Lab10-HTTP-Server\r\n"
                    "Last-Modified: %s\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    "Content-Length: %lld\r\n"
                    "Connection: close\r\n\r\n",
                    date, modified, (long long)st.st_size
                );

                send(client_fd, headers, header_len, 0);

                if (strcmp(method, "GET") == 0) {
                    char file_buffer[BUFFER_SIZE];
                    size_t bytes;

                    while ((bytes = fread(
                        file_buffer, 1,
                        sizeof(file_buffer), file)) > 0) {
                        send(client_fd, file_buffer,
                             bytes, 0);
                    }
                }

                fclose(file);
            }
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}