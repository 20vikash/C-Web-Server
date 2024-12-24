#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define APP_MAX_BUFFER 1024
#define PORT 8080

int main(int argc, char const *argv[])
{
    fd_set fs;
    int server_fd, client_fd;
    int client_fds[100];
    int index = 0;
    struct sockaddr_in address;
    int address_len = sizeof(address);

    char buffer[APP_MAX_BUFFER] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, address_len) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        FD_ZERO(&fs);
        FD_SET(server_fd, &fs);

        int max_fd = server_fd;

        for (int i = 0; i < index; i++) {
            FD_SET(client_fds[i], &fs);
            if (client_fds[i] > max_fd) {
                max_fd = client_fds[i];
            }
        }

        select(max_fd + 1, &fs, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &fs)) {
            client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&address_len);
            if (client_fd < 0) {
                perror("Accept failed");
                continue;
            }

            client_fds[index] = client_fd;
            index++;
            printf("New client connected (fd: %d)\n", client_fd);
        }

        for (int i = 0; i < index; i++) {
            if (FD_ISSET(client_fds[i], &fs)) {
                int read_bytes = read(client_fds[i], buffer, APP_MAX_BUFFER);
                if (read_bytes <= 0) {
                    printf("Client disconnected (fd: %d)\n", client_fds[i]);
                    close(client_fds[i]);

                    for (int j = i; j < index - 1; j++) {
                        client_fds[j] = client_fds[j + 1];
                    }
                    index--;
                    i--;
                } else {
                    printf("Received message: %s\n", buffer);

                    char *http_response = "HTTP/1.1 200 OK\n"
                                        "Content-Type: text/plain\n"
                                        "Connection: keep-alive\n"
                                        "Content-Length: 13\n\n"
                                        "Hello world!\n";

                    write(client_fds[i], http_response, strlen(http_response));
                }
            }
        }
    }

    return 0;
}
