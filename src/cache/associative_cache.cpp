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
bool AssociativeCache::determine_way(unsigned& way, mem_unit& victim)
{
	// Scan through the caches looking for a free slot, if any
	bool free = false;
	for (unsigned i = 0; i < n_ways; ++i) {
		/* TODO Check if that cache is free */
		if (free) {
			way = i;
			return false;
		}
	}
	
	// At this point, no cache is free, hence we must pick a victim
	if (is_vict_predet) {
		/* TODO: (predetermined victim) find the way corresponding to vict_predet_addr */
	} else {
		/* TODO: use the replacement policy to determine the way */
	}
	return true;
}


/* Routine to follow when a cache miss is generated */
void AssociativeCache::cache_miss_routine(addr_t addr)
{
	bool is_replacement;
	mem_unit victim;
	
	status.push(AssCacheStatus::READ_UP);
	
	is_replacement = determine_way(this->target_way, victim);
	
	cache_message *read_below;
	if (is_replacement) {
		// Request to read from below (don't forget to specify the victim) */
		read_below = craft_ass_cache_msg(OP_READ, (mem_unit){.addr=addr, .data=nullptr}, 
				(mem_unit){.addr=victim.addr, .data=victim.data});
	} else {
		// Request to read from below (victim absent)
		read_below = craft_ass_cache_msg(OP_READ, (mem_unit){.addr=addr, .data=nullptr}, 
				(mem_unit){.addr=0, .data=nullptr});
	}
	message *m = craft_msg(prev_name, read_below);
	sendWithDelay(m, 0);
}


/* Routine to follow when receiving a 'read' message from the upper level */
void AssociativeCache::handle_msg_read_prev(cache_message *cm)
{
	status.push(AssCacheStatus::READ_UP);
	
	// Save the victim address if predetermined by upper level
	is_vict_predet = (cm->victim.data != nullptr);
	if (is_vict_predet)
		vict_predet_addr = cm->victim.addr;
	
	// Reset state for inner cache operation
	n_dcache_replies = 0;
	op_hit = false;
	word_t *op_data = nullptr;
	
	// Send request to each direct cache
	for (auto &dc : ways) {
		// Ask to read the whole block from cache (independently from needed size)
		SAC_to_CWP *read_dcache = new SAC_to_CWP{LOAD, cm->target.addr, nullptr};
		message *m = craft_msg(prev_name, read_dcache);
		sendWithDelay(m, 0);
	}
	
	status.push(AssCacheStatus::READ_IN);
}


/* Routine to follow when receiving a 'read' message from the lower level */
void AssociativeCache::handle_msg_read_next(cache_message *cm) 
{	
	AssCacheStatus acs = status.top();
	status.pop();
	assert(acs == AssCacheStatus::MISS);
	
	/* TODO: replace the previously chosen victim with the just received block */
	status.push(AssCacheStatus::WRITE_BLOCK_IN);
	/* TODO: send request */
}


/* Routine to follow when receiving a 'read' message from a nested direct cache */
void AssociativeCache::handle_msg_read_inner(CWP_to_SAC *cm, unsigned way_idx)
{
	// Preprocess the response
	++n_dcache_replies;
	if (cm->hit_flag) {
		op_hit = true;
		op_data = cm->data;
		op_hit_way = way_idx;
	}
	
	// If this is not the last reply (i.e. we expect more responses), return
	if (n_dcache_replies < n_ways)
		return;
	
	// Update global state
	AssCacheStatus acs = status.top();
	status.pop();
	assert(acs == AssCacheStatus::READ_IN);
	
	// Size of the data to be actually read
	bool is_size_word = (mem_unit_size == sizeof(word_t));
	
	if (op_hit) {	// read hit
		void *response_data = malloc(mem_unit_size);
		
		if (is_size_word) {
			unsigned offset_size = (unsigned)std::round(std::log2(block_size));
			unsigned offset = cm->address & ((1 << offset_size)-1);
			memcpy(response_data, (void *)(op_data + offset/2), mem_unit_size);
		} else {
			memcpy(response_data, (void *)op_data, mem_unit_size);
		}
		// Send back requested data to previous level
		cache_message *read_response = craft_ass_cache_msg(
				OP_READ, mem_unit{cm->address, (addr_t *)response_data}, mem_unit{0, nullptr}
		);
		message *m = craft_msg(prev_name, read_response);
		sendWithDelay(m, 0);
	} else {		// read miss
		cache_miss_routine(cm->address);
	}
}


