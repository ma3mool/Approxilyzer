/**********************************************************************
 * C source implementing hand-shaking between functional and RTL/gate 
 * level simulators
 *
 * Communication over two named pipes, which are supposed to be generated
 * in advance  
 ***********************************************************************/

#include <stdlib.h>    /* ANSI C standard library */
#include <stdio.h>     /* ANSI C standard input/output library */
#include <malloc.h>

/* IPC support */
#include <unistd.h>
#include <sys/types.h>	
#include <sys/stat.h>	
#include <fcntl.h>

#include "vpi_user.h" 
#include "vpi_user_cds.h"

#define REG_WIDTH 65

#define VERY_VERBOSE 0

void readStimuliRegister();
void writeLatchRegister();

PLI_INT32 openReadPipeStartOfSim(p_cb_data cb_data);
PLI_INT32 openWritePipeStartOfSim(p_cb_data cb_data);

PLI_INT32 readStimuli_calltf(PLI_BYTE8 *user_data);
PLI_INT32 writeLatch_calltf(PLI_BYTE8 *user_data);

PLI_INT32 readStimuli_compiletf(PLI_BYTE8 *user_data);
PLI_INT32 writeLatch_compiletf(PLI_BYTE8 *user_data);

// Generally this fields are to be set when migrating from structure to structure
// the interface - inputs and outputs of the infected structure - is defined here:

//---------------------------- task specific work areas
// all data needed to generate circuit outputs to be put here:
 typedef struct readStim {
	vpiHandle se;
	vpiHandle si;
   
	vpiHandle byp_alu_rs1_data_e;   // source operand 1, 64 bit
	vpiHandle byp_alu_rs2_data_e;  // source operand 2, 64 bit
	vpiHandle ecl_alu_cin_e;            // cin for adder
  
	 vpiHandle byp_alu_rs3_data_e;  // source operand 3, 64 bit  
	vpiHandle byp_alu_rcc_data_e;  // source operand for reg condition codes, 64 bit  
	vpiHandle ifu_exu_invert_d; // this field indicates whether op2 should be complemented [applies only for logical operations]
   
	vpiHandle  ecl_alu_log_sel_and_e;	// These 4 vpiHandles are select lines for the logic
	vpiHandle  ecl_alu_log_sel_or_e;	// block mux.  They are active high and choose the
	vpiHandle  ecl_alu_log_sel_xor_e;	// output they describe.   
	vpiHandle  ecl_alu_log_sel_move_e;   
   
	vpiHandle  ecl_alu_out_sel_sum_e_l;	// The following 4 are select lines for 
	vpiHandle  ecl_alu_out_sel_rs3_e_l;	// the output stage mux.  They are active low
	vpiHandle  ecl_alu_out_sel_shift_e_l;	// and choose the output of the respective block.
	vpiHandle  ecl_alu_out_sel_logic_e_l;
   
	vpiHandle shft_alu_shift_out_e;	// result from shifter, 64 bit 
	vpiHandle ecl_alu_sethi_inst_e;   
	vpiHandle ifu_lsu_casa_e; 

	vpiHandle ecl_shft_op32_e;
	vpiHandle ecl_shft_shift4_e;
	vpiHandle ecl_shft_shift1_e;
	vpiHandle ecl_shft_enshift_e_l;
	vpiHandle ecl_shft_extendbit_e;
	vpiHandle ecl_shft_extend32bit_e_l;
	vpiHandle ecl_shft_lshift_e_l;

	int fdRead;
} readStim_s, *readStim_p;

