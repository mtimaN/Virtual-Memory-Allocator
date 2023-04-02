#include "vma.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void add_miniblock(node_t *curr, const uint64_t address, const uint64_t size)
{
	node_t *new = malloc(sizeof(node_t));
	new->data = malloc(sizeof(miniblock_t));

	// initializing the fields
	((miniblock_t*)new->data)->start_address = address;
	((miniblock_t*)new->data)->size = size;
	((miniblock_t*)new->data)->perm = 6;
	((miniblock_t*)new->data)->rw_buffer = malloc(size);

	// linking the new miniblock
	if (((block_t*)curr->data)->miniblock_list->size == 0) {
		// new block
		((block_t*)curr->data)->miniblock_list->head = new;
		new->next = new;
		new->prev = new;
	} else {
		new->next = ((block_t*)curr->data)->miniblock_list->head;
		new->prev = ((block_t*)curr->data)->miniblock_list->head->prev;
		((block_t*)curr->data)->miniblock_list->head->prev->next = new;
		((block_t*)curr->data)->miniblock_list->head->prev = new;
	}

	// updating the block
	((block_t*)curr->data)->size += size;
	((block_t*)curr->data)->miniblock_list->size++;
}

node_t *add_newblock(node_t *curr, const uint64_t address)
{
	// creating a new block
	node_t *new = malloc(sizeof(node_t));
	new->data = malloc(sizeof(block_t));
	((block_t*)new->data)->start_address = address;
	((block_t*)new->data)->size = 0;
	((block_t*)new->data)->miniblock_list = malloc(sizeof(list_t));
	((block_t*)new->data)->miniblock_list->size = 0;
	((block_t*)new->data)->miniblock_list->head = NULL;
	
	// linking
	if (curr) {
		new->prev = curr;
		new->next = curr->next;
		curr->next = new;
	} else {
		// first node
		new->prev = new;
		new->next = new;
	}
	return new;
}

void combine_blocks(block_t *first, block_t *second)
{
	// combine the blocks
	first->miniblock_list->size +=
	second->miniblock_list->size;
	
	// 2nd block's tail -> 1st block's head
	second->miniblock_list->head->prev->next =
	first->miniblock_list->head;

	// 1st block's tail -> 2nd block's head
	first->miniblock_list->head->prev->next =
	second->miniblock_list->head;

	// 1st block's head -> 2nd block's tail;
	first->miniblock_list->head->prev =
	second->miniblock_list->head->prev;

	// 2nd block's head -> 1st block's tail
	second->miniblock_list->head->prev =
	first->miniblock_list->head->prev;
}

arena_t *alloc_arena(const uint64_t size)
{
    arena_t *arena = malloc(sizeof(arena_t));
	arena->arena_size = size;
	arena->alloc_list = malloc(sizeof(list_t));
	arena->alloc_list->size = 0;
	arena->alloc_list->head = NULL;
	return arena;
}

