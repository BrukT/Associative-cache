#pragma once

enum class WritePolicy {
	WRITE_THROUGH,
	WRITE_BACK
};

enum class AllocationPolicy {
	WRITE_AROUND,
	WRITE_ALLOCATE
};

enum class ReplacementPolicy {
	PREDETERMINED,
	PLRU,
	LFU
};