// all data (basically circuit outputs) to be transferred to the HLS to be put here:
 typedef struct writeLatch {
   
   vpiHandle so; 
   vpiHandle alu_byp_rd_data_e; // alu result, 64 bit   
   vpiHandle exu_ifu_brpc_e; // branch pc output, 48 bit   
   vpiHandle exu_lsu_ldst_va_e; // address for lsu, 48 bit   
   vpiHandle exu_lsu_early_va_e; // faster bits for cache, 8 bit
   vpiHandle exu_mmu_early_va_e; // 8 bit

   vpiHandle alu_ecl_add_n64_e;
   vpiHandle alu_ecl_add_n32_e;
   vpiHandle alu_ecl_log_n64_e;
   vpiHandle alu_ecl_log_n32_e;
   
   vpiHandle alu_ecl_zhigh_e;
   vpiHandle alu_ecl_zlow_e;
   
   vpiHandle exu_ifu_regz_e;              // rs1_data == 0 
   vpiHandle exu_ifu_regn_e;
   
   vpiHandle alu_ecl_adderin2_63_e;
   vpiHandle alu_ecl_adderin2_31_e;
   
   vpiHandle alu_ecl_adder_out_63_e;
   vpiHandle alu_ecl_cout32_e;       // To ecl of sparc_exu_ecl.v
   vpiHandle alu_ecl_cout64_e_l;       // To ecl of sparc_exu_ecl.v
   
   vpiHandle alu_ecl_mem_addr_invalid_e_l; // adder_out[63:48] not all 1 or all 0
  
   int fdWrite;
} writeLatch_s, *writeLatch_p;


//--------------------------- compiletf functions 
PLI_INT32 readStimuli_compiletf(PLI_BYTE8 *user_data)
{
  vpiHandle cb_handle;
  s_cb_data cb_data;
  p_cb_data cb_data_p = &cb_data;

  vpiHandle systf_h; 
  systf_h = vpi_handle(vpiSysTfCall, NULL);
 
  // register a start of simulation call back to open the read pipe
  cb_data_p->reason = cbStartOfSimulation;
  cb_data_p->cb_rtn = openReadPipeStartOfSim;
  cb_data_p->obj = NULL;
  cb_data_p->time = NULL;
  cb_data_p->value = NULL;
  cb_data_p-> user_data = (PLI_BYTE8 *) systf_h;
  cb_handle = vpi_register_cb(cb_data_p);
  vpi_free_object(cb_handle);

  return (0);
}

PLI_INT32 writeLatch_compiletf(PLI_BYTE8 *user_data)
{
  vpiHandle cb_handle;
  s_cb_data cb_data;
  p_cb_data cb_data_p = &cb_data;

  vpiHandle systf_h; 
  systf_h = vpi_handle(vpiSysTfCall, NULL);

  // register a start of simulation call back to open the write pipe
  cb_data_p->reason = cbStartOfSimulation;
  cb_data_p->cb_rtn = openWritePipeStartOfSim;
  cb_data_p->obj = NULL;
  cb_data_p->time = NULL;
  cb_data_p->value = NULL;
  cb_data_p-> user_data = (PLI_BYTE8 *) systf_h;
  cb_handle = vpi_register_cb(cb_data_p);
  vpi_free_object(cb_handle);

  return(0);
}

//--------------------------- registration functions
void readStimuliRegister()
{	
//  vpiHandle systf_h;
  s_vpi_systf_data tf_data;
  p_vpi_systf_data tf_data_p = &tf_data;
  
  // register $readStimuli
  tf_data_p->type        = vpiSysTask;
  tf_data_p->tfname      = "$readStimuli";
  tf_data_p->compiletf	=(PLI_INT32(*)()) readStimuli_compiletf;
  tf_data_p->calltf   = (PLI_INT32(*)()) readStimuli_calltf;
  tf_data_p->sizetf = NULL;
  tf_data_p->user_data = NULL; 
  vpi_register_systf(tf_data_p);
  return;
}

