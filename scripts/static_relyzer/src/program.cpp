#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <ctype.h>
#include "program.h"

using namespace std;
bool is_number (string);

/* Functions of instructions class */
program_t::program_t(std::string prg_name, std::string ap_name) {
	program_name = "";
	app_name = "";
	program.clear();
	function_name_map.clear();
	function_cfg_map.clear(); // function name is key
	pc_cfg_map.clear(); // first_instr_pc is key
	arcs.clear();
	pc_map.clear();
}
program_t::~program_t() {
 	program_name = "";
 	app_name = "";
 	program.clear();
 	function_name_map.clear();
 	function_cfg_map.clear(); // function name is key
 	pc_cfg_map.clear(); // first_instr_pc is key
 	arcs.clear();
	pc_map.clear();
}

// This is where the initialization is performed
bool program_t::populate_instructions(string prg_name, string a_name) {
	program_name = prg_name;
	app_name = a_name;

#if(UNOPT_APPS) 
	string file_name = "apps/unopt/" + prg_name + "/" + app_name + ".clean.dis";
#else
//	string file_name = "apps/optimized/full_optimized/" + prg_name + "/" + app_name + ".clean.dis";
	string file_name = string(std::getenv("RELYZER_APPS"))  + "/" + prg_name + "/" + app_name + ".clean.dis";
#endif
	if(DEBUG_PARSER)
		cout << "file_name = -" << file_name << "-\n";

	ifstream in_file(file_name.c_str(), ifstream::in);
	if(!in_file.good()) {
		cout << file_name << ": input file is not good loc0\n";
		return false;
	}

	string line = "";
	while(in_file.good()) {
		// input will have the following syntax 
		// For instructions: 1 c_line asm_line pc opcode op1 op2 op3 
		// For labels: 0 c_line asm_line label 

		int flag = -1; 
		in_file  >> flag ;
		if(in_file.eof()) 
			break;
		if(flag > 2) {  // for debugging
			// 1- instr, 0- label, 2- function_name 
			cout << "something is wrong: flag=" << flag << endl;
			exit(-1);
		}

		if(flag == 1) { // instruction

			int c_line_frequency, c_line_number, asm_line_number;
			c_line_frequency = 0;
			string program_counter, opcode, op[NUM_OPS];
			// c_line_number asm_line_number pc opcode op1 op2 op3 
			in_file >> c_line_frequency >> c_line_number >> asm_line_number >> program_counter >> opcode;
			for(int i=0; i<NUM_OPS; i++) {
				in_file >> op[i]; 
			}
			//special clase for call. Record the parameters also
			vector<string> params_list;
			vector<string> ret_val_list;
			if(opcode.compare("call") == 0) {
				int num_params = 0;
				in_file >> num_params;
				for(int i=0; i<num_params; i++) {
					string temp_param;
					in_file >> temp_param;
					params_list.push_back(temp_param);
				}
				// get the ret param also
				num_params = 0;
				in_file >> num_params;
				for(int i=0; i<num_params; i++) {
					string temp_param;
					in_file >> temp_param;
					ret_val_list.push_back(temp_param);
				}

			}
			if(opcode.compare("ret") == 0) {
				int num_params = 0;
				in_file >> num_params;
				for(int i=0; i<num_params; i++) {
					string temp_param;
					in_file >> temp_param;
					ret_val_list.push_back(temp_param);
				}
			}

			// these are not instructions; ignore them
			// ignore prefetch instructions
			if(! (opcode.compare("type") == 0 || opcode.compare("size") == 0 || opcode.compare("word") == 0 || opcode.compare("align") == 0 || opcode.compare("skip") == 0) ) {
				instruction_t inst(program_name, app_name, c_line_frequency, c_line_number, asm_line_number, program_counter, opcode, op, params_list, ret_val_list);

				program.push_back(inst); 
				pc_map[program_counter] = program.size()-1;
				if(DEBUG_PARSER)
					program.back().print_instruction();
			}

		} else if (flag == 2) { // function name
			int c_line_frequency, c_line_number, asm_line_number;
			string function_name; 
			// c_line_number asm_line_number function_name
			in_file >> c_line_frequency >> c_line_number >> asm_line_number >> function_name;
			while (function_name_map.find(function_name) != function_name_map.end()) {
					function_name = function_name + "_";
			}
			instruction_t inst(program_name, app_name, c_line_frequency, c_line_number, asm_line_number, function_name);
			program.push_back(inst); 
			function_name_map[function_name] = program.size()-1;
			if(DEBUG_PARSER)
				program.back().print_instruction();
		}  // ignore labels - we dont need them anymore
	}

	in_file.close();

	// process_line_frequency();
	cout << "Num instructions parsed: " << program.size() << endl;

	return true;
}

