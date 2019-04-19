#pragma once

#include <string>
#include <vector>

#include "system/module.h"
#include "system/structures.h"

#include "mock_direct_cache.hpp"

class AssociativeCache : public module
{
	unsigned n_ways;
	std::vector<DirectCache*> ways;
	
public:
	AssociativeCache(string name, unsigned n_ways,
		std::vector<DirectCache*> ways, int priority=0);
	void onNotify(message* m);
};
