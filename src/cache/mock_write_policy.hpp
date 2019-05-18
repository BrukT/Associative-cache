#ifndef CACHE_WRITE_POLICIES
#define CACHE_WRITE_POLICIES

#include <cstdint>

#include "system/structures.h"
#include "system/module.h"
#include "mock_dm_cache.hpp"

#include "messages.hpp"
#include "policies.hpp"


class CacheWritePolicies : public dm_cache::Cache, public module {
	
	WritePolicy hit_policy;
	AllocationPolicy miss_policy;
	
	message* WP_create_message(std::string destination);
	
	CWP_to_SAC* WP_set_dirty(SAC_to_CWP* request_struct);
	CWP_to_SAC* WP_check_validity_dirty(SAC_to_CWP* request_struct);
	CWP_to_SAC* WP_check_data_validity(SAC_to_CWP* request_struct);
	CWP_to_SAC* WP_check_dirty(SAC_to_CWP* request_struct);
	CWP_to_SAC* WP_invalid_line(SAC_to_CWP* request_struct);
	CWP_to_SAC* WP_load(SAC_to_CWP* request_struct);
	CWP_to_SAC* WP_store(SAC_to_CWP* request_struct);
	CWP_to_SAC* WP_write_with_policies(SAC_to_CWP* request_struct);
		
public:

	CacheWritePolicies(string name, int priority, uint16_t cache_size, uint16_t line_size, WritePolicy hp, AllocationPolicy mp);
	void onNotify(message *m);	
};


#endif //CACHE_WRITE_POLICIES