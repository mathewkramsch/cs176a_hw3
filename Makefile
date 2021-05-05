# Makefile

CC = gcc
ARGS = -Wall

PingClient: PingClient.c
	$(CC) $(ARGS) $^ -o $@
clean:
	rm -f PingClient
