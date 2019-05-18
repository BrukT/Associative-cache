#include "mock_write_policy.hpp"
#include "messages.hpp"

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

/**
Constructor of the class
@param name 		Name of the Cache write policies module for the orchestrator
@param priority		Priority of the Cache write policies module for the orchestrator
@param cache_size 	size of the cache module to instanciate(in bytes)
@param line_size	sieze of a line of cache (in bytes)
@param hp			miss policy to be followed by the module
@param mp			hit policy to be followed by the module
*/
CacheWritePolicies::CacheWritePolicies(string name, int priority, uint16_t cache_size, uint16_t line_size, HIT_POLICY hp, MISS_POLICY mp) : Cache(line_size*MEM_LINE/2, cache_size*MEM_LINE/2), module(name, priority) {
	hit_policy = hp;
	miss_policy = mp;
}


/**
Function that is called by the orchestrator whenever a message arrives
@param m	The structure is the message that is sent by the module that wants to communicate with us
*/
void CacheWritePolicies::onNotify(message *m) {
	
	//Don't forget to check if the message was for me since we are in a broadcast environment
	if(m->dest != getName())
		return;

	#ifdef DEBUG
	//print a message to log the message delivery
	cout << "Message " << m->id << ": Sent at " << m->timestamp << ", current timestamp " << getTime() << ". My name is " << getName() << "." << endl;
	#endif
	
	//convert the magic struct to our type
	SAC_to_CWP* request_struct = SAC_TO_CWP(m->magic_struct);
	string sender_name = string(m->source);
	//allocate the pointer to the response magic struct which will be created by the functions handling each particular case
	CWP_to_SAC* response_struct = NULL;

	//depending on the operation to perform, we call a specific function (that creates the response struct according to its actions)
	switch(request_struct->op_type) {
		
		case DirCacheOp::SET_DIRTY:
			response_struct = WP_set_dirty(request_struct);
			break;
		case DirCacheOp::CHECK_DIRTY:
			response_struct = WP_check_dirty(request_struct);
			break;
		case DirCacheOp::CHECK_VALIDITY_DIRTY:
			response_struct = WP_check_validity_dirty(request_struct);
			break;
		case DirCacheOp::CHECK_DATA_VALIDITY:
			response_struct = WP_check_data_validity(request_struct);
			break;
		case DirCacheOp::INVALID_LINE:
			response_struct = WP_invalid_line(request_struct);
			break;
		case DirCacheOp::LOAD:
			response_struct = WP_load(request_struct);
			break;
		case DirCacheOp::STORE:
			response_struct = WP_store(request_struct);
			break;		
		case DirCacheOp::WRITE_WITH_POLICIES:
			response_struct = WP_write_with_policies(request_struct);
			break;			
		default:
			cout << "Cache write policies: Bad request type" << endl;
			break;
	}
	
	#ifdef DEBUG
	cout << "Cache write policies: Operation completed - sending reply..." << endl;
	#endif
	
	// delete the received message and structure (!!!!! CHECK IF IT MUST BE DONE THIS WAY)
	delete request_struct;
	delete m;

	//create the response message
	message* response_message = WP_create_message(sender_name);
	response_message->magic_struct = (void*)response_struct;
	//send the response message
	sendWithDelay(response_message, CWR_RESPONSE_DELAY);
}

/**
Function which handles the set_dirty functionality
@param request_struct	The structure containing the data to fulfill the request
@return 				The pointer to the CWP_to_SAC magic_struct to be included in the response
*/
CWP_to_SAC* CacheWritePolicies::WP_set_dirty(SAC_to_CWP* request_struct) {
	
	#ifdef DEBUG
	cout << "Cache write policies: WP_set_dirty called" << endl;
	#endif
	
	//check if the block is valid
	bool data_valid = check_data_validity(request_struct->address);
	//perform the operation->set to 1 the dirty bit of the desired block (if it's valid)
	if(data_valid)
		set_dirty(request_struct->address, 1);
	
	//create the reply structure
	CWP_to_SAC* response_struct = new CWP_to_SAC();
	response_struct->hit_flag = data_valid;
	response_struct->data = NULL;
	response_struct->address = request_struct->address;
	response_struct->wr = NOT_NEEDED;
	
	return response_struct;
}