void writeLatchRegister()
{ 
//  vpiHandle systf_h;
  s_vpi_systf_data tf_data;
  p_vpi_systf_data tf_data_p = &tf_data;
  
  // register $writeLatch
  tf_data_p->type        = vpiSysTask;
  tf_data_p->tfname      = "$writeLatch";
  tf_data_p->compiletf	= (PLI_INT32(*)()) writeLatch_compiletf;
  tf_data_p->calltf   = (PLI_INT32(*)()) writeLatch_calltf;
  tf_data_p->sizetf = NULL;
  tf_data_p->user_data = NULL; 
  vpi_register_systf(tf_data_p);
  return;
}

//--------------------------- callback routines
PLI_INT32 openReadPipeStartOfSim(p_cb_data cb_data)
{
	int fdRead, nread;
	vpiHandle systf_h, arg_itr;
	char * readFIFO = "./stimuli";	
	readStim_p data;
	
	// DEBUG
	//	vpi_printf("LLS> openReadPipeStartOfSim: entering\n");
	//	vpi_printf("LLS> SERVING OVER %s\n", readFIFO);
	
	data = (readStim_p)malloc(sizeof(readStim_s)); 
	//DEBUG
	if(!data){
		vpi_printf("LLS> openReadPipeStartOfSim: malloc returns NULL\n");
		vpi_control(vpiFinish, 0); // abort simulation
	}

	// TODO check mode
	fdRead = open(readFIFO, O_RDWR|O_ASYNC);
	// DEBUG
	if(fdRead<0){
		vpi_printf("Read pipe cannot be opened\n");
		vpi_control(vpiFinish, 0); // abort simulation
	}
	data->fdRead = fdRead;

	// obtain a handle for the system task instance and its args 
	systf_h = (vpiHandle)cb_data->user_data;
 	arg_itr = vpi_iterate(vpiArgument, systf_h);

	//data->se = vpi_scan(arg_itr);
   	data->byp_alu_rs1_data_e = vpi_scan(arg_itr);   // source operand 1, 64 bit
   	data->byp_alu_rs2_data_e = vpi_scan(arg_itr);  // source operand 2, 64 bit
  	data->ecl_alu_cin_e = vpi_scan(arg_itr);            // cin for adder
  	//data->shft_alu_shift_out_e = vpi_scan(arg_itr);	// result from shifter, 64 bit 
 	//data->byp_alu_rs3_data_e = vpi_scan(arg_itr);  // source operand 3, 64 bit
 
   	data-> ifu_exu_invert_d = vpi_scan(arg_itr); // this field indicates whether op2 should be complemented [applies only for logical operations]
	
   	data->ecl_alu_log_sel_and_e = vpi_scan(arg_itr);	// These 4 vpiHandles are select lines for the logic
 	data->ecl_alu_log_sel_or_e = vpi_scan(arg_itr);	// block mux.  They are active high and choose the
 	data->ecl_alu_log_sel_xor_e = vpi_scan(arg_itr);	// output they describe.
  	data->ecl_alu_log_sel_move_e = vpi_scan(arg_itr);

   	data->ecl_alu_out_sel_sum_e_l = vpi_scan(arg_itr);	// The following 4 are select lines for 
   	data->ecl_alu_out_sel_shift_e_l = vpi_scan(arg_itr);	// and choose the output of the respective block.
   	//data->ecl_alu_out_sel_rs3_e_l = vpi_scan(arg_itr);	// the output stage mux.  They are active high
   	data->ecl_alu_out_sel_logic_e_l = vpi_scan(arg_itr);
  	data->ecl_alu_sethi_inst_e = vpi_scan(arg_itr); 
	 	
	//data-> byp_alu_rcc_data_e = vpi_scan(arg_itr);  // source operand for reg condition codes, 64 bit
	//data-> ifu_lsu_casa_e = vpi_scan(arg_itr);	
	//data-> si = vpi_scan(arg_itr);

	data->ecl_shft_op32_e = vpi_scan(arg_itr);
	data->ecl_shft_shift4_e = vpi_scan(arg_itr);
	data->ecl_shft_shift1_e = vpi_scan(arg_itr);
	data->ecl_shft_enshift_e_l = vpi_scan(arg_itr);
	data->ecl_shft_extendbit_e = vpi_scan(arg_itr);
	data->ecl_shft_extend32bit_e_l = vpi_scan(arg_itr);
	data->ecl_shft_lshift_e_l = vpi_scan(arg_itr);



//	data->alu_byp_rd_data_e =  vpi_scan(arg_itr);

	vpi_free_object(arg_itr);
	vpi_put_userdata(systf_h, (void *)data);

	// DEBUG
//	vpi_printf("LLS> openReadPipeStartOfSim: registration done, exiting\n");
	return(0);
}

