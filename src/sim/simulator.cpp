#include <vector>

#include "cache/associative_cache.hpp"
#include "cache/mock_direct_cache.hpp"
#include "example_module.h"
#include "system/System.h"

int main() {
	System system;
	
	//Initialize the system here by adding modules to the system object
	system.addModule(new ExampleModule("Alice"));
	system.addModule(new ExampleModule("Bob", 10));

	DirectCache dc0("L1_0");
	DirectCache dc1("L1_1");
	std::vector<DirectCache*> dcv = {&dc0, &dc1};
	system.addModule(&dc0);
	system.addModule(&dc1);
	system.addModule(new AssociativeCache("L1", 2, dcv));

	//Call run() to start the simulation
	system.run();
}
