#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>

#define MAX_CLIENTS  FD_SETSIZE
#define BUFFER_SIZE 1024

unsigned long long int carbon = 0, oxygen = 0, hydrogen = 0;

void print_stock() {
    printf("Stock - Carbon: %llu, Oxygen: %llu, Hydrogen: %llu\n", carbon, oxygen, hydrogen);
}

void handle_command_tcp(const char *cmd) {
    char type[32];
    unsigned long long int amount;

    if (sscanf(cmd, "ADD %31s %llu", type, &amount) == 2) {
        if (strcmp(type, "CARBON") == 0) {
            carbon += amount;
        } else if (strcmp(type, "OXYGEN") == 0) {
            oxygen += amount;
        } else if (strcmp(type, "HYDROGEN") == 0) {
            hydrogen += amount;
        }
        print_stock();
        return;
    }
    printf("ERROR: Invalid Command\n");
}

int handle_command_udp(const char *cmd) {
    unsigned long long int amount;
    if (sscanf(cmd, "DELIVER WATER %llu", &amount) == 1) {
        if (hydrogen >= 2 * amount && oxygen >= amount) {
            hydrogen -= 2 * amount;
            oxygen -= amount;
            print_stock();
            return 1;
        } else {
            return 0;
        }
    } else if (sscanf(cmd, "DELIVER CARBON DIOXIDE %llu", &amount) == 1) {
        if (carbon >= amount && oxygen >= 2 * amount) {
            carbon -= amount;
            oxygen -= 2 * amount;
            print_stock();
            return 1;
        } else {
            return 0;
        }
    } else if (sscanf(cmd, "DELIVER ALCOHOL %llu", &amount) == 1) {
        if (carbon >= 2 * amount && hydrogen >= 6 * amount && oxygen >= amount) {
            carbon -= 2 * amount;
            hydrogen -= 6 * amount;
            oxygen -= amount;
            print_stock();
            return 1;
        } else {
            return 0;
        }
    } else if (sscanf(cmd, "DELIVER GLUCOSE %llu", &amount) == 1) {
        if (carbon >= 6 * amount && hydrogen >= 12 * amount && oxygen >= 6 * amount) {
            carbon -= 6 * amount;
            hydrogen -= 12 * amount;
            oxygen -= 6 * amount;
            print_stock();
            return 1;
        } else {
            return 0;
        }
    }
    printf("ERROR: Invalid Command\n");
    return -1; // Invalid command
}

void handle_console_command(const char *cmd) {
    if (strncmp(cmd, "GEN ", 4) != 0) {
        printf("ERROR: Unknown console command\n");
        return;
    }

    const char *drink = cmd + 4;
    int amount = 0;

    if (strncmp(drink, "SOFT DRINK", 10) == 0) {
        int c = carbon / 7;
        int o = oxygen / 9;
        int h = hydrogen / 14;
        amount = (c < o && c < h) ? c : ((o < h) ? o : h);
    } else if (strncmp(drink, "VODKA", 5) == 0) {
        int c = carbon / 8;
        int o = oxygen / 8;
        int h = hydrogen / 20;
        amount = (c < o && c < h) ? c : ((o < h) ? o : h);
    } else if (strncmp(drink, "CHAMPAGNE", 9) == 0) {
        int c = carbon / 3;
        int o = oxygen / 4;
        int h = hydrogen / 8;
        amount = (c < o && c < h) ? c : ((o < h) ? o : h);
    } else {
        printf("ERROR: Unknown drink type\n");
        return;
    }

    printf("Can generate %d units of %s\n", amount, drink);
}

int create_unix_socket(const char *path, int type) {
    int sock;
    struct sockaddr_un addr;

    // Create socket
    if ((sock = socket(AF_UNIX, type, 0)) < 0) {
        perror("Unix socket creation failed");
        return -1;
    }

    // Remove existing socket file if it exists
    unlink(path);

    // Setup address
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    // Bind socket
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Unix socket bind failed");
        close(sock);
        unlink(path);
        return -1;
    }

    // Listen if it's a stream socket
    if (type == SOCK_STREAM) {
        if (listen(sock, SOMAXCONN) < 0) {
            perror("Unix socket listen failed");
            close(sock);
            unlink(path);
            return -1;
        }
    }

    return sock;
}

