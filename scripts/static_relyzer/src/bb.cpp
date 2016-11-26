#include "bb.h"
#include <queue>
#include <sstream>
#define DEBUG 0

#define CONTROL_EXPLORE_DEPTH 6

using namespace std;

int sese_count = 0;
int bb_count = 0;

struct explore_list_elem_{
	bb_t * bb_ptr;
	string reg_name;
};
typedef struct explore_list_elem_ explore_list_elem;

bb_t::bb_t(int i, cfg_t * c) {
	// id = i;
	id = bb_count++;
	next_t = NULL;
	next_nt = NULL;
	parent_cfg = c;
	ends_with_call = false;
	call_cfg = NULL;
	last_instr_type = -1;
	post_dom_changed = false;

	visited = false;
	reachable = false;

	exit_points.clear();

	in.clear();
	out.clear();

//	in.insert("%g1");
//	in.insert("%g2");
//	in.insert("%g3");
//	in.insert("%g3");
//	in.insert("%g4");
//	in.insert("%g5");
//	in.insert("%g6");
//	in.insert("%g7");
//	in.insert("%i0");
//	in.insert("%i1");
//	in.insert("%i2");
//	in.insert("%i3");
//	in.insert("%i4");
//	in.insert("%i5");
//	in.insert("%i6");
//	in.insert("%i7");
//	in.insert("%o0");
//	in.insert("%o1");
//	in.insert("%o2");
//	in.insert("%o3");
//	in.insert("%o4");
//	in.insert("%o5");
//	in.insert("%o6");
//	in.insert("%o7");
}

bb_t::~bb_t() {
	predecessors.clear();
	instrns.clear();
	in.clear();
	out.clear();
	dom.clear();
	post_dom.clear();
	map<string, vector< list<string> > >::iterator it;
	for (it=pc_patterns.begin(); it != pc_patterns.end(); it++)  {
		vector< list<string> >::iterator vit;
		for (vit=(*it).second.begin(); vit != (*it).second.end(); vit++)  {
			(*vit).clear();
		}
		(*it).second.clear();
	}
	pc_patterns.clear();
}

void bb_t::add_inst(instruction_t * i) {
	instrns.push_back(i);
}

bool bb_t::is_empty() {
	if(instrns.size() == 0)
		return 1;
	else 
		return 0;
}

int bb_t::size() {
	return instrns.size();
}

void bb_t::move_instructions(string pc, bb_t * bb2) {
	int bb_size = instrns.size();
	int move_point = -1;
	for(int i=0; i<bb_size; i++) {
		if(instrns[i]->get_program_counter().compare(pc) == 0) {
			move_point = i;
			break;
		}
	}
	if(move_point != -1) {
		for(int i=move_point; i<bb_size; i++)  {
			bb2->add_inst(instrns[i]);
			instrns[i]->set_bb(bb2);
		}
	
		// remove elements from move_point
		for(int i=move_point; i<bb_size; i++) 
			instrns.pop_back();
	}
}

int bb_t::get_id() {
	return id;
}

string bb_t::first_instr_pc() {
	if(!is_empty())
		return instrns[0]->get_program_counter();
	if(parent_cfg->get_end() == this) 
		return "end";

	ostringstream temp;
    temp<<get_id();
	return temp.str();
}
string bb_t::second_instr_pc() {
	if(size()>=2)
		return instrns[1]->get_program_counter();
	return "";
}
string bb_t::last_1_instr_pc() {
	if(size()>=2)
		return instrns[instrns.size()-2]->get_program_counter();
	return "";
}
string bb_t::last_instr_pc() {
	if(!is_empty())
		return instrns[instrns.size()-1]->get_program_counter();
	return "";
}


void bb_t::prune_duplicate_uses() {
    int bb_size = instrns.size();
    // for each instruction in bb
    //

	for(int i=0; i<bb_size; i++) {
        set<string> dup_uses = instrns[i]->get_duplicate_uses();
        for(set<string>::iterator it=dup_uses.begin(); it!=dup_uses.end(); ++it) {
            //cout << "FOUND ONE!!!" << endl;
            string dup_use = *it;
            instrns[i]->remove_from_fault_list(dup_use, false);
        }
	}
}

void bb_t::prune_defs() {
	int bb_size = instrns.size();
    // for each instruction in bb
	for(int i=0; i<bb_size; i++) {
		string def = instrns[i]->get_def(); // getting def of instruction
		if(def.compare("") == 0) // ensure not null (don't need to prune)
				continue;
		if(instrns[i]->is_conditional_op())
				continue;

		bool found_use = false; //trying to find a use. if found, mark def as pruned away
		// search for first use
		int j = i+1;
		for(; j<bb_size; j++) {

			// if (instrns[i]->get_program_counter().compare("0x1000035e8") == 0) {
			//  	cout << found_use << ": " << endl;
			//  	instrns[j]->print_instruction();
			// }

			if(instrns[j]->find_use(def)) {
				// check if the use is not removed
				// this can happen only because of constant propagation
				if(!instrns[j]->is_op0_removed()) {
	 				instrns[j]->mark_for_extra_counting(def,instrns[i]->get_extra_count(def, true), instrns[i]->get_program_counter());
	 				found_use = true;
	 				break;
				} else {
					// def in this instruction should not be removed
					if(!instrns[j]->is_def_removed()) {
	 					instrns[j]->mark_for_extra_counting(def,instrns[i]->get_extra_count(def, true), instrns[i]->get_program_counter());
	 					found_use = true;
	 					break;
					} else {
						// found first use but couldnt mark the def for removing, so dont set found_use to true
						break;
					}
				}
			}
		}

		// if (instrns[i]->get_program_counter().compare("0x1000035e8") == 0) {
		//  	cout << found_use << ": " << endl;
		//  	instrns[i]->print_instruction();
		//  	exit(1);
		// }

		// if the use is found in the straight line code, then simply continue
		// to the next def/instruction
		if(found_use) {
	 		instrns[i]->remove_from_fault_list(def, true);
			continue;
		}

		continue; // WARNING: for now, do this only within basic block

		// on every path forward, look for a use before def 

		set<int> visited; // store the id of visited bbs

		queue <explore_list_elem> explore_list;
		explore_list_elem elem1;
		elem1.bb_ptr = next_t;
		elem1.reg_name = def;
		explore_list.push(elem1);

		explore_list_elem elem2;
		elem2.bb_ptr = next_nt;
		elem2.reg_name = def;
		if(is_last_inst_restore()) { // do this only for case when call is taken 
			elem2.reg_name = reg_after_restore(def);
		}

		//if (instrns[i]->get_program_counter().compare("0x100003234") == 0) {
		//	cout << "added:" << elem2.reg_name << ":" << curr_bb->first_instr_pc() << endl;
		//}
		explore_list.push(elem2);

		while(!explore_list.empty()) {
			bb_t * curr_bb = explore_list.front().bb_ptr;
			string reg = explore_list.front().reg_name;
			if(curr_bb != NULL && reg.compare("") != 0) {
				if( visited.find(curr_bb->get_id()) == visited.end() ) { // this bb is not visited before
					visited.insert(curr_bb->get_id()); // mark this bb visited

					if(curr_bb->is_first_inst_save()) {
						reg = reg_after_save(reg);
					}

					int res = curr_bb->has_use_before_def(reg);
					// return -1: no uses or defs
					// return 0: found a def before any use
					// return >=1: has a use before def and ret_val is the index

					// if (instrns[i]->get_program_counter().compare("0x1000026e0") == 0) {
					// 	cout << reg << ":" << res << ":" << curr_bb->first_instr_pc() << ": " << endl;
					// }

					if(res >= 1) {
						// stop exploring and return 
						curr_bb->mark_for_extra_counting(res,def, "0x0");
						found_use = true;

					} else if(res == -1) {

						// continue exploring
						explore_list_elem elem1;
						elem1.bb_ptr = curr_bb->next_t;
						elem1.reg_name = reg; 
						// if it ends with a call, last instr in a restore 
						if( curr_bb->is_last_inst_restore() ) {
							elem1.reg_name = reg_after_restore(reg); 
						}
						explore_list.push(elem1);

						explore_list_elem elem2;
						elem2.bb_ptr = curr_bb->next_nt;
						elem2.reg_name = reg;
						explore_list.push(elem2);

						// if (instrns[i]->get_program_counter().compare("0x100003128") == 0) {
						// 	cout << elem1.bb_ptr->first_instr_pc() << ":" << elem1.reg_name << ":" << curr_bb->first_instr_pc() << ": " << endl;
						// 	cout << elem2.bb_ptr->first_instr_pc() << ":" << elem2.reg_name << ":" << curr_bb->first_instr_pc() << ": " << endl;
						// }

					}
				}
			}
			explore_list.pop(); // remove first element from explore list
		}

		if(found_use) {
	 		instrns[i]->remove_from_fault_list(def, true);
		} else {
			//instrns[i]->print_instruction();
		}

		// if (instrns[i]->get_program_counter().compare("0x1000026e0") == 0) {
		// 	cout << found_use << ": " << endl;
		// 	instrns[i]->print_instruction();
		// 	exit(1);
		// }
	}

}