void program_t::create_cfg() {
	int num_bbs = 0;
	int num_patterns = 0;
	collect_arcs();
	// go through all the instructions and function_names
	for(int i=0; i<program.size(); i++) {

		if(program[i].is_func_name()) {
			// if a function is found then create a CFG for this function
			// assumption is that duplicate functions will not be found in the disassembeled binaries, which is true


			cfg_t * cfg = new cfg_t(this, program[i].get_function_name());
			function_cfg_map[program[i].get_function_name()] = cfg; // Map function name with the pointer of the CFG
			if(i+1 < program.size()) {
				pc_cfg_map[program[i+1].get_program_counter()] = cfg; // Map PC with the pointer of the CFG
			}

			cfg->create_cfg(&program, ++i); // i is being incremented here

			cfg->fill_holes(program_name, app_name, &program, arcs);	// use dynamic information to link broken/missing edges
																		// destination for jump instructions is not known statically

			// cfg->clean_bbs(&program);	// break basic blocks at the end of ret/restore instruction
			cfg->mark_nops();

			if(SESE) {
				// We should clean up basic blocks - look at prelim doc - shorten
				// blocks with multiple entry or exit edges. The way its
				// implemented now, I have to write another function to do this
				// (similar to make_missing_edges) after clean_bbs
				cfg->prepare_for_sese(&program);
			}

			num_bbs += cfg->get_num_bbs();

			if(DEBUG) {
				cout << program[i-1].get_function_name() << ": ";
				cout << "num_bbs=" << cfg->get_num_bbs() << endl;

				if(program[i-1].get_function_name().compare("__1cILargeNetNdoBuildInside6M_v_") != 0) {
			 		path_t * path = new path_t();
			 		for(int k=0; k<cfg->get_count_id(); k++) {
			 			path->push_back(false);
			 		}
			 		if(cfg->get_start() != NULL) {
			 			cfg->list_backedges(path, cfg->get_start());
			 			cout << "backedges done" << endl;
			 		}
				}
			}
	
			if(SESE) {

				cout << program[i-1].get_function_name() << endl;
				// if(program[i-1].get_function_name().compare("do_jump") == 0) {
				// }

				cfg->create_edges();

				cfg->find_sese_regions(); // create SESE regions

				if(CFG_DEBUG) {
					ofstream ofs_sese;
					string ofs_sese_filename = "cfg/" + program_name + "/" + app_name + "." + program[i-1].get_function_name() + ".dot";
					//cout << ofs_sese_filename << endl;
					ofs_sese.open(ofs_sese_filename.c_str(), ios::out);

					cfg->print_cfg(ofs_sese, exit_points_stream);

					ofs_sese.close();
				}

				// not needed any more because cycle equivalence is sufficient for creating PST and finding SESE regions
				// cfg->create_dominators();
				// cfg->create_post_dominators();
			}


			// int temp_num_patterns =  cfg->get_num_patterns(cfg->get_start(), empty_set);
			// num_patterns += temp_num_patterns;
			// cout << " num_patterns=" << temp_num_patterns << endl;
			// cout << " test= " << num_patterns << endl;
		}
	}

	//pc_cfg_map should be populated by now
	for(int i=0; i<program.size(); i++) {
		if(program[i].is_call_site()) {
			string t = program[i].get_call_target();
			if(pc_cfg_map.find(t) != pc_cfg_map.end()){
				// TODO: If target is not found then take a conservative approach in live register analysis
				cfg_t * tc = pc_cfg_map[t];
				program[i].set_call_cfg(tc);
				tc->add_caller(program[i].get_bb());
			}
		}
	}

	cout << "Total number of basic blocks: " << num_bbs << endl;
	int avg_bb_size = program.size()/num_bbs;
	cout << "Average size of basic blocks: " << avg_bb_size << endl;

}

