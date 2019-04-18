#pragma once
#include <string>

#include "module.h"
#include "structures.h"

class AssociativeCache : public module
{
	size_t nuber_of_caches;
public:
	AssociativeCache(string name, int priority = 0);
	void onNotify(message* m);
	void onMiss(message* m);
	void onHit(message* m);
};
