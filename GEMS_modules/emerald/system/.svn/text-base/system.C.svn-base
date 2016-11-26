#include "system.h"
#include <string>

#define DEBUG_EQUALIZATION 0


uint64 TEXT_START, TEXT_END;
uint64 RECORD_TEXT_START, RECORD_TEXT_END;
int HEAP, STACK;

using namespace std;
using namespace __gnu_cxx;

system_t::system_t() 
{
	min_stack = 0;
	max_stack = 0xFFFFFFFFFFFFFFFFULL;
	max_heap = 0;
	min_heap = 0x80100000000ULL;
	num_heap_accesses = 0;
	num_equalized = 0;
	pc_sum = 0;
	local_cycles = SIM_cycle_count(cpu_obj);
	time(&start);
	num_defs = 0;
	num_uses = 0;
	use_count_sum = 0;
	num_cycles = 0;
	def_use_chain_t* pool = NULL; 
	pool_index = -1;
	pc_list_index = 0;
	num_writes = 0;
	for(int i=0; i<NUM_PCS; i++) {
		pc_list[i] = 0;
	}
}

uint64 system_t::operate(conf_object_t *obj, conf_object_t *space, map_list_t *map, generic_transaction_t *g)
{
	// used this for debugging
	// uint64 current_pc = -1;
	// current_pc =  SIM_get_program_counter(cpu_obj);
	// uint64 local_cycle_num = SIM_cycle_count(cpu_obj);
	// if(local_cycle_num - num_cycles >= 100000000) {
	// 	num_cycles = local_cycle_num;
	// 	//	if(local_cycle_num >= 381855702490) 
	// 	printf( "0x%llx: %ld\n", current_pc, local_cycle_num);
	// 	// if(local_cycle_num >= 381855702500) 
	// 	//	exit(1);
	// }

	int is_physmem = !strcmp(space->name, PHYS_MEM_NAME);
 
#ifdef PROFILE_APP
	profileAddress(g, is_physmem);
#endif

#ifdef DEPENDENCE_ANALYSIS
	updateDefUseChains(g, is_physmem);
#endif

//	testCheckpointSize(g,is_physmem);
 
 	return 0 ;
}

// The initialization checkpoint. Start output buffering, input recording and checkpoint devices
void system_t::initialize()
{
}

#define WITH_LOADS 1
void system_t::testCheckpointSize(generic_transaction_t *g, int is_physmem) 
{
	cycles_t curr_cycle_count = SIM_cycle_count(cpu_obj);
	static int clear_count = 0;
	if(curr_cycle_count > num_cycles + 100000) {
		clear_count++;
		num_cycles = curr_cycle_count;
		cout << "SO:" << (int) store_orig_addresses.size() << "\t" ;
		cout << "S:" << (int) store_addresses.size() << endl;
		// cout << "L:" << (int) load_addresses.size() << endl;
		// printf  ("PC: %llx\n", SIM_get_program_counter(g->ini_ptr) );
		store_addresses.clear();
		store_orig_addresses.clear();
		//load_addresses.clear();
	}
	if (SIM_mem_op_is_from_cpu(g) && is_physmem) {

		uint64 addr = g->logical_address;
		//address_t addr = g->physical_address;
		pair<set<uint64>::iterator,bool> ret;

		if(SIM_mem_op_is_write(g)) { // WRITE

			if(load_addresses.find(addr) != load_addresses.end() && store_orig_addresses.find(addr) == store_orig_addresses.end()) {
				ret = store_addresses.insert(addr);	
				// if (ret.second) {
				// 	cout << "SA:" << addr << endl;
				// }
			}
			store_orig_addresses.insert(addr);
			load_addresses.erase(addr);

		} else { // READ
			ret = load_addresses.insert(addr);	
		}
	}
}