void program_t::collect_arcs() {
#if(UNOPT_APPS) 
	string file_name = "apps/unopt/" + program_name + "/" + app_name + ".br_prof";
#else
//	string file_name = "apps/optimized/full_optimized/" + program_name + "/" + app_name + ".br_prof";
	string file_name = string(std::getenv("RELYZER_APPS"))  + "/" + program_name + "/" + app_name + ".br_prof";
#endif
	//cout << "file_name = -" << file_name << "-\n";

	ifstream in_file(file_name.c_str(), ifstream::in);
	if(!in_file.good()) {
		cout << file_name << ": input file is not good loc1\n";
		return;
	}

	string line = "";
	while(in_file.good()) {
		// input will have the following syntax 
		// For each line: pc -> pc        num exception

		string from_pc = "", ignore = "", to_pc = "";
		in_file  >> from_pc;
		if(from_pc.compare("exception") == 0) {
			in_file  >> from_pc;
		}
		in_file  >> ignore;
		in_file  >> to_pc;
		in_file  >> ignore;
		if(in_file.eof()) 
			break;

		from_pc.erase(2, 7);
		//cout << " from_pc: " << from_pc << endl;
		if(to_pc.substr(2,8).compare("00000001") != 0) {
			// if target of this branch is not within the text section then skip this branch
			continue;
		}

		to_pc.erase(2, 7);
		//cout << " to_pc: " << to_pc << endl;
		arcs[from_pc] = to_pc;
	}
	in_file.close();
}

// The following should be done after all the instruction marking is done 
// we dont need this anymore becuase the number of control patterns is done completely through dynamic profiler
// DEFUNCT
void program_t::gen_control_patterns() {
	long num_patterns = 0;
	map<std::string, cfg_t*>::iterator it;

	// counts the possible control flow paths within the function upto a certain depth
	for (it=function_cfg_map.begin(); it != function_cfg_map.end(); it++) { // for all cfgs
		cfg_t * cfg = (*it).second;
		int temp_num_patterns =  cfg->gen_control_patterns(program_name);
		//cout << (*it).first << ": ";
		//cout << " num_patterns=" << temp_num_patterns << endl;
		num_patterns += temp_num_patterns;
	}
	cout << "Total control patterns: " << num_patterns << endl;

	// int temp_num_patterns =  cfg->get_num_patterns(cfg->get_start(), empty_set);
			// num_patterns += temp_num_patterns;
			// cout << " num_patterns=" << temp_num_patterns << endl;
			// cout << " test= " << num_patterns << endl;

}

bb_t* program_t::get_cfg_start(string pc) {
	if(pc_cfg_map.find(pc) == pc_cfg_map.end()) 
		return NULL;
	else
		return pc_cfg_map[pc]->get_start();
}

void program_t::live_reg_analysis() {
	map<std::string, cfg_t*>::iterator it;
	for (it=function_cfg_map.begin(); it != function_cfg_map.end(); it++) { // for all cfgs
		cfg_t * cfg = (*it).second;
		cfg->live_reg_analysis();
	}
}

void program_t::mark_unlive_registers() {
	map<std::string, cfg_t*>::iterator it;
	for (it=function_cfg_map.begin(); it != function_cfg_map.end(); it++) { // for all cfgs
		cfg_t * cfg = (*it).second;
		cfg->mark_unlive_registers();
	}
}

void program_t::create_fault_set() {
	for(int i=0; i<program.size(); i++) {
		program[i].create_fault_list();
	}
}

