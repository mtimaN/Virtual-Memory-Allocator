// Copyright Mantu Matei-Cristian 2022-2023
#pragma once
#include "vma.h"
#include <inttypes.h>
#include <stddef.h>
#include <errno.h>

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);				        \
		}							\
	} while (0)

#define ERR "Malloc failed!\n"

void add_miniblock(node_t *curr, const uint64_t address, const uint64_t size);

node_t *add_newblock(node_t *curr, const uint64_t address);

void combine_blocks(block_t *first, block_t *second);

void add_new_head(arena_t *arena, const uint64_t address, const uint64_t size);

void delete_head(node_t *curr, node_t *mini_curr, list_t *mini_list);

void delete_tail(node_t *curr, node_t *mini_curr, list_t *mini_list);

void split_block(arena_t *arena, node_t *curr, node_t *mini_curr,
				 list_t *mini_list, uint64_t new_size, uint64_t no_items);
