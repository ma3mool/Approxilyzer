#include "cfg.h"
#include <string>
#include <stack>
#include <ctime>
#include <cassert>

#define DEBUG 0
#define DEBUG_SESE 0

#define min(x,y) (x < y ? x : y)

using namespace std;
using namespace __gnu_cxx;

// bb - basic block

cfg_t::cfg_t(program_t * program, std::string cn) {
	count_id = 0;
	start = NULL;
	end = new bb_t(count_id++, this);
	pc_bb_map["end"] = end;
	start_index = 0;
	end_index = 0;
	cfg_name = cn;

	prog = program;
	eq_class_count = 0;
	top_region = NULL;
	callers.clear();
}

cfg_t::~cfg_t() {
	map<string, bb_t*>::iterator itb;
	for (itb=pc_bb_map.begin(); itb != pc_bb_map.end(); itb++)  {
		delete (*itb).second;
	}
	pc_bb_map.clear();

	map<string, edge_t*>::iterator ite;
	for (ite=pcs_edge_map_st.begin(); ite != pcs_edge_map_st.end(); ite++)  {
		delete (*ite).second;
	}
	pcs_edge_map_st.clear();

	callers.clear();
}

// input - entire program, sequence of instructions from binary
// s - starting point for this function in this sequence
void cfg_t::create_cfg(std::vector <instruction_t> * prg, int s) {
	start_index = s;

	// create new bb and mark it as start, count_id is used to count the number of bbs created
	bb_t  * curr_bb = new bb_t(count_id++, this);
	start = curr_bb;
	for(int i=start_index; i<prg->size(); i++) {
		// end current bb if you see a branch, call, or a ret
		if(prg->at(i).is_branch_op()) {
			if(curr_bb->is_empty()) {
				// first instr in this bb, so set the pc_bb_map
				pc_bb_map[prg->at(i).get_program_counter()] = curr_bb;
			}

			curr_bb->add_inst(&(prg->at(i)));
			prg->at(i).set_bb(curr_bb);
			// add delay slot instruction to curr_bb
			curr_bb->add_inst(&(prg->at(i+1)));
			prg->at(i+1).set_bb(curr_bb);

			//new bb for next bb
			bb_t * new_curr_bb = new bb_t(count_id++, this);

			// create edges to next 1 or 2 bbs
			curr_bb->next_t = NULL;
			if(prg->at(i).is_branch_always()) {
				if(pc_bb_map.find(prg->at(i).get_branch_target()) != pc_bb_map.end()) {
					curr_bb->next_t = pc_bb_map[prg->at(i).get_branch_target()];
					// set parents to the next bb
					curr_bb->next_t->add_predecessor(curr_bb);
				}
				curr_bb->next_nt = NULL;
			} else if (prg->at(i).is_branch_conditional()) {
				if(pc_bb_map.find(prg->at(i).get_branch_target()) != pc_bb_map.end()) {
					curr_bb->next_t = pc_bb_map[prg->at(i).get_branch_target()];
					// set parents to the next bb
					curr_bb->next_t->add_predecessor(curr_bb);
				}
				curr_bb->next_nt = new_curr_bb;
				// set parents to the next bb
				curr_bb->next_nt->add_predecessor(curr_bb);
			}

			curr_bb->set_last_instr_type(BRANCH_TYPE); 
			curr_bb = new_curr_bb;
			// set parent for curr_bb

			// skip branch delay slot instruction becuase its been accounted for
			i++;

		} else if(prg->at(i).is_jump()) {
			if(curr_bb->is_empty()) {
				// first instr in this bb, so set the pc_bb_map
				pc_bb_map[prg->at(i).get_program_counter()] = curr_bb;
			}

			curr_bb->add_inst(&(prg->at(i)));
			prg->at(i).set_bb(curr_bb);

			// add delay slot instruction to curr_bb
			curr_bb->add_inst(&(prg->at(i+1)));
			prg->at(i+1).set_bb(curr_bb);

			//new bb for next bb
			bb_t * new_curr_bb = new bb_t(count_id++, this);

			// create edges to next 1 or 2 bbs
			curr_bb->next_t = NULL;
			curr_bb->next_nt = new_curr_bb;
			// set parents to the next bb
			curr_bb->next_nt->add_predecessor(curr_bb);

			curr_bb->set_last_instr_type(JUMP_TYPE); 
			curr_bb = new_curr_bb;
			// set parent for curr_bb

			// skip branch delay slot instruction becuase its been accounted for
			i++;

		} else if (prg->at(i).is_call_site()) {
			if(curr_bb->is_empty()) {
				// first instr in this bb, so set the pc_bb_map
				pc_bb_map[prg->at(i).get_program_counter()] = curr_bb;
			}
			curr_bb->add_inst(&(prg->at(i)));
			prg->at(i).set_bb(curr_bb);
			// add delay slot instruction to curr_bb
			curr_bb->add_inst(&(prg->at(i+1)));
			prg->at(i+1).set_bb(curr_bb);

			curr_bb->set_ends_with_call();
			//cout << "ends_with_call: " << curr_bb->first_instr_pc() <<  endl;

			//new bb for next bb
			bb_t * new_curr_bb = new bb_t(count_id++, this);

			// nt is null uf ends_with_call is true
			curr_bb->next_t = new_curr_bb;
			curr_bb->next_nt = NULL;
			// set parents to the next bb
			new_curr_bb->add_predecessor(curr_bb);

			curr_bb->set_last_instr_type(CALL_TYPE); 
			curr_bb = new_curr_bb;

			// skip branch delay slot instruction becuase its been accounted for
			i++;

		} else if (prg->at(i).is_ret() ) {
			if(curr_bb->is_empty()) {
				// first instr in this bb, so set the pc_bb_map
				pc_bb_map[prg->at(i).get_program_counter()] = curr_bb;
			}

			curr_bb->add_inst(&(prg->at(i)));
			prg->at(i).set_bb(curr_bb);

			// add delay slot instruction to curr_bb
			curr_bb->add_inst(&(prg->at(i+1)));
			prg->at(i+1).set_bb(curr_bb);

			//end of CFG
			curr_bb->next_t = NULL;
			curr_bb->next_nt = NULL;

			curr_bb->set_last_instr_type(RET_TYPE); 

			// skip branch delay slot instruction becuase its been accounted for
			i++;

			// end of this basic block
			bb_t * curr_bb = new bb_t(count_id++, this);

		} else if(prg->at(i).is_func_name()) {
			//end of CFG
			curr_bb->next_t = NULL;
			curr_bb->next_nt = NULL;
			end_index = i-1;
			break;
	
		} else {
			bool test = false;
			if(curr_bb->is_empty()) {
				// first instr in this bb, so set the pc_bb_map
				pc_bb_map[prg->at(i).get_program_counter()] = curr_bb;
				test = true;
			}
			curr_bb->add_inst((&prg->at(i)));
			prg->at(i).set_bb(curr_bb);
			if(DEBUG)
				if(test) 
					cout << prg->at(i).get_program_counter() << " " << curr_bb->get_id() << endl;
		}
	}

	// TODO: deal with the "$" node

	clean_bbs(prg);
	// this function should be called explicitly after creat_cfg 
	make_missing_edges(prg);	// this function has to be called right at the end of create_cfg 
								// It adds edges that are missing from the CFG because
								// we might have modified the basic blocks and hence some
								// edges might be missing.
}

