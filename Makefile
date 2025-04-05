CC=clang
CFLAGS=-Wall -Werror -O3

.PHONY: all clean

all: bf.o
	$(CC) $(CFLAGS) $^ -o bf

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf *.o bf