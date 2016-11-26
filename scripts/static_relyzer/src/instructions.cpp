#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <ctype.h>
#include "instructions.h"
#define DEBUG_PARSER 0

#define BRANCH_FLATTENING
#define CALL_SITE_FLATTENING
#define RET_FLATTENING

using namespace std;
bool is_number (string);

/* Functions of instructions class */
instructions::instructions(std::string prg_name, std::string ap_name) {
	program_name = "";
	app_name = "";
}

bool instructions::populate_instructions(string prg_name, string a_name) {
	program_name = prg_name;
	app_name = a_name;

	string file_name = "apps/" + prg_name + "/" + app_name + ".clean.s";
	if(DEBUG_PARSER)
		cout << "file_name = -" << file_name << "-\n";

	ifstream in_file(file_name.c_str(), ifstream::in);
	if(!in_file.good()) {
		cout << file_name << ": input file is not good\n";
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
		if(flag > 1) {  // for debugging
			cout << "something is wrong\n";
			exit(-1);
		}
		//cout << flag << ":";

		if(flag) { // instruction

			int c_line_frequency, c_line_number, asm_line_number;
			string program_counter, opcode, op[NUM_OPS];
			// c_line_number asm_line_number pc opcode op1 op2 op3 
			in_file >> c_line_frequency >> c_line_number >> asm_line_number >> program_counter >> opcode;
			for(int i=0; i<NUM_OPS; i++) {
				in_file >> op[i]; 
			}
			//spaecial clase for call. Record the parameters also
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
			if(! (opcode.compare("type") == 0 || opcode.compare("size") == 0 || opcode.compare("word") == 0 || opcode.compare("align") == 0 || opcode.compare("skip") == 0) ) {
				instruction inst(program_name, app_name, c_line_frequency, c_line_number, asm_line_number, program_counter, opcode, op, params_list, ret_val_list);
				program.push_back(inst); 
				if(DEBUG_PARSER)
					program.back().print_instruction();
			}

		} else { // label

			int c_line_frequency, c_line_number, asm_line_number;
			string label; 
			// c_line_number asm_line_number label 
			in_file >> c_line_frequency >> c_line_number >> asm_line_number >> label;
			instruction inst(program_name, app_name, c_line_frequency, c_line_number, asm_line_number, label);
			program.push_back(inst); 
			// setting the label - program_index map
			label_map[label] = program.size()-1;
			if(DEBUG_PARSER)
				program.back().print_instruction();
		}
	}

	in_file.close();

	process_line_frequency();
	cout << "Num instructions parsed: " << program.size() << endl;
	return true;
}

void instructions::create_fault_set() {
	for(int i=0; i<program.size(); i++) {
		program[i].create_fault_list();
	}
}

void instructions::print_fault_set(bool original) {
	// cout << "Fault Set: \n";
	// format - c_line_frequency[ equivalent_pc ] : PC : faults
	fstream output_file;
	string fault_list_file_name = "output/" + program_name + "_" + app_name + "_fault_list.txt";
	output_file.open(fault_list_file_name.c_str(), ios::out);
	
	for(int i=0; i<program.size(); i++) {
		program[i].print_fault_list(original, output_file);
	}
	output_file.close();
}