void cfg_t::make_missing_edges(std::vector <instruction_t> * prg) {
	for(int i=start_index; i<=end_index; i++) {
		if(prg->at(i).is_branch_op()) {
			string target = "";
			target = prg->at(i).get_branch_target();
			bool new_flag = false;
			// if(target.compare("0x100003870") == 0) {
			// 	new_flag = true;
			// }

			bb_t * curr_bb = prg->at(i).get_bb();
			if(curr_bb->next_t == NULL) {
				if(pc_bb_map.find(target) != pc_bb_map.end()) {
					curr_bb->next_t = pc_bb_map[target];
					curr_bb->next_t->add_predecessor(curr_bb);
					// if(DEBUG)
					if(new_flag)
						cout << " fix:" << target << " " << curr_bb->next_t->get_id() << endl;
				} else {
					// if(DEBUG)
					if(new_flag)
						cout << prg->at(i).get_program_counter() << ":attmpt to fix: " << target << endl;

					find_and_split(prg, target);
					if(pc_bb_map.find(target) != pc_bb_map.end()) {
						curr_bb->next_t = pc_bb_map[target];
						curr_bb->next_t->add_predecessor(curr_bb);
						// if(DEBUG)
						if(new_flag)
							cout << "after attemp to fix:" << target << " " << curr_bb->next_t->get_id() << endl;
					}
				}
			}
			// curr_bb->next_nt was correctly set earlier 
		} else if (prg->at(i).is_ret()) {
			// find the caller and link curr_bb->next_t. There can be many callers; what to do in this case?
			// nothing for now - cfg is created per function
		}
	}

	//cout << "count id now : " << count_id << endl;
}

void cfg_t::fill_holes(string prg_name, string app_name, std::vector <instruction_t> * prg, map<string,string> &arcs) {

	for(int i=start_index; i<=end_index; i++) {
		// if the pc is from_pc and its a branch instruction
		// it may be a call instruction, in which case we skip it 
		if((prg->at(i).is_branch_op() || prg->at(i).is_jump()) ) {

			string from_pc = prg->at(i).get_program_counter();
			map<string,string>::iterator it = arcs.find(from_pc);
			if ( it != arcs.end() ) {
				string to_pc = (*it).second;

				string target = "";
				target = prg->at(i).get_branch_target();
				if(target.compare("") == 0) { // only for the cases where the target is not available
		
					bb_t * curr_bb = prg->at(i).get_bb(); // current basic block
					if(curr_bb->next_t == NULL) {
						if(pc_bb_map.find(to_pc) != pc_bb_map.end()) {
							curr_bb->next_t = pc_bb_map[to_pc];
							curr_bb->next_t->add_predecessor(curr_bb);
							if(DEBUG)
								cout << " fix:" << to_pc << " " << curr_bb->next_t->get_id() << endl;
						} else {
		
							if(DEBUG)
								cout << prg->at(i).get_program_counter() << ":attmpt to fix: " << to_pc << endl;
		
							find_and_split(prg, to_pc);
							if(pc_bb_map.find(target) != pc_bb_map.end()) {
								curr_bb->next_t = pc_bb_map[target];
								curr_bb->next_t->add_predecessor(curr_bb);
								if(DEBUG)
									cout << "after attemp to fix:" << target << " " << curr_bb->next_t->get_id() << endl;
							}
						}
					}
				}
				// if pc is found then just stop searching for this pc
				break;
			}
			
		}
	}
}

// find instruction with target as pc and split bb at this point split bb0 in to bb0 and bb2
void cfg_t::find_and_split(std::vector <instruction_t> * prg, std::string target) {

	for(int j=start_index; j<=end_index; j++) {
		if(target.compare(prg->at(j).get_program_counter()) == 0) { // found the instruction

			split_bb(prg->at(j).get_bb(), prg->at(j).get_program_counter());
			break;
		}
	}
}

void cfg_t::split_bb(bb_t *curr_bb, std::string pc) {

	if(pc.compare("") == 0) {
		cout << "Warning: pc is \"\": in bb" << curr_bb->first_instr_pc() << endl;
		return;
	}
	if(curr_bb->size() == 1) {
		cout << "Size of this block is 1!!! Why do you want to split it??" << endl;
		return;
	}


	// split curr_bb in to bb0 and bb2: predecessors->curr_bb->successors to predecessors->bb0->bb2->successors
	bb_t * bb0 = curr_bb;
	bb_t * bb2 = new bb_t(count_id++, this);
	bb0->move_instructions(pc, bb2); // move instructions from bb0 to bb2 starting from j 
	if(DEBUG)
		cout << " fix:" << pc << " " << bb2->get_id() << endl;

	{
		/* I use to have this statement here but I no longer think that it is correct to have it here*/ 
		// order is important becuase of a special case
		// if curr_bb is bb0 then next_t of curr_bb should be updated before copyting it to bb2
		// curr_bb->next_t = bb2;
	}

	bb2->next_t = bb0->next_t;
	bb2->next_nt = bb0->next_nt;

	bb0->next_t = bb2;
	bb0->next_nt = NULL;

	// set parents to the next bb
	bb2->add_predecessor(bb0);
	// bb2->add_predecessor(curr_bb);

	// remove bb0 from bb2->next_t and bb2_next_nt's predecessors list
	if(bb2->next_t != NULL) {
		bb2->next_t->remove_predecessor(bb0);
		bb2->next_t->add_predecessor(bb2);
	}
	if(bb2->next_nt != NULL) {
		bb2->next_nt->remove_predecessor(bb0);
		bb2->next_nt->add_predecessor(bb2);
	}
	// if(curr_bb == bb0) {
	//	bb0->remove_predecessor(bb0);
	// }

	if(bb0->does_end_with_call()) {
		bb2->set_ends_with_call();
		bb0->unset_ends_with_call();
	}

	// bb2's last instruction type is same as bb0's last instruction type
	bb2->set_last_instr_type(bb0->get_last_instr_type());

	// set last instruction type for bb0
	if(bb0->find_last_instr_type() != -1) {
		bb0->set_last_instr_type(bb0->find_last_instr_type());
	} else {
		bb0->set_last_instr_type(NONE_TYPE);
	}

	if(bb0->get_last_instr_type() == RET_TYPE) {
		bb0->next_t = NULL;
		bb0->next_nt = NULL;
		bb2->remove_predecessor(bb0);
	}
	//bb0->print_bb();

	// update map
	pc_bb_map[pc] = bb2;
	//cout << " fix:" << pc << " " << bb2->first_instr_pc() ;
}

// break basic blocks at the end of ret/restore instruction
void cfg_t::clean_bbs(std::vector <instruction_t> * prg) {
	for(int i=start_index; i<=end_index; i++) {
		if(prg->at(i).is_ret()) {
			bb_t * curr_bb = prg->at(i).get_bb();
			int next_start = i+2;
			if(next_start <= end_index) {
				bb_t * next_instr_bb = prg->at(next_start).get_bb();
				if (next_instr_bb == curr_bb) { 
					// split bb at this point split bb0 in to bb0 and bb2
					// this is a special split - there will be no parents to the parents of the new block (bb2)
					int j = next_start;
					bb_t * bb0 = prg->at(j).get_bb(); // this should be curr_bb
					bb_t * bb2 = new bb_t(count_id++, this);
					bb0->move_instructions(prg->at(j).get_program_counter(), bb2); // starting from j move instructions from bb0 to bb2
					if(DEBUG)
						cout << " fix:" << prg->at(j).get_program_counter() << " " << bb2->get_id() << endl;

					bb2->next_t = bb0->next_t;
					bb2->next_nt = bb0->next_nt;

					bb0->next_t = NULL;
					bb0->next_nt = NULL;

					// set parents of the next bb  - there are no parents for bb2 

					// remove bb0 from bb2->next_t and bb2_next_nt's predecessors list
					if(bb2->next_t != NULL) {
						bb2->next_t->remove_predecessor(bb0);
						bb2->next_t->add_predecessor(bb2);
					}
					if(bb2->next_nt != NULL) {
						bb2->next_nt->remove_predecessor(bb0);
						bb2->next_nt->add_predecessor(bb2);
					}

					if(bb0->does_end_with_call()) {
						bb2->set_ends_with_call();
						bb0->unset_ends_with_call();
					}

					// bb2's last instruction type is same as bb0's last instruction type
					bb2->set_last_instr_type(bb0->get_last_instr_type());

					// set last instruction type for bb0
					if(bb0->find_last_instr_type() != -1) {
						bb0->set_last_instr_type(bb0->find_last_instr_type());
					} else {
						bb0->set_last_instr_type(NONE_TYPE);
					}

					//bb0->print_bb();

					// update map
					pc_bb_map[prg->at(j).get_program_counter()] = bb2;
					if(prg->at(i).get_program_counter().compare("0x1000dccd0") == 0) {
						cout << " fix:" << prg->at(j).get_program_counter() << " " << bb2->first_instr_pc() ;
					}
				}
			}
		}
	}
}
void cfg_t::mark_nops() {
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t * curr_bb = (*it).second;
		if(curr_bb != NULL) {
			curr_bb->mark_nops();
		}
	}
}

