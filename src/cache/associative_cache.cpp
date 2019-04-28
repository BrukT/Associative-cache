#include <cstring>
#include <iostream>

#include "associative_cache.hpp"


ass_cache_msg * AssociativeCache::craft_ass_cache_msg(bool op, mem_unit tgt, mem_unit vcm)
{
	ass_cache_msg *msg = new ass_cache_msg();
	
	msg->op_type = op;
	msg->target = tgt;
	msg->victim = vcm;

	return msg;
}


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


AssociativeCache::AssociativeCache(string name, unsigned n_ways,
		std::vector<DirectCache*> ways, int priority)
	: module(name, priority),
	  n_ways(n_ways),
	  ways(ways)
{
	std::cout << getName() << ": building associative cache" << std::endl;	// DEBUG 
}


void AssociativeCache::onNotify(message* m)
{
	std::cout << getName() << ": was notified" << std::endl;	// DEBUG
	
	/* Pseudocode */
	// Check if this is the recipient of the message (if not, exit)
	// Check the message type (LOAD, STORE or RESPONSE)
	// If LOAD or STORE, forward the request to the inner direct caches 
	// If RESPONSE, check if HIT/MISS/ACK and handle it accordingly
	// Send a message to previous/next level
}
