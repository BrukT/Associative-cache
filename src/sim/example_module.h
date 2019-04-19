#pragma once
#include <string>

#include "system/module.h"
#include "system/structures.h"

class ExampleModule : public module
{
public:
	ExampleModule(string name, int priority = 0);
	void onNotify(message* m);
};
