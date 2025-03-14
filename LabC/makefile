all: mypipeline myshell looper

myshell: myshell.o lineParser.o mypipeline.o
	gcc -g -m32 -Wall -o myshell myshell.o lineParser.o 

myshell.o: myshell.c
	gcc -g -m32 -Wall -c myshell.c

lineParser.o: lineParser.c
	gcc -m32 -g -Wall -c -o lineParser.o lineParser.c

mypipeline: mypipeline.o
	gcc -g -m32 -Wall -o mypipeline mypipeline.o

mypipeline.o: mypipeline.c
	gcc -m32 -g -Wall -c -o mypipeline.o mypipeline.c

looper: looper.o
	gcc -g -m32 -Wall -o looper looper.o

looper.o: looper.c
	gcc -m32 -g -Wall -c -o looper.o looper.c

clean:
	rm -f *.o myshell mypipeline looper
