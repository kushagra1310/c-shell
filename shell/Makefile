

CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Wall -Wextra -Werror -Wno-unused-parameter -fno-asm
SRC = src/part_A1.c src/part_A2.c src/part_A3.c src/part_B1.c src/part_B2.c src/main.c src/vectorlib.c 
OBJ = src/part_A1.o src/part_A2.o src/part_A3.o src/part_B1.o src/part_B2.o src/main.o src/vectorlib.o

TARGET = shell.out

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJ) $(TARGET)
