#ifndef DDG_H
#define DDG_H

#include <vector>
#include "statici.h" 

using namespace std ;

typedef vector<ddg_node_t*> ddg_node_list_t ;
struct uint64_cmp {
	bool operator() (uint64 A, uint64 B) {
		return A < B ;
	}
} ;
typedef set<uint64, uint64_cmp> inst_set_t ;

class ddg_node_t {
	public:
	uint64 seq ;
	uint64 cycle ;
	uint64 pc ;
	ddg_node_list_t parents ;
	ddg_node_list_t children ;
	uint64 ddg_size ;
	uint64 last_reader ;

	ddg_node_list_t special_children ; // Nodes in subtree that have multiple parents

	ddg_node_t(uint64 s, uint64 c, uint64 p) ;
	~ddg_node_t() ;

	void addParent(ddg_node_t* D) ;
	bool isOrphan() 		{ return parents.empty() ; }
	uint64 getNumParents() ;

	void addChild(ddg_node_t* C) ;
	bool isChildless() 		{ return children.empty() ; }
	uint64 getNumChildren() ;

	void printDDG() ;
	void printReverseDDG() ;

	uint64 getDDGSize() ;
	uint64 getDDGSize(inst_set_t &already_seen) ;
	// inst_set_t getUniqueNodes() ;
	uint64 getLastReadInst() ;

	void addSpecialChild(ddg_node_t *C)	;

} ;

#endif // DDG_H
