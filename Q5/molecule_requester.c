#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUFFER_SIZE 1024
/*
int connect_unix_socket(const char *path, int type) {
    int sock;
    struct sockaddr_un addr;

    // Create socket
    if ((sock = socket(AF_UNIX, type, 0)) < 0) {
        perror("Unix socket creation failed");
        return -1;
    }

    // Setup address
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    // Connect (for stream sockets)
    if (type == SOCK_STREAM) {
        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("Unix socket connect failed");
            close(sock);
            return -1;
        }
    }

    return sock;
}

int connect_inet_socket(const char *hostname, int port, int type) {
    int sock;
    struct sockaddr_in addr;
    struct hostent *host_entry;

    // Create socket
    if ((sock = socket(AF_INET, type, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Resolve hostname
    host_entry = gethostbyname(hostname);
    if (host_entry == NULL) {
        fprintf(stderr, "Failed to resolve hostname: %s\n", hostname);
        close(sock);
        return -1;
    }

    // Setup address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);

    // Connect (for stream sockets)
    if (type == SOCK_STREAM) {
        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("Connect failed");
            close(sock);
            return -1;
        }
    }

    return sock;
}
*/

int main(int argc, char *argv[]) {
    char *hostname = NULL;
    int port = -1;

    // ניתוח פרמטרים: -h <hostname/IP> -p <port>
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

    // יצירת סוקט UDP
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

    // קלט מהמשתמש
    char buffer[BUFFER_SIZE];
    printf("Enter molecule request (DELIVER WATER / CARBON DIOXIDE / ALCOHOL / GLUCOSE): \n");

    while (fgets(buffer, BUFFER_SIZE, stdin)) {
        // Remove newline character if present
        buffer[strcspn(buffer, "\n")] = '\0';

        // שליחת הפקודה לשרת
        sendto(sockfd, buffer, strlen(buffer), 0,
           (struct sockaddr *)&serv_addr, sizeof(serv_addr));

        // קבלת תגובה מהשרת
        socklen_t serv_len = sizeof(serv_addr);
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr *)&serv_addr, &serv_len);
        if (n < 0) {
            perror("Error receiving from server");
            break;
        }

        buffer[n] = '\0';  // Null-terminate the response
        printf("%s\n", buffer);

    }

    close(sockfd);
    return 0;
}
