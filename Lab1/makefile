all: addresses base

addresses: addresses.o 
	gcc -m32 -g -Wall -o addresses addresses.o -lm

addresses.o: addresses.c
	gcc -m32 -g -Wall -c -o addresses.o addresses.c

base: base.o
	gcc -m32 -g -Wall -o base base.o -lm

base.o: base.c
	gcc -m32 -g -Wall -c -o base.o base.c

clean:
	rm -f *.o addresses base