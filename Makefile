CFLAGS=-std=c11 -g -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

gencc: $(OBJS)
	$(CC) -o gencc $(OBJS) $(LDFLAGS) $(CFLAGS)

$(OBJS): gencc.h

test: gencc
	./test.sh

clean:
	rm -f gencc *.o *~ tmp*

.PHONY: test clean