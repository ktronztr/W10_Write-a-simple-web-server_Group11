#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    const char *method = "GET";

    if (argc > 1 &&
        (strcmp(argv[1], "GET") == 0 ||
         strcmp(argv[1], "HEAD") == 0)) {
        method = argv[1];
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if (connect(sock, (struct sockaddr *)&server,
                sizeof(server)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    char request[512];

    snprintf(request, sizeof(request),
             "%s / HTTP/1.0\r\n"
             "Host: localhost\r\n"
             "Connection: close\r\n\r\n",
             method);

    send(sock, request, strlen(request), 0);

    printf("HTTP Request:\n%s", request);
    printf("\nHTTP Response:\n");

    char buffer[BUFFER_SIZE];
    ssize_t bytes;

    while ((bytes = recv(sock, buffer,
                         sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes, stdout);
    }

    close(sock);
    return 0;
}