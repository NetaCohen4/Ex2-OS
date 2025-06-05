#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>  
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define BUFFER_SIZE 1024

void usage(const char *progname) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s -h <hostname> -p <port>        (for TCP/IP)\n", progname);
    fprintf(stderr, "  %s -f <stream_path>         (for UNIX socket)\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    char *hostname = NULL;
    int port = -1;
    char *stream_path = NULL;
    int sockfd = -1;
    char buffer[BUFFER_SIZE];

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            hostname = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            stream_path = argv[++i];
        } else {
            usage(argv[0]);
        }
    }

    // Validate args: must choose either -h/-p or -f
    if ((hostname && stream_path) || (port > 0 && stream_path) ||
        (!hostname && port <= 0 && !stream_path)) {
        usage(argv[0]);
    }

    // UNIX 
    if (stream_path) {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, stream_path, sizeof(addr.sun_path) - 1);

        sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd == -1) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
            perror("connect");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("Connected via UNIX socket: %s\n", stream_path);


    } else { // TCP
        
        struct addrinfo hints, *res, *p;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        char port_str[10];
        snprintf(port_str, sizeof(port_str), "%d", port);

        if (getaddrinfo(hostname, port_str, &hints, &res) != 0) {
            perror("getaddrinfo");
            exit(EXIT_FAILURE);
        }

        for (p = res; p != NULL; p = p->ai_next) {
            sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sockfd < 0) continue;

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0) break;

            close(sockfd);
        }

        if (p == NULL) {
            fprintf(stderr, "Failed to connect to %s:%d\n", hostname, port);
            freeaddrinfo(res);
            exit(EXIT_FAILURE);
        }

        freeaddrinfo(res);
        printf("Connected to %s:%d\n", hostname, port);
    }

    printf("Enter commands (ADD CARBON / OXYGEN / HYDROGEN):\n");

    while (fgets(buffer, BUFFER_SIZE, stdin)) {
        if (send(sockfd, buffer, strlen(buffer), 0) == -1) {
            perror("send");
            break;
        }
    }

    close(sockfd);
    return 0;
}
