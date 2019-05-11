#include <cassert>
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
	bool is_size_word = (mem_unit_size == sizeof(word_t));
	
	status.push(AssCacheStatus::READ_UP);
	
	// Save the victim address if predetermined by upper level
	is_vict_predet = (cm->victim.data != nullptr);
	if (is_vict_predet)
		vict_predet_addr = cm->victim.addr;
	
	for (auto &dc : ways) {
		/* TODO: Specify size of read request (depending on is_size_word) */
		dir_cache_message *read_dcache = new dir_cache_message(LOAD, cm->target.addr);
		message *m = craft_msg(prev_name, read_dcache);
		sendWithDelay(m, 0);
	}
	
	if (is_size_word)
		status.push(AssCacheStatus::READ_WORD_IN);
	else
		status.push(AssCacheStatus::READ_BLOCK_IN);
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
void AssociativeCache::handle_msg_read_inner(dir_cache_message *cm, unsigned way_idx)
{
	bool hit;
	/* TODO: store the content of the message (for future use) */
	/* TODO: if this is not the last reply (i.e. we expect more responses), return */
	/* TODO: compute hit/miss on the basis of ALL the received replies */
	if (hit) {
		/* TODO: craft response message for previous level */
		/* TODO: send it */
	} else {
		cache_miss_routine(cm->addr);
	}
}


/* Routine to follow when receiving a 'write' message from the upper level */
void AssociativeCache::handle_msg_write_prev(cache_message *cm)
{
	status.push(AssCacheStatus::WRITE_UP);
	
	// Save the victim address if predetermined by upper level
	is_vict_predet = (cm->victim.data != nullptr);
	if (is_vict_predet)
		vict_predet_addr = cm->victim.addr;
	
	for (auto &dc : ways) {
		/* TODO: craft write request to direct cache
		 * TODO: dispatch message 
		 * */
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
void AssociativeCache::handle_msg_write_inner(dir_cache_message *cm, unsigned way_idx)
{
	/* TODO ... */
}


AssociativeCache::AssociativeCache(System& sys, string name, string prev_name, string next_name,
			unsigned n_ways, unsigned cache_size, unsigned block_size, unsigned mem_unit_size,
			WritePolicy write_policy, AllocationPolicy alloc_policy, 
			ReplacementPolicy repl_policy, int priority)
	: module(name, priority),
	  prev_name(prev_name),
	  next_name(next_name),
	  n_ways(n_ways),
	  cache_size(cache_size),
	  block_size(block_size),
	  mem_unit_size(mem_unit_size),
	  write_policy(write_policy),
	  alloc_policy(alloc_policy),
	  repl_policy(repl_policy)
{
	std::cout << getName() << ": building associative cache" << std::endl;	// DEBUG
	for (unsigned i = 0; i < n_ways; ++i) {
		DirectCache *dcache = new DirectCache(name + "_" + std::to_string(i));
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
		dir_cache_message *cm = (dir_cache_message *)m->magic_struct;
		
		switch (cm->op) {
			case LOAD:
				handle_msg_read_inner(cm, way_idx);
				break;
			case STORE:
				handle_msg_write_inner(cm, way_idx);
				break;
		}
	} else {
		std::cerr << "Error: can't recognize message sender" << std::endl;
	}
}
