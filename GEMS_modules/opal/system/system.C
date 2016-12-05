
/*
    Copyright (C) 1999-2005 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Opal Timing-First Microarchitectural
    Simulator, a component of the Multifacet GEMS (General 
    Execution-driven Multiprocessor Simulator) software toolset 
    originally developed at the University of Wisconsin-Madison.

    Opal was originally developed by Carl Mauer based upon code by
    Craig Zilles.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Manoj Plakal, Daniel Sorin, Min Xu, and Luke Yen.

    --------------------------------------------------------------------

    If your use of this software contributes to a published paper, we
    request that you (1) cite our summary paper that appears on our
    website (http://www.cs.wisc.edu/gems/) and (2) e-mail a citation
    for your published paper to gems@cs.wisc.edu.

    If you redistribute derivatives of this software, we request that
    you notify us and either (1) ask people to register with us at our
    website (http://www.cs.wisc.edu/gems/) or (2) collect registration
    information and periodically send it to us.

    --------------------------------------------------------------------

    Multifacet GEMS is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    Multifacet GEMS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Multifacet GEMS; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307, USA

    The GNU General Public License is contained in the file LICENSE.

### END HEADER ###
*/
/*
 * FileName:  system.C
 * Synopsis:  Implementation of the processor, memory model system
 * Author:    cmauer
 * Version:   $Id: system.C 1.89 06/02/13 18:49:23-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include "hfa.h"
#include "hfacore.h"

// getdirents headers
#ifdef USE_DIRENT
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
//#include <sys/stat.h>
//#include <fcntl.h>
#else
// sparc V9
#include <sys/types.h>
#include <sys/dir.h>
#endif
// end getdirent

// #include <sys/dirent.h>

#include "fileio.h"
#include "pseq.h"
#include "scheduler.h"         // event_t definitions
#include "transaction.h"       // transaction_t definitions
#include "confio.h"            // confio_t definitions
#include "hfa_init.h"
#include "mf_api.h"
#include "amber_api.h" 
#include "pipestate.h"
#include "rubycache.h"
#include "flow.h"
#include "sysstat.h"
#include "ptrace.h"
#include "symtrace.h"
#include "chain.h"
#include "opal.h"              // hfa_conf_object declaration
#include "exec.h"              // exec_fn_initialize() declaration
#include "system.h"

// The Simics API Wrappers
#include "interface.h"

#define LXL_NO_OPAL_CONFIG
#define MAX_INSTR_RUN 30*1000*1000

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/** global system object */
system_t  *system_t::inst = NULL;

/** global ruby interface object */
extern mf_opal_api_t hfa_ruby_interface;

/** The highest number the SIMICS_current_proc_no() can return */
static int32 s_max_processor_id = 0;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/// used to count the number of processors
static void s_count_processors( conf_object_t *processor, void *count_i32 );

/// used as a selector in readInstructionMap
static int  selectImapFiles( const struct dirent * );

// handles transaction completion events
static void system_transaction_callback( void *system_obj, conf_object_t *cpu,
                                         uint64 immediate );

// handles breakpoints
static void system_break_handler( void *pseq_obj, uint64 access_type,
                                  uint64 break_num,
                                  void *reg_ptr, uint64 reg_size );

// reads the MB of memory used, total (using linux /var file system)
static double process_memory_total( void );
static double process_memory_resident( void );

// external function defined in ruby/tester/testFramework.h
// CM fix: should structure includes so system.C sees the same definition
//         as ruby. This is used only when ruby and opal are compiled into
//         a stand-alone tester.
extern void tester_install_opal( mf_opal_api_t  *opal_api, mf_ruby_api_t *ruby_api );

// slew of functions for handling memory transactions
void     system_post_tick( void );

cycles_t system_memory_snoop( conf_object_t *obj,
                              conf_object_t *space,
                              map_list_t *map, 
                              generic_transaction_t *g);

cycles_t system_memory_operation_stats( conf_object_t *obj,
                                        conf_object_t *space,
                                        map_list_t *map, 
                                        generic_transaction_t *g);

cycles_t system_memory_operation_trace( conf_object_t *obj,
                                        conf_object_t *space,
                                        map_list_t *map, 
                                        generic_transaction_t *g);

cycles_t system_memory_operation_mlp_trace( conf_object_t *obj,
                                            conf_object_t *space,
                                            map_list_t *map, 
                                            generic_transaction_t *g);

cycles_t system_memory_operation_warmup( conf_object_t *obj,
                                         conf_object_t *space,
                                         map_list_t *map, 
                                         generic_transaction_t *g);

cycles_t system_memory_operation_symbol_mode( conf_object_t *obj,
                                              conf_object_t *space,
                                              map_list_t *map, 
                                              generic_transaction_t *g);

static void system_exception_handler( void *obj, conf_object_t *proc,
                                      uint64 exception );

static void system_exception_tracer( void *obj, conf_object_t *proc,
                                     uint64 exception );

// C++ Template: explicit instantiation
template class map<breakpoint_id_t, breakpoint_action_t *>;

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
system_t::system_t( const char *configurationFile )
{
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("[ 10] system_t() constructor\n");
#endif
  hfa_checkerr("system initialization");

  if (configurationFile == NULL)
    m_configFile = NULL;
  else
    m_configFile = strdup( configurationFile );

  // establish how many processors there are in the system
  int numProcs = SIMICS_number_processors();
  server_client_mode = -1;
  server_start_num = -1;
  client_start_num = -1;

  for ( int i = 0; i < numProcs; i++ ) {
      char tmp[10];
      conf_object_t *conf_obj = SIM_get_processor(i);

      strncpy(tmp,conf_obj->name, 3);
      tmp[3]='\0';

      if (strcmp(tmp,"cpu") == false) {
    	  break;
      } else {
    	  server_client_mode = 2;
    	  printf("cpu name is %s\n",conf_obj->name);
    	  if (strncmp(conf_obj->name, "server_", 7) == false) {
    		  server_start_num = i;
    	  } else if (strncmp(conf_obj->name, "client_", 7) == false) {
    		  client_start_num = i;
    	  } else {
    		  printf("I do not recognize this cpu prefix %s\n",conf_obj->name);
    	  }
      }

  }

  // LXL-FIXME: still hack to force one proc
  if (server_client_mode == 2) {
      numProcs = 1;
  }

  m_numProcs = numProcs;
  // s_max_processor_id = 0;
  // SIMICS_for_all_processors( s_count_processors, &s_max_processor_id );

  if (m_numProcs > 1) {
	  CONFIG_MULTIPROCESSOR = true;
  } else {
	  // LXL: need to hack this for multi system sim
	  if (server_client_mode ==2) {
		  CONFIG_MULTIPROCESSOR = true;
	  } else {
		  CONFIG_MULTIPROCESSOR = false;
	  }
  }

  m_mhinstalled      = false;
  m_snoop_installed  = false;
  m_sim_status = SIMSTATUS_BREAK;
  sprintf( m_sim_message_buffer, "situation nominal" );

  // establish opal ruby api
  m_opal_api = &hfa_ruby_interface;
  m_opal_api->hitCallback    = &system_t::rubyCompletedRequest;
  m_opal_api->notifyCallback = &system_t::rubyNotifyRecieve;
  m_opal_api->getInstructionCount = &system_t::rubyInstructionQuery;
  queryRubyInterface();
  connectAmber() ;

  m_sys_stat = new sys_stat_t( m_numProcs );
  m_breakpoint_table = new BreakpointTable();
  m_symtrace = NULL;

  

  // create a number of sequencers ...
  m_seq   = (pseq_t **)   malloc( sizeof(pseq_t *)*numProcs );
  m_state = (pstate_t **) malloc( sizeof(pstate_t *)*numProcs );
  m_chain = (chain_t **) malloc( sizeof(chain_t *)*numProcs );

  m_multicore_diagnosis = new multicore_diagnosis_t(this, m_state, numProcs);

  for ( int i = 0; i < numProcs; i++ ) {
    // new sequencer
#ifdef MODINIT_VERBOSE
    DEBUG_OUT("[400] pseq_t() construction proc=%d\n", i);
#endif
    // must create the state interface objects first!
    m_state[i] = new pstate_t( i , m_multicore_diagnosis);
    m_seq[i]   = new pseq_t( i , m_multicore_diagnosis);
    m_chain[i] = new chain_t( i, IWINDOW_WIN_SIZE );
  }

  // initalize other global objects
  m_trace = new ptrace_t( m_numProcs );
  dynamic_inst_t::initialize();
  exec_fn_initialize();
  
  // discover the thread pointer for these processors
  initThreadPointers();

  // global debugging messages always appear
  debugio_t::setDebugTime( 0 );
  debugio_t::setNumProcs(numProcs);
  m_ruby_api = NULL;
  m_global_cycles = 0;

	//debugio_t::populateLoopMap();
	
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("[425] system_t() finished\n");
#endif

  CORE_ID=-1;
  m_is_unknown_run=false;
  m_is_detected = false ;

#ifdef LL_DECODER
  initOpcodeExecTable();

#endif


}

