CC = gcc
CFLAGS=-Wall -Wextra
TARGET=SimpleProgram

all:
	$(CC) $(CFLAGS) SimpleProgram.c L1Cache.c -o $(TARGET)

clean:
	rm $(TARGET)