void system_t::profileAddress(generic_transaction_t *g, int is_physmem) 
{
	if (SIM_mem_op_is_from_cpu(g) && is_physmem) {
		if(SIM_processor_privilege_level(g->ini_ptr) == 0) {

			uint64 current_pc = SIM_get_program_counter(g->ini_ptr);
			//obtained by disassembling binary
			if(current_pc >= TEXT_START && current_pc <= TEXT_END) {

				// http://docs.sun.com/app/docs/doc/806-0477/6j9r2e2b9?a=view

				//Stack
				//read stack pointer
				int sp_reg_num = SIM_get_register_number(g->ini_ptr, "sp");
				uint64 stack_top = SIM_read_register(g->ini_ptr, sp_reg_num);
				stack_top += 2047*8;
				//read frame pointer
				int fp_reg_num = SIM_get_register_number(g->ini_ptr, "fp");
				uint64 frame_pointer = SIM_read_register(g->ini_ptr, fp_reg_num);

				bool changed = false;
				// stack grows downwards
				if(frame_pointer > min_stack ) {
					min_stack = frame_pointer;
					changed = true;
				}
				if(stack_top < max_stack )  {
					max_stack = stack_top;
					changed = true;
				}

				//Heap
				if(g->logical_address < 0x80100000000ULL && g->logical_address > 0x100000000ULL) { // heap address
					if(g->logical_address > max_heap )  {
						max_heap = g->logical_address;
						changed = true;
					}
					if(g->logical_address < min_heap )  {
						min_heap = g->logical_address;
						changed = true;
					}
					num_heap_accesses++;

					fprintf(m_outfp, "Stack top: 0x%llx, ", stack_top);
					fprintf(m_outfp, "Logical address: 0x%llx, ", g->logical_address);
					fprintf(m_outfp, "PC: 0x%llx\n", SIM_get_program_counter(g->ini_ptr));
				} 
				if(changed) {
					// cycle \t min stack \t max stack \t min heap \t max heap \n"
					fprintf(m_spprofilefp, "%lld\t%llx\t%llx\t%llx\t%llx\n", SIM_cycle_count(cpu_obj), min_stack, max_stack, min_heap, max_heap);
				}
			}
		}
	}
}

void system_t::updateDefUseChains(generic_transaction_t *g, int is_physmem)
{
	if (SIM_mem_op_is_from_cpu(g) && is_physmem) {

		uint64 current_pc = SIM_get_program_counter(g->ini_ptr);
		if(current_pc >= TEXT_START && current_pc <= TEXT_END) {
#ifdef PC_LIST_BEFORE
			pc_list[pc_list_index] = (current_pc & 0xFFFFF);
			pc_list_index = (pc_list_index+1)%NUM_PCS;
#endif
			// add(current_pc, g->logical_address, SIM_cycle_count(cpu_obj), SIM_mem_op_is_write(g)?WRITE:READ);
			cycles_t curr_cycle_count = SIM_cycle_count(cpu_obj);

			//obtained by disassembling binary
			if(HEAP) {
				//Heap and globals
				if(g->logical_address < 0x80100000000ULL && g->logical_address > TEXT_START) { // heap address
					add(current_pc, g->logical_address, curr_cycle_count, SIM_mem_op_is_write(g)?WRITE:READ);
				}
			}

			if(STACK) {
				if(g->logical_address > 0xFFFFF7FF00000000ULL) { // Non-heap address
					//Stack
					// stack grows downwards
					//fprintf(m_outfp, "\n logical address = 0x%llx\n", g->logical_address);
					add(current_pc, g->logical_address, curr_cycle_count, SIM_mem_op_is_write(g)?WRITE:READ);
				}
			}


			printHeartBeat(curr_cycle_count);
		}
	}
}

void system_t::printStats()
{
    fprintf(stdout, "Abdulrahman's Counters!\n");
    cycles_t end_cyc = SIM_cycle_count(cpu_obj) ;
	cycles_t total_cyc = end_cyc - start_cyc ;
	printf("server cycles\t%lld\n", total_cyc) ;
	fprintf(stdout, "\nMin Stack = 0x%llx, Max Stack = 0x%llx\n", min_stack, max_stack);
	fprintf(stdout, "Min Heap = 0x%llx, Max Heap = 0x%llx\n", min_heap, max_heap);
	fprintf(stdout, "Num heap accesses: 0x%llx\n", num_heap_accesses);
//m_outfp
}

void system_t::printHeapUseCounts()
{
	analyzeChains();
}

// bs = block size = 10,000 = for l2 size = 1MB;
// returns the end iterator
chain_t::iterator system_t::removeDuplicates(int bs, chains_t::iterator pc_it, chain_t::iterator start_it) {

	int temp_num_equalized = 0;
	chain_t::iterator end_it = start_it;
	while( (bs > 0) && (end_it != (*pc_it).second->end()) ) {
		end_it++;
		bs--;
	}

	chain_t::iterator vit;
	// order n^sqare algo to remove duplicates
	for(vit=start_it; vit != end_it; vit++) {
		chain_t::iterator vcit = vit;
		++vcit; // skip comparing same elements

		while(vcit != end_it) { 
			// find in pc_chains if similar patter is found
			if( (*vit)->is_chain_equal((*vcit)) ) {
				(*vit)->add_cycle( (*vcit)->get_cycle() );

				vcit = (*pc_it).second->erase(vcit);
				temp_num_equalized++;
			} else {
				++vcit;
			}
		}
	}

	//cout << " " << temp_num_equalized;
	//fflush(stdout);

	return end_it;
}