void dealloc_arena(arena_t *arena)
{
	node_t *curr = arena->alloc_list->head;
	for (uint64_t i = 1; i <= arena->alloc_list->size; ++i) {

		list_t *minilist_ptr = ((block_t*)curr->data)->miniblock_list;
		node_t *mini_curr = minilist_ptr->head;

		// freeing every miniblock
		for (uint64_t j = 1; j <= minilist_ptr->size; ++j) {
			node_t *mini_tmp = mini_curr;
			mini_curr = mini_curr->next;
			free(((miniblock_t*)mini_tmp->data)->rw_buffer);
			free(mini_tmp->data);
			free(mini_tmp);
		}
		node_t *tmp = curr;
		curr = curr->next;
		free(((block_t*)tmp->data)->miniblock_list);
		free(tmp->data);
		free(tmp);
	}
	free(arena->alloc_list);
	free(arena);
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return;
	}
	if (address + size > arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return;
	}
	node_t *curr = arena->alloc_list->head;
	// treating the add_first cases
	if (arena->alloc_list->size == 0) {
		node_t *new = add_newblock(curr, address);
		arena->alloc_list->head = new;
		arena->alloc_list->size++;
		add_miniblock(new, address, size);
		return;
	}
	if (address < ((block_t*)curr->data)->start_address) {
		if (address + size > ((block_t*)curr->data)->start_address) {
			printf("This zone was already allocated.\n");
			return;
		}
		if (curr)
			curr = curr->prev;
		node_t *new = add_newblock(curr, address);
		add_miniblock(new, address, size);
		arena->alloc_list->size++;
		arena->alloc_list->head = new;
		// if new block is adjacent to old head
		if (address + size == ((block_t*)curr->data)->start_address) {
			((block_t*)new->data)->size += ((block_t*)curr->data)->size;
			combine_blocks(((block_t*)new->data), ((block_t*)curr->data));
			free(((block_t*)curr->data)->miniblock_list);
			free(((block_t*)curr->data));
			free(curr);
			arena->alloc_list->size--;
		}
		return;
	}
	// curr will be right before the position where we want to insert
	uint64_t i = 0;
	while (i++ < arena->alloc_list->size - 1 && address > 
	((block_t*)curr->data)->start_address + ((block_t*)curr->data)->size)
		curr = curr->next;

	// if the space between the 2 nodes is too small
	if (address < ((block_t*)curr->data)->start_address + ((block_t*)curr->data)->size) {
		printf("This zone was already allocated.\n");
		return;
	}
	if (i < arena->alloc_list->size - 1 && (address + size > ((block_t*)curr->next->data)->start_address)) {
		printf("This zone was already allocated\n");
		return;
	}
	
	if (address == ((block_t*)curr->data)->start_address + ((block_t*)curr->data)->size) {
		add_miniblock(curr, address, size);
	} else {
		node_t *new = add_newblock(curr, address);
		arena->alloc_list->size++;
		add_miniblock(new, address, size);
	}

	// if two blocks are next to eachother
	if (address + size == ((block_t*)curr->next->data)->start_address) {

		((block_t*)curr->data)->size += ((block_t*)curr->next->data)->size;
		combine_blocks(((block_t*)curr->data), ((block_t*)curr->next->data));
		// free the 2nd block
		free(((block_t*)curr->next->data)->miniblock_list);
		free(((block_t*)curr->next->data));
		arena->alloc_list->size--;

		node_t *tmp = curr->next;
		curr->next->next->prev = curr;
		curr->next = curr->next->next;
		free(tmp);
	}
}

