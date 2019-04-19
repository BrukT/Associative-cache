#include <iostream>

#include "mock_direct_cache.hpp"


DirectCache::DirectCache(string name, int priority)
	: module(name, priority)
{
	std::cout << getName() << ": building direct cache" << std::endl;
}


void DirectCache::onNotify(message* m)
{
	std::cout << getName() << ": was notified" << std::endl;
}