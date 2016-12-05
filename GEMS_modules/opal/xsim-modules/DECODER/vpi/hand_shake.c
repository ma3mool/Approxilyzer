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
    vpiHandle inst ;
    int fdRead ;
} readStim_s, *readStim_p;

// all data (basically circuit outputs) to be transferred to the HLS to be put here:
typedef struct writeLatch {
    vpiHandle rd ;
    vpiHandle rd_type ;
    vpiHandle rd_valid ;
    vpiHandle rd2 ;
    vpiHandle rd2_type ;
    vpiHandle rd2_valid ;
    vpiHandle rs1 ;
    vpiHandle rs1_type ;
    vpiHandle rs1_valid ;
    vpiHandle rs2 ;
    vpiHandle rs2_type ;
    vpiHandle rs2_valid ;
    vpiHandle rs3 ;
    vpiHandle rs3_type ;
    vpiHandle rs3_valid ;
    vpiHandle rs4 ;
    vpiHandle rs4_type ;
    vpiHandle rs4_valid ;
    vpiHandle m_type ;
    vpiHandle m_futype ;
    vpiHandle m_opcode ;
    vpiHandle m_branch_type ;
    vpiHandle m_imm ;
    vpiHandle m_ccshift ;
    vpiHandle m_access_size ;
    vpiHandle m_flags ;
    vpiHandle fail ;
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
  tf_data_p->compiletf    =(PLI_INT32(*)()) readStimuli_compiletf;
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
  tf_data_p->compiletf    = (PLI_INT32(*)()) writeLatch_compiletf;
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

    // Iterate and get the objects
    data->inst = vpi_scan(arg_itr) ;

    vpi_free_object(arg_itr);
    vpi_put_userdata(systf_h, (void *)data);

    // DEBUG
//    vpi_printf("LLS> openReadPipeStartOfSim: registration done, exiting\n");
    return(0);
}

PLI_INT32 openWritePipeStartOfSim(p_cb_data cb_data)
{
    int fdWrite;
    vpiHandle systf_h, arg_itr;
    char * writeFIFO = "./response";
    writeLatch_p data;

    // DEBUG
//    vpi_printf("LLS> openWritePipeStartOfSim: entering\n");
    
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

    data->rd = vpi_scan(arg_itr) ;
    data->rd_type = vpi_scan(arg_itr) ;
    data->rd_valid = vpi_scan(arg_itr) ;

    data->rd2 = vpi_scan(arg_itr) ;
    data->rd2_type = vpi_scan(arg_itr) ;
    data->rd2_valid = vpi_scan(arg_itr) ;

    data->rs1 = vpi_scan(arg_itr) ;
    data->rs1_type = vpi_scan(arg_itr) ;
    data->rs1_valid = vpi_scan(arg_itr) ;

    data->rs2 = vpi_scan(arg_itr) ;
    data->rs2_type = vpi_scan(arg_itr) ;
    data->rs2_valid = vpi_scan(arg_itr) ;

    data->rs3 = vpi_scan(arg_itr) ;
    data->rs3_type = vpi_scan(arg_itr) ;
    data->rs3_valid = vpi_scan(arg_itr) ;

    data->rs4 = vpi_scan(arg_itr) ;
    data->rs4_type = vpi_scan(arg_itr) ;
    data->rs4_valid = vpi_scan(arg_itr) ;

    data->m_type = vpi_scan(arg_itr) ;
    data->m_futype = vpi_scan(arg_itr) ;
    data->m_opcode = vpi_scan(arg_itr) ;
    data->m_branch_type = vpi_scan(arg_itr) ;
    data->m_imm = vpi_scan(arg_itr) ;
    data->m_ccshift = vpi_scan(arg_itr) ;
    data->m_access_size = vpi_scan(arg_itr) ;
    data->m_flags = vpi_scan(arg_itr) ;
    data->fail = vpi_scan(arg_itr) ;

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

    char inst_vec[33] ;
    
    // obtain a handle for the system task instance and its args 
    systf_h = vpi_handle(vpiSysTfCall, NULL); 
    data = (readStim_p)vpi_get_userdata(systf_h);
    fdRead = data->fdRead;

    nread = read(fdRead, &inst_vec, 33) ;

	// vpi_printf("VPI : inst_vec --%s--\n", inst_vec) ;
    // Send values back to Verilog domain
    value_s.format = vpiBinStrVal;
    value_s.value.str = inst_vec ;
    vpi_put_value(data->inst, &value_s, NULL, vpiNoDelay);

    return (0);
}