void bb_t::update_out() {
	out = in;

	// add all instructions to the 'out' to start with
	int bb_size = instrns.size();
	for(int i=0; i<bb_size; i++) {
		string def = instrns[i]->get_def();
		if(def.compare("") != 0) 
			out[def] = i; // last def will be recorded, along with its index

		// do not add uses to the out set

		//set<string> uses = instrns[i]->get_uses();
		//set<string>::iterator it;
		//for (it=uses.begin(); it != uses.end(); it++)  {
		//	if(out.find((*it)) == out.end())  {
		//		out.insert(*it);
		//		// cout << "adding = " << (*it) << endl;
		//	}
		//}
	}

	map<string,int>::iterator it;

	#define DEBUG_LIVE_REG 0
	if(DEBUG_LIVE_REG) {
		cout << first_instr_pc() << ":" << size() << ":" << ":out={ ";
		for (it=out.begin(); it != out.end(); it++)  {
			string reg = (*it).first;
			cout << reg << ", ";
		}
		cout << "} " << endl;
	}

	double num_faults = 0.0;
	// if(!is_empty())
	// 	num_faults += out.size()*((double)instrns[0]->get_pruned_c_line_frequency()/1000000);
	// cout << "initial_faults = " << num_faults << endl;


	int num_deleted = 0;
	// if ends_with_call and reg is g, f, d, or o
	//		dont do anything
	// if ends_with_call and reg is l 
	//		do the normal thing do anything
	// 'i' registers are used for retuning values from a function hence,
	// conservatively mark all 'i' register defs are live unless they are
	// killed within

	// on every path forward, look for defs before uses
	// for all paths if no use exists before a def then remove it from out

	//cout << "erasing:";
	string check_pc = "0x100124db8as";
	if(first_instr_pc().compare(check_pc) == 0) {
		cout << endl;
		cout << first_instr_pc() << ":" << size() << ":" << num_deleted << ":out={ ";
		for (it=out.begin(); it != out.end(); it++)  {
			string reg = (*it).first;
			cout << reg << ", ";
		}
		cout << "} " << endl;
	}
	for (it=out.begin(); it != out.end();)  {
		map<string,int>::iterator erase_it = it++; 
		string reg = (*erase_it).first;
		bool remove = false;

		int why = -10;
		queue<bb_t*> explore_list;
		set<int> visited; // store the id of visited bbs

		// if it does not end with call or jump 
		bool cond = !(ends_with_call || last_instr_type == JUMP_TYPE);
		// if it ends with a call and does NOT do a restore then the reg should be a l or i
		cond = cond || (ends_with_call && !is_last_inst_restore() && ( reg.find('l') != string::npos || reg.find('i') != string::npos ));
		// if it ends with a call and does a restore then the all the registers will be live in the next function body
		cond = cond || (ends_with_call && is_last_inst_restore() && false);
		// cond = cond || (ends_with_call && reg.find('l') != string::npos );
		if(cond) {
			explore_list.push(next_t);
			explore_list.push(next_nt);

			if(DEBUG_LIVE_REG) {
				if(next_nt != NULL) {
					cout << "adding t: " << next_nt->get_id() << " first_pc: " << next_nt->first_instr_pc() << endl;
				} 
				if (next_t != NULL) {
					cout << "adding nt: " << next_t->get_id() << " first_pc: " << next_t->first_instr_pc() << endl;
				}
			}
		}
		while(!explore_list.empty()) {
			bb_t * curr_bb = explore_list.front();
			if(curr_bb != NULL) {
				if(visited.find(curr_bb->get_id()) == visited.end()) { // this bb is not visited before
					visited.insert(curr_bb->get_id()); // mark this bb visited
					
					//if(DEBUG_LIVE_REG) 
					if(first_instr_pc().compare(check_pc) == 0) 
						cout << reg << ":visiting: " << curr_bb->get_id() << " first_pc: " << curr_bb->first_instr_pc() << endl;

					int res = curr_bb->has_use_before_def(reg);
					// return -1: no uses or defs
					// return 0: found a def before any use
					// return >=1: has a use before def and ret_val is the index
					if(first_instr_pc().compare(check_pc) == 0) 
						cout << " res = " << res << endl;

					if(res == 0) {
						remove = true;
						why = 0;
						// no need to explore the next_t and next_nt of current node 
					} else if(res >= 1) {
						// stop exploring and return failure
						// cout << "returnting false" << endl;
						remove = false;
						why = 10;
						break; 
					} else if(res == -1) {

						bool cond = !(curr_bb->does_end_with_call() || (curr_bb->get_last_instr_type() == JUMP_TYPE));
						cond = cond || ((curr_bb->does_end_with_call() || (curr_bb->get_last_instr_type() == JUMP_TYPE)) && ( reg.find('l') != string::npos || reg.find('i') != string::npos ));
						//cond = cond || ((curr_bb->does_end_with_call() || (curr_bb->get_last_instr_type() == JUMP_TYPE)) && reg.find('l') != string::npos );
						if(cond) {
							// if it ends with a call, last instr in a restore 
							// then 'i' regs should be live as it can be used in the function body
							// We dont remove them from the the out set and quit exploring
							if(curr_bb->does_end_with_call() && curr_bb->is_last_inst_restore() && reg.find('i') != string::npos) {
								remove = false;
								break;
							}
							// if you reach a return instruction and the
							// register under consideration is o then terminate
							// the search and diclare it live
							if(curr_bb->get_last_instr_type() == RET_TYPE && reg.find('i') != string::npos) {
								remove = false;
								break;
							}

							// continue exploring
							explore_list.push(curr_bb->next_t);
							explore_list.push(curr_bb->next_nt);
							
							remove = true;
							why = -1;
							// do not remove %o registers becuase they may be used after the function returns
							// do not remove %d registers becuase they may be used after the function returns
							// do not remove %f registers becuase they may be used after the function returns
							// do not remove %i registers becuase they may be used after the function returns
							// on return in registers become out registers
							if(reg.find('o') != string::npos || reg.find('d') != string::npos || reg.find('f') != string::npos || reg.find('i') != string::npos ) {
								remove = false;
							}
						} else {
							if(reg.find('g') != string::npos && curr_bb->does_end_with_call()) {
								remove = false;
								break;
							}
							if(reg.find('o') != string::npos && curr_bb->does_end_with_call()) {
								remove = false;
								break;
							}
						}
					}
				}
			}
			explore_list.pop(); // remove first element from explore list
		}

		// if this is the last basic block of the function and the reg is local ('l')
		if(last_instr_type == RET_TYPE && reg.find('l') != string::npos ) { 
				remove = true;
		}
		if(remove) {
			if(DEBUG_LIVE_REG)
				cout << reg << ":" << why << ", ";
			out.erase(erase_it);
			num_deleted++;
		}
	}
	if(first_instr_pc().compare(check_pc) == 0) {
		cout << endl;
		cout << first_instr_pc() << ":" << size() << ":" << num_deleted << ":out={ ";
		for (it=out.begin(); it != out.end(); it++)  {
			string reg = (*it).first;
			cout << reg << ", ";
		}
		cout << "} " << endl;
	}

	if(DEBUG_LIVE_REG) {
		cout << endl;
		cout << first_instr_pc() << ":" << size() << ":" << num_deleted << ":out={ ";
		for (it=out.begin(); it != out.end(); it++)  {
			string reg = (*it).first;
			cout << reg << ", ";
		}
		cout << "} " << endl;
	}

	for (it=out.begin(); it != out.end(); it++)  {
		string reg = (*it).first;
		int instr_index = (*it).second;

		if(instrns[instr_index]->get_dep_type() == STORE) {
			instrns[instr_index]->mark_instruction_force(instrns[bb_size-2]->get_program_counter(), STORE_AND_LIVE, bb_size-1-instr_index);
		} else {
			//instrns[instr_index]->mark_instruction(instrns[bb_size-1]->get_program_counter(), LIVE, bb_size-1-instr_index);
			instrns[instr_index]->mark_instruction(instrns[instr_index]->get_program_counter(), LIVE, 0);
		}

		// if this is a conditional op or if the previous instruction is a branch instruction with annul bit set
		// then mark the insturcitons with the same def as LIVE
		bool cond = false;
		cond |=  instrns[instr_index]->is_conditional_op();
		if(instr_index>0) {
			cond |= instrns[instr_index-1]->is_branch_with_annul_bit();
		}

		if(cond) {
			// go back and mark the other def as LIVE
			for(int j=instr_index-1; j>=0; j--) {
				string def = instrns[j]->get_def();
				if(def.compare(reg) == 0)  {
			
					if(instrns[j]->get_dep_type() == STORE) {
						instrns[j]->mark_instruction_force(instrns[bb_size-2]->get_program_counter(), STORE_AND_LIVE, bb_size-1-j);
					} else {
						instrns[j]->mark_instruction(instrns[j]->get_program_counter(), LIVE, 0);
						//instrns[j]->mark_instruction(instrns[bb_size-1]->get_program_counter(), LIVE, bb_size-1-j);
					}
					// check again if cond is true
					cond |=  instrns[j]->is_conditional_op();
					if(j>0) {
						cond |= instrns[j-1]->is_branch_with_annul_bit();
					}
					if(!cond)
							break;
				}
			}
		}
	}

	num_faults = 0.0;
	if(!is_empty())
		num_faults += out.size()*((double)instrns[0]->get_pruned_c_line_frequency()/1000000);
	//cout << "num_faults = " << num_faults << endl;
}