//***************************************************************************
system_t::~system_t( void )
{
  // free each processor
  for ( int i = 0; i < m_numProcs; i++ ) {
    // delete processor, state pointer for proc #i
    if (m_seq[i])
      delete m_seq[i];
    if (m_state[i])
      delete m_state[i];
    if (m_chain[i])
      delete m_chain[i];
  }

  if (m_symtrace)
    delete m_symtrace;
  if (m_sys_stat)
    delete m_sys_stat;
  if (m_breakpoint_table)
    delete m_breakpoint_table;

  // delete the sequencer array
  free( m_seq );
  free( m_state );
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//***************************************************************************
static void
system_breakpoint( void *data, conf_object_t *cpu, integer_t parameter )
{
  if ( parameter != 4UL << 16 ) {
    // ignore all transaction completion calls
    /* sprintf( system_t::inst->m_sim_message_buffer,
             "other breakpoint (ignoring) 0x%llx", parameter );
    */
    return;
  }
  DEBUG_OUT("got magic break!!\n");
  sprintf( system_t::inst->m_sim_message_buffer,
           "magic breakpoint reached" );
  HALT_SIMULATION;
  return;
}

//***************************************************************************
void system_t::simulate( uint64 instrCount )
{
  //  
  // event driven simulation main loop
  //
  bool inst_count_reset = false ;
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("[1000] system_t() simulation beginning\n");
#endif
  if (CONFIG_MULTIPROCESSOR) {
    // this is multi-processor optimization: disable all processors then
    // enbable only those which should run...

    // disable all processors, so we can step one at a time
    for (int j = 0; j < m_numProcs; j++ ) {
      m_state[j]->disable();
    }
  }

  if (CONFIG_IN_SIMICS) {
    // register for magic breakpoints: 4 represents magic_call_break
    hap_type_t magic_break = SIMICS_hap_get_number("Core_Magic_Instruction");


#ifndef SIMICS_3_0
	callback_arguments_t args      = SIMICS_get_hap_arguments( magic_break, 0 );
    const char          *paramlist = SIMICS_get_callback_argument_string( args );
    if (strcmp(paramlist, "nocI" )) {
      ERROR_OUT("error: system_t::installHapHandlers: expect hap to take parameters %s. Current simics executable takes: %s\n",
                "nocI", paramlist );
      SYSTEM_EXIT;
    }
#endif	
    
	SIMICS_hap_add_callback( "Core_Magic_Instruction", (obj_hap_func_t) system_breakpoint, NULL );
  }

#if LXL_PROB_FM
  debugio_t::processFMProfile();
#endif

  for (int j = 0; j < m_numProcs; j++ ) {
    m_seq[j]->startTime();
	m_seq[j]->setStartPC() ;
#ifdef RANGE_CHECK
	m_seq[j]->readInstRangesFile() ;
#endif // RANGE_CHECK

#ifdef HARD_BOUND_BASE
	debugio_t::readObjectTable() ;
#endif // HARD_BOUND_BASE

#ifdef HARD_BOUND
	debugio_t::readObjectTable() ;
#endif // HARD_BOUND

#ifdef GEN_DATA_VALUE_INV
 // For range based invariants
	m_seq[j]->readDetectorList() ;
#endif // GEN_DATA_VALUE_INV

#ifdef CHECK_DATA_VALUE_INV
	m_seq[j]->readDetectorRanges() ;
#endif // CHECK_DATA_VALUE_INV

#ifdef CHECK_DATA_ONLY_VALUES
	m_seq[j]->readDataOnlyValuesList() ;
#endif // CHECK_DATA_ONLY_VALUES
  	

  }

 run_more_please:
  while ( m_sim_status == SIMSTATUS_OK &&
		  continueSimulation(instrCount) ) {

	  for (int j = 0; j < m_numProcs; j++ ) {
		  // cycle each of the processors one step
		  if(MULTICORE_DIAGNOSIS_ONLY) {
			  if(!m_multicore_diagnosis->dont_continue(j)) {
				  m_seq[j]->advanceCycle();
			  }
		  } else 
			  m_seq[j]->advanceCycle();
#ifdef HARD_BOUND
		  if(HARD_BOUND_WARMUP) {
			  if(debugio_t::isinWarmup() && m_seq[0]->m_stat_committed >= WARMUP_INST) {
				  debugio_t::stopWarmup() ;
			  }
		  }
#endif // HARD_BOUND
	  }

	  // increment the global cycle count...
	  m_global_cycles++;

	  //
	  // advance the ruby cache time
	  //
	  if (CONFIG_WITH_RUBY) {
		  if ( getGlobalCycle() % RUBY_CLOCK_DIVISOR == 0 )
			  m_ruby_api->advanceTime();
	  }
  }
  // DEBUG_OUT("sim status is %d\n",m_sim_status);
#ifdef LXL_SNET_RECOVERY  
  if (m_seq[0]->recovered && m_sim_status==SIMSTATUS_OK && (instrCount < m_seq[0]->m_stat_committed)) {
	  DEBUG_OUT("system got here ic %d com %d\n",instrCount,m_seq[0]->m_stat_committed);
	  
	  if (m_seq[0]->m_stat_committed < MAX_INSTR_RUN) {
		  instrCount=MAX_INSTR_RUN;
		  goto run_more_please;
	  }
  }

  if(! MULTICORE_DIAGNOSIS_ONLY)
	m_seq[0]->m_diagnosis->printDiagnosisResult();
#endif

  for (int j = 0; j < m_numProcs; j++ ) {
    m_seq[j]->stopTime();
  }
  for (int j = 0; j < m_numProcs; j++ ) {
    m_sys_stat->observeThreadSwitch( j );
  }
  // for (int j = 0; j < m_numProcs; j++ ) {
  // 	DEBUG_OUT("Simulated: processor %d, completed %lld instructions, cycle: %lld\n",
  //       	    j, m_seq[j]->m_stat_committed, m_seq[j]->getLocalCycle() );
  // }
#ifndef FIXED_INSTRUCTIONS
  for (int j = 0; j < m_numProcs; j++ ) {
    if(!inst_count_reset && m_seq[j]->getArchMismatch() && m_sim_status==SIMSTATUS_OK) {
		uint64 first_mismatch = m_seq[j]->getArchCycle();
#ifdef HARD_BOUND
		instrCount=first_mismatch+(instrCount-WARMUP_INST) ; // Reduce warmup inst as it does no faults
#else // HARD_BOUND
		instrCount=first_mismatch+(instrCount-FAULT_START_INST) ;
#endif // HARD_BOUND
		DEBUG_OUT("Adding (first_arch_mismatch_inst - 0)instructions to %lld\n",instrCount);
		inst_count_reset = true ;

		goto run_more_please;
	}
  }
#endif // FIXED_INSTRUCTIONS

  if ( m_sim_status == SIMSTATUS_OK ) {
	  for (int j = 0; j < m_numProcs; j++ ) {
		  if (m_seq[j]->getArchMismatch()) {
			  m_is_unknown_run = true;
			  break;
		  }
	  }
  }
  for (int j = 0; j < m_numProcs; j++ ) {
	  m_seq[j]->setStopPC() ;
	  FAULT_STAT_OUT("Simulated %s\n", m_seq[j]->getStartStopPC()) ;
  }
  notifyAmberDrainBuffer() ;
#ifdef BUILD_SLICE
  for (int j = 0; j < m_numProcs; j++ ) {
	  m_seq[j]->printDataSlice() ;
	  // m_seq[j]->printDDG() ;
	  // m_seq[j]->printReverseDDG() ;
  }
#endif // BUILD_SLICE

#ifdef FIND_UNIQUE_PCS
  for (int j = 0; j < m_numProcs; j++ ) {
	  m_seq[j]->printRetirePCs() ;
  }
#endif // FIND_UNIQUE_PCS

#ifdef GEN_DATA_VALUE_INV
 // For range based invariants
  for (int j = 0; j < m_numProcs; j++ ) {
	m_seq[j]->printDetectorRanges() ;
  }
#endif // GEN_DATA_VALUE_INV

}

//***************************************************************************


bool system_t::getArchMismatch(int core_id)
{
	return m_seq[core_id]->fault_stat->getArchMismatch();
}

void system_t::takeCheckpoint(int core_id)
{
	if(MULTICORE_DIAGNOSIS_ONLY)
		m_seq[core_id]->takeProcessorCheckpoint();
}

void system_t::prepareRollback(int core_id) 
{ 
	if(MULTICORE_DIAGNOSIS_ONLY)
		m_seq[core_id]->prepareRollback();
}

void system_t::postRollback(int core_id) 
{
	if(MULTICORE_DIAGNOSIS_ONLY)
		m_seq[core_id]->postRollback();
}

void system_t::clearLLB(int core_id)
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		m_seq[core_id]->getLLB()->clear();
		m_seq[core_id]->func_inst_buffer.clear();
	}
}

bool system_t::isLLBFull(int core_id)
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_seq[core_id]->getLLB()->isLLBFull()) 
			return true;
		else
			return false;
	}
	return false;
}

llb_t*  system_t::getLLB(int core_id) 
{
	if(CONCISE_VERBOSE)
		DEBUG_OUT("%d: LLB size = %d, dictionary size=%d\n", core_id, m_seq[core_id]->llb->getLLBQueueSize(), m_seq[core_id]->llb->getDictionarySize());
	return m_seq[core_id]->llb;
}

func_inst_buffer_t& system_t::getFuncLog(int core_id) 
{
	//DEBUG_OUT("%d: func buffer size = %d\n", core_id, m_seq[core_id]->func_inst_buffer.size());
	return m_seq[core_id]->func_inst_buffer;
}

void system_t::putLLB(llb_t* load_buf, int core_id) 
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		m_seq[core_id]->llb->clear();
		int num_loads=0;

		m_seq[core_id]->llb->copyLLB(load_buf);
	}
}

bool system_t::compareState(core_state_t *ps, int core_id)
{
	return m_seq[core_id]->compareState(ps);
}

void system_t::putFuncLog(func_inst_buffer_t func_inst_buf, int core_id) 
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		m_seq[core_id]->func_inst_buffer.clear();

		m_seq[core_id]->func_inst_buffer.insert(m_seq[core_id]->func_inst_buffer.end(),func_inst_buf.begin(),  func_inst_buf.end());
		//DEBUG_OUT("Copied %d elements, func inst buffer to core_id=%d \n",func_inst_buf.size(), core_id);
	}
}

uint64 system_t::getLocalCycle(int core_id)
{
	return m_seq[core_id]->getLocalCycle();
}	

//right now, no one is calling this function
uint64 system_t::minCommittedInstr()
{
	uint64 min = m_seq[0]->m_stat_committed;

	for(int j=1; j<m_numProcs; j++) {
		if(m_seq[j]->m_stat_committed < min)
			min = m_seq[j]->m_stat_committed;
	}
	return min;
}

void system_t::disableInterrupts(int core_id)
{
	m_state[core_id]->disableInterrupts();
}

bool system_t::continueSimulation(uint64 instrCount)
{
#if MULTICORE_DIAGNOSIS_ONLY
	if(!m_multicore_diagnosis->hasDiagnosisStarted()) 	//if diagnosis hasnt started then stop after instrCount
#endif	
	{
		if(instrCount > m_seq[0]->m_stat_committed)
			return true;

		//trying to optimize a bit, by not checking for the number of commited
		//instrucitons on all the cores in every cycle.
		for(int j=1; j<m_numProcs; j++) {
			if(instrCount > m_seq[j]->m_stat_committed)
				return true;
		}

		return false;
	}
#ifdef REPLAY_PHASE
	//if diagnosis has started and we are doing replay phase then stop after 2*instrCount
	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(2*instrCount > m_seq[0]->m_stat_committed)
			return true;

		//trying to optimize a bit, by not checking for the number of commited
		//instrucitons on all the cores in every cycle.
		for(int j=1; j<m_numProcs; j++) {
			if(2*instrCount > m_seq[j]->m_stat_committed)
				return true;
		}

		return false;
	}
#endif	
	return true;
}

void system_t::printBuildParameters( void )
{
#ifdef FAKE_RUBY
  DEBUG_OUT( "error: macro FAKE_RUBY should never defined in opal.\n");
  SIM_HALT;
#endif

#ifdef MT_OPAL
  DEBUG_OUT( "error: macro MT_OPAL should never defined in opal.\n");
  SIM_HALT;
#endif

#ifdef EXPENSIVE_ASSERTIONS
  DEBUG_OUT( "warning: macro EXPENSIVE_ASSERTIONS defined: run time may be slow\n");
#endif

#ifdef CHECK_REGFILE
  DEBUG_OUT( "warning: macro CHECK_REGFILE defined: run time may be slow\n");
#endif
}

//***************************************************************************