int main(int argc, char *argv[]) {
    int tcp_port = -1, udp_port = -1;
    int initial_c = 0, initial_h = 0, initial_o = 0;
    int timeout = 0;
    char *stream_path = NULL, *datagram_path = NULL;

    static struct option long_options[] = {
        {"tcp-port", required_argument, 0, 'T'},
        {"udp-port", required_argument, 0, 'U'},
        {"carbon", required_argument, 0, 'c'},
        {"oxygen", required_argument, 0, 'o'},
        {"hydrogen", required_argument, 0, 'h'},
        {"timeout", required_argument, 0, 't'},
        {"stream-path", required_argument, 0, 's'},
        {"datagram-path", required_argument, 0, 'd'},
        {0, 0, 0, 0}
    };

    int opt;
    
    while ((opt = getopt_long(argc, argv, ":T:U:c:o:h:t:s:d:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'T': tcp_port = atoi(optarg); break;
            case 'U': udp_port = atoi(optarg); break;
            case 'c': initial_c = atoi(optarg); break;
            case 'h': initial_h = atoi(optarg); break;
            case 'o': initial_o = atoi(optarg); break;
            case 't': timeout = atoi(optarg); break;
            case 's': stream_path = optarg; break;
            case 'd': datagram_path = optarg; break;
            case '?': 
                printf("Unknown option: %c\n", optopt); 
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "Usage: %s -T <tcp_port> -U <udp_port> [options]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (tcp_port < 0 || udp_port < 0) {
        fprintf(stderr, "ERROR: TCP and UDP ports are required.\n");
        exit(EXIT_FAILURE);
    }

    carbon = initial_c;
    hydrogen = initial_h;
    oxygen = initial_o;

    // Socket variables
    int tcp_socket, new_socket, udp_socket;
    int uds_stream_socket = -1, uds_datagram_socket = -1;
    struct sockaddr_in tcp_addr, udp_addr, client_addr;
    struct sockaddr_un uds_client_addr;
    socklen_t addrlen = sizeof(tcp_addr);
    socklen_t client_len = sizeof(client_addr);
    socklen_t uds_client_len = sizeof(uds_client_addr);

    int client_sockets[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
        client_sockets[i] = -1;

    // Create TCP socket
    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP socket failed");
        exit(EXIT_FAILURE);
    }

    int opt_reuse = 1;
    setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &opt_reuse, sizeof(opt_reuse));

    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(tcp_port);

    if (bind(tcp_socket, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("TCP bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(tcp_socket, SOMAXCONN) < 0) {
        perror("TCP listen failed");
        exit(EXIT_FAILURE);
    }

    // Create UDP socket
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(udp_port);

    if (bind(udp_socket, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }

    // Create UDS sockets if paths provided
    if (stream_path) {
        uds_stream_socket = create_unix_socket(stream_path, SOCK_STREAM);
        if (uds_stream_socket < 0) {
            fprintf(stderr, "Failed to create UDS stream socket\n");
            exit(EXIT_FAILURE);
        }
        printf("UDS Stream socket listening on: %s\n", stream_path);
    }

    if (datagram_path) {
        uds_datagram_socket = create_unix_socket(datagram_path, SOCK_DGRAM);
        if (uds_datagram_socket < 0) {
            fprintf(stderr, "Failed to create UDS datagram socket\n");
            exit(EXIT_FAILURE);
        }
        printf("UDS Datagram socket listening on: %s\n", datagram_path);
    }

    printf("Server is listening on TCP port %d and UDP port %d...\n", tcp_port, udp_port);

    fd_set readfds;
    int max_fd;
    time_t last_activity = time(NULL);

    while (1) {
        // Check timeout
        if (timeout > 0) {
            time_t now = time(NULL);
            if (difftime(now, last_activity) >= timeout) {
                printf("Timeout reached with no activity. Exiting.\n");
                break;
            }
        }

        FD_ZERO(&readfds);
        FD_SET(tcp_socket, &readfds);
        FD_SET(udp_socket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        max_fd = (tcp_socket > udp_socket) ? tcp_socket : udp_socket;
        if (STDIN_FILENO > max_fd) max_fd = STDIN_FILENO;

        // Add UDS sockets to select
        if (uds_stream_socket >= 0) {
            FD_SET(uds_stream_socket, &readfds);
            if (uds_stream_socket > max_fd) max_fd = uds_stream_socket;
        }
        if (uds_datagram_socket >= 0) {
            FD_SET(uds_datagram_socket, &readfds);
            if (uds_datagram_socket > max_fd) max_fd = uds_datagram_socket;
        }

        // Add client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
                if (sd > max_fd) max_fd = sd;
            }
        }
        
        struct timeval tv;
        if (timeout > 0) {
            time_t now = time(NULL);
            int time_left = timeout - (int)difftime(now, last_activity);
            if (time_left <= 0) {
                printf("Timeout reached with no activity. Exiting.\n");
                break;
            }
            tv.tv_sec = time_left;
            tv.tv_usec = 0;
        } else {
            tv.tv_sec = 1;
            tv.tv_usec = 0;
        }

        int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            continue;
        }

        if (activity == 0 && timeout > 0) {
            time_t now = time(NULL);
            if (difftime(now, last_activity) >= timeout) {
                printf("Timeout reached with no activity. Exiting.\n");
                break;
            }
            continue;
        }

        // Handle new TCP connection
        if (FD_ISSET(tcp_socket, &readfds)) {
            if ((new_socket = accept(tcp_socket, (struct sockaddr *)&tcp_addr, &addrlen)) >= 0) {
                int added = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_sockets[i] == -1) {
                        client_sockets[i] = new_socket;
                        added = 1;
                        break;
                    }
                }
                if (!added) {
                    printf("Too many clients. Rejecting TCP connection.\n");
                    close(new_socket);
                }
                last_activity = time(NULL);
            }
        }

        // Handle new UDS stream connection
        if (uds_stream_socket >= 0 && FD_ISSET(uds_stream_socket, &readfds)) {
            if ((new_socket = accept(uds_stream_socket, (struct sockaddr *)&uds_client_addr, &uds_client_len)) >= 0) {
                int added = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_sockets[i] == -1) {
                        client_sockets[i] = new_socket;
                        added = 1;
                        break;
                    }
                }
                if (!added) {
                    printf("Too many clients. Rejecting UDS stream connection.\n");
                    close(new_socket);
                }
                last_activity = time(NULL);
            }
        }

        // Handle client messages (both TCP and UDS stream)
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd != -1 && FD_ISSET(sd, &readfds)) {
                char buffer[BUFFER_SIZE] = {0};
                int valread = read(sd, buffer, BUFFER_SIZE - 1);
                if (valread <= 0) {
                    close(sd);
                    client_sockets[i] = -1;
                } else {
                    handle_command_tcp(buffer);
                    last_activity = time(NULL);
                }
            }
        }

        // Handle UDP message
        if (FD_ISSET(udp_socket, &readfds)) {
            char buffer[BUFFER_SIZE] = {0};
            int len = recvfrom(udp_socket, buffer, BUFFER_SIZE - 1, 0, 
                              (struct sockaddr *)&client_addr, &client_len);
            if (len > 0) {
                buffer[len] = '\0';
                int result = handle_command_udp(buffer);
                const char *response;

                if (result == -1) {
                    response = "ERROR: Invalid command";
                } else if (result == 1) {
                    response = "The request was provided";
                } else {
                    response = "Not enough in stock";
                }

                sendto(udp_socket, response, strlen(response), 0, 
                       (struct sockaddr *)&client_addr, client_len);
                last_activity = time(NULL);
            }
        }

        // Handle UDS datagram message
        if (uds_datagram_socket >= 0 && FD_ISSET(uds_datagram_socket, &readfds)) {
            char buffer[BUFFER_SIZE] = {0};
            int len = recvfrom(uds_datagram_socket, buffer, BUFFER_SIZE - 1, 0, 
                              (struct sockaddr *)&uds_client_addr, &uds_client_len);
            if (len > 0) {
                buffer[len] = '\0';
                int result = handle_command_udp(buffer);
                const char *response;

                if (result == -1) {
                    response = "ERROR: Invalid command";
                } else if (result == 1) {
                    response = "The request was provided";
                } else {
                    response = "Not enough in stock";
                }

                sendto(uds_datagram_socket, response, strlen(response), 0, 
                       (struct sockaddr *)&uds_client_addr, uds_client_len);
                last_activity = time(NULL);
            }
        }

        // Handle keyboard input
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char input[BUFFER_SIZE] = {0};
            if (fgets(input, BUFFER_SIZE, stdin) != NULL) {
                input[strcspn(input, "\n")] = '\0';
                handle_console_command(input);
                last_activity = time(NULL);
            }
        }
    }

    // Cleanup
    close(tcp_socket);
    close(udp_socket);
    
    if (uds_stream_socket >= 0) {
        close(uds_stream_socket);
        if (stream_path) unlink(stream_path);
    }
    
    if (uds_datagram_socket >= 0) {
        close(uds_datagram_socket);
        if (datagram_path) unlink(datagram_path);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1) {
            close(client_sockets[i]);
        }
    }

    return 0;
}