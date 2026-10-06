#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_CLIENTS 32
#define BUFFER_SIZE 2048

// Global Shared Workspace Structure
typedef struct {
    int client_fds[MAX_CLIENTS];
    int count;
    pthread_mutex_t mutex;
} server_state_t;

// Initialize global instance with static macros
server_state_t server = {
    .count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

// Thread function prototypes
void* client_handler(void* arg);
void broadcast_message(const char* message, int sender_fd);
void remove_client(int client_fd);

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

    // Wipe tracking array clear
    memset(server.client_fds, 0, sizeof(server.client_fds));

    // 1. Setup BSD Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    // Set reuse socket option to avoid "Address already in use" errors on restarts
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // 2. Bind and Listen
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

    printf("[System] Multi-Threaded Chat Server started on port %d.\n", port);
    printf("[System] Waiting for connections...\n");

    // 3. Main Listener Loop
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        /* 
        * ====================================================
        * TODO: BLANK #1 - MAIN LISTENER & THREAD REGISTRATION
        * ====================================================
        * Instructions:
        * 1. If 'server.count < MAX_CLIENTS', find the first open slot (where client_fds[i] == 0),
        *    store 'new_socket' there, and increment 'server.count'. Unlock the mutex immediately after.
        * 2. If the server is full, unlock the mutex, send an error message to the client, and close the socket.
        * 3. For successfully registered sockets, move on to spawn your threads as follows.
        */
        // CRITICAL SECTION: Safe status modification via Mutex
        pthread_mutex_lock(&server.mutex);
        if (server.count < MAX_CLIENTS) {
            // Register socket into an open slot in the workspace array
            // TODO:
            // === STUDENT CODE START ===
            
            // === STUDENT CODE END ===
            pthread_mutex_unlock(&server.mutex);

            // Pass descriptor directly by value to avoid reference race conditions
            pthread_t tid;
            if (pthread_create(&tid, NULL, client_handler, (void*)(long)new_socket) != 0) {
                perror("Failed to create worker thread");
                remove_client(new_socket);
                close(new_socket);
            } else {
                // Detach thread to automatically reap system resources upon close
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

// Handler routine executed by each concurrent worker thread
void* client_handler(void* arg) {
    int client_fd = (int)(long)arg;
    char buffer[BUFFER_SIZE];
    int read_size;

    printf("[System] Client thread spawned for Socket FD: %d\n", client_fd);

    // Continuous receive stream loop for this worker channel
    while ((read_size = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[read_size] = '\0'; // Safeguard string truncation boundary
        
        // Broadcast payload out to alternative clients
        broadcast_message(buffer, client_fd);
    }

    // Out of loop implies connection ended gracefully (0) or collapsed (< 0)
    printf("[System] Client on Socket FD %d disconnected.\n", client_fd);
    
    // Clear registration entry and shut descriptor
    remove_client(client_fd);
    close(client_fd);
    
    return NULL;
}

// Safely broadcast message payload to all other active channels
/* 
 * ====================================================
 * TODO: BLANK #2 - MUTEX-PROTECTED GLOBAL BROADCAST
 * ====================================================
 * Instructions:
 * 1. Lock the global mutex 'server.mutex' before reading from the shared global array.
 * 2. Loop through all indices in 'server.client_fds'.
 * 3. Identify valid socket file descriptors (non-zero) and ensure you DO NOT send the 
 *    message back to the original 'sender_fd'.
 * 4. Call 'send()' to forward the text message payload to the identified target sockets.
 * 5. Safely unlock 'server.mutex' immediately after completing the loop iteration.
 */
void broadcast_message(const char* message, int sender_fd) {
    // CRITICAL SECTION: Must grab lock to read safely from the shared registry array
    pthread_mutex_lock(&server.mutex);
    // Iterate through all registered client sockets and send the message to each, excluding the sender
    // TODO:
    // === STUDENT CODE START === 
    
    // === STUDENT CODE END ===
    pthread_mutex_unlock(&server.mutex);
}

/* 
 * ====================================================
 * TODO: BLANK #3 - MUTEX-PROTECTED CLIENT DEREGISTRATION
 * ====================================================
 * Instructions:
 * 1. [DONE] Lock the global mutex 'server.mutex' before modifying the shared global array.
 * 2. Loop through all indices in 'server.client_fds' to find the matching 'client_fd'.
 * 3. Set the matching entry to 0 (indicating the slot is now free) and decrement 'server.count'.
 * 4. [DONE] Safely unlock 'server.mutex' immediately after completing the deregistration. 
 */
// Safely clear out client entry from global workspace tracker matrix
void remove_client(int client_fd) {
    pthread_mutex_lock(&server.mutex);
    // Find the client_fd in the array and clear it    
    // TODO:
    // === STUDENT CODE START === 
    
    // === STUDENT CODE END ===
    pthread_mutex_unlock(&server.mutex);
}
