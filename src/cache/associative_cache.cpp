#include <iostream>

#include "associative_cache.hpp"


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
