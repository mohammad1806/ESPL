all: multi

multi: multi.o
	gcc -m32 -o multi multi.o

multi.o: multi.s
	nasm -f elf32 multi.s -o multi.o

clean:
	rm -f *.o multi
# all: multi

# multi: multi.o rand.o
# 	gcc -m32 -o multi multi.o rand.o

# multi.o: multi.s
# 	nasm -f elf32 multi.s -o multi.o

# rand.o: rand.s
# 	nasm -f elf32 rand.s -o rand.o

# clean:
# 	rm -f *.o multi