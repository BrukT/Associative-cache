#pragma once

#include "CacheWritePolicies.h"

#define WritePolicy 		HIT_POLICY
#define AllocationPolicy 	MISS_POLICY

#define WRITE_AROUND 		WRITE_NO_ALLOCATE

#define W_HIT_PROPAGATE 	PROPAGATE
#define W_HIT_NO_PROP 		NO_PROPAGATE
#define W_MISS_ALLOCATE 	LOAD_RECALL
#define W_MISS_NO_ALLOC 	CHECK_NEXT


enum class ReplacementPolicy {
	PREDETERMINED,
	PLRU,
	LFU,
	RND
};