#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "associative_cache.hpp"


/* Create the payload of a message to interact with the other memory levels in the hierarchy */
cache_message * AssociativeCache::craft_ass_cache_msg(bool op, mem_unit tgt, mem_unit vcm)
{
	cache_message *msg = new cache_message();
	
	msg->op_type = op;
	msg->target = tgt;
	msg->victim = vcm;

	return msg;
}


/* Create a message with custom content to send to the specified recipient */
message * AssociativeCache::craft_msg(string dest, void *content)
{
	message *msg = new message();
	
	msg->valid = 1;
	msg->timestamp = getTime();
	strncpy(msg->source, getName().c_str(), 10);
	strncpy(msg->dest, dest.c_str(), 10);
	msg->magic_struct = content;
	
	return msg;
}


/* Determine which direct cache will be used to store the new data.
 * Returns true if a victim had to be selected (its address and content 
 * are returned in the parameters), false otherwise.
 */
bool AssociativeCache::determine_way(mem_unit& victim)
{
	if (free_way_available) {
		target_way = free_way;
		return false;
	}
	
	// At this point, no cache is free, hence we must pick a victim
	if (this->repl_policy != ReplacementPolicy::PREDETERMINED) {
		// Use the replacement handler to determine the way
		target_way = rh->findVictim(target_addr);
	} /* else {
		// Else do nothing, since the way relative to the predetermined index was already identified in handle_msg_read_inner
	} */
	return true;
}


/* Routine to follow when a cache miss is generated */
void AssociativeCache::cache_miss_routine(addr_t addr)
{
	bool is_replacement;
	mem_unit victim;
	
	status.push(AssCacheStatus::MISS);
	
	// Decide the way where to perform the operation, and invalidate the statistics at the corresponding index (if policy is used)
	is_replacement = determine_way(victim);
	if (rh)
		rh->invalidateStatistics(target_addr, target_way);
	
	cache_message *read_below;
	if (is_replacement) {
		// Request to read from below (don't forget to specify the victim)
		read_below = craft_ass_cache_msg(OP_READ, (mem_unit){.addr=addr, .data=nullptr}, 
				(mem_unit){.addr=victim.addr, .data=victim.data});
	} else {
		// Request to read from below (victim absent)
		read_below = craft_ass_cache_msg(OP_READ, (mem_unit){.addr=addr, .data=nullptr}, 
				(mem_unit){.addr=0, .data=nullptr});
	}
	message *m = craft_msg(lower_name, read_below);
	sendWithDelay(m, 0);
}


/* Store the victim that was evicted from previous level, and sent to this level through read request. */
void AssociativeCache::deposit_victim()
{
	status.push(AssCacheStatus::DEPOSIT_VICT_BLOCK_IN);
	SAC_to_CWP *deposit_vict = new SAC_to_CWP{STORE, vict_predet_addr, vict_predet_data};
	message *m = craft_msg(getName() + "_" + std::to_string(this->vict_predet_way), deposit_vict);
	sendWithDelay(m, 0);
}


/* Begin core procedure to accomplish a read request */
void AssociativeCache::read_begin()
{
	// Reset state for inner cache operation
	fetched_data = nullptr;
	op_hit = false;
	free_way_available = false;
	n_dcache_replies = 0;
	
	// Send request to each direct cache
	for (auto &dc : ways) {
		// Ask to read the whole block from cache (independently from needed size)
		SAC_to_CWP *read_dcache = new SAC_to_CWP{LOAD, target_addr, nullptr};
		message *m = craft_msg(dc->getName(), read_dcache);
		sendWithDelay(m, 0);
	}
	
	status.push(AssCacheStatus::READ_IN);
}


/* After the requested data has been fetched, reply to the read request */
void AssociativeCache::read_complete()
{
	AssCacheStatus acs = status.top();
	status.pop();
	assert(acs == AssCacheStatus::READ_UP);
	
	// Size of the data to be actually read
	bool is_size_word = (mem_unit_size == sizeof(word_t));
	
	void *response_data = malloc(mem_unit_size);

	if (is_size_word) {
		unsigned offset_size = (unsigned)std::round(std::log2(block_size));
		unsigned offset = target_addr & ((1 << offset_size)-1);
		memcpy(response_data, (void *)(fetched_data + offset/2), mem_unit_size);
	} else {
		memcpy(response_data, (void *)fetched_data, mem_unit_size);
	}
	// Send back requested data to previous level
	cache_message *read_response = craft_ass_cache_msg(
			OP_READ, mem_unit{target_addr, (addr_t *)response_data}, mem_unit{0, nullptr}
	);
	message *m = craft_msg(upper_name, read_response);
	sendWithDelay(m, 0);
}