void system_t::detectedSymp( void )
{
	for(int j=0; j<m_numProcs; j++) {
		m_seq[j]->simicsDetection() ;
	}
}

void system_t::breakSimulation( void )
{
  HALT_SIMULATION;
  sprintf( m_sim_message_buffer, "external call to break_simulation\n");
}

//***************************************************************************
void system_t::printStats( void )
{
  for (int j = 0; j < m_numProcs; j++ ) {
    m_seq[j]->printStats();
  }
  m_sys_stat->printStats();
}

//***************************************************************************
void system_t::printInflight( void )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    DEBUG_OUT("SEQUENCER: %d\n", i);
    m_seq[i]->printInflight();
  }
}

//***************************************************************************
void system_t::printMemoryStats( void )
{
  out_intf_t *log = m_seq[0];
  if (log == NULL) {
    return;
  }
  dynamic_inst_t::m_myalloc.print( log );
  load_inst_t::m_myalloc.print( log );
  store_inst_t::m_myalloc.print( log );
  atomic_inst_t::m_myalloc.print( log );
  prefetch_inst_t::m_myalloc.print( log );
  control_inst_t::m_myalloc.print( log );

  ruby_request_t::m_myalloc.print( log );
  flow_inst_t::m_myalloc.print( log );

  for ( int i = 0; i < m_numProcs; i++ ) {
    if (m_chain[i]->isInitialized()) {
      m_chain[i]->printMemoryStats();
    }
  }
}

//***************************************************************************
tick_t system_t::getGlobalCycle( void )
{
  return ( m_global_cycles );
}

//***************************************************************************
void   system_t::stepInorder( uint64 instrCount )
{
  DEBUG_OUT("step inorder: called: %d\n", instrCount );
  for ( uint64 k = 0; k < instrCount; k++ ) {
    for ( int i = 0; i < m_numProcs; i++ ) {
      m_seq[i]->stepInorder();
    }
  }
}

//***************************************************************************
void system_t::openLogfiles( char *logname )
{
  debugio_t::openLog( logname );
  printLogHeader();

  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->setLog( debugio_t::getLog() );
    m_seq[i]->installInterfaces();
    m_seq[i]->registerCheckpoint();
  }
  setupBreakpointDispatch( false );
  installHapHandlers();
  installExceptionHandler( SIMSTATUS_OK );
  m_sim_status = SIMSTATUS_OK;
}

//***************************************************************************
void system_t::closeLogfiles( void )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->removeInterfaces();
  }

  removeHapHandlers();
  removeExceptionHandler();
  removeMemoryHierarchy();
  
  debugio_t::closeLog();
  debugio_t::closeFaultLog();
  m_sim_status = SIMSTATUS_BREAK;
}

// Functions for Fault related things
void system_t::openFaultLog( char *logname )
{
  debugio_t::openFaultLog(logname) ;
}

void system_t::setChkptInterval(int interval)
{ 
  for (int j = 0; j < m_numProcs; j++ ) {
	  m_state[j]->setChkptInterval(interval) ;
  }
}

void system_t::startCheckpointing()
{
  if(server_client_mode != server_start_num) {
	  ERROR_OUT("start-checkpointing is only tested on server-client\n") ;
	  exit(1) ;
  }
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->startCheckpointing();
  }
}

void system_t::setServerClient( int mode ) 
{
	// Server is 0, client is 1
	if (mode == 0) {
		server_client_mode = server_start_num;
	} else if (mode == 1) {
		server_client_mode = client_start_num;

	} else {
		DEBUG_OUT("Invalid server-client-mode %d\n",mode);
		exit(0);

	}
	assert(server_client_mode == 0);
}

void system_t::connectAmber(void)
{
	conf_object_t *amber = SIMICS_get_object("amber0") ;
	if(amber!=NULL) {
		m_amber_api = (amber_api_t*) SIMICS_get_interface(amber, "amber-api") ;
		if(!m_amber_api) {
			ERROR_OUT("Error: Amber doesn't implement the amber-api interface\n") ;
		} else {
			DEBUG_OUT("Connected amber0 to opal0\n") ;
		}
	} else {
		DEBUG_OUT("No amber0 module found\n") ;
	}
}

void system_t::closeFaultLog(void)
{
  debugio_t::closeFaultLog() ;
}

void system_t::setFaultType( int t )
{
	if (CORE_ID<0) {
		DEBUG_OUT("Core id not set yet. Please use 'faulty-core <core_id>' first.\n");
		exit(0);
	}
	m_seq[CORE_ID]->setFaultType(t) ;
}

void system_t::setFaultBit( int b )
{
	if (CORE_ID<0) {
		DEBUG_OUT("Core id not set yet. Please use 'faulty-core <core_id>' first.\n");
		exit(0);
	}
	m_seq[CORE_ID]->setFaultBit(b) ;
}

void  system_t::setFaultStuckat( int s )
{
	if (CORE_ID<0) {
		DEBUG_OUT("Core id not set yet. Please use 'faulty-core <core_id>' first.\n");
		exit(0);
	}
	m_seq[CORE_ID]->setFaultStuckat(s) ;
}

void  system_t::setFaultyReg( int r )
{
	if (CORE_ID<0) {
		DEBUG_OUT("Core id not set yet. Please use 'faulty-core <core_id>' first.\n");
		exit(0);
	}
	m_seq[CORE_ID]->setFaultyReg(r) ;
}

void  system_t::setSrcDestReg( int sd )
{
	if (CORE_ID<0) {
		DEBUG_OUT("Core id not set yet. Please use 'faulty-core <core_id>' first.\n");
		exit(0);
	}
	DEBUG_OUT("Src_dest set to %d \n", sd);
	m_seq[CORE_ID]->setSrcDestReg(sd) ;
}
void  system_t::setStartLogging( int sd )
{
	if (CORE_ID<0) {
		DEBUG_OUT("Core id not set yet. Please use 'faulty-core <core_id>' first.\n");
		exit(0);
	}
	m_seq[CORE_ID]->setStartLogging(sd) ;
}
void  system_t::setComparePoint( int sd )
{
	if (CORE_ID<0) {
		DEBUG_OUT("Core id not set yet. Please use 'faulty-core <core_id>' first.\n");
		exit(0);
	}
	m_seq[CORE_ID]->setComparePoint(sd) ;
}
//----------------------------------------------- GATE LEVEL SAFSIM ----------------------------------------------//
/// Sets the faulty gateId
void  system_t::setFaultyGateId( int s )
{
	m_seq[CORE_ID]->setFaultyGateId(s);
}

/// Sets the faulty fanoutId
void  system_t::setFaultyFanoutId( int s )
{
	m_seq[CORE_ID]->setFaultyFanoutId(s);
}

/// Print saf statistics
/*void system_t::printSafFaultStats( )
{
	m_seq[CORE_ID]->printSafFaultStats() ;
}
*/
// NCVLOG RELATED

/// Sets the faulty machine (faulty net) name
void  system_t::setFaultyMachine( string s )
{
	m_seq[CORE_ID]->setFaultyMachine(s);
}

void  system_t::setStimPipe( string s )
{
	m_seq[CORE_ID]->setStimPipe(s);
}

void  system_t::setResponsePipe( string s )
{
	m_seq[CORE_ID]->setResponsePipe(s);
}

// ------------GATE LEVEL commands ----------//

void  system_t::setFaultyCore( int c )
{
	CORE_ID=c;
	debugio_t::setFaultyCore(c);
}

void  system_t::setInfLoopCycle( uint64 cycle )
{
	if (CORE_ID<0) {
		DEBUG_OUT("Core id not set yet. Please use 'faulty-core <core_id>' first.\n");
		exit(0);
	}
	m_seq[CORE_ID]->setInfLoopCycle(cycle) ;
}

void system_t::setFaultInjInst(int value)
{
	if (CORE_ID<0) {
		DEBUG_OUT("Core id not set yet. Please use 'faulty-core <core_id>' first.\n");
		exit(0);
	}
#ifdef HARD_BOUND 
	if(HARD_BOUND_WARMUP) {
		value += WARMUP_INST ;
		debugio_t::startWarmup() ;
	}
#else // HARD_BOUND
	value += FAULT_START_INST ;
	if(FAULT_START_INST!=0) {
		DEBUG_OUT("Resetting fault-inj-inst to %d\n", value) ;
	}
#endif  // HARD_BOUND
	m_seq[CORE_ID]->setFaultInjInst(value);
	if(MULTICORE_DIAGNOSIS_ONLY) {
#ifdef REPLAY_PHASE
		m_seq[CORE_ID]->setFaultInjInst(value);
#else
		m_seq[CORE_ID]->setFaultInjInst(1);
#endif
	}
}

void system_t::printFaultStats( )
{
	for (int i = 0; i < m_numProcs; i++) 
		m_seq[i]->printFaultStats() ;

#ifdef TRACK_MEM_SHARING
	debugio_t::out_info("APP:size of map = %d \n",debugio_t::mapSize(0));
	debugio_t::printSharedAddresses(0);
	debugio_t::out_info("OS:size of map = %d \n",debugio_t::mapSize(1));
	debugio_t::printSharedAddresses(1);
#endif
}

