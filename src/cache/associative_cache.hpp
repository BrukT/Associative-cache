#pragma once

#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "system/module.h"
#include "system/structures.h"
#include "system/System.h"

#include "messages.hpp"
#include "mock_direct_cache.hpp"


class AssociativeCache : public module
{
	enum class AssCacheStatus {
		READ_UP,
		READ_DOWN,
		WRITE_UP,
		WRITE_DOWN,
		READ_WORD_IN,
		READ_BLOCK_IN,
		WRITE_WORD_IN,
		WRITE_BLOCK_IN,
		MISS
	};
	
	string prev_name; 		// previous entity in the memory hierarchy (closer to cpu)
	string next_name; 		// next entity in the memory hierarchy (farther from cpu)
	unsigned n_ways;		// number of associative ways
	unsigned cache_size; 	// [byte] total size of the cache
	unsigned block_size; 	// [byte] size of a block
	unsigned mem_unit_size; // [byte] size of data to read
	bool write_policy;		// cache write policy | 0: write through; 1: write back
	bool allocate_policy; 	// write miss policy  | 0: write around; 1: write allocate
	std::vector<DirectCache*> ways;
	std::stack<AssCacheStatus> status;
	
	cache_message * craft_ass_cache_msg(bool op, mem_unit tgt, mem_unit vcm);
	message * craft_msg(char *dest, void *content);
	void handle_msg_read_prev(cache_message *cm);
	void handle_msg_read_next(cache_message *cm);
	void handle_msg_write_prev(cache_message *cm);
	void handle_msg_write_next(cache_message *cm);
	
	
public:
	AssociativeCache(System& sys, string name, string prev_name, string next_name,
			unsigned n_ways, unsigned cache_size, unsigned block_size, unsigned mem_unit_size,
			bool write_policy, bool allocate_policy, int priority=0);
	void onNotify(message* m);
};
