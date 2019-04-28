#pragma once

#include <memory>
#include <string>
#include <vector>

#include "system/module.h"
#include "system/structures.h"

#include "messages.hpp"
#include "mock_direct_cache.hpp"


class AssociativeCache : public module
{
	unsigned n_ways;
	std::vector<DirectCache*> ways;
	
	ass_cache_msg * craft_ass_cache_msg(bool op, mem_unit tgt, mem_unit vcm);
	message * craft_msg(char *dest, void *content);
	
	
public:
	AssociativeCache(string name, unsigned n_ways,
			std::vector<DirectCache*> ways, int priority=0);
	void onNotify(message* m);
};