void AssociativeCache::handle_read_hit_or_miss()
{
	if (op_hit) {	// read hit
		read_complete();
	} else {		// read miss
		cache_miss_routine(target_addr);
	}
}


/* Routine to follow when receiving a 'read' message from the upper level */
void AssociativeCache::handle_msg_read_upper(cache_message *cm)
{
	status.push(AssCacheStatus::READ_UP);
	
	// Save the victim address if predetermined by upper level
	if (this->repl_policy == ReplacementPolicy::PREDETERMINED) {
		vict_predet_addr = cm->victim.addr;
		vict_predet_data = cm->victim.data;
	}
	
	// Save the target address
	target_addr = cm->target.addr;
	
	// Begin read core procedure
	read_begin();
}


/* Routine to follow when receiving a 'read' message from the lower level */
void AssociativeCache::handle_msg_read_lower(cache_message *cm) 
{
	AssCacheStatus acs = status.top();
	status.pop();
	assert(acs == AssCacheStatus::MISS);
	
	// Keep the data pointer to reply (later) to the original request (if read)
	fetched_data = cm->target.data;
	
	// Write the received block in the previously chosen location (possibly overwriting victim)
	status.push(AssCacheStatus::REPLACE_BLOCK_IN);
	SAC_to_CWP *overwrite = new SAC_to_CWP{STORE, cm->target.addr, cm->target.data};
	message *m = craft_msg(getName() + "_" + std::to_string(this->target_way), overwrite);
	sendWithDelay(m, 0);
}


/* Routine to follow when receiving a 'read' message from a nested direct cache */
void AssociativeCache::handle_msg_read_inner(CWP_to_SAC *cm, unsigned way_idx)
{
	// Preprocess the response, storing useful information
	++n_dcache_replies;
	if (cm->hit_flag) {		// hit
		op_hit = true;
		fetched_data = cm->data;
		if (rh)
			rh->updateStatistics(target_addr, way_idx);
	} else {				// miss
		if (cm->address == target_addr)	{	// miss + preserve addr means that location is free
			free_way_available = true;
			free_way = way_idx;
		} else if (repl_policy == ReplacementPolicy::PREDETERMINED && cm->address == vict_predet_addr) {
			vict_predet_way = way_idx;
			target_way = way_idx; 			// will be changed later if any dcache is free
		}
	}
	
	// If this is not the last reply (i.e. we expect more responses), return
	if (n_dcache_replies < n_ways)
		return;
	
	// Update global state
	AssCacheStatus acs = status.top();
	status.pop();
	assert(acs == AssCacheStatus::READ_IN);
	
	/* Deposit the victim. Must be done exactly here: before we wouldn't have info about the position (way) of the victim. 
	 * Later, instead, we may select that address as victim again (before its content was updated locally), hence 
	 * making the protocol much more complicated. */
	if (this->repl_policy == ReplacementPolicy::PREDETERMINED && vict_predet_data != nullptr) {
		deposit_victim();
		return;
	}
	
	handle_read_hit_or_miss();
}


/* Routine to follow when receiving a 'write' message from the upper level */
void AssociativeCache::handle_msg_write_upper(cache_message *cm)
{
	status.push(AssCacheStatus::WRITE_UP);

	// Save the victim address if predetermined by upper level
	if (this->repl_policy == ReplacementPolicy::PREDETERMINED)
		vict_predet_addr = cm->victim.addr;
	
	// Save the target address and fresh data
	target_addr = cm->target.addr;
	fresh_data = cm->target.data;
	// Reset state for inner cache operation
	free_way_available = false;
	n_dcache_replies = 0;
	op_hit = false;
	
	// Try to write to all direct caches
	for (auto &dc : ways) {
		SAC_to_CWP *write_dcache = new SAC_to_CWP{WRITE_WITH_POLICIES, cm->target.addr, cm->target.data};
		message *m = craft_msg(dc->getName(), write_dcache);
		sendWithDelay(m, 0);
	}
	status.push(AssCacheStatus::WRITE_WORD_IN);
}


