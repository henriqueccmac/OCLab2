CC = gcc
CFLAGS=-Wall -Wextra
TARGET=SimpleCache
TARGETL1=L1Cache
TARGETUTIL=util

all:
	$(CC) $(CFLAGS) SimpleProgram.c L1Cache.c -o $(TARGET)
l1:
	$(CC) $(CFLAGS) L1Program.c L1Cache.c -o $(TARGETL1)

clean:
	rm $(TARGET)
