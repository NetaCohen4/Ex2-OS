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

    // יצירת חיבור TCP
    struct sockaddr_in serv_addr;
    struct hostent *server = gethostbyname(hostname);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // קלט מהמשתמש
    char buffer[BUFFER_SIZE];
    printf("Enter ADD command (e.g., ADD OXYGEN 10): ");
    fgets(buffer, BUFFER_SIZE, stdin);

    write(sockfd, buffer, strlen(buffer));
    close(sockfd);
    return 0;
}
