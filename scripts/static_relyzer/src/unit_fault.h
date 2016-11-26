#ifndef UNIT_FAULT_H
#define UNIT_FAULT_H
#include <string>
#include <iostream>
#include <stdint.h>
#include <fstream>

enum prune_category {
	MASKED,
	SDCS,
	DETECTED,
	EQUIVALENT,
	REMAINING
};

struct _statistics {
	uint64_t masked;
	uint64_t sdcs;
	uint64_t detected;
	uint64_t equivalent;
	uint64_t remaining;
};

typedef struct _statistics statistics;

class unit_fault {
	int num_bits;
	int num_pruned_bits; // bits remaining after pruning 
	std::string unit;
	bool is_addr;
	int high;
	int low;
	statistics stats;
	bool is_def;
	int extra_count;
	std::string def_pc;

	public:
	unit_fault();
	unit_fault(int num_bits, std::string unit, bool is_address, bool is_def);
	bool is_address();
	bool print_unit_fault(std::fstream & output_stream);
	bool print_pruned_unit_fault(std::fstream & output_stream);
	bool print_pruned_unit_fault_extended(std::fstream & output_stream);
	void set_pruned_bits(int pruned_bits, prune_category c);
	int get_num_bits();
	int get_pruned_bits();
	std::string get_unit();
	void accumulate(statistics &aggr_stats, int c_line_freq, int rem_c_line_freq);
	bool is_unit_fault_pruned();
	void prune_uses();
	bool get_is_def();
	void mark_for_extra_counting(int count = 1, std::string def_pc = "0x0");
	int get_extra_count();
};

#endif // UNIT_FAULT_H