void cfg_t::prepare_for_sese(std::vector <instruction_t> * prg) {
	vector<bb_t*> bb_list;
	map<string, bb_t*>::iterator it;
	// for every basic block in this cfg
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t * curr_bb = (*it).second;
		if(curr_bb != NULL) {
			bb_list.push_back(curr_bb);
		}
	}

	int bb_list_size = bb_list.size();
	for(int i=0; i<bb_list_size; i++) {

		bb_t * curr_bb = bb_list[i];

		// if this bb has >1 predecessors then split it in to two
		// if it has >1 successors then split it in to two
		// if it has >1 predecessors and successors then split it in to three
		// No not modify the pc_bb_map until the end of this loop
		// All this has to be done if there are sufficient instructions to split the new bbs should have at least one non-noop instruction

		// find the length of bb (num instructions in bb)
		// if this bb does not have sufficient insturctions to split then skip it and move to the next block
		if(curr_bb->size()<=1) {
				continue;
		} else if(curr_bb->size()==2) {
			if(curr_bb->get_last_instr_type() != NONE_TYPE) // it is either BRANCH_TYPE, CALL_TYPE, or RET_TYPE
				continue;
		}

		bool gt_1_successors = (curr_bb->next_t != NULL && curr_bb->next_nt != NULL); // ture: >1, false: <=1

		// if (>1 predecessors or >1 successors)
		if(curr_bb->num_predecessors() > 1 || gt_1_successors) { // extra unnecessary check - you can remove this

			if(curr_bb->num_predecessors() > 1 && gt_1_successors) {	// if (>1 predecessors and >1 successors)
				// need to split in to three
				// at least 3 instructions before spliting

				// first split into two s.t. num_successors of curr_bb is 1, then split curr_bb into two again
				if(curr_bb->size()>=3) {
					split_bb(curr_bb, curr_bb->last_1_instr_pc());
					split_bb(curr_bb, curr_bb->second_instr_pc());
				}

			} else if(curr_bb->num_predecessors() > 1) {	// if (>1 predecessors)

				// need to split in to two 
				// at least 2 instructions before spliting
				if(curr_bb->size()>=3 || (curr_bb->size()==2 && curr_bb->get_last_instr_type()==NONE_TYPE)) {
					split_bb(curr_bb, curr_bb->second_instr_pc()); // updates pc_bb_map
				}
			} else if(gt_1_successors) {	// if (>1 successors)

				if(curr_bb->size()>=3) {
					split_bb(curr_bb, curr_bb->last_1_instr_pc());
				}
				// need to split in to two 
			}
		} // else do nothing - this is already a sese
	} // end of for each bb in cfg

}

void cfg_t::print_cfg() {
	cout << "printing cfg" << endl;
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t * curr_bb = (*it).second;
		if(curr_bb != NULL) {
			cout << curr_bb->get_id() << ":" << (*it).first << ":" << curr_bb->size() << "(" << curr_bb->does_end_with_call() << ") -> ";
			if(curr_bb->next_t != NULL) 
				cout << "[ " << curr_bb->next_t->get_id() << ":" << curr_bb->next_t->first_instr_pc() << ":" << curr_bb->next_t->size() << "(" << curr_bb->next_t->does_end_with_call() << ") ]";
			if(curr_bb->next_nt != NULL) 
				cout << "[ " << curr_bb->next_nt->get_id() << ":" << curr_bb->next_nt->first_instr_pc() << ":" << curr_bb->next_nt->size() <<  "(" << curr_bb->next_nt->does_end_with_call() << ") ]";

			curr_bb->print_predecessors();
		}
	}
	cout << endl;
}

void cfg_t::print_pst(sese_region_t * r, ofstream & ofs, int & c) {
	// if r has no children and we dont want to show small regions then dont create a sub graph
	bool show_small_sese_regions = true; // false; 

	if(r != NULL) {
		if(!r->has_children()) { // no children
			if(r!=top_region) {
				if(show_small_sese_regions) {
					// start a new subgraph 
					c++;
					string col = get_random_color();
					ofs << endl << " subgraph cluster_" << c << " { \n node [style=filled, color=\"#" << col << "\" ]; \n color=\"#" << col << "\"; \n  penwidth=5;" << endl;

					print_region_subgraph(r, ofs, c, true);

					ofs << " } " << endl;
				}
			}
		} else { // has children
			// start a new subgraph 
			c++;
			string col = get_random_color();
			ofs << endl << " subgraph cluster_" << c << " { \n node [style=filled, color=\"#" << col << "\" ]; \n color=\"#" << col << "\"; \n  penwidth=5;" << endl;

			list<sese_region_t*> c_list = r->get_children();
			list<sese_region_t*>::iterator it;
			for(it=c_list.begin(); it!=c_list.end(); it++) {
				sese_region_t * child = (*it);
				if(child != NULL) {
					print_pst(child, ofs, c);
				}
			}

			print_region_subgraph(r, ofs, c, true);

			ofs << " } " << endl;
		}

	}
}


// Use this function if you want to plot the CFG using "dot". See http://www.graphviz.org/pdf/dotguide.pdf
void cfg_t::print_cfg(std::ofstream & ofs_sese, std::ofstream & ofs_exit_points) {
	ofs_sese << "strict digraph G{ \n";

	int c = 0;
	// // print sese regions
 	// list<sese_region_t*>::iterator rit;
	// for(rit=sese_regions.begin(); rit!=sese_regions.end(); rit++) {
	// 	sese_region_t * r = (*rit);
	// 	print_region_subgraph(r, ofs_sese, c++); 
	// }

	// print pst
	print_pst(top_region, ofs_sese, c);
	cout << "Num nodes in PST: " << c << endl;


	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t * curr_bb = (*it).second;

		// print subsequent sese exit points for every instruction
	 	curr_bb->print_exit_points(ofs_exit_points);

		if(curr_bb != NULL) {
			if(curr_bb->is_reachable()) {
				if(curr_bb->next_t != NULL) {
					ofs_sese << "\"PC" << (*it).first << "-S" << curr_bb->size() << "(" << curr_bb->does_end_with_call() << ")\" -> ";
					ofs_sese << "\"PC" << curr_bb->next_t->first_instr_pc() << "-S" << curr_bb->next_t->size() << "(" << curr_bb->next_t->does_end_with_call() << ")\";\n";
				}
				if(curr_bb->next_nt != NULL) {
					ofs_sese << "\"PC" << (*it).first << "-S" << curr_bb->size() << "(" << curr_bb->does_end_with_call() << ")\" -> ";
					ofs_sese << "\"PC" << curr_bb->next_nt->first_instr_pc() << "-S" << curr_bb->next_nt->size() <<  "(" << curr_bb->next_nt->does_end_with_call() << ")\";\n";
				}
			} else {
				// cout << "Unreachable: " << curr_bb->first_instr_pc() << endl;
			}

		}
	}
	ofs_sese << "}\n";
}