// fm
//	0 - all faults
// 	1 - register faults
// 	2 - integer register faults
void program_t::print_fault_set(bool original, int fm) {
	// cout << "Fault Set: \n";
	// format - c_line_frequency[ equivalent_pc ] : PC : faults
	fstream output_file;
#if(UNOPT_APPS) // DEFUNCT
	string fault_list_file_name = "output/unopt/" + program_name + "_" + app_name + "_fault_list.txt";
#else
	string fault_list_file_name =  string(std::getenv("APPROXILYZER"))  + "/" + FAULT_LIST_DIR + program_name +/* TODO "_" + app_name +*/ "_fault_list.txt";
#endif
	output_file.open(fault_list_file_name.c_str(), ios::out);
	
	for(int i=0; i<program.size(); i++) {
		program[i].print_fault_list(original, output_file, fm);
	}
	output_file.close();
}

void program_t::print_pruned_categories() {
	cout << "\nCategories breakdown: ";
	statistics aggr_stats;
	aggr_stats.masked = 0;
	aggr_stats.sdcs = 0;
	aggr_stats.detected = 0;
	aggr_stats.equivalent = 0;
	aggr_stats.remaining = 0;
	for(int i=0; i<program.size(); i++) {
		program[i].accumulate_fault_stats(aggr_stats);
	}
	uint64_t total_pruned = aggr_stats.masked+aggr_stats.sdcs+aggr_stats.detected+aggr_stats.equivalent+aggr_stats.remaining;
	cout << "\n Total pruned: " << total_pruned/1000 << "K" << endl;
	cout << "\n Masked: " << aggr_stats.masked/1000 << "K: " << ((100*aggr_stats.masked)/total_pruned) << "%";
	cout << "\n SDCS: " << aggr_stats.sdcs/1000 << "K: " << ((100*aggr_stats.sdcs)/total_pruned) << "%";
	cout << "\n Detected: " << aggr_stats.detected/1000 << "K: " << ((100*aggr_stats.detected)/total_pruned) << "%";
	cout << "\n Equivalent: " << aggr_stats.equivalent/1000 << "K: " << ((100*aggr_stats.equivalent)/total_pruned) << "%";
	cout << endl;
	//cout << "\n Remaining: " << aggr_stats.remaining << "K: " << ((double)aggr_stats.remaining/total_pruned)*100 << "%";

}

double program_t::count_fault_set(bool original, int fm) {
	double fault_set_size = 0;
	for(int i=0; i<program.size(); i++) {
		fault_set_size += program[i].count_fault_list(original, fm);
	}
	if(original)
		cout << "Fault set size for fm=" << fm << ": " << fault_set_size << "M" << endl;
	else 
		cout << "Remaining (for fm=" << fm << "): " << fault_set_size << "M" << endl;
	cout << fault_set_size << "M" << endl;
	printf ("%.5f\n", fault_set_size);
	return fault_set_size;
}

int program_t::get_block_instr(string label) {
//	label += ":";
//	for(int i=0; i<program.size(); i++) {
//		if(program[i].is_label_op()) {
//			if(label.compare(label) == 0)
//				return i;
//		}
//	}
//	//cout << "Label should have been found in the program: " << label << endl;
	return -1;
}

