CC = gcc
CFLAGS =
COMMON_FILES = common.c
CLIENT_DIR = client
SERVER_DIR = server

all: $(CLIENT_DIR)/client $(SERVER_DIR)/server

$(CLIENT_DIR)/client: client.c $(COMMON_FILES)
	mkdir -p $(CLIENT_DIR)
	$(CC) $(CFLAGS) -o $@ client.c $(COMMON_FILES)

$(SERVER_DIR)/server: server.c $(COMMON_FILES)
	mkdir -p $(SERVER_DIR)
	$(CC) $(CFLAGS) -o $@ server.c $(COMMON_FILES)

clean:
	rm -rf $(CLIENT_DIR) $(SERVER_DIR)