void system_t::analyzeChains()
{
	printf("Analyzing...\n");
	fflush(stdout);
	chains_t::iterator it;

	time(&start);

// #define MCF
#ifdef MCF

		for(it=address_chains.begin(); it != address_chains.end(); it++) { // for each pc_chain
			(*it).second->back()->set_def_done();
		}
		printf("set done \n");

		// for each pc_chains element, i.e., vector of def_use_chains
		for(it=pc_chains.begin(); it != pc_chains.end(); it++) {

			printf("\nPC: 0x%llx: %d", (*it).first, (*it).second->size());
			fflush(stdout);
			chain_t::iterator vit;

			map<string, def_use_chain_t*>  pattern_map;

			for(vit=(*it).second->begin(); vit != (*it).second->end(); vit++) {
				string key_str = (*vit)->to_string_key();
				map<string, def_use_chain_t*>::iterator pit = pattern_map.find(key_str);
				if(pit != pattern_map.end()) {
				 	(*pit).second->add_cycle( (*vit)->get_cycle() );
				} else {
				 	pattern_map[key_str] = (*vit);
				}
			}

			fprintf(m_injfp, "\n0x%llx: ", (*it).first);

			map<string, def_use_chain_t*>::iterator pit ;

			for(pit=pattern_map.begin(); pit != pattern_map.end(); pit++) {
				fprintf(m_injfp, "%lld ", (*pit).second->get_cycle());

				//printing equivalence classes
				fprintf(m_classfp, "\n0x%llx: ", (*it).first); // pc
				fprintf(m_classfp, "%lld: ", (*pit).second->get_cycle()); //pilot
				fprintf(m_classfp, "%lld: ", (*pit).second->get_population()); //population
				(*pit).second->print_equalized_class(m_classfp);
			}
		}

#else // MCF

	if(MEM_OPTIMIZATION) {
		for(it=address_chains.begin(); it != address_chains.end(); it++) { // for each pc_chain
			(*it).second->back()->set_def_done();

			removeFromPcChains(it);

#if 0
#ifndef USE_LIST
			// def_use_chain of last element in address_chain
			def_use_chain_t * last_element = (*it).second->back();
			bool please_erase = false;
			bool stop_checking = false;
		
			// PC of the last element in the def_use_chain
			uint64 last_element_pc = last_element->get_pc();
			chain_t *pc_chain_elem_ptr = (pc_chains[last_element_pc]);
			//chain_t pc_chain_elem = pc_chains[last_element_pc];
	
			int i, pc_chain_remove_elem_index = 0;
			int vector_size = pc_chain_elem_ptr->size();
			for(i=0; i<vector_size; i++ ) { 
				if(pc_chain_elem_ptr->at(i) != last_element) {
					pc_chain_elem_ptr->at(i)->print_def_use_chain();
					if(!stop_checking)
						if(pc_chain_elem_ptr->at(i)->is_chain_equal(last_element)) {
							please_erase = true;
							// add cycle of last_element to pc_chain_elem_ptr->at(i) to keep track of the equivalence class
							pc_chain_elem_ptr->at(i)->add_cycle(last_element->get_cycle());
							stop_checking = true;
							// there is no break here because we want to obtain the pc_chain_remove_elem_index
						} 
				} else {
					pc_chain_remove_elem_index = i;
				}
			}
			if(please_erase) {
				//remove last_element from address_chains
				(*it).second->pop_back();
				//remove last_element from pc_chains
				pc_chain_elem_ptr->erase(pc_chain_elem_ptr->begin() + pc_chain_remove_elem_index);
	
				//free mem
				delete last_element;
				num_equalized++;
	
				printf(".");
				fflush(stdout);
			}
#else // if defined USE_LIST
		cout << " Please use USE_LIST\n";
		exit(1);
#endif // USE_LIST
#endif
		}
	

	} else {

		for(it=address_chains.begin(); it != address_chains.end(); it++) { // for each pc_chain
			(*it).second->back()->set_def_done();
		}
		printf("set done \n");

		chains_t  new_pc_chains;

		// for each pc_chains element, i.e., vector of def_use_chains
		for(it=pc_chains.begin(); it != pc_chains.end(); it++) {

			printf("\nPC: 0x%llx: %d", (*it).first, (*it).second->size());
			fflush(stdout);
			chain_t::iterator vit;

			// // cache optimization - utilize locality - still not helpful
			// int bs = 100;
			// int num_iter = 8;
			// // optimization utilizing locality
			// for(int i=0; i<num_iter; i++) { 
			// 	for(int j=0; j<num_iter-i; j++) { 
			// 		chain_t::iterator temp_it = removeDuplicates(bs, it, (*it).second->begin());
			// 		while(temp_it != (*it).second->end())  {
			// 			temp_it = removeDuplicates(bs, it, ++temp_it);
			// 		}
			// 		printf("\nPC: 0x%llx: %d", (*it).first, (*it).second->size());
			// 		cout << "bs=" << bs << " done" << endl;
			// 	}
			// 	bs *= 2;
			// }

			//printf("done with one pass\n"); 
			//fflush(stdout);
			// printf("\nPC: 0x%llx: %d\n", (*it).first, (*it).second->size());
			//fflush(stdout);

			// search for duplicate elements and erase them - O(n^2) algo
			// with cache optimization - for every 5000 elements compare every other
			// element for dupliate elements and continue in chunks of 5000
			// for(vit=(*it).second->begin(); vit != (*it).second->end(); vit++) {
			// 	// we know that first 1000 are unique 
			// 	// collect them in to a list

			// 	// compare start it
			// 	chain_t::iterator csit = vit;
			// 	int num_erased = 0;

			// 	// skip forward 'bs' elements
			// 	int num_collect = bs;
			// 	while(num_collect != 0 && vit != (*it).second->end()) {
			// 		num_collect--;
			// 		vit++;
			// 	}

			// 	if(vit == (*it).second->end()) {
			// 		break;
			// 	}

			// 	chain_t::iterator vcit = vit;
			// 	vcit++; // skip comparing same elements
		
			// 	uint64 num_iterations = 0;
			// 	// from 'bs'th element to end
			// 	while(vcit != (*it).second->end()) { 
			// 		num_iterations++;
			// 		// find in pc_chains if similar patter is found
			// 		bool remove_flag = false;
			// 		chain_t::iterator cit = csit;
			// 		for(int i=0; i<bs; i++,cit++) {
 			// 			if( (*cit)->is_chain_equal((*vcit)) ) {
			// 				(*cit)->add_cycle( (*vcit)->get_cycle() );
			// 				remove_flag = true;
			// 				break;
			// 			}
			// 		}
			// 		if(remove_flag) { 
			// 			vcit = (*it).second->erase(vcit);
			// 			num_equalized++;
			// 			num_erased++;
			// 		} else {
			// 			vcit++;
			// 		}
			// 	}

			// 	//cout << " num_iterations: " << num_iterations << " num_erased: " << num_erased << endl;
			// }


			// int temp_count = 0;
			// // search for duplicate elements and erase them - O(n^2) algo
			// for(vit=(*it).second->begin(); vit != (*it).second->end(); vit++) {
			// 	chain_t::iterator vcit = vit;
			// 	++vcit; // skip comparing same elements

			// 	temp_count++;
			// 	if(temp_count %1000 == 0) 
			// 		printf("\nPC: 0x%llx: %d", (*it).first, (*it).second->size());

			// 	while(vcit != (*it).second->end()) { 
			// 		// find in pc_chains if similar patter is found
 			// 		if( (*vit)->is_chain_equal((*vcit)) ) {
			// 			(*vit)->add_cycle( (*vcit)->get_cycle() );
			// 			vcit = (*it).second->erase(vcit);
			// 			num_equalized++;
			// 		} else {
			// 			++vcit;
			// 		}
			// 	}
			// }
		}
	}

	time(&end);
    double dif;
	dif = difftime(end,start);
	printf ("time = %.4lf sec\n", dif );

	fprintf (m_outfp, "\nAnalyzing pc chains\n");
	fprintf (m_outfp, "Num equalized: %lld \n", num_equalized);
	fprintf (m_outfp, "Num unique PCs: %d \n", pc_chains.size());
	chains_t::iterator cit;
	// for each pc_chains element, i.e., vector of def_use_chains
	for(cit=pc_chains.begin(); cit != pc_chains.end(); cit++) {
		// cit points to a vector of def_use_chain
		chain_t::iterator vit;

		// used to collect some stats about how many are qualized, and how many are not, etc.
		// map<int,int> bins1; // <num_uses, how_many>
		// map<uint64,int,ltuint64> bins2; // <use_pc, how_many>

		fprintf(m_injfp, "\n0x%llx: ", (*cit).first);

		// for each def_use_chain 
		for(vit=(*cit).second->begin(); vit != (*cit).second->end(); vit++) {
			// vit points to a def_use_chain

			// if(bins1.find((*vit)->get_use_count()) != bins1.end())
			// 	bins1[(*vit)->get_use_count()]++; 
			// else 
			// 	bins1[(*vit)->get_use_count()] = 1;

			// if(bins2.find((*vit)->get_first_use_pc()) != bins2.end())
			// 	bins2[(*vit)->get_first_use_pc()]++; 
			// else 
			// 	bins2[(*vit)->get_first_use_pc()] = 1;

			//Print all unique patterns
			// (*vit)->print_def_use_chain();
			fprintf(m_injfp, "%lld ", (*vit)->get_cycle());

			//printing equivalence classes
			fprintf(m_classfp, "\n0x%llx: ", (*cit).first); // pc
			if((*cit).first != (*vit)->get_pc()) {
				cout <<" pcs dont match!! - "<< (*cit).first << ":" << (*vit)->get_pc();
			}
			fprintf(m_classfp, "%lld: ", (*vit)->get_cycle()); //pilot
			fprintf(m_classfp, "%lld: ", (*vit)->get_population()); //population
			(*vit)->print_equalized_class(m_classfp);
		}
		fprintf(m_outfp, "\n0x%llx:", (*cit).first);
		// fprintf(m_outfp, "num_counts_cats: %d ", bins1.size());
		// fprintf(m_outfp, "num_first_use_pc_cats: %d\n", bins2.size());
		//Print all unique patterns
		//if(bins1.size() != bins2.size()) {
		//	for(vit=(*cit).second.begin(); vit != (*cit).second.end(); vit++) {
		//		(*vit)->print_def_use_chain();
		//	}
		//}
	}
	time(&end);
	dif = difftime(end,start);
	printf ("time = %.4lf sec\n", dif );
#endif // MCF

}

