#include "unit_fault.h"
#include <iostream>
using namespace std;

unit_fault::unit_fault() {
	num_bits = 0;
	num_pruned_bits = 0;
	unit = "";
	is_addr = false;
	high = 63;
	low = 0;
	is_def = false;
	extra_count = 0;
	def_pc = "0x0";
	stats.masked = 0;
	stats.sdcs = 0;
	stats.detected = 0;
	stats.equivalent = 0;
	stats.remaining = 0;
}

unit_fault::unit_fault(int num_bits, string unit, bool is_address, bool is_def) { 
	this->num_bits = num_bits;
	this->unit = unit;
	this->is_addr = is_address;
	this->num_pruned_bits = num_bits;
	this->high = num_bits-1;
	this->low = 0;
	this->is_def = is_def;
	this->extra_count = 0;
	this->def_pc = "0x0";
	this->stats.masked = 0;
	this->stats.sdcs = 0;
	this->stats.detected = 0;
	this->stats.equivalent = 0;
	this->stats.remaining = 0;
}

bool unit_fault::is_address() {
	return is_addr;
}

int unit_fault::get_num_bits() {
	return num_bits;
}

int unit_fault::get_pruned_bits() {
	//return (high-low+1);
	return num_pruned_bits;
}

string unit_fault::get_unit() {
	return unit;
}

// return true if something is printed
// return false if nothing is printed
bool unit_fault::print_unit_fault(fstream & output_stream) {
	if(num_bits != 0) {
		output_stream << " (" << unit << ", " << num_bits << ", " << is_def<< ")"; 
		return true;
	}
	return false;
}

// return true if something is printed
// return false if nothing is printed
bool unit_fault::print_pruned_unit_fault(fstream & output_stream) {
	if(num_pruned_bits != 0) {
		output_stream << " (" << unit << ", " << num_pruned_bits << ", " << is_addr << ", " << is_def << ", " << extra_count << ", " << def_pc << ")";
		return true;
	}
	return false;
}

bool unit_fault::print_pruned_unit_fault_extended(fstream & output_stream) {
	if(num_pruned_bits != 0) {
		output_stream << " (" << unit << ", " << num_pruned_bits << ", " << is_addr << ", " << is_def << ", " << extra_count << ", " << def_pc << "," << num_bits << ")";
		return true;
	}
	return false;
}
void unit_fault::set_pruned_bits(int pruned_bits, prune_category c) {
	if(pruned_bits > num_pruned_bits) {
		return; 
	}
	if(c == MASKED) {
		stats.masked += num_pruned_bits - pruned_bits;
	} else if (c == SDCS) {
		stats.sdcs += num_pruned_bits - pruned_bits;
	} else if (c == DETECTED)  {
		stats.detected += num_pruned_bits - pruned_bits;
	} else if (c == EQUIVALENT) {
		stats.equivalent += num_pruned_bits - pruned_bits;
	} 

	num_pruned_bits = pruned_bits;
}

void unit_fault::accumulate(statistics &aggr_stats, int c_line_freq, int rem_c_line_freq) {
	aggr_stats.masked += (stats.masked*(uint64_t)c_line_freq);
	aggr_stats.sdcs += (stats.sdcs*(uint64_t)c_line_freq);
	aggr_stats.detected += (stats.detected*(uint64_t)c_line_freq);
	aggr_stats.equivalent += (stats.equivalent*(uint64_t)c_line_freq);
	aggr_stats.equivalent += num_pruned_bits*((uint64_t)(c_line_freq-rem_c_line_freq)); // special because of loop flattening
}

bool unit_fault::is_unit_fault_pruned() {
	if(num_pruned_bits == 0)
		return true;
	return false;
}

void unit_fault::prune_uses() {
	if(!is_def) {
		set_pruned_bits(0, EQUIVALENT);
	}
}

bool unit_fault::get_is_def() {
	return is_def;
}

void unit_fault::mark_for_extra_counting(int count, string pc) {
	extra_count += count;
	def_pc = pc;
}

int unit_fault::get_extra_count() {
	return extra_count;
}