// return -1: no uses or defs
// return 0: found a def before any use
// return >=1: has a use before def and ret_val is the index
int bb_t::has_use_before_def(string reg) {
	int bb_size = instrns.size();
	bool has_function_call = false;
	for(int i=0; i<bb_size; i++) {
		//instrns[i]->print_instruction() ;
		set<string> uses = instrns[i]->get_uses();
		set<string>::iterator it;
		for (it=uses.begin(); it != uses.end(); it++)  {
			if(reg.compare((*it)) == 0)  {
				// found use before def
				//cout << instrns[i]->get_program_counter() << ":found use before def:" << (*it) << endl ;
				return i+1;
			}
			//cout << instrns[i]->get_program_counter() << ":use:" << (*it) << endl ;
		}

		string def = instrns[i]->get_def();
		if(reg.compare(def) == 0) {
			//cout << instrns[i]->get_program_counter() << ":def:" << def << endl ;
			// found def before use
			if(instrns[i]->is_conditional_op()) {
				// if this is a condictional instruction then treat it as a use
				return i+1;
			}
			if(i>0) {
				if(instrns[i-1]->is_branch_with_annul_bit()) {
					// if the previous instruction has annul bit set then treat it as a use
					return i+1;
				}
			}
			return 0;
		}
		if(instrns[i]->is_call_site()) {
			has_function_call = true;
		}
	}

	// special case
	// if there is a function call and the we have a %o register then assume that a use is found
	if(has_function_call && reg.substr(0,2).compare("%o") == 0) 
		return 1;

	// if there is a function call and the we have a %o register then assume that a use is found
	if(last_instr_type == JUMP_TYPE) 
			return 1;
	

	return -1;
}

void print_vector_list(vector< list<string> > vl) {
	for(int i=0; i<vl.size(); i++) {
		list<string>::iterator it; 
		for(it=vl[i].begin(); it!=vl[i].end(); it++) {
				cout<< (*it) << " ";
		}
		cout<< endl;
	}
	cout<< endl;
}

long bb_t::gen_control_patterns(string prg_name) {
	long num_patterns = 0;
	int bb_size = instrns.size();
	map<string,int>::iterator it;

	// for all bbs 
	string pc;
	pc = instrns[bb_size-1]->get_program_counter();
	if(pc.compare("") == 0) 
		return 0;


	list<string> pc_list;
	pc_list.push_back(instrns[bb_size-1]->get_program_counter());
	
	if(next_t != NULL) {
		// if this bb that ends with a function call then explore the function and then return to the next bb
		vector< list<string> > pc_lists;
		if(does_end_with_call()) {
			if(get_call_cfg_start() != NULL)  {
				pc_lists = get_call_cfg_start()->get_cpattern(pc_list, CONTROL_EXPLORE_DEPTH);
			}
		}

		if(pc_lists.size() == 0) {
			pc_patterns[pc] = next_t->get_cpattern(pc_list, CONTROL_EXPLORE_DEPTH);
			//cout << first_instr_pc() << "->" << next_t->first_instr_pc() << endl;
		} else {
			for(int j=0; j<pc_lists.size(); j++) {
				vector< list<string> > pc_lists_temp = next_t->get_cpattern(pc_lists[j], CONTROL_EXPLORE_DEPTH);
				for(int k=0; k<pc_lists_temp.size(); k++) {
					pc_patterns[pc].push_back(pc_lists_temp[k]);
				}
			}
		}
	}

	if(next_nt != NULL) {
		//cout << first_instr_pc() << "->" << next_nt->first_instr_pc() << endl;
		vector< list<string> > pc_lists = next_nt->get_cpattern(pc_list, CONTROL_EXPLORE_DEPTH);
		for(int j=0; j<pc_lists.size(); j++) {
			pc_patterns[pc].push_back(pc_lists[j]);
		}
	}
	//cout << reg << ":";
	string out_file = "output/" + prg_name + "_control_patterns.txt";
	ofstream fout;
	fout.open(out_file.c_str(), ios::out | ios::app);
	fout << pc << ": ";
	for(int i=0; i<pc_patterns[pc].size(); i++) {
		list<string>::iterator it; 
		for(it=pc_patterns[pc][i].begin(); it!=pc_patterns[pc][i].end(); it++) {
			fout<< (*it) << " ";
		}
		fout << " - ";
	}
	fout << "\n";

	num_patterns += pc_patterns[pc].size();

	//print_vector_list(pc_patterns[pc]);

	return num_patterns;

}

