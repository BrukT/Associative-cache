#pragma once

#include <stdint.h>

#include "arch.hpp"
#include "system/structures.h"

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


/* ===============
 * Inner interface
 * ===============*/

#define SAC_TO_CWP(m) static_cast<SAC_to_CWP*>(m)
#define CWP_TO_SAC(m) (CWP_to_SAC*)m

#define CWR_RESPONSE_DELAY	10


enum DirCacheOp {  
		SET_DIRTY,
		CHECK_DIRTY,
		CHECK_VALIDITY_DIRTY,
		CHECK_DATA_VALIDITY,
		INVALID_LINE,
		LOAD,
		STORE,
		WRITE_WITH_POLICIES
};
			
enum WriteResponse {NOT_NEEDED, PROPAGATE, NO_PROPAGATE, LOAD_RECALL, CHECK_NEXT};

/* request structure. possible combinations:
		-SET_DIRTY: set the address of the cache line to set dirty
		-CHECK_DIRTY: set the address of the cache line whose dirty bit must be checked
		-CHECK_VALIDITY_DIRTY: set the address to check
		-CHECK_DATA_VALIDITY: set the address to check
		-INVALID_LINE: set the address to invalid
		-LOAD: set the address of the block to load
		-STORE: set the address where to store the block and its data
		-WRITE_WITH_POLICIES: set the address where to store the word and its data
*/
		

struct SAC_to_CWP {
	
	DirCacheOp op_type;
	uint16_t address; 
	uint16_t* data;
};

/* response structure. possible combinations:
		-SET_DIRTY: address of the dirty cache line and the hit flag: 0 if the line is invalid
		-CHECK_DIRTY: address of the checked cache line and the hit flag: 1 if the line is dirty
		-CHECK_VALIDITY_DIRTY: address of the checked cache line and the hit flag: 1 if the line is dirty, 0 either if the line is invalid or if it's not dirty
		-CHECK_DATA_VALIDITY: address of the checked cache line and the hit flag: 1 if the addressed line is valid and it's the requested one
		-INVALID_LINE: address of the invalidated cache line and the hit flag: 0 if the line was already invalid
		-LOAD: address of the requested cache line, hit flag (1 in case of hit) and the read data (null in case of miss)
		-STORE: address of the written cache line, hit flag 1 if the line was valid, 0 otherwise
		-WRITE_WITH_POLICIES: 	if (hit_flag == 1)
									address: the address of the written word
								else
									if (the block is invalid)
										address: the address of the word to be written
									else
										address: the address of the block which is occupying the requested location
								wr: action that the caller must perform, depending on the write policy	
*/

struct CWP_to_SAC {
	
	//read
	bool hit_flag;
	uint16_t* data;
	uint16_t address; 
	//write
	WriteResponse wr;
};