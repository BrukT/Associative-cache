#pragma once

#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "system/module.h"
#include "system/structures.h"
#include "system/System.h"

#include "messages.hpp"
#include "mock_write_policy.hpp"
#include "policies.hpp"
#include "ReplacementHandler.h"


class AssociativeCache : public module
{
	enum class AssCacheStatus {
		READ_UP,
		READ_DOWN,
		WRITE_UP,
		WRITE_DOWN,
		READ_IN,
		WRITE_WORD_IN,
		REPLACE_BLOCK_IN,
		DEPOSIT_VICT_BLOCK_IN,
		MISS
	};
	
	/* Naming */
	string upper_name; 		// previous entity in the memory hierarchy (closer to cpu)
	string lower_name; 		// next entity in the memory hierarchy (farther from cpu)
	/* Dimensions */
	unsigned n_ways;		// number of associative ways
	unsigned cache_size; 	// [byte] total size of the cache
	unsigned block_size; 	// [byte] size of a block
	unsigned mem_unit_size; // [byte] size of data to read
	/* Policies */
	WritePolicy write_policy;		// cache write policy
	AllocationPolicy alloc_policy; 	// write miss allocation policy
	ReplacementPolicy repl_policy;	// replacement policy (victim choice)
	/* Objects */
	std::vector<CacheWritePolicies*> ways;	// direct caches
	/* Global state */
	std::stack<AssCacheStatus> status; 	// stateful component
	/* State for replacement */
	ReplacementHandler* rh;		// replacement policy handler
	unsigned target_way;		// direct cache index where to write the missing block
	addr_t vict_predet_addr; 	// address of victim when predetermined
	word_t *vict_predet_data; 	// data of victim from previous level
	unsigned vict_predet_way;	// index of way in which the predetermined victim is stored
	/* Inner operation state */
	addr_t target_addr;			// address of the operation in progress
	word_t *fresh_data;			// fresh data to be written as requested by upper level
	word_t *fetched_data;		// data fetched from dcache or below
	bool free_way_available;	// true if any way is free for target index
	unsigned free_way;			// index of the free way for target index
	unsigned n_dcache_replies;	// current number of replies received by direct cache
	bool op_hit;				// hit/miss in any direct cache for current operation
	bool propagate;				// propagate write request (issued by inner caches)
	bool allocate; 				// allocate on write miss (issued by inner caches)
	
	cache_message * craft_ass_cache_msg(bool op, mem_unit tgt, mem_unit vcm);
	message * craft_msg(string dest, void *content);
	bool determine_way(mem_unit& victim);
	void cache_miss_routine(addr_t addr);
	void deposit_victim();
	void read_begin();
	void read_complete();
	void handle_read_hit_or_miss();
	void handle_msg_read_upper(cache_message *cm);
	void handle_msg_read_lower(cache_message *cm);
	void handle_msg_read_inner(CWP_to_SAC *cm, unsigned way_idx);
	void handle_msg_write_upper(cache_message *cm);
	void handle_msg_write_lower(cache_message *cm);
	void handle_msg_write_inner(CWP_to_SAC *cm, unsigned way_idx);
	void handle_msg_overwrite_inner(unsigned way_idx);
	
	
public:
	AssociativeCache(System& sys, string name, string upper_name, string lower_name,
			unsigned n_ways, unsigned cache_size, unsigned block_size, unsigned mem_unit_size,
			WritePolicy write_policy, AllocationPolicy alloc_policy,
			ReplacementPolicy repl_policy, int priority=0);
	void onNotify(message* m);
};
