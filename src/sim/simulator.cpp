#include "cache/associative_cache.hpp"
#include "system/System.h"

int main() {
	System system;

	//Create a direct cache
	system.addModule(
		new AssociativeCache(system, "L1", "cpu", "L2", 2, 192000, 64, 2,
							 WritePolicy::WRITE_THROUGH, AllocationPolicy::WRITE_AROUND,
							 ReplacementPolicy::PLRU)
	);

	//Call run() to start the simulation
	system.run();
}