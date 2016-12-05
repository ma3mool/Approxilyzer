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

#define DEBUG 0

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
	vpiHandle op1 ;
	vpiHandle s1 ;
	vpiHandle imm ;
	vpiHandle use_imm ;
	int fdRead ;
} readStim_s, *readStim_p;

// all data (basically circuit outputs) to be transferred to the HLS to be put here:
typedef struct writeLatch {
	vpiHandle address ;
	int fdWrite ;
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
	
	if(DEBUG) {
		vpi_printf("LLS> openReadPipeStartOfSim: entering\n");
		vpi_printf("LLS> SERVING OVER %s\n", readFIFO);
	}
	
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

	// Iterate and geth objects
	data->op1 = vpi_scan(arg_itr) ;
	data->s1 = vpi_scan(arg_itr) ;
	data->imm = vpi_scan(arg_itr) ;
	data->use_imm = vpi_scan(arg_itr) ;

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

	data->address =  vpi_scan(arg_itr);

	vpi_free_object(arg_itr);
	vpi_put_userdata(systf_h, (void *)data);

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

	char op1_vec[REG_WIDTH] ;
	char s1_vec[REG_WIDTH] ;
	char imm_vec[REG_WIDTH] ;
	int use_imm ;
	
	// obtain a handle for the system task instance and its args 
	systf_h = vpi_handle(vpiSysTfCall, NULL); 
	data = (readStim_p)vpi_get_userdata(systf_h);
	fdRead = data->fdRead;

	nread = read(fdRead, &op1_vec, REG_WIDTH) ;
	nread = read(fdRead, &s1_vec, REG_WIDTH) ;
	nread = read(fdRead, &imm_vec, REG_WIDTH) ;
	nread = read(fdRead, &use_imm, sizeof(int)) ;

	// Send values back to Verilog domain
	value_s.format = vpiBinStrVal;
	value_s.value.str = op1_vec ;
	vpi_put_value(data->op1, &value_s, NULL, vpiNoDelay);
	value_s.value.str = s1_vec ;
	vpi_put_value(data->s1, &value_s, NULL, vpiNoDelay);
	value_s.value.str = imm_vec ;
	vpi_put_value(data->imm, &value_s, NULL, vpiNoDelay);

	value_s.format = vpiIntVal;
	value_s.value.integer = use_imm;
	vpi_put_value(data->use_imm, &value_s, NULL, vpiNoDelay);

  	return (0);	
}

PLI_INT32 writeLatch_calltf(PLI_BYTE8 *user_data)
{ 
	int fdWrite;
	int nwrite;

	vpiHandle address, systf_h;
	s_vpi_value value_s;
	writeLatch_p data;
	
	char address_vec[REG_WIDTH] ;
	char result[REG_WIDTH] ;
	
	// obtain a handle for the system task instance and its args 
	systf_h = vpi_handle(vpiSysTfCall, NULL);
	data = (writeLatch_p)vpi_get_userdata(systf_h);

	fdWrite = data->fdWrite;
	address = data->address;

	// Get values from Verilog domain		
	value_s.format = vpiBinStrVal;
	vpi_get_value(address, &value_s);
	strcpy(result, value_s.value.str);

	// Send result to HLS
	nwrite = write(fdWrite, &result, REG_WIDTH);
	//printf("result is %s\n",result);

  	vpi_free_object(systf_h);

  	return (0);
}

