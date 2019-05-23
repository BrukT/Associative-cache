#pragma once
#include <string>

#include "orchestrator/module.h"
#include "orchestrator/structures.h"

class ExampleModule : public module
{
public:
	ExampleModule(string name, int priority = 0);
	void onNotify(message* m);
};
