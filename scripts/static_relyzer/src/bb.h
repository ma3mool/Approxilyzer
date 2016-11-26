#ifndef BB_H
#define BB_H

#include <string>
#include <set>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>


enum dep_type_t{
	STORE, // 0
	CONTROL, // 1
	LIVE, // 2
	CALL_SITE, // 3
	ILLTRAP, // 4
	RET_SITE, // 5
	SAVE_SITE, // 6
	DEAD, // 7
	STATEREG_OP, // 8
	STORE_AND_LIVE, // 9
	COMPARE, // 10
	REMAINING_OTHERS // 11
};

enum {
	BRANCH_TYPE, // 0
	CALL_TYPE, // 1
	RET_TYPE, // 2
	NONE_TYPE, // 3
	JUMP_TYPE // 4
};

extern int sese_count;
extern int bb_count;

#include "instruction.h"
#include "cfg.h"

class cfg_t;
class instruction_t;
class edge_t;
class sese_region_t;

class var_info_t {
	private:
		int num_uses;
		int num_patterns;
	public:
		int get_num_patterns() {
			return num_patterns;
		}
		void set_num_patterns(int n){
			num_patterns = n;
		}
}; 


class bb_t {
	std::vector <instruction_t*> instrns;
	int id;
	std::vector <bb_t*> predecessors;

	std::map<std::string, int> in; // registers
	std::map<std::string, int> out; // registers

	std::set<int> dom;
	std::set<int> post_dom;
	bool post_dom_changed;

	std::map<std::string, std::vector< std::list<std::string> > > pc_patterns; // pc -> list of lists
	cfg_t * parent_cfg;
	cfg_t * call_cfg;
	bool ends_with_call;
	int last_instr_type;

	// needed by cycle equivalence function (to find SESE regions)
	int dfs_num; 
	int hi;
	bool visited;
	bool reachable;

	std::list<std::string> exit_points;

	public:

		// I am making an exception by placing a vector in public scope. 
		// Try to make it a private variable
		std::vector <edge_t *> blist; // needed by cycle equivalence function (to find SESE regions)

		bb_t(int id, cfg_t * c);
		~bb_t();
		bb_t* next_t;
		bb_t* next_nt;

		void add_inst(instruction_t * i);
		void move_instructions(std::string pc, bb_t * bb2);

		bool is_empty();
		int get_id();
		std::string first_instr_pc();
		std::string second_instr_pc();
		std::string last_1_instr_pc();
		std::string last_instr_pc();
		int size();

		bool is_def_killed_within(std::string reg, int index);
		int find_leading_live_def(int index);
		int has_use_before_def(std::string reg);

		void update_out();
		void find_invariant_detectors();
		std::vector<std::string> find_incement_regs(std::string& pc);
		void mark_store_instructions();
		void mark_control_instructions();
		void mark_other_instructions();
		void mark_leading_instructions(int bb_index, std::string def, std::string pc, dep_type_t dep_type, int leading_index);
		bool is_def_matching(std::string def, std::string reg);
		void mark_unlive_registers();
		void prune_defs();
        void prune_duplicate_uses(); 
		void mark_nops();

		std::string reg_after_save(std::string);
		std::string reg_after_restore(std::string);

		// void set_dom(std::string);
		// void set_dom(std::set<std::string>);
		// std::set<std::string> get_dom();
		// bool is_in_dom(std::string);
		// bool intersect_doms_with_predecessors();
		// void set_post_dom(std::string);
		// void set_post_dom(std::set<std::string>);
		// std::set<std::string> get_post_dom();
		// bool is_in_post_dom(std::string);
		// bool intersect_post_doms_with_successors();
		void set_dom(int);
		void set_dom(std::set<int>);
		std::set<int> get_dom();
		bool is_in_dom(int);
		bool intersect_doms_with_predecessors();
		void set_post_dom(int);
		void set_post_dom(std::set<int>);
		std::set<int> get_post_dom();
		bool is_in_post_dom(int);
		bool intersect_post_doms_with_successors();
		std::vector<bb_t*> get_predecessors();
		std::vector<bb_t*> get_successors();
		void  get_adjacent_nodes(std::vector<bb_t*> & v);

		void set_dfs_num(int n);
		int get_dfs_num();
		void mark_visited();
		bool is_visited();
		void clear_visited();
		void set_hi(int h);
		int get_hi();
		bool test_backedge(std::set<std::string>, std::set<std::string>, bb_t*, bool);
		void set_reachable();
		bool is_reachable();

		void add_predecessor(bb_t*); 
		void remove_predecessor(bb_t*); 
		int num_predecessors();
		void print_predecessors();

		bool is_back_edge( bb_t *n1, bb_t *n2); 
		void print_set(std::set<int>);

		void set_ends_with_call();
		void unset_ends_with_call();
		bool is_last_inst_restore();
		bool is_first_inst_save();
		bool does_end_with_call();
		cfg_t * get_target_cfg();
		cfg_t * get_cfg(std::string);
		bb_t * get_call_cfg_start();
		int find_last_instr_type();
		void set_last_instr_type(int);
		int get_last_instr_type();
		bool has_post_dom_changed();
		void set_post_dom_changed();
		void unset_post_dom_changed();
		void mark_for_extra_counting(int,std::string, std::string);

		int frequency();

		void print_bb();

		std::vector< std::list<std::string> > get_cpattern(std::list<std::string> pc_list, int depth);
		long gen_control_patterns(std::string prg_name);

		void add_exit_point(std::string str);
		void print_exit_points(std::ofstream &);

};


class edge_t {
	bb_t * pred;
	bb_t * succ;
	bool backedge;
	std::string name;

	sese_region_t* sese_entry;
	sese_region_t* sese_exit;

	int eq_class;
	bool visited;
	int recent_size;
	int recent_class;
	bool last_class;

	bool capping;

	int dfs_num;
	public:
		edge_t();
		edge_t(bb_t* pred, bb_t* succ);
		void set_pred(bb_t *);
		void set_succ(bb_t *);
		void set_name(std::string n);
		std::string get_name();
		bb_t * get_pred();
		bb_t * get_succ();

		void set_backedge();
		void clear_backedge();
		bool is_backedge();

		void set_eq_class(int);
		bool undefined_eq_class();
		int get_eq_class();

		int get_recent_size();
		void set_recent_size(int s);

		int get_recent_class();
		void set_recent_class(int c);

		void set_last_class();
		bool is_last_class();

		void set_dfs_num(int n);
		int get_dfs_num();

		bool check_name(std::string s);

		void set_capping_edge();
		bool is_capping_edge();

		void print_edge();

		void mark_visited();
		bool is_visited();
		void clear_visited();

		void set_sese_entry(sese_region_t *s);
		void set_sese_exit(sese_region_t *s);
		sese_region_t * get_sese_exit();
		sese_region_t * get_sese_entry();
};


class sese_region_t {
	edge_t * entry;
	edge_t * exit;
	int id;

	sese_region_t * parent;
	std::list<sese_region_t *> children;
	int num_instructions;
	public:
		sese_region_t();
		sese_region_t(edge_t * e);
		void set_exit(edge_t *e);

		edge_t * get_entry();
		edge_t * get_exit();
		void print_region();

		void set_parent(sese_region_t * p);
		sese_region_t * get_parent();

		void set_child(sese_region_t* c);
		std::list<sese_region_t*> get_children();
		bool has_children();

		void set_num_instructions(int a);
		int get_num_instructions();
		int get_id() { return id; }
};

#endif // BB_H