vector< list<string> > bb_t::get_cpattern(list<string> pc_list, int depth) {
	vector< list<string> > ret_list;
	int bb_size  = instrns.size();
	if(depth > 0 &&  bb_size > 0) {
		if(instrns[0]->get_c_line_frequency() > 0) {
			// add all instructions in this block to the pattern list
			for(int i=0; i<bb_size; i++) {
				pc_list.push_back(instrns[i]->get_program_counter());
			}

			//explore next_t and next_nt nodes
			if(next_t != NULL) {
				// if this bb that ends with a function call then explore the function and then return to the next bb
				vector< list<string> > pc_lists;
				if(does_end_with_call()) {
					if(get_call_cfg_start() != NULL)  {
						pc_lists = get_call_cfg_start()->get_cpattern(pc_list, depth-1);
					}
				}
				if(pc_lists.size() == 0) {
					ret_list = next_t->get_cpattern(pc_list, depth-1);
					//cout << first_instr_pc() << "->" << next_t->first_instr_pc() << endl;
				} else {
					for(int j=0; j<pc_lists.size(); j++) {
						vector< list<string> > pc_lists_temp = next_t->get_cpattern(pc_lists[j], depth-1);
						for(int k=0; k<pc_lists_temp.size(); k++) {
							ret_list.push_back(pc_lists_temp[k]);
						}
					}
				}
			}
			if(next_nt != NULL) {
				//cout << first_instr_pc() << "->" << next_nt->first_instr_pc() << endl;
				vector< list<string> > pc_lists = next_nt->get_cpattern(pc_list, depth-1);
				for(int j=0; j<pc_lists.size(); j++) {
					ret_list.push_back(pc_lists[j]);
				}
			}
			if(ret_list.size() == 0) {
				ret_list.push_back(pc_list);
			}
		}
	}
	return ret_list;
}

void bb_t::find_invariant_detectors() {
	string denom = "";
	vector <string> numerators;
	int bb_size = instrns.size();
	for(int i=0; i<bb_size; i++ ) {
		if(instrns[i]->is_add_op()) {
			// we know that all three operands exist
			// look for an increment operation
			// first operand should be equal to the third and the second should be a constant
			if(instrns[i]->get_op(0).compare(instrns[i]->get_op(2)) == 0) {
				if(instrns[i]->get_op(1).compare("0x1") == 0) {
					denom = instrns[i]->get_op(0);
				} else if(instrns[i]->get_op(1).substr(0,2).compare("0x") == 0) {
					string str = instrns[i]->get_op(0) + "," + instrns[i]->get_op(1);
					numerators.push_back(str);
				}
			}
		}
	}
	string check_pc = "";
	if(next_nt == NULL) {
		if(next_t != NULL) {
			vector<string> temp_numerators = next_t->find_incement_regs(check_pc);
			for(int i=0; i<temp_numerators.size(); i++) {
				numerators.push_back(temp_numerators[i]);
			}
		}
	} else {
		check_pc = next_nt->first_instr_pc();
	}
	if(denom.compare("") != 0) {
		if(numerators.size() >= 1) {
			cout << instrns[0]->get_program_counter() << ":" << check_pc << ":";
			for(int i=0; i<numerators.size(); i++) {
				cout << numerators[i] << "," << denom << " " ;
			}
			cout << endl;
		}
	}
}

vector<string> bb_t::find_incement_regs(string & pc) {
	vector<string> numerators;
	int bb_size = instrns.size();
	for(int i=0; i<bb_size-1; i++ ) {
		if(instrns[i]->is_add_op()) {
			// we know that all three operands exist
			// look for an increment operation
			// first operand should be equal to the third and the second should be a constant
			if(instrns[i]->get_op(0).compare(instrns[i]->get_op(2)) == 0) {
				if(instrns[i]->get_op(1).substr(0,2).compare("0x") == 0) {
					string str = instrns[i]->get_op(0) + "," + instrns[i]->get_op(1);
					numerators.push_back(str);
				}
			}
		}
	}
	pc = "";
	if(next_nt == NULL) {
		if(next_t != NULL) {
			vector<string> temp_numerators = next_t->find_incement_regs(pc);
			for(int i=0; i<temp_numerators.size(); i++) {
				numerators.push_back(temp_numerators[i]);
			}
		}
	} else {
		pc = next_nt->first_instr_pc();
	}

	return numerators;
}

void bb_t::mark_nops() {
	int bb_size = instrns.size();
	if(bb_size>=2) {
		instruction_t * inst = instrns[bb_size-1];

		//delay slot instructions with dest as g0. These instructions should not be modifying condition codes
		if(instrns[bb_size-2]->is_branch_op() && !inst->is_restore() && inst->get_def().compare("\%g0") == 0 && ( (inst->is_arithmetic_op() && !inst->is_addcc()) || (inst->is_logical_op() && !inst->is_orcc()) || inst->is_mov_op() || inst->is_movcc_op()) ) {
			//inst->print_instruction();
			inst->make_nop();
			//inst->print_instruction();
		}
		// i have seen cases like: (example from gcc)
		// 0x1003be310:   mov       0x1, %o0
		// 0x1003be314:   retl
		// 0x1003be318:   mov       0x1, %o0
		int i=bb_size-3;
		if(i>=0) { 
			bool cond = inst->get_opcode().compare(instrns[i]->get_opcode()) == 0;
			for (int j=0; j<NUM_OPS; j++) {
				cond &= inst->get_op(j).compare(instrns[i]->get_op(j)) == 0;
			}
			cond &= (inst->get_opcode().compare("mov") == 0 || inst->get_opcode().compare("clr") == 0); // one of mov or clr instructions
			
			if(cond) {
				cout << instrns[i]->get_program_counter() << endl;
				instrns[i]->print_instruction();
				instrns[i]->make_nop();
			}
		}
	}

	// for load instructions with destination as g0
	for(int i=0; i<bb_size; i++ ) {
		if(instrns[i]->is_load_op() && instrns[i]->get_def().compare("\%g0") == 0) {
			cout << instrns[i]->get_program_counter() << endl;
			instrns[i]->print_instruction();
			instrns[i]->make_nop();
		}
	}
}

void bb_t::mark_store_instructions() {
	int bb_size = instrns.size();
	for(int i=0; i<bb_size; i++ ) {
		if(instrns[i]->is_store_op()) {
			set<string> uses = instrns[i]->get_uses();
			set<string>::iterator it;
			for (it=uses.begin(); it != uses.end(); it++)  { // for all reg uses in this instruction
				string reg = *it;
				mark_leading_instructions(i, reg, instrns[i]->get_program_counter(), STORE, i); 
			}
			instrns[i]->set_leading_store_pc(instrns[i]->get_program_counter(), STORE, 0);

		} else if(instrns[i]->is_clr_mem()) {
			set<string> uses = instrns[i]->get_uses();
			set<string>::iterator it;
			for (it=uses.begin(); it != uses.end(); it++)  { // for all reg uses in this instruction
				string reg = *it;
				mark_leading_instructions(i, reg, instrns[i]->get_program_counter(), STORE, i); 
			}
			instrns[i]->set_leading_store_pc(instrns[i]->get_program_counter(), STORE, 0);

		}
	}
}

