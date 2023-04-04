CC=gcc
CFLAGS=-g -Wall -Wextra -Werror -std=c99

build: vma.o main.o
	$(CC) vma.o main.o -o vma

vma: vma.c
	$(CC) -c vma.c -o vma.o

main: main.c
	$(CC) -c main.c -o main.o

run_vma: vma
	./vma

.PHONY: clean

# Target to clean files created during compilation
clean:
	rm -f *.o vma
