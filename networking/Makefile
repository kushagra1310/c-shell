# LLM
# Makefile for SHAM Protocol Implementation

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = -lcrypto

# Target executables
SERVER = server
CLIENT = client

# Source files
SERVER_SRC = server.c
CLIENT_SRC = client.c
HEADER = sham.h

# Default target
all: $(SERVER) $(CLIENT)

# Server compilation
$(SERVER): $(SERVER_SRC) $(HEADER)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC) $(LDFLAGS)

# Client compilation  
$(CLIENT): $(CLIENT_SRC) $(HEADER)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC) $(LDFLAGS)

# Clean up compiled files
clean:
	rm -f $(SERVER) $(CLIENT) *.o server_log.txt client_log.txt received_file.txt

# Test file transfer (requires a test file)
test: all
	@echo "Creating test file..."
	@echo "This is a test file for SHAM protocol" > test_input.txt
	@echo "Starting server in background..."
	@./$(SERVER) 8080 &
	@sleep 1
	@echo "Starting client..."
	@./$(CLIENT) 127.0.0.1 8080 test_input.txt output.txt
	@echo "Test completed"
	@pkill -f "./$(SERVER)"

# Help target
help:
	@echo "Available targets:"
	@echo "  all     - Build both server and client"
	@echo "  server  - Build only server"
	@echo "  client  - Build only client"
	@echo "  clean   - Remove compiled files and logs"
	@echo "  test    - Run a basic file transfer test"
	@echo "  help    - Show this help message"

# Declare phony targets
.PHONY: all clean test help
# LLM