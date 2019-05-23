#include "cache/associative_cache.hpp"
#include "orchestrator/System.h"

int main() {
	System system;

	//Create a direct cache
	system.addModule(
		new AssociativeCache(system, "L1", "cpu", "L2", 2, 192000, 64, 2,
							 WRITE_THROUGH, WRITE_AROUND, ReplacementPolicy::PLRU)
	);

	//Call run() to start the simulation
	system.run();
}