CC = gcc
CFLAGS = -Wall -Wextra -std=c11
THREAD_FLAGS = -pthread

.PHONY: all clean

all: chat_client chat_server chat_server_extra

chat_client: chat_client.c
	$(CC) $(CFLAGS) $(THREAD_FLAGS) -o $@ $<

chat_server: chat_server.c
	$(CC) $(CFLAGS) $(THREAD_FLAGS) -o $@ $<

chat_server_extra: chat_server_extra.c
	$(CC) $(CFLAGS) $(THREAD_FLAGS) -o $@ $<

clean:
	rm -f chat_client chat_server chat_server_extra