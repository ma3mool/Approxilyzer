#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <stdio.h>
#include <iostream>
#include "globals.h"
#include "emerald_api.h"
#include "def_use_chain.h"
#include <map>
#include <list>
#include <vector>
#include <ctime>
#include <set>
#include <ext/hash_map>

#define NUM_PCS 10

// #define LIMIT_NUM_LOADS
// #define MAX_TRACKING_LOADS 10
// #define USE_LIST
#define MEM_OPTIMIZATION 1

class def_use_chain_t;

struct ltuint64 {
  bool operator()(uint64 ui1, uint64 ui2) {
    return ui1 < ui2;
  }
};

struct equint64 {
  bool operator()(uint64 ui1, uint64 ui2) {
    return ui1 == ui2;
  }
};



//list is more efficient because there is no need for the adjacent elements to sit together
#ifdef USE_LIST
typedef std::list<def_use_chain_t*> chain_t;
#else 
typedef std::vector<def_use_chain_t*> chain_t;
#endif

typedef	std::map<uint64, chain_t* , ltuint64>  chains_t;
// typedef	__gnu_cxx::hash_map<uint64, chain_t, __gnu_cxx::hash<uint64_t>, equint64>  chains_t;


class system_t {
public:

	system_t() ;

	uint64 operate(conf_object_t *obj, conf_object_t *space, map_list_t *map, generic_transaction_t *g) ;

	// Functions relevant to checkpointing
	void printStats() ;
	void printHeapUseCounts() ;
	void analyzeChains() ;
	chain_t::iterator system_t::removeDuplicates(int bs, chains_t::iterator pc_it, chain_t::iterator start_it);
	void updateDefUseChains(generic_transaction_t *g, int is_physmem);
	void profileAddress(generic_transaction_t *g, int is_physmem);
	void testCheckpointSize(generic_transaction_t *g, int is_physmem);
	void initialize() ;
	void add(uint64 current_pc, uint64 logical_address, uint64 cycle_count, int type);
	void printHeartBeat(cycles_t);
	void setAddressSpace(int);
	void textStart(uint64);
	void textEnd(uint64);
	void recordTextStart(uint64);
	void recordTextEnd(uint64);
	void removeFromPcChains(chains_t::iterator it);
	uint64 get_pc_hash();
	def_use_chain_t * create_def_use_chain_elem();
private:

	uint64 min_stack;
	uint64 max_stack;
	uint64 max_heap;
	uint64 min_heap;
	uint64 num_heap_accesses;
	uint64 num_equalized;
	uint64 local_cycles;
	int num_defs;
	int num_uses;
	int use_count_sum;
	uint64 num_cycles;
	time_t start, end;
	def_use_chain_t * pool;
	int pool_index;

	chains_t  address_chains;
	chains_t  pc_chains;
	uint32 pc_list[NUM_PCS];
	int pc_list_index;
	uint64 pc_sum;
	int num_writes;
	std::set<uint64> store_addresses;
	std::set<uint64> store_orig_addresses;
	std::set<uint64> load_addresses;

} ;
#endif // __SYSTEM_H__