bool program_t::is_within_loop(int instr) {
	
//	string def = program[instr].get_def();	
//	if(def.compare("") == 0) {
//		// check for special case of store instructions
//		//cout << " returning false\n";
//		return false; 
//	}
//
//	vector<int> explore_list;
//	vector<string> explore_def;
//	explore_list.push_back(instr+1);
//	explore_def.push_back(def);
//
//	for(int i=0; i<explore_list.size(); i++) {
//		def = explore_def[i];
//		//cout << program[explore_list[i]].get_asm_line_number() << ": exploring:" << def << " " << explore_list[i] << endl;
//		for(int j=explore_list[i]; j<program.size(); j++) {
//			
//			if(program[j].is_ret()) // end of function
//				break;
//			//cout << "-" << program[j].get_def() << "- "; // found a new def for same reg
//			if(program[j].get_def().compare(def) == 0) // found a new def for same reg
//				break;
//			if(def.substr(0,2).compare("%g") == 0 && program[j].is_call_site()) // For globals, the chain will be broken at call sites
//				break;
//			if(program[j].is_branch_op()) {
//				//cout << " found branch: " << j <<endl;
//				int new_instr;
//				if(program[j].get_opcode().find(",pn") != -1)
//					new_instr = get_block_instr(program[j].get_op(1));
//				else 
//					new_instr = get_block_instr(program[j].get_op(0));
//				bool found = false;
//				for(int k=0; k<i; k++) {
//					if(explore_list[k] == new_instr)  // if not already explored
//						found = true;
//				}
//				if(!found) {
//						explore_list.push_back(new_instr);
//						explore_def.push_back(def);
//					}
//			}
//
//			if(program[j].find_use(def)) {
//				if(is_outside_loop(j)) {
//					//cout << program[j].get_asm_line_number() << ":returning false:" << def << endl;
//					return false;
//				} else if ( program[j].is_store_op()) {
//					//cout << program[j].get_asm_line_number() << ":returning false:" << def << endl;
//					return false;
//				} else {
//					//cout << program[j].get_asm_line_number() << ":" << "found use: " << j <<endl;
//					bool found = false;
//					for(int k=0; k<i; k++) {
//						if(explore_list[k] == j+1)  // if not already explored
//							found = true;
//					}
//					if(!found) { 
//						explore_list.push_back(j+1);
//						explore_def.push_back(program[j].get_def());
//					}
//				}
//			}
//		}
//	}
//
	//cout << " returning ture " << endl;
	//exit(1);
	return true;
}

bool program_t::is_outside_loop(int instr) {
	if(program[instr].get_c_line_number() > 33 && program[instr].get_c_line_number() <= 37) 
		return false;
	return true;
}
	

bool program_t::can_be_flattened(int instr) {
	// Correct thing to check is:
	// 1. Check if its within the loop
	// 2. Check if the fault in this instruction will remain withing the loop
	// 3. Check for loop-independence

	if(program[instr].get_c_line_number() > 33 && program[instr].get_c_line_number() <= 37) {
		return false;
		//if(!is_within_loop(instr)) {
		//	return false;
		//}
		//cout << program[instr].get_asm_line_number() << ":" << program[instr].get_def() << endl;
	}

	if(program[instr].get_c_line_number() > 14 && program[instr].get_c_line_number() <= 17) 
		return true;
	
	if(program[instr].get_c_line_number() > 33 && program[instr].get_c_line_number() <= 37) 
		return true;

	return false;
}

// find invariant based detectors
// 1. find the registers that should be compared for the invariant
// 2. find the points where the value should be computed and where the value should be checked
void program_t::find_invariant_detectors() {
	map<std::string, cfg_t*>::iterator it;
	for (it=function_cfg_map.begin(); it != function_cfg_map.end(); it++) { // for all cfgs
			// (*it).second is cfg pointer
			cfg_t * cfg = (*it).second;
			cfg->find_invariant_detectors();
	}
}

// Perform loop flattening
void program_t::prune_loop_flattening() {
	cout << "Flattening loop..\n";
	for(int i=0; i<program.size(); i++) {
		if(can_be_flattened(i)) {
			for(int j=i; j>=0; j--) {
				if(program[j].get_c_line_frequency() != program[i].get_c_line_frequency()) {
					program[i].set_pruned_c_line_frequency(program[j].get_c_line_frequency());
					program[i].set_equivalent_pc(program[j].get_program_counter());
					break;
				}
			}
		}
	}
}

// Prune Heap, Stack, and Global Address accesses that go outside the boundaries 
void program_t::prune_addresses() {
	cout << "Pruning address ranges..\n";
	for(int i=0; i<program.size(); i++) 
		program[i].prune_addresses();
}

// Prune Branch Targets
void program_t::prune_branch_targets() {
	cout << "Pruning branch targets..\n";
	for(int i=0; i<program.size(); i++) 
		program[i].prune_branch_targets();
}

