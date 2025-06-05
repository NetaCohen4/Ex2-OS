#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define MAX_CLIENTS  FD_SETSIZE
#define BUFFER_SIZE 1024

unsigned int carbon = 0, oxygen = 0, hydrogen = 0;

void print_stock() {
    printf("Stock - Carbon: %u, Oxygen: %u, Hydrogen: %u\n", carbon, oxygen, hydrogen);
}

void handle_command(const char *cmd) {
    char type[32];
    unsigned int amount;

    if (sscanf(cmd, "ADD %31s %u", type, &amount) == 2) {
        if (strcmp(type, "CARBON") == 0) {
            carbon += amount;
        } else if (strcmp(type, "OXYGEN") == 0) {
            oxygen += amount;
        } else if (strcmp(type, "HYDROGEN") == 0) {
            hydrogen += amount;
        } else {
            printf("ERROR: Invalid atom type\n");
            return;
        }
        print_stock();
    } else {
        printf("ERROR: Invalid command\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <TCP_PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    int client_sockets[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
        client_sockets[i] = -1;

    fd_set readfds;
    int max_fd;

    // יצירת סוקט TCP
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // listen
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Atom warehouse server is listening on port %d...\n", port);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        // add all clients 
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_fd)
                max_fd = sd;
        }

        // waiting for action 
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            continue;
        }

        // new connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                perror("accept");
                continue;
            }

            // finding place
            int added = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == -1) {
                    client_sockets[i] = new_socket;
                    added = 1;
                    break;
                }
            }
            if (!added) {
                printf("Too many clients. Rejecting connection.\n");
                close(new_socket);
            }
        }

        // handle clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd != -1 && FD_ISSET(sd, &readfds)) {
                char buffer[BUFFER_SIZE] = {0};
                int valread = read(sd, buffer, BUFFER_SIZE - 1);
                if (valread <= 0) {
                    close(sd);
                    client_sockets[i] = -1;
                } else {
                    handle_command(buffer);
                }
            }
        }
    }

    // Cleanup
    close(server_fd);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1) {
            close(client_sockets[i]);
        }
    }

    return 0;
}