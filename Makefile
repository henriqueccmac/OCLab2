CC = gcc
CFLAGS=-Wall -Wextra
TARGET=SimpleCache
L1TARGET=L1Cache
L2TARGET=L2Cache

all:
	$(CC) $(CFLAGS) SimpleProgram.c SimpleCache.c -o $(TARGET)

l1:
	$(CC) $(CFLAGS) L1Program.c L1Cache.c -o $(L1TARGET)

l2:
	$(CC) $(CFLAGS) L2Program.c L2Cache.c -o $(L2TARGET)	

clean:
	rm $(TARGET)
