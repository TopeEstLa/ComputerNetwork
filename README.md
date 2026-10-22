# Assignment 1 - Socket Programming

## 1. Standard Implementation

### chat\_server.c

- **Blank #1**:

Loops through `server.client_fds` to find the first index equal to `0`, assigns `new_socket` to that slot, increments `server.count`, and breaks out of the loop.

- **Blank #2**:


Ignores incoming `/register` messages so they are not broadcast to other clients. Loops through `server.client_fds` and sends the message to every active socket descriptor except the sender (`sender_fd`).

- **Blank #3**:


Finds the index of `client_fd` in `server.client_fds`, resets to `0`, decrements `server.count`, and exit the loop.

### chat\_client.c

- **Blank #1**:


Creates a TCP socket (SOCK_STREAM), sets up the `server_addr` structure using `inet_pton` and `htons`, and connects to the server with `connect()`.

- **Blank #2**:


Create a thread running `receive_handler` using `pthread_create`, passing `client_fd` by value as argument, and calls `pthread_detach` so thread resources are freed automatically.

- **Blank #3**:


Sends `/register <nickname>` right after connecting. In a loop, reads user input with `fgets`, removes trailing newlines, exits if `/exit` is typed, formats messages as `[nickname]: message\n`, sends them with `send()`, and reprints the `>`  prompt.

---

## 2. Extra Features (chat\_server\_extra.c)

- **Nickname Storage (for /list & /whisper)**:


Replaced the `client_fds` array with an array of `client_entry_t` structs holding both the `fd` int and a `nickname` string.

- **`/register <name>`**:


When a client connects he send a registration message, the server extracts the nickname, stores it in the client array under mutex, and prints it on the server terminal.

- **`/list`**:


When a client sends `/list`, the server locks the mutex, loops through all active clients, and builds a list of the nicknames prints it in the list on the server terminal and sends it back only to the client

- **`/whisper <target> <message>`**:


Parses the target user and the message. It searches the client array under mutex for a matching nickname:
  - If found: sends the message directly to the target socket (`[Whisper from <sender>]: ...`), sends a confirmation to the sender (`[Whisper to <target>]: ...`), and logs the whisper on the server terminal.
  - If not found: sends `[Server] User '<target>' not found.` back to the sender and logs a warning on the server terminal.

- **Regular messages**:


If a message is not a command, it is broadcast to all other connected clients as `[nickname]: message`.