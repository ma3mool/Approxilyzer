#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <set>
#include <vector>
#include <iostream>
#include <fstream>
#include "unit_fault.h"
#include "bb.h"

#define NUM_OPS 3

class bb_t;
class cfg_t;

// struct ltstring
// {
// 	bool operator()(std::string s1, std::string s2) const {
// 		return strcmp(s1.c_str(), s2.c_str()) < 0;
// 	}
// };

class instruction_t {
	int c_line_frequency;
	int c_line_number;
	int asm_line_number;
	bool is_function_name;
	std::string program_counter;
	std::string opcode;
	std::string function_name;
	std::string op[NUM_OPS]; 
	int pruned_c_line_frequency;
	std::string equivalent_pc;
	std::string def;
	std::string leading_instruction_pc;
	int leading_index;
	int dep_type;

	//for call site
	std::vector<std::string> params_list;
	std::vector<std::string> ret_val_list;

	//fault
	std::vector<unit_fault> fault_list;
	std::string program_name;
	std::string app_name;

	//basic block
	bb_t * bb;

	cfg_t * call_cfg;

	public:
	instruction_t();
	instruction_t(std::string program_name, std::string app_name, int c_line_frequency, int c_line_number, int asm_line_number, std::string program_counter, std::string opcode, std::string *op, std::vector<std::string> p_list, std::vector<std::string> ret_val_list); 
	instruction_t(std::string program_name, std::string app_name, int c_line_frequency, int c_line_number, int asm_line_number, std::string function_name);

	void create_fault_list();
	double count_fault_list(bool original, int fm);
	bool is_global_address();
	bool is_stack_address();
	bool find_use(std::string local_op);
	int remove_from_fault_list(std::string local_op, bool is_def);
	void mark_for_extra_counting(std::string reg, int count, std::string pc);
	void mark_def_for_extra_counting();
	int get_extra_count(std::string reg, bool is_def);
	void prune_bits(std::string reg, bool is_def, int s, int e);
	void prune_g0_faults();
	bool has_const_op();
	bool is_op0_removed();
	bool is_def_removed();
	void accumulate_fault_stats(statistics &aggr_stats);
	bool is_instruction_pruned();
	bool is_register (std::string op);
	bool is_condition_code (std::string op);
	bool is_int_register (std::string op);
	bool is_floating_point_register (std::string op);
    bool is_single_fp_register (std::string op);
    bool is_double_fp_register (std::string op);
	bool is_const(std::string op);

	void prune_shift_const();
	void prune_addresses();
	void prune_branch_targets();

	int get_asm_line_number();
	int get_c_line_number();
	int get_c_line_frequency();
	std::string get_op(int op_num);
	std::string get_def();
	std::set<std::string> get_uses();
    std::set<std::string> get_duplicate_uses();
	bool is_def(std::string op);
	std::string get_reg(std::string, int);
	std::string get_program_counter();
	std::string get_function_name();
	int get_num_call_parameters();
	std::string get_call_parameter(int i);
	int get_num_ret_parameters();
	std::string get_ret_parameter(int i);

	void set_call_cfg(cfg_t * c);
	cfg_t * get_call_cfg();

	void set_equivalent_pc(std::string pc);
	void set_pruned_c_line_frequency(int num);
	int get_pruned_c_line_frequency();
	void set_c_line_frequency(int num);
	std::string get_opcode();
	int get_fault_bits(std::string op);
	void set_fault_bits(std::string op, int bits, int is_val = 0);
	void set_leading_store_pc(std::string pc, dep_type_t dep_type, int l_index);
	void force_leading_store_pc(std::string pc, dep_type_t dep_type, int l_index);
	std::string get_leading_store_pc();
	int find_function_name(std::string function_name);

	void make_nop();
	bool is_load_op();
	bool is_store_op();
	bool is_memory_op();
	bool is_shift_op();
	bool is_branch_op();
	bool is_branch_always();
	bool is_branch_with_annul_bit();
	bool is_branch_conditional();
	bool is_compare_op();
	bool is_fcompare_op();
	bool is_logical_op();
	bool is_arithmetic_op();
	bool is_add_op();
	bool is_convert_fp_op();
	bool is_mov_op();
	bool is_movcc_op();
	bool is_nop();
	bool is_prefetch();
	bool is_ret();
	bool is_save();
	bool is_restore();
	bool is_call_site();
	bool is_illtrap();
	bool is_func_name();
	bool is_sethi_op();
	bool is_clr();
	bool is_clr_reg();
	bool is_clr_mem();
	bool is_jump();
	bool is_jmpl();
	bool is_orcc();
	bool is_addcc();
	bool is_read_state_reg();
	bool is_write_state_reg();
	bool is_conditional_op();

	std::string get_branch_target();
	std::string get_call_target();

	bb_t * get_bb();
	void set_bb(bb_t *);
	void mark_instruction(std::string pc, dep_type_t dep_type, int distance);
	void mark_instruction_force(std::string pc, dep_type_t dep_type, int distance);
	int get_dep_type();
	void prune_uses();

	void print_fault_list(bool original, std::fstream &output_stream, int fm);
	void print_instruction();
};

#endif //INSTRUCTION_H
