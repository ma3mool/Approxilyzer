#ifndef CFG_H
#define CFG_H

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <map>
#include "bb.h"
#include "program.h"
#include "instruction.h"
#include <ext/hash_map>


std::string get_random_color();

struct eqint {
	bool operator()(int i1, int i2) {
		return i1 == i2;
	}
};

//typedef __gnu_cxx::hash_map<int, int, __gnu_cxx::hash<int>, eqint>  path_t;
typedef std::vector<bool> path_t;


class program_t;
class bb_t;
class instruction_t;
class edge_t;
class sese_region_t;


class cfg_t {
	bb_t * start;
	bb_t * end;
	program_t * prog;
	std::map<std::string, bb_t*> pc_bb_map; // pc to basic block map

	// for directed CFG
	std::map<std::string , edge_t*> pcs_edge_map; // pc-pc to edge map  - start_pc->end_pc for CFG 

	std::map<int, bb_t*> dfs_num_bb_map; // access a bb by its dfs_num

	// edges for undirected graph
	// start_pc1 -> [ map of end_pc->edge_t* ]
	// start_pc2 -> [ map of end_pc->edge_t* ]
	// .
	// .
	// start_pcn -> [ map of end_pc->edge_t* ]
	std::map<std::string, std::map<std::string, edge_t*> > start_end_edge_map; // both start_pc->end_pc and end_pc->start_pc map to the cfg edge

	std::map<std::string , edge_t*> pcs_edge_map_st; // pc-pc to edge map  - start_pc->end_pc for dfs spanning tree
	std::list<sese_region_t*> sese_regions;
	int start_index;
	int end_index;
	int count_id;
	std::string cfg_name;
	int eq_class_count; 
	sese_region_t * top_region;

	std::list<bb_t *> callers; // list of bbs that have a call instruction to this function

	public:
		cfg_t(program_t * prg, std::string cn);
		~cfg_t();
		void create_cfg(std::vector <instruction_t> * prg, int start);
		void make_missing_edges(std::vector <instruction_t> * prg);
		void fill_holes(std::string prg_name, std::string app_name, std::vector <instruction_t> * prg, std::map<std::string,std::string>&arcs);
		void clean_bbs(std::vector <instruction_t> * prg);
		void prepare_for_sese(std::vector <instruction_t> * prg);
		void find_and_split(std::vector <instruction_t> * prg, std::string target);
		void split_bb(bb_t *b, std::string pc);
		bb_t* get_end();
		bool is_terminal_node(bb_t *);

		// SESE related stuff
		void create_edges();
		std::string get_name() {return cfg_name; }
		void traverse_add_endnode(bb_t* n);
		void cycle_equivalence();
		void find_sese_regions();
		std::list<bb_t*> get_dfs_bb_list(bb_t * start, int & dfs_num);
		std::list<edge_t*> get_dfs_edge_list();
		std::list<bb_t*> get_adjacent_nodes_udgraph(bb_t* n);
		std::list<bb_t*> get_children_dfs_st(bb_t* n);
		bb_t* get_parent_dfs_st(bb_t* n); 
		void build_bracket_list(bb_t* n, int nl_size);
		int new_class();
		void print_region_subgraph(sese_region_t *r, std::ofstream & ofs, int c, bool f);
		void print_pst(sese_region_t * r, std::ofstream & ofs, int & c);
		void build_pst(edge_t * e, sese_region_t * r);
		void print_blist(bb_t *);

		void mark_nops();
		void print_cfg();
		void print_cfg(std::ofstream & ofs_sese, std::ofstream & ofs_exit_points);
		int get_num_bbs();
		int get_count_id();
		bb_t* get_start();

		long get_num_patterns(bb_t*, std::set<std::string>);
		long gen_control_patterns(std::string prg_name);

		void prune_defs();
		void find_invariant_detectors();
		void live_reg_analysis();
		void mark_unlive_registers();
		void mark_store_instructions();
		void mark_control_instructions();
		void mark_other_instructions();
		void create_dominators();
		void create_post_dominators();
		bool list_backedges(path_t *, bb_t*);

		cfg_t * get_cfg(std::string str);
		bb_t * get_cfg_start(std::string str);

		program_t * get_program();

		void add_caller(bb_t *);
		std::list<bb_t*> get_callers();
};

#endif // CFG_H
