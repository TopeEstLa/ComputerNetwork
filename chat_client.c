#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 2048
#define NICK_SIZE 32

// Global volatile flag to handle a graceful exit across threads
volatile int running = 1;

void* receive_handler(void* arg);
void clear_line_stdout();

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* server_ip = argv[1];
    int port = atoi(argv[2]);

    int client_fd;
    struct sockaddr_in server_addr;
    char nickname[NICK_SIZE];
    char buffer[BUFFER_SIZE];
    char send_buffer[BUFFER_SIZE + NICK_SIZE + 4];

    // 1. Get user nickname before connecting
    printf("Enter your nickname (Max %d chars): ", NICK_SIZE - 1);
    if (fgets(nickname, sizeof(nickname), stdin) == NULL) {
        return EXIT_FAILURE;
    }
    // Remove newline character from nickname
    nickname[strcspn(nickname, "\n")] = '\0';

    if (strlen(nickname) == 0) {
        fprintf(stderr, "Nickname cannot be empty.\n");
        return EXIT_FAILURE;
    }

    /* 
     * ==========================================
     * TODO: BLANK #1 - SOCKET SETUP & CONNECTION
     * ==========================================
     * Instructions:
     * 1. Create a standard BSD TCP socket (AF_INET, SOCK_STREAM).
     * 2. Initialize the 'server_addr' structure using the 'port' and 'server_ip'.
     *    Hint: Use 'inet_pton' to convert the server_ip string to network byte order.
     * 3. Call 'connect' to establish a 3-way handshake with the chat server.
     * 4. On any failure, print an error and return EXIT_FAILURE.
    */
    // === STUDENT CODE START ===


    // === STUDENT CODE END ===

    printf("[System] Connected to server successfully. Happy chatting!\n");

    /* 
     * ==========================================
     * TODO: BLANK #2 - READER THREAD SPRAWL
     * ==========================================
     * Instructions:
     * 1. Launch a background POSIX thread using 'pthread_create'.
     *    - It must execute the routine 'receive_handler'.
     *    - Pass the socket descriptor 'client_fd' safely as the argument.
     * 2. To avoid reference race conditions, pass 'client_fd' by value (cast to void*).
     * 3. Detach the thread immediately using 'pthread_detach' so resources are reaped.
     */
    // === STUDENT CODE START ===


    // === STUDENT CODE END ===

    // 5. Main Thread Input Loop (stdin -> socket)
    printf("> ");
    fflush(stdout);

    /* 
     * ==========================================
     * TODO: BLANK #3 - INPUT TRANSMISSION LOOP
     * ==========================================
     * Instructions:
     * 1. Inside this while loop, read text from standard input using 'fgets'.
     * 2. Remove the trailing newline character from 'buffer'.
     * 3. Check if the user typed "/exit". If so, set 'running = 0' and break the loop.
     * 4. If message is not empty, use 'snprintf' to format the output into 'send_buffer' 
     *    exactly like this: "[Nickname]: Message\n"
     * 5. Call 'send()' to transmit 'send_buffer' over the 'client_fd' socket channel.
     * 6. Don't forget to reprint the prompt "> " and call 'fflush(stdout)' at the end of the loop iteration.
     */
    // === STUDENT CODE START ===
    while (running /* Add your conditions here */) {

    }
    // === STUDENT CODE END ===

    // 6. Graceful Cleanup
    printf("[System] Closing connection...\n");
    close(client_fd);
    return EXIT_SUCCESS;
}

// Background thread loop to receive data from the server
void* receive_handler(void* arg) {
    int socket_fd = (int)(long)arg;
    char buffer[BUFFER_SIZE];
    int read_size;

    while (running && (read_size = recv(socket_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[read_size] = '\0';

        // Clear the current input prompt (> ), print the received message, and reprint prompt
        clear_line_stdout();
        printf("%s", buffer);
        printf("> ");
        fflush(stdout);
    }

    if (running) {
        // If recv returned 0/error but the client didn't intentionally trigger an exit
        clear_line_stdout();
        printf("\n[System] Connection lost or closed by the server.\n");
        running = 0;
        exit(EXIT_FAILURE); 
    }

    return NULL;
}

// Helper to wipe the active shell line so the incoming stream doesn't mangle user's current half-typed input
// This is a simple ANSI escape sequence to clear the line and return the cursor to the start. The ANSI Escape Sequence (\r\33[2K\r):
void clear_line_stdout() {
    printf("\r\33[2K\r");
    fflush(stdout);
}