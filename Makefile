CFLAGS=-std=c11 -g -fno-common

gencc: main.o
	$(CC) -o gencc main.o $(LDFLAGS)

test: gencc
	./test.sh

clean:
	rm -f gencc *.o *~ tmp*

.PHONY: clean