/** print compile and run time information into the log file */
//**************************************************************************
void system_t::printLogHeader( void )
{
  FILE *logfp = debugio_t::getLog();

#ifdef LXL_NO_OPAL_CONFIG
  return;
#endif

  if (logfp == NULL) {
    DEBUG_OUT("error: log file not open: not printing header to file\n");
    hfa_list_param( NULL );
    return;
  }

  if (system_t::inst->m_configFile != NULL)
    fprintf( logfp, "# Configuration File named       : %s\n", system_t::inst->m_configFile );
  fprintf( logfp, "# system status: warmup          : %d\n", system_t::inst->isWarmingUp() );
  fprintf( logfp, "# system status: simulation      : %d\n", system_t::inst->isSimulating() );
  fprintf( logfp, "# system status: trace           : %d\n", system_t::inst->isTracing() );
  
  fprintf( logfp, "# IWINDOW_ROB_SIZE:                %d\n", IWINDOW_ROB_SIZE);
  fprintf( logfp, "# IWINDOW_WIN_SIZE:                %d\n", IWINDOW_WIN_SIZE);
  
  fprintf( logfp, "# Cache model:\n");
  if(!CONFIG_WITH_RUBY) {
    fprintf( logfp, "# L1 data cache: block: %dB, %dx assoc, %d sets. %dB total.\n",
             1 << DL1_BLOCK_BITS, DL1_ASSOC, DL1_NUM_SETS,
             (1 << DL1_BLOCK_BITS) * DL1_ASSOC * DL1_NUM_SETS );
    fprintf( logfp, "# DL1_MSHR_ENTRIES:            %d\n", DL1_MSHR_ENTRIES);
    fprintf( logfp, "# DL1_STREAM_BUFFERS:          %d\n", DL1_STREAM_BUFFERS);
    fprintf( logfp, "# DL1_IDEAL:                   %d\n", DL1_IDEAL);
  
    fprintf( logfp, "# L1 inst cache: block: %dB, %dx assoc, %d sets. %dB total.\n",
             1 << IL1_BLOCK_BITS, IL1_ASSOC, IL1_NUM_SETS,
             (1 << IL1_BLOCK_BITS) * IL1_ASSOC * IL1_NUM_SETS );
    fprintf( logfp, "# IL1_MSHR_ENTRIES:            %d\n", IL1_MSHR_ENTRIES);
    fprintf( logfp, "# IL1_MSHR_QUEUE_SIZE:         %d\n", IL1_MSHR_QUEUE_SIZE);
    fprintf( logfp, "# IL1_MSHR_QUEUE_ISSUE_WIDTH:  %d\n", IL1_MSHR_QUEUE_ISSUE_WIDTH);
    fprintf( logfp, "# IL1_STREAM_BUFFERS:          %d\n", IL1_STREAM_BUFFERS);
    fprintf( logfp, "# IL1_IDEAL:                   %d\n", IL1_IDEAL);
  
    fprintf( logfp, "# L2 unified cache: block: %dB, %dx assoc, %d sets. %dB total.\n",
             1 << L2_BLOCK_BITS, L2_ASSOC, (1 << L2_SET_BITS),
             (1 << L2_BLOCK_BITS) * L2_ASSOC * (1 << L2_SET_BITS) );
    fprintf( logfp, "# L2_MSHR_ENTRIES:            %d\n", L2_MSHR_ENTRIES);
    fprintf( logfp, "# L2_STREAM_BUFFERS:          %d\n", L2_STREAM_BUFFERS);
    fprintf( logfp, "# L2_LATENCY:                 %d\n", L2_LATENCY);
    fprintf( logfp, "# L2_IDEAL:                   %d\n", L2_IDEAL);
    fprintf( logfp, "# MEMORY_DRAM_LATENCY:        %d\n", MEMORY_DRAM_LATENCY);
    fprintf( logfp, "# L1_FILL_BUS_CYCLES:         %d\n", L1_FILL_BUS_CYCLES);
    fprintf( logfp, "# L2_FILL_BUS_CYCLES:         %d\n", L2_FILL_BUS_CYCLES);
  } else {
    fprintf( logfp, "# RUBY_CLOCK_DIVISOR          %d\n", RUBY_CLOCK_DIVISOR );
  }
  fprintf( logfp, "\n");

  fprintf( logfp, "# Branch Predictor model:\n");  
  fprintf( logfp, "#   Predictor Type: %s\n", BRANCHPRED_TYPE);
  fprintf( logfp, "#   mispenalty    : %d\n", BRANCHPRED_MISPRED_PENALTY);
  fprintf( logfp, "\n");

  fprintf( logfp, "#   YAGS Predictor:\n");
  fprintf( logfp, "#     PHT table         : %d entries\n", 1<<BRANCHPRED_PHT_BITS);
  fprintf( logfp, "#     Exception table   : %d entries\n", 1<<BRANCHPRED_EXCEPTION_BITS);
  fprintf( logfp, "#     Tag bits          : %d bits\n", BRANCHPRED_TAG_BITS);
  fprintf( logfp, "#     size = (2*PHT + 2*Except*(TAG+2))/8 = %d bytes\n",
           (2*(1 << BRANCHPRED_PHT_BITS) + 2 * (1 << BRANCHPRED_EXCEPTION_BITS) * (BRANCHPRED_TAG_BITS+2))/8);
  fprintf( logfp, "\n" );
  
  fprintf( logfp, "# Cascaded Indirect Predictor:\n");
  fprintf( logfp, "#   simple size   : %d entries\n", 1 << CAS_TABLE_BITS );
  fprintf( logfp, "#   except size   : %d entries\n", 1 << CAS_EXCEPT_BITS );
  fprintf( logfp, "#   tag size      : %d bits\n", 1 << CAS_EXCEPT_SHIFT );
  fprintf( logfp, "\n" );

  fprintf( logfp, "# RAS size        : %d entries\n", 1 << RAS_BITS );
  fprintf( logfp, "# RAS exception   : %d entries\n", 1 << RAS_EXCEPTION_TABLE_BITS );
  fprintf( logfp, "\n");

  fprintf( logfp, "# CONFIG_IREG_PHYSICAL           : %d\n", CONFIG_IREG_PHYSICAL);
  fprintf( logfp, "# CONFIG_IREG_LOGICAL            : %d\n", CONFIG_IREG_LOGICAL);
  fprintf( logfp, "# CONFIG_CCREG_PHYSICAL          : %d\n", CONFIG_CCREG_PHYSICAL);
  fprintf( logfp, "# CONFIG_NUM_CONTROL_SETS        : %d\n", CONFIG_NUM_CONTROL_SETS);
  fprintf( logfp, "\n");

  fprintf( logfp, "# Maximum trap level (MAXTL)     : %d\n", MAXTL);
  fprintf( logfp, "# Number of windows  (NUMWINDOWS): %d\n", NWINDOWS);
  fprintf( logfp, "# Num logical integer registers (IREG_LOGICAL)   : %d\n", CONFIG_IREG_LOGICAL );
  fprintf( logfp, "# Num logical fp registers      (FPREG_LOGICAL)  : %d\n", CONFIG_FPREG_LOGICAL );
  fprintf( logfp, "# Num logical cc registers      (CCREG_LOGICAL)  : %d\n", CONFIG_FPREG_LOGICAL );

  fprintf( logfp, "# Pipeline model:\n");
  fprintf( logfp, "#    I$ latency              : %d\n", ICACHE_CYCLE );
  fprintf( logfp, "#    fetch pass cache line   : %d\n", FETCH_PASS_CACHE_LINE );
  fprintf( logfp, "#    fetch pass taken branch : %d\n", FETCH_PASS_TAKEN_BRANCH );
  fprintf( logfp, "#    fetch stages            : %d\n", FETCH_STAGES );
  fprintf( logfp, "#    decode stages           : %d\n", DECODE_STAGES );
  fprintf( logfp, "#    retire stages           : %d\n", RETIRE_STAGES );
  fprintf( logfp, "#    max fetch    / cycle: %d\n", MAX_FETCH );
  fprintf( logfp, "#    max decode   / cycle: %d\n", MAX_DECODE );
  fprintf( logfp, "#    max dispatch / cycle: %d\n", MAX_DISPATCH );
  fprintf( logfp, "#    max execute  / cycle: %d\n", MAX_EXECUTE );
  fprintf( logfp, "#    max retire   / cycle: %d\n", MAX_RETIRE );

  fprintf( logfp, "# FU TYPE             #   LATENCY\n");
  for (uint32 i = 0; i < sizeof(CONFIG_NUM_ALUS)/sizeof(char); i++ ) {
    fprintf( logfp, "# %-12s: %8d %8d\n",
             pstate_t::fu_type_menomic( (fu_type_t) i ),
             CONFIG_NUM_ALUS[i], CONFIG_ALU_LATENCY[i] );
  }

  fprintf( logfp, "#    autogenerated listing of all parameters\n");
  hfa_list_param( logfp );
}

/*------------------------------------------------------------------------*/
/* Breakpoint methods                                                     */
/*------------------------------------------------------------------------*/

//**************************************************************************
void system_t::haltSimulation( void )
{
  m_sim_status = SIMSTATUS_BREAK;

  // hanging m_is_detected such that it is set with a condition
  // m_is_detected = true ;
  if(!debugio_t::isMasked())
	m_is_detected = true;
}

//**************************************************************************
void system_t::systemExit( void )
{
  exit(1);
}

//**************************************************************************
void system_t::postedMagicBreak( void )
{
  int32 id = SIMICS_get_current_proc_no();
  if (m_symtrace) {
    m_symtrace->transactionCompletes( id );
  }
  m_sys_stat->observeTransactionComplete( id );
}

//**************************************************************************
void system_t::postedBreakpoint( uint32 id, uint32 access )
{
  // search the breakpoint table for this id
  BreakpointTable::iterator iter;
  iter = m_breakpoint_table->find(id);
  if (iter == m_breakpoint_table->end()) {
    //DEBUG_OUT("Posted breakpoint: %u. Unable to find in dispatch table.\n");
    return;
  }
  breakpoint_action_t *bp = iter->second;
  breakpointf_t pmf = bp->action;
  (this->*pmf)( bp->address, (access_t) access, bp->user_data );
}

//**************************************************************************
void system_t::setSimicsBreakpoint( la_t address, void *user_data )
{
  setSimicsBreakpoint( address, Sim_Break_Virtual, Sim_Access_Execute,
                       &system_t::breakpoint_print, user_data );
}

//**************************************************************************
void system_t::setSimicsBreakpoint( la_t address, breakpoint_kind_t breaktype,
                                    access_t access, breakpointf_t action_f,
                                    void *user_data )
{
  breakpoint_action_t *bp = new breakpoint_action_t();
  bp->address = address;
  bp->action  = action_f;
  bp->user_data = user_data;
  breakpoint_id_t id;
  if (breaktype == Sim_Break_Virtual) {
    id = SIMICS_breakpoint( SIMICS_get_object("primary-context"),
                         breaktype, access, address,
    // 4 == length of 32-bit intstr in bytes. 0 == always trigger breakpoint
                         4, 0 );
  } else if (breaktype == Sim_Break_Physical) {
    id = SIMICS_breakpoint( SIMICS_get_object("phys_mem0"),
                         breaktype, access, address,
    // 4 == length of 32-bit intstr in bytes. 0 == always trigger breakpoint
                         4, 0 );
    
  } else {
    ERROR_OUT("setBreakpoint: unknown break type = %d\n", breaktype);
    SIM_HALT;
  }
  ASSERT( m_breakpoint_table->find(id) == m_breakpoint_table->end() );
  (*m_breakpoint_table)[id] = bp;
}

//**************************************************************************
void system_t::breakpoint_print( uint64 address, access_t access,
                                 void *name )
{
  DEBUG_OUT( "\t0x%llx %s\n", address, (char *) name );
}

//**************************************************************************
void system_t::breakpoint_switch( uint64 address, access_t access, void *data )
{
#if 0
  // typically: resume() 0x1002c824 or 0x1002c8c4
  DEBUG_OUT("-- observing switch: PC: 0x%0llx\n",
            m_state[(int) data]->getControl( CONTROL_PC ));
#endif
  m_sys_stat->observeThreadSwitch( (int) data );
}

//**************************************************************************
void system_t::breakpoint_os_resume( uint64 address, access_t access,
                                     void *unused )
{
  // find the cpu for the current processor
  int cpuno = SIMICS_get_current_proc_no();

  pstate_t *state = system_t::inst->m_state[cpuno];
  int cwp         = state->getControl( CONTROL_CWP );
  ireg_t threadid = state->getIntWp( 9, cwp );

  // set the current process thread id
  if (m_symtrace) {
    m_symtrace->threadSwitch( cpuno, threadid );
  }
}