// connect terminal nodes to the start node - should I create a single end node and connect it to the start?
void cfg_t::traverse_add_endnode(bb_t * n){
	vector<bb_t*> adj_nodes;
	n->get_adjacent_nodes(adj_nodes);  // in CFG
	for(int i=0; i<adj_nodes.size(); i++) {
		if(!adj_nodes[i]->is_visited()) {
			adj_nodes[i]->mark_visited();
			adj_nodes[i]->set_reachable();
			traverse_add_endnode(adj_nodes[i]);
		}
	}
	// no adjacent nodes
	if(adj_nodes.size() == 0) {
		edge_t * e = new edge_t(n,end);
		string n_name = n->first_instr_pc();
		string e_name = end->first_instr_pc();
		start_end_edge_map[n_name][e_name] = e;
		start_end_edge_map[e_name][n_name] = e;
		pcs_edge_map[(n_name+'-'+e_name)] = e;
		n->next_t = end;

		pc_bb_map[n_name] = n; // Fix to add 'n' to the pc_bb_map. Since, n was
								// never added to the pc_bb_map before because
								// no instructions were added. 
	}
}

void cfg_t::create_edges() {
	// for every bb add edges bb->successors to the list
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		string curr_bb_name = (*it).first;
		bb_t * curr_bb = (*it).second;
		if(curr_bb->next_t != NULL) {
			string next_bb_name = curr_bb->next_t->first_instr_pc();

			edge_t * e = new edge_t(curr_bb, curr_bb->next_t);
			string edge_name = curr_bb_name + '-' + next_bb_name;
			e->set_name(edge_name);
			pcs_edge_map[edge_name] = e;

			start_end_edge_map[curr_bb_name][next_bb_name] = e;
			start_end_edge_map[next_bb_name][curr_bb_name] = e;// adding the reverse edge to make an undirected graph

			// check if curr_bb->next_t is in the pc_bb_map, if not add it
			if (pc_bb_map.find(next_bb_name) == pc_bb_map.end()) {
					pc_bb_map[next_bb_name] = curr_bb->next_t;
			}
			if(DEBUG) {	
				if(curr_bb_name.compare("117547") == 0 || curr_bb->next_t->first_instr_pc().compare("117547") == 0) {
					cout << "t:" << (curr_bb_name+'-'+next_bb_name) << endl;
				}
			}
			 
		}
		if(curr_bb->next_nt != NULL) {
			string next_bb_name = curr_bb->next_nt->first_instr_pc();

			edge_t * e = new edge_t(curr_bb, curr_bb->next_nt);
			string edge_name = curr_bb_name + '-' + next_bb_name;
			e->set_name(edge_name);
			pcs_edge_map[edge_name] = e;

			start_end_edge_map[curr_bb_name][next_bb_name] = e;
			start_end_edge_map[next_bb_name][curr_bb_name] = e;// adding the reverse edge to make an undirected graph

			// check if curr_bb->next_t is in the pc_bb_map, if not add it
			if (pc_bb_map.find(next_bb_name) == pc_bb_map.end()) {
					pc_bb_map[next_bb_name] = curr_bb->next_nt;
			}

			if(DEBUG) {	
				if(curr_bb_name.compare("117547") == 0 || curr_bb->next_nt->first_instr_pc().compare("117547") == 0) {
					cout << "nt:" << (curr_bb_name+'-'+curr_bb->next_nt->first_instr_pc()) << endl;
				}
			}

		}
	}
	start->set_reachable();
	end->set_reachable();
	traverse_add_endnode(start);
	// connect end-start
	edge_t * e = new edge_t(end,start);
	string s_name = start->first_instr_pc();
	string e_name = end->first_instr_pc();
	start_end_edge_map[s_name][e_name] = e;
	start_end_edge_map[e_name][s_name] = e;

	if(DEBUG) {	
		if(s_name.compare("117547") == 0 || e_name.compare("117547") == 0) {
			cout << "e:" << (s_name+'-'+e_name) << endl;
		}
	}


	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t * b = (*it).second;
		b->clear_visited();
	}
}

void cfg_t::live_reg_analysis() {
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t * curr_bb = (*it).second;
		if(curr_bb != NULL) {
			curr_bb->update_out();
		}
	}
}

void cfg_t::prune_defs() {
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t * curr_bb = (*it).second;
		if(curr_bb != NULL) {
			curr_bb->prune_defs();
            // Prunes both defs and uses TODO split into seperate functions for clarity (or rename func)
            curr_bb->prune_duplicate_uses();

		}
	}
}

int cfg_t::get_num_bbs() {
		return pc_bb_map.size();
}
int cfg_t::get_count_id() {
		return count_id;
}
void cfg_t::find_invariant_detectors() {
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) { // for all bbs
		bb_t * curr_bb = (*it).second;
		curr_bb->find_invariant_detectors();
	}
}
void cfg_t::mark_store_instructions() {
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) { // for all bbs
		bb_t * curr_bb = (*it).second;
		curr_bb->mark_store_instructions();
	}
}
void cfg_t::mark_control_instructions() {
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) { // for all bbs
		bb_t * curr_bb = (*it).second;
		curr_bb->mark_control_instructions();
	}
}
void cfg_t::mark_other_instructions() {
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) { // for all bbs
		bb_t * curr_bb = (*it).second;
		curr_bb->mark_other_instructions();
	}
}
void cfg_t::mark_unlive_registers() {
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t * curr_bb = (*it).second;
		if(curr_bb != NULL) {
			curr_bb->mark_unlive_registers();
		}
	}
}

void cfg_t::create_dominators() {

	// create N
	set<int> N;

	// add all bbs in the cfg to N
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		// N.insert((*it).first);
		N.insert((*it).second->get_id());
	}

	// initialize dom sets for all the bbs
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t* curr_bb =  (*it).second;
		if(curr_bb == start) {
			curr_bb->set_dom((*it).second->get_id());
		} else {
			curr_bb->set_dom(N);
		}
	}

	bool change = true;
	while(change) {
		change = false;
		// for all basic blocks
		for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
			change |= ((*it).second)->intersect_doms_with_predecessors();
		}

		if(DEBUG) {
			map<string, bb_t*>::iterator it1;
			for (it1=pc_bb_map.begin(); it1!=pc_bb_map.end(); it1++) {
				bb_t * curr_bb = (*it1).second;
				cout << "Dom: " << curr_bb->get_id() << ":" ; 
				curr_bb->print_set(curr_bb->get_dom());
			}
			cout << endl;
		}
	}

	if(DEBUG) {
		// print backedges
		// n->d
		for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
			bb_t * curr_bb = (*it).second;
			//cout << "Dom: " << curr_bb->get_id() << ":" ; 
			//curr_bb->print_set(curr_bb->get_dom());

		//	if(curr_bb->next_t != NULL) {
		//		if(curr_bb->is_in_dom(curr_bb->next_t->first_instr_pc())) {
		//			cout << "back edge: " << curr_bb->first_instr_pc() << "-" << curr_bb->next_t->first_instr_pc() << endl;
		//		}
		//	}
		//	if(curr_bb->next_nt != NULL) {
		//		if(curr_bb->is_in_dom(curr_bb->next_nt->first_instr_pc())) {
		//			cout << "back edge: " << curr_bb->first_instr_pc() << "-" << curr_bb->next_nt->first_instr_pc() << endl;
		//		}
		//	}
		}
	}
}

void cfg_t::create_post_dominators() {
	// create N
	set<int> N;
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		N.insert((*it).second->get_id());
	}

	// initialize post-dom sets for all the bbs
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t* curr_bb =  (*it).second;
		if(curr_bb!=NULL) {
			if(curr_bb->next_t == NULL && curr_bb->next_nt == NULL) { // exit node
				curr_bb->set_post_dom((*it).second->get_id());
			} else {
				curr_bb->set_post_dom(N);
			}
		}
	}

	bool change = true;
	while(change) {
		change = false;
		// for all basic blocks
		for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
			change |= ((*it).second)->intersect_post_doms_with_successors();
		}
		//cout << ".";
		//fflush(stdout);

		//map<const char*, bb_t*, ltstr>::iterator it1;
		//for (it1=pc_bb_map.begin(); it1!=pc_bb_map.end(); it1++) {
		//	bb_t * curr_bb = (*it1).second;
		//	cout << "Dom: " << curr_bb->get_id() << ":" ; 
		//	curr_bb->print_set(curr_bb->get_dom());
		//}
		//cout << endl;
	}
}