// Example:
// Prune def (use def-use chain)
// for every use that is found in the source, count them twice.
// if the use is not found, then keep the fault intact - it may be setting an argument for function call
void program_t::prune_defs() {
	cout << "Pruning def from def-use chain..\n";
	map<std::string, cfg_t*>::iterator it;
	for (it=function_cfg_map.begin(); it != function_cfg_map.end(); it++) { // for all cfgs
		cfg_t * cfg = (*it).second;
		cfg->prune_defs(); 
	}

	// for(int i=0; i<program.size(); i++) {
	// 	string def = program[i].get_def();
	// 	if(def.compare("") == 0) {
	// 		continue;
	// 	}

	// 	bool found_use = false;
	// 	for(int j=i; j<program.size(); j++) {
	// 		if(program[j].find_use(def)) {
	// 			found_use = true;
	// 			program[j].mark_for_extra_counting(def);
	// 			break;
	// 		}

	// 		if(program[j].is_branch_op() || program[j].is_jump() || program[j].is_call_site() || program[j].is_ret()) {
	// 			break;
	// 		}
	// 	}
	// 	if(found_use) {
	// 		// remove def
	// 		program[i].remove_from_fault_list(def, true);
	// 	}
	// }
}

void program_t::prune_g0_faults() {
	cout << "Pruning g0 faults ..\n";
	for(int i=0; i<program.size(); i++) {
		program[i].prune_g0_faults();
	}
}

void program_t::prune_shift_consts() {
	cout << "Pruning shift constants..\n";
	for(int i=0; i<program.size(); i++) {
		if(program[i].is_shift_op()) { 
			program[i].prune_shift_const();
		}
	}
}

void program_t::prune_use_in_const_instr() {
	cout << "Pruning through constant propagation..\n";
	for(int i=0; i<program.size(); i++) {
		if(program[i].has_const_op()) {
			if(program[i].is_logical_op() || program[i].is_shift_op()) {
				bool flag = false;
				// only 3 operand instructions
				// move faults in operand 1 to operand 3
				if(program[i].get_opcode().substr(0,1).compare("x") == 0 || program[i].get_opcode().substr(0,3).compare("not") == 0) {
	 				program[i].mark_def_for_extra_counting();
					flag = true;
				}
				if(program[i].get_opcode().substr(0,2).compare("sr") == 0 || program[i].get_opcode().substr(0,2).compare("sl") == 0 || program[i].get_opcode().substr(0,2).compare("or") == 0 ) {
					string const_op = program[i].get_op(1);
					if(const_op.compare("0x0") == 0) { // only if (constant) second op is 0
		 			 	program[i].mark_def_for_extra_counting();
						flag = true;
					}
				}
				if(program[i].get_opcode().substr(0,2).compare("sr") == 0 || program[i].get_opcode().substr(0,2).compare("sl") == 0) {

					string const_op = program[i].get_op(1);
					if(const_op.compare("0x0") != 0) { // only if (constant) second op is 0
						// cout << "Const op: " << const_op << ": ";
						int const_var = -1;
						sscanf(const_op.c_str(), "0x%x", &const_var);
						// cout << ": " << const_var << ": ";
						// program[i].print_instruction();
						// prune faults from low-order bits from 0 to const_var
						// if(program[i].get_opcode().substr(0,2).compare("sr") == 0)
						//	program[i].prune_bits(program[i].get_op(0), false, 0, const_var) ; // reg name, is_def, start, end
						if(program[i].get_opcode().substr(0,2).compare("sl") == 0) {
							program[i].prune_bits(program[i].get_op(0), false, 64-const_var, 64) ; // reg name, is_def, start, end
						}
					}
				}

				// TODO
				if(flag) {
						program[i].remove_from_fault_list(program[i].get_op(0), false);
				}
			}
		}
	}
}


void program_t::prune_const_propagation() {
	cout << "Pruning through constant propagation..\n";
	bool found_adjustment = true;
	int max = 100;
	while(found_adjustment && max>0) {
		found_adjustment = false;
		max--;
		for(int i=0; i<program.size(); i++) {
			if(program[i].has_const_op()) {
				if(program[i].is_logical_op() || program[i].is_arithmetic_op()) {
					//only 3 operand instructions
					int exch_instr = i;
					if(program[i].is_op0_removed()) {
						exch_instr = find_def(program[i].get_op(0), i);
					}
					if(exch_instr != -1) {
						if(adjust_bounds(i,exch_instr)) 
							found_adjustment = true;
					}
				}
			}
		}
	}
}

