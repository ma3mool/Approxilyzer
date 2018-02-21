/******************************************************************************
 *
 * 
 * 
 *
 * This is a configuration file used to set parameters for static relyzer.
 *
 *
 *
 * ***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H


/* use enum */
/* might want to change order */
/* brief explaination of what these groups are */
#define ALL_FAULTS 0
#define ONLY_REGISTER_FAULTS 1
#define ONLY_INT_REGISTER_FAULTS 2
#define ONLY_INT_AGEN_FAULTS 3
#define ONLY_FLOATING_POINT_FAULTS 4
#define G0_PRUNED_OUTPUT 5

/* USER ATTENTION: select which type of analysis to include */
#define USER_SELECTED_ANALYSIS_TYPE ONLY_REGISTER_FAULTS
//#define USER_SELECTED_ANALYSIS_TYPE G0_PRUNED_OUTPUT 
#define INSTR_DIST_FILE "distr.txt"
#define FAULT_LIST_DIR "fault_list_output/"

#define NUM_OPS 3
#define DEBUG_PARSER 0   // from program.cpp
#define DEBUG 0          // from program.cpp
#define BRANCH_FLATTENING

#define HEAP_BLACKSCHOLES_SIMLARGE 	23
#define HEAP_BLACKSCHOLES_INPUT_RUN_00000 	23
#define HEAP_BLACKSCHOLES_INPUT_RUN_00001 	23
#define HEAP_BLACKSCHOLES_INPUT_RUN_00002 	23
#define HEAP_BLACKSCHOLES_INPUT_RUN_00003 	23
#define HEAP_BLACKSCHOLES_INPUT_RUN_00004 	23
#define HEAP_BLACKSCHOLES_RUN_FULL 23
#define HEAP_FLUIDANIMATE	44
#define HEAP_SOBEL	21
#define HEAP_STREAMCLUSTER	22
#define HEAP_SWAPTIONS_SIMSMALL		21
#define HEAP_FFT_SMALL			22
#define HEAP_LU_REDUCED				22
#define HEAP_OCEAN			24
#define HEAP_WATER_SMALL			23
#define HEAP_WATER_RUN_MIN0			23
#define HEAP_GCC			25
#define HEAP_MCF			28
#define HEAP_LIBQUANTUM		21
#define HEAP_OMNETPP		24
#define HEAP_BLACKSCHOLES_RUN_21_B	23
#define HEAP_BLACKSCHOLES_RUN_32	23
#define HEAP_BLACKSCHOLES_RUN_64	23
#define HEAP_BLACKSCHOLES_RUN_128	23
#define HEAP_BLACKSCHOLES_RUN_256	23


#define STACK_BLACKSCHOLES_SIMLARGE 	14			
#define STACK_BLACKSCHOLES_INPUT_RUN_00000 	14			
#define STACK_BLACKSCHOLES_INPUT_RUN_00001 	14			
#define STACK_BLACKSCHOLES_INPUT_RUN_00002 	14			
#define STACK_BLACKSCHOLES_INPUT_RUN_00003 	14			
#define STACK_BLACKSCHOLES_INPUT_RUN_00004 	14			
#define STACK_BLACKSCHOLES_RUN_FULL 14
#define STACK_FLUIDANIMATE	14			
#define STACK_SOBEL	14			
#define STACK_STREAMCLUSTER	14			
#define STACK_SWAPTIONS_SIMSMALL		14		
#define STACK_FFT_SMALL			14	
#define STACK_LU_REDUCED			14	
#define STACK_OCEAN			14	
#define STACK_WATER_SMALL			14	
#define STACK_WATER_RUN_MIN0			14	
#define STACK_GCC			15	
#define STACK_MCF			32	
#define STACK_LIBQUANTUM	14			
#define STACK_OMNETPP		24		
#define STACK_BLACKSCHOLES_RUN_21_B	14
#define STACK_BLACKSCHOLES_RUN_32	14
#define STACK_BLACKSCHOLES_RUN_64	14
#define STACK_BLACKSCHOLES_RUN_128	14
#define STACK_BLACKSCHOLES_RUN_256	14







#endif // CONFIG_H