PLI_INT32 openWritePipeStartOfSim(p_cb_data cb_data)
{
	int fdWrite;
	vpiHandle systf_h, arg_itr;
	char * writeFIFO = "./response";
	writeLatch_p data;

	// DEBUG
//	vpi_printf("LLS> openWritePipeStartOfSim: entering\n");
	
	data = (writeLatch_p)malloc(sizeof(writeLatch_s));
	//DEBUG
	if(!data){
		vpi_printf("LLS> openWritePipeStartOfSim: malloc returns NULL\n");
		vpi_control(vpiFinish, 0); // abort simulation
	}
	
	// TODO check mode 
//	fdWrite = open(writeFIFO, O_WRONLY|O_ASYNC);
	fdWrite = open(writeFIFO, O_WRONLY);
	// DEBUG
	if(fdWrite<0){
		vpi_printf("Read pipe cannot be opened\n");
		vpi_control(vpiFinish, 0); // abort simulation
	}

	data->fdWrite = fdWrite;

	// obtain a handle for the system task instance and its args 
	systf_h = (vpiHandle)cb_data->user_data;
 	arg_itr = vpi_iterate(vpiArgument, systf_h);

	data->alu_byp_rd_data_e =  vpi_scan(arg_itr);

	// FIXME all this data to be included
 
  /*	data->so  = vpi_scan(arg_itr); 

   	data-> exu_ifu_brpc_e  = vpi_scan(arg_itr); // branch pc output, 48 bit   
   	data-> exu_lsu_ldst_va_e  = vpi_scan(arg_itr); // address for lsu, 48 bit   
   	data-> exu_lsu_early_va_e = vpi_scan(arg_itr); // faster bits for cache, 8 bit
   	data-> exu_mmu_early_va_e = vpi_scan(arg_itr); // 8 bit

   	data-> alu_ecl_add_n64_e = vpi_scan(arg_itr);
   	data-> alu_ecl_add_n32_e = vpi_scan(arg_itr);
   	data-> alu_ecl_log_n64_e = vpi_scan(arg_itr);
   	data-> alu_ecl_log_n32_e = vpi_scan(arg_itr);
   
   	data-> alu_ecl_zhigh_e = vpi_scan(arg_itr);
   	data-> alu_ecl_zlow_e = vpi_scan(arg_itr);
   
   	data-> exu_ifu_regz_e = vpi_scan(arg_itr);              // rs1_data == 0 
   	data-> exu_ifu_regn_e = vpi_scan(arg_itr);
   
   	data-> alu_ecl_adderin2_63_e = vpi_scan(arg_itr);
   	data-> alu_ecl_adderin2_31_e = vpi_scan(arg_itr);
   
   	data-> alu_ecl_adder_out_63_e = vpi_scan(arg_itr);
   	data-> alu_ecl_cout32_e = vpi_scan(arg_itr);       // To ecl of sparc_exu_ecl.v
   	data-> alu_ecl_cout64_e_l = vpi_scan(arg_itr);       // To ecl of sparc_exu_ecl.v
   
   	data-> alu_ecl_mem_addr_invalid_e_l = vpi_scan(arg_itr); // adder_out[63:48] not all 1 or all 0
*/
	vpi_free_object(arg_itr);
	vpi_put_userdata(systf_h, (void *)data);

	// DEBUG
//	vpi_printf("LLS> openWritePipeStartOfSim: registration done, exiting\n");

	return(0);
}