PLI_INT32 writeLatch_calltf(PLI_BYTE8 *user_data)
{ 
    int fdWrite;
    int nwrite;

    vpiHandle systf_h;
    vpiHandle rd ;
    vpiHandle rd_type ;
    vpiHandle rd_valid ;
    vpiHandle rd2 ;
    vpiHandle rd2_type ;
    vpiHandle rd2_valid ;
    vpiHandle rs1 ;
    vpiHandle rs1_type ;
    vpiHandle rs1_valid ;
    vpiHandle rs2 ;
    vpiHandle rs2_type ;
    vpiHandle rs2_valid ;
    vpiHandle rs3 ;
    vpiHandle rs3_type ;
    vpiHandle rs3_valid ;
    vpiHandle rs4 ;
    vpiHandle rs4_type ;
    vpiHandle rs4_valid ;
    vpiHandle m_type ;
    vpiHandle m_futype ;
    vpiHandle m_opcode ;
    vpiHandle m_branch_type ;
    vpiHandle m_imm ;
    vpiHandle m_ccshift ;
    vpiHandle m_access_size ;
    vpiHandle m_flags ;
    vpiHandle fail ;

    s_vpi_value value_s;
    writeLatch_p data;
    
    char v_rd[7] ;
    char v_rd_type[5] ;
	int v_rd_valid ;

    char v_rd2[7] ;
    char v_rd2_type[5] ;
	int v_rd2_valid ;

    char v_rs1[7] ;
    char v_rs1_type[5] ;
	int v_rs1_valid ;

    char v_rs2[7] ;
    char v_rs2_type[5] ;
	int v_rs2_valid ;

    char v_rs3[7] ;
    char v_rs3_type[5] ;
	int v_rs3_valid ;

    char v_rs4[7] ;
    char v_rs4_type[5] ;
	int v_rs4_valid ;

    char v_m_type[4] ;
    char v_m_futype[5] ;
    char v_m_opcode[10] ;
    char v_m_branch_type[5] ;
    char v_m_imm[65] ;
    char v_m_ccshift[9] ;
    char v_m_access_size[9] ;
    char v_m_flags[9] ;
    int v_fail ;
    
    // obtain a handle for the system task instance and its args 
    systf_h = vpi_handle(vpiSysTfCall, NULL);
    data = (writeLatch_p)vpi_get_userdata(systf_h);

    fdWrite = data->fdWrite;

    rd             = data->rd ;
    rd_type        = data->rd_type ;
    rd_valid       = data->rd_valid ;

    rd2            = data->rd2 ;
    rd2_type       = data->rd2_type ;
    rd2_valid      = data->rd2_valid ;

    rs1            = data->rs1 ;
    rs1_type       = data->rs1_type ;
    rs1_valid      = data->rs1_valid ;

    rs2            = data->rs2 ;
    rs2_type       = data->rs2_type ;
    rs2_valid      = data->rs2_valid ;

    rs3            = data->rs3 ;
    rs3_type       = data->rs3_type ;
    rs3_valid      = data->rs3_valid ;

    rs4            = data->rs4 ;
    rs4_type       = data->rs4_type ;
    rs4_valid      = data->rs4_valid ;

    m_type         = data->m_type ;
    m_futype       = data->m_futype ;
    m_opcode       = data->m_opcode ;
    m_branch_type  = data->m_branch_type ;
    m_imm          = data->m_imm ;
    m_ccshift      = data->m_ccshift ;
    m_access_size  = data->m_access_size ;
    m_flags        = data->m_flags ;
    fail           = data->fail ;

    // Get values from Verilog domain and send it to HLS
	// rd
    value_s.format = vpiBinStrVal;
    vpi_get_value(rd, &value_s);
    strcpy(v_rd, value_s.value.str);
    nwrite = write(fdWrite, &v_rd, 7);

    value_s.format = vpiBinStrVal;
    vpi_get_value(rd_type, &value_s);
    strcpy(v_rd_type, value_s.value.str);
    nwrite = write(fdWrite, &v_rd_type, 5);

    value_s.format = vpiIntVal ;
    vpi_get_value(rd_valid, &value_s);
	// vpi_printf("VPI: rd_valid = %d\n", value_s.value.integer) ;
    v_rd_valid = value_s.value.integer ;
    nwrite = write(fdWrite, &v_rd_valid, sizeof(int));

	// rd2
    value_s.format = vpiBinStrVal;
    vpi_get_value(rd2, &value_s);
    strcpy(v_rd2, value_s.value.str);
    nwrite = write(fdWrite, &v_rd2, 7);

    value_s.format = vpiBinStrVal;
    vpi_get_value(rd2_type, &value_s);
    strcpy(v_rd2_type, value_s.value.str);
    nwrite = write(fdWrite, &v_rd2_type, 5);

    value_s.format = vpiIntVal ;
    vpi_get_value(rd2_valid, &value_s);
	// vpi_printf("VPI: rd2_valid = %d\n", value_s.value.integer) ;
    v_rd2_valid = value_s.value.integer ;
    nwrite = write(fdWrite, &v_rd2_valid, sizeof(int));

	// rs1
    value_s.format = vpiBinStrVal;
    vpi_get_value(rs1, &value_s);
    strcpy(v_rs1, value_s.value.str);
    nwrite = write(fdWrite, &v_rs1, 7);

    value_s.format = vpiBinStrVal;
    vpi_get_value(rs1_type, &value_s);
    strcpy(v_rs1_type, value_s.value.str);
    nwrite = write(fdWrite, &v_rs1_type, 5);

    value_s.format = vpiIntVal ;
    vpi_get_value(rs1_valid, &value_s);
	// vpi_printf("VPI: rs1_valid = %d\n", value_s.value.integer) ;
    v_rs1_valid = value_s.value.integer ;
    nwrite = write(fdWrite, &v_rs1_valid, sizeof(int));

	// rs2
    value_s.format = vpiBinStrVal;
    vpi_get_value(rs2, &value_s);
    strcpy(v_rs2, value_s.value.str);
    nwrite = write(fdWrite, &v_rs2, 7);

    value_s.format = vpiBinStrVal;
    vpi_get_value(rs2_type, &value_s);
    strcpy(v_rs2_type, value_s.value.str);
    nwrite = write(fdWrite, &v_rs2_type, 5);

    value_s.format = vpiIntVal ;
    vpi_get_value(rs2_valid, &value_s);
	// vpi_printf("VPI: rs2_valid = %d\n", value_s.value.integer) ;
    v_rs2_valid = value_s.value.integer ;
    nwrite = write(fdWrite, &v_rs2_valid, sizeof(int));

	// rs3
    value_s.format = vpiBinStrVal;
    vpi_get_value(rs3, &value_s);
    strcpy(v_rs3, value_s.value.str);
    nwrite = write(fdWrite, &v_rs3, 7);

    value_s.format = vpiBinStrVal;
    vpi_get_value(rs3_type, &value_s);
    strcpy(v_rs3_type, value_s.value.str);
    nwrite = write(fdWrite, &v_rs3_type, 5);

    value_s.format = vpiIntVal ;
    vpi_get_value(rs3_valid, &value_s);
	// vpi_printf("VPI: rs3_valid = %d\n", value_s.value.integer) ;
    v_rs3_valid = value_s.value.integer ;
    nwrite = write(fdWrite, &v_rs3_valid, sizeof(int));

	// rs4
    value_s.format = vpiBinStrVal;
    vpi_get_value(rs4, &value_s);
    strcpy(v_rs4, value_s.value.str);
    nwrite = write(fdWrite, &v_rs4, 7);

    value_s.format = vpiBinStrVal;
    vpi_get_value(rs4_type, &value_s);
    strcpy(v_rs4_type, value_s.value.str);
    nwrite = write(fdWrite, &v_rs4_type, 5);

    value_s.format = vpiIntVal ;
    vpi_get_value(rs4_valid, &value_s);
	// vpi_printf("VPI: rs4_valid = %d\n", value_s.value.integer) ;
    v_rs4_valid = value_s.value.integer ;
    nwrite = write(fdWrite, &v_rs4_valid, sizeof(int));

	// Other signals
    value_s.format = vpiBinStrVal;
    vpi_get_value(m_type, &value_s);
    strcpy(v_m_type, value_s.value.str);
	// vpi_printf("VPI: m_type = %s\n", v_m_type) ;
    nwrite = write(fdWrite, &v_m_type, 4);

    value_s.format = vpiBinStrVal;
    vpi_get_value(m_futype, &value_s);
    strcpy(v_m_futype, value_s.value.str);
	// vpi_printf("VPI: m_futype = %s\n", v_m_futype) ;
    nwrite = write(fdWrite, &v_m_futype, 5);

    value_s.format = vpiBinStrVal;
    vpi_get_value(m_opcode, &value_s);
    strcpy(v_m_opcode, value_s.value.str);
	// vpi_printf("VPI: m_opcode = %s\n", v_m_opcode) ;
    nwrite = write(fdWrite, &v_m_opcode, 10);

    value_s.format = vpiBinStrVal;
    vpi_get_value(m_branch_type, &value_s);
    strcpy(v_m_branch_type, value_s.value.str);
	// vpi_printf("VPI: m_branch_type= %s\n", v_m_branch_type) ;
    nwrite = write(fdWrite, &v_m_branch_type, 5);

    value_s.format = vpiBinStrVal;
    vpi_get_value(m_imm, &value_s);
    strcpy(v_m_imm, value_s.value.str);
    nwrite = write(fdWrite, &v_m_imm, 65);

    value_s.format = vpiBinStrVal;
    vpi_get_value(m_ccshift, &value_s);
    strcpy(v_m_ccshift, value_s.value.str);
    nwrite = write(fdWrite, &v_m_ccshift, 9);

    value_s.format = vpiBinStrVal;
    vpi_get_value(m_access_size, &value_s);
    strcpy(v_m_access_size, value_s.value.str);
    nwrite = write(fdWrite, &v_m_access_size, 9);

    value_s.format = vpiBinStrVal;
    vpi_get_value(m_flags, &value_s);
    strcpy(v_m_flags, value_s.value.str);
    nwrite = write(fdWrite, &v_m_flags, 9);

    value_s.format = vpiIntVal ;
    vpi_get_value(fail, &value_s);
    v_fail = value_s.value.integer ;
    nwrite = write(fdWrite, &v_fail, sizeof(int));

    vpi_free_object(systf_h);

    return (0);
}