void instructions::print_pruned_categories() {
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

double instructions::count_fault_set(bool original) {
	double fault_set_size = 0;
	for(int i=0; i<program.size(); i++) {
		fault_set_size += program[i].count_fault_list(original);
	}
	if(original)
		cout << "Fault set size: " << fault_set_size << "M" << endl;
	else 
		cout << "Remaining: " << fault_set_size << "M" << endl;
	return fault_set_size;
}

int instructions::get_block_instr(string label) {
	label += ":";
	for(int i=0; i<program.size(); i++) {
		if(program[i].is_label_op()) {
			if(label.compare(label) == 0)
				return i;
		}
	}
	//cout << "Label should have been found in the program: " << label << endl;
	return -1;
}

bool instructions::is_within_loop(int instr) {
	
	string def = program[instr].get_def();	
	if(def.compare("") == 0) {
		// check for special case of store instructions
		//cout << " returning false\n";
		return false; 
	}

	vector<int> explore_list;
	vector<string> explore_def;
	explore_list.push_back(instr+1);
	explore_def.push_back(def);

	for(int i=0; i<explore_list.size(); i++) {
		def = explore_def[i];
		//cout << program[explore_list[i]].get_asm_line_number() << ": exploring:" << def << " " << explore_list[i] << endl;
		for(int j=explore_list[i]; j<program.size(); j++) {
			
			if(program[j].is_ret()) // end of function
				break;
			//cout << "-" << program[j].get_def() << "- "; // found a new def for same reg
			if(program[j].get_def().compare(def) == 0) // found a new def for same reg
				break;
			if(def.substr(0,2).compare("%g") == 0 && program[j].is_call_site()) // For globals, the chain will be broken at call sites
				break;
			if(program[j].is_branch_op()) {
				//cout << " found branch: " << j <<endl;
				int new_instr;
				if(program[j].get_opcode().find(",pn") != -1)
					new_instr = get_block_instr(program[j].get_op(1));
				else 
					new_instr = get_block_instr(program[j].get_op(0));
				bool found = false;
				for(int k=0; k<i; k++) {
					if(explore_list[k] == new_instr)  // if not already explored
						found = true;
				}
				if(!found) {
						explore_list.push_back(new_instr);
						explore_def.push_back(def);
					}
			}

			if(program[j].find_use(def)) {
				if(is_outside_loop(j)) {
					//cout << program[j].get_asm_line_number() << ":returning false:" << def << endl;
					return false;
				} else if ( program[j].is_store_op()) {
					//cout << program[j].get_asm_line_number() << ":returning false:" << def << endl;
					return false;
				} else {
					//cout << program[j].get_asm_line_number() << ":" << "found use: " << j <<endl;
					bool found = false;
					for(int k=0; k<i; k++) {
						if(explore_list[k] == j+1)  // if not already explored
							found = true;
					}
					if(!found) { 
						explore_list.push_back(j+1);
						explore_def.push_back(program[j].get_def());
					}
				}
			}
		}
	}

	//cout << " returning ture " << endl;
	//exit(1);
	return true;
}

bool instructions::is_outside_loop(int instr) {
	if(program[instr].get_c_line_number() > 33 && program[instr].get_c_line_number() <= 37) 
		return false;
	return true;
}
	

bool instructions::can_be_flattened(int instr) {
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

// Perform loop flattening
void instructions::prune_loop_flattening() {
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
void instructions::prune_addresses() {
	cout << "Pruning address ranges..\n";
	for(int i=0; i<program.size(); i++) 
		program[i].prune_addresses();
}

// Prune Branch Targets
void instructions::prune_branch_targets() {
	cout << "Pruning branch targets..\n";
	for(int i=0; i<program.size(); i++) 
		program[i].prune_branch_targets();
}

// Example:
// ld      [%fp+2039],%g2
// add     %g2,1,%g3
// prune %g2 in second instructions
// Prune uses of a def (use def-use chain, compiler term)

// works only within a function
void instructions::prune_use_defs() {
	cout << "Pruning uses of a def..\n";
	for(int i=0; i<program.size(); i++) {
		string def = program[i].get_def();
		if(def.compare("") == 0) {
			// check for special case for store instructions
			// store %reg -> [addr] 
			// prune value of [addr]
			if(program[i].is_store_op()) {
				// cout << "st:" << program[i].get_op(1) << "\n";
				program[i].set_fault_bits(program[i].get_op(1), 0, 1);
			}
			continue;
		}

		for(int j=i+1; j<program.size(); j++) {
			if(program[j].is_ret()) // end of function
				break;
			if(program[j].get_def().compare(def) == 0) // found a new def for same reg
				break;
			if(def.substr(0,2).compare("%g") == 0 && program[j].is_call_site()) // For globals, the chain will be broken at call sites
				break;
			if(program[j].is_branch_op()) // for now being conservative and terminating the chain at branch instructions
				break;
			
			if(program[j].find_use(def)) {
				//cout << program[i].get_asm_line_number() << ":";
				int removed_bits = program[j].remove_from_fault_list(def);

				if(program[i].get_fault_bits(def) > removed_bits)
					program[i].set_fault_bits(def,removed_bits);
			}
		}
	
		// special case for load instructions
		// load [addr] -> %reg
		// prune value of [addr]
		if(program[i].is_load_op()) {
			// cout << "ld:" << program[i].get_op(0) << "\n";
			program[i].set_fault_bits(program[i].get_op(0), 0, 1);
		} 
	}
}

void instructions::prune_shift_consts() {
	cout << "Pruning shift constants..\n";
	for(int i=0; i<program.size(); i++) {
		if(program[i].is_shift_op()) { 
			program[i].prune_shift_const();
		}
	}
}

void instructions::prune_const_propagation() {
	cout << "Pruning through constant propagation..\n";
	bool found_adjustment = true;
	while(found_adjustment) {
		found_adjustment = false;
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
int instructions::find_def(string use, int use_instr) {
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
bool instructions::adjust_bounds(int instr1, int instr2) {
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
		int const_var = atoi(program[instr1].get_op(1).c_str());

		if(program[instr1].is_shift_op()) { // for shift operations
			if(program[instr1].get_opcode().substr(0,2).compare("sr")) { //shift right operation
				if(pruned_bits1 == pruned_bits2 + const_var) ; // do nothing
				else if(pruned_bits1 > pruned_bits2 + const_var) {
					program[instr1].set_fault_bits(instr1_op,pruned_bits2+const_var);
					adjusted = true;
				} else {
					program[instr2].set_fault_bits(instr2_op,pruned_bits1-const_var);
					adjusted = true;
				}
			}
			if(program[instr1].get_opcode().substr(0,2).compare("sl")) { //shift left operation
				if(pruned_bits1 + const_var == pruned_bits2) ; // do nothing
				else if(pruned_bits1 + const_var > pruned_bits2) {
					program[instr1].set_fault_bits(instr1_op, pruned_bits2-const_var);
					adjusted = true;
				} else { 
					program[instr2].set_fault_bits(instr2_op, pruned_bits1 + const_var);
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
void instructions::set_leading_store_pcs() {
	for(int i=0; i<program.size(); i++) {
		if(program[i].is_store_op()) {

			//cout << "ST:" << program[i].get_program_counter() ;
			// call for the first operand register
			mark_dependent_instructions(i, program[i].get_op(0), program[i].get_program_counter(), STORE_DEP);

			// call for the registers used in address
			// find if this op (def)  has a reg in it
			string local_def =  program[i].get_op(1);
			int found = local_def.find_first_of("%");
			while (found != string::npos) {
				string local_reg = get_reg(local_def, found);
				if(local_reg.compare("") != 0)  {
					mark_dependent_instructions(i, local_reg, program[i].get_program_counter(), STORE_DEP);
				}

				found = local_def.find_first_of("%", found+1);
			}
			program[i].set_leading_store_pc(program[i].get_program_counter(), STORE_DEP);

		} 
	}
	for(int i=0; i<program.size(); i++) {
		if(program[i].is_compare_op()) {
#ifdef BRANCH_FLATTENING
			// for distribution
			if (program[i].is_register(program[i].get_op(0)))
				mark_dependent_instructions(i, program[i].get_op(0), program[i].get_program_counter(), COMPARE_DEP);

			if (program[i].is_register(program[i].get_op(1)))
				mark_dependent_instructions(i, program[i].get_op(1), program[i].get_program_counter(), COMPARE_DEP);

			if(program[i].is_fcompare_op())
				if (program[i].is_register(program[i].get_op(2)))
					mark_dependent_instructions(i, program[i].get_op(2), program[i].get_program_counter(), COMPARE_DEP);

			// for direction
			program[i].set_leading_store_pc(program[i].get_program_counter(), COMPARE_DIR);
			//program[i].print_instruction();

#endif //BRANCH_FLATTENING
		}
	}
	for(int i=0; i<program.size(); i++) {
		if(program[i].is_branch_op()) {

			program[i].set_leading_store_pc(program[i].get_program_counter(), BRANCH_INST);

		} 
	}

	for(int i=0; i<program.size(); i++) {
		if(program[i].is_save()) {
			program[i].set_leading_store_pc(program[i].get_program_counter(), SAVE_INST);
		}
	}
}

//Function names like this comeup if you are not interested
void instructions::mark_dependent_instructions(int program_index, string local_def, string leading_store_pc, dep_type_t dep_type) {
	// cout << program_name << ":" << program[program_index].get_asm_line_number() << ":" << local_def << "\n";

#ifdef BRANCH_FLATTENING
	if(!check_read_before_write(program_index, local_def, true))
		return;
#endif // BRANCH_FLATTENING

	for(int i=program_index-1; i>=0; i--) {

		if(program[i].is_branch_op()) // this is done only within a basic block
			return; 
		if(program[i].is_save()) // this is done only within a basic block
			return; 
		if(program[i].is_call_site()) // this is done only within a basic block
			return; 
		if(!program[i].is_store_op()) {

			if(program[i].get_def().compare(local_def) == 0)  {  // reached the def 
				if(program[i].is_movcc_op()) {
					program[i].set_leading_store_pc(leading_store_pc, dep_type);

					// call for the registers used in 2nd operand of MOVcc
					// find if this op has a reg in it
					string local_def =  program[i].get_op(1);
					int found = local_def.find_first_of("%");
					while (found != string::npos) {
						string local_reg = get_reg(local_def, found);
						if(local_reg.compare("") != 0)  {
							mark_dependent_instructions(i, local_reg, program[i].get_program_counter(), STORE_DEP);
						}
		
						found = local_def.find_first_of("%", found+1);
					}

				} else {

					program[i].set_leading_store_pc(leading_store_pc, dep_type);
					//cout << leading_store_pc << "=" << program[i].get_program_counter() << " " ;
					// call this function for all the source operands of this instruction
					for(int j=0; j < NUM_OPS; j++) {
						string local_op = program[i].get_op(j);
						if(local_op.compare(local_def) != 0) { // not the def
							if( program[i].is_register(local_op) ) { // only if the operand is a register
								//if((leading_store_pc.find("0017b4") != string::npos))
								//	cout << leading_store_pc << "-" << program[i].get_program_counter() << " " ;
								//cout << local_op <<".";
								mark_dependent_instructions(i, local_op, leading_store_pc, dep_type);
							} else {
								// find if this op has a reg in it
								int found = local_op.find_first_of("%");
								while (found != string::npos) {
									string local_reg = get_reg(local_op, found);
									if(local_reg.compare("") != 0)  {
										//		if((leading_store_pc.find("0017b4") != string::npos))
										//			cout << leading_store_pc << "-" << program[i].get_program_counter() << " " ;
										mark_dependent_instructions(i, local_reg, leading_store_pc, dep_type);
									}

									found = local_op.find_first_of("%", found+1);
								}
							}
						} else 
							break;
					}

					return; // found the def. You can now return 
				}
			}
		}
	}
}


// return non %fp, %sp registers
// checks for registers of this format  %gx %lx %lo %fxx
string instructions::get_reg(string str, int start) {
	string local_reg = "";
	int found = str.find_first_of("%", start);
	if(str.size() - found >= 3) {
		if(found != string::npos) {
			if( str[found+1] == 'f' ) {
				if(isdigit(str[found+2])) { 
					local_reg = str.substr(found, 3);
					if(str.size() - found >= 4)
						if(isdigit(str[found+3]))  
							local_reg = str.substr(found, 4);
				}
			}

			if( (str[found+1] == 'g' || str[found+1] == 'l' || str[found+1] == 'o') ) {
				if(isdigit(str[found+2])) { 
					local_reg = str.substr(found, 3);
					if(str.size() - found >= 4)
						if(isdigit(str[found+3]))  // not a register: %g44, %l33
							local_reg = "";
				} 
			}
		}
	}

	//if(local_reg.compare("") != 0) 
	//	cout << "\nfound reg = " << local_reg;
	return local_reg;
}


//Algorithm:
// 1. On a non-conditional branch, go to the label and start exploring the basic block
// 2. On a conditional branch take both the directions, and start exploring the basic block

//return true if a def is found before a use
//return false if a use is found before a def
bool instructions::check_read_before_write(int program_index, string op, bool main) {

	if(op.compare("\%xcc") == 0 || op.compare("\%icc") == 0 || op.compare("\%g0") == 0) {
		return true;
	}

	// to avoid deadlock
	if(main)
		touched.clear();


	for(int i=program_index+1; i<program.size(); i++) {
		if(program[i].is_branch_op()) {
			if(program[i].is_branch_conditional()) {

				string local_label = program[i].get_op(1);
				//find the label
				int j = label_map[local_label];
				if(program[j].is_label_op()) {
					//cout << program[j].get_label() << " : " << local_label << endl;
					if(program[j].get_label().compare(local_label) == 0) {
						if(touched.find(local_label) == touched.end())  {
							//cout << program[j].get_asm_line_number() << ":" << op << ":"  << local_label << endl;
							touched[local_label] = true;
							check_read_before_write(j, op, false);
							//cout << ".";
						}
						break;
					}
				}
				// for fallthough case
				check_read_before_write(i+1, op, false);

			} else { // branch always

				string local_label = program[i].get_op(0);
				//find the label
				int j = label_map[local_label];
				if(program[j].is_label_op()) {
					if(program[j].get_label().compare(local_label) == 0) {
						if(touched.find(local_label) == touched.end())  {
							touched[local_label] = true;
							check_read_before_write(j, op, false);
							//cout << "-";
						}
						break;
					}
				}
				// no fallthough case
			}
		} else {
			if(main)  // do not search in the main calling basic block
				continue;
			if(program[i].get_def().compare(op) == 0)
				return true;
			if(program[i].find_use(op))
				return false;
		}
	}
	//no use or def found 
	return true;
}

void instructions::process_line_frequency() {
	for(int i=0; i<program.size(); i++) {
		if(program[i].is_branch_op() || program[i].is_ret()) {
			for(int j=i-1; j>0; j--) {
				if(!program[j].is_label_op()) { 
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
		if(!program[j].is_label_op()) { 
			if(program[j].get_c_line_frequency() == 0)
				cout << ".";
		}
	}
}

void instructions::get_func_list(func_list_t *func_list) {
	for (int i=0; i<program.size(); i++) {
		if(program[i].is_call_site()) {
			//find the function name and get the index of the label
			string function_name = program[i].get_op(0); // function name
			if(!func_list->find(function_name)) {
				func_signature_t new_function;
				new_function.function_name = function_name;
				
				for(int j=0; j<program[i].get_num_call_parameters(); j++) {
					string local_reg =  program[i].get_call_parameter(j);
					new_function.params.push_back(local_reg);
					new_function.leading_inst_pc.push_back("");
				}
				for(int j=0; j<program[i].get_num_ret_parameters(); j++) {
					string local_reg =  program[i].get_ret_parameter(j);
					new_function.ret_vals.push_back(local_reg);

					string leading_inst_ret_val_pc = "";
					for(int k=i+1; k<program.size(); k++) {
						if(program[k].find_use(local_reg)) {
							leading_inst_ret_val_pc = program[k].get_leading_store_pc();
							break;
						}
						if(program[k].is_branch_op())
							break;
					}
					new_function.leading_inst_ret_val_pc.push_back(leading_inst_ret_val_pc);

				}
				func_list->add(new_function);
			}
		}
	}
}

bool instructions::update_func_list(func_list_t * func_list, int f_i) {
	string local_label = func_list->function_name(f_i);
	int start = -1;
	for (int i=0; i<program.size(); i++) 
		if(program[i].is_label_op())
			if(program[i].get_label().compare(local_label)) {
				// function start
				start = i;
				break;
			}

	if(start != -1) {
		vector<string> params = func_list->get_params(f_i);
		for (int i=0; i<params.size(); i++) {

			string leading_inst_pc = "";
			string local_reg = params[i];
			if(local_reg[1] == 'i')
				local_reg[1] = 'o';

			for(int k=start; k<program.size(); k++) {
				if(program[k].is_ret())
					break;
				if(program[k].find_use(local_reg)) {
					leading_inst_pc = program[k].get_leading_store_pc();
					break;
				}
			}
			func_list->set_leading_inst_pc(f_i, i, leading_inst_pc);
		}
		return true;
	}
	return false;
}

void instructions::call_site_flattening(func_list_t *func_list) {

	for(int i=0; i<program.size(); i++) {
		if(program[i].is_call_site()) {
#ifdef CALL_SITE_FLATTENING
			program[i].set_leading_store_pc(program[i].get_program_counter(), CALL_SITE);

			//find the function name and get the index of the label
			string function_name = program[i].get_op(0); // function name
			for(int j=0; j<program[i].get_num_call_parameters(); j++) {
				string local_reg =  program[i].get_call_parameter(j);
				if(program[i].is_register(local_reg)) {

					string leading_inst_pc = program[i].get_program_counter();
					dep_type_t local_dep_type = CALL_SITE_DEP;

					// find the first use of the parameter in the function body
					// get the leading store instruction pc
					if(func_list->find(function_name)) {
						string leading_pc = func_list->get_leading_inst_pc(function_name, local_reg);
						if(leading_pc.compare("") != 0) {
							leading_inst_pc = leading_pc;
							local_dep_type = STORE_DEP;
						}
					} else {
						// system calls or library function calls
					}
					mark_dependent_instructions(i, local_reg, leading_inst_pc, local_dep_type);
				}
			}
#endif //CALL_SITE_FLATTENING
		} 
	}
}
void instructions::ret_site_flattening(func_list_t *func_list) {
	for(int i=0; i<program.size(); i++) {
		if(program[i].is_ret()) {
#ifdef RET_FLATTENING
			program[i].set_leading_store_pc(program[i].get_program_counter(), RET_SITE);

			int k;
			for (k=i; k>0; k--) {
				if(program[k].is_save()) 
					break;
			}
			//label before save should be the function name
			k--;
			string function_name = "";
			if(program[k].is_label_op()){
				function_name = program[k].get_label();
			} else 
				cout << "Not Possible\n";

			for(int j=0; j<program[i].get_num_ret_parameters(); j++) {
				string local_reg =  program[i].get_ret_parameter(j);
				if(program[i].is_register(local_reg)) {

					string leading_inst_pc = program[i].get_program_counter();
					dep_type_t d_type = RET_DEP;

					if(func_list->find(function_name)) {
						string leading_pc = func_list->get_leading_inst_ret_val_pc(function_name, local_reg);
						if(leading_pc.compare("") != 0) {
							leading_inst_pc = leading_pc;
							d_type = STORE_DEP;
						}
					}
	
					//program[i].print_instruction();
					mark_dependent_instructions(i, local_reg, leading_inst_pc, d_type);
				}
			}
#endif 
		}
	}
}

string instructions::get_app_name() {
	return app_name;
}
