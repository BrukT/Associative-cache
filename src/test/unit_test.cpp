#define CATCH_CONFIG_MAIN

#include <iostream>
#include <cstring>
#include <type_traits>

#include "catch.hpp"

//#include "cache/associative_cache.hpp"
#include "testable_associative_cache.hpp"
#include "orchestrator/System.h"


SCENARIO( "Associative caches can be instantiated and work properly", "[ass_cache][init]") {

    GIVEN( "A system") {
        System system;

        WHEN( "A (testable) associative cache is created") {
			string name = "L1";
            TestableAssociativeCache *ac = new TestableAssociativeCache(system, name, "cpu", "L2", 2, 192000, 64, 2,
							 WritePolicy::WRITE_BACK, AllocationPolicy::WRITE_ALLOCATE,
							 ReplacementPolicy::PLRU);

            THEN( "Its name is accessible and correct") {
                REQUIRE (name.compare(ac->getName()) == 0);
            }
			
			AND_THEN( "Its internal stack is accessible and working ") {
				ac->push_stack(AssociativeCache::AssCacheStatus::READ_UP);
				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::READ_UP);
				
				ac->push_stack(AssociativeCache::AssCacheStatus::MISS);
				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::MISS);
				
				ac->pop_stack();
				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::READ_UP);
				ac->pop_stack();
			}
			
			AND_THEN( "It can be added as module to the system") {
				REQUIRE( std::is_base_of<module, AssociativeCache>::value );
				
				system.addModule(ac);
			}
        }
    }

    GIVEN( "An associative cache ") {
    	System system;
    	TestableAssociativeCache *ac = new TestableAssociativeCache(system, "L1", "cpu", "L2", 2, 65536, 64, 2,
				WritePolicy::WRITE_THROUGH, AllocationPolicy::WRITE_AROUND, ReplacementPolicy::PLRU);

    	WHEN ("A read request from the previous level is processed") {

    		message *m = TestableAssociativeCache::create_outprot_message(
    				"cpu", "L1", 0, OP_READ, 0xAAAA, nullptr, 0x00, nullptr);
    		ac->put_message(m);

    	  	THEN ("The internal state (stack) remains consistent") {

				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::READ_IN);
				ac->pop_stack();
				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::READ_UP);

    		} 

            THEN ("The requests sent to direct caches are correct") {

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
    		}

			word_t *block = (word_t*)malloc(64);
			memcpy(block, "abcdABCDefghEFGHijklIJKLmnopMNOPabcdABCDefghEFGHijklIJKLmnopMNOP", 64);
			REQUIRE(block[21] == 0x6867);	// 'hg' (reversed because of little-endianness)

			AND_WHEN ("One of the direct caches has the requested data (hit)") {

				message *reply0 = TestableAssociativeCache::create_inprot_reply_message(
						"L1_0", "L1", 0, false, 0xAAAA, nullptr, NOT_NEEDED);
				message *reply1 = TestableAssociativeCache::create_inprot_reply_message(
						"L1_1", "L1", 0, true, 0xAAAA, block, NOT_NEEDED);
				ac->put_message(reply0);
	    		ac->put_message(reply1);
	    		std::vector<event*> evts = ac->initialize();
	    		REQUIRE(evts.size() == 3);
	    		message *m = evts[2]->m;	// reply to upper level
                REQUIRE(string("cpu").compare(m->dest) == 0);
	    		cache_message *payload = (cache_message *)m->magic_struct;

	    		THEN("Internal state remains consistent") {
	    			
	    			REQUIRE (ac->size_stack() == 0);
                }

                THEN("Response to upper level is correct") {

	    			REQUIRE (payload->op_type == OP_READ);
	    			REQUIRE (payload->target.addr == 0xAAAA);
	    			REQUIRE (*(payload->target.data) == 0x6867 );	// 'hg' (reversed because of little-endianness)

	    		}
	    	} AND_WHEN ("None of the direct caches has the requested data (miss)") {

				message *reply0 = TestableAssociativeCache::create_inprot_reply_message(
						"L1_0", "L1", 0, false, 0xAAAA, nullptr, NOT_NEEDED);
				message *reply1 = TestableAssociativeCache::create_inprot_reply_message(
						"L1_1", "L1", 0, false, 0xAAAA, nullptr, NOT_NEEDED);
				ac->put_message(reply0);
	    		ac->put_message(reply1);

	    		std::vector<event*> evts = ac->initialize();
	    		REQUIRE(evts.size() == 3);
	    		message *m = evts[2]->m;	// request propagated below
                REQUIRE(string("L2").compare(m->dest) == 0);
	    		cache_message *payload = (cache_message *)m->magic_struct;

	    		THEN("The internal state remains consistent ") {
	    			
	    			REQUIRE (ac->size_stack() == 2);
	    			REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::MISS);
                }

                THEN ("A request to read from next level is correctly sent ") {

	    			REQUIRE (payload->op_type == OP_READ);
	    			REQUIRE (payload->target.addr == 0xAAAA);
	    			REQUIRE (payload->target.data == nullptr );
                }

                AND_WHEN ("Requested data eventually comes from the next level ") {

                    word_t *block = (word_t*)malloc(64);
                    memcpy(block, "abcdABCDefghEFGHijklIJKLmnopMNOPabcdABCDefghEFGHijklIJKLmnopMNOP", 64);
                    message *m = TestableAssociativeCache::create_outprot_message(
                            "L2", "L1", 0, OP_READ, 0xAAAA, block, 0x00, nullptr);
                    ac->put_message(m);

                    THEN ("The internal state remains consistent ") {
                        REQUIRE (ac->size_stack() == 2);
                        REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::REPLACE_BLOCK_IN);
                    }

                    THEN ("A direct cache is correctly prompted to store the just fetched block ") {

                        std::vector<event*> evts = ac->initialize();
                        REQUIRE(evts.size() == 1);
                        message *m = evts[0]->m;    // request to store the fetched block
                        SAC_to_CWP *payload = (SAC_to_CWP *)m->magic_struct;
                        REQUIRE(payload->op_type == STORE);
                        REQUIRE(payload->address == 0xAAAA);
                        REQUIRE(payload->data == block);
                    }

                    AND_WHEN ("The fetched block has been stored in this cache") {

                        message *store_ack = TestableAssociativeCache::create_inprot_reply_message(
                                "L1_0", "L1", 0, true, 0xAAAA, nullptr, NOT_NEEDED);
                        ac->put_message(store_ack);

                        std::vector<event*> evts = ac->initialize();
                        REQUIRE(evts.size() == 2);
                        message *m = evts[1]->m;    // response containing read block
                        cache_message *payload = (cache_message *)m->magic_struct;

                        THEN ("The internal state remains consistent") {

                            REQUIRE (ac->size_stack() == 0);
                        }

                        THEN ("The data is correctly sent to the previous level too, that requested it first") {

                            REQUIRE(string("cpu").compare(m->dest) == 0);
                            REQUIRE (payload->op_type == OP_READ);
                            REQUIRE (payload->target.addr == 0xAAAA);
                            REQUIRE (*(payload->target.data) == 0x6867 );   // 'hg' (reversed because of little-endianness)
                        }
                    }
                }
	    	}
    	}

    	WHEN ("A write request arrives from the previous level") {

    		word_t *data = (word_t*)malloc(1);
    		*data = 0x1234;
    		message *m = TestableAssociativeCache::create_outprot_message(
    				"cpu", "L1", 0, OP_WRITE, 0xAAAA, data, 0x00, nullptr);
    		ac->put_message(m);

    	  	THEN ("The internal state (stack) remains consistent") {

				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::WRITE_WORD_IN);
				ac->pop_stack();
				REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::WRITE_UP);

    		} AND_THEN ( "The requests sent to direct caches are correct") {

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

            word_t *block = (word_t*)malloc(64);
            memcpy(block, "abcdABCDefghEFGHijklIJKLmnopMNOPabcdABCDefghEFGHijklIJKLmnopMNOP", 64);
            REQUIRE(block[21] == 0x6867);   // 'hg' (reversed because of little-endianness)

            AND_WHEN ("One of the direct caches contains that line of cache, so that write succeeds (hit)") {

                message *reply0 = TestableAssociativeCache::create_inprot_reply_message(
                        "L1_0", "L1", 0, false, 0xAAAA, nullptr, W_MISS_ALLOCATE);
                message *reply1 = TestableAssociativeCache::create_inprot_reply_message(
                        "L1_1", "L1", 0, true, 0xAAAA, nullptr, W_HIT_NO_PROP);
                ac->put_message(reply0);
                ac->put_message(reply1);
                std::vector<event*> evts = ac->initialize();
                REQUIRE(evts.size() == 3);
                message *m = evts[2]->m;    // reply to upper level
                REQUIRE(string("cpu").compare(m->dest) == 0);
                cache_message *payload = (cache_message *)m->magic_struct;

                THEN("Internal state remains consistent") {
                    
                    REQUIRE (ac->size_stack() == 0);
                }

                THEN("Response to upper level is correct") {

                    REQUIRE (payload->op_type == OP_WRITE);
                    REQUIRE (payload->target.addr == 0xAAAA);
                    REQUIRE (payload->target.data == nullptr);

                }
            }  AND_WHEN ("None of the direct caches has the right block where to write (miss)") {

                message *reply0 = TestableAssociativeCache::create_inprot_reply_message(
                        "L1_0", "L1", 0, false, 0xAAAA, nullptr, W_MISS_ALLOCATE);
                message *reply1 = TestableAssociativeCache::create_inprot_reply_message(
                        "L1_1", "L1", 0, false, 0xAAAA, nullptr, W_MISS_ALLOCATE);
                ac->put_message(reply0);
                ac->put_message(reply1);

                std::vector<event*> evts = ac->initialize();
                REQUIRE(evts.size() == 3);
                message *m = evts[2]->m;    // request to fetch block from below
                REQUIRE(string("L2").compare(m->dest) == 0);
                cache_message *payload = (cache_message *)m->magic_struct;

                THEN("The internal state remains consistent ") {
                    
                    REQUIRE (ac->size_stack() == 2);
                    REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::MISS);
                }

                THEN ("A request to read from next level is correctly sent ") {

                    REQUIRE (payload->op_type == OP_READ);
                    REQUIRE (payload->target.addr == 0xAAAA);
                    REQUIRE (payload->target.data == nullptr );
                }

                AND_WHEN ("Requested data eventually comes from the next level ") {

                    word_t *block = (word_t*)malloc(64);
                    memcpy(block, "abcdABCDefghEFGHijklIJKLmnopMNOPabcdABCDefghEFGHijklIJKLmnopMNOP", 64);
                    message *m = TestableAssociativeCache::create_outprot_message(
                            "L2", "L1", 0, OP_READ, 0xAAAA, block, 0x00, nullptr);
                    ac->put_message(m);

                    THEN ("The internal state remains consistent ") {
                        REQUIRE (ac->size_stack() == 2);
                        REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::REPLACE_BLOCK_IN);
                    }

                    THEN ("A direct cache is correctly prompted to store the just fetched block ") {

                        std::vector<event*> evts = ac->initialize();
                        REQUIRE(evts.size() == 1);
                        message *m = evts[0]->m;    // request to store the fetched block
                        SAC_to_CWP *payload = (SAC_to_CWP *)m->magic_struct;
                        REQUIRE(payload->op_type == STORE);
                        REQUIRE(payload->address == 0xAAAA);
                        REQUIRE(payload->data == block);
                    }

                    AND_WHEN ("The block fetched from below has been also stored in this cache") {

                        message *store_ack = TestableAssociativeCache::create_inprot_reply_message(
                                "L1_0", "L1", 0, true, 0xAAAA, nullptr, NOT_NEEDED);
                        ac->put_message(store_ack);

                        THEN ("The internal state remains consistent") {

                            REQUIRE (ac->size_stack() == 2);
                            REQUIRE (ac->top_stack() == AssociativeCache::AssCacheStatus::WRITE_WORD_IN);
                        }

                        THEN ("The write request that previously missed gets repeated") {

                            std::vector<event*> evts = ac->initialize();
                            REQUIRE(evts.size() == 3);
                            message *m0 = evts[1]->m;    // write to inner cache 0
                            message *m1 = evts[2]->m;    // write to inner cache 1
                            SAC_to_CWP *payload0 = (SAC_to_CWP *)m0->magic_struct;
                            SAC_to_CWP *payload1 = (SAC_to_CWP *)m1->magic_struct;
                            REQUIRE(payload0->op_type == WRITE_WITH_POLICIES);
                            REQUIRE(payload0->address == 0xAAAA);
                            REQUIRE(*(payload0->data) == 0x1234);
                            REQUIRE(payload1->op_type == WRITE_WITH_POLICIES);
                            REQUIRE(payload1->address == 0xAAAA);
                            REQUIRE(*(payload1->data) == 0x1234);
                        }

                        AND_WHEN ("The new data (coming from previous level) has been also written") {

                            message *reply0 = TestableAssociativeCache::create_inprot_reply_message(
                                    "L1_0", "L1", 0, true, 0xAAAA, nullptr, W_HIT_NO_PROP);
                            message *reply1 = TestableAssociativeCache::create_inprot_reply_message(
                                    "L1_1", "L1", 0, false, 0xAAAA, nullptr, W_MISS_ALLOCATE);
                            ac->put_message(reply0);
                            ac->put_message(reply1);

                            std::vector<event*> evts = ac->initialize();
                            REQUIRE(evts.size() == 4);
                            message *m = evts[3]->m;    // reply to upper level
                            REQUIRE(string("cpu").compare(m->dest) == 0);
                            cache_message *payload = (cache_message *)m->magic_struct;

                            THEN("Internal state remains consistent") {
                                
                                REQUIRE (ac->size_stack() == 0);
                            }

                            THEN("Response to upper level is correct") {

                                REQUIRE (payload->op_type == OP_WRITE);
                                REQUIRE (payload->target.addr == 0xAAAA);
                                REQUIRE (payload->target.data == nullptr);
                            }
                        }
                    }
                }
            }
    	}
    }
}