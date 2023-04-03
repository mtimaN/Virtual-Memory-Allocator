#include "vma.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
    arena_t *arena;
    char command[50];
    while (scanf("%s", command)) {
        if (strcmp(command, "ALLOC_ARENA") == 0) {
            uint64_t size;
            scanf("%lu", &size);
            arena = alloc_arena(size);
        } else if (strcmp(command, "ALLOC_BLOCK") == 0) {
            uint64_t address;
            size_t size;
            scanf("%lu%lu", &address, &size);
            alloc_block(arena, address, size);
        } else if (strcmp(command, "FREE_BLOCK") == 0) {
            uint64_t address;
            scanf("%lu", &address);
            free_block(arena, address);
        } else if (strcmp(command, "PMAP") == 0) {
            pmap(arena);
        } else if (strcmp(command, "READ") == 0) {
            uint64_t size, address;
            scanf("%lu%lu", &address, &size);
            read(arena, address, size);
        } else if (strcmp(command, "WRITE") == 0) {
            uint64_t address, size;
            scanf("%lu%lu", &address, &size);
            getc(stdin);
            int8_t *data = calloc(1, size + 2), *buffer = calloc(1, size + 1);
            while(strlen((char*)data) < size) {
                fgets((char*)buffer, size + 1, stdin);
                strcat((char*)data, (char*)buffer);
            }
            data[strlen((char*)data) - 1] = '\0';
            write(arena, address, size, data);
            free(data);
            free(buffer);
        } else if (strcmp(command, "MPROTECT") == 0) {
            int8_t *prot = malloc(10);
            uint64_t address;
            scanf("%lu%s", &address, prot);
            mprotect(arena, address, prot);
            free(prot);
        } else if (strcmp(command, "DEALLOC_ARENA") == 0) {
            break;
        } else printf("Invalid command. Please try again.\n");
    }
    dealloc_arena(arena);
    return 0;
}