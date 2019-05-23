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
};