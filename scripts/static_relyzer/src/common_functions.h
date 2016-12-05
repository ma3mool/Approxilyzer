/******************************************************************************
 *
 * 
 * 
 *
 * This file has common functions that are used throughout the static relyzer
 * code.
 *
 *
 *
 * ***************************************************************************/



#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H
#include <string>
#include <cctype>
#include "config.h"

std::string get_random_color() {
	std::string color = "";
	char colors[] = { '0','1', '2', '3', '4','5', '6', '7', '8', '9', 'a', 'b','c','d', 'e', 'f'} ; 
	for(int i=0; i<6; i++) {
		color += colors[rand()%16];
	}
	return color;
}

int get_stack_limit(std::string program_name) {
	if(program_name.compare("prime") == 0) 
		return 8;
	else if(program_name.compare("blackscholes_simlarge") == 0) 
		return STACK_BLACKSCHOLES_SIMLARGE;
	else if(program_name.compare("blackscholes_input_run_00000") == 0) 
		return STACK_BLACKSCHOLES_INPUT_RUN_00000;
	else if(program_name.compare("blackscholes_input_run_00001") == 0) 
		return STACK_BLACKSCHOLES_INPUT_RUN_00001;
	else if(program_name.compare("blackscholes_input_run_00002") == 0) 
		return STACK_BLACKSCHOLES_INPUT_RUN_00002;
	else if(program_name.compare("blackscholes_input_run_00003") == 0) 
		return STACK_BLACKSCHOLES_INPUT_RUN_00003;
	else if(program_name.compare("blackscholes_input_run_00004") == 0) 
		return STACK_BLACKSCHOLES_INPUT_RUN_00004;
	else if(program_name.compare("blackscholes_run_full") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_00000") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_00001") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_00002") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_00003") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_00004") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_21_a") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_21_c") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_5") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_5_rand") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_1k") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_1k_rand") == 0) 
		return STACK_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("fluidanimate") == 0) 
		return STACK_FLUIDANIMATE;
	else if(program_name.compare("streamcluster") == 0) 
		return STACK_STREAMCLUSTER;
	else if(program_name.compare("swaptions_simsmall") == 0) 
		return STACK_SWAPTIONS_SIMSMALL;
	else if(program_name.compare("swaptions_run_input") == 0) 
		return STACK_SWAPTIONS_SIMSMALL;
	else if(program_name.compare("fft_small") == 0) 
		return STACK_FFT_SMALL;
	else if(program_name.compare("fft_run_0") == 0) 
		return STACK_FFT_SMALL;
	else if(program_name.compare("fft_run_1") == 0) 
		return STACK_FFT_SMALL;
	else if(program_name.compare("fft_run_2") == 0) 
		return STACK_FFT_SMALL;
	else if(program_name.compare("fft_run_3") == 0) 
		return STACK_FFT_SMALL;
	else if(program_name.compare("fft_run_4") == 0) 
		return STACK_FFT_SMALL;
	else if(program_name.compare("fft_run_5") == 0) 
		return STACK_FFT_SMALL;
	else if(program_name.compare("fft_run_6") == 0) 
		return STACK_FFT_SMALL;
	else if(program_name.compare("lu_reduced") == 0) 
		return STACK_LU_REDUCED;
	else if(program_name.compare("lu_run_input_0") == 0) 
		return STACK_LU_REDUCED;
	else if(program_name.compare("lu_run_full") == 0) 
		return STACK_LU_REDUCED;
	else if(program_name.compare("ocean") == 0) 
		return STACK_OCEAN;
	else if(program_name.compare("water_small") == 0) 
		return STACK_WATER_SMALL;
	else if(program_name.compare("water_run_full") == 0) 
		return STACK_WATER_SMALL;
	else if(program_name.compare("water_run_min") == 0) 
		return STACK_WATER_SMALL;
	else if(program_name.compare("water_run_input_0") == 0) 
		return STACK_WATER_SMALL;
	else if(program_name.compare("water_run_input_1") == 0) 
		return STACK_WATER_SMALL;
	else if(program_name.compare("water_run_input_2") == 0) 
		return STACK_WATER_SMALL;
	else if(program_name.compare("water_run_input_3") == 0) 
		return STACK_WATER_SMALL;
	else if(program_name.compare("gcc") == 0) 
		return STACK_GCC;
	else if(program_name.compare("mcf") == 0) 
		return STACK_MCF;
	else if(program_name.compare("omnetpp") == 0) 
		return STACK_OMNETPP;
	else if(program_name.compare("libquantum") == 0) 
		return STACK_LIBQUANTUM;
	else 
		return 64;
}