/**
Function which handles the check_validity_dirty functionality
@param request_struct	The structure containing the data to fulfill the request
@return 				The pointer to the CWP_to_SAC magic_struct to be included in the response
*/
CWP_to_SAC* CacheWritePolicies::WP_check_validity_dirty(SAC_to_CWP* request_struct) {
	
	#ifdef DEBUG
	cout << "Cache write policies: WP_check_validity_dirty called" << endl;
	#endif
	
	//get the validity of the dta
	bool data_valid_dirty = check_validity_dirty(request_struct->address);
	
	//create the reply structure
	CWP_to_SAC* response_struct = new CWP_to_SAC();
	response_struct->hit_flag = data_valid_dirty;
	response_struct->data = NULL;
	response_struct->address = request_struct->address;
	response_struct->wr = NOT_NEEDED;
	
	return response_struct;
}


/**
Function which handles the check_validity_dirty functionality
@param request_struct	The structure containing the data to fulfill the request
@return 				The pointer to the CWP_to_SAC magic_struct to be included in the response
*/
CWP_to_SAC* CacheWritePolicies::WP_check_data_validity(SAC_to_CWP* request_struct) {
	
	#ifdef DEBUG
	cout << "Cache write policies: WP_check_data_validity called" << endl;
	#endif
	
	bool data_valid = check_data_validity(request_struct->address);
	
	//create the reply structure
	CWP_to_SAC* response_struct = new CWP_to_SAC();
	response_struct->hit_flag = data_valid;
	response_struct->data = NULL;
	response_struct->address = request_struct->address;
	response_struct->wr = NOT_NEEDED;
	
	return response_struct;
}

/**
Function which handles the check_dirty functionality
@param request_struct	The structure containing the data to fulfill the request
@return 				The pointer to the CWP_to_SAC magic_struct to be included in the response
*/
CWP_to_SAC* CacheWritePolicies::WP_check_dirty(SAC_to_CWP* request_struct) {
	
	#ifdef DEBUG
	cout << "Cache write policies: WP_check_dirty called" << endl;
	#endif

	bool dirty = check_dirty(request_struct->address);
	
	//create the reply structure
	CWP_to_SAC* response_struct = new CWP_to_SAC();
	response_struct->hit_flag = dirty;
	response_struct->data = NULL;
	response_struct->address = request_struct->address;
	response_struct->wr = NOT_NEEDED;
	
	return response_struct;
}

/**
Function which handles the invalid_line functionality
@param request_struct	The structure containing the data to fulfill the request
@return 				The pointer to the CWP_to_SAC magic_struct to be included in the response
*/
CWP_to_SAC* CacheWritePolicies::WP_invalid_line(SAC_to_CWP* request_struct) {
	
	#ifdef DEBUG
	cout << "Cache write policies: WP_invalid_line called" << endl;
	#endif
	
	bool data_valid = check_data_validity(request_struct->address);
	
	//invalid the line if it was valid
	if(data_valid)
		invalid_line(request_struct->address);
	
	//create the reply structure
	CWP_to_SAC* response_struct = new CWP_to_SAC();
	response_struct->hit_flag = data_valid;
	response_struct->data = NULL;
	response_struct->address = request_struct->address;
	response_struct->wr = NOT_NEEDED;
	
	return response_struct;
}

/**
Function which handles the load functionality
@param request_struct	The structure containing the data to fulfill the request
@return 				The pointer to the CWP_to_SAC magic_struct to be included in the response
*/
CWP_to_SAC* CacheWritePolicies::WP_load(SAC_to_CWP* request_struct) {
	
	#ifdef DEBUG
	cout << "Cache write policies: WP_load called" << endl;
	#endif
	
	//if the data is valid and it's the same block the caller wants to write on -> HIT
	bool hit = check_data_validity(request_struct->address);

	//create the reply structure
	CWP_to_SAC* response_struct = new CWP_to_SAC();
	response_struct->hit_flag = hit;
	
	if(hit) {
		vector<uint16_t> data_read = load(request_struct->address);
		response_struct->data = new uint16_t[size_line];
		//convert the data to a uint16_t[]
		copy(data_read.begin(), data_read.end(), response_struct->data);
	}
	else { //miss
		response_struct->data = NULL;
	}
	
	response_struct->address = request_struct->address;
	response_struct->wr = NOT_NEEDED;
	
	return response_struct;
}

