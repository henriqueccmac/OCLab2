CC = gcc
CFLAGS=-Wall -Wextra
TARGET=SimpleProgram

all:
	$(CC) $(CFLAGS) SimpleProgramteste.c L2Cache.c -o $(TARGET)

clean:
	rm $(TARGET)