void system_t::removeFromPcChains(chains_t::iterator it) {
	// def_use_chain of last element in address_chain
	def_use_chain_t * last_element = (*it).second->back();
	bool please_erase = false;
	bool stop_checking = false;

	// PC of the last element in the def_use_chain
	uint64 last_element_pc = last_element->get_pc();
	chain_t *pc_chain_elem_ptr = (pc_chains[last_element_pc]);
	//chain_t pc_chain_elem = pc_chains[last_element_pc];

	int i, pc_chain_remove_elem_index = 0;

#ifdef USE_LIST
	chain_t::iterator erase_it; 
	// search for last_element in pc_chain_elem
	chain_t::iterator vit;
	for(vit=pc_chain_elem_ptr->begin(); vit != pc_chain_elem_ptr->end(); vit++) {
		if((*vit) != last_element) {
			if(!stop_checking) {
				if((*vit)->is_chain_equal(last_element)) {
					please_erase = true;
					// add cycle of last_element to pc_chain_elem_ptr->at(i) to keep track of the equivalence class
					(*vit)->add_cycle(last_element->get_cycle());
					stop_checking = true;
					// there is no break here because we want to obtain the pc_chain_remove_elem_index
				} else {
					//(*vit)->print_def_use_chain(); 
				}
			}
		} else {
			erase_it = vit;
		}
	}
	if(please_erase) {
		//remove last_element from address_chains
		(*it).second->pop_back();
		//remove last_element from pc_chains
		pc_chain_elem_ptr->erase(erase_it);

		//free mem
		delete last_element;
		num_equalized++;

		// printf(".");
		// fflush(stdout);
	}

#else // USE_LIST

	int vector_size = pc_chain_elem_ptr->size();
	for(i=0; i<vector_size; i++ ) { 
		if(pc_chain_elem_ptr->at(i) != last_element) {
			if(!stop_checking)
				if(pc_chain_elem_ptr->at(i)->is_chain_equal(last_element)) {
					please_erase = true;
					// add cycle of last_element to pc_chain_elem_ptr->at(i) to keep track of the equivalence class
					pc_chain_elem_ptr->at(i)->add_cycle(last_element->get_cycle());
					stop_checking = true;
					// there is no break here because we want to obtain the pc_chain_remove_elem_index
				} 
		} else {
			pc_chain_remove_elem_index = i;
		}
	}
	if(please_erase) {
		//remove last_element from address_chains
		(*it).second->pop_back();
		//remove last_element from pc_chains
		pc_chain_elem_ptr->erase(pc_chain_elem_ptr->begin() + pc_chain_remove_elem_index);

		//free mem
		delete last_element;
		num_equalized++;

		// printf(".");
		// fflush(stdout);
	}
#endif // USE_LIST
}


