#ifndef __DEF_USE_CHAIN_H__
#define __DEF_USE_CHAIN_H__

#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include "globals.h"
#include "emerald_api.h"
#include <list>

#define MAX_EQUIVALENT_CLASS_SIZE 750

#define NUM_PCS 10

#ifdef PC_LIST_AFTER
#define PC_LIST_SIZE 10
#endif

enum {READ, WRITE, NONE};
enum {LOAD, STORE};

typedef std::list<uint64> pc_list_t;

typedef uint32 pc_type_t;

struct struct_use_t {
//	uint64 cycle;
	pc_type_t pc;
#ifdef PC_LIST_BEFORE
#ifdef USE_HASH
	uint64 pc_hash;
#else // USE_HASH
	uint32 pc_list[NUM_PCS];
#endif // USE_HASH
#endif
};

typedef struct struct_use_t use_t;

class def_use_chain_t {
public:
	def_use_chain_t();
	def_use_chain_t(uint64 pc, uint64 logical_address, uint64 cycle);
	~def_use_chain_t();
	uint64 get_pc();
	//uint64 get_logical_address();
	uint64 get_cycle();
	int get_use_count();
	uint64 get_first_use_pc();
#ifdef PC_LIST_BEFORE
#ifdef USE_HASH
	int add_use(uint64 pc, uint64 cycle, uint64 pc_sum);
#else // USE_HASH
	int add_use(uint64 pc, uint64 cycle, uint32* pc_list);
#endif // USE_HASH
#else // PC_LIST_BEFORE
	int add_use(uint64 pc, uint64 cycle);
#endif
	void print_def_use_chain();
	std::string to_string_key();
	bool is_chain_equal(def_use_chain_t *element);
	void add_cycle(uint64 cycle);
	use_t get_use(int i);
	bool is_def_done();
	void set_def_done();
	void print_equalized_class(FILE * classfp);
	uint64 get_population();
	void init(uint64 pc, uint64 logical_address, uint64 cycle);
private:
	uint64 pc; 
	//uint64 logical_address;
	uint64 cycle;
	bool def_done;
	uint64 population;
	std::vector <use_t> uses;
	std::list <uint64> equalized_cycles;
	use_t temp_use;
};

#endif // __DEF_USE_CHAIN_H__