// get adjacent nodes of "n" in the undirected graph created from cfg
list<bb_t*> cfg_t::get_adjacent_nodes_udgraph(bb_t* n) {
	string n_name = n->first_instr_pc();
	list <bb_t*> an; //adjacent nodes
	assert(start_end_edge_map.find(n_name) != start_end_edge_map.end());

	map<string, edge_t*>::iterator it;
	for(it=start_end_edge_map[n_name].begin(); it != start_end_edge_map[n_name].end(); it++) {
		string pc_name = (*it).first;
		// cout << pc_name << endl;

		if(pc_bb_map.find(pc_name) == pc_bb_map.end()) {
			if(DEBUG_SESE) {
				cout << pc_name << endl;
			 	ofstream ofs_sese, ofs_ep;
			 	string ofs_sese_filename = "test." + cfg_name + ".1.dot";
			 	string ofs_ep_filename = "test." + cfg_name + ".1.ep";
			 	cout << ofs_sese_filename << " " << ofs_ep_filename << endl;

			 	ofs_sese.open(ofs_sese_filename.c_str(), ios::out);
			 	ofs_ep.open(ofs_ep_filename.c_str(), ios::out);
			 	print_cfg(ofs_sese, ofs_ep);
			 	ofs_sese.close();
			 	ofs_ep.close();
			}
			
			// Corner case: if the block is dead and this is an empty node created, then this assertion will fail because this node would never have been added to the pc_bb_map
			assert(pc_bb_map.find(pc_name) != pc_bb_map.end());
		}
		an.push_back(pc_bb_map[pc_name]);
	}
	// an.reverse();
	return an;
}

bb_t* cfg_t::get_parent_dfs_st(bb_t* n) {
	bb_t * p = NULL;
	assert(start_end_edge_map.find(n->first_instr_pc()) != start_end_edge_map.end());
	list <bb_t *> adj_nodes = get_adjacent_nodes_udgraph(n);

	// for all adjacent nodes
	list<bb_t*>::iterator lit;
	for(lit = adj_nodes.begin(); lit != adj_nodes.end(); lit++) {
		bb_t * adj_node = (*lit);

		// check if the edge adj_node->n is found in the dfs spanning tree
		// if so, then adj_node is a child
		string edge_name = adj_node->first_instr_pc() + '-' + n->first_instr_pc();
		if(pcs_edge_map_st.find(edge_name) != pcs_edge_map_st.end()) {
			p = adj_node;
		}
	}

	assert(p != NULL);
	return p;
}

// get children of node "n" from the DFS spanning tree (pcs_edge_map_st)
list<bb_t*> cfg_t::get_children_dfs_st(bb_t* n) {
	list <bb_t*> c; //children
	assert(start_end_edge_map.find(n->first_instr_pc()) != start_end_edge_map.end());

	list <bb_t *> adj_nodes = get_adjacent_nodes_udgraph(n);

	// for all adjacent nodes
	list<bb_t*>::iterator lit;
	for(lit = adj_nodes.begin(); lit != adj_nodes.end(); lit++) {
		bb_t * adj_node = (*lit);

		// check if the edge n->adjacent_node is found in the dfs spanning tree
		// if so, then adj_node is a child
		string edge_name = n->first_instr_pc() + '-' + adj_node->first_instr_pc();
		if(pcs_edge_map_st.find(edge_name) != pcs_edge_map_st.end()) {
			c.push_back(adj_node);
		}
	}

	return c;
}

//do dfs on the undirected graph of CFG and get a list of nodes in the dfs traversal
// creates dfs_num_bb map s.t. we can access a bb by its dfs_num
// creates pcs_edge_map_st - DFS spanning tree
list<bb_t *> cfg_t::get_dfs_bb_list(bb_t * n, int & dfs_num) {
	
	if(DEBUG_SESE) {
		cout << dfs_num << endl;
	}

	list<bb_t*> dfs_list;

	n->mark_visited();
	dfs_list.push_back(n);

	// record the dfs number
	dfs_num_bb_map[dfs_num] = n;
	n->set_dfs_num(dfs_num++);

	// get adjacent nodes
	list<bb_t*> adjacent_nodes = get_adjacent_nodes_udgraph(n); // for the undirected graph, not the CFG

	// for all adjacent nodes 
	list<bb_t*>::iterator anit;
	for (anit=adjacent_nodes.begin(); anit!=adjacent_nodes.end(); anit++) {
		bb_t * adjacent_node = (*anit);
		if(!adjacent_node->is_visited()) {
			// add the edge connecting top_node->adjacent_node to spanning tree 
			string edge_name = n->first_instr_pc() + '-' + adjacent_node->first_instr_pc();
			edge_t * temp_e = start_end_edge_map[n->first_instr_pc()][adjacent_node->first_instr_pc()]; // start->end should should exist in the start_end_edge_map
			pcs_edge_map_st[edge_name] = temp_e;

			if(DEBUG_SESE) {
				cout << edge_name << endl;
			}

			// this edge is not a back-edge
			temp_e->clear_backedge();

			list<bb_t *> temp_list = get_dfs_bb_list(adjacent_node, dfs_num); // recursion

 			dfs_list.insert(dfs_list.end(), temp_list.begin(), temp_list.end()); // concatenate two vectors
		}
	}
	return dfs_list;
}


#if 0 
//do dfs on the undirected graph of CFG and get a list of nodes in the dfs traversal
list<bb_t *> cfg_t::get_dfs_bb_list() {

	list<bb_t*> dfs_list;
	stack<bb_t*> node_stack;
	int dfs_num = 0;

	map<string, bool> bb_visited_map; // bb (pc) to visited-flag map

	// mark all nodes as unvisited
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t * a = (*it).second;
		bb_visited_map[a->first_instr_pc()] = false;
	}

	// push start node to the node_stack
	node_stack.push(get_start());

	while(!node_stack.empty()) {
		
		bb_t* top_node = node_stack.top(); // get the top node
		node_stack.pop(); // pop the top value from the stack

		assert(bb_visited_map.find(top_node->first_instr_pc()) != bb_visited_map.end());

		if(!bb_visited_map[top_node->first_instr_pc()]) {
			bb_visited_map[top_node->first_instr_pc()] = true;

			// record the dfs number
			top_node->set_dfs_num(dfs_num++);

			dfs_list.push_back(top_node);

			// get adjacent nodes
			list<bb_t*> adjacent_nodes = get_adjacent_nodes_udgraph(top_node); // for the undirected graph, not the CFG

			// for all adjacent nodes 
			list<bb_t*>::iterator anit;
			for (anit=adjacent_nodes.begin(); anit!=adjacent_nodes.end(); anit++) {
				bb_t * adjacent_node = (*anit);
				if(!bb_visited_map[adjacent_node->first_instr_pc()]) {
					// add it to the node stack
					node_stack.push(adjacent_node);

					// add the edge connecting top_node->adjacent_node to spanning tree 
					string edge_name = top_node->first_instr_pc() + '-' + adjacent_node->first_instr_pc();
					edge_t * temp_e = start_end_edge_map[top_node->first_instr_pc()][adjacent_node->first_instr_pc()]; // start->end should should exist in the start_end_edge_map
					pcs_edge_map_st[edge_name] = temp_e;
				}
			}
		}
	}

	if(DEBUG_SESE) {
		cout << cfg_name << ": " ;
		list<bb_t*>::iterator lit;
		for (lit=dfs_list.begin(); lit!=dfs_list.end(); lit++) {
			bb_t * a = (*lit);
			cout << a->first_instr_pc() << " - ";
		}
		cout << endl;
	}
	return dfs_list;
}
#endif