//--------------------------- calltf routines
PLI_INT32 readStimuli_calltf(PLI_BYTE8 *user_data)
{	
  	vpiHandle systf_h;
	int fdRead;
	int nread;
	readStim_p data;
	s_vpi_value value_s;
	char op1[REG_WIDTH];
	char op2[REG_WIDTH];
	char op3[REG_WIDTH];	
	char opShift[REG_WIDTH];	
	char rcc[REG_WIDTH];	
	int seVal;
	int cin; 
	int invert;
	int and_sel;
	int or_sel;
	int xor_sel;
	int move_sel;
	int sel_sum;
	int sel_shift; 
	int sel_rs3;	 
	int sel_logic;
	int null_32;
	int lsu;
	int siVal;
	int ecl_shft_op32_e;
	char ecl_shft_shift4_e[5];
	char ecl_shft_shift1_e[5];
	int ecl_shft_enshift_e_l;
	int ecl_shft_extendbit_e;
	int ecl_shft_extend32bit_e_l;
	int ecl_shft_lshift_e_l;


  	// DEBUG
 // 	vpi_printf("LLS> ------------------------------ Entering readStimuli_calltf\n");
	
	// obtain a handle for the system task instance and its args 
	systf_h = vpi_handle(vpiSysTfCall, NULL); 
	data = (readStim_p)vpi_get_userdata(systf_h);
	fdRead = data->fdRead;

	//nread = read(fdRead, &seVal, sizeof(int));
	nread = read(fdRead, &op1, REG_WIDTH);
	nread = read(fdRead, &op2, REG_WIDTH);
	nread = read(fdRead, &cin, sizeof(int));
	//nread = read(fdRead, &opShift, REG_WIDTH);
	//nread = read(fdRead, &op3, REG_WIDTH);
	nread = read(fdRead, &invert, sizeof(int));
	nread = read(fdRead, &and_sel, sizeof(int));
	nread = read(fdRead, &or_sel, sizeof(int));
	nread = read(fdRead, &xor_sel, sizeof(int));
	nread = read(fdRead, &move_sel, sizeof(int));
	nread = read(fdRead, &sel_sum, sizeof(int));
	nread = read(fdRead, &sel_shift, sizeof(int));
	//nread = read(fdRead, &sel_rs3, sizeof(int));
	nread = read(fdRead, &sel_logic, sizeof(int));
	nread = read(fdRead, &null_32, sizeof(int));
	//nread = read(fdRead, &rcc, REG_WIDTH);
	//nread = read(fdRead, &lsu, sizeof(int));
	//nread = read(fdRead, &siVal, sizeof(int));
	nread = read(fdRead, &ecl_shft_op32_e, sizeof(int));
	nread = read(fdRead, &ecl_shft_shift4_e, 5);
	nread = read(fdRead, &ecl_shft_shift1_e, 5);
	nread = read(fdRead, &ecl_shft_enshift_e_l, sizeof(int));
	nread = read(fdRead, &ecl_shft_extendbit_e, sizeof(int));
	nread = read(fdRead, &ecl_shft_extend32bit_e_l, sizeof(int));
	nread = read(fdRead, &ecl_shft_lshift_e_l, sizeof(int));	
	// DEBUG
	if (VERY_VERBOSE) {
		vpi_printf("LLS> HLS sent op1 as %s\n", op1);
		vpi_printf("LLS> HLS sent op2 as %s\n", op2);
		vpi_printf("LLS> HLS sent cin as %d\n", cin);
		vpi_printf("LLS> HLS sent invert as %d\n", invert);	
		vpi_printf("LLS> HLS sent and as %d\n", and_sel);
		vpi_printf("LLS> HLS sent or as %d\n", or_sel);
		vpi_printf("LLS> HLS sent xor as %d\n", xor_sel);
		vpi_printf("LLS> HLS sent move_sel as %d\n", move_sel);
		vpi_printf("LLS> HLS sent sel_sum as %d\n", sel_sum);
		vpi_printf("LLS> HLS sent sel_shift as %d\n", sel_shift);
		vpi_printf("LLS> HLS sent sel_logic as %d\n",sel_logic);
		vpi_printf("LLS> HLS sent null_32 as %d\n",null_32);
		vpi_printf("LLS> HLS sent ecl_shft_op32_e as %d\n", ecl_shft_op32_e);
		vpi_printf("LLS> HLS sent ecl_shft_shift4_e as %s\n", ecl_shft_shift4_e);
		vpi_printf("LLS> HLS sent ecl_shft_shift1_e as %s\n", ecl_shft_shift1_e);
		vpi_printf("LLS> HLS sent ecl_shft_enshift_e_l as %d\n", ecl_shft_enshift_e_l);
		vpi_printf("LLS> HLS sent ecl_shft_extendbit_e as %d\n", ecl_shft_extendbit_e);
		vpi_printf("LLS> HLS sent ecl_shft_extend32bit_e_l as %d\n", ecl_shft_extend32bit_e_l);
		vpi_printf("LLS> HLS sent ecl_shft_lshift_e_l as %d\n", ecl_shft_lshift_e_l);

	}
	// Send values back to Verilog domain
/* 	value_s.format = vpiIntVal; */
/* 	value_s.value.integer = seVal; */
/* 	vpi_put_value(data->se, &value_s, NULL, vpiNoDelay); */

	value_s.format = vpiBinStrVal;
	value_s.value.str = op1;
	vpi_put_value(data->byp_alu_rs1_data_e, &value_s, NULL, vpiNoDelay);
	value_s.value.str = op2;
	vpi_put_value(data->byp_alu_rs2_data_e, &value_s, NULL, vpiNoDelay);

	value_s.format = vpiIntVal;
	value_s.value.integer = cin;
	vpi_put_value(data->ecl_alu_cin_e, &value_s, NULL, vpiNoDelay);

/* 	value_s.format = vpiBinStrVal; */
/* 	value_s.value.str = opShift; */
/* 	vpi_put_value(data-> shft_alu_shift_out_e, &value_s, NULL, vpiNoDelay); */
/* 	value_s.value.str = op3; */
/* 	vpi_put_value(data-> byp_alu_rs3_data_e, &value_s, NULL, vpiNoDelay); */

	value_s.format = vpiIntVal;
	value_s.value.integer = invert;
	vpi_put_value(data-> ifu_exu_invert_d, &value_s, NULL, vpiNoDelay);	
	value_s.value.integer = and_sel;
	vpi_put_value(data->ecl_alu_log_sel_and_e, &value_s, NULL, vpiNoDelay);
	value_s.value.integer = or_sel;
	vpi_put_value(data->ecl_alu_log_sel_or_e, &value_s, NULL, vpiNoDelay);
	value_s.value.integer = xor_sel;
	vpi_put_value(data->ecl_alu_log_sel_xor_e, &value_s, NULL, vpiNoDelay);
	value_s.value.integer = move_sel;
	vpi_put_value(data-> ecl_alu_log_sel_move_e, &value_s, NULL, vpiNoDelay);
	value_s.value.integer = sel_sum;
	vpi_put_value(data->ecl_alu_out_sel_sum_e_l, &value_s, NULL, vpiNoDelay);
/* 	value_s.value.integer = sel_rs3; */
/* 	vpi_put_value(data->ecl_alu_out_sel_rs3_e_l, &value_s, NULL, vpiNoDelay); */
	value_s.value.integer = sel_shift;
	vpi_put_value(data->ecl_alu_out_sel_shift_e_l, &value_s, NULL, vpiNoDelay);
	value_s.value.integer = sel_logic;
	vpi_put_value(data->ecl_alu_out_sel_logic_e_l, &value_s, NULL, vpiNoDelay);
	value_s.value.integer = null_32;
	vpi_put_value(data-> ecl_alu_sethi_inst_e, &value_s, NULL, vpiNoDelay);

/* 	value_s.format = vpiBinStrVal; */
/* 	value_s.value.str = rcc; */
/* 	vpi_put_value(data-> byp_alu_rcc_data_e, &value_s, NULL, vpiNoDelay); */

/* 	value_s.format = vpiIntVal; */
/* 	value_s.value.integer = lsu; */
/* 	vpi_put_value(data-> ifu_lsu_casa_e, &value_s, NULL, vpiNoDelay); */
/* 	value_s.value.integer = siVal; */
/* 	vpi_put_value(data-> si, &value_s, NULL, vpiNoDelay); */

	value_s.value.integer = ecl_shft_op32_e;
	vpi_put_value(data->ecl_shft_op32_e, &value_s, NULL, vpiNoDelay);
	value_s.value.integer = ecl_shft_enshift_e_l;
	vpi_put_value(data->ecl_shft_enshift_e_l, &value_s, NULL, vpiNoDelay);
	value_s.value.integer = ecl_shft_extendbit_e;
	vpi_put_value(data->ecl_shft_extendbit_e, &value_s, NULL, vpiNoDelay);
	value_s.value.integer = ecl_shft_extend32bit_e_l;
	vpi_put_value(data->ecl_shft_extend32bit_e_l, &value_s, NULL, vpiNoDelay);
	value_s.value.integer = ecl_shft_lshift_e_l;
	vpi_put_value(data->ecl_shft_lshift_e_l, &value_s, NULL, vpiNoDelay);

	value_s.format = vpiBinStrVal;
	value_s.value.str = ecl_shft_shift4_e;
	vpi_put_value(data->ecl_shft_shift4_e, &value_s, NULL, vpiNoDelay);
	value_s.value.str = ecl_shft_shift1_e;
	vpi_put_value(data->ecl_shft_shift1_e, &value_s, NULL, vpiNoDelay);
  	// DEBUG
//  	vpi_printf("LLS> ------------ Exiting readStimuli_calltf\n");
	
  	return (0);	
}