//**************************************************************************
void system_t::breakpoint_memcpy( uint64 address, access_t access,
                                  void *unused )
{
  // find the cpu for the current processor
  int cpuno = SIMICS_get_current_proc_no();

  pstate_t *state = system_t::inst->m_state[cpuno];
  int cwp         = state->getControl( CONTROL_CWP );
  ireg_t o0 = state->getIntWp( 8, cwp );
  ireg_t o1 = state->getIntWp( 9, cwp );
  ireg_t o2 = state->getIntWp(10, cwp );
  
  // o0 is src
  // o1 is dest
  // o2 is size
  DEBUG_OUT( "0x%0llx 0x%0llx %lld\n", o0, o1, o2 );
}

//**************************************************************************
void system_t::setupBreakpointDispatch( bool symbolTracing )
{
  if (!CONFIG_IN_SIMICS)
    return;

  // install the core breakpoint handler
  hap_type_t break_hap = SIMICS_hap_get_number( "Core_Breakpoint" );

#ifndef SIMICS_3_0
  // verify the type safety of this call
  callback_arguments_t args      = SIMICS_get_hap_arguments( break_hap, 0 );
  const char          *paramlist = SIMICS_get_callback_argument_string( args );
  if (strcmp(paramlist, "noIIvI" )) {
    ERROR_OUT("error: system_t::setupBreakpointDispatch: expect hap to take parameters %s. Current simics executable takes: %s\n",
              "noIIvI", paramlist );
    SYSTEM_EXIT;
  }
#endif

  int rv = SIMICS_hap_add_callback( "Core_Breakpoint",
                                     (obj_hap_func_t) &system_break_handler,
                                     (void *) this );
  ASSERT( rv != -1 );

  if (symbolTracing) {
    // Sets a breakpoint in resume(), after the thread ID is loaded in %o1
    // How to find the hardcoded constant for other Solaris versions is
    // described in multifacet/docs/symbol-table.html.  
    setSimicsBreakpoint( 0x102a984, Sim_Break_Virtual, Sim_Access_Execute,
                         &system_t::breakpoint_os_resume, NULL );
    setSimicsBreakpoint( 0xf93884e0, Sim_Break_Physical, Sim_Access_Execute,
                         &system_t::breakpoint_memcpy, NULL );
  }
  
#if 0
  // The following examples apply to (Solaris 7) SunFire checkpoints
  setSimicsBreakpoint( 0x1002c794, Sim_Break_Virtual, Sim_Access_Execute,
                       &system_t::breakpoint_os_resume, NULL );

  // add a set of functions to break on-- dispatching them to simple functions
  setSimicsBreakpoint( 0x10034c20,
                       (void *) "mutex_enter" );
  setSimicsBreakpoint( 0x10034c80,
                       (void *) "mutex_tryenter" );
  setSimicsBreakpoint( 0x10034ca0,
                       (void *) "mutex_exit" );
  setSimicsBreakpoint( 0x10034d00,
                       (void *) "rw_enter" );
  setSimicsBreakpoint( 0x100480d4,
                       (void *) "rw_tryenter" );
  setSimicsBreakpoint( 0x10034d60,
                       (void *) "rw_exit" );
  setSimicsBreakpoint( 0x100779a0,
                       (void *) "cv_wait" );
  setSimicsBreakpoint( 0x100779f0,
                       (void *) "cv_timedwait" );
  setSimicsBreakpoint( 0x10077aac,
                       (void *) "cv_wait_sig" );
  setSimicsBreakpoint( 0x10103bb8,
                       (void *) "turnstile_block" );
  setSimicsBreakpoint( 0x7f81ad30,
                       (void *) "turnstile_block" );
  setSimicsBreakpoint( 0x10034b7c,
                       (void *) "disp_lock_enter" );
  setSimicsBreakpoint( 0x10034bc8,
                       (void *) "disp_lock_exit" );
  
  for (int32 i = 0; i < m_numProcs; i++ ) {
    setSimicsBreakpoint( m_seq[i]->getThreadPhysAddress(), Sim_Break_Physical,
                         Sim_Access_Write, &system_t::breakpoint_switch,
                         (void *) i );
  }
#endif
}

/*------------------------------------------------------------------------*/
/* Tracing methods                                                        */
/*------------------------------------------------------------------------*/

//***************************************************************************
set_error_t system_t::commandSetDispatch( attr_value_t *val )
{
  // count the number of arguments in the command
  if (val->kind != Sim_Val_List) {
    ERROR_OUT("system: commandSetDispatch: non-list passed to trace subcommand.\n");
    return Sim_Set_Illegal_Value;
  }

  uint32 list_size       = val->u.list.size;
  attr_value_t  attr_fn  = val->u.list.vector[0];
  if (attr_fn.kind != Sim_Val_String) {
    ERROR_OUT("system: commandSetDispatch: non-string command\n");
    return Sim_Set_Illegal_Value;
  }
  
  const char *command = attr_fn.u.string;
  if (!strcmp( command, "start" )) {

    if (!isRubyLoaded()) {
      ERROR_OUT("system: commandSetDispatch: ruby is not loaded. unable to start timing.\n");
      return Sim_Set_Illegal_Value;
    }

    // open, initialize trace file
    if (list_size != 2) {
      ERROR_OUT("system: commandSetDispatch: need more arguments for start command (got %d)\n", list_size);
      return Sim_Set_Illegal_Value;
    }

    attr_value_t attr_filename = val->u.list.vector[1];
    if (attr_filename.kind != Sim_Val_String) {
      ERROR_OUT("system: commandSetDispatch: need string argument for start command.\n");
      return Sim_Set_Illegal_Value;
    }

    const char *filename = attr_filename.u.string;
    m_trace->writeTrace( filename );
    // install tracing interfaces
    system_post_tick();

    installMemoryHierarchy( SIMSTATUS_MLP_TRACE );
    installExceptionHandler( SIMSTATUS_MLP_TRACE );

  } else if (!strcmp( command, "start-depend" )) {
    if (!isRubyLoaded()) {
      ERROR_OUT("system: commandSetDispatch: ruby is not loaded. unable to start timing.\n");
      return Sim_Set_Illegal_Value;
    }
    // allocate each chain analyzer
    for (int i = 0; i < m_numProcs; i++) {
      m_chain[i]->allocate();
    }

    // connect the chain analyzer to the ptrace object
    m_trace->onlineAnalysis( chain_t::chain_inst_consumer,
                             chain_t::chain_mem_consumer );

    // install tracing interfaces
    system_post_tick();
    installMemoryHierarchy( SIMSTATUS_MLP_TRACE );
    installExceptionHandler( SIMSTATUS_MLP_TRACE );


  } else if (!strcmp( command, "start-symbol" )) {
    m_symtrace = new symtrace_t( m_numProcs );

    // install a memory hierarchy
    //    (to build page table & monitor thread memory accesses)
    installMemoryHierarchy( SIMSTATUS_SYMBOL_MODE );

    // post a breakpoint to determine which threads are running
    setupBreakpointDispatch( true );

    // install a transaction counting metric
    installHapHandlers();
    
  } else if (!strcmp( command, "print-symbol" )) {
    if (m_symtrace) {
      m_symtrace->printStats();
    }
    
  } else if (!strncmp( command, "open-log", strlen("open-log") )) {
    // read the filenae from the command string
    int32 len = strlen("open-log");
    char  filename[FILEIO_MAX_FILENAME];

    strcpy( filename, &command[len + 1] );
    debugio_t::openLog( filename );
    printLogHeader();
    for ( int i = 0; i < m_numProcs; i++ ) {
      m_seq[i]->setLog( debugio_t::getLog() );
    }

  } else if (!strcmp( command, "print-depend" )) {
    for (int i = 0; i < m_numProcs; i++) {
      m_chain[i]->printStats();
    }

  } else if (!strcmp( command, "print-memory" )) {
    system_t::inst->printMemoryStats();

  } else if (!strcmp( command, "start-time" )) {
    for (int i = 0; i < m_numProcs; i++) {
      m_chain[i]->startTimer();
    }
  } else if (!strcmp( command, "run" )) {
    // run a trace
    DEBUG_OUT("system: commandSetDispatch: running...\n");
    
  } else if (!strcmp( command, "stop" )) {
    DEBUG_OUT("system: commandSetDispatch: stopping...\n");

    m_trace->closeTrace();
    debugio_t::closeLog();

  } else {
    ERROR_OUT("error: system: commandSetDispatch: unrecognized trace subcommand: %s.\n", command);
    return Sim_Set_Illegal_Value;
  }
  return Sim_Set_Ok;
}

//***************************************************************************
void system_t::openTrace( char *tracename )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->installInterfaces();
    m_seq[i]->openTrace( tracename );
  }
  installHapHandlers();
  m_sim_status = SIMSTATUS_TRACING;
}

//***************************************************************************
void system_t::writeTraceStep( void )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->writeTraceStep();
  }
}

//***************************************************************************
void system_t::writeSkipTraceStep( void )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->writeSkipTraceStep();
  }
}

//***************************************************************************
void system_t::closeTrace( void )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->closeTrace();
    m_seq[i]->removeInterfaces();
    // doesnt work 7/13/00
    // m_seq[i]->writeInstructionTable( NULL );
  }
  removeHapHandlers();
  removeMemoryHierarchy();
  m_sim_status = SIMSTATUS_BREAK;
}

/*------------------------------------------------------------------------*/
/* Branch Trace methods                                                   */
/*------------------------------------------------------------------------*/

//***************************************************************************
void system_t::openBranchTrace( char *tracename )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->installInterfaces();
    m_seq[i]->openBranchTrace( tracename );
  }
  installHapHandlers();
  m_sim_status = SIMSTATUS_TRACING;
}

//***************************************************************************
void system_t::writeBranchStep( void )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->writeBranchStep();
  }
}

//***************************************************************************
bool system_t::writeBranchNextFile( void )
{
  bool  rc = false;
  for ( int i = 0; i < m_numProcs; i++ ) {
    rc = m_seq[i]->writeBranchNextFile();
  }
  return (rc);
}

//***************************************************************************
void system_t::closeBranchTrace( void )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->closeTrace();
    m_seq[i]->removeInterfaces();
    // doesnt work 7/13/00
    // m_seq[i]->writeInstructionTable( NULL );
  }
  removeHapHandlers();
  removeMemoryHierarchy();
  m_sim_status = SIMSTATUS_BREAK;
}

/** Prints the contents of a trace
 */
//***************************************************************************
void system_t::printTrace( void )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->validateTrace();
  }
}

/** builds and save the instruction mappings
 */