void bb_t::mark_control_instructions() {
//	cout << first_instr_pc() << endl;
	int bb_size = instrns.size();
	for(int i=0; i<bb_size; i++ ) {
		string pc = instrns[bb_size-1]->get_program_counter(); // pc of last instruction of the bb
		if(instrns[i]->is_compare_op()) {
			dep_type_t dep = COMPARE;

			if(get_last_instr_type() == BRANCH_TYPE && bb_size > 2) {
				dep = CONTROL;
			}

			// for distribution
			if (instrns[i]->is_register(instrns[i]->get_op(0)))
				mark_leading_instructions(i, instrns[i]->get_op(0), pc, dep, bb_size-1);

			if (instrns[i]->is_register(instrns[i]->get_op(1)))
				mark_leading_instructions(i, instrns[i]->get_op(1), pc, dep, bb_size-1);

			if(instrns[i]->is_fcompare_op())
				if (instrns[i]->is_register(instrns[i]->get_op(2)))
					mark_leading_instructions(i, instrns[i]->get_op(2), pc, dep, bb_size-1);

			// for direction
			instrns[i]->set_leading_store_pc(pc, dep, bb_size-1-i);

		} else if(instrns[i]->is_branch_op()) {
			// i should be bb_size - 2
			// if first operand is a register then mark leading instructions as branch_dep
			if (instrns[i]->is_register(instrns[i]->get_op(0)))
				mark_leading_instructions(i, instrns[i]->get_op(0), pc, CONTROL, bb_size-1);

			instrns[i]->set_leading_store_pc(pc, CONTROL, bb_size-1-i);

		} else if(instrns[i]->is_jump()) {
			// i should be bb_size - 2
			// if first operand is a register then mark leading instructions as branch_dep
			set<string> uses = instrns[i]->get_uses();
			set<string>::iterator it;
			for (it=uses.begin(); it != uses.end(); it++)  { // for all reg uses in this instruction
				string local_reg = *it;
				mark_leading_instructions(i, local_reg, pc, CONTROL, bb_size-1);
			}

			instrns[i]->set_leading_store_pc(pc, CONTROL, bb_size-1-i);
		}
	}
}

void bb_t::mark_other_instructions() {
//	cout << first_instr_pc() << endl;
	int bb_size = instrns.size();
	for(int i=0; i<bb_size; i++ ) {
		if(instrns[i]->is_call_site()) {
			instrns[i]->set_leading_store_pc(instrns[i]->get_program_counter(), CALL_SITE, 0);

		} else if(instrns[i]->is_illtrap()) {
			instrns[i]->set_leading_store_pc(instrns[i]->get_program_counter(), ILLTRAP, 0);

		} else if(instrns[i]->is_ret()) {
			instrns[i]->set_leading_store_pc(instrns[i]->get_program_counter(), RET_SITE, 0);

		// } else if(instrns[i]->is_save()) {
		// 	instrns[i]->set_leading_store_pc(instrns[i]->get_program_counter(), SAVE_SITE, 0);

		} else if(instrns[i]->is_read_state_reg()) {
			instrns[i]->set_leading_store_pc(instrns[i]->get_program_counter(), STATEREG_OP, 0);

		} else if(instrns[i]->is_write_state_reg()) {
			instrns[i]->set_leading_store_pc(instrns[i]->get_program_counter(), STATEREG_OP, 0);

			if (instrns[i]->is_register(instrns[i]->get_op(0)))
				mark_leading_instructions(i, instrns[i]->get_op(0), instrns[i]->get_program_counter(), STATEREG_OP, 0);

			if (instrns[i]->is_register(instrns[i]->get_op(1)))
				mark_leading_instructions(i, instrns[i]->get_op(1), instrns[i]->get_program_counter(), STATEREG_OP, 0);
		}
	}
}

//leading_index = the index of the instruction that forms the terminating point of the chain of instrctions that lead to it
void bb_t::mark_leading_instructions(int bb_index, string local_def, string leading_store_pc, dep_type_t dep_type, int leading_index) {

	dep_type_t d_type;
	d_type = dep_type;

	for(int i=bb_index-1; i>=0; i--) {

		// if the leading store pc is set, then skip this instruction
		// it's been taken care of before
		if(instrns[i]->get_leading_store_pc().compare("") != 0) 
			continue;

		if(!instrns[i]->is_store_op()) {

			if(is_def_matching(instrns[i]->get_def(), local_def))  {  // reached the def 
			// if(instrns[i]->get_def().compare(local_def) == 0)  {  // reached the def 
				instrns[i]->set_leading_store_pc(leading_store_pc, dep_type, leading_index-i);

				set<string> uses = instrns[i]->get_uses();
				set<string>::iterator it;
				for (it=uses.begin(); it != uses.end(); it++)  { // for all reg uses in this instruction
					string local_reg = *it;
					mark_leading_instructions(i, local_reg, leading_store_pc, d_type, leading_index);
				}
				if(instrns[i]->is_conditional_op()) {
					mark_leading_instructions(i, instrns[i]->get_def(), leading_store_pc, d_type, leading_index);
				}
				if(i>0) {
					if(instrns[i-1]->is_branch_with_annul_bit()) { 
							// if instructions in delay slot of a branch instruction with annule bit set
							// then continue marking instructions with the def of this instruction
						mark_leading_instructions(i, instrns[i]->get_def(), leading_store_pc, d_type, leading_index);
					}
				}

				return; // found the def. You can now return 
			}
		}
	}
}

bool bb_t::is_def_matching(string def, string reg) {
	// check if the reg is int
	if(def.compare(reg) == 0) {
		return true;
	} else if( reg[0] == '%' && reg[1] == 'f' ) { // if single-precision floating point register 

		int f_reg_no = atoi( reg.substr(2).c_str() ); 
		//cout << " floating point register number = " << f_reg_no << endl;

		if(f_reg_no%2 == 0) { // even register number
			stringstream ss; 
			ss << f_reg_no;
			string new_reg = "%d" + ss.str();
			if(def.compare(new_reg) == 0)  {
				//cout << " matched "<< reg <<" with "<< new_reg << endl;
				return true;
			}

		} else if(f_reg_no%2 == 1) {// f_reg_no is odd
			stringstream ss; 
			ss << f_reg_no-1;
			string new_reg = "%d" + ss.str();
			if(def.compare(new_reg) == 0)  {
				//cout << " matched "<< reg <<" with "<< new_reg << endl;
				return true;
			}
		}

	} else if( reg[0] == '%' && reg[1] == 'd' ) { // if double-precision floating point register 
		int d_reg_no = atoi( reg.substr(2).c_str() ); 
		//cout << " d" << d_reg_no << endl;

		stringstream ss; 
		ss << d_reg_no;
		string new_reg = "%f" + ss.str();
		if(def.compare(new_reg) == 0) {
			//cout << " matched "<< reg <<" with "<< new_reg << endl;
			return true;
		} else {
			ss.str("");
			ss << d_reg_no+1;
			new_reg = "%f" + ss.str();
			if(def.compare(new_reg) == 0) {
				//cout << " matched "<< reg <<" with "<< new_reg << endl;
				return true;
			}
		} 
	}

	return false;
}