// works only within a function (or basic block?)
int program_t::find_def(string use, int use_instr) {
	for(int i=use_instr-1; i>0; i--) {
		if(program[i].is_save()) // start of function
			return -1;
		if(use.substr(0,2).compare("%g") == 0 && program[i].is_call_site()) // For globals, the chain will be broken at call sites
			return -1;
		if(program[i].is_branch_op()) // for now being conservative and terminating the chain at branch instructions
			return -1;

		if(program[i].get_def().compare(use) == 0)
			return i;
	}
	return -1;
}

// returns true if adjustment is done
// returns false if no adjustment is done
bool program_t::adjust_bounds(int instr1, int instr2) {
	bool adjusted = false;
	//use opcode of instr1 in both these cases
	string instr1_op = "";
	string instr2_op = program[instr2].get_def();
	if(instr1 == instr2) {
		//exchange first op with the def
		instr1_op = program[instr1].get_op(0);
	} else {
		//exchange bounds of def of instr1 with def of instr2
		instr1_op = program[instr1].get_def();
	}
	
	if(program[instr1].is_logical_op()) {
		int pruned_bits1 = program[instr1].get_fault_bits(instr1_op);
		int pruned_bits2 = program[instr2].get_fault_bits(instr2_op);
		// int const_var = atoi(program[instr1].get_op(1).c_str());
		int const_var;
		sscanf(program[instr1].get_op(1).c_str(), "%x", &const_var);
		//cout << program[instr1].get_op(1) << ":" << const_var <<endl; 

		if(program[instr1].is_shift_op()) { // for shift operations
			if(program[instr1].get_opcode().substr(0,2).compare("sr")) { //shift right operation
				if(pruned_bits1 == pruned_bits2 + const_var) ; // do nothing
				else if(pruned_bits1 > pruned_bits2 + const_var) {
					program[instr1].set_fault_bits(instr1_op,pruned_bits2+const_var);
					//cout << "adjusting sr: " << instr1 << "\n";
					adjusted = true;
				} else {
					program[instr2].set_fault_bits(instr2_op,pruned_bits1-const_var);
					//cout << "adjusting sr: " << instr2 << "\n";
					adjusted = true;
				}
			}
			if(program[instr1].get_opcode().substr(0,2).compare("sl")) { //shift left operation
				if(pruned_bits1 + const_var == pruned_bits2) ; // do nothing
				else if(pruned_bits1 + const_var > pruned_bits2) {
					program[instr1].set_fault_bits(instr1_op, pruned_bits2-const_var);
					//cout << "adjusting sl: " << instr1 << "\n";
					adjusted = true;
				} else { 
					program[instr2].set_fault_bits(instr2_op, pruned_bits1 + const_var);
					//cout << "adjusting sl: " << instr2 << "\n";
					adjusted = true;
				}
			}

		} else { //or, and, xor, xnor
			if(program[instr1].get_opcode().substr(0,3).compare("and") == 0) {
				int const_var_bits = const_var != 0 ? (int)floor(log(const_var)/log(2) + 1) : 0;
				if(pruned_bits1  != const_var_bits) {
					program[instr1].set_fault_bits(instr1_op, const_var_bits);
					//cout << "adjusting const_var: " << const_var << " logical: " << const_var_bits << "\n";
					adjusted = true;
				}
			}
		}
	}
	return adjusted;
}

// list of instructions that lead to a store 
// Algorithm:
// linear search on the program
// if instruction is store
// 	call a function that goes till its def
//	function: 
//		if def is not a load or store 
//			call function on both the the operands
void program_t::mark_store_instructions() {
	map<std::string, cfg_t*>::iterator it;
	for (it=function_cfg_map.begin(); it != function_cfg_map.end(); it++) { // for all cfgs
			// (*it).second is cfg pointer
			cfg_t * cfg = (*it).second;
			cfg->mark_store_instructions();
	}
}

