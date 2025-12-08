CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
LDFLAGS = -pthread

all: doip_server doip_client

doip_server: doip_server.c
	$(CC) $(CFLAGS) -o doip_server doip_server.c $(LDFLAGS)

doip_client: doip_client.c
	$(CC) $(CFLAGS) -o doip_client doip_client.c $(LDFLAGS)

clean:
	rm -f doip_server doip_client

.PHONY: all clean
