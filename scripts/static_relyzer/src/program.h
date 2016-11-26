#ifndef PROGRAM_H
#define PROGRAM_H

#include <string>
#include <vector>
#include <map>
#include "instruction.h"
#include "cfg.h"
#include "bb.h"
#include "config.h"

class instruction_t;
class cfg_t;
class bb_t;

class program_t {
	std::vector<instruction_t> program; // a program is a list of instructions

	std::map<std::string, int> function_name_map;
	std::map<std::string, cfg_t*> function_cfg_map; // function name is key
	std::map<std::string, cfg_t*> pc_cfg_map; // first_instr_pc is key
	std::map<std::string, std::string> arcs;  // control flow edges from_pc -> to_pc

	std::map<std::string, int> pc_map;

	std::string app_name;
	std::string program_name;
	std::ofstream exit_points_stream;

	public:
	program_t(std::string prg_name, std::string a_name);
	~program_t();
	bool populate_instructions(std::string prg_name, std::string a_name);
	void create_fault_set();

	bool can_be_flattened(int instr);
	bool is_within_loop(int instr);
	bool is_outside_loop(int instr);
	int find_def(std::string use, int use_instr);
	bool adjust_bounds(int instr1, int instr2);
	int get_block_instr(std::string label);
	//void mark_dependent_instructions(int pi, std::string op, std::string leading_store_pc, dep_type_t dep_type, int l_index);
	//bool check_read_before_write(int program_index, std::string op, bool main);
	void process_line_frequency();
	std::string get_app_name();
	std::string get_program_name();
	void get_instruction_distribution();

	void create_cfg();
	void live_reg_analysis();
	void mark_unlive_registers();
	cfg_t * get_cfg(std::string str);
	bb_t * get_cfg_start(std::string str);
	void gen_control_patterns();
	
	void check();

	// passes on the program
	void find_invariant_detectors(); 
	void prune_loop_flattening();
	void prune_addresses();
	void prune_branch_targets();
	void prune_g0_faults();
	void prune_defs();
	void prune_shift_consts();
	void prune_const_propagation();
	void prune_use_in_const_instr();
	void mark_store_instructions();
	void mark_control_instructions();
	void mark_other_instructions();
	void mark_for_extra_counting(std::string reg);
	
	//stats and printing
	double count_fault_set( bool original, int fm);
	void print_fault_set(bool original, int fm);
	void print_pruned_categories();
	void collect_arcs();
	void open_exit_points_file(); 
	void close_exit_points_file(); 
};

#endif //PROGRAM_H
