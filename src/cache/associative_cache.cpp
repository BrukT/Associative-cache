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
message * AssociativeCache::craft_msg(char *dest, void *content)
{
	message *msg = new message();
	
	msg->valid = 1;
	msg->timestamp = getTime();
	strncpy(msg->source, getName().c_str(), 10);
	strncpy(msg->dest, dest, 10);
	msg->magic_struct = content;
	
	return msg;
}


/* Routine to follow when a cache miss is generated */
void AssociativeCache::cache_miss_routine(addr_t addr)
{
	/* TODO */
}


/* Routine to follow when receiving a 'read' message from the upper level */
void AssociativeCache::handle_msg_read_prev(cache_message *cm)
{
	bool is_size_word;	// 0: block; 1: word
	
	status.push(AssCacheStatus::READ_UP);
	
	/* TODO: determine size of the read requests (depends on mem_unit_size attribute) */
	for (auto &dc : ways) {
		/* TODO: craft read request to direct cache
		 * TODO: dispatch message 
		 * */
	}
	
	if (is_size_word) 
		status.push(AssCacheStatus::READ_WORD_IN);
	else /* read size is a block */
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
void AssociativeCache::handle_msg_read_inner(cache_message *cm)
{
	bool hit;
	/* TODO: store the content of the message (for future use) */
	/* TODO: if this is not the last reply (i.e. we expect more responses), return */
	/* TODO: compute hit/miss on the basis of ALL the received replies */
	if (hit) {
		/* TODO: craft response message for previous level */
		/* TODO: send it */
	} else {
		cache_miss_routine(cm->target.addr);
	}
}


/* Routine to follow when receiving a 'write' message from the upper level */
void AssociativeCache::handle_msg_write_prev(cache_message *cm)
{
	status.push(AssCacheStatus::WRITE_UP);
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
	
	/* TODO: propagate ack message to previous level (same fields) */
}


/* Routine to follow when receiving a 'write' message from a nested direct cache */
void AssociativeCache::handle_msg_write_inner(cache_message *cm)
{
	/* TODO ... */
}


AssociativeCache::AssociativeCache(System& sys, string name, string prev_name, string next_name,
			unsigned n_ways, unsigned cache_size, unsigned block_size, unsigned mem_unit_size,
			bool write_policy, bool allocate_policy, int priority)
	: module(name, priority),
	  prev_name(prev_name),
	  next_name(next_name),
	  n_ways(n_ways),
	  cache_size(cache_size),
	  block_size(block_size),
	  mem_unit_size(mem_unit_size),
	  write_policy(write_policy),
	  allocate_policy(allocate_policy)
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

	// Demultiplex based on source
	if (prev_name.compare(m->source) == 0) {
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
	} else if (next_name.compare(m->source) == 0) {
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
	} else if (getName().compare(0, getName().size(), m->source, 0, getName().size()) == 0) {
		// inner direct cache
		/* TODO */
	} else {
		// unknown
		std::cerr << "Error: bad message source" << std::endl;
	}
	// Check the message type (LOAD, STORE or RESPONSE)
	// If LOAD or STORE, forward the request to the inner direct caches 
	// If RESPONSE, check if HIT/MISS/ACK and handle it accordingly
	// Send a message to previous/next level
}
