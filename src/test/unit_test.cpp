#define CATCH_CONFIG_MAIN

#include <type_traits>

#include "catch.hpp"

//#include "cache/associative_cache.hpp"
#include "testable_associative_cache.hpp"
#include "orchestrator/System.h"


SCENARIO( "Associative caches can created and added to the system", "[ass_cache][init]" ) {

    GIVEN( "A system" ) {
        System system;

        WHEN( "A (testable) associative cache is created" ) {
			string name = "L1";
            TestableAssociativeCache *ac = new TestableAssociativeCache(system, name, "cpu", "L2", 2, 192000, 64, 2,
							 WritePolicy::WRITE_THROUGH, AllocationPolicy::WRITE_AROUND,
							 ReplacementPolicy::PLRU);

            THEN( "its name is accessible and correct" ) {
                REQUIRE (name.compare(ac->getName()) == 0);
            }
			
			AND_THEN( "stack is accessible and working " ) {
				ac->push_stack(AssociativeCache::AssCacheStatus::READ_UP);
				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::READ_UP);
				
				ac->push_stack(AssociativeCache::AssCacheStatus::READ_DOWN);
				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::READ_DOWN);
				
				ac->pop_stack();
				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::READ_UP);
				ac->pop_stack();
			}
			
			AND_THEN( "it can be added as module to the system" ) {
				REQUIRE( std::is_base_of<module, AssociativeCache>::value );
				
				system.addModule(ac);
			}
        }
    }
}