//***************************************************************************
void system_t::saveInstructionMap( char *baseFilename, char *traceFilename )
{
  char          filename[FILEIO_MAX_FILENAME];
  abstract_pc_t apc;
  la_t          program_counter;
  pa_t          physical_pc;
  unsigned int  instr;
  int           i;
  bool          success = true;
  static_inst_t *query_instr;

  // attach the trace files to each sequencer
  for ( i = 0; i < m_numProcs; i++ ) {

    // attach to this trace
    sprintf( filename, "%s%s", baseFilename, traceFilename );
    m_seq[i]->attachTrace( filename, false );
    //  m_seq[i]->validateTrace();

    // read the trace
    while (success) {
      success = m_seq[i]->readTraceStep( program_counter,
                                         physical_pc, instr );
      apc.pc = program_counter;
      apc.tl = m_state[i]->getControl(CONTROL_TL);
      
      // add this PC, instruction to the ipage map
      if (success) {
        
        // only insert the instruction if hasn't already been inserted...
        if ( !m_seq[i]->queryInstruction( physical_pc, query_instr) ||
             query_instr->getInst() != instr ) {
          m_seq[i]->insertInstruction( physical_pc, instr );
        }
      }
    }
    m_seq[i]->closeTrace();
    m_seq[i]->printIpage( false );
    
    sprintf( filename, "%s%s%s", baseFilename, "imap-", traceFilename);
    m_seq[i]->writeInstructionTable( filename );
  }
}

/** builds and save the instruction mappings
 */
//***************************************************************************
void system_t::readInstructionMap( char *baseFilename, char *traceFilename,
                                   bool validate )
{
  char          *hyphen_ptr;
  char           filename[MAXNAMLEN];

  // attach the trace files to each sequencer
  for ( int i = 0; i < m_numProcs; i++ ) {
    // search the directory entry for things matching imap-*
    int   nread;
#ifdef USE_DIRENT
    DIR           *basedp = opendir( baseFilename );
    struct dirent *mydent;
    if ( basedp == NULL ) {
      ERROR_OUT("error: unable to open directory %s\n", baseFilename);
      return;
    }
    // initialize nread to a bogus value,
    //    we will break out of the loop when were done
    nread = 1000;
#elif 0
    int    fd = open( baseFilename, O_RDONLY, 0 );
    int    dircount = 100;
    struct dirent *dirbuf = (struct dirent *) malloc(dircount*sizeof(struct dirent));

    if (fd < 0) {
      ERROR_OUT("error: unable to open directory %s\n", baseFilename);
      return;
    }
    int nread = getdents( fd, dirbuf, dircount*sizeof(struct dirent) );
    if (nread < 0) {
      ERROR_OUT("error: getdents call fails! nread = %d\n", nread);
      return;
    }
    nread = nread / sizeof(struct dirent);
    for (int j = 0; j < nread; j++) {
      ERROR_OUT("%d file %s\n", j, dirbuf[j].d_name);
    }
#else
    struct dirent **namelist;
    nread = scandir( baseFilename, &namelist, selectImapFiles,
                     NULL );
    if (nread == -1) {
      ERROR_OUT("error: unable to access directory: %s\n", baseFilename);      
      return;
    }
#endif
    for (int j = 0; j < nread; j++) {

#ifdef USE_DIRENT
      mydent = readdir( basedp );
      if ( mydent == NULL ) {
        closedir( basedp );
        break;
      }
      strcpy( filename, mydent->d_name );
      if (!selectImapFiles( mydent ))
        continue;
#elif 0
      strcpy( filename, mydent->d_name );
      DEBUG_OUT("file %s\n", filename);
      if (!selectImapFiles( &dirbuf[j] ))
        continue;
      DEBUG_OUT("--selected\n");
#else     
      strcpy( filename, namelist[j]->d_name );
#endif
      hyphen_ptr = strrchr( filename, '-' );
      if (hyphen_ptr == NULL) {
        ERROR_OUT("warning: malformed imap filename: %s\n", filename);
        continue;
      }
      hyphen_ptr++;
      int context = atoi( hyphen_ptr );
      DEBUG_OUT("system_t(): readInstructionMap found context %s %d\n",
                filename, context);
      
      // for each pid in the system, read the instruction table
      sprintf( filename, "%s%s%s", baseFilename, "imap-", traceFilename);
      bool success = m_seq[i]->readInstructionTable( filename, context );
      if (!success) {
        DEBUG_OUT("Error: unable to read instruction map file: %s\n", filename);
        SYSTEM_EXIT;
      }
    } // end j (read all instruction pages)

    if (validate) {
      bool          success = true;
      la_t          program_counter;
      pa_t          physical_pc;
      unsigned int  instr;
      static_inst_t *s_instr;
      bool          found = false;
      abstract_pc_t apc;

      // attach to this trace
      sprintf( filename, "%s%s", baseFilename, traceFilename );
      m_seq[i]->attachTrace( filename, true );
      
      // read the trace
      while (success) {
        success = m_seq[i]->readTraceStep( program_counter,
                                           physical_pc, instr );

        apc.pc = program_counter;
        apc.tl = m_state[i]->getControl(CONTROL_TL);
        
        found = m_seq[i]->queryInstruction( physical_pc, s_instr );
        if (found) {
          if ( s_instr->getInst() != instr ) {
            DEBUG_OUT("warning: CONFLICT: 0x%0llx 0x%0x 0x%0x\n", 
                      program_counter, s_instr->getInst(), instr);
          }
        } else {
          DEBUG_OUT("not found: 0x%0llx 0x%0llx 0x%0x\n",
                    program_counter, physical_pc, instr);
        }
      } // end while loop
      
      m_seq[i]->closeTrace();
      
      m_seq[i]->printIpage( true );
      // sprintf( filename, "%s%s", baseFilename, "imap-p1-00");
      // m_seq[i]->writeInstructionTable( filename );
    } // end if validate
  } // end i loop
}

/** Attach a trace-mode file
 *  Actually Ipage test currently.
 *  @param filename The name of the trace to open for reading
 */
//***************************************************************************
void system_t::attachTrace( char *baseFilename, char *traceFilename )
{
  char          filename[FILEIO_MAX_FILENAME];

  // attach the trace files to each sequencer
  for ( int i = 0; i < m_numProcs; i++ ) {

    // -- "/p/multifacet/users/cmauer/multifacet/opal/good/imap-p1-00" );

    // attach to this trace
    sprintf( filename, "%s%s", baseFilename, traceFilename);
    // CMSWAP
    m_seq[i]->attachTrace( filename, true );
    // --"/p/multifacet/users/cmauer/multifacet/opal/good/trace-p1-00" );
  }
}

/** attach to a memory trace file.
 */
//***************************************************************************
void system_t::attachMemTrace( char *baseFilename, char *traceFilename )
{
  char          filename[FILEIO_MAX_FILENAME];
  
  // attach the trace files to each sequencer
  for ( int i = 0; i < m_numProcs; i++ ) {

    // attach to this trace
    //DEBUG_OUT("attaching to memtrace: %s -- %s\n", baseFilename, traceFilename);
    sprintf( filename, "%s%s%s", baseFilename, "mem-", traceFilename);
    // CMSWAP
    m_seq[i]->attachMemTrace( filename );
  }

}

//***************************************************************************
void system_t::attachTLBTrace( char *baseFilename, char *traceFilename )
{
  char          filename[FILEIO_MAX_FILENAME];

  // attach the trace files to each sequencer
  for ( int i = 0; i < m_numProcs; i++ ) {
    // attach to a TLB map file
    //DEBUG_OUT("attaching to memtrace: %s -- %s\n", baseFilename, traceFilename);
    sprintf( filename, "%s%s%s", baseFilename, "tlb-", traceFilename);
    // CMSWAP
    m_seq[i]->attachTLBTrace( filename );
  }
}

//***************************************************************************
void system_t::installHapHandlers( void )
{
  if (!CONFIG_IN_SIMICS)
    return;

  hap_type_t magic_break = SIMICS_hap_get_number("Core_Magic_Instruction");
#ifndef SIMICS_3_0
  // verify the type safety of this call
  callback_arguments_t args      = SIMICS_get_hap_arguments( magic_break, 0 );
  const char          *paramlist = SIMICS_get_callback_argument_string( args );
  if (strcmp(paramlist, "nocI" )) {
    ERROR_OUT("error: system_t::installHapHandlers: expect hap to take parameters %s. Current simics executable takes: %s\n",
              "nocI", paramlist );
    SYSTEM_EXIT;
  }
#endif

  m_magic_call_hap = SIMICS_hap_add_callback("Core_Magic_Instruction",
                                              (obj_hap_func_t) system_transaction_callback, 
                                              (void *) this );
}

//***************************************************************************
void system_t::removeHapHandlers( void )
{
  if (!CONFIG_IN_SIMICS)
    return;

  SIMICS_hap_delete_callback_id( "Core_Magic_Instruction", m_magic_call_hap );
}

//***************************************************************************
void system_t::installMemoryHierarchy( sim_status_t status )
{
  DEBUG_OUT("installing memory hierarchy\n");
  if (CONFIG_IN_SIMICS && !m_mhinstalled) {
    /* connect the hfa object to the phys-mem object.
     * undef this call if the object is created in the configration file. */
    attr_value_t   val;
    conf_object_t *cpu_obj;
    attr_value_t   phys_attr;
    conf_object_t *phys_mem0;

    cpu_obj     = SIMICS_proc_no_2_ptr( 0 );
    phys_attr   = SIMICS_get_attribute(cpu_obj, "physical_memory");
    if (phys_attr.kind != Sim_Val_Object) {
      ERROR_OUT("error: phys_mem0 is not an object as it should be\n");
      SIM_HALT;
    }
    phys_mem0   = phys_attr.u.object;
    if (phys_mem0 == NULL) {
      ERROR_OUT("error: unable to locate object: phys_mem0\n");
      SIM_HALT;
    }
    val.kind  = Sim_Val_String;
    val.u.string = "opal0";

    // change the timing model to point at the appropriate handler
    switch (status) {
    case SIMSTATUS_OK:
      hfa_conf_object->timing_interface->operate = system_memory_operation_stats;
      break;
    case SIMSTATUS_TRACING:
      hfa_conf_object->timing_interface->operate = system_memory_operation_trace;
      break;
    case SIMSTATUS_WARMUP:
      hfa_conf_object->timing_interface->operate = system_memory_operation_warmup;
      break;
    case SIMSTATUS_MLP_TRACE:
      hfa_conf_object->timing_interface->operate = system_memory_operation_mlp_trace;
      break;
    case SIMSTATUS_SYMBOL_MODE:
      hfa_conf_object->timing_interface->operate = system_memory_operation_symbol_mode;
      break;
    default:
      ERROR_OUT("system: installMemoryHierarchy: unknown status: %d\n",
                (int) status );
      return;
    }

    SIMICS_set_attribute(phys_mem0, "timing_model", &val);
    hfa_checkerr("system: installMemoryHierarchy: phys_mem0");

    m_mhinstalled = true;
  }
}

