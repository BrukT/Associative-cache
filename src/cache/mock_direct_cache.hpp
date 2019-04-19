#pragma once

#include <string>

#include "system/module.h"
#include "system/structures.h"

class DirectCache : public module
{
public:
	DirectCache(string name, int priority = 0);
	void onNotify(message* m);
};