void free_block(arena_t *arena, const uint64_t address)
{
	node_t *curr = arena->alloc_list->head;
	if (!curr) {
		printf("Invalid address for free.\n");
		return;
	}
	uint64_t i = 0;
	while (i++ < arena->alloc_list->size - 1 && address > 
	((block_t*)curr->data)->start_address + ((block_t*)curr->data)->size)
		curr = curr->next;
	list_t *mini_list = ((block_t*)curr->data)->miniblock_list;
	node_t *mini_curr = mini_list->head;
	uint64_t new_size = 0, no_items = 0;
	i = 0;
	while (i++ < mini_list->size - 1 && address >
	((miniblock_t*)mini_curr->data)->start_address) {
		new_size += ((miniblock_t*)mini_curr->data)->size;
		no_items++;
		mini_curr = mini_curr->next;
	}

	// address not found
	if (((miniblock_t*)mini_curr->data)->start_address != address) {
		printf("Invalid address for free.\n");
		return;
	}

	if (mini_curr == mini_list->head) {
		((block_t*)curr->data)->start_address += ((miniblock_t*)mini_curr->data)->size;
		((block_t*)curr->data)->size -= ((miniblock_t*)mini_curr->data)->size;

		// relinking
		mini_list->head = mini_curr->next;
		mini_curr->next->prev = mini_curr->prev;	
		mini_curr->prev->next = mini_curr->next;
		mini_list->size--;
	} else if (mini_curr == mini_list->head->prev) {
		((block_t*)curr->data)->size -= ((miniblock_t*)mini_curr->data)->size;

		// relinking
		mini_curr->next->prev = mini_curr->prev;	
		mini_curr->prev->next = mini_curr->next;
		mini_list->size--;
	} else {

		// splitting into 2 blocks
		block_t *second_block = malloc(sizeof(block_t));
		second_block->start_address =
		((miniblock_t*)mini_curr->data)->start_address +
		((miniblock_t*)mini_curr->data)->size;
		second_block->size = ((block_t*)curr->data)->size - new_size - ((miniblock_t*)mini_curr->data)->size;
		second_block->miniblock_list = malloc(sizeof(list_t));
		second_block->miniblock_list->size = mini_list->size - no_items - 1;
		mini_list->size = no_items;

		((block_t*)curr->data)->size = new_size;
		// linking the 2 lists
		// second list
		mini_list->head->prev->next = mini_curr->next;
		mini_curr->next->prev = mini_list->head->prev;
		second_block->miniblock_list->head = mini_curr->next;
		// first list
		mini_curr->prev->next = mini_list->head;
		mini_list->head->prev = mini_curr->prev;

		node_t *new = malloc(sizeof(node_t*));
		new->data = second_block;
		new->prev = curr;
		new->next = curr->next;
		curr->next->prev = new;
		curr->next = new;
		arena->alloc_list->size++;
	}
	free(((miniblock_t*)mini_curr->data)->rw_buffer);
	free(mini_curr->data);
	free(mini_curr);
	if (mini_list->size == 0) {
		free(mini_list);
		if (curr == arena->alloc_list->head)
			arena->alloc_list->head = curr->next;
		curr->prev->next = curr->next;
		curr->next->prev = curr->prev;
		free(curr->data);
		free(curr);
		arena->alloc_list->size--;
	}
	if (arena->alloc_list->size == 0)
		arena->alloc_list->head = NULL;
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	int8_t *buffer = malloc(size + 1);
	uint64_t new_size = size;
	uint64_t i = 0;
	node_t *curr = arena->alloc_list->head;
	while (i++ < arena->alloc_list->size - 1 && address > 
	((block_t*)curr->data)->start_address)
		curr = curr->next;
	if (address != ((block_t*)curr->data)->start_address) {
		printf("Invalid address for read\n");
		return;
	}
	list_t *mini_list = ((block_t*)curr->data)->miniblock_list;
	node_t *mini_curr = mini_list->head;
	uint32_t check_size = 0;
	i = 0;
	while (i++ < mini_list->size && check_size < size) {
		if (((miniblock_t*)mini_curr->data)->perm < 4) {
			printf("Invalid permissions for read.\n");
			break;
		}
		mini_curr = mini_curr->next;
		check_size += (((miniblock_t*)mini_curr->data)->size);
	}
	if (check_size < size) {
		printf("Warning: size was bigger than the block size. Reading %u characters.\n", check_size);
		new_size = check_size;
	}
	node_t *new = mini_list->head;
	uint64_t read = 0;
	while (new_size) {
		miniblock_t *info = new->data;
		uint64_t chunk = info->size;
		if (info->size > size)
			chunk = size;
		memcpy(buffer + read, info->rw_buffer, chunk);
		read += chunk;
		new_size -= chunk;
		new = new->next;
	}
	buffer[read] = '\0';
	printf("%s\n", buffer);
	free(buffer);
}

void write(arena_t *arena, const uint64_t address, const uint64_t size, int8_t *data)
{
	uint64_t i = 0;
	uint64_t new_size = size;
	node_t *curr = arena->alloc_list->head;
	while (i++ < arena->alloc_list->size - 1 && address > 
	((block_t*)curr->data)->start_address)
		curr = curr->next;

	if (address != ((block_t*)curr->data)->start_address) {
		printf("Invalid address for write\n");
		return;
	}
	list_t *mini_list = ((block_t*)curr->data)->miniblock_list;
	node_t *mini_curr = mini_list->head;
	uint64_t check_size = 0;
	i = 0;
	while (i++ < mini_list->size && check_size < size) {
		if (((miniblock_t*)mini_curr->data)->perm % 4 < 2) {
			printf("Invalid permissions for write.\n");
			return;
		}
		mini_curr = mini_curr->next;
		check_size += (((miniblock_t*)mini_curr->data)->size);
	}
	if (check_size < new_size) {
		printf("Warning: size was bigger than the block size. Writing %lu characters.\n", check_size);
		new_size = check_size;
	}
	uint64_t written = 0;
	mini_curr = mini_list->head;
	while (new_size) {
		miniblock_t *info = mini_curr->data;
		uint64_t chunk = info->size;
		if (info->size > size)
			chunk = size;
		memcpy(info->rw_buffer, data + written, chunk);
		written += chunk;
		new_size -= chunk;
		mini_curr = mini_curr->next;
	}
}