// flag is used to supress creating a new subgraph - true:supress
void cfg_t::print_region_subgraph(sese_region_t * r, ofstream & ofs, int c, bool flag) {
	edge_t * s = r->get_entry(); // entry edge
	edge_t * e = r->get_exit(); // exit edge

	int num_instructions = 0;

	if( (s != NULL) && (e != NULL) && ( s != e) ) {

		string exit_point = e->get_succ()->first_instr_pc(); 

		if(!flag)  {
			ofs << endl << " subgraph cluster_" << c << " { " << endl;
			string col = get_random_color();
			ofs << " node [style=filled, color=\"#" << col << "\" ];" << endl;
			ofs << " color=\"#" << col << "\";" << endl;
		}
		// s->print_edge();
		// cout << endl;
		// e->print_edge();
		// cout << endl;

		map<string, bool> edge_visited_map; // edge (pc-pc) to visited-flag map
		stack<edge_t*> edge_stack;
	
		// mark all edges as unvisited
		map<string, edge_t*>::iterator it;
		for(it=pcs_edge_map.begin(); it!= pcs_edge_map.end(); it++) {
			edge_t* e = (*it).second;
			edge_visited_map[e->get_name()] = false;
		}
	
		// s->successor is the first node in the sese_region
		vector<bb_t*> adj_nodes;
		s->get_succ()->get_adjacent_nodes(adj_nodes); // adjacent nodes
		for(int i=0; i<adj_nodes.size(); i++) {
			bb_t* n1 = s->get_succ();
			bb_t* n2 = adj_nodes[i];
			string edge_name = n1->first_instr_pc() + '-' + n2->first_instr_pc();
			assert(pcs_edge_map.find(edge_name) != pcs_edge_map.end());
			edge_stack.push(pcs_edge_map[edge_name]);
			num_instructions += n2->size();

			n1->add_exit_point(exit_point);
		}

		int count = 0;
	
		while(!edge_stack.empty()) {
	
			edge_t* top_edge = edge_stack.top(); // get the top node
			edge_stack.pop(); // pop the top value from the stack

			if(top_edge == e) { // top is exit edge; do not add its successors to the list
				continue;
			}

			count++;

			bb_t* n1 = top_edge->get_pred();
			bb_t* n2 = top_edge->get_succ();
			string n1_name = n1->first_instr_pc();
			string n2_name = n2->first_instr_pc();
			ofs << "\"PC" << n1_name << "-S" << n1->size() << "(" << n1->does_end_with_call() << ")\" -> ";
			ofs << "\"PC" << n2_name << "-S" << n2->size() << "(" << n2->does_end_with_call() << ")\";\n";
			num_instructions += n2->size();

			n2->add_exit_point(exit_point);
			if(n2->first_instr_pc().compare("0x100003870") == 0) {
				if (exit_point.compare("0x1000033a0") == 0) {
					cout << "********++++++*********: " << exit_point << endl;
				}
			}

			// if not visited
			if( !edge_visited_map[top_edge->get_name()]) { 
	
				edge_visited_map[top_edge->get_name()] = true; // visting this edge
	
				// get adjacent nodes of successor of top_edge
				vector<bb_t*> adj_nodes;
				top_edge->get_succ()->get_adjacent_nodes(adj_nodes); // adjacent nodes
				for(int i=0; i<adj_nodes.size(); i++) {
					string edge_name = top_edge->get_succ()->first_instr_pc() + '-' + adj_nodes[i]->first_instr_pc();
					assert(pcs_edge_map.find(edge_name) != pcs_edge_map.end());
					edge_stack.push(pcs_edge_map[edge_name]);
				}
			}
		}
		// do not print sese regions that have just one bb 
		if(count == 0) {
			bb_t* n1 = s->get_succ();
			string n1_name = n1->first_instr_pc();
			ofs << "\"PC" << n1_name << "-S" << n1->size() << "(" << n1->does_end_with_call() << ")\" ";

			n1->add_exit_point(exit_point);
		}

		if(!flag) 
			ofs << "}" << endl;

	} else {
		// r->print_region();
	}
	
	// cout << "SESERegionSize=" << num_instructions << endl;
}


//do dfs on the CFG and get a list of edges in the dfs traversal
list<edge_t *> cfg_t::get_dfs_edge_list() {

	list<edge_t*> dfs_list;
	stack<edge_t*> edge_stack;
	int dfs_num = 0;

	map<string, bool> edge_visited_map; // edge (pc-pc) to visited-flag map

	// mark all edges as unvisited
	map<string, edge_t*>::iterator it;
	for(it=pcs_edge_map.begin(); it!= pcs_edge_map.end(); it++) {
		edge_t* e = (*it).second;
		edge_visited_map[e->get_name()] = false;
	}

	// push start->successors edges to the edge_stack
	vector<bb_t*> adj_nodes;
	get_start()->get_adjacent_nodes(adj_nodes); // adjacent nodes
	for(int i=0; i<adj_nodes.size(); i++) {
		string edge_name = get_start()->first_instr_pc() + '-' + adj_nodes[i]->first_instr_pc();
		assert(pcs_edge_map.find(edge_name) != pcs_edge_map.end());
		edge_stack.push(pcs_edge_map[edge_name]);
	}

	while(!edge_stack.empty()) {

		edge_t* top_edge = edge_stack.top(); // get the top node
		edge_stack.pop(); // pop the top value from the stack

		// if not visited
		if( !edge_visited_map[top_edge->get_name()]) { 

			edge_visited_map[top_edge->get_name()] = true; // visting this edge

			// record the dfs number
			top_edge->set_dfs_num(dfs_num++);
			dfs_list.push_back(top_edge);

			if(top_edge->get_succ() == end) {
				edge_stack.push(start_end_edge_map[end->first_instr_pc()][start->first_instr_pc()]); 
			} else {
				// get adjacent nodes of successor of top_edge
				vector<bb_t*> adj_nodes;
				top_edge->get_succ()->get_adjacent_nodes(adj_nodes); // adjacent nodes
				for(int i=0; i<adj_nodes.size(); i++) {
					string edge_name = top_edge->get_succ()->first_instr_pc() + '-' + adj_nodes[i]->first_instr_pc();
					assert(pcs_edge_map.find(edge_name) != pcs_edge_map.end());
					edge_stack.push(pcs_edge_map[edge_name]);
				}
			}
		}
	}

	return dfs_list;
}


int cfg_t::new_class() {
	return eq_class_count++;
}

