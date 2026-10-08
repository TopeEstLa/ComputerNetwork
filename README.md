# Multi-Threaded Concurrent Chat System

A concurrent, multi-user chat server and client implemented in C using BSD Sockets and POSIX Threads (`pthreads`).

This repository contains the implementation for Assignment 1 (Socket Programming) including the bonus application-layer command parser features.

---

## Project Structure

- `chat_client.c`: Chat client supporting simultaneous sending and receiving via dual POSIX threads.
- `chat_server.c`: Multi-threaded TCP chat server with mutex-protected shared workspace and broadcast functionality.
- `chat_server_extra.c`: Enhanced chat server featuring an application-layer command parser (`/list` and `/whisper`).
- `Makefile`: Build script compiling all targets with `gcc -Wall -Wextra -std=c11 -pthread`.
- `README.md`: System documentation, design decisions, and execution guide.

---

## System Architecture & Implementation

### 1. Chat Client (`chat_client.c`)

The client provides full-duplex communication through two concurrent timelines:

- **Socket Setup & Connection (Blank #1):**
  - Creates a standard TCP socket using `socket(AF_INET, SOCK_STREAM, 0)`.
  - Configures the server address struct with `htons(port)` and `inet_pton(AF_INET, server_ip, ...)`.
  - Establishes connection using `connect()`.

- **Asynchronous Reader Thread (Blank #2):**
  - Spawns a background POSIX thread running `receive_handler()`, passing the socket descriptor by value `(void*)(long)client_fd` to prevent race conditions.
  - Immediately detaches the thread with `pthread_detach()` for automatic resource reclamation upon termination.
  - The reader thread blocks on `recv()`. When incoming data arrives, it uses ANSI escape sequences (`\r\33[2K\r`) to wipe the prompt line, prints the incoming message, and restores the prompt (`> `).

- **Main Thread Input Loop (Blank #3):**
  - Sends a registration message (`/register <nickname>`) upon initial connection.
  - Reads lines from standard input (`fgets`).
  - Strips trailing newline and checks for the `/exit` termination command.
  - Formats user input as `[Nickname]: Message\n` and sends it to the server using `send()`.
  - On `/exit` or EOF, closes `client_fd` and terminates cleanly.

---

### 2. Standard Chat Server (`chat_server.c`)

The server manages concurrent client connections and broadcasts chat messages across channels:

- **Connection Listener & Worker Thread Allocation (Blank #1):**
  - Binds to the specified port with `SO_REUSEADDR` to avoid `EADDRINUSE` on fast restarts.
  - In the listener loop (`accept()`), acquires `server.mutex` and checks if `server.count < MAX_CLIENTS` (capacity: 32).
  - Finds the first available slot in `server.client_fds` (where entry is 0), registers the new descriptor, and increments `server.count`.
  - Spawns a detached worker thread executing `client_handler()` for each client socket and immediately returns to listening.

- **Global Broadcast (Blank #2):**
  - When a message is received from a client, `broadcast_message()` acquires `server.mutex`.
  - Loops over all registered file descriptors in `server.client_fds`.
  - Forwards the message to every active socket, strictly skipping the sender socket descriptor to prevent echo.
  - Filters out internal registration messages so they are not echoed as chatter.

- **Client Deregistration (Blank #3):**
  - When `recv()` returns 0 (client disconnect) or an error occurs, `remove_client()` acquires `server.mutex`.
  - Locates the client's socket descriptor in `server.client_fds`, resets the slot to 0, and decrements `server.count`.

---

### 3. Enhanced Chat Server with Bonus Features (`chat_server_extra.c`)

Implements an application-layer command parser supporting active nickname management, user discovery, and direct private messaging:

- **Client Registry (`client_entry_t`):**
  - Tracks both socket file descriptors and associated user nicknames:
    ```c
    typedef struct {
        int fd;
        char nickname[NICK_SIZE];
    } client_entry_t;
    ```
  - Thread-safe updates protected by `server.mutex`.

- **Nickname Registration:**
  - Extracts and binds nicknames upon `/register <nick>` or from formatted `[Nickname]: ...` incoming payloads.
  - Logs client registration events to the server console.

- **Command Parser:**
  - `/list`:
    - Iterates over active clients in `server.clients` under mutex protection.
    - Assembles a comma-separated list of all currently connected nicknames.
    - Sends the user list directly and exclusively to the requesting client: `[Server] Connected clients: Alice, Bob, Charlie`.
    - Logs the `/list` query and active client roster to the server terminal.
  - `/whisper <name> <message>`:
    - Parses the target username and whisper payload.
    - Scans `server.clients` under mutex protection for a matching nickname.
    - If found:
      - Routes the message exclusively to the recipient socket: `[Whisper from Alice]: <message>`.
      - Sends confirmation to the sender socket: `[Whisper to Bob]: <message>`.
      - Logs the whisper transfer to the server terminal: `[Whisper] Alice (FD X) -> Bob (FD Y): <message>`.
      - Does not broadcast to any other clients.
    - If recipient is not found:
      - Sends an informative error directly to the sender: `[Server] User '<name>' not found.`.
      - Logs a warning to the server console.

---

## Build Instructions

Compile all binaries (`chat_client`, `chat_server`, and `chat_server_extra`):

```bash
make all
```

Clean build artifacts:

```bash
make clean
```

Or build individually:

```bash
make chat_client
make chat_server
make chat_server_extra
```

---

## Running and Testing

### 1. Testing Standard Server (`chat_server`)

Open separate terminal windows:

**Terminal 1 (Server):**
```bash
./chat_server 8080
```

**Terminal 2 (Client 1):**
```bash
./chat_client 127.0.0.1 8080
# Enter nickname: Alice
```

**Terminal 3 (Client 2):**
```bash
./chat_client 127.0.0.1 8080
# Enter nickname: Bob
```

- When Alice sends `Hello Bob`, Bob sees `[Alice]: Hello Bob`.
- Alice does not receive an echo of her own message.
- Type `/exit` in a client to disconnect cleanly.

---

### 2. Testing Bonus Functions (`chat_server_extra`)

**Terminal 1 (Server):**
```bash
./chat_server_extra 8080
```

**Terminal 2 (Client 1 - Alice):**
```bash
./chat_client 127.0.0.1 8080
# Enter nickname: Alice
```

**Terminal 3 (Client 2 - Bob):**
```bash
./chat_client 127.0.0.1 8080
# Enter nickname: Bob
```

**Terminal 4 (Client 3 - Charlie):**
```bash
./chat_client 127.0.0.1 8080
# Enter nickname: Charlie
```

#### Test `/list`:
In Alice's terminal, type:
```
/list
```
- Alice receives: `[Server] Connected clients: Alice, Bob, Charlie`
- Server terminal displays:
  ```
  [Command] /list executed by 'Alice' (FD 4). Connected clients: Alice, Bob, Charlie
  ```
- Bob and Charlie receive nothing.

#### Test `/whisper`:
In Alice's terminal, type:
```
/whisper Bob Hello Bob, this is a private message!
```
- Bob receives:
  ```
  [Whisper from Alice]: Hello Bob, this is a private message!
  ```
- Alice receives:
  ```
  [Whisper to Bob]: Hello Bob, this is a private message!
  ```
- Server terminal displays:
  ```
  [Whisper] Alice (FD 4) -> Bob (FD 5): Hello Bob, this is a private message!
  ```
- Charlie receives nothing.