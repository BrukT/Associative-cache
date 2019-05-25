#include <iostream>
#include "test/testable_associative_cache.hpp"

TestableAssociativeCache::TestableAssociativeCache(System& sys, string name, string upper_name, string lower_name,
			unsigned n_ways, unsigned cache_size, unsigned block_size, unsigned mem_unit_size,
			WritePolicy wp, AllocationPolicy ap, ReplacementPolicy rp, int prio)
	: AssociativeCache(sys, name, upper_name, lower_name, n_ways, cache_size, block_size, mem_unit_size, wp, ap, rp, prio)
{
	std::cout << "Allocated testable version of AssociativeCache" << std::endl;
}


void TestableAssociativeCache::push_stack(AssociativeCache::AssCacheStatus sts)
{
	this->status.push(sts);
}


AssociativeCache::AssCacheStatus TestableAssociativeCache::top_stack()
{
	return this->status.top();
}


void TestableAssociativeCache::pop_stack()
{
	this->status.pop();
}


void TestableAssociativeCache::put_message(message *m)
{
	this->onNotify(m);
}


cache_message * TestableAssociativeCache::create_outprot_payload(
		bool op, addr_t taddr, word_t *tdata, addr_t vaddr, word_t *vdata)
{
	cache_message *msg = new cache_message();
	
	msg->op_type = op;
	msg->target.addr = taddr;
	msg->target.data = tdata;
	msg->victim.addr = vaddr;
	msg->victim.data = vdata;

	return msg;
}


message * TestableAssociativeCache::create_outprot_message(
		string src, string dest, int time, bool op, addr_t taddr, word_t *tdata, addr_t vaddr, word_t *vdata)
{
	message *msg = new message();
	
	void *content = (void*)create_outprot_payload(op, taddr, tdata, vaddr, vdata);

	msg->valid = 1;
	msg->timestamp = time;
	strncpy(msg->source, src.c_str(), 10);
	strncpy(msg->dest, dest.c_str(), 10);
	msg->magic_struct = content;
	
	return msg;
}

message * TestableAssociativeCache::create_inprot_reply_message(
		string src, string dest, int time, bool hit_flag, addr_t addr, word_t *data, WriteResponse wr)
{
	message *msg = new message();

	CWP_to_SAC *reply = new CWP_to_SAC{hit_flag, data, addr, wr};

	void *content = (void*)reply;

	msg->valid = 1;
	msg->timestamp = time;
	strncpy(msg->source, src.c_str(), 10);
	strncpy(msg->dest, dest.c_str(), 10);
	msg->magic_struct = content;

	return msg;
}