//***************************************************************************
void system_t::removeMemoryHierarchy( void )
{
  DEBUG_OUT("removing memory hierarchy\n");
  if (CONFIG_IN_SIMICS && m_mhinstalled) {
    attr_value_t   val;
    conf_object_t *cpu_obj;
    conf_object_t *phys_mem0;

    cpu_obj    = SIMICS_proc_no_2_ptr( 0 );
    phys_mem0  = SIMICS_get_object(SIMICS_get_attribute(cpu_obj, "physical_memory").u.string);
    memset( &val, 0, sizeof(attr_value_t) );
    val.kind = Sim_Val_Invalid;  
    SIMICS_set_attribute(phys_mem0, "timing_model", &val);
    m_mhinstalled = false;
  }
}

//***************************************************************************
void system_t::installMemoryObserver( void )
{
  DEBUG_OUT("installing snoop device interface\n");
  if (CONFIG_IN_SIMICS && !m_snoop_installed) {

    /* connect the hfa object to the phys-mem object.
     * undef this call if the object is created in the configration file. */
    attr_value_t   val;
    conf_object_t *cpu_obj;
    attr_value_t   phys_attr;
    conf_object_t *phys_mem0;

    cpu_obj     = SIMICS_proc_no_2_ptr( 0 );
    phys_attr   = SIMICS_get_attribute(cpu_obj, "physical_memory");
    if (phys_attr.kind != Sim_Val_Object) {
      ERROR_OUT("error: phys_mem0 is not an object as it should be\n");
      SIM_HALT;
    }
    phys_mem0   = phys_attr.u.object;
    if (phys_mem0 == NULL) {
      ERROR_OUT("error: unable to locate object: phys_mem0\n");
      SIM_HALT;
    }

    val.kind  = Sim_Val_String;
    val.u.string = "opal0";
    
    // set the operate function for the snoop memory observer
    hfa_conf_object->snoop_interface->operate = system_memory_operation_stats;

    SIMICS_set_attribute(phys_mem0, "snoop_device", &val);
    hfa_checkerr("system: installMemoryObserver: phys_mem0");
    m_snoop_installed = true;
  }
}

//***************************************************************************
void system_t::removeMemoryObserver( void )
{
  DEBUG_OUT("removing snoop device interface\n");
  if (CONFIG_IN_SIMICS && m_snoop_installed) {
    attr_value_t   val;
    conf_object_t *cpu_obj;
    conf_object_t *phys_mem0;

    cpu_obj    = SIMICS_proc_no_2_ptr( 0 );
    phys_mem0  = SIMICS_get_object(SIMICS_get_attribute(cpu_obj, "physical_memory").u.string);
    memset( &val, 0, sizeof(attr_value_t) );
    val.kind = Sim_Val_Invalid;  
    SIMICS_set_attribute(phys_mem0, SNOOP_MEMORY_INTERFACE, &val);
    hfa_checkerr("system: removeMemoryObserver: phys_mem0");
    m_snoop_installed = false;
  }
}

//***************************************************************************
void system_t::installExceptionHandler( sim_status_t status )
{
  if (!CONFIG_IN_SIMICS)
    return;

  // install a hap handler on exceptions / interrupts
  hap_type_t simcore_exception = SIMICS_hap_get_number("Core_Exception");
#ifndef SIMICS_3_0
  callback_arguments_t args    = SIMICS_get_hap_arguments( simcore_exception, 0 );
  const char        *paramlist = SIMICS_get_callback_argument_string( args );
  if (strcmp(paramlist, "nocI" )) {
    ERROR_OUT("error: system_t::installExceptionHandler: expect hap to take parameters %s. Current simics executable takes: %s\n",
              "nocI", paramlist );
    SYSTEM_EXIT;
  }
#endif

  switch (status) {
  case SIMSTATUS_OK:
    m_exception_haphandle = SIMICS_hap_add_callback( "Core_Exception", 
                                        (obj_hap_func_t) &system_exception_handler,
                                                      NULL );
    break;
  case SIMSTATUS_MLP_TRACE:
    m_exception_haphandle = SIMICS_hap_add_callback( "Core_Exception", 
                                        (obj_hap_func_t) &system_exception_tracer,
                                                      NULL );
    break;
  default:
    DEBUG_OUT("error: install exception handler: unknown status code: %d\n",
              status);
  }
  if (m_exception_haphandle <= 0) {
    ERROR_OUT("error: installHapHandlers: exception haphandle <= 0\n");
    SYSTEM_EXIT;
  }
}

//***************************************************************************
void system_t::removeExceptionHandler( void )
{
  if (!CONFIG_IN_SIMICS)
    return;

  // remove the callback
  SIMICS_hap_delete_callback_id( "Core_Exception", m_exception_haphandle );
}

//***************************************************************************
void system_t::queryRubyInterface( void )
{
  // don't use system->inst in this function. it is not initialized yet!
  if (CONFIG_IN_SIMICS) {
    conf_object_t *ruby = SIMICS_get_object("ruby0");
    if (ruby == NULL) {
      if (SIMICS_number_processors() > 1) {
        ERROR_OUT("error: unable to locate object: ruby0\n");
      } else {
        DEBUG_OUT("info: using opal cache hierarchy -- ruby not found.\n");
      }
      CONFIG_WITH_RUBY = false;
      // If ruby is loaded second, clear this exception. Otherwise keep to
      // halt simulation.
      SIMICS_clear_exception();
#ifdef FAKE_RUBY
      DEBUG_OUT( "info: queryRubyInterface: faking ruby interface\n" );
      CONFIG_WITH_RUBY = true;
      SIMICS_clear_exception();
#endif
    } else {
      m_ruby_api = (mf_ruby_api_t *) SIMICS_get_interface( ruby, "mf-ruby-api" );
      if (m_ruby_api != NULL) {
        CONFIG_WITH_RUBY = true;
      } else {
        ERROR_OUT("error: queryRubyInterface unable to find mf-ruby-api interface.\n");
      }
    }
  } else {
    // not in simics: stand-alone mode
#ifdef UNITESTER
    // initalize the interface between ruby & opal
    m_ruby_api = new mf_ruby_api_t();
    tester_install_opal( m_opal_api, m_ruby_api );
    CONFIG_WITH_RUBY = true;
#endif
  }
}

//***************************************************************************
void system_t::removeRubyInterface( void )
{
  m_ruby_api = NULL;
  CONFIG_WITH_RUBY = false;
}

//***************************************************************************
void system_t::rubyNotifySend( int status )
{
  if (CONFIG_IN_SIMICS) {
#ifdef DEBUG_RUBY
    DEBUG_OUT("rubyNotifySend( int status )\n");
#endif
    mf_ruby_api_t *ruby_intf = system_t::inst->m_ruby_api;
    if (ruby_intf == NULL) {
      DEBUG_OUT("warning: ruby not present or it does not implement mf-ruby-api interface.\n");
    } else {
      if ( ruby_intf->notifyCallback == NULL ) {
        DEBUG_OUT("warning: notifyCallback() is NULL in mf-ruby-api interface.\n");
      } else {
        (*ruby_intf->notifyCallback)( status );
      }
    }
  }
}

//***************************************************************************
void system_t::rubyNotifyRecieve( int status )
{
#ifdef DEBUG_RUBY
  DEBUG_OUT("opalsystem:: rubyNotifyRecieve( int status ) == %d\n", status);
#endif
  if ( status == 0 ) {
    
  } else if ( status == 1 ) {
    // install notification: query ruby for its interface
    if (CONFIG_WITH_RUBY) {
      DEBUG_OUT("Opal: opal-ruby link established.\n");
    }

  } else if ( status == 2 ) {
    // unload notification
    // NOTE: this is not tested, as we can't unload ruby or opal right now.
    system_t::inst->removeRubyInterface();
  }

}

//***************************************************************************
integer_t system_t::rubyInstructionQuery( int cpuNumber )
{
#ifdef DEBUG_RUBY
  DEBUG_OUT("opalsystem:: rubyInstructionQuery( ) == %d\n", cpuNumber);
#endif

  if (system_t::inst != NULL &&
      cpuNumber >= 0 &&
      cpuNumber < system_t::inst->m_numProcs ) {
    return (system_t::inst->m_seq[cpuNumber]->m_stat_committed);
  }
  return (1);
}

//***************************************************************************
void system_t::rubyCompletedRequest( int cpuNumber,
                                     pa_t physicalAddr )
{
#ifdef DEBUG_RUBY
  DEBUG_OUT("opalsystem:: cpu %d completed 0x%0llx\n", cpuNumber, physicalAddr );
#endif
  ASSERT( (cpuNumber >= 0) && (cpuNumber < system_t::inst->m_numProcs) );
  system_t::inst->m_seq[cpuNumber]->completedRequest( physicalAddr );
}

/// write configuration info to disk
//***************************************************************************
void system_t::writeConfiguration( char *configurationName )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->writeCheckpoint(configurationName );
  }
}

/// read configuration info from disk
//***************************************************************************
void system_t::readConfiguration( char *configurationName )
{
  for ( int i = 0; i < m_numProcs; i++ ) {
    m_seq[i]->readCheckpoint(configurationName);
  }
}

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//**************************************************************************
void system_t::initThreadPointers( void ) {
  // The following addresses are found using script acquire.py in opal/python
  // directory. The script is recovered from
  // /p/multifacet/projects/traces3/home/merged
  // directory, I don't really know how exactly Carl get the cpu data structure
  // base address, or breakpoint address. But, with source code and a function
  // name that you know that is modifing the thread it, it should be possible
  // to get the following addresses again for new checkpoints.
  if ( m_numProcs == 1 ) {
    m_seq[0]->setThreadPhysAddress( 0x7f81ad30 );
  } else if ( m_numProcs == 2 ) {
    m_seq[0]->setThreadPhysAddress( 0x7f81ad30 );
    m_seq[1]->setThreadPhysAddress( 0x7c4f0018 );
  } else if ( m_numProcs == 4 ) {
    m_seq[0]->setThreadPhysAddress( 0x7f81ad30 );
    m_seq[1]->setThreadPhysAddress( 0x7c4f0018 );
    m_seq[2]->setThreadPhysAddress( 0x7acfa020 );
    m_seq[3]->setThreadPhysAddress( 0x7aa88aa8 );
  } else if ( m_numProcs == 8 ) {
    m_seq[0]->setThreadPhysAddress( 0x7f81ad30 );
    m_seq[1]->setThreadPhysAddress( 0x7c4f0018 );
    m_seq[2]->setThreadPhysAddress( 0x7acf6020 );
    m_seq[3]->setThreadPhysAddress( 0x7a9c0aa8 );
    m_seq[4]->setThreadPhysAddress( 0x7a9b5530 );
    m_seq[5]->setThreadPhysAddress( 0x7a9b4030 );
    m_seq[6]->setThreadPhysAddress( 0x7a830ab8 );
    m_seq[7]->setThreadPhysAddress( 0x7b031540 );
  } else if ( m_numProcs == 16 ) {
    m_seq[0]->setThreadPhysAddress( 0x7f81ad30 );
    m_seq[1]->setThreadPhysAddress( 0x7c4f0018 );
    m_seq[2]->setThreadPhysAddress( 0x7a89b550 );
    m_seq[3]->setThreadPhysAddress( 0x7a89a050 );

    m_seq[4]->setThreadPhysAddress( 0x7a796ad8 );
    m_seq[5]->setThreadPhysAddress( 0x7a715560 );
    m_seq[6]->setThreadPhysAddress( 0x7a714060 );
    m_seq[7]->setThreadPhysAddress( 0x7a690ae8 );

    m_seq[8]->setThreadPhysAddress( 0x7ac66020 );
    m_seq[9]->setThreadPhysAddress( 0x7a8b6aa8 );
    m_seq[10]->setThreadPhysAddress( 0x7a9ad530 );
    m_seq[11]->setThreadPhysAddress( 0x7a9ac030 );

    m_seq[12]->setThreadPhysAddress( 0x7aa28ab8 );
    m_seq[13]->setThreadPhysAddress( 0x7ada1540 );
    m_seq[14]->setThreadPhysAddress( 0x7ada0040 );
    m_seq[15]->setThreadPhysAddress( 0x7a81cac8 );
  } else {
    /* you can remove this code and continue, but some functionality will
     * not be available */
    DEBUG_OUT("system_t::initThreadPointers: unrecognized system configuration.\n");
    DEBUG_OUT("        :: halting...\n");
    SIM_HALT;
  }
}