void bb_t::mark_unlive_registers() {
	int bb_size = instrns.size();
	for(int i=0; i<bb_size; i++ ) {
		if(instrns[i]->get_dep_type() == -1) { // no one else has marked the instruction
			if(instrns[i]->is_restore()) {
				// this is a hack for now - restore instruction is the last
				// instruction in a bb.  If its not acting as a nop then it
				// should is usually live some times it acts as nop - for now
				// we are conservatively injecting faults in it. Ideally we
				// should just ignore all faults in this instruction

				instrns[i]->mark_instruction_force(instrns[i]->get_program_counter(), LIVE, 0);
			} else if(!(instrns[i]->is_call_site() || instrns[i]->is_ret())) {
				if(instrns[i]->get_def().compare("") == 0) {
					//cout << "no def: " << instrns[i]->get_program_counter() << ": " ;
					//instrns[i]->print_instruction() ;
				} else {
					if(out.find(instrns[i]->get_def()) == out.end()) { // def is not found in live set
						int instr_index = find_leading_live_def(i);
			
						if(instr_index >= 0) {

							if(instrns[i]->get_dep_type() == STORE) {
								instrns[i]->mark_instruction_force(instrns[bb_size-2]->get_program_counter(), STORE_AND_LIVE, bb_size-1-i);
							} else {
								//instrns[i]->mark_instruction(instrns[bb_size-1]->get_program_counter(), LIVE, bb_size-1-i);
								instrns[i]->mark_instruction(instrns[i]->get_program_counter(), LIVE, 0);
							}
						} else if(instr_index == -2) {
								if(instrns[i]->get_c_line_frequency() == 0) {
									instrns[i]->mark_instruction("", DEAD, 0);
								} else {
									//instrns[i]->mark_instruction("", REMAINING_OTHERS, 0);
									instrns[i]->mark_instruction("", DEAD, 0);
								}
						}
					} else { // def is found in live set 
						if(is_def_killed_within(instrns[i]->get_def(), i))  {
							int instr_index = find_leading_live_def(i);
							if(instr_index >= 0) {
	
								if(instrns[i]->get_dep_type() == STORE) {
									instrns[i]->mark_instruction_force(instrns[bb_size-2]->get_program_counter(), STORE_AND_LIVE, bb_size-1-i);
								} else {
									//instrns[i]->mark_instruction(instrns[bb_size-1]->get_program_counter(), LIVE, bb_size-1-i);
									instrns[i]->mark_instruction(instrns[i]->get_program_counter(), LIVE, 0);
								}

							} else if (instr_index == -2) {
								if(instrns[i]->get_c_line_frequency() == 0) {
									instrns[i]->mark_instruction("", DEAD, 0);
								} else {
									instrns[i]->mark_instruction("", REMAINING_OTHERS, 0);
								}
							}
						} else {
							// for all the instructions that are found in live set 
							// remove faults from uses; keep faults only in def
							// instrns[i]->prune_uses();
						}
					}
				}
			}
		}
	}
}

// return -2 if no use is found 
// find the live register definition that uses this register
int bb_t::find_leading_live_def(int index) {
	string search_def = instrns[index]->get_def();
	int i;
	bool found_use = false;
	for(i=index+1; i<instrns.size(); i++ ) {
		bool local_found_use = false;
		set<string> uses = instrns[i]->get_uses();
		set<string>::iterator it;

		if(uses.find(search_def) != uses.end()) { // found a use
			local_found_use = true;

		} else { // register with same name is not found
			// special case for single and double precision registers
			// check for single-precision and double-precision registers

			for (it=uses.begin(); it != uses.end(); it++)  {
				string reg = (*it);

				if( reg[0] == '%' && reg[1] == 'f' ) { // if single-precision floating point register 
					int f_reg_no = atoi( reg.substr(2).c_str() ); 
					stringstream ss; 
					ss << f_reg_no - f_reg_no%2; // subtract 1 if f_reg_no is odd
					string new_reg = "%d" + ss.str();
					if(search_def.compare(new_reg) == 0)  {
						//cout << " matched "<< reg <<" with "<< new_reg << endl;
						local_found_use = true; 
					}

				} else if( reg[0] == '%' && reg[1] == 'd' ) { // if double-precision floating point register 
					int d_reg_no = atoi( reg.substr(2).c_str() ); 
					stringstream ss; 
					ss << d_reg_no;
					string new_reg = "%f" + ss.str();
					if(search_def.compare(new_reg) == 0) {
						//cout << " matched "<< reg <<" with "<< new_reg << endl;
						local_found_use = true;
					} else {
						ss.str("");
						ss << d_reg_no+1;
						new_reg = "%f" + ss.str();
						if(search_def.compare(new_reg) == 0) {
							//cout << " matched "<< reg <<" with "<< new_reg << endl;
							local_found_use = true;
						}
					}
				}
			}
		}
		if(local_found_use) {

			if(out.find(instrns[i]->get_def()) != out.end()) { // def is live
				return i;
			} else {
				search_def =  instrns[i]->get_def();
				if(search_def.compare("") == 0) {
					//instrns[i]->print_instruction();
					return -1;
				}
			}
			found_use = true;
		}

	}
	if(!found_use) {
		return -2;
	}
	//cout << instrns[index]->get_program_counter() << " returning -1 : ";
	//cout << "search def = " << search_def << " out = " ;

	//map<string,int>::iterator it;
	//for (it=out.begin(); it != out.end(); it++)  {
	//	string reg = (*it).first;
	//	cout << reg << ", ";
	//}
	//cout << endl; 

	return -1;
}

bool bb_t::is_def_killed_within(string reg, int index) {
	for(int i=index+1; i<instrns.size(); i++ ) {
		if(reg.compare(instrns[i]->get_def()) == 0) { // another def of same register found
			if(i>0) {
				if (!instrns[i-1]->is_branch_with_annul_bit()) 
					return true;
			} else {
				if(!instrns[i]->is_movcc_op()) 
					return true;
			}
		}
	}
	return  false;
}

void bb_t::set_dom(set<int> N) {
	dom = N;
}
void bb_t::set_post_dom(set<int> N) {
	post_dom = N;
}

void bb_t::set_dom(int d) {
	dom.clear();
	dom.insert(d);
}
void bb_t::set_post_dom(int d) {
	post_dom.clear();
	post_dom.insert(d);
}

set<int> bb_t::get_dom() {
		return dom;
}
set<int> bb_t::get_post_dom() {
		return post_dom;
}

set<int> intersect(set<int> set1, set<int> set2) {
	set<int> out_set;
	
	if(set1.empty() || set2.empty())
		return out_set;
			
	set<int>::iterator it;
	if(set1.size() < set2.size()) {
		for(it=set1.begin(); it!=set1.end(); it++) {
			if(set2.find(*it) != set2.end()) {
				out_set.insert(*it);
			}
		}
	} else {
		for(it=set2.begin(); it!=set2.end(); it++) {
			if(set1.find(*it) != set1.end()) {
				out_set.insert(*it);
			}
		}
	}
	return out_set;
}


set<string> intersect(set<string> set1, set<string> set2) {
	set<string> out_set;
	
	if(set1.empty() || set2.empty())
		return out_set;
			
	set<string>::iterator it;
	if(set1.size() < set2.size()) {
		for(it=set1.begin(); it!=set1.end(); it++) {
			if(set2.find(*it) != set2.end()) {
				out_set.insert(*it);
			}
		}
	} else {
		for(it=set2.begin(); it!=set2.end(); it++) {
			if(set1.find(*it) != set1.end()) {
				out_set.insert(*it);
			}
		}
	}
	return out_set;
}

// returns if there is any change in dom set
// true - there were changes
// false - no changes 

// there seem to be a bug in dom - please check if the dom.clear() is correct
bool bb_t::intersect_doms_with_predecessors() {
	set<int> init_dom = dom;

	// for all predecessors
	dom.clear();
	if(predecessors.size() == 1)  {
		dom = predecessors[0]->get_dom();
	} else if(predecessors.size() > 1) {
		dom = predecessors[0]->get_dom();
		for(int i=1; i<predecessors.size(); i++) {
			set<int> t_dom  = dom;
			dom.clear();
			dom = intersect(t_dom, (predecessors[i]->get_dom()));
			t_dom.clear();
		}
	}
	// dom = intersect (doms of all predecessors)
	if(dom.find(get_id()) == dom.end()) { // not found
		dom.insert(get_id());
	}
	//cout << "initial_set: "; print_set(init_dom);
	//cout << "final_set : ";  print_set(dom);

	if(init_dom.size() == dom.size()) {
		set<int>::iterator it;
		for(it=init_dom.begin(); it!=init_dom.end(); it++) 
			if(dom.find(*it) == dom.end())
				return true;
	} else 
		return true;

	return false;
}