// only for unopt apps
// void program_t::call_site_analysis() {
// 	map<std::string, cfg_t*>::iterator it;
// 	for (it=function_cfg_map.begin(); it != function_cfg_map.end(); it++) { // for all cfgs
// 			// (*it).second is cfg pointer
// 			cfg_t * cfg = (*it).second;
// 			cfg->mark_store_instructions();
// 	}
// }

void program_t::mark_control_instructions() {
	map<std::string, cfg_t*>::iterator it;
	for (it=function_cfg_map.begin(); it != function_cfg_map.end(); it++) { // for all cfgs
			// (*it).second is cfg pointer
			cfg_t * cfg = (*it).second;
			cfg->mark_control_instructions();
	}
}

void program_t::mark_other_instructions() {
	map<std::string, cfg_t*>::iterator it;
	for (it=function_cfg_map.begin(); it != function_cfg_map.end(); it++) { // for all cfgs
			// (*it).second is cfg pointer
			cfg_t * cfg = (*it).second;
			cfg->mark_other_instructions();
	}
}
void program_t::process_line_frequency() {
	for(int i=0; i<program.size(); i++) {
		if(program[i].is_branch_op() || program[i].is_ret()) {
			for(int j=i-1; j>0; j--) {
				if(!program[j].is_func_name()) { 
					if(program[j].is_branch_op() || program[j].is_ret())
						break;

					program[j].set_c_line_frequency(program[i].get_c_line_frequency());
					program[j].set_pruned_c_line_frequency(program[i].get_c_line_frequency());
					//cout << ".";
					//if(program[j].is_compare_op()) 
					//	program[j].print_instruction();
					if(program[j].is_save()) 
						break;
				}
			}
		}
	}
	return; 
	// check
	for(int j=0; j<program.size(); j++) {
		if(!program[j].is_func_name()) { 
			if(program[j].get_c_line_frequency() == 0)
				cout << ".";
		}
	}
}

string program_t::get_app_name() {
	return app_name;
}

cfg_t * program_t::get_cfg(string pc) {
		return pc_cfg_map[pc];
}

void program_t::check() {
	for(int i=0; i<program.size(); i++) {
		if(!(program[i].is_call_site() || program[i].is_ret())) {
			if(program[i].get_def().compare("") == 0) {
				program[i].print_instruction();
			}
		}
	}
}

void program_t::get_instruction_distribution() {
	int store = 0;
	int control = 0;
	int store_and_control = 0;
	int dead = 0;
	int ignored = 0;
	int sum = 0;
	for(int i=0; i<program.size(); i++) {
		if(program[i].get_c_line_frequency() > 0) {

				switch(program[i].get_dep_type()) {

					case 	STORE:  // 0
							store++;
							break;
					case 	CONTROL: // 1
					case 	LIVE: // 2
							control ++;
							break;
					case 	CALL_SITE: // 3
							ignored++;
							break;
					case 	ILLTRAP: // 4
							break;
					case 	RET_SITE: // 5
					case 	SAVE_SITE: // 6
							ignored++;
							break;
					case 	DEAD: // 7
							dead++;
							sum+= program[i].get_c_line_frequency() ;
							break;
					case 	STATEREG_OP: // 8
							ignored++;
							break;
					case 	STORE_AND_LIVE: // 9
							store_and_control++;
							break;
					case 	COMPARE: // 10
							control++;
							break;
					case 	REMAINING_OTHERS: // 11
							ignored++;
							break;
				}
		}
	}
	string fn = INSTR_DIST_FILE;
	fstream output_file;
	output_file.open(fn.c_str(), ios::out);
	
	output_file << store << "\t" << control << "\t" << store_and_control << "\t" << dead << "\t" << ignored << endl;
	output_file.close();
	cout << sum << endl;
}

void program_t::open_exit_points_file()
{
	string ofs_ep_filename = "exit_points/exit_points." + program_name + ".ep";
	exit_points_stream.open(ofs_ep_filename.c_str(), ios::out);
}
void program_t::close_exit_points_file()
{
	exit_points_stream.close();
}