/* Routine to follow when receiving a 'write' message from the lower level */
void AssociativeCache::handle_msg_write_lower(cache_message *cm)
{
	AssCacheStatus acs = status.top();
	status.pop();
	assert(acs == AssCacheStatus::WRITE_UP);
	
	// Propagate ack message to previous level (same fields)
	cache_message *ack = craft_ass_cache_msg(OP_WRITE, cm->target, cm->victim);
	message *m = craft_msg(upper_name, ack);
	sendWithDelay(m, 0);
	
	// Cleanup
	delete cm;
}


/* Routine to follow when receiving a 'write with policies' message from a nested direct cache */
void AssociativeCache::handle_msg_write_inner(CWP_to_SAC *cm, unsigned way_idx)
{
	message *m;
	
	// Preprocess the response
	++n_dcache_replies;
	if (cm->hit_flag) {		// hit
		op_hit = true;
		if (rh)
			rh->updateStatistics(target_addr, way_idx);
		switch (cm->wr) {
		case W_HIT_PROPAGATE:
			propagate = true;
			break;
		case W_HIT_NO_PROP:
			propagate = false;
			break;
		default:
			std::cerr << "Error: wrong response type for write hit" << std::endl;
		}
	} else {				// miss
		switch (cm->wr) {
		case W_MISS_ALLOCATE:
			allocate = true;
			if (cm->address == target_addr)	{	// miss + preserve addr means that location is free
				free_way_available = true;
				free_way = way_idx;
			}
			break;
		case W_MISS_NO_ALLOC:
			allocate = false;
			break;
		default:
			std::cerr << "Error: wrong response type for write miss" << std::endl;
		}
	}
	
	// If this is not the last reply (i.e. we expect more responses), return
	if (n_dcache_replies < n_ways)
		return;
	
	// Update global state
	AssCacheStatus acs = status.top();
	status.pop();
	assert(acs == AssCacheStatus::WRITE_WORD_IN);
	
	if (op_hit) {
		if (propagate) {	// hit w/ write propagate
			// propagate write to lower level
			cache_message *write_below = craft_ass_cache_msg(OP_WRITE, 
					(mem_unit){.addr=target_addr, .data=fresh_data},
					(mem_unit){.addr=0, .data=nullptr}); // In case of replacement, victim was already sent when fetching
			m = craft_msg(upper_name, write_below);
		} else {			// hit w/ write back
			AssCacheStatus acs2 = status.top();
			status.pop();
			assert(acs2 == AssCacheStatus::WRITE_UP);
			// Generate ack message for previous level
			cache_message *ack = craft_ass_cache_msg(OP_WRITE,
					(mem_unit){.addr=target_addr, .data=nullptr},
					(mem_unit){.addr=0, .data=nullptr});
			m = craft_msg(upper_name, ack);
		}
		sendWithDelay(m, 0);
	} else {
		if (allocate) {		// miss w/ write allocate
			cache_miss_routine(target_addr);
		} else {			// miss w/ write around
			// Simply forward the write request to next level
			cache_message *write_below = craft_ass_cache_msg(OP_WRITE, 
					(mem_unit){.addr=target_addr, .data=fresh_data},
					(mem_unit){.addr=0, .data=nullptr}); // no replacement because write around
			m = craft_msg(upper_name, write_below);
		}
	}
}


/* Routine to follow after a 'store' (overwrite) to nested direct cache has finished.
 * That operation could be:
 * - storing the victim of the previous level
 * - storing a block that was just fetched from next level, after a miss 
 */
void AssociativeCache::handle_msg_overwrite_inner(unsigned way_idx)
{
	AssCacheStatus acs = status.top();
	status.pop();
	assert(acs == AssCacheStatus::REPLACE_BLOCK_IN ||
		   acs == AssCacheStatus::DEPOSIT_VICT_BLOCK_IN);
	
	// Resume pending action
	if (acs == AssCacheStatus::REPLACE_BLOCK_IN) {
		acs = status.top();
		switch (acs) {
		case AssCacheStatus::READ_UP:	// pending read
			// Complete the read that previously missed
			if (rh)
				rh->updateStatistics(target_addr, way_idx);
			read_complete();
			break;
		case AssCacheStatus::WRITE_UP: 	// pending write
			// Retry the same write that previously missed
			status.pop();	// WRITE_UP, will be re-pushed by handle_msg_write_upper
			cache_message *repeat_write = craft_ass_cache_msg(OP_WRITE, // build clone of original request
					(mem_unit){.addr=target_addr, .data=fresh_data}, 	// same address/data of before
					(mem_unit){.addr=0, .data=nullptr}); 				// won't miss this time
			handle_msg_write_upper(repeat_write);
			break;
		}
	} else {	// DEPOSIT_VICT_BLOCK_IN; resume previous read, checking if it was hit or miss
		handle_read_hit_or_miss();
	}
}


