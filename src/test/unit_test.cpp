#define CATCH_CONFIG_MAIN

#include <iostream>
#include <cstring>
#include <type_traits>

#include "catch.hpp"

//#include "cache/associative_cache.hpp"
#include "testable_associative_cache.hpp"
#include "orchestrator/System.h"


SCENARIO( "Associative caches can be instantiated and work properly", "[ass_cache][init]" ) {

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
				
				ac->push_stack(AssociativeCache::AssCacheStatus::MISS);
				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::MISS);
				
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

    GIVEN( "An associative cache ") {
    	System system;
    	TestableAssociativeCache *ac = new TestableAssociativeCache(system, "L1", "cpu", "L2", 2, 65536, 64, 2,
				WritePolicy::WRITE_THROUGH, AllocationPolicy::WRITE_AROUND, ReplacementPolicy::PLRU);

    	WHEN (" A read request arrives from the previous level") {

    		message *m = TestableAssociativeCache::create_outprot_message(
    				"cpu", "L1", 0, OP_READ, 0xAAAA, nullptr, 0x00, nullptr);
    		ac->put_message(m);

    	  	THEN (" Internal state remains consistent ") {

				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::READ_IN);
				ac->pop_stack();
				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::READ_UP);

    		} AND_THEN ( " Requests sent to direct caches are correct " ) {

    			std::vector<event*> evts = ac->initialize();
    			REQUIRE(evts.size() == 2);
    			message *m0 = evts[0]->m;
    			message *m1 = evts[1]->m;
    			SAC_to_CWP *payload0 = (SAC_to_CWP *)m0->magic_struct;
    			SAC_to_CWP *payload1 = (SAC_to_CWP *)m1->magic_struct;
    			REQUIRE(payload0->op_type == LOAD);
    			REQUIRE(payload0->address == 0xAAAA);
    			REQUIRE(payload1->op_type == LOAD);
    			REQUIRE(payload1->address == 0xAAAA);
    		} AND_THEN ( " It behaves correctly to replies from direct caches " ) {

    			word_t *block = (word_t*)malloc(64);
    			memcpy(block, "abcdABCDefghEFGHijklIJKLmnopMNOPabcdABCDefghEFGHijklIJKLmnopMNOP", 64);
    			REQUIRE(block[21] == 0x6867);	// 'hg' (reversed because of little-endianness)

    			WHEN (" One of the responses is a hit ") {

    				message *reply0 = TestableAssociativeCache::create_inprot_reply_message(
    						"L1_0", "L1", 0, false, 0xAAAA, nullptr, NOT_NEEDED);
    				message *reply1 = TestableAssociativeCache::create_inprot_reply_message(
    						"L1_1", "L1", 0, true, 0xAAAA, block, NOT_NEEDED);
    				ac->put_message(reply0);
		    		ac->put_message(reply1);
		    		std::vector<event*> evts = ac->initialize();
		    		REQUIRE(evts.size() == 3);
		    		message *m = evts[2]->m;	// reply to upper level
		    		cache_message *payload = (cache_message *)m->magic_struct;

		    		THEN(" State and response to upper level are both correct " ) {
		    			
		    			REQUIRE (ac->size_stack() == 0);
		    			REQUIRE (payload->op_type == OP_READ);
		    			REQUIRE (payload->target.addr == 0xAAAA);
		    			REQUIRE (*(payload->target.data) == 0x6867 );	// 'hg' (reversed because of little-endianness)

		    		}
		    	} AND_WHEN (" All the responses are misses ") {

    				message *reply0 = TestableAssociativeCache::create_inprot_reply_message(
    						"L1_0", "L1", 0, false, 0xAAAA, nullptr, NOT_NEEDED);
    				message *reply1 = TestableAssociativeCache::create_inprot_reply_message(
    						"L1_1", "L1", 0, false, 0xAAAA, nullptr, NOT_NEEDED);
    				ac->put_message(reply0);
		    		ac->put_message(reply1);
		    		std::vector<event*> evts = ac->initialize();
		    		REQUIRE(evts.size() == 3);
		    		message *m = evts[2]->m;	// request propagated below
		    		cache_message *payload = (cache_message *)m->magic_struct;

		    		THEN( " TODO: status and next request are correct " ) {
		    			
		    			REQUIRE (ac->size_stack() == 2);
		    			REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::MISS);
		    			REQUIRE (payload->op_type == OP_READ);
		    			REQUIRE (payload->target.addr == 0xAAAA);
		    			REQUIRE (payload->target.data == nullptr );
		    		}
		    	}
    		}
    	}

    	WHEN (" A write request arrives from the previous level") {

    		word_t *data = (word_t*)malloc(1);
    		*data = 0x1234;
    		message *m = TestableAssociativeCache::create_outprot_message(
    				"cpu", "L1", 0, OP_WRITE, 0xAAAA, data, 0x00, nullptr);
    		ac->put_message(m);

    	  	THEN (" internal state remains consistent ") {

				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::WRITE_WORD_IN);
				ac->pop_stack();
				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::WRITE_UP);

    		} AND_THEN ( "response message is the expected one " ) {

    			std::vector<event*> evts = ac->initialize();
    			REQUIRE(evts.size() == 2);
    			message *m0 = evts[0]->m;
    			message *m1 = evts[1]->m;
    			SAC_to_CWP *payload0 = (SAC_to_CWP *)m0->magic_struct;
    			SAC_to_CWP *payload1 = (SAC_to_CWP *)m1->magic_struct;
    			REQUIRE(payload0->op_type == WRITE_WITH_POLICIES);
    			REQUIRE(payload0->address == 0xAAAA);
    			REQUIRE(*(payload0->data) == 0x1234);
    			REQUIRE(payload1->op_type == WRITE_WITH_POLICIES);
    			REQUIRE(payload1->address == 0xAAAA);
    			REQUIRE(*(payload1->data) == 0x1234);
    		}
    	}
    }
}