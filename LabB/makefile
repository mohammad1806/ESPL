all: Bubblesort hexaPrint virusDetector

Bubblesort: Bubblesort.o
	gcc -m32 -g -Wall -o Bubblesort Bubblesort.o -lm

hexaPrint: hexaPrint.o
	gcc -m32 -g -Wall -o hexaPrint hexaPrint.o -lm

virusDetector: virusDetector.o
	gcc -m32 -g -Wall -o virusDetector virusDetector.o -lm

Bubblesort.o: Bubblesort.c
	gcc -m32 -g -Wall -c -o Bubblesort.o Bubblesort.c

hexaPrint.o: hexaPrint.c
	gcc -m32 -g -Wall -c -o hexaPrint.o hexaPrint.c

virusDetector.o: virusDetector.c
	gcc -m32 -g -Wall -c -o virusDetector.o virusDetector.c

clean:
	rm -f *.o Bubblesort hexaPrint virusDetector
