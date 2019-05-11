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

enum class WritePolicy {
	WRITE_THROUGH,
	WRITE_BACK
};

enum class AllocationPolicy {
	WRITE_AROUND,
	WRITE_ALLOCATE
};

enum class ReplacementPolicy {
	PLRU,
	LFU
};


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
	WritePolicy write_policy;		// cache write policy
	AllocationPolicy alloc_policy; 	// write miss allocation policy
	ReplacementPolicy repl_policy;	// replacement policy (victim choice)
	std::vector<DirectCache*> ways;		// direct caches
	std::stack<AssCacheStatus> status; 	// stateful component
	unsigned target_way;		// direct cache index where to write the missing block
	bool is_vict_predet;		// victim predetermined by upper levels
	addr_t vict_predet_addr; 	// address of victim when predetermined
	
	cache_message * craft_ass_cache_msg(bool op, mem_unit tgt, mem_unit vcm);
	message * craft_msg(string dest, void *content);
	bool determine_way(unsigned& way, mem_unit& victim);
	void cache_miss_routine(addr_t addr);
	void handle_msg_read_prev(cache_message *cm);
	void handle_msg_read_next(cache_message *cm);
	void handle_msg_read_inner(dir_cache_message *cm, unsigned way_idx);
	void handle_msg_write_prev(cache_message *cm);
	void handle_msg_write_next(cache_message *cm);
	void handle_msg_write_inner(dir_cache_message *cm, unsigned way_idx);
	
	
public:
	AssociativeCache(System& sys, string name, string prev_name, string next_name,
			unsigned n_ways, unsigned cache_size, unsigned block_size, unsigned mem_unit_size,
			WritePolicy write_policy, AllocationPolicy alloc_policy,
			ReplacementPolicy repl_policy, int priority=0);
	void onNotify(message* m);
};
