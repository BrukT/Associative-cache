#pragma once

#include "cache/associative_cache.hpp"

class TestableAssociativeCache : public AssociativeCache
{
public:
	TestableAssociativeCache(System& sys, string name, string upper_name, string lower_name,
			unsigned n_ways, unsigned cache_size, unsigned block_size, unsigned mem_unit_size,
			WritePolicy write_policy, AllocationPolicy alloc_policy,
			ReplacementPolicy repl_policy, int priority=0);

	void push_stack(AssociativeCache::AssCacheStatus sts);
	AssociativeCache::AssCacheStatus top_stack();
	void pop_stack();
	unsigned size_stack();

	void put_message(message *m);

	static cache_message * create_outprot_payload(bool op, addr_t taddr, word_t *tdata, addr_t vaddr, word_t *vdata);
	static message * create_outprot_message(string src, string dest, int time, bool op, addr_t taddr, word_t *tdata, addr_t vaddr, word_t *vdata);
	static message * create_inprot_reply_message(string src, string dest, int time, bool hit_flag, addr_t addr, word_t *data, WriteResponse wr);
};