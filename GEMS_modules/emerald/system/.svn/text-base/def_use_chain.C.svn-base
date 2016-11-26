#include <stdio.h>
#include <iostream>
#include "def_use_chain.h"

#define EXPECTED_USE_CHAIN_SIZE 10

using namespace std;

def_use_chain_t::def_use_chain_t()
{
	pc = 0x0;
	//logical_address = 0x0;
	cycle = 0x0;
	def_done = false;
	population = 0;
	uses.reserve(EXPECTED_USE_CHAIN_SIZE);
}

def_use_chain_t::def_use_chain_t(uint64 pc, uint64 logical_address, uint64 cycle)
{
	this->pc = pc;
	//this->logical_address = logical_address;
	this->cycle = cycle;
	def_done = false;
	population = 0;
	uses.reserve(EXPECTED_USE_CHAIN_SIZE);
}

void def_use_chain_t::init(uint64 pc, uint64 logical_address, uint64 cycle)
{
	this->pc = pc;
	//this->logical_address = logical_address;
	this->cycle = cycle;
	def_done = false;
	population = 0;
	uses.clear();
	uses.reserve(EXPECTED_USE_CHAIN_SIZE);
}

def_use_chain_t::~def_use_chain_t()
{
	uses.clear();
}

uint64 def_use_chain_t::get_pc()
{
	return pc;
}

//uint64 def_use_chain_t::get_logical_address()
//{
//	return logical_address;
//}

uint64 def_use_chain_t::get_cycle()
{
	return cycle;
}

int def_use_chain_t::get_use_count()
{
	return uses.size();
}

uint64 def_use_chain_t::get_first_use_pc()
{
	if(uses.size() > 0) {
		uint64 local_pc = 0x100000000ULL | (uint64) uses[0].pc; 
		return local_pc; 
	} else 
		return 0;
}

#ifdef PC_LIST_BEFORE
#ifdef USE_HASH
int def_use_chain_t::add_use(uint64 pc, uint64 cycle, uint64 pc_hash)
#else // USE_HASH
int def_use_chain_t::add_use(uint64 pc, uint64 cycle, uint32 *pc_list)
#endif // USE_HASH
#else
int def_use_chain_t::add_use(uint64 pc, uint64 cycle)
#endif
{
	//temp_use.cycle = cycle;
	temp_use.pc = (pc_type_t) (pc & 0xFFFFF);
	//temp_use.pc = pc;

#ifdef PC_LIST_BEFORE

#ifdef USE_HASH
	temp_use.pc_hash = pc_hash;
#else // USE_HASH
	for(int i=0; i<NUM_PCS; i++)
		temp_use.pc_list[i] = pc_list[i];
#endif // USE_HASH

#endif // PC_LIST_BEFORE
	uses.push_back(temp_use);
	return uses.size();
}

void def_use_chain_t::print_def_use_chain()
{
	fprintf(m_outfp, "\n%d:", def_done);
	fprintf(m_outfp, "%lld:U-", cycle);
	if(uses.size() > 0) {
		for(int i=0; i<uses.size(); i++)
			fprintf(m_outfp, "%x ", uses[i].pc);
	}
}

string def_use_chain_t::to_string_key()
{
#ifdef PC_LIST_BEFORE
	cout << " Please do not use PC_LIST_BEFORE\n";
	exit(1);
#endif
#ifdef USE_HASH
	cout << " Please do not use use USE_HASH\n";
	exit(1);
#endif
	string ret_str="";
	if(uses.size() > 0) {
		for(int i=0; i<uses.size(); i++) {
			uint64 local_copy = uses[i].pc;
			while(local_copy != 0) {
				ret_str += (char) (local_copy & 0xFF);
				local_copy = local_copy >> 8;
			}
		}
	}
	return ret_str;
}

bool def_use_chain_t::is_chain_equal(def_use_chain_t * element)
{
	if(!def_done) 
		return false;
	if((uses.size() == element->get_use_count())) {
		for(int i=0; i<uses.size(); i++) {
			if(uses[i].pc != element->get_use(i).pc)
				return false;
#ifdef PC_LIST_BEFORE

#ifdef USE_HASH
			if(uses[i].pc_hash != element->get_use(i).pc_hash)
				return false;
#else // USE_HASH
			for(int j=0; j<NUM_PCS; j++)
				if(uses[i].pc_list[j] != element->get_use(i).pc_list[j])
					return false;
#endif // USE_HASH
#endif // PC_LIST_BEFORE
		}
		return true;
	}
	return false;
}

use_t def_use_chain_t::get_use(int i)
{
	if(uses.size() < i)
		exit(1);
	return uses[i];
}

void def_use_chain_t::set_def_done()
{
	def_done = true;
}

bool def_use_chain_t::is_def_done()
{
	return def_done;
}

void def_use_chain_t::add_cycle(uint64 cycle)
{
	population++;
	if(equalized_cycles.size() >= MAX_EQUIVALENT_CLASS_SIZE) {
		// add this cycle only 5% of times - this makes sure that the
		// sample is not biased towards the end of the population
		int pass_percent = 5;
		if(population > 3000)
			pass_percent = 1;
		if (rand()%100 <= pass_percent) { 
			equalized_cycles.push_back(cycle);
			int remove_index = rand()%MAX_EQUIVALENT_CLASS_SIZE;
			if(remove_index != 0) {
				remove_index--;
			}

			list<uint64>::iterator it = equalized_cycles.begin();
			for(int j=0; j<remove_index; j++) {
				it++;
			}
			equalized_cycles.erase(it);
		}
	} else {
		equalized_cycles.push_back(cycle);
	}
}
void def_use_chain_t::print_equalized_class(FILE * classfp)
{
	list<uint64>::iterator it; 
	for(it=equalized_cycles.begin(); it!=equalized_cycles.end(); it++) {
		fprintf( classfp, "%lld ", (*it) );
	}
}

uint64 def_use_chain_t::get_population()
{
	return population;
}
