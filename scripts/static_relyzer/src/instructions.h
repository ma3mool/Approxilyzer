#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <string>
#include <vector>
#include <map>
#include "instruction.h"
#include "func_list.h"

#define NUM_OPS 3

class instructions {
	std::vector<instruction> program;

	std::map<std::string, bool> touched;

	std::map<std::string, int> label_map;

	std::string app_name;
	std::string program_name;

	public:
	instructions(std::string prg_name, std::string a_name);
	bool populate_instructions(std::string prg_name, std::string a_name);
	void create_fault_set();

	bool can_be_flattened(int instr);
	bool is_within_loop(int instr);
	bool is_outside_loop(int instr);
	int find_def(std::string use, int use_instr);
	bool adjust_bounds(int instr1, int instr2);
	int get_block_instr(std::string label);
	void mark_dependent_instructions(int pi, std::string op, std::string leading_store_pc, dep_type_t dep_type);
	std::string get_reg(std::string, int);
	bool check_read_before_write(int program_index, std::string op, bool main);
	void process_line_frequency();
	void call_site_flattening(func_list_t* func_list);
	void ret_site_flattening(func_list_t* func_list);
	bool update_func_list(func_list_t* func_list, int f_i);
	void get_func_list(func_list_t * func_list);
	std::string get_app_name();
	std::string get_program_name();

	// passes on the program
	void prune_loop_flattening();
	void prune_addresses();
	void prune_branch_targets();
	void prune_use_defs();
	void prune_shift_consts();
	void prune_const_propagation();
	void set_leading_store_pcs();
	
	//stats and printing
	double count_fault_set( bool original);
	void print_fault_set(bool original);
	void print_pruned_categories();
};

#endif //INSTRUCTIONS_H
