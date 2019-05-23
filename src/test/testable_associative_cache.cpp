#include <iostream>
#include "test/testable_associative_cache.hpp"

TestableAssociativeCache::TestableAssociativeCache(System& sys, string name, string upper_name, string lower_name,
			unsigned n_ways, unsigned cache_size, unsigned block_size, unsigned mem_unit_size,
			WritePolicy wp, AllocationPolicy ap, ReplacementPolicy rp, int prio)
	: AssociativeCache(sys, name, upper_name, lower_name, n_ways, cache_size, block_size, mem_unit_size, wp, ap, rp, prio)
{
	std::cout << "Allocated testable version of AssociativeCache" << std::endl;
}


void TestableAssociativeCache::push_stack(AssociativeCache::AssCacheStatus sts)
{
	this->status.push(sts);
}


AssociativeCache::AssCacheStatus TestableAssociativeCache::top_stack()
{
	return this->status.top();
}


void TestableAssociativeCache::pop_stack()
{
	this->status.pop();
}