/* Routine to follow when receiving a 'write' message from the upper level */
void AssociativeCache::handle_msg_write_prev(cache_message *cm)
{
	status.push(AssCacheStatus::WRITE_UP);
	
	// Save the victim address if predetermined by upper level
	is_vict_predet = (cm->victim.data != nullptr);	/* TODO: protocol has changed, this should be a constructor parameter */
	if (is_vict_predet)
		vict_predet_addr = cm->victim.addr;
	
	// Reset state for inner cache operation
	n_dcache_replies = 0;
	op_hit = false;
	/* Add some other status info to handle all the possible response cases */
	
	// Try to write to all direct caches
	for (auto &dc : ways) {
		SAC_to_CWP *write_dcache = new SAC_to_CWP{WRITE_WITH_POLICIES, cm->target.addr, cm->target.data};
		message *m = craft_msg(prev_name, write_dcache);
		sendWithDelay(m, 0);
	}
	status.push(AssCacheStatus::WRITE_WORD_IN);
}


/* Routine to follow when receiving a 'write' message from the lower level */
void AssociativeCache::handle_msg_write_next(cache_message *cm)
{
	AssCacheStatus acs = status.top();
	status.pop();
	assert(acs == AssCacheStatus::WRITE_UP);
	
	// Propagate ack message to previous level (same fields)
	cache_message *ack = craft_ass_cache_msg(cm->op_type, cm->target, cm->victim);
	message *m = craft_msg(prev_name, ack);
	sendWithDelay(m, 0);
	
	// Cleanup
	delete cm;
}


/* Routine to follow when receiving a 'write' message from a nested direct cache */
void AssociativeCache::handle_msg_write_inner(CWP_to_SAC *cm, unsigned way_idx)
{
	/* TODO ... */
}


AssociativeCache::AssociativeCache(System& sys, string name, string prev_name, string next_name,
			unsigned n_ways, unsigned cache_size, unsigned block_size, unsigned mem_unit_size,
			WritePolicy wp, AllocationPolicy ap, ReplacementPolicy rp, int prio)
	: module(name, prio),
	  prev_name(prev_name),
	  next_name(next_name),
	  n_ways(n_ways),
	  cache_size(cache_size),
	  block_size(block_size),
	  mem_unit_size(mem_unit_size),
	  write_policy(write_policy),
	  alloc_policy(alloc_policy),
	  repl_policy(rp)
{
	std::cout << getName() << ": building associative cache" << std::endl;	// DEBUG
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
	if (prev_name.compare(sender) == 0) {
		// previous level
		cache_message *cm = (cache_message *)m->magic_struct;
		switch (cm->op_type) {
			case OP_READ:
				handle_msg_read_prev(cm);
				break;
			case OP_WRITE:
				handle_msg_write_prev(cm);
				break;
		}
	} else if (next_name.compare(sender) == 0) {
		// next level
		cache_message *cm = (cache_message *)m->magic_struct;
		switch (cm->op_type) {
			case OP_READ:
				handle_msg_read_next(cm);
				break;
			case OP_WRITE:
				handle_msg_write_next(cm);
				break;
		}
	} else if (getName().compare(0, getName().size(), sender, 0, getName().size()) == 0) {
		// inner direct cache
		unsigned way_idx = std::stoi(sender.substr(getName().size() + 1));
		CWP_to_SAC *cm = (CWP_to_SAC *)m->magic_struct;
		
		switch (cm->wr) {
			case NOT_NEEDED:

				break;
			case PROPAGATE:
				
				break;
			case NO_PROPAGATE:

				break;
			case LOAD_RECALL:
			
				break;
			case CHECK_NEXT:
			
				break;
/*	outdated
			case LOAD:
				handle_msg_read_inner(cm, way_idx);
				break;
			case STORE:
				handle_msg_write_inner(cm, way_idx);
				break;
*/ 
		}
	} else {
		std::cerr << "Error: can't recognize message sender" << std::endl;
	}
}
