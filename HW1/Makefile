# all: server client
CC = gcc
CFLAGS = -Wall -Werror

SERVER_SRC = server.c
CLIENT_SRC = client.c

SERVER_TARGET = server
CLIENT_TARGET = client

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_SRC)

$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SRC)


clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)