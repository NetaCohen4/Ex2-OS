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

void usage(const char *prog) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s -h <host> -p <port>       (UDP IPv4)\n", prog);
    fprintf(stderr, "  %s -f <uds_path>             (UDS Datagram)\n", prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    char *hostname = NULL;
    int port = -1;
    char *uds_path = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "h:p:f:")) != -1) {
        switch (opt) {
            case 'h': hostname = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 'f': uds_path = optarg; break;
            default: usage(argv[0]);
        }
    }

    // וידוא פרמטרים תקינים: או uds_path או host+port, לא שניהם
    if ((uds_path && (hostname || port != -1)) || (!uds_path && (!hostname || port == -1))) {
        usage(argv[0]);
    }

    int sockfd;
    struct sockaddr_in serv_addr_in;
    struct sockaddr_un serv_addr_un;
    struct sockaddr *serv_addr;
    socklen_t serv_addr_len;

    if (uds_path) {
        // יצירת סוקט UDS Datagram
        sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_un client_addr_un;
        memset(&client_addr_un, 0, sizeof(client_addr_un));
        client_addr_un.sun_family = AF_UNIX;

        // Create a unique temporary client socket path
        snprintf(client_addr_un.sun_path, sizeof(client_addr_un.sun_path), "/tmp/client_%d.sock", getpid());

        // Unlink in case it already exists
        unlink(client_addr_un.sun_path);

        // Bind the client socket to the local address
        if (bind(sockfd, (struct sockaddr *)&client_addr_un, sizeof(client_addr_un)) < 0) {
            perror("bind");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Prepare server address
        memset(&serv_addr_un, 0, sizeof(serv_addr_un));
        serv_addr_un.sun_family = AF_UNIX;
        strncpy(serv_addr_un.sun_path, uds_path, sizeof(serv_addr_un.sun_path) - 1);

        serv_addr = (struct sockaddr *)&serv_addr_un;
        serv_addr_len = sizeof(serv_addr_un);


        serv_addr = (struct sockaddr *)&serv_addr_un;
        serv_addr_len = sizeof(serv_addr_un);
    } else {
        // יצירת סוקט UDP IPv4
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        struct hostent *server = gethostbyname(hostname);
        if (!server) {
            fprintf(stderr, "Error: no such host %s\n", hostname);
            exit(EXIT_FAILURE);
        }

        memset(&serv_addr_in, 0, sizeof(serv_addr_in));
        serv_addr_in.sin_family = AF_INET;
        serv_addr_in.sin_port = htons(port);
        memcpy(&serv_addr_in.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

        serv_addr = (struct sockaddr *)&serv_addr_in;
        serv_addr_len = sizeof(serv_addr_in);
    }

    char buffer[BUFFER_SIZE];
    printf("Enter molecule request (DELIVER WATER / CARBON DIOXIDE / ALCOHOL / GLUCOSE): \n");

    while (fgets(buffer, BUFFER_SIZE, stdin)) {
        // הסרת תו שורה חדשה
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strlen(buffer) == 0) break; // יציאה אם שורה ריקה

        // שליחת הפקודה לשרת
        ssize_t sent = sendto(sockfd, buffer, strlen(buffer), 0, serv_addr, serv_addr_len);
        if (sent < 0) {
            perror("sendto");
            break;
        }

        // קבלת תגובה מהשרת
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, NULL, NULL);
        if (n < 0) {
            perror("recvfrom");
            break;
        }

        buffer[n] = '\0';  // סיום מחרוזת
        printf("Server response: %s", buffer);
    }

    // if (uds_path) {
    //     unlink(client_addr_un.sun_path);
    // }

    close(sockfd);

    return 0;
}