// returns if there is any change in dom set
// true - there were changes
// false - no changes 

bool bb_t::intersect_post_doms_with_successors() {
	bool change = false;
	set<int> temp_post_dom; 
	if(next_t != NULL && next_nt != NULL) {
		temp_post_dom = intersect(next_t->get_post_dom(), next_nt->get_post_dom());

	} else if(next_t != NULL && next_nt == NULL ) {

		set<int> t_post_dom = next_t->get_post_dom(); 
		set<int>::iterator it;
		for(it=post_dom.begin(); it!=post_dom.end(); it++)  {
			if(t_post_dom.find(*it) == t_post_dom.end()) { // not found
				if(get_id() != (*it)) {
					change = true;
					break;
				}
			}
		}
		if(!change) {
			return false;
		}

		temp_post_dom = next_t->get_post_dom(); 

	} else if(next_t == NULL && next_nt != NULL ) {

		set<int> t_post_dom = next_nt->get_post_dom(); 
		set<int>::iterator it;
		for(it=post_dom.begin(); it!=post_dom.end(); it++)  {
			if(t_post_dom.find(*it) == t_post_dom.end()) { // not found
				if(get_id() != (*it)) {
					change = true;
					break;
				}
			}
		}
		if(!change) {
			return false;
		}

		temp_post_dom = next_nt->get_post_dom();
	}

	// post_dom = intersect (post_doms of all successors)
	if(temp_post_dom.find(get_id()) == temp_post_dom.end()) { // not found
		temp_post_dom.insert(get_id());
	}
	//cout << "initial_set: "; print_set(init_post_dom);
	//cout << "final_set : ";  print_set(post_dom);

	if(temp_post_dom.size() == post_dom.size()) {
		set<int>::iterator it;
		for(it=temp_post_dom.begin(); it!=temp_post_dom.end(); it++)  {
			if(post_dom.find(*it) == post_dom.end()) {
				change = true;
				break;
			}
		}
	} else {
		change = true;
	}
	if(change) {
		post_dom.clear();
		post_dom = temp_post_dom;
	}
	//post_dom_changed = change;
	return change;
}

bool bb_t::is_in_dom(int str) {
		return (dom.find(str) != dom.end());
}
bool bb_t::is_in_post_dom(int str) {
		return (post_dom.find(str) != post_dom.end());
}

void bb_t::add_predecessor(bb_t * b) {
	if(b != NULL)  {
		//cout << "adding: " << b->get_id() << " as pred to " << id << endl;
		predecessors.push_back(b);
	}
}

int bb_t::num_predecessors() {
	return predecessors.size();
}

vector<bb_t*> bb_t::get_predecessors() {
	return predecessors;
}

void bb_t::get_adjacent_nodes(vector<bb_t*> & v) {
	vector<bb_t*> adjacent_nodes;
	if(next_t != NULL) {
		adjacent_nodes.push_back(next_t);
	}
	if(next_nt != NULL) {
		adjacent_nodes.push_back(next_nt);
	}

	// TODO
	// vector<bb_t*> pred = get_predecessors(); // should we include predecessors in the adjacent list?
	// adjacent_nodes.insert(adjacent_nodes.end(), pred.begin(), pred.end());
	v.swap(adjacent_nodes);
}

vector<bb_t*> bb_t::get_successors() {
	vector<bb_t*> successors;
	if(next_t != NULL) {
		successors.push_back(next_t);
	}
	if(next_nt != NULL) {
		successors.push_back(next_nt);
	}
	return successors;
}

void bb_t::remove_predecessor(bb_t * b) {
	if(b != NULL)  {
		//cout << "removing: " << b->get_id() << " as pred to " << id << endl;
		int i; 
		for(i=0; i<predecessors.size(); i++) {
			if(predecessors[i] == b) {
				break;
			}
		}
		if(i<predecessors.size()) 
			predecessors.erase(predecessors.begin()+i);
	}
}

void bb_t::print_set(set<int> s) {
	cout << s.size()<< ":{ ";
	set<int>::iterator it;
	for (it=s.begin(); it != s.end(); it++)  {
		cout << *it << ", ";
	}
	cout << "} " << endl;
}
void bb_t::print_predecessors() {
	cout << endl << "pred:" ;
	for(int i=0; i<predecessors.size(); i++) {
		cout << " " << predecessors[i]->get_id() ;
	}
	cout << endl ;
}

bool bb_t::is_back_edge( bb_t *n1, bb_t *n2) { // is n1->n2 a backedge?
	if(n1 != NULL && n2 != NULL) 
		if(n1->is_in_dom(n2->get_id())) 
			return true;
	return false;
}

cfg_t * bb_t::get_cfg(string pc) {
	parent_cfg->get_cfg(pc);
}

bb_t * bb_t::get_call_cfg_start() {
	// -2 because of delay slot
	return parent_cfg->get_cfg_start(instrns[instrns.size()-2]->get_call_target());
}

bool bb_t::is_last_inst_restore() {
	int bb_size = instrns.size();
	if (bb_size > 0) {
		return instrns[bb_size-1]->is_restore();
	}
	return false;
}

bool bb_t::is_first_inst_save() {
	int bb_size = instrns.size();
	if (bb_size > 0) {
		return instrns[0]->is_save();
	}
	return false;
}

bool bb_t::does_end_with_call() {
	return ends_with_call;
}

cfg_t * bb_t::get_target_cfg() {
	int bb_size = instrns.size();
	if(bb_size > 2) {
		if(instrns[bb_size-2]->is_call_site()) {
			return instrns[bb_size-2]->get_call_cfg();
		}
	}
	return NULL;
}


void bb_t::set_ends_with_call() {
	ends_with_call = true;
}

void bb_t::unset_ends_with_call() {
	ends_with_call = false;
}

void bb_t::set_last_instr_type(int t) {
	last_instr_type = t;
}

int bb_t::get_last_instr_type() {
	return last_instr_type;
}

int bb_t::find_last_instr_type() {
	int size = instrns.size();
	if(size > 1) {
		if(instrns[size-2]->is_branch_op()) 
			return BRANCH_TYPE; 
		if(instrns[size-2]->is_call_site()) 
			return CALL_TYPE; 
		if(instrns[size-2]->is_ret()) 
			return RET_TYPE; 
	}
	return -1;
}

void bb_t::print_bb() {
	cout << "Basic Block starting at " << first_instr_pc() << endl;
	cout << "size = " << instrns.size() << endl;
	for(int i=0; i<instrns.size(); i++) {
		instrns[i]->print_instruction();
	}
}

//input - rolling path1 until now
//		  path2 - path until now or b
//		  bb_t where we should stop adding bbs to path
//		  found - indicates whether bb already exists in path
bool bb_t::test_backedge(set<string> path1, set<string> path2, bb_t* b, bool found) {

	bool local_found = found;
	if(local_found) { // test if fist_instr_pc already exists in path2
		if(path2.find(first_instr_pc()) != path2.end()) 
			return false;
	}

	// safe to proceed
	if(path1.find(first_instr_pc()) == path1.end()) { // this bb is not explored
		path1.insert(first_instr_pc());

		if(!local_found) {
			path2.insert(first_instr_pc());
			if(get_id() == b->get_id()) { // b reached
				local_found = true;
			}
		} // if found is true - do not change path2

		bool result  = true;
		if(next_t != NULL) {
			result = next_t->test_backedge(path1, path2, b, local_found);
		}
		if(!result) 
			return false;;
		if(next_nt != NULL) {
			result = next_nt->test_backedge(path1, path2, b, local_found);
		}
		if(!result) 
			return false;

		if(!local_found &&  next_nt == NULL && next_t == NULL) 
			return false;
	}
	return true;
}

