CFLAGS=-std=c11 -g -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

gencc: $(OBJS)
	$(CC) -o gencc $(OBJS) $(LDFLAGS) $(CFLAGS)

$(OBJS): gencc.h

test: gencc
	./gencc tests > tmp.s
	gcc -static -o tmp tmp.s
	./tmp

clean:
	rm -f gencc *.o *~ tmp*

.PHONY: test clean