int get_heap_limit(std::string program_name) {
	if(program_name.compare("prime") == 0) 
		return 23;
	else if(program_name.compare("blackscholes_simlarge") == 0) 
		return HEAP_BLACKSCHOLES_SIMLARGE;
	else if(program_name.compare("blackscholes_input_run_00000") == 0) 
        return HEAP_BLACKSCHOLES_INPUT_RUN_00000;
	else if(program_name.compare("blackscholes_input_run_00001") == 0) 
        return HEAP_BLACKSCHOLES_INPUT_RUN_00001;
	else if(program_name.compare("blackscholes_input_run_00002") == 0) 
        return HEAP_BLACKSCHOLES_INPUT_RUN_00002;
	else if(program_name.compare("blackscholes_input_run_00003") == 0) 
        return HEAP_BLACKSCHOLES_INPUT_RUN_00003;
	else if(program_name.compare("blackscholes_input_run_00004") == 0) 
        return HEAP_BLACKSCHOLES_INPUT_RUN_00004;
	else if(program_name.compare("blackscholes_run_00000") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_00001") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_00002") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_00003") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_00004") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_full") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_21_a") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_21_c") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_5") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_5_rand") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_1k") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("blackscholes_run_1k_rand") == 0) 
        return HEAP_BLACKSCHOLES_RUN_FULL;
	else if(program_name.compare("fluidanimate") == 0) 
		return HEAP_FLUIDANIMATE;
	else if(program_name.compare("streamcluster") == 0) 
		return HEAP_STREAMCLUSTER;
	else if(program_name.compare("swaptions_simsmall") == 0) 
		return HEAP_SWAPTIONS_SIMSMALL;
	else if(program_name.compare("swaptions_run_input") == 0) 
		return HEAP_SWAPTIONS_SIMSMALL;
	else if(program_name.compare("fft_small") == 0) 
		return HEAP_FFT_SMALL;
	else if(program_name.compare("fft_run_0") == 0) 
		return HEAP_FFT_SMALL;
	else if(program_name.compare("fft_run_1") == 0) 
		return HEAP_FFT_SMALL;
	else if(program_name.compare("fft_run_2") == 0) 
		return HEAP_FFT_SMALL;
	else if(program_name.compare("fft_run_3") == 0) 
		return HEAP_FFT_SMALL;
	else if(program_name.compare("fft_run_4") == 0) 
		return HEAP_FFT_SMALL;
	else if(program_name.compare("fft_run_5") == 0) 
		return HEAP_FFT_SMALL;
	else if(program_name.compare("fft_run_6") == 0) 
		return HEAP_FFT_SMALL;
	else if(program_name.compare("lu_reduced") == 0) 
		return HEAP_LU_REDUCED;
	else if(program_name.compare("lu_run_input_0") == 0) 
		return HEAP_LU_REDUCED;
	else if(program_name.compare("lu_run_full") == 0) 
		return HEAP_LU_REDUCED;
	else if(program_name.compare("ocean") == 0) 
		return HEAP_OCEAN;
	else if(program_name.compare("water_small") == 0) 
		return HEAP_WATER_SMALL;
	else if(program_name.compare("water_run_min") == 0) 
		return HEAP_WATER_SMALL;
	else if(program_name.compare("water_run_full") == 0) 
		return HEAP_WATER_SMALL;
	else if(program_name.compare("water_run_input_0") == 0) 
		return HEAP_WATER_SMALL;
	else if(program_name.compare("water_run_input_1") == 0) 
		return HEAP_WATER_SMALL;
	else if(program_name.compare("water_run_input_2") == 0) 
		return HEAP_WATER_SMALL;
	else if(program_name.compare("water_run_input_3") == 0) 
		return HEAP_WATER_SMALL;
	else if(program_name.compare("gcc") == 0) 
		return HEAP_GCC;
	else if(program_name.compare("mcf") == 0) 
		return HEAP_MCF;
	else if(program_name.compare("omnetpp") == 0) 
		return HEAP_OMNETPP;
	else if(program_name.compare("libquantum") == 0) 
		return HEAP_LIBQUANTUM;
	else 
		return 64;
}

int get_global_limit(std::string program_name) {
	return get_heap_limit(program_name);
}

int get_text_size(std::string program_name) {
	if(program_name.compare("prime") == 0)  {
		// size = 240*4 - num instructions*4, approx 10 bits
		return 10;
	} 
	else if(program_name.compare("lu") == 0) 
		return 15;
	else if(program_name.compare("lu_run_input_0") == 0) 
		return 15;
	else if(program_name.compare("lu_run_full") == 0) 
		return 15;
	else if(program_name.compare("fft") == 0) 
		return 15;
	else if(program_name.compare("fft_run_0") == 0) 
		return 15;
	else if(program_name.compare("fft_run_1") == 0) 
		return 15;
	else if(program_name.compare("fft_run_2") == 0) 
		return 15;
	else if(program_name.compare("fft_run_3") == 0) 
		return 15;
	else if(program_name.compare("fft_run_4") == 0) 
		return 15;
	else if(program_name.compare("fft_run_5") == 0) 
		return 15;
	else if(program_name.compare("fft_run_6") == 0) 
		return 15;
	else if(program_name.compare("swaptions") == 0) 
		return 15;
	else if(program_name.compare("swaptions_input_0") == 0) 
		return 15;
	else if(program_name.compare("blackscholes") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_21_a") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_21_c") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_5") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_5_rand") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_1k") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_1k_rand") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_full") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_00000") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_00001") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_00002") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_00003") == 0) 
		return 15;
	else if(program_name.compare("blackscholes_run_00004") == 0) 
		return 15;
	else if(program_name.compare("omnetpp") == 0) 
		return 20;
	else if(program_name.compare("gcc") == 0) 
		return 22;
	else if(program_name.compare("libquantum") == 0) 
		return 16;
	else if(program_name.compare("water_run_full") == 0) 
		return 30;
	else if(program_name.compare("water_run_min") == 0) 
		return 30;
	else if(program_name.compare("water_run_input_0") == 0) 
		return 30;
	else if(program_name.compare("water_run_input_1") == 0) 
		return 30;
	else if(program_name.compare("water_run_input_2") == 0) 
		return 30;
	else if(program_name.compare("water_run_input_3") == 0) 
		return 30;
	else 
		return 64;
}

bool is_number(std::string str) {
	bool return_val = true;
	for(int i=0; i<str.size(); i++) 
		if(!isdigit(str[i])) {
			return_val = false;
			break;
		}
	if(str.substr(0,2).compare("0x") == 0) 
		return true;
	return return_val;
}
#endif // COMMON_FUNCTIONS_H
