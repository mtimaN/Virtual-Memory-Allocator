CC=gcc
CFLAGS=-Wall -Wextra -Werror -std=c99

build: vma.o main.o utils.o
	$(CC) vma.o main.o utils.o -o vma

vma: vma.c
	$(CC) -c vma.c -o vma.o

main: main.c
	$(CC) -c main.c -o main.o

utils: utils.c
	$(CC) -c utils.c -o utils.o

run_vma: vma
	./vma

pack:
	zip -FSr 312CA_MantuMatei_Tema1.zip README Makefile *.c *.h

.PHONY: clean

# Target to clean files created during compilation
clean:
	rm -f *.o vma