void system_t::add(uint64 current_pc, uint64 logical_address, uint64 cycle_count, int type)
{
	if(MEM_OPTIMIZATION) {
#ifdef USE_LIST
		// cout << " Please turn off use USE_LIST in system/system.C\n";
		// exit(1);
#endif // USE_LIST
	}
	if (type == WRITE && !(current_pc >= RECORD_TEXT_START && current_pc <= RECORD_TEXT_END)) {
		return;
	}
	chains_t::iterator it, cit;

#if 0
	if(type == WRITE && 0) {
		//for debugging
		//if(address_chains.size() % 10000 == 0) {
		//	printf("current map size = %d\n", address_chains.size());
		//}
		num_defs++;
		it = address_chains.find(logical_address);

		//this can reduce memory footprint of the simulator, this will increase computation
		//For the same address, check the last def with others in the pc_chains to compress it. 
		//if found, remove it from the address_chains.
		if(it != address_chains.end()) {
			(*it).second->back()->set_def_done();
			def_use_chain_t * temp_element = (*it).second->back();
			chain_t::iterator vit;
			bool please_erase = false;
			chain_t *pc_chain_elem = (pc_chains[temp_element->get_pc()]);
			if(DEBUG_EQUALIZATION) 
				fprintf(m_outfp, "size of pc_chain: %lld\n", pc_chain_elem->size());
			for(vit=pc_chain_elem->begin(); vit != pc_chain_elem->end(); vit++) {
				if((*vit) != temp_element) {
					// check if the def_use chain is equal to any one that previously exists
					if((*vit)->is_chain_equal(temp_element)) {
						if(DEBUG_EQUALIZATION) {
							fprintf(m_outfp, "\nNext 2 uses are equal for PC:%llx", temp_element->get_pc());
							(*vit)->print_def_use_chain();
							temp_element->print_def_use_chain();
						}
						please_erase = true;
						break;
					} 
				}
			}
			if(please_erase) {
				for(vit=pc_chain_elem->begin(); vit != pc_chain_elem->end(); vit++) {
					if((*vit) == temp_element) {
						//printf(".");
						//fflush(stdout);
						//remove from address_chains
						(*it).second->pop_back();
						//remove from pc_chains
						pc_chain_elem->erase(vit);

						//free mem
						delete temp_element;
						num_equalized++;
						if(pc_chain_elem->size() == 0)  
							printf("size of pc_chain after removing: %lld\n", pc_chain_elem->size());
						break;
					}
				}
			}
		}
	}
#endif

	 
	it = address_chains.find(logical_address);
	cit = it;

	if (MEM_OPTIMIZATION)  {
		// optimization to reduce the memory footprint 
		if(type == WRITE) {
			//For the same address, check the last def with others in the pc_chains to compress it. 
			//if found, remove it from the address_chains.
			if(it != address_chains.end()) {
				(*it).second->back()->set_def_done();

				removeFromPcChains(it);
#if 0
				// def_use_chain of last element in address_chain
				def_use_chain_t * last_element = (*it).second->back();
				bool please_erase = false;
				bool stop_checking = false;
		
				// PC of the last element in the def_use_chain
				uint64 last_element_pc = last_element->get_pc();
				chain_t *pc_chain_elem_ptr = (pc_chains[last_element_pc]);
				//chain_t pc_chain_elem = pc_chains[last_element_pc];
	
				int i, pc_chain_remove_elem_index = 0;

#ifdef USE_LIST
				chain_t::iterator erase_it; 
				// search for last_element in pc_chain_elem
				chain_t::iterator vit;
				for(vit=pc_chain_elem_ptr->begin(); vit != pc_chain_elem_ptr->end(); vit++) {
					if((*vit) != last_element) {
						if(!stop_checking) {
							if((*vit)->is_chain_equal(last_element)) {
								please_erase = true;
								// add cycle of last_element to pc_chain_elem_ptr->at(i) to keep track of the equivalence class
								(*vit)->add_cycle(last_element->get_cycle());
								stop_checking = true;
								// there is no break here because we want to obtain the pc_chain_remove_elem_index
							} else {
								//(*vit)->print_def_use_chain(); 
							}
						}
					} else {
						erase_it = vit;
					}
				}
				if(please_erase) {
					//remove last_element from address_chains
					(*it).second->pop_back();
					//remove last_element from pc_chains
					pc_chain_elem_ptr->erase(erase_it);
	
					//free mem
					delete last_element;
					num_equalized++;
	
					// printf(".");
					// fflush(stdout);
				}

#else // USE_LIST

				int vector_size = pc_chain_elem_ptr->size();
				for(i=0; i<vector_size; i++ ) { 
					if(pc_chain_elem_ptr->at(i) != last_element) {
						if(!stop_checking)
							if(pc_chain_elem_ptr->at(i)->is_chain_equal(last_element)) {
								please_erase = true;
								// add cycle of last_element to pc_chain_elem_ptr->at(i) to keep track of the equivalence class
								pc_chain_elem_ptr->at(i)->add_cycle(last_element->get_cycle());
								stop_checking = true;
								// there is no break here because we want to obtain the pc_chain_remove_elem_index
							} 
					} else {
						pc_chain_remove_elem_index = i;
					}
				}
				if(please_erase) {
					//remove last_element from address_chains
					(*it).second->pop_back();
					//remove last_element from pc_chains
					pc_chain_elem_ptr->erase(pc_chain_elem_ptr->begin() + pc_chain_remove_elem_index);
	
					//free mem
					delete last_element;
					num_equalized++;
	
					// printf(".");
					// fflush(stdout);
				}
#endif // USE_LIST
#endif
			}
		}
	}

	it = cit; 

	if(type == WRITE) {

		num_defs++;
		num_writes++;
		def_use_chain_t * new_element = new def_use_chain_t(current_pc, logical_address, cycle_count);
		// def_use_chain_t * new_element = create_def_use_chain_elem();
		// new_element->init(current_pc, logical_address, cycle_count);

		//insert in to address_chains
		if(it != address_chains.end()) {
			if( !(*it).second->empty() ) { // important
				use_count_sum  += (*it).second->back()->get_use_count(); 
				(*it).second->back()->set_def_done();
			}
			(*it).second->push_back(new_element);
		} else {
			address_chains[logical_address] = new chain_t(); 
			address_chains[logical_address]->push_back(new_element);
		}

		//update pc_chains
		chains_t::iterator pc_it = pc_chains.find(current_pc);
		if(pc_it != pc_chains.end())
			(*pc_it).second->push_back(new_element);
		else {
			pc_chains[current_pc] = new chain_t(); 
			pc_chains[current_pc]->push_back(new_element);
		}

	} else {
		if(it != address_chains.end())  {
			if((*it).second->empty())  {
				cout << "size cannot be zero \n";
				exit(1);
			}
			if( ! (*it).second->back()->is_def_done() ) {
				int num_loads = -1;

#ifdef PC_LIST_BEFORE

#ifdef USE_HASH
				num_loads = (*it).second->back()->add_use(current_pc, cycle_count, get_pc_hash());
#else  // USE_HASH
				num_loads = (*it).second->back()->add_use(current_pc, cycle_count, pc_list);
#endif // USE_HASH

#else // PC_LIST_BEFORE
				num_loads = (*it).second->back()->add_use(current_pc, cycle_count);
#endif //PC_LIST_BEFORE

#ifdef LIMIT_NUM_LOADS
				if(num_loads >= MAX_TRACKING_LOADS)  {
					if(!MEM_OPTIMIZATION) {
						// cout << " Please use MEM_OPTIMIZATION\n";
						// exit(1);
					} else {
						removeFromPcChains(it);
					}
#if 0
#ifdef USE_LIST
					cout << " Please do not use USE_LIST\n";
					exit(1);
#else // USE_LIST
					(*it).second->back()->set_def_done();

					// def_use_chain of last element in address_chain
					def_use_chain_t * last_element = (*it).second->back();
					bool please_erase = false;
					bool stop_checking = false;
			
					// PC of the last element in the def_use_chain
					uint64 last_element_pc = last_element->get_pc();
					chain_t *pc_chain_elem_ptr = (pc_chains[last_element_pc]);
					//chain_t pc_chain_elem = pc_chains[last_element_pc];
		
					int i, pc_chain_remove_elem_index = 0;


					int vector_size = pc_chain_elem_ptr->size();
					for(i=0; i<vector_size; i++ ) { 
						if(pc_chain_elem_ptr->at(i) != last_element) {
							if(!stop_checking)
								if(pc_chain_elem_ptr->at(i)->is_chain_equal(last_element)) {
									please_erase = true;
									// add cycle of last_element to pc_chain_elem_ptr->at(i) to keep track of the equivalence class
									pc_chain_elem_ptr->at(i)->add_cycle(last_element->get_cycle());
									stop_checking = true;
									// there is no break here because we want to obtain the pc_chain_remove_elem_index
								} 
						} else {
							pc_chain_remove_elem_index = i;
						}
					}
					if(please_erase) {
						//remove last_element from address_chains
						(*it).second->pop_back();
						//remove last_element from pc_chains
						pc_chain_elem_ptr->erase(pc_chain_elem_ptr->begin() + pc_chain_remove_elem_index);
		
						//free mem
						delete last_element;
						num_equalized++;
		
						// printf(".");
						// fflush(stdout);
					}
#endif // USE_LIST
#endif
				}
#endif // LIMIT_NUM_LOADS
				num_uses++;
			}
		} else {
			//TODO: understand and debug this case
			//cout << "Found a USE before DEF!\n";
			//exit(1);
		}
	}
}

