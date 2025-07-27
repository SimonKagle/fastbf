CC=clang
CFLAGS=-Wall -Werror -O3 -gdwarf-4

.PHONY: all clean d

all: bf.o bf_utils.o
	$(CC) $(CFLAGS) $^ -o bf

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf *.o bf