/**
Function which handles the store functionality
@param request_struct	The structure containing the data to fulfill the request
@return 				The pointer to the CWP_to_SAC magic_struct to be included in the response
*/
CWP_to_SAC* CacheWritePolicies::WP_store(SAC_to_CWP* request_struct) {
	
	#ifdef DEBUG
	cout << "Cache write policies: WP_store called" << endl;
	#endif
	
	bool data_valid = check_data_validity(request_struct->address);
	//convert the data to a vector<uint16_t>
	vector<uint16_t> data_as_vector(request_struct->data, request_struct->data + size_line/MEM_LINE);

	//reset the dirty bit (the functionality is used by the replacement)
	set_dirty(request_struct->address, 0);
	//store the line of cache
	bool res = store(request_struct->address, data_as_vector);

	if(res == false)
		cout << "Cache module returned bad dimension error" << endl;	

	//deallocate data array
	delete request_struct->data;
	
	//create the reply structure
	CWP_to_SAC* response_struct = new CWP_to_SAC();
	response_struct->hit_flag = data_valid;
	response_struct->data = NULL;
	response_struct->address = request_struct->address;
	response_struct->wr = NOT_NEEDED;
		
	return response_struct;
}

/**
Function which handles the write_with_policies functionality
@param request_struct	The structure containing the data to fulfill the request
@return 				The pointer to the CWP_to_SAC magic_struct to be included in the response
*/
CWP_to_SAC* CacheWritePolicies::WP_write_with_policies(SAC_to_CWP* request_struct) {

	#ifdef DEBUG
	cout << "Cache write policies: WP_write_with_policies called" << endl;
	#endif
	
	//create the reply structure
	CWP_to_SAC* response_struct = new CWP_to_SAC();

	//if the data is valid and it's the same block the caller wants to write on -> HIT
	bool hit = check_data_validity(request_struct->address);
	//get the tag of the data currently present in the line of cache
	uint16_t tag = get_tag(request_struct->address);
	
	//convert the data to a vector<uint16_t>
	vector<uint16_t> data_as_vector(request_struct->data, request_struct->data + 1);
	
	if(hit) {
		if(hit_policy == WRITE_BACK)
			response_struct->wr = NO_PROPAGATE;
		else if(hit_policy == WRITE_THROUGH)
			response_struct->wr = PROPAGATE;
		
		response_struct->address = request_struct->address;
		
		set_dirty(request_struct->address, 1);
		//read the old data and modify the word; then write again
		vector<uint16_t> data_read = load(request_struct->address);
		//modify the specific word inside the cache block
		data_read.at(request_struct->address & ~(0xffffffff << offset_size)) = data_as_vector.at(0);
		//rewrite the block
		bool res = store(request_struct->address, data_read);
		if(res == false)
			cout << "Cache write policies: cache module returned bad dimension error" << endl;	
	}
	else { 	//MISS
		if(miss_policy == WRITE_ALLOCATE)
			response_struct->wr = LOAD_RECALL;	
		else if(miss_policy == WRITE_NO_ALLOCATE)
			response_struct->wr = CHECK_NEXT;
		
		bool data_valid = check_used(request_struct->address);
		
		if(!data_valid)		//if the data is not valid, return the same address that was in the request
			response_struct->address = request_struct->address;
		else		//if the data is valid, return return the address of the block curerntly occupying the line
			response_struct->address = (tag << index_size) | resolve_index(request_struct->address);
	}
	
	//deallocate data array
	delete request_struct->data;
	
	response_struct->hit_flag = hit;
	response_struct->data = NULL;
	
	return response_struct;
}

/**
Function which creares the message structure and inserts the data required for the transmission
@param destination 		The name of the dstination module
@return 				pointer to the message struct created
*/
message* CacheWritePolicies::WP_create_message(string destination) {

	message* myMessage = new message();
	
	myMessage->valid = 1;
	myMessage->timestamp = getTime();
	strcpy(myMessage->source, getName().c_str());
	strcpy(myMessage->dest, destination.c_str());
	myMessage->magic_struct = NULL;
	
	return myMessage;
}