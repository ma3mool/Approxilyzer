#include "hfa.h"
#include "regfile.h"
#include "arf.h"
#include "opcode.h"
#include "dynamic.h"
#include "system.h"

#include "interface.h"
#include "pstate.h"
#include "ddg.h" 

#define DEBUG_DDG 0
#define DEBUG_DDG_SIZE 1

ddg_node_t::ddg_node_t(uint64 s, uint64 c, uint64 p) {
	seq = s ;
	cycle = c ;
	pc = p ;
	parents.clear() ;
	children.clear() ;
	special_children.clear() ;
	ddg_size = 0 ;
	last_reader = 0 ;
}

ddg_node_t::~ddg_node_t() {
	parents.clear() ;
	children.clear() ;
	special_children.clear() ;
}

void ddg_node_t::addParent(ddg_node_t* D)
{
	bool found = false ;
	for(ddg_node_list_t::iterator I=parents.begin(), E=parents.end();
			I!=E; I++) {
		if(*I==D) {
			found = true ;
			break ;
		}
	}
	if(!found) {
		parents.push_back(D) ;
		if(DEBUG_DDG) {
			DEBUG_OUT("%d is parent of %d\n", D->seq, this->seq) ;
		}
	}
}

uint64 ddg_node_t::getNumParents()
{
	return ((uint64) parents.size()) ;
}

uint64 ddg_node_t::getNumChildren()
{
	return ((uint64) children.size()) ;
}

void ddg_node_t::addChild(ddg_node_t* C) 
{
	bool found = false ;
	for(ddg_node_list_t::iterator I=children.begin(), E=children.end();
			I!=E; I++) {
		if(*I==C) {
			found = true ;
			break ;
		}
	}
	if(!found) {
		children.push_back(C) ;
		if(DEBUG_DDG) {
			DEBUG_OUT("%d is child of %d\n", C->seq, this->seq) ;
		}
	}
}

void ddg_node_t::printDDG()
{
	if(!isChildless()) {
		DEBUG_OUT("Children of %d: ", seq) ;
		for(ddg_node_list_t::iterator I=children.begin(), E=children.end();
				I!=E; I++) {
			ddg_node_t *child = *I ;
			DEBUG_OUT("%d, ", child->seq) ;
		}
		DEBUG_OUT(" Size = %d", getDDGSize()) ;
		DEBUG_OUT("\n") ;
		for(ddg_node_list_t::iterator I=children.begin(), E=children.end();
				I!=E; I++) {
			ddg_node_t *child = *I ;
			child->printDDG() ;
		}
	}
}

void ddg_node_t::printReverseDDG()
{
	if(!isOrphan()) {
		DEBUG_OUT("parents of %d: ", seq) ;
		for(ddg_node_list_t::iterator I=parents.begin(), E=parents.end();
				I!=E; I++) {
			ddg_node_t *parent = *I ;
			DEBUG_OUT("%d, ", parent->seq) ;
		}
		DEBUG_OUT("\n") ;
		for(ddg_node_list_t::iterator I=parents.begin(), E=parents.end();
				I!=E; I++) {
			ddg_node_t *parent = *I ;
			parent->printReverseDDG() ;
		}
	}
}

// Get the size of the subtree rooted at the given node.
uint64 ddg_node_t::getDDGSize()
{
	if(ddg_size != 0) {
		return ddg_size ;
	} 
	inst_set_t already_seen ;
	return getDDGSize(already_seen) ;
}

uint64 ddg_node_t::getDDGSize(inst_set_t &already_seen)
{
	ddg_size = 1 ;
	if(!isChildless()) {
		for(ddg_node_list_t::iterator I=children.begin(), E=children.end();
				I!=E; I++) {
			ddg_node_t *child = *I ;
			if(already_seen.find(child->seq)==already_seen.end()) {
				child->ddg_size = child->getDDGSize(already_seen) ;
				ddg_size += child->ddg_size ;
				already_seen.insert(child->seq) ;
			}
		}
	}
	return ddg_size ;
}

