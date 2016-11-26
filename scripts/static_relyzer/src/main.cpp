#include <iostream>
#include <fstream>
#include <string>
#include "config.h"
#include "program.h"
#include <ctime>
#include <iomanip>

using namespace std;

int main(int argc, char* argv[]) {


    //////////////////////////////////////////////////////////////////////
    // INITIALIZATION 
    //////////////////////////////////////////////////////////////////////

    string output_file_name, input_file_name;
    if(argc == 2) {
        input_file_name = argv[1];
        output_file_name = "table.txt";
    } else if (argc == 3) {
        input_file_name = argv[1];
        output_file_name = argv[2];
    } else {
		cout << "Usage: relyzer-front-end <file_list.txt> optional:<output_summary.txt>\n";
		return -1;
	}

	time_t start, end;
	double dif;

	ifstream in_file(input_file_name.c_str(), ifstream::in);
	if(!in_file.good()) {
		cout << "Input file could not be opened : " << argv[1] << "\n";
		return -1;
	}

	ofstream out_file(output_file_name.c_str(), ofstream::out);


    //////////////////////////////////////////////////////////////////////
    // For each program name in the input file name, run static relyzer 
    //////////////////////////////////////////////////////////////////////
    
	while(in_file.good()) {
		string program_name = "", app_name = "";

		in_file >> program_name >> app_name;
		if(!in_file.good())
			break;

		cout << "\n" << program_name << ": " << app_name << endl;
		out_file <<  program_name << "\t";

		program_t program(program_name, app_name);

		// initiate the program by parsing the application binary (clean.dis)
		if(!program.populate_instructions(program_name, app_name))
			break; 

		program.open_exit_points_file();
		program.create_cfg();


        //////////////////////////////////////////////////////////////////
	    // mark instructions 
        //////////////////////////////////////////////////////////////////

    
        // set leading instructions for (1) store and (2) control
		program.mark_store_instructions();
		program.mark_control_instructions();

        // For debugging purposes for unoptimized applications (DEFUNCT)
		if(UNOPT_APPS) {
			//program.call_site_analysis();
		}

		// set leading instructions for (3) live 
		// sets live instructions as LIVE
		// if a reg is live and store
		//    its label changes from LIVE to LIVE_AND_STORE
		program.live_reg_analysis(); // constructs out sets for all bbs
		program.mark_unlive_registers(); //TODO change unlive -> non_live

		// set leading instructions for (4) other (call-site, illtraps, 
		// ret site, save site, dead, read state register, remaining, 
		// store_and_live)
		program.mark_other_instructions();

        // Collects stats on instruction types in the program
		program.get_instruction_distribution();

		// program.gen_control_patterns(); // DEFUNCT 


        //////////////////////////////////////////////////////////////////
	    // Performing program analysis and pruning, as well as counting
        // error sites.
        //////////////////////////////////////////////////////////////////

		time(&start);
        double original_count = 0.0; 
		double pruned_count = 0; 
	

        program.create_fault_set();

		
        program.prune_g0_faults();

        
        original_count = program.count_fault_set(true, USER_SELECTED_ANALYSIS_TYPE); // true for original 
		out_file <<  original_count << "\t";

		program.prune_addresses();
		pruned_count = program.count_fault_set(false, USER_SELECTED_ANALYSIS_TYPE); // false for pruned set
		out_file <<  pruned_count << "\t";

		// Not any more: We think it's not correct to do this because
		// faults in targets may take a jump to a valid instructions
		// (especially to shared library code). 
		// 
		// program.prune_branch_targets(); //DEFUNCT
		// pruned_count = program.count_fault_set(false, USER_SELECTED_ANALYSIS_TYPE); // false for pruned set
		// out_file << pruned_count << "\t";


        // TODO:Potentially useful but DEFUNCT
		// program.find_invariant_detectors(); 


		if(UNOPT_APPS) { // DEFUNCT. ALL OF IT!
			
// 			// apply this before def-use pruning
// 			program.prune_use_in_const_instr();
// 			pruned_count = program.count_fault_set(false, USER_SELECTED_ANALYSIS_TYPE); // false for pruned set
// 			out_file << pruned_count << "\t";
// 
			// program.prune_shift_consts();
			// pruned_count = program.count_fault_set(false, USER_SELECTED_ANALYSIS_TYPE); // false for pruned set
			// out_file << pruned_count << "\t";

			// program.prune_const_propagation();
			// pruned_count = program.count_fault_set(false, USER_SELECTED_ANALYSIS_TYPE); // false for pruned set
			// out_file << pruned_count << "\t";
		}

		//program.print_pruned_categories(); // DEFUNCT?

        //program.print_fault_set(false, USER_SELECTED_ANALYSIS_TYPE); // ABDULRAHMAN & RADHA

		program.prune_defs();
        //TODO: if you want, can combine next two lines...
		pruned_count = program.count_fault_set(false, USER_SELECTED_ANALYSIS_TYPE); 
		out_file <<  pruned_count << "\t";

        //////////////////////////////////////////////////////////////////
	    // Output fault set to a file, which will be used by dynamic
        // relyzer.
        //////////////////////////////////////////////////////////////////

		time(&end);
		dif = difftime(end,start);
		printf ("time = %.2lf sec\n", dif );


        program.print_fault_set(false, USER_SELECTED_ANALYSIS_TYPE);
		out_file << endl;
		cout << "done" << endl;
		program.close_exit_points_file();
	}

    //////////////////////////////////////////////////////////////////
	// End of static relyzer
    //////////////////////////////////////////////////////////////////

    in_file.close();
	out_file.close();
	cout << "done everything" << endl;

	return 0;
}

