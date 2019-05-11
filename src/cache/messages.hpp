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

typedef enum {
    SET_DIRTY,
    CHECK_VALIDITY_DIRTY,
    CHECK_DATA_VALIDITY,
    INVALID_LINE,
    LOAD,
    STORE
} DirCacheOp;


struct dir_cache_message {
    DirCacheOp op;     // Type of requested operation
    addr_t addr; // Address on which perform the operation
    bool arg;      // in/out value that depends on the requested operation

    /* Cache line of both load and store operation. This field must be filled in
     * STORE and LOAD operation */
    std::vector<word_t> data;

    dir_cache_message(DirCacheOp op, word_t addr, bool arg = true,
                 std::vector<word_t> data = std::vector<word_t>())
        : op(op), addr(addr), arg(arg), data(data){};

    // Default copy costructor
    dir_cache_message(const dir_cache_message &c) = default;
};