#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_CLIENTS 32
#define BUFFER_SIZE 2048
#define NICK_SIZE 32

typedef struct {
    int fd;
    char nickname[NICK_SIZE];
} client_entry_t;

typedef struct {
    client_entry_t clients[MAX_CLIENTS];
    int count;
    pthread_mutex_t mutex;
} server_state_t;

server_state_t server = {
    .count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

void* client_handler(void* arg);
void broadcast_message(const char* message, int sender_fd);
void remove_client(int client_fd);
void set_client_nickname(int client_fd, const char* nick);
void get_client_nickname(int client_fd, char* dest, size_t max_len);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    memset(server.clients, 0, sizeof(server.clients));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("[System] Multi-Threaded Chat Server (Extra Features) started on port %d.\n", port);
    printf("[System] Waiting for connections...\n");

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&server.mutex);
        if (server.count < MAX_CLIENTS) {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (server.clients[i].fd == 0) {
                    server.clients[i].fd = new_socket;
                    server.clients[i].nickname[0] = '\0';
                    server.count++;
                    break;
                }
            }
            pthread_mutex_unlock(&server.mutex);

            pthread_t tid;
            if (pthread_create(&tid, NULL, client_handler, (void*)(long)new_socket) != 0) {
                perror("Failed to create worker thread");
                remove_client(new_socket);
                close(new_socket);
            } else {
                pthread_detach(tid);
            }
        } else {
            pthread_mutex_unlock(&server.mutex);
            printf("[Warning] Server capacity reached (%d). Rejecting incoming link.\n", MAX_CLIENTS);
            send(new_socket, "[System] Server full. Connection rejected.\n", 42, 0);
            close(new_socket);
        }
    }

    close(server_fd);
    return EXIT_SUCCESS;
}

void set_client_nickname(int client_fd, const char* nick) {
    pthread_mutex_lock(&server.mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd == client_fd) {
            strncpy(server.clients[i].nickname, nick, NICK_SIZE - 1);
            server.clients[i].nickname[NICK_SIZE - 1] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&server.mutex);
}

void get_client_nickname(int client_fd, char* dest, size_t max_len) {
    dest[0] = '\0';
    pthread_mutex_lock(&server.mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd == client_fd) {
            strncpy(dest, server.clients[i].nickname, max_len - 1);
            dest[max_len - 1] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&server.mutex);
}

void remove_client(int client_fd) {
    char nick[NICK_SIZE] = "";
    pthread_mutex_lock(&server.mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd == client_fd) {
            strncpy(nick, server.clients[i].nickname, sizeof(nick) - 1);
            server.clients[i].fd = 0;
            server.clients[i].nickname[0] = '\0';
            server.count--;
            break;
        }
    }
    pthread_mutex_unlock(&server.mutex);

    if (strlen(nick) > 0) {
        printf("[System] Client '%s' (FD %d) disconnected.\n", nick, client_fd);
    } else {
        printf("[System] Client on Socket FD %d disconnected.\n", client_fd);
    }
}