// See Figure 3.7 in http://dspace.library.cornell.edu/bitstream/1813/6073/1/94-1464.pdf
void cfg_t::build_bracket_list(bb_t* n, int nl_size) {

	string n_name = n->first_instr_pc();

 	/* compute n.hi*/ 
 
 	/* hi0 = min { t.dfsnum | (n,t) is a backedge } */
 
 	int hi0 = nl_size + 1;
	// for all edges starting at n
	map<string, edge_t*>::iterator it;
	for(it=start_end_edge_map[n_name].begin(); it != start_end_edge_map[n_name].end(); it++) {
		string pc_name = (*it).first;
		edge_t * ae = (*it).second;

		if(ae->is_backedge()) {
			assert(pc_bb_map.find(pc_name) != pc_bb_map.end());
 			// min (hi0, adjacent_node->get_dfs_num() )
			hi0 = min (hi0, pc_bb_map[pc_name]->get_dfs_num());
		}
	}

 	/* hi1 = min { c.hi | c is a child of n } */
 
 	int hi1 = nl_size + 1;

	list<bb_t*> children = get_children_dfs_st(n);
	list<bb_t*>::iterator lit;
 	for (lit=children.begin(); lit!=children.end(); lit++) {
 		bb_t * child = (*lit);
 
 		// min (hi1, adjacent_node->get_hi() )
 		hi1  = min (hi1, child->get_hi());
 	}

 	/* n.hi = min { hi0, hi1 } */
 	n->set_hi( min(hi0, hi1) );
 
 	/* hichild = any child c of n having c.hi = hi1 */
 	bb_t * hichild = NULL;
 	for (lit=children.begin(); lit!=children.end(); lit++) {
 		bb_t * child = (*lit);
 
 		if(child->get_hi() == hi1) {
 			hichild = child;
 			break;
 		}
 	}
 
 	/* hi2 = min { c.hi | c is a child of n other than hichild } */
 	int hi2 = nl_size + 1;
 	for (lit=children.begin(); lit!=children.end(); lit++) {
 		bb_t * child = (*lit);
 		if (hichild != child) {
 			// hi2 = min(hi2, child->get_hi());
			if(child->get_hi() < hi2) {
				hi2 = child->get_hi();
			}
 		}
 	}

	if(DEBUG_SESE) {
		cout << n->first_instr_pc() << ": hi0=" << hi0 << ", hi1=" << hi1 << ", n.hi=" << n->get_hi() << ", hi2=" << hi2 << endl; 
	}
 
 	/* compute bracketlist */
 
 	/* n.blist = create() */
 	n->blist.clear();
 	
 	/* for each child c of n do
 	     n.blist := concat (c.blist, n.blist) 
 	   endfor  */
 	for (lit=children.begin(); lit!=children.end(); lit++) {
 		bb_t * child = (*lit);
 		n->blist.insert(n->blist.end(), child->blist.begin(), child->blist.end()); // concatenate two vectors
 	}
	print_blist(n);
 
 	/* for each capping backedge d from a decendant of n to n do
 	     delete (n.blist, d);
 	   endfor */
 	// TODO: how to identify a capping backedge
 
 	/* for each backedge b from a decendant of n to n do
 	     delete (n.blist, b);
 		 if b.class undefined then
 			 b.class = new-class();
     	 end if
 	   endfor  

	   OR 

 	   for each edge e in blist n.blist do
 	     delete it from n.blist if e is a backedge from a decendant of n to n
 
 		 if e is a backedge but not the capping backedge then
 		   if e.class undefined then
 		     e.class = new-class();
     	   end if
     	 end if
	   endfor
 	*/
	vector <int> erase_list;
 	for(int i=0; i<n->blist.size(); i++) {
 
 		// e connects n1 and n2. It is an undirected edge, so do not give importance to pred and succ
 		edge_t * e = n->blist[i];

		string n1 = e->get_pred()->first_instr_pc();
		string n2 = e->get_succ()->first_instr_pc();

		bool flag  = n1.compare(n_name) == 0 && e->get_succ()->get_dfs_num() > n->get_dfs_num(); // second part is "is descendant"
		flag = flag || n2.compare(n_name) == 0 && e->get_pred()->get_dfs_num() > n->get_dfs_num(); 
		if(flag) {

			if(e->is_capping_edge()) {
				erase_list.push_back(i);

			} else if(e->is_backedge()) {
				erase_list.push_back(i);

				// need to check that its is not a capping backedge
				if(e->undefined_eq_class()) {
					e->set_eq_class(new_class());

					if(DEBUG_SESE) {
						cout << " eq_class: " << e->get_recent_class() << " in build-bracket_list at " << n->first_instr_pc() << " for edge: ";
						e->print_edge();
						cout << endl;
					}
				}
			}
		}
 	}
	for(int j=erase_list.size()-1; j>=0; j--) {
		n->blist.erase(n->blist.begin() + erase_list[j]);
	}
	print_blist(n);
 
 	/* for each backedge e from n to an ancestor of n do
 	     push (n.blist, e);
 	   endfor */
 	// for all edges starting at n
	for(it=start_end_edge_map[n_name].begin(); it != start_end_edge_map[n_name].end(); it++) {
		string pc_name = (*it).first;
		edge_t * ae = (*it).second; // adjacent edge

		if(ae->is_backedge()) { // is backedge
			assert(pc_bb_map.find(pc_name) != pc_bb_map.end());

			if(pc_bb_map[pc_name]->get_dfs_num() < n->get_dfs_num()) { // if ancestor
				n->blist.push_back(ae);
			}
		}
	}
	print_blist(n);

	// cout << "hi2 = " << hi2 << ", hi0 = " << hi0 << endl;

 	/* if hi2 < hi0 then
 	     d := (n, node[hi2]) // create capping backedge
 		 push (n.blist, d);
 	*/
 	if( hi2 < hi0 ) {
		// assert(start_end_edge_map[n_name].find(hi2n->first_instr_pc()) != start_end_edge_map[n_name].end());
		assert(dfs_num_bb_map.find(hi2) != dfs_num_bb_map.end());
		bb_t * hi2n = dfs_num_bb_map[hi2];

		edge_t * e = NULL;
		if(start_end_edge_map[n_name].find(hi2n->first_instr_pc()) != start_end_edge_map[n_name].end()) {
			e = start_end_edge_map[n_name][hi2n->first_instr_pc()];
		} else {
			e = new edge_t(n, hi2n);
		}
		n->blist.push_back(e);
		e->set_capping_edge();
 	}
	print_blist(n);
}

void cfg_t::print_blist(bb_t * n) {

	if(DEBUG_SESE) {
		cout << "Bracket list for node " << n->first_instr_pc() << " is: " ;
		for(int i=0; i < n->blist.size(); i++) {
			(n->blist[i])->print_edge();
			cout << ", ";
		}
		cout << endl;
	}
}

void cfg_t::cycle_equivalence() {

	// reverse DFS
	int dfs_num = 0;
	list<bb_t*> node_list = get_dfs_bb_list(start, dfs_num);

	if(DEBUG_SESE) {
		cout << cfg_name << ": " ;
		list<bb_t*>::iterator lit;
		for (lit=node_list.begin(); lit!=node_list.end(); lit++) {
			bb_t * a = (*lit);
			cout << a->first_instr_pc() << " - ";
		}
		cout << endl;
	}

	reverse(node_list.begin(), node_list.end());

	/* for each node n in reverse depth-frist order do */
	list<bb_t*>::iterator lit;
	for (lit=node_list.begin(); lit!=node_list.end(); lit++) {
		bb_t * n = (*lit);

		build_bracket_list(n, node_list.size()); // updates n.blist
		
		if(n->get_dfs_num() != 0) {
 			bb_t * parent_n = get_parent_dfs_st(n);
 			string edge_name = parent_n->first_instr_pc() + '-' + n->first_instr_pc();
 			edge_t * e = pcs_edge_map_st[edge_name];
 
 			if(n->blist.size() > 0) {
	 			edge_t * b = n->blist.back();
	
	 			if(b->get_recent_size() != n->blist.size()) {
	 				b->set_recent_size(n->blist.size());
	 				b->set_recent_class(new_class());
	 			}
	 
	 			e->set_eq_class(b->get_recent_class());
				if(DEBUG_SESE) {
					cout << " eq_class: " << b->get_recent_class() << " at node " << n->first_instr_pc() << " for edge: ";
					e->print_edge();
					cout << endl;
				}
	 
	 			/* check for e,b equivalence */
	 			if(b->get_recent_size() == 1) {
	 				b->set_eq_class(e->get_eq_class());
					if(DEBUG_SESE) {
						cout << " eq_class: " << b->get_recent_class() << " at node " << n->first_instr_pc() << " for edge: ";
						b->print_edge();
						cout << endl;
					}
	 			}
			}
		}
	}

}

