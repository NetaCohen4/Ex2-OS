#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    char *hostname = NULL;
    int port = -1;

    // usage
    int opt;
    while ((opt = getopt(argc, argv, "h:p:")) != -1) {
        switch (opt) {
            case 'h': hostname = optarg; break;
            case 'p': port = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -h <host> -p <port>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!hostname || port == -1) {
        fprintf(stderr, "Missing hostname or port.\n");
        exit(EXIT_FAILURE);
    }

    // UDP socket
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "No such host\n");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    char buffer[BUFFER_SIZE];
    printf("Enter molecule request (DELIVER WATER / CARBON DIOXIDE / ALCOHOL / GLUCOSE): \n");

    while (fgets(buffer, BUFFER_SIZE, stdin)) {
        // Remove newline character if present
        buffer[strcspn(buffer, "\n")] = '\0';

        // send to server
        sendto(sockfd, buffer, strlen(buffer), 0,
           (struct sockaddr *)&serv_addr, sizeof(serv_addr));

        // get respond from server
        socklen_t serv_len = sizeof(serv_addr);
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr *)&serv_addr, &serv_len);
        if (n < 0) {
            perror("Error receiving from server");
            break;
        }

        buffer[n] = '\0';  
        printf("%s\n", buffer);

    }

    close(sockfd);
    return 0;
}