AssociativeCache::AssociativeCache(System& sys, string name, string upper_name, string lower_name,
			unsigned n_ways, unsigned cache_size, unsigned block_size, unsigned mem_unit_size,
			WritePolicy wp, AllocationPolicy ap, ReplacementPolicy rp, int prio)
	: module(name, prio),
	  upper_name(upper_name),
	  lower_name(lower_name),
	  n_ways(n_ways),
	  cache_size(cache_size),
	  block_size(block_size),
	  mem_unit_size(mem_unit_size),
	  write_policy(write_policy),
	  alloc_policy(alloc_policy),
	  repl_policy(rp),
	  rh(nullptr)
{
	std::cout << getName() << ": building associative cache" << std::endl;	// DEBUG
	
	// Initialize replacement handler
	unsigned offset_size = (unsigned)std::round(std::log2(block_size));
	unsigned index_size = (unsigned)std::round(std::log2(cache_size/n_ways/block_size));
	switch (rp) {
	case ReplacementPolicy::PLRU:
		rh = new ReplacementHandler(index_size, offset_size, n_ways, PLRU);
		break;
	case ReplacementPolicy::LFU:
		rh = new ReplacementHandler(index_size, offset_size, n_ways, LFU);
		break;
	case ReplacementPolicy::RND:
		rh = new ReplacementHandler(index_size, offset_size, n_ways, RND);
		break;
	case ReplacementPolicy::PREDETERMINED:
		// do nothing
		break;
	}
	
	// Allocate direct caches
	for (unsigned i = 0; i < n_ways; ++i) {
		string dcache_name = name + "_" + std::to_string(i);
		uint16_t dcache_size = cache_size / n_ways;
		uint16_t dcache_block_size = block_size;
		
		CacheWritePolicies *dcache = new CacheWritePolicies(dcache_name, 0, dcache_size,
															dcache_block_size, wp, ap);
		ways.push_back(dcache);
		sys.addModule(dcache);
	}
}


void AssociativeCache::onNotify(message* m)
{
	std::cout << getName() << ": was notified" << std::endl;	// DEBUG
	
	// Check if this is the recipient of the message (if not, exit)
	if (getName().compare(m->dest) != 0)
		return;

	string sender = m->source;

	// Demultiplex based on sender
	if (upper_name.compare(sender) == 0) {
		// previous level
		cache_message *cm = (cache_message *)m->magic_struct;
		switch (cm->op_type) {
		case OP_READ:
			handle_msg_read_upper(cm);
			break;
		case OP_WRITE:
			handle_msg_write_upper(cm);
			break;
		}
	} else if (lower_name.compare(sender) == 0) {
		// next level
		cache_message *cm = (cache_message *)m->magic_struct;
		switch (cm->op_type) {
		case OP_READ:
			handle_msg_read_lower(cm);
			break;
		case OP_WRITE:
			handle_msg_write_lower(cm);
			break;
		}
	} else if (getName().compare(0, getName().size(), sender, 0, getName().size()) == 0) {
		// inner direct cache
		unsigned way_idx = std::stoi(sender.substr(getName().size() + 1));
		CWP_to_SAC *cm = (CWP_to_SAC *)m->magic_struct;
		
		AssCacheStatus acs = status.top();
		switch (acs) {
		case AssCacheStatus::READ_IN:				// read from direct cache
			assert(cm->wr == NOT_NEEDED);
			handle_msg_read_inner(cm, way_idx);
			break;
		case AssCacheStatus::REPLACE_BLOCK_IN:		// store (overwrite) to direct cache
			assert(cm->wr == NOT_NEEDED);
			handle_msg_overwrite_inner(way_idx);
			break;
		case AssCacheStatus::WRITE_WORD_IN:			// write (using policies) to direct cache
			handle_msg_write_inner(cm, way_idx);
			break;
		default:
			std::cerr << "Error: direct cache response received in wrong state" << std::endl;
			break;
		}
	} else {
		std::cerr << "Error: can't recognize message sender" << std::endl;
	}
}
