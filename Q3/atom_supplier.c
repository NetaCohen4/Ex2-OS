#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>  
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>



#define BUFFER_SIZE 1024

void usage(const char *progname) {
    fprintf(stderr, "Usage: %s -h <hostname/IP> -p <port>\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    char *hostname = NULL;
    int port = -1;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            hostname = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else {
            usage(argv[0]);
        }
    }

    if (!hostname || port <= 0) {
        usage(argv[0]);
    }

    // Resolve hostname
    struct addrinfo hints, *res, *p;
    int sockfd;
    char buffer[BUFFER_SIZE];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // Use first result
    for (p = res; p != NULL; p = p->ai_next) {
        struct sockaddr_in *addr = (struct sockaddr_in *)p->ai_addr;
        addr->sin_port = htons(port);

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) continue;

        if (connect(sockfd, (struct sockaddr *)addr, sizeof(*addr)) == 0) {
            break;
        }

        close(sockfd);
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to connect to %s:%d\n", hostname, port);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    printf("Connected to %s:%d\n", hostname, port);
    printf("Enter commands (ADD CARBON / OXYGEN / HYDROGEN):\n");

    while (fgets(buffer, BUFFER_SIZE, stdin)) {
        send(sockfd, buffer, strlen(buffer), 0);
    }

    close(sockfd);
    return 0;
}