void broadcast_message(const char* message, int sender_fd) {
    pthread_mutex_lock(&server.mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd != 0 && server.clients[i].fd != sender_fd) {
            send(server.clients[i].fd, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&server.mutex);
}

void* client_handler(void* arg) {
    int client_fd = (int)(long)arg;
    char buffer[BUFFER_SIZE];
    int read_size;

    printf("[System] Client thread spawned for Socket FD: %d\n", client_fd);

    while ((read_size = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[read_size] = '\0';

        char sender_nick[NICK_SIZE] = "";
        char payload[BUFFER_SIZE] = "";

        if (buffer[0] == '[') {
            char* close_bracket = strchr(buffer, ']');
            if (close_bracket != NULL) {
                int nlen = close_bracket - (buffer + 1);
                if (nlen > 0 && nlen < NICK_SIZE) {
                    strncpy(sender_nick, buffer + 1, nlen);
                    sender_nick[nlen] = '\0';
                    set_client_nickname(client_fd, sender_nick);
                }
                char* colon = strchr(close_bracket, ':');
                if (colon != NULL) {
                    colon++;
                    while (*colon == ' ') colon++;
                    strncpy(payload, colon, sizeof(payload) - 1);
                } else {
                    strncpy(payload, close_bracket + 1, sizeof(payload) - 1);
                }
            } else {
                strncpy(payload, buffer, sizeof(payload) - 1);
            }
        } else {
            strncpy(payload, buffer, sizeof(payload) - 1);
        }

        payload[strcspn(payload, "\r\n")] = '\0';

        if (sender_nick[0] == '\0') {
            get_client_nickname(client_fd, sender_nick, sizeof(sender_nick));
            if (sender_nick[0] == '\0') {
                snprintf(sender_nick, sizeof(sender_nick), "Client_%d", client_fd);
            }
        }

        if (strncmp(payload, "/register", 9) == 0) {
            char* reg_name = payload + 9;
            while (*reg_name == ' ') reg_name++;
            if (strlen(reg_name) > 0) {
                set_client_nickname(client_fd, reg_name);
                strncpy(sender_nick, reg_name, sizeof(sender_nick) - 1);
                printf("[System] Client (FD %d) registered as '%s'.\n", client_fd, reg_name);
            }
            continue;
        }

        if (strcmp(payload, "/list") == 0) {
            char user_list[BUFFER_SIZE] = "";
            int first = 1;

            pthread_mutex_lock(&server.mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (server.clients[i].fd != 0) {
                    if (!first) {
                        strncat(user_list, ", ", sizeof(user_list) - strlen(user_list) - 1);
                    }
                    const char* name = (server.clients[i].nickname[0] != '\0') ? 
                                       server.clients[i].nickname : "Anonymous";
                    strncat(user_list, name, sizeof(user_list) - strlen(user_list) - 1);
                    first = 0;
                }
            }
            pthread_mutex_unlock(&server.mutex);

            printf("[Command] /list executed by '%s' (FD %d). Connected clients: %s\n", 
                   sender_nick, client_fd, user_list);
            fflush(stdout);

            char response[BUFFER_SIZE + 64];
            snprintf(response, sizeof(response), "[Server] Connected clients: %s\n", user_list);
            send(client_fd, response, strlen(response), 0);
            continue;
        }

        if (strncmp(payload, "/whisper", 8) == 0) {
            char* ptr = payload + 8;
            while (*ptr == ' ') ptr++;

            char target_name[NICK_SIZE] = "";
            int idx = 0;
            while (*ptr != '\0' && *ptr != ' ' && idx < NICK_SIZE - 1) {
                target_name[idx++] = *ptr++;
            }
            target_name[idx] = '\0';

            while (*ptr == ' ') ptr++;
            char* whisper_msg = ptr;

            if (strlen(target_name) == 0 || strlen(whisper_msg) == 0) {
                char err[BUFFER_SIZE];
                snprintf(err, sizeof(err), "[Server] Usage: /whisper <name> <message>\n");
                send(client_fd, err, strlen(err), 0);
                continue;
            }

            int target_fd = -1;
            pthread_mutex_lock(&server.mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (server.clients[i].fd != 0 && strcmp(server.clients[i].nickname, target_name) == 0) {
                    target_fd = server.clients[i].fd;
                    break;
                }
            }
            pthread_mutex_unlock(&server.mutex);

            if (target_fd != -1) {
                printf("[Whisper] %s (FD %d) -> %s (FD %d): %s\n", 
                       sender_nick, client_fd, target_name, target_fd, whisper_msg);
                fflush(stdout);

                char whisper_out[BUFFER_SIZE + NICK_SIZE + 64];
                snprintf(whisper_out, sizeof(whisper_out), "[Whisper from %s]: %s\n", sender_nick, whisper_msg);
                send(target_fd, whisper_out, strlen(whisper_out), 0);

                char confirm_out[BUFFER_SIZE + NICK_SIZE + 64];
                snprintf(confirm_out, sizeof(confirm_out), "[Whisper to %s]: %s\n", target_name, whisper_msg);
                send(client_fd, confirm_out, strlen(confirm_out), 0);
            } else {
                printf("[Warning] /whisper failed: user '%s' not found (requested by %s)\n", 
                       target_name, sender_nick);
                fflush(stdout);

                char not_found[BUFFER_SIZE + NICK_SIZE + 64];
                snprintf(not_found, sizeof(not_found), "[Server] User '%s' not found.\n", target_name);
                send(client_fd, not_found, strlen(not_found), 0);
            }
            continue;
        }

        if (buffer[0] == '[') {
            broadcast_message(buffer, client_fd);
        } else {
            char formatted[BUFFER_SIZE + NICK_SIZE + 64];
            snprintf(formatted, sizeof(formatted), "[%s]: %s\n", sender_nick, payload);
            broadcast_message(formatted, client_fd);
        }
    }

    remove_client(client_fd);
    close(client_fd);

    return NULL;
}