void bb_t::unset_post_dom_changed() {
	post_dom_changed = false;
}
void bb_t::set_post_dom_changed() {
	post_dom_changed = true;
}
bool bb_t::has_post_dom_changed() {
	return post_dom_changed;
}

int bb_t::frequency() {
	if(instrns.size() > 1)
		return instrns[0]->get_c_line_frequency();
	else 
		return -1;
}

string bb_t::reg_after_restore(string reg) {
	string ret_reg = "";
	if(reg.find('i') != string::npos) { // reg is an input register
		int pos = reg.find('i');
		ret_reg = reg;
		ret_reg.replace(pos,1,"o");

	} else if(reg.find('g') != string::npos || reg.find('d') != string::npos || reg.find('f') != string::npos) { 
		ret_reg = reg;
	}
	return ret_reg;
}

string bb_t::reg_after_save(string reg) {
	string ret_reg = "";
	if(reg.find('o') != string::npos) { // reg is an input register
		int pos = reg.find('o');
		ret_reg = reg;
		ret_reg.replace(pos,1,"i");

	} else if(reg.find('g') != string::npos || reg.find('d') != string::npos || reg.find('f') != string::npos) { 
		ret_reg = reg;
	}
	return ret_reg;
}

void bb_t::mark_for_extra_counting(int i, string reg, string pc) {
	instrns[i-1]->mark_for_extra_counting(reg, 1, pc);
}

void bb_t::set_dfs_num(int n) {
	dfs_num = n;
}

int bb_t::get_dfs_num() {
	return dfs_num;
}

void bb_t::mark_visited() {
	visited = true;
}
void bb_t::clear_visited() {
	visited = false;
}
bool bb_t::is_visited() {
	return visited;
}
void bb_t::set_reachable() {
	reachable = true;
}
bool bb_t::is_reachable() {
	return reachable;
}


void bb_t::set_hi(int h) {
	hi = h;
}

int bb_t::get_hi() {
	return hi;
}

void bb_t::add_exit_point(string str) {
	if (str.compare("end") != 0) {
		list<string>::iterator it; 
		for(it=exit_points.begin(); it!=exit_points.end(); it++) {
			if (str.compare((*it)) == 0) 
				return; 
		}
		exit_points.push_back(str);

	}
}

void bb_t::print_exit_points(std::ofstream & ofs) {

	string eps = ""; // exit points
	list<string>::iterator it; 
	for(it=exit_points.begin(); it!=exit_points.end(); it++) {
		string lep = (*it);
		eps += (*it);
		eps += "-";
	}

	int instr_size = instrns.size();
	for(int i=0; i<instr_size; i++) {
		//ofs << parent_cfg->get_name() << ":" << instrns[i]->get_program_counter() << ":" << id << ":" << eps << endl;
		ofs << instrns[i]->get_program_counter() << ":" << id << ":" << eps << endl;
	}
}

/***********************************************
*********** Functions of edge_t ****************
***********************************************/

edge_t::edge_t() {
	pred = NULL;
	succ = NULL;
	backedge = true;
	name = "";

	eq_class = -1;
	visited = false;
	recent_size = -1;
	recent_class = -1;
	last_class = false;

	capping = false;

	sese_entry = NULL;
	sese_exit = NULL;
}

edge_t::edge_t(bb_t* p, bb_t* s) {
	pred = p;
	succ = s;
	backedge = true;
	if(p!= NULL) {
		name = p->first_instr_pc() + '-' + s->first_instr_pc();
	} else {
		name = '-' + s->first_instr_pc();
	}

	eq_class = -1;
	visited = false;
	recent_size = -1;
	recent_class = -1;
	last_class = false;

	capping = false;

	sese_entry = NULL;
	sese_exit = NULL;
}

void edge_t::set_name(string n) {
	name = n;
}

string edge_t::get_name() {
	return name;
}

void edge_t::set_pred(bb_t * p) {
	pred = p;
}

void edge_t::set_succ(bb_t * s) {
	succ = s;
}

bb_t * edge_t::get_pred() {
	return pred;
}

bb_t * edge_t::get_succ() {
	return succ;
}

void edge_t::set_backedge() {
	backedge = true;
}

void edge_t::set_eq_class(int e) {
	eq_class = e;
}

int edge_t::get_eq_class() {
	return eq_class;
}

void edge_t::clear_backedge() {
	backedge = false;
}

bool edge_t::is_backedge() {
	return backedge;
}

bool edge_t::undefined_eq_class() {
	return (eq_class == -1);
}

void edge_t::set_recent_size(int s) {
	recent_size = s;
}

int edge_t::get_recent_size() {
	return recent_size;
}

void edge_t::set_recent_class(int c) {
	recent_class = c;
}

int edge_t::get_recent_class() {
	return recent_class;
}

void edge_t::set_last_class() {
	// cout << " last_edge:" ;
	// print_edge();
	// cout << endl;
	last_class = true;
}

bool edge_t::is_last_class() {
	return last_class;
}

void edge_t::print_edge() {
	if(pred==NULL) {
		cout << '-' << succ->first_instr_pc();
	} else {
		cout << pred->first_instr_pc() << '-' << succ->first_instr_pc();
	}
}

bool edge_t::check_name(string en) {
	string name;
	if(pred != NULL) {
		name += pred->first_instr_pc();
	}
	name += "-";
	if(succ != NULL) {
		name += succ->first_instr_pc();
	}
	if(name.compare(en) == 0) {
		return true;
	}
	return false;
}

void edge_t::set_dfs_num(int n) {
	dfs_num = n;
}

int edge_t::get_dfs_num() {
	return dfs_num;
}

void edge_t::set_capping_edge() {
	capping = true;
}

bool edge_t::is_capping_edge() {
	return capping;
}

void edge_t::mark_visited() {
	visited = true;
}
void edge_t::clear_visited() {
	visited = false;
}
bool edge_t::is_visited() {
	return visited;
}

void edge_t::set_sese_entry(sese_region_t * s) {
	sese_entry = s;
}

void edge_t::set_sese_exit(sese_region_t * s) {
	sese_exit = s;
}

sese_region_t * edge_t::get_sese_entry() {
	return sese_entry;
}

sese_region_t * edge_t::get_sese_exit() {
	return sese_exit;
}


/***********************************************
******** Functions of sese_region_t ************
***********************************************/

sese_region_t::sese_region_t() {
	entry = NULL;
	exit = NULL;
	parent = NULL;
	children.clear();
	num_instructions = 0;
	id = sese_count++;
}

sese_region_t::sese_region_t(edge_t * e) {
	entry = e;
	exit = NULL;
	parent = NULL;
	children.clear();
	num_instructions = 0;
	id = sese_count++;
}

void sese_region_t::set_exit(edge_t * e) {
	exit = e;
}

edge_t * sese_region_t::get_entry() {
	return entry;
}

edge_t * sese_region_t::get_exit() {
	return exit;
}
void sese_region_t::print_region() {
	cout << "[ ";
	if(entry != NULL) {
		entry->print_edge();
	}
	cout << " --- ";
	if(exit != NULL) {
		exit->print_edge();
	}
	cout << " ]";
}

void sese_region_t::set_parent(sese_region_t * p) {
	parent = p;
}

sese_region_t * sese_region_t::get_parent() {
	return parent;
}

void sese_region_t::set_child(sese_region_t* c) {
	children.push_back(c);
}

list<sese_region_t*> sese_region_t::get_children() {
	return children;
}
bool sese_region_t::has_children() {
	return (children.size() != 0);
}

void sese_region_t::set_num_instructions(int a) {
	num_instructions = a;
}

int sese_region_t::get_num_instructions() {
	return num_instructions;
}