/*------------------------------------------------------------------------*/
/* Static functions                                                       */
/*------------------------------------------------------------------------*/

/// used to count the number of processors
static void s_count_processors( conf_object_t *processor, void *count_i32 )
{
  int32 *count_p = (int32 *) count_i32;
  if ( *count_p < SIMICS_get_proc_no(processor) ) {
    *count_p = SIMICS_get_proc_no(processor);
  }
}

//**************************************************************************
static int selectImapFiles( const struct dirent *d )
{
  bool selected = !strncmp(d->d_name, "imap-", 4);
  return (selected);
}

//**************************************************************************
static void system_transaction_callback( void *system_obj, conf_object_t *cpu,
                                         uint64 immediate )
{
  system_t *system = (system_t *) system_obj;
  if ( immediate == 5UL << 16 ) {
    system->postedMagicBreak();
    return;
  } else {
    DEBUG_OUT("system_transaction: info: unusual magic instruction (0x%0llx)\n", immediate);
  }
}

//**************************************************************************
static void system_break_handler( void *system_obj, uint64 access_type,
                                  uint64 break_num,
                                  void *reg_ptr, uint64 reg_size )
{
  system_t *system = (system_t *) system_obj;
  uint32 break_as_int = (uint32) break_num;
  uint32 access_as_int = (uint32) access_type;
  system->postedBreakpoint( break_as_int, access_as_int );
}

//**************************************************************************
static double process_memory_total( void )
{
  // 4kB page size, 1024*1024 bytes per MB, 
  const double MULTIPLIER = 4096.0/(1024.0*1024.0);

  ifstream proc_file;
  proc_file.open("/proc/self/statm");
  int total_size_in_pages = 0;
  int res_size_in_pages = 0;
  proc_file >> total_size_in_pages;
  proc_file >> res_size_in_pages;
  return double(total_size_in_pages)*MULTIPLIER; // size in megabytes
}

//**************************************************************************
static double process_memory_resident( void )
{
  // 4kB page size, 1024*1024 bytes per MB, 
  const double MULTIPLIER = 4096.0/(1024.0*1024.0);
  ifstream proc_file;
  proc_file.open("/proc/self/statm");
  int total_size_in_pages = 0;
  int res_size_in_pages = 0;
  proc_file >> total_size_in_pages;
  proc_file >> res_size_in_pages;
  return double(res_size_in_pages)*MULTIPLIER; // size in megabytes
}

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
void system_mlp_trace_tick( conf_object_t* obj, lang_void* arg )
{
  system_t::inst->m_ruby_api->advanceTime();
  system_post_tick();
}

//**************************************************************************
void system_post_tick( void )
{
  const int SIMICS_MULTIPLIER = 4;

  conf_object_t* obj_ptr = (conf_object_t*) SIMICS_proc_no_2_ptr(0);
  SIMICS_time_post_cycle(obj_ptr, SIMICS_MULTIPLIER,
                      Sim_Sync_Processor, &system_mlp_trace_tick, NULL);
}

//**************************************************************************
cycles_t system_memory_snoop( conf_object_t *obj,
                              conf_object_t *space,
                              map_list_t *map, 
                              generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // code to handle I/O memory space transactions
  if ( IS_DEV_MEM_OP( mem_op->s.ini_type )) {
    // DEBUG_OUT( "device access: 0x%0llx\n", mem_op->s.physical_address );
    system_t::inst->m_sys_stat->observeIOAction( mem_op );
  }

  return (0);
}

//**************************************************************************
cycles_t system_memory_operation_warmup( conf_object_t *obj,
                                         conf_object_t *space,
                                         map_list_t *map, 
                                         generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // find the cpu for the current processor
  int cpuno = SIMICS_get_current_proc_no();

  // we are in "warm-up mode": pass the memory transaction
  //                           directly to the cache interface
  system_t::inst->m_seq[cpuno]->warmupCache( mem_op );
  return 0;
}

//**************************************************************************
cycles_t system_memory_operation_symbol_mode( conf_object_t *obj,
                                              conf_object_t *space,
                                              map_list_t *map, 
                                              generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // data accesses finish immediately (and can be placed in the STC)
  if ( SIMICS_mem_op_is_data( &mem_op->s ) )
    return 0;

  // non CPU accesses finish immediately.
  if (!IS_CPU_MEM_OP(mem_op->s.ini_type)) {
    return 0;
  }
  
  // call out to the symbol object for I-fetches
  int cpuno = SIMICS_get_proc_no(mem_op->s.ini_ptr);
  system_t::inst->m_symtrace->memoryAccess( cpuno, mem_op );
  return 0;
}

//**************************************************************************
cycles_t system_memory_operation_trace( conf_object_t *obj,
                                        conf_object_t *space,
                                        map_list_t *map, 
                                        generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // find the cpu for the current processor
  int cpuno = SIMICS_get_current_proc_no();

  // we are in "trace mode": pass the memory transaction to the trace log
  transaction_t my_trans(mem_op);
  system_t::inst->m_seq[cpuno]->writeTraceMemop( &my_trans );
  return 0;
}

//**************************************************************************
cycles_t system_memory_operation_mlp_trace( conf_object_t *obj,
                                            conf_object_t *space,
                                            map_list_t *map, 
                                            generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // find the cpu for the current processor
  int cpuno = SIMICS_get_current_proc_no();

#ifdef VERIFY_SIMICS
  // check to see if id is same
  if ( IS_CPU_MEM_OP( mem_op->s.ini_type ) ) {
    conf_object_t *originator = mem_op->s.ini_ptr;
    int check_id = SIMICS_get_proc_no(originator);
    if ( (check_id != -1) && (check_id != cpuno) ) {
      ERROR_OUT( "warning: originator (%d) != current (%d)\n",
                 check_id, cpuno );
    }
  }
#endif
  
  // to create a trace of a given timing execution, pass the memory op
  // to the sequencer (it will be formatted & passed to ruby)
  return ptrace_t::mlpMemopDispatch( cpuno, mem_op );
}

//**************************************************************************
cycles_t system_memory_operation_stats( conf_object_t *obj,
                                        conf_object_t *space,
                                        map_list_t *map, 
                                        generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // code to handle I/O memory space transactions
  if ( IS_DEV_MEM_OP( mem_op->s.ini_type )) {
    system_t::inst->m_sys_stat->observeIOAction( mem_op );
    //DEBUG_OUT( "device access: 0x%0llx\n", mem_op->s.logical_address );
  }
  return 0;
}

//**************************************************************************
static void system_exception_handler( void *obj, conf_object_t *proc,
                                      uint64 exception )
{
  uint32  exception_as_int = (uint32) exception;
    
  // find which CPU experienced the interrupt
  int id = SIMICS_get_proc_no( SIMICS_current_processor() );
  // LXL-FIXME again, hacked to make this work
  if (id == system_t::inst->server_client_mode) {
	  pseq_t *pseq = system_t::inst->m_seq[id];
	  
	  // post an exception to the instruction on the current processor
	  pseq->postException( exception_as_int );
  }

	//if(exception == 0x4a)	{
	//	debugio_t::out_info("\n --- %lld\n", SIMICS_cycle_count(proc));
	//	debugio_t::out_info("\n --- %lld\n", pseq->getLocalCycle());
	//}

}

//**************************************************************************
static void system_exception_tracer( void *obj, conf_object_t *proc,
                                     uint64 exception )
{
  uint32  exception_as_int = (uint32) exception;

  // find which CPU experienced the interrupt
  int cpuno = SIMICS_get_proc_no(proc);
  ptrace_t::mlpExceptionDispatch( cpuno, exception_as_int );
}

#ifdef MEASURE_FP
int system_t::getFP(int i, int j)
{ return m_seq[0]->getFP(i,j);}
#endif

#ifdef LL_DECODER

#include "opcode.h"

void system_t::initOpcodeExecTable() {
	opcode_to_exec_type = (dyn_execute_type_t *) malloc(i_maxcount*sizeof(dyn_execute_type_t));

	for (int i=0; i<i_maxcount; i++) {
		if ((i>= i_fba && i<=i_call) ||
			(i>= i_done && i<=i_jmpl) ||
			(i>= i_retrn && i<=i_bvs) ||
			(i>= i_ta && i<=i_tvs) ||
			(i>= i_save && i<=i_restore) ||
			(i==i_retry) || (i==i_jmp) || (i==i_wrpr)) {

			opcode_to_exec_type[i]=DYN_CONTROL;

		} else if ((i== i_ldstub) || (i== i_swap) || (i== i_ldstuba) ||
				   (i== i_swapa) || (i== i_casa) || (i== i_casxa) ) {
			opcode_to_exec_type[i]=DYN_ATOMIC;

		} else if ((i== i_prefetch) || (i== i_prefetcha)) {

			opcode_to_exec_type[i]=DYN_PREFETCH;

		} else if ((i>= i_ldf && i<=i_ldqa) ||
				   (i>= i_ldfsr && i<=i_ldxfsr)) {

			opcode_to_exec_type[i]=DYN_LOAD;

		} else if ((i>= i_stfsr && i<=i_stxfsr) ||
				   (i>= i_stf && i<=i_stda) ) {

			opcode_to_exec_type[i]=DYN_STORE;

		} else if (i==i_ill) {
			opcode_to_exec_type[i]=DYN_NONE;
		} else {
			opcode_to_exec_type[i]=DYN_EXECUTE;
		}
	}


}

dyn_execute_type_t system_t::lookupOpExecTab(int y) {
	int x = (i_opcode)y;
	if (x>=i_maxcount) return DYN_NONE;

	return opcode_to_exec_type[x];
}

#endif
