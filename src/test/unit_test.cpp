#define CATCH_CONFIG_MAIN

#include <type_traits>

#include "catch.hpp"

#include "cache/associative_cache.hpp"
#include "orchestrator/System.h"


SCENARIO( "Associative caches can be added to the system", "[ass_cache][init]" ) {

    GIVEN( "A system" ) {
        System system;
		
		REQUIRE( std::is_base_of<module, AssociativeCache>::value );

        WHEN( "An associative cache is created" ) {
			string name = "L1";
            AssociativeCache *ac = new AssociativeCache(system, name, "cpu", "L2", 2, 192000, 64, 2,
							 WritePolicy::WRITE_THROUGH, AllocationPolicy::WRITE_AROUND,
							 ReplacementPolicy::PLRU);

            THEN( "its name is accessible and correct" ) {
                REQUIRE (name.compare(ac->getName()) == 0);
            }
        }
    }
}