/*
מה נשאר לעשות:
1. לטפל ב mutex
2. לעבור על כל הסעיפים של השאלה ולוודא שהכל עובד
3. code coverage
4. להוסיף הערות 

*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
#define MAX_ATOMS 1000000000000000000ULL // 10^18


typedef struct {
    unsigned long long carbon;
    unsigned long long oxygen;
    unsigned long long hydrogen;
    pthread_mutex_t mutex;
} AtomInventory;

// Global variables
AtomInventory *inventory = NULL;
int tcp_port = -1, udp_port = -1;
char *uds_stream_path = NULL;
char *uds_dgram_path = NULL;
char *save_file_path = NULL;
int timeout_seconds = 0;
int save_fd = -1;
int use_file_storage = 0;

// פונקציות עזר למלאי
void print_inventory() {
    printf("Current inventory: Carbon=%llu, Oxygen=%llu, Hydrogen=%llu\n", 
           inventory->carbon, inventory->oxygen, inventory->hydrogen);
}

void lock_inventory() {
    if (use_file_storage) {
        pthread_mutex_lock(&inventory->mutex);
    }
}

void unlock_inventory() {
    if (use_file_storage) {
        pthread_mutex_unlock(&inventory->mutex);
    }
}

void do_work() {
    int rc = pthread_mutex_trylock(&inventory->mutex);
    if (rc == EOWNERDEAD) {
        pthread_mutex_consistent(&inventory->mutex);
        pthread_mutex_unlock(&inventory->mutex);
    }
}

// Write from RAM to DISK (sync)
void sync_to_file() {
    if (!use_file_storage || save_fd == -1) return;
    
    // מסנכרן מהזיכרון לקובץ
    if (msync(inventory, sizeof(AtomInventory), MS_SYNC) == -1) {
        perror("msync");
    }
}

int setup_file_storage() {
    if (!save_file_path) return 0;
    
    use_file_storage = 1;
    
    // בדיקה אם הקובץ קיים
    struct stat st;
    int file_exists = (stat(save_file_path, &st) == 0);
    
    // פתיחת הקובץ
    save_fd = open(save_file_path, O_RDWR | O_CREAT, 0666);
    if (save_fd == -1) {
        perror("open save file");
        return -1;
    }
    
    if (!file_exists) {
        // קובץ חדש - יצירת מבנה ריק
        AtomInventory empty_inventory = {inventory->carbon, inventory->oxygen, inventory->hydrogen, PTHREAD_MUTEX_INITIALIZER};
        if (write(save_fd, &empty_inventory, sizeof(AtomInventory)) != sizeof(AtomInventory)) {
            perror("write to save file");
            close(save_fd);
            return -1;
        }
    }
    
    // הגדלת הקובץ לגודל הנדרש
    if (ftruncate(save_fd, sizeof(AtomInventory)) == -1) {
        perror("ftruncate");
        close(save_fd);
        return -1;
    }
    
    // מיפוי הקובץ לזיכרון
    inventory = mmap(NULL, sizeof(AtomInventory), PROT_READ | PROT_WRITE, 
                    MAP_SHARED, save_fd, 0);
    if (inventory == MAP_FAILED) {
        perror("mmap");
        close(save_fd);
        return -1;
    }
    
    // אם קובץ חדש, אתחול mutex
    if (!file_exists) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);   // תמיכה ב-EOWNERDEAD
        pthread_mutex_init(&inventory->mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    sync_to_file();
    
    return 0;
}

int add_atoms(const char *type, unsigned long long amount) {
    do_work();
    lock_inventory();
    int result = 0;
    if (strcmp(type, "CARBON") == 0) {
        if (inventory->carbon + amount <= MAX_ATOMS) {
            inventory->carbon += amount;
            result = 1;
        }
    } else if (strcmp(type, "OXYGEN") == 0) {
        if (inventory->oxygen + amount <= MAX_ATOMS) {
            inventory->oxygen += amount;
            result = 1;
        }
    } else if (strcmp(type, "HYDROGEN") == 0) {
        if (inventory->hydrogen + amount <= MAX_ATOMS) {
            inventory->hydrogen += amount;
            result = 1;
        }
    }
    
    if (result) {
        sync_to_file();
    }

    unlock_inventory();
    return result;
}

int can_deliver_molecules(const char *molecule, unsigned long long amount) {
    do_work();
    lock_inventory();
    
    int result = 0;
    if (strcmp(molecule, "WATER") == 0) {
        result = (inventory->hydrogen >= 2 * amount && inventory->oxygen >= amount);
    } else if (strcmp(molecule, "CARBON DIOXIDE") == 0) {
        result = (inventory->carbon >= amount && inventory->oxygen >= 2 * amount);
    } else if (strcmp(molecule, "GLUCOSE") == 0) {
        result = (inventory->carbon >= 6 * amount && 
                inventory->hydrogen >= 12 * amount && 
                inventory->oxygen >= 6 * amount);
    } else if (strcmp(molecule, "ALCOHOL") == 0) {
        result = (inventory->carbon >= 2 * amount && 
                inventory->hydrogen >= 6 * amount && 
                inventory->oxygen >= amount);
    }
    
    unlock_inventory();
    return result;
}

void deliver_molecules(const char *molecule, unsigned long long amount) {
    do_work();
    lock_inventory();
    
    if (strcmp(molecule, "WATER") == 0) {
        inventory->hydrogen -= 2 * amount;
        inventory->oxygen -= amount;
    } else if (strcmp(molecule, "CARBON_DIOXIDE") == 0) {
        inventory->carbon -= amount;
        inventory->oxygen -= 2 * amount;
    } else if (strcmp(molecule, "GLUCOSE") == 0) {
        inventory->carbon -= 6 * amount;
        inventory->hydrogen -= 12 * amount;
        inventory->oxygen -= 6 * amount;
    } else if (strcmp(molecule, "ALCOHOL") == 0) {
        inventory->carbon -= 2 * amount;
        inventory->hydrogen -= 6 * amount;
        inventory->oxygen -= amount;
    }
    
    sync_to_file();
    unlock_inventory();
}


void handle_console_command(const char *drink) {
    do_work();
    lock_inventory();
    int amount = 0;

    if (strncmp(drink, "SOFT DRINK", 10) == 0) {
        // water + co2 + glucose
        // CARBON 7 OXYGEN 9 HYDROGEN 14
        int c = inventory->carbon / 7;
        int o = inventory->oxygen / 9;
        int h = inventory->hydrogen / 14;
        amount = (c < o && c < h) ? c : ((o < h) ? o : h);
    } else if (strncmp(drink, "VODKA", 5) == 0) {
        // water + alcohol + glucose
        // CARBON 8 OXYGEN 8 HYDROGEN 20
        int c = inventory->carbon / 8;
        int o = inventory->oxygen / 8;
        int h = inventory->hydrogen / 20;
        amount = (c < o && c < h) ? c : ((o < h) ? o : h);
    } else if (strncmp(drink, "CHAMPAGNE", 9) == 0) {
        // water + co2 + alcohol
        // CARBON 3 OXYGEN 4 HYDROGEN 8
        int c = inventory->carbon / 3;
        int o = inventory->oxygen / 4;
        int h = inventory->hydrogen / 8;
        amount = (c < o && c < h) ? c : ((o < h) ? o : h);
    } else {
        printf("ERROR: Unknown drink type\n");
        return;
    }

    printf("Can generate %d units of %s\n", amount, drink);

    sync_to_file();
    unlock_inventory();
}

// יצירת סוקטים - ללא שינוי
int setup_tcp_socket(int port) {
    int sockfd;
    struct sockaddr_in addr;
    int opt = 1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP socket");
        return -1;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt TCP");
        close(sockfd);
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind TCP");
        close(sockfd);
        return -1;
    }
    if (listen(sockfd, 10) < 0) {
        perror("listen TCP");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int setup_udp_socket(int port) {
    int sockfd;
    struct sockaddr_in addr;
    int opt = 1;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket");
        return -1;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt UDP");
        close(sockfd);
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind UDP");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int setup_uds_stream_socket(const char *path) {
    int sockfd;
    struct sockaddr_un addr;

    unlink(path);
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("UDS stream socket");
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind UDS stream");
        close(sockfd);
        return -1;
    }
    if (listen(sockfd, 10) < 0) {
        perror("listen UDS stream");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int setup_uds_dgram_socket(const char *path) {
    int sockfd;
    struct sockaddr_un addr;

    unlink(path);
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("UDS dgram socket");
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind UDS dgram");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

// טיפול בפקודות TCP/UDS Stream
int handle_stream_client(int client_fd, int client_fds[]) {
    char buffer[BUFFER_SIZE];
    ssize_t nbytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (nbytes <= 0) {
        if (nbytes != 0) {
            perror("recv stream client");
        }
        close(client_fd);
        // מסירה מהמערך
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] == client_fd) {
                client_fds[i] = -1;
                break;
            }
        }
        return -1;
    }

    buffer[nbytes] = '\0';
    if (buffer[nbytes-1] == '\n') buffer[nbytes-1] = '\0';

    char atom_type[64];
    unsigned long long amount;
    
    if (sscanf(buffer, "ADD %s %llu", atom_type, &amount) == 2) {
        if (add_atoms(atom_type, amount)) {
            print_inventory();
        } else {
            const char *response = "ERROR: Invalid atom type or amount too large\n";
            send(client_fd, response, strlen(response), 0);
        }
    } else {
        const char *response = "ERROR: Invalid command\n";
        send(client_fd, response, strlen(response), 0);
    }
    
    return 0;
}

// טיפול בפקודות UDP/UDS Datagram
void handle_dgram_socket(int dgram_fd) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    ssize_t nbytes = recvfrom(dgram_fd, buffer, sizeof(buffer) - 1, 0,
                              (struct sockaddr*)&client_addr, &addr_len);
    if (nbytes < 0) {
        perror("recvfrom dgram");
        return;
    }

    buffer[nbytes] = '\0';
    if (buffer[nbytes-1] == '\n') buffer[nbytes-1] = '\0';
    
    char molecule_type[64];
    unsigned long long amount;
    
    if (sscanf(buffer, "DELIVER %s %llu", molecule_type, &amount) == 2) {
        if (can_deliver_molecules(molecule_type, amount)) {
            deliver_molecules(molecule_type, amount);
            const char *response = "YES\n";
            sendto(dgram_fd, response, strlen(response), 0,
                   (struct sockaddr*)&client_addr, addr_len);
            print_inventory();
        } else {
            const char *response = "NO\n";
            sendto(dgram_fd, response, strlen(response), 0,
                   (struct sockaddr*)&client_addr, addr_len);
        }
    } else {
        const char *response = "ERROR: Invalid command\n";
        sendto(dgram_fd, response, strlen(response), 0,
               (struct sockaddr*)&client_addr, addr_len);
    }
}

// טיפול בקלט מהמקלדת
void handle_keyboard_input() {
    char buffer[BUFFER_SIZE];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) return;
    
    buffer[strcspn(buffer, "\n")] = '\0';
    
    if (strcmp(buffer, "GEN SOFT DRINK") == 0) {
        handle_console_command("SOFT_DRINK");
    } else if (strcmp(buffer, "GEN VODKA") == 0) {
        handle_console_command("VODKA");
    } else if (strcmp(buffer, "GEN CHAMPAGNE") == 0) {
        handle_console_command("CHAMPAGNE");
    } else {
        printf("Invalid command. Use: GEN SOFT DRINK, GEN VODKA, or GEN CHAMPAGNE\n");
    }
}

void print_usage(const char *prog_name) {
    printf("Usage: %s -T <tcp_port> -U <udp_port> [options]\n", prog_name);
    printf("Required options:\n");
    printf("  -T, --tcp-port <port>       TCP port\n");
    printf("  -U, --udp-port <port>       UDP port\n");
    printf("Optional options:\n");
    printf("  -c, --carbon <amount>       Initial carbon atoms\n");
    printf("  -o, --oxygen <amount>       Initial oxygen atoms\n");
    printf("  -h, --hydrogen <amount>     Initial hydrogen atoms\n");
    printf("  -t, --timeout <seconds>     Timeout in seconds\n");
    printf("  -s, --stream-path <path>    UDS stream socket path\n");
    printf("  -d, --datagram-path <path>  UDS datagram socket path\n");
    printf("  -f, --save-file <path>      Save file path for inventory\n");
}

void cleanup() {
    if (use_file_storage && inventory != NULL) {
        munmap(inventory, sizeof(AtomInventory));
        inventory = NULL;
    }
    if (save_fd != -1) {
        close(save_fd);
        save_fd = -1;
    }
    if (uds_stream_path) {
        unlink(uds_stream_path);
    }
    if (uds_dgram_path) {
        unlink(uds_dgram_path);
    }
}

int main(int argc, char *argv[]) {
    int opt;
    int tcp_sock = -1, udp_sock = -1, uds_stream_sock = -1, uds_dgram_sock = -1;
    int client_fds[MAX_CLIENTS];
    fd_set read_fds, master_fds;
    int max_fd;
    inventory = malloc(sizeof(AtomInventory));
    if (!inventory) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }


    // אתחול מערך לקוחות
    for (int i = 0; i < MAX_CLIENTS; i++) client_fds[i] = -1;

    static struct option long_options[] = {
        {"tcp-port", required_argument, 0, 'T'},
        {"udp-port", required_argument, 0, 'U'},
        {"carbon", required_argument, 0, 'c'},
        {"oxygen", required_argument, 0, 'o'},
        {"hydrogen", required_argument, 0, 'h'},
        {"timeout", required_argument, 0, 't'},
        {"stream-path", required_argument, 0, 's'},
        {"datagram-path", required_argument, 0, 'd'},
        {"save-file", required_argument, 0, 'f'},
        {"help", no_argument, 0, '?'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "T:U:c:o:h:t:s:d:f:?", long_options, NULL)) != -1) {
        switch (opt) {
            case 'T':
                tcp_port = atoi(optarg);
                break;
            case 'U':
                udp_port = atoi(optarg);
                break;
            case 'c':
                inventory->carbon = strtoull(optarg, NULL, 10);
                break;
            case 'o':
                inventory->oxygen = strtoull(optarg, NULL, 10);
                break;
            case 'h':
                inventory->hydrogen = strtoull(optarg, NULL, 10);
                break;
            case 't':
                timeout_seconds = atoi(optarg);
                break;
            case 's':
                uds_stream_path = optarg;
                break;
            case 'd':
                uds_dgram_path = optarg;
                break;
            case 'f':
                save_file_path = optarg;
                break;
            case '?':
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // בדיקת פרמטרים הכרחיים
    if (tcp_port == -1 || udp_port == -1) {
        fprintf(stderr, "Error: TCP port (-T) and UDP port (-U) are required\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // הגדרת מיפוי קובץ או זיכרון רגיל
    if (save_file_path) {
        if (setup_file_storage() < 0) {
            fprintf(stderr, "Error: Failed to setup file storage\n");
            exit(EXIT_FAILURE);
        }
    }

    // Go over all the sockets and the file and clean what not necessary
    atexit(cleanup);

    // יצירת סוקטים
    tcp_sock = setup_tcp_socket(tcp_port);
    if (tcp_sock < 0) {
        close(tcp_sock);
        exit(EXIT_FAILURE);
    }

    udp_sock = setup_udp_socket(udp_port);
    if (udp_sock < 0) {
        close(udp_sock);
        exit(EXIT_FAILURE);
    }

    if (uds_stream_path) {
        uds_stream_sock = setup_uds_stream_socket(uds_stream_path);
        if (uds_stream_sock < 0) {
            fprintf(stderr, "Warning: Could not create UDS stream socket\n");
        }
    }

    if (uds_dgram_path) {
        uds_dgram_sock = setup_uds_dgram_socket(uds_dgram_path);
        if (uds_dgram_sock < 0) {
            fprintf(stderr, "Warning: Could not create UDS datagram socket\n");
        }
    }

    // הגדרת timeout אם נדרש
    struct timeval timeout;
    struct timeval *timeout_ptr = NULL;
    if (timeout_seconds > 0) {
        timeout.tv_sec = timeout_seconds;
        timeout.tv_usec = 0;
        timeout_ptr = &timeout;
    }

    printf("Server started:\n");
    printf(" - TCP port: %d\n", tcp_port);
    printf(" - UDP port: %d\n", udp_port);
    if (uds_stream_sock >= 0) printf(" - UDS Stream: %s\n", uds_stream_path);
    if (uds_dgram_sock >= 0) printf(" - UDS Datagram: %s\n", uds_dgram_path);
    if (save_file_path) printf(" - Save file: %s\n", save_file_path);
    if (timeout_seconds > 0) printf(" - Timeout: %d seconds\n", timeout_seconds);
    print_inventory();

    // הגדרת fd_set
    FD_ZERO(&master_fds);
    FD_SET(tcp_sock, &master_fds);
    FD_SET(udp_sock, &master_fds);
    FD_SET(STDIN_FILENO, &master_fds);
    
    max_fd = (tcp_sock > udp_sock) ? tcp_sock : udp_sock;
    
    if (uds_stream_sock >= 0) {
        FD_SET(uds_stream_sock, &master_fds);
        if (uds_stream_sock > max_fd) max_fd = uds_stream_sock;
    }
    if (uds_dgram_sock >= 0) {
        FD_SET(uds_dgram_sock, &master_fds);
        if (uds_dgram_sock > max_fd) max_fd = uds_dgram_sock;
    }

    // לולאת השרת הראשית
    while (1) {
        read_fds = master_fds;
        
        // הוספת לקוחות מחוברים לסט
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] != -1) {
                FD_SET(client_fds[i], &read_fds);
                if (client_fds[i] > max_fd) max_fd = client_fds[i];
            }
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, timeout_ptr);
        
        if (activity < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }
        
        if (activity == 0) {
            printf("Timeout reached, shutting down server\n");
            break;
        }

        // קלט מהמקלדת
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            handle_keyboard_input();
        }

        // חיבור TCP חדש
        if (FD_ISSET(tcp_sock, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_fd = accept(tcp_sock, (struct sockaddr*)&client_addr, &addr_len);
            if (new_fd >= 0) {
                int added = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_fds[i] == -1) {
                        client_fds[i] = new_fd;
                        added = 1;
                        break;
                    }
                }
                if (!added) {
                    fprintf(stderr, "Too many clients\n");
                    close(new_fd);
                }
            }
        }

        
        // חיבור UDS Stream חדש
        if (uds_stream_sock >= 0 && FD_ISSET(uds_stream_sock, &read_fds)) {
            struct sockaddr_un client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_fd = accept(uds_stream_sock, (struct sockaddr*)&client_addr, &addr_len);
            if (new_fd >= 0) {
                int added = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_fds[i] == -1) {
                        client_fds[i] = new_fd;
                        added = 1;
                        break;
                    }
                }
                if (!added) {
                    fprintf(stderr, "Too many clients\n");
                    close(new_fd);
                }
            }
        }

        // טיפול בלקוחות stream & TCP קיימים
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] != -1 && FD_ISSET(client_fds[i], &read_fds)) {
                handle_stream_client(client_fds[i], client_fds);
            }
        }

        // טיפול ב-UDP
        if (FD_ISSET(udp_sock, &read_fds)) {
            handle_dgram_socket(udp_sock);
        }

        // טיפול ב-UDS Datagram
        if (uds_dgram_sock >= 0 && FD_ISSET(uds_dgram_sock, &read_fds)) {
            handle_dgram_socket(uds_dgram_sock);
        }
    }

    // ניקוי
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] != -1) close(client_fds[i]);
    }
    
    if (tcp_sock >= 0) close(tcp_sock);
    if (udp_sock >= 0) close(udp_sock);
    if (uds_stream_sock >= 0) close(uds_stream_sock);
    if (uds_dgram_sock >= 0) close(uds_dgram_sock);
    
    if (uds_stream_path) {
        unlink(uds_stream_path);
        free(uds_stream_path);
    }
    if (uds_dgram_path) {
        unlink(uds_dgram_path);
        free(uds_dgram_path);
    }
    free(inventory);

    return 0;
}