PLI_INT32 writeLatch_calltf(PLI_BYTE8 *user_data)
{ 
	int fdWrite;
	int nwrite;

	vpiHandle alu_byp_rd_data_e, systf_h;
	s_vpi_value value_s;
	writeLatch_p data;
	char result[REG_WIDTH];
	
	// DEBUG
//	vpi_printf("LLS> writeLatch_calltf: entering\n");
	int u;
	
	// obtain a handle for the system task instance and its args 
	systf_h = vpi_handle(vpiSysTfCall, NULL);
	data = (writeLatch_p)vpi_get_userdata(systf_h);

	fdWrite = data->fdWrite;
	alu_byp_rd_data_e = data->alu_byp_rd_data_e;

	// FIXME format again
		
	// Get values from Verilog domain		
	value_s.format = vpiBinStrVal;
	vpi_get_value(alu_byp_rd_data_e, &value_s);
	strcpy(result, value_s.value.str);

	// Send result to HLS
	nwrite = write(fdWrite, &result, REG_WIDTH);

	// DEBUG
	if (VERY_VERBOSE) {
		vpi_printf("LLS> Result (bin string)  ");
		for(u=0;u<=REG_WIDTH-1; u++){
			vpi_printf("%c", result[u]);
			if (u%4==3) vpi_printf (" ");
		}
		vpi_printf("\n");
	}

  	vpi_free_object(systf_h);

	// DEBUG
//	vpi_printf("LLS> writeLatch_calltf: exiting\n");

  	return (0);
}