#define POOL_SIZE 50000
// optimized memory allocator - reduces the number of calls to 'new' by POOL_SIZE times
def_use_chain_t * system_t::create_def_use_chain_elem() {

	def_use_chain_t * ret_ptr; 
	if(pool_index < 0) {
		pool = new def_use_chain_t[POOL_SIZE];
		pool_index = POOL_SIZE-1;
	}
	ret_ptr = &(pool[pool_index]);
	pool_index--;
	return ret_ptr;
}

void system_t::printHeartBeat(cycles_t current_cyc)
{
	cycles_t total_cyc = current_cyc - local_cycles;
	if(total_cyc > 10000000 )  {
		// printf("cycles %lld\n", current_cyc) ;
		local_cycles = current_cyc;

		time(&end);	
		double dif = difftime(end,start);
		time(&start);
		printf ("time = %.4lf sec\n", dif );
		printf ("size of address_chains = %d\n", address_chains.size());

		printf ("size of pc_chains = %d\n", pc_chains.size());
		chains_t::iterator it;
		for(it=pc_chains.begin(); it != pc_chains.end(); it++) {
			printf("\tPC: 0x%llx: %d\n", (*it).first, (*it).second->size());
		}
		cout << endl;
		// printf("num uses: %d\n", num_uses);
		// printf("num defs: %d\n", num_defs);
		// if(num_uses >0) 
		// 	printf("average use_count size = : %d\n", use_count_sum/num_defs);
		num_uses = 0;
		num_defs = 0;
		use_count_sum = 0;
		// cout << "size of def_use_chain_t = " << sizeof(def_use_chain_t) << endl;

		fflush(stdout);
	}
}

void system_t::setAddressSpace(int space) {
	HEAP = 0;
	STACK = 0;
	if(space == 0)
		HEAP = 1;
	if(space == 1)
		STACK = 1;
}

void system_t::textStart(uint64 start)
{
	TEXT_START = start;
	RECORD_TEXT_START = start;
	printf("Text Start = %lld\n", TEXT_START);
}

void system_t::textEnd(uint64 end)
{
	TEXT_END = end;
	RECORD_TEXT_END = end;
	printf("Text End = %lld\n", TEXT_END);
}

void system_t::recordTextStart(uint64 start)
{
	RECORD_TEXT_START = start;
	printf("Record Text Start = %lld\n", RECORD_TEXT_START);
}

void system_t::recordTextEnd(uint64 end)
{
	RECORD_TEXT_END = end;
	printf("Record Text End = %lld\n", RECORD_TEXT_END);
}


#ifdef PC_LIST_BEFORE
#ifdef USE_HASH
uint64 system_t::get_pc_hash()
{
	uint64 local_pc_hash = 0;
	for(int i=0; i<NUM_PCS; i++) {
		local_pc_hash += (i+1)*pc_list[i];
	}
	return local_pc_hash;
}
#endif // USE_HASH
#endif // PC_LIST_BEFORE