void pmap(const arena_t *arena)
{
	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	node_t *curr = arena->alloc_list->head;
	uint64_t free_mem = arena->arena_size, no_miniblocks = 0;
	for (uint64_t i = 1; i <= arena->alloc_list->size; ++i) {
		free_mem -= ((block_t*)curr->data)->size;
		no_miniblocks += ((block_t*)curr->data)->miniblock_list->size;
		curr = curr->next;
	}
	printf("Free memory: 0x%lX bytes\n", free_mem);
	printf("Number of allocated blocks: %u\n", arena->alloc_list->size);
	printf("Number of allocated miniblocks: %lu\n", no_miniblocks);

	curr = arena->alloc_list->head;
	for (uint64_t i = 1; i <= arena->alloc_list->size; ++i) {
		printf("\n");
		printf("Block %lu begin\n", i);
		uint64_t s_address = ((block_t*)curr->data)->start_address;
		uint64_t e_address = ((block_t*)curr->data)->size + s_address;
		printf("Zone: 0x%lX - 0x%lX\n", s_address, e_address);

		list_t *minilist_ptr = ((block_t*)curr->data)->miniblock_list;
		node_t *mini_curr = minilist_ptr->head;
		for (uint64_t j = 1; j <= minilist_ptr->size; ++j) {
			s_address = ((miniblock_t*)mini_curr->data)->start_address;
			e_address = ((miniblock_t*)mini_curr->data)->size + s_address;
			printf("Miniblock %lu:\t\t0x%lX\t\t-\t\t0x%lX\t\t| ", j, s_address, e_address);

			// printing permissions
			if (((miniblock_t*)mini_curr->data)->perm > 3)
				printf("R");
			else printf("-");
			if (((miniblock_t*)mini_curr->data)->perm  % 4 > 1)
				printf("W");
			else printf("-");
			if (((miniblock_t*)mini_curr->data)->perm % 2 == 1)
				printf("X");
			else printf("-");
			printf("\n");

			mini_curr = mini_curr->next;
		}
		printf("Block %lu end\n", i);
		curr = curr->next;
	}
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
	uint64_t i = 0;
	node_t *curr = arena->alloc_list->head;
	while (i++ < arena->alloc_list->size - 1 && address > 
	((block_t*)curr->data)->start_address) {
		curr = curr->next;
	}

	list_t *mini_list = ((block_t*)curr->data)->miniblock_list;
	node_t *mini_curr = mini_list->head;

	i = 0;
	while (i++ < mini_list->size - 1 && address >
	((miniblock_t*)mini_curr->data)->start_address) {
		mini_curr = mini_curr->next;
	}

	if (address != ((miniblock_t*)mini_curr->data)->start_address) {
		printf("Address not found\n");
		return;
	}

	if (strcmp((char*)permission, "PROT_NONE") == 0)
		((miniblock_t*)mini_curr->data)->perm = 0;
	if (strcmp((char*)permission, "PROT_READ") == 0)
		((miniblock_t*)mini_curr->data)->perm |= (1<<2);
	if (strcmp((char*)permission, "PROT_WRITE") == 0)
		((miniblock_t*)mini_curr->data)->perm |= (1<<1);
	if (strcmp((char*)permission, "PROT_EXEC") == 0)
		((miniblock_t*)mini_curr->data)->perm |= 1;
}