// use this, not the next one to find sese regions
void cfg_t::find_sese_regions() {

	cycle_equivalence();

	list<edge_t*>::iterator lit;
	list<edge_t*> edge_list = get_dfs_edge_list(); // dfs order

 	if(DEBUG_SESE) {
		// print eq_class for each edge
		cout << "Class information: " << endl;
		for(lit = edge_list.begin(); lit != edge_list.end(); lit++) {
			edge_t * e = (*lit);
			cout << "(" << e->get_pred()->first_instr_pc() << "-" << e->get_succ()->first_instr_pc() << ") = " << e->get_eq_class() << endl;
		}
		// map<string, edge_t*>::iterator ite;
		// for(ite=start_end_edge_map[end->first_instr_pc()].begin(); ite!=start_end_edge_map[end->first_instr_pc()].end(); ite++) {
		// 	edge_t * e = (*ite).second;
		// 	cout << "(" << e->get_pred()->first_instr_pc() << "-" << e->get_succ()->first_instr_pc() << ") = " << e->get_eq_class() << ", $=last edge in cfg" << endl;
		// }
	}

	edge_list.reverse(); // reverse dfs order

	map<int, bool> class_found;
	// set last class info
	for(lit=edge_list.begin(); lit!= edge_list.end(); lit++) {
		edge_t * e = (*lit);
		// if eq_class is being seen for the first time then mark it as the last_class
		if(class_found.find(e->get_eq_class()) == class_found.end()) {
			if(!e->undefined_eq_class()) { // fix added later to not let arbitrary regions form by undefined edges (undefined means eq_class = -1)
				class_found[e->get_eq_class()] = true;
				e->set_last_class();
			}
		}
	}

	map<int, sese_region_t*> last_region;
	edge_list.reverse(); // dfs order
	// in dfs order
	for(lit=edge_list.begin(); lit!= edge_list.end(); lit++) {
		edge_t * e = (*lit);
	 	string e_name = e->get_name();

		if(!e->undefined_eq_class()) { // fix added later to not let arbitrary regions form by undefined edges (undefined means eq_class = -1)
			/* class := e.class */
	 		int temp_class = e->get_eq_class();

			/* is edge the exit of a previous region? */
			if(last_region.find(temp_class) != last_region.end()) {
				sese_region_t * r = last_region[temp_class];
				r->set_exit(e);
				e->set_sese_exit(r);
			}
			
			if(!e->is_last_class()) {
	 			sese_region_t * r = new sese_region_t(e);
				e->set_sese_entry(r);
				last_region[temp_class] = r;
				sese_regions.push_back(r);
			}
		}
	}

	if(DEBUG_SESE) {
	 	cout << "SESE regions: ";
		cout << sese_regions.size() << endl ;
	 	list<sese_region_t*>::iterator it;
		for(it=sese_regions.begin(); it!=sese_regions.end(); it++) {
			sese_region_t * r = (*it);
			r->print_region();
			cout << endl;
		}
	}

	// clear all edges' "visited" bit - should be done before build_pst call 
	map<string, edge_t*>::iterator eit;
	for(eit=pcs_edge_map.begin(); eit!=pcs_edge_map.end(); eit++) {
		edge_t * e = (*eit).second;
		e->clear_visited();
	}

	edge_t * start_edge = new edge_t(NULL, start);
	top_region = new sese_region_t();
	if(top_region != NULL) {
		// cout << "Building PST.. " << endl;
		build_pst(start_edge, top_region); // build program struction tree;
	}

}

void cfg_t::build_pst(edge_t * e, sese_region_t * r) {
	bb_t * n = e->get_succ();
	e->mark_visited();
	// e->print_edge();
	// cout << endl;
	assert(r != NULL);

	// if e is a SESE exit edge (and e is not an exit and entry for same region) then
	if( e->get_sese_exit() != NULL && e->get_sese_entry() != e->get_sese_exit() ) {
		if(r != top_region) { 
			r = r->get_parent();
		}
	}

	if(e->get_sese_entry() != NULL) {
		sese_region_t * s = e->get_sese_entry(); // get local region

		/* update PST links */
		s->set_parent(r);
		// s->set_sibling(r->get_child());
		r->set_child(s);

		/* if region is non-empty, update current region */
		if(s->get_entry() != s->get_exit()) {
			r = s;
		}
	}

	vector<bb_t*> adj_nodes;
	n->get_adjacent_nodes(adj_nodes);  // in CFG
	for(int i=0; i<adj_nodes.size(); i++) {
		string e_name = n->first_instr_pc() + '-' + adj_nodes[i]->first_instr_pc();
 		edge_t * e = pcs_edge_map[e_name];

		if(!e->is_visited()) {
			build_pst(e, r);
		}
	}
}

bool cfg_t::list_backedges(path_t *path, bb_t* curr_bb) {

// 	(*path)[curr_bb->get_id()] = true; // add curr_bb to path
// 
// 	for(int i=0; i<2; i++) {
// 		bb_t * b = NULL; // new candidate
// 		if(i==0) {
// 			b = curr_bb->next_t;
// 		} else {
// 			b = curr_bb->next_nt;
// 		}
// 		if(b == NULL)
// 			continue;
// 
// 		if((*path)[b->get_id()]) {
// 			//backedge found curr_bb->b
// 			backedges[curr_bb->first_instr_pc().c_str()] = b->first_instr_pc();
// 		} else {
// 			list_backedges(path, b);
// 			(*path)[b->get_id()] = false; // add curr_bb to path
// 		}
// 	}
}

long cfg_t::get_num_patterns(bb_t* curr_bb, set<string> path) {

	long num_patterns = 0;

	if(curr_bb != NULL) {
		for(int i=0; i<2; i++) {
			bb_t * next_bb = NULL;
			if(i==0) {  // next_t
				next_bb = curr_bb->next_t;
			} else { // next_nt
				next_bb = curr_bb->next_nt;
			}

			if(next_bb != NULL) {

				bool proceed = false;
				if(path.find(next_bb->first_instr_pc()) != path.end()) { // next_bb found in path
					proceed = false;
				} else {
					proceed = true;
				}
				if(proceed) {
					//cout << curr_bb->get_id() << "->" << next_bb->get_id() << endl;
					path.insert(next_bb->first_instr_pc());
					num_patterns = num_patterns + get_num_patterns(next_bb, path) + 1;
				}
			}
		}
	}
	return num_patterns;
}

long cfg_t::gen_control_patterns(string prg_name) {
	long num_patterns = 0;
	map<string, bb_t*>::iterator it;
	for (it=pc_bb_map.begin(); it!=pc_bb_map.end(); it++) {
		bb_t * curr_bb = (*it).second;
		if(curr_bb != NULL) {
			num_patterns += curr_bb->gen_control_patterns(prg_name);
		}
	}
	return num_patterns;
}

bb_t * cfg_t::get_start() {
	return start;
}

cfg_t * cfg_t::get_cfg(string pc) {
	return prog->get_cfg(pc);
}

program_t * cfg_t::get_program() {
	return prog;
}

bb_t * cfg_t::get_cfg_start(string pc) {
	return prog->get_cfg_start(pc);
}

bb_t * cfg_t::get_end() {
	return end;
}

void cfg_t::add_caller(bb_t * b) {
	callers.push_back(b);
}

list<bb_t*> cfg_t::get_callers() {
	return callers;
}

bool cfg_t::is_terminal_node(bb_t* n) {

	string n_name = end->first_instr_pc();
	list <bb_t*> an; //adjacent nodes
	assert(start_end_edge_map.find(n_name) != start_end_edge_map.end());

	map<string, edge_t*>::iterator it;
	for(it=start_end_edge_map[n_name].begin(); it != start_end_edge_map[n_name].end(); it++) {
		string pc_name = (*it).first;

		assert(pc_bb_map.find(pc_name) != pc_bb_map.end());
		bb_t * adj_n = pc_bb_map[pc_name];
		if(adj_n != start) {
			if(adj_n == n) {
				return true;
			}
		}
	}
	return false;
}