// TODO - The following implementation tries to do memorization of intermediate
// ddgsizes too, but it is complex as keeping track of just just mulitple-parent
// children is insufficient. If all parents are contained within a given node sub-tree
// that child is no longer special.
// uint64 ddg_node_t::getDDGSize()
// {
// 	if(DEBUG_DDG_SIZE) {
// 		DEBUG_OUT("getDDGSize(%d)\n", seq) ;
// 	}
// 	if(ddg_size == 0) { // Then compute it now
// 		ddg_size = 1 ;
// 		if(!isChildless()) {
// 			for(ddg_node_list_t::iterator I=children.begin(), E=children.end();
// 					I!=E; I++) {
// 				ddg_node_t *child = *I ;
// 				child->ddg_size = child->getDDGSize() ;
// 
// 				// Take information from the child, like size and special children.
// 				ddg_size += child->ddg_size ;
// 				ddg_node_list_t &child_sc = child->special_children ;
// 				for(ddg_node_list_t::iterator II=child_sc.begin(), EE=child_sc.end(); II!=EE; II++) {
// 					ddg_node_t *special_child = *II ;
// 					addSpecialChild(special_child) ;
// 				}
// 				if(child->getNumParents()>1) {
// 					// Repeats are ok here as I will take care of them when doing the subtraction
// 					addSpecialChild(child) ;
// 				}
// 			}
// 			// inst_set_t dependent_inst = getUniqueNodes() ;
// 			// ddg_size += dependent_inst.size() ;
// 		}
// 
// 		// Each node with multiple parents is counted as many times. Subtract them out
// 		DEBUG_OUT("Size[%d] before subtract = %d\n", seq, ddg_size) ;
// 		inst_set_t already_seen ;
// 		for(ddg_node_list_t::iterator I=special_children.begin(),
// 				E=special_children.end(); I!=E; I++) {
// 			ddg_node_t *sc = *I ;
// 			if(already_seen.find(sc->seq)==already_seen.end()) {
// 				// This means that this is the first time this node is seen. Do nothing
// 				// for this subtree, although this may be useful in the bigger subtree
// 				already_seen.insert(sc->seq) ;
// 			} else {
// 				// This node is repeating in this subtree. So subtrat.	
// 				ddg_size -= sc->ddg_size ;
// 			}
// 		}
// 	}
// 	if(DEBUG_DDG_SIZE) {
// 		DEBUG_OUT("special_children[%d] = ", seq) ;
// 		for(ddg_node_list_t::iterator II=special_children.begin(),
// 				EE=special_children.end(); II!=EE; II++) {
// 			ddg_node_t *node = *II ;
// 			DEBUG_OUT("%d, ", node->seq) ;
// 		}
// 		DEBUG_OUT("\n") ;
// 		DEBUG_OUT("Size[%d] = %d\n", seq, ddg_size) ;
// 	}
// 	return ddg_size ;
// }

void ddg_node_t::addSpecialChild(ddg_node_t *C)
{
	special_children.push_back(C) ;
}

// inst_set_t ddg_node_t::getUniqueNodes()
// {
// 	inst_set_t unique_nodes ;
// 	unique_nodes.clear() ;
// 	for(ddg_node_list_t::iterator I=children.begin(), E=children.end();
// 			I!=E; I++) {
// 		ddg_node_t *child = *I ;
// 		unique_nodes.insert(child->seq) ;
// 		inst_set_t child_unique_nodes = child->getUniqueNodes() ;
// 		for(inst_set_t::iterator CI=child_unique_nodes.begin(), CE=child_unique_nodes.end();
// 				CI!=CE; CI++) {
// 			uint64 c = *CI ;
// 			unique_nodes.insert(c) ;
// 		}
// 	}
// 	return unique_nodes;
// }

// Return the sequence number of the last instruction that reads this value
// Essentially the newest instruction in the DDG rooted @ this given node
uint64 ddg_node_t::getLastReadInst()
{
	if(last_reader != 0) {
		return last_reader ;
	}
	last_reader = seq ;
	// Find the youngest instruction in each child, and find youngest amongst them
	for(ddg_node_list_t::iterator I=children.begin(), E=children.end();
			I!=E; I++) {
		ddg_node_t *child = *I ;
		if(child->last_reader==0) {
			child->last_reader = child->getLastReadInst() ;
		}
		if(child->last_reader > last_reader ) {
			last_reader = child->last_reader ;
		}
	}
	return last_reader ;
}
