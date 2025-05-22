#include <arpa/inet.h>
// #include <getopt.h>
#include <fcntl.h>
#include "inventory.h"
// #include "atom_supplier.h"

int tcp_port = -1, udp_port = -1;
int timeout_secs = 0;
unsigned int init_h = 0, init_o = 0, init_c = 0;

void parse_arguments(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"tcp-port", required_argument, 0, 'T'},
        {"udp-port", required_argument, 0, 'U'},
        {"oxygen", required_argument, 0, 'o'},
        {"carbon", required_argument, 0, 'c'},
        {"hydrogen", required_argument, 0, 'h'},
        {"timeout", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, ":T:U:o:c:h:t:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'T': tcp_port = atoi(optarg); break;
            case 'U': udp_port = atoi(optarg); break;
            case 'o': init_o = atoi(optarg); break;
            case 'c': init_c = atoi(optarg); break;
            case 'h': init_h = atoi(optarg); break;
            case 't': timeout_secs = atoi(optarg); break;
            case '?':
                fprintf(stderr, "Unknown option: %c\n", optopt);
                exit(EXIT_FAILURE);
            case ':':
                fprintf(stderr, "Missing argument for option: %c\n", optopt);
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "Usage: %s -T <tcp-port> -U <udp-port> [...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (tcp_port == -1 || udp_port == -1) {
        fprintf(stderr, "Error: TCP and UDP ports are mandatory.\n");
        exit(EXIT_FAILURE);
    }
}

// יצירת סוקט TCP מאזין
int setup_tcp_listener() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(tcp_port), .sin_addr.s_addr = INADDR_ANY };
    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sockfd, 5);
    return sockfd;
}

// יצירת סוקט UDP
int setup_udp_socket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(udp_port), .sin_addr.s_addr = INADDR_ANY };
    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    return sockfd;
}

int main(int argc, char *argv[]) {
    parse_arguments(argc, argv);
    init_inventory(&global_inv, init_h, init_o, init_c);

    int tcp_sock = setup_tcp_listener();
    int udp_sock = setup_udp_socket();

    struct pollfd fds[3];
    fds[0].fd = tcp_sock; fds[0].events = POLLIN;
    fds[1].fd = udp_sock; fds[1].events = POLLIN;
    fds[2].fd = STDIN_FILENO; fds[2].events = POLLIN;

    if (timeout_secs > 0) alarm(timeout_secs);

    while (1) {
        int ret = poll(fds, 3, -1);
        if (ret < 0) continue;

        // TCP
        if (fds[0].revents & POLLIN) {
            int client = accept(tcp_sock, NULL, NULL);
            char buffer[BUFFER_SIZE] = {0};
            read(client, buffer, BUFFER_SIZE);
            handle_tcp_command(buffer);
            close(client);
        }

        // UDP
        if (fds[1].revents & POLLIN) {
            char buffer[BUFFER_SIZE] = {0};
            struct sockaddr_in client_addr;
            socklen_t len = sizeof(client_addr);
            recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &len);
            handle_udp_command(buffer);
        }

        // STDIN
        if (fds[2].revents & POLLIN) {
            char buffer[BUFFER_SIZE] = {0};
            fgets(buffer, BUFFER_SIZE, stdin);
            // כאן אתה יכול לקרוא פקודות כמו GEN VODKA ולממש את ההיגיון
            printf("Console command received: %s\n", buffer);
        }
    }

    return 0;
}
