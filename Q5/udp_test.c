#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <getopt.h>

#define BUFFER_SIZE 1024

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

// Example client for atom supplier (TCP/UDS Stream)
int atom_supplier_client(char *hostname, int port, char *uds_path) {
    int sock;
    
    if (uds_path) {
        sock = connect_unix_socket(uds_path, SOCK_STREAM);
        printf("Connected via UDS Stream: %s\n", uds_path);
    } else {
        sock = connect_inet_socket(hostname, port, SOCK_STREAM);
        printf("Connected via TCP to %s:%d\n", hostname, port);
    }
    
    if (sock < 0) {
        return -1;
    }

    // Example commands
    const char *commands[] = {
        "ADD CARBON 100\n",
        "ADD OXYGEN 200\n", 
        "ADD HYDROGEN 300\n"
    };

    for (int i = 0; i < 3; i++) {
        send(sock, commands[i], strlen(commands[i]), 0);
        printf("Sent: %s", commands[i]);
        usleep(500000); // Wait 0.5 seconds between commands
    }

    close(sock);
    return 0;
}

// Example client for molecule requester (UDP/UDS Datagram)
int molecule_requester_client(char *hostname, int port, char *uds_path) {
    int sock;
    struct sockaddr_in inet_addr;
    struct sockaddr_un unix_addr;
    struct sockaddr *addr;
    socklen_t addr_len;
    
    if (uds_path) {
        sock = connect_unix_socket(uds_path, SOCK_DGRAM);
        
        memset(&unix_addr, 0, sizeof(unix_addr));
        unix_addr.sun_family = AF_UNIX;
        strncpy(unix_addr.sun_path, uds_path, sizeof(unix_addr.sun_path) - 1);
        
        addr = (struct sockaddr *)&unix_addr;
        addr_len = sizeof(unix_addr);
        printf("Using UDS Datagram: %s\n", uds_path);
    } else {
        sock = connect_inet_socket(hostname, port, SOCK_DGRAM);
        
        struct hostent *host_entry = gethostbyname(hostname);
        if (host_entry == NULL) {
            fprintf(stderr, "Failed to resolve hostname: %s\n", hostname);
            return -1;
        }
        
        memset(&inet_addr, 0, sizeof(inet_addr));
        inet_addr.sin_family = AF_INET;
        inet_addr.sin_port = htons(port);
        memcpy(&inet_addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);
        
        addr = (struct sockaddr *)&inet_addr;
        addr_len = sizeof(inet_addr);
        printf("Using UDP to %s:%d\n", hostname, port);
    }
    
    if (sock < 0) {
        return -1;
    }

    // Example commands
    const char *commands[] = {
        "DELIVER WATER 5",
        "DELIVER CARBON DIOXIDE 3",
        "DELIVER ALCOHOL 2",
        "DELIVER GLUCOSE 1"
    };

    char buffer[BUFFER_SIZE];
    
    for (int i = 0; i < 4; i++) {
        // Send request
        sendto(sock, commands[i], strlen(commands[i]), 0, addr, addr_len);
        printf("Sent: %s\n", commands[i]);
        
        // Receive response
        int len = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, NULL, NULL);
        if (len > 0) {
            buffer[len] = '\0';
            printf("Response: %s\n", buffer);
        }
        
        usleep(500000); // Wait 0.5 seconds between commands
    }

    close(sock);
    return 0;
}

int main(int argc, char *argv[]) {
    char *hostname = NULL;
    int port = -1;
    char *uds_path = NULL;
    
    static struct option long_options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"file", required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "h:p:f:m", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h': hostname = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 'f': uds_path = optarg; break;
            default:
                fprintf(stderr, "Usage: %s [-h hostname -p port] OR [-f uds_path] [-m for molecule client]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Validate arguments
    if (uds_path && (hostname || port != -1)) {
        fprintf(stderr, "ERROR: Cannot specify both UDS path and hostname/port\n");
        exit(EXIT_FAILURE);
    }
    
    if (!uds_path && (!hostname || port == -1)) {
        fprintf(stderr, "ERROR: Must specify either UDS path or hostname+port\n");
        exit(EXIT_FAILURE);
    }

    // Run appropriate client
    return atom_supplier_client(hostname, port, uds_path);
    
}