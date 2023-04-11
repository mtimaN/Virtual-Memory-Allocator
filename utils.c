#include "vma.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void add_miniblock(node_t *curr, const uint64_t address, const uint64_t size)
{
	node_t *new = malloc(sizeof(node_t));
	DIE(!new, ERR);
	new->data = malloc(sizeof(miniblock_t));
	DIE(!new->data, ERR);

	// initializing the fields
	((miniblock_t *)new->data)->start_address = address;
	((miniblock_t *)new->data)->size = size;
	((miniblock_t *)new->data)->perm = 6;
	((miniblock_t *)new->data)->rw_buffer = NULL;
	// linking the new miniblock
	if (((block_t *)curr->data)->miniblock_list->size == 0) {
		// new block
		((block_t *)curr->data)->miniblock_list->head = new;
		new->next = new;
		new->prev = new;
	} else {
		new->next = ((block_t *)curr->data)->miniblock_list->head;
		new->prev = ((block_t *)curr->data)->miniblock_list->head->prev;
		((block_t *)curr->data)->miniblock_list->head->prev->next = new;
		((block_t *)curr->data)->miniblock_list->head->prev = new;
	}

	// updating the block
	((block_t *)curr->data)->size += size;
	((block_t *)curr->data)->miniblock_list->size++;
}

node_t *add_newblock(node_t *curr, const uint64_t address)
{
	// creating a new block
	node_t *new = malloc(sizeof(node_t));
	DIE(!new, ERR);
	new->data = malloc(sizeof(block_t));
	DIE(!new->data, ERR);
	((block_t *)new->data)->start_address = address;
	((block_t *)new->data)->size = 0;
	((block_t *)new->data)->miniblock_list = malloc(sizeof(list_t));
	DIE(!((block_t *)new->data)->miniblock_list, ERR);
	((block_t *)new->data)->miniblock_list->size = 0;
	((block_t *)new->data)->miniblock_list->head = NULL;

	// linking
	if (curr) {
		new->prev = curr;
		new->next = curr->next;
		curr->next->prev = new;
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

	node_t *tmp = first->miniblock_list->head->prev;
	// 1st block's head -> 2nd block's tail;
	first->miniblock_list->head->prev =
	second->miniblock_list->head->prev;

	// 2nd block's head -> 1st block's tail
	second->miniblock_list->head->prev =
	tmp;
}

void add_new_head(arena_t *arena, const uint64_t address, const uint64_t size)
{
	node_t *curr = arena->alloc_list->head;
	if (address + size > ((block_t *)curr->data)->start_address) {
		printf("This zone was already allocated.\n");
		return;
	}
	if (curr)
		curr = curr->prev;
	node_t *new = add_newblock(curr, address);
	add_miniblock(new, address, size);
	arena->alloc_list->size++;
	curr = arena->alloc_list->head;
	// if new block is adjacent to old head
	if (address + size == ((block_t *)curr->data)->start_address) {
		((block_t *)new->data)->size += ((block_t *)curr->data)->size;
		combine_blocks(((block_t *)new->data), ((block_t *)curr->data));
		free(((block_t *)curr->data)->miniblock_list);
		free(((block_t *)curr->data));
		curr->next->prev = new;
		new->next = curr->next;
		free(curr);
		arena->alloc_list->size--;
	}
	arena->alloc_list->head = new;
}

void delete_head(node_t *curr, node_t *mini_curr, list_t *mini_list)
{
	// old start_address gets shifted
	((block_t *)curr->data)->start_address +=
	((miniblock_t *)mini_curr->data)->size;
	// size is decreased
	((block_t *)curr->data)->size -= ((miniblock_t *)mini_curr->data)->size;

	// relinking
	mini_list->head = mini_curr->next;
	mini_curr->next->prev = mini_curr->prev;
	mini_curr->prev->next = mini_curr->next;
	mini_list->size--;
}

void delete_tail(node_t *curr, node_t *mini_curr, list_t *mini_list)
{
	// size is decreased
	((block_t *)curr->data)->size -= ((miniblock_t *)mini_curr->data)->size;

	// relinking
	mini_curr->next->prev = mini_curr->prev;
	mini_curr->prev->next = mini_curr->next;
	mini_list->size--;
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
	if (address < ((block_t *)curr->data)->start_address) {
		add_new_head(arena, address, size);
		return;
	}
	// curr will be right before the position where we want to insert
	uint64_t i = 0;
	while (i++ < arena->alloc_list->size - 1 && address >
			((block_t *)curr->next->data)->start_address)
		curr = curr->next;

	// if the space between the 2 nodes is too small
	if (address < ((block_t *)curr->data)->start_address +
					((block_t *)curr->data)->size) {
		printf("This zone was already allocated.\n");
		return;
	}
	if (i < arena->alloc_list->size &&
		(address + size > ((block_t *)curr->next->data)->start_address)) {
		printf("This zone was already allocated.\n");
		return;
	}
	if (address == ((block_t *)curr->data)->start_address +
					((block_t *)curr->data)->size) {
		add_miniblock(curr, address, size);
	} else {
		node_t *new = add_newblock(curr, address);
		arena->alloc_list->size++;
		add_miniblock(new, address, size);
		curr = new;
	}
	// if two blocks are next to eachother
	if (address + size == ((block_t *)curr->next->data)->start_address) {
		((block_t *)curr->data)->size += ((block_t *)curr->next->data)->size;
		combine_blocks(((block_t *)curr->data), ((block_t *)curr->next->data));

		// free the 2nd block
		free(((block_t *)curr->next->data)->miniblock_list);
		free(((block_t *)curr->next->data));
		arena->alloc_list->size--;
		node_t *tmp = curr->next;
		curr->next->next->prev = curr;
		curr->next = curr->next->next;
		free(tmp);
	}
}

void split_block(arena_t *arena, node_t *curr, node_t *mini_curr,
				 list_t *mini_list, uint64_t new_size, uint64_t no_items)
{
	// splitting into 2 blocks
	block_t *second_block = malloc(sizeof(block_t));
	DIE(!second_block, ERR);
	second_block->start_address =
	((miniblock_t *)mini_curr->data)->start_address +
	((miniblock_t *)mini_curr->data)->size;
	second_block->size = ((block_t *)curr->data)->size -
						new_size - ((miniblock_t *)mini_curr->data)->size;
	second_block->miniblock_list = malloc(sizeof(list_t));
	DIE(!second_block->miniblock_list, ERR);
	second_block->miniblock_list->size = mini_list->size - no_items - 1;
	mini_list->size = no_items;

	((block_t *)curr->data)->size = new_size;
	// linking the 2 lists
	// second list
	second_block->miniblock_list->head = mini_curr->next;
	second_block->miniblock_list->head->prev = mini_list->head->prev;
	mini_list->head->prev->next = second_block->miniblock_list->head;
	// first list
	mini_curr->prev->next = mini_list->head;
	mini_list->head->prev = mini_curr->prev;

	// adding the new block to the list
	node_t *new = malloc(sizeof(node_t));
	DIE(!new, ERR);
	new->data = second_block;
	new->prev = curr;
	new->next = curr->next;
	curr->next->prev = new;
	curr->next = new;
	arena->alloc_list->size++;
}
