#pragma once

#include <stdint.h>

#include "arch.hpp"
#include "orchestrator/structures.h"

/* ===============
 * Inner interface
 * ===============*/
 
#include "SetAssociative_WritePolicies_message.h"

typedef struct message message;


/* ===============
 * Outer interface
 * ===============*/
 
# define OP_READ 		false
# define OP_WRITE 		true


typedef struct block {
	addr_t 		addr;
	word_t		*data;
} block;


typedef block 	mem_unit;


typedef struct cache_message {
	bool 		op_type;
	mem_unit 	target;
	mem_unit 	victim;
} ass_cache_msg;
