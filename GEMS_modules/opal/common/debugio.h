
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
#ifndef _DEBUGIO_H_
#define _DEBUGIO_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <set>

#include <string>

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*----------------------------------------------------------------------*/
#define DECODER_INPUT_FAULT   0
#define INTALU_OUTPUT_FAULT   1
#define FPALU_OUTPUT_FAULT    2
#define	REG_DATA_LINE_FAULT   3
#define	REG_FILE_FAULT        4
#define LSQ_ADDRESS_FAULT     5
#define INST_WINDOW_FAULT     6
#define RAT_FAULT             7
#define AGEN_FAULT            8
#define FP_REG_FAULT          9
#define BRANCH_TARGET_FAULT   10
#define LSQ_DATA_FAULT        11
#define INVALID_FAULT         22

#define STUCK_AT_ZERO         0
#define STUCK_AT_ONE          1
#define DOM_ZERO              2
#define DOM_ONE               3
#define TRANSIENT             4

#define NUM_CORES	16
#define PREF_CNT_THRESHOLD    2
#define PREF_CNT_SATURATION   2*PREF_CNT_THRESHOLD


#define FORWARD_PROGRESS_THRESHOLD 20000

/// global macro for printing out informational messages
#define  DEBUG_OUT \
     debugio_t::out_info

/// print info to trace
#define  TRACE_OUT \
     debugio_t::out_trace 

/// global macro for printing out log only (doesn't appear on screen)
#define  DEBUG_LOG \
     debugio_t::out_log

/// global macro for printing out error messages (doesn't halt program)
#define  ERROR_OUT \
     debugio_t::out_error

#define FAULT_STAT_OUT \
    debugio_t::out_fault_stat

#define FAULT_RET_OUT \
    debugio_t::out_fault_ret

#define FAULT_OUT \
	debugio_t::out_fault

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* The out_intf_t class provides a virtual interface for logging
* messages in to separate informational (printed to screen and log) and
* log (printed only to log) messages. This interface allows classes
* associated with a particular sequencer to print their messages
* automatically prefixed by the ID of that sequencer. For instance,
* the cache (for seq #2) message may be: "[2] Total # of reads   1,500".
*
* @see     pseq_t
* @author  cmauer
* @version $Id: debugio.h 1.12 06/02/13 18:49:10-06:00 mikem@maya.cs.wisc.edu $
*/


struct addr_info {
	uint32 core_id;
	bool rw;
	int trap_level;
	uint64 pc;
	bool corrupted;
	uint64 val;
};

class out_intf_t {
public:
  /// Constructor
  out_intf_t( int id ) {
    m_id    = id;
    m_starting_cycle = 0ULL;
    m_logfp = NULL;
  };

  /// Destructor  
  ~out_intf_t( void ) {};
  
  /** out_info prints out to stdout.
   */
  void   out_info( const char *fmt, ...);
  
  /** out_log prints out to only the log file.
   */
  void   out_log( const char *fmt, ...);

  /// returns true if this message should _not_ be filtered
  bool   validDebug( void );

  /// sets the debugging time for this debugio filter.
  void   setDebugTime(uint64 cycle);

  /// gets the debugging time for this debugio filter.
  uint64 getDebugTime( void ) {
    return m_starting_cycle;
  }

  /** returns a pointer to this sequencers log file */
  FILE   *getLog( void ) {
    return m_logfp;
  }
  
  /** sets the log file pointer for this object. */
  void    setLog( FILE *fp ) {
    m_logfp = fp;
  }
  
private:
  /// identifier for this output logger
  int             m_id;

  /// The log file for this particular object
  FILE           *m_logfp;

  /// determines starting cycle time to start filtering messages
  uint64          m_starting_cycle;
};

/**
* The DebugIO class formats, filters debugging log file messages.
*
* @author  cmauer
* @version $Id: debugio.h 1.12 06/02/13 18:49:10-06:00 mikem@maya.cs.wisc.edu $
*/
class debugio_t {
	public:
		/**
		 * @name Constructor(s) / destructor
		 */
		//@{
		/// Constructors
		debugio_t();

		/// Destructor
		~debugio_t();
		//@}

		/**
		 * @name public methods
		 */
		//@{
		/** out_info prints out to stdout and a file at the same time.
		 */
		static void out_info( const char *fmt, ...);

		/** out_log prints out only to the log file.
		 */
		static void out_log( const char *fmt, ...);

		/** out_error prints out to standard error */
		static void out_error( const char *fmt, ...);

		/** out_fault prints out to Fault log file */
		static void out_fault( const char *fmt, ...);
		static void out_fault_ret( const char *fmt, ...);
		static void out_fault_stat( const char *fmt, ...);

		/** file for tracing execution */
		static void out_trace( const char *fmt, ... ) ;
		static void out_trap_info(uint64 cyc, uint64 inst, const char* trap_name, 
				unsigned int tt, la_t tpc, int priv, la_t thpc, int tl,int coreId);
		/** returns the system wide out_info log file */
		static FILE *getLog( void ) {
			return m_logfp;
		}

		/** opens the system wide out_info log file */
		static void openLog( const char *logFileName );

		/** closes the system wide out_info log file */
		static void closeLog( void );

		/** opens the system wide fault_log log file */
		static void openFaultLog( char *logFileName );

		/** closes the system wide out_info log file */
		static void closeFaultLog( void );

		/** sets the debugging time for the global filter */
		static void setDebugTime(uint64 cycle) {
			m_global_cycle = cycle;
		}

		static void setFaultyCore(uint64 core_id) {
			m_faulty_core = core_id;
		}
		static uint32 getFaultyCore() {
			return m_faulty_core;
		}

		static bool isMasked() { return m_is_masked; }
		static void setMasked() { m_is_masked = true; }
		static void updateForwardProgress(int core_id, bool priv);
		static bool makingForwardProgress();
		static bool suspectForwardProgress();
		static void clearForwardProgress(); 

		static void setSymptomDetected();
		static bool checkSymptomDetected(int core_id) { return symptomDetected[core_id];}
		static void checkedSymptomDetected( int core_id) { symptomDetected[core_id] = false;}
		static void clearSymptomDetected() { 
			for(int i=0;i<NUM_CORES; i++) 
				symptomDetected[i] = false;
		}

		static uint64 getStartTime() {return m_start_time;}

		static bool isShared(uint64 physical_address, int priv);
		static uint32 getMappedCoreID(uint64 physical_address, int priv);
		static void updateMap(uint64 physical_address, uint32 core_id, int priv);
		static int mapSize(int priv) { return shared_address_map[priv].size(); }
		static void clear_corrupted_address_map();
		static bool isCorruptedNow(uint64 addr, int priv);
		static void add_shared_address(uint64 addr, int priv, struct addr_info info);
		static void printSharedAddresses(int map_id);

		// Functions to manipulate the corrupt_addresses map
		static bool isCorruptAddress(uint64 addr, int p) ;
		static void addCorruptAddress(uint64 addr, int p) ;
		static void removeCorruptAddress(uint64 addr, int p) ;
		static void printCorruptAddress() ;

		static void setStateMismatch() { state_mismatch = true; }
		static bool hasStateMismatch() { return state_mismatch; }

		static void setNumProcs(uint32 num_procs) { m_num_procs = num_procs; }

		static void printSafFaultStats();

		//@}

		static uint64 m_start_time;

		static char* getAppName() { return m_app_name;}
		static void setFaultyMachine( string r )  { 
			m_faulty_machine = r; 

		}
		static void setFaultStuckat( int s ) {
			m_faultstuckat = s ;
		}
		static void processFMProfile();

		// The tables based on malloc's values
		struct ireg_cmp {
			bool operator() (ireg_t A, ireg_t B) {
				return A < B ;
			}
		} ;
		typedef pair<ireg_t, ireg_t> addr_range_t ;
		// struct addr_cmp {
		// 	bool operator() (addr_range_t A, addr_range_t B) {
		// 		if(A.first!=B.first) {
		// 			return A.first < B.first ;
		// 		} else if(A.second!=B.second) {
		// 			return A.second < B.second ;
		// 		} else { // the two are equal
		// 			return false ;
		// 		}
		// 	}
		// } ;
		typedef map<ireg_t, addr_range_t, ireg_cmp> prune_object_table_t ;
		typedef vector<addr_range_t> object_table_t ;
		struct char_cmp {
			bool operator() (char* a, char* b) const {
				return strcmp(a,b)<0 ;
			}
		} ;

		typedef map<char *, ireg_t, char_cmp> char_int_t ;
		struct half_cmp {
			bool operator() (half_t A, half_t B) {
				return A < B ;
			}
		} ;
		typedef map<half_t, addr_range_t, half_cmp> reg_object_table_t ;
		typedef map<ireg_t, addr_range_t, ireg_cmp> mem_object_table_t ;

		static ireg_t min_tos, max_tos ;
		static ireg_t max_heap, min_heap ;
		static void recordMemAddr(ireg_t addr) {
			if(addr == 0x0) {
				return ;
			}

#ifdef BINARY_32_BIT
			if( addr>0xF0000000 ) {
#else // BINARY_32_BIT
			if( addr>0xffffffff7f000000) {
#endif // BINARY_32_BIT
				if(addr<min_tos ) {
					min_tos = addr ;
					// DEBUG_OUT("New min tos = 0x%llx\n", min_tos) ;
				}
				if(addr>max_tos) { 
					max_tos = addr ;
				}
			} else {
				if(addr > max_heap) {
					max_heap = addr ;
				} else if (addr < min_heap) {
					min_heap = addr ;
				}
			}
		}
		static ireg_t getMinTos() { return min_tos ; }
		static ireg_t getMaxTos() { return max_tos ; }
		static ireg_t getMinHeap() { return min_heap ; }
		static ireg_t getMaxHeap() { return max_heap ; }

		static bool in_warmup ;
		static void startWarmup() {
			DEBUG_OUT("Warmup for %d instructions\n", WARMUP_INST) ;
			in_warmup = true ;
		}
		static bool isinWarmup() { return in_warmup ; }
		static void stopWarmup() {
			DEBUG_OUT("Stopping warmup\n") ;
			in_warmup = false ;
		}

		static object_table_t object_table ;
		static prune_object_table_t prune_object_table ;
		static char_int_t stack_start ;
		static char_int_t stack_end ;
		static char_int_t heap_start ;
		static char_int_t heap_end ;
		static reg_object_table_t reg_object_table ;
		static mem_object_table_t mem_object_table ;
		static void printRegObjectTable() ;
		static void printMemObjectTable() ;

		// Functions
		static bool isHeapAccess(ireg_t addr) ;
		static bool isHeapAddress(ireg_t addr) ;
		static void readObjectTable() ;
		static addr_range_t getObject(ireg_t addr) ;
		static bool isValidObject(ireg_t addr) ;
		// Return 0 if successful. 1 on failure
		static int handleObjectLoad(half_t dest, half_t src1, half_t src2, ireg_t addr, int priv, int in_trap=0) ;
		static int handleObjectStore(half_t dest, half_t src1, half_t src2,  ireg_t addr, int priv, int in_trap=0) ;
		static int handleObjectPropagation(half_t dest, half_t src1, half_t src2, int priv, int in_trap=0) ;

		static bool isRegValueinObj(half_t dest, ireg_t value) ;
		static bool isMemValueinObj(ireg_t addr, ireg_t value) ;

	private:
		// Private Methods

		/// Private copy constructor
		debugio_t(const debugio_t& obj);
		/// Private assignment operator
		debugio_t& operator=(const debugio_t& obj);

		static void extractAppName(char* log_name, char* out_string);

		/// determines the starting cycle for filtering global messages
		static uint64 m_global_cycle;

		/// The system wide (not processor specific) log file.
		static FILE *m_logfp;

		/// Fault related fields
		static FILE *m_faultlogfp;
		static FILE *m_faultstatfp;
		static FILE *m_faultretfp;
		static FILE *m_tracefp ;

		static uint32 m_faulty_core;
		static uint32 m_num_procs;
		static bool m_is_masked ;

		struct shared_address_map_cmp {
			bool operator() ( uint64 a, uint64 b) const {
				return a < b;
			}
		};

		typedef map<uint64, vector<struct addr_info>, shared_address_map_cmp> shared_address_map_t;
		static shared_address_map_t shared_address_map[2];

		// A map to store all the corrupt mem addresses in the system
		typedef set<uint64, shared_address_map_cmp> corrupt_addresses_t ;
		static corrupt_addresses_t corrupt_addresses[2] ;

		//FIXME: bad coding - I hardcoded it for 8 cores
		//This could be the reason for seg fault, if you get one
		static int count_core[16];

		static bool state_mismatch;

		static bool symptomDetected[16];

		static char m_app_name[256];
		static string m_faulty_machine ;
		static int m_faultstuckat;


};

/*************************************************************************
Class fault_stats
*************************************************************************/
//Added by siva
class fault_stats {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /// Constructors
  fault_stats(unsigned int core_id);

  /// Destructor
  ~fault_stats();
  //@}
  
  /**
   * @name public methods
   */

  /** the fault type **/
   void setFaultType( int t ) { m_faulttype = t ; }
   int getFaultType( void ) 	{ return m_faulttype ; }

  /** the fault type **/
   void setFaultBit( int b ) {
      m_faultbit = b ; 
      if (m_faulttype==INST_WINDOW_FAULT) {
          m_faultbit = m_faultbit & 0xff;
          m_fault_src_dst = (b>>8) & 0x7;
      }
  }

   int getFaultBit( void ) 	{ return m_faultbit ; }

   int getFaultSrcDst( void ) 	{ return m_fault_src_dst ; }

  /** the fault type **/
   void setFaultStuckat( int s ) {
	  m_faultstuckat = s ;
#if LXL_PROB_FM
	  debugio_t::setFaultStuckat(s);
#endif
  }

   int getFaultStuckat( void ) 	{ return m_faultstuckat ; }

  /** the faulty register**/
   void setFaultyReg( int r ) 	{ m_faultyreg = r ; }
   int getFaultyReg( void ) 	{ return m_faultyreg ; }

  /** source or dest register**/
   void setSrcDestReg( int r ) 	{ m_srcdest_reg = r ; }
   int getSrcDestReg( void ) 	{ return m_srcdest_reg ; }

   void setStartLogging( int r ) 	{ m_start_logging_inst = r ; }
   int getStartLogging( void ) 	{ return m_start_logging_inst; }

   void setComparePoint( int r ) 	{ m_compare_point = r ; }
   int getComparePoint( void ) 	{ return m_compare_point; }

  /** Print the fault stats**/
   void printFaultStats() ;

   void setInfLoopCycle( uint64 cycle ) 	{ m_inf_loop_start = cycle ; }
   uint64 getInfLoopCycle( void ) 	{ return m_inf_loop_start; }

  /** Fault injection functions. Overloaded to take unsigned short (16bits), unit32 and uint64 */
   byte_t injectFault( byte_t value);
   half_t injectFault( half_t value);
   uint32 injectFault( uint32 value);
   uint64 injectFault( uint64 value);

   void setFaultInjInst ( uint64 value ) { 
      m_fault_inj_inst = value ; 
      m_trans_inj_cyc = value;
      setFaultInjCycle(value);
#if LXL_PROB_FM
	  srand(value);
#endif
  }

   void setFaultInjCycle(uint64 value) {
      m_fault_inj_cycle = value;
  }

   uint64 getFaultInjCycle() {return m_fault_inj_cycle;}

   uint64 getFaultInjInst ( ) {return m_fault_inj_inst;}

   void setFaultInjPC(la_t pc) { m_fault_inj_pc = pc ; }
   la_t getFaultInjPC() { return m_fault_inj_pc ; }

   void setFaultInjOp(const char *s) { strcpy(m_fault_inj_op, s) ; }
   char *getFaultInjOp() { return m_fault_inj_op; }

  /** the faulty gate **/
  void setFaultyGateId( int r ) 	{ m_faulty_gate_id = r ; }
  int getFaultyGateId( void ) 	{ return m_faulty_gate_id ; }

  /** the faulty fanout id **/
  void setFaultyFanoutId( int r )  { m_faulty_fanout_id = r ; }
  int getFaultyFanoutId( void ) 	{ return m_faulty_fanout_id ; }

  // NCVLOG 
  
  /** the faulty machine, net name **/
  void setFaultyMachine( string r )  { 
	  m_faulty_machine = r; 
#if LXL_PROB_FM
	  debugio_t::setFaultyMachine(r);
#endif

  }
  string getFaultyMachine( void ) 	{ return m_faulty_machine; }

  /** pipes **/
  void setStimPipe( string r )  { m_stim_pipe = r; }
  string getStimPipe( void ) 	{ return m_stim_pipe; }
  void setResponsePipe( string r )  { m_response_pipe = r; }
  string getResponsePipe( void ) 	{ return m_response_pipe; }

  void injectSafFault()
  {
	  // these variables to be set only at the time of the first injection...
	  if(!stat_first_inject_c) {
		  stat_first_inject_c = getClock() ;
		  stat_first_inject_i = getRetInst() ; 
	  }
  }
// #endif

  /** Crucial numbers **/
   void clearDoFaultInjection() { do_fault_injection = false ; }
   void setDoFaultInjection() { do_fault_injection = true ; }
   bool getDoFaultInjection() { return do_fault_injection ; }

	/** Clock from pseq **/
	 void setClock( uint64 c ) {
		curr_cycle = c ;
	}
	 uint64 getClock( ) { return curr_cycle ; }

	/** Current and retiring instruction counters **/
	 void setCurrInst( uint64 i ) { curr_inst = i ; }
	 uint64 getCurrInst () { return curr_inst ; }

	 void setRetInst( uint64 i ) ;
	 uint64 getRetInst () { return ret_inst ; }

	 void setRetPriv( unsigned int i ) { ret_priv = i ; }
	 unsigned int getRetPriv () { return ret_priv ; }

	 void setPriv( int p ) { priv = p ; }
	 int getPriv() { return priv ; }

	 void inFuncTrap() { in_func_trap = 1 ; }
	 void clearFuncTrap() { in_func_trap = 0 ; }

	 void setNonRetTrap() { non_ret_trap = 1 ; }

	/** Arch cycle number from arf **/
	// Arch state is mismatched only when
	// 1 - A fault has been injected, and
	// 2 - The mismatch was not becuase of a func trap of unimplemented inst
	// 3 - and the arch state mismatch has not been seen yet
	 void setArchCycle() {
		if( getRetPriv() ) { // Then in the OS
			if( !stat_os_arch_state_mismatch ) {
				stat_os_arch_state_mismatch = 1 ;
				debugio_t::setStateMismatch();
				stat_os_first_arch_state_c = getClock() ;
				stat_os_first_arch_state_i = getRetInst() ;
                FAULT_RET_OUT("OS mismatch\n");
			}
		} else { // Then in the app
			if( !stat_app_arch_state_mismatch ) {
				stat_app_arch_state_mismatch = 1 ;
				stat_app_first_arch_state_c = getClock() ;
				stat_app_first_arch_state_i = getRetInst() ;
                FAULT_RET_OUT("APP mismatch\n");
			}
		}
	}

	 uint64 getArchCycle() {

		 uint64 first_mismatch_cycle;

		 if (stat_os_arch_state_mismatch) {
			 first_mismatch_cycle=stat_os_first_arch_state_i;
		 }

		 if (stat_app_arch_state_mismatch) {
			 if (first_mismatch_cycle>stat_app_first_arch_state_i)
				 first_mismatch_cycle=stat_app_first_arch_state_i;
		 }

		 return first_mismatch_cycle;
	 }

	// Mismatch of arch state when the faulty register was in use.
	// This is the more liberal definition. perhaps more useful
	// as sparc's register file is large
	 void setArchInuseCycle() {
		if( getRetPriv() ) { // Then in the OS
			if( !stat_os_arch_state_inuse_mismatch ) {
				//out_info( "app- Arch inuse %d\n", getRetInst() ) ;
				stat_os_arch_state_inuse_mismatch = 1 ;
				stat_os_first_arch_state_inuse_c = getClock() ;
				stat_os_first_arch_state_inuse_i = getRetInst() ;
                FAULT_RET_OUT("OS in-use mismatch\n");
			}
		} else { // Then in the app
			if( !stat_app_arch_state_inuse_mismatch ) {
				//out_info( "os - Arch inuse %d\n", getRetInst() ) ;
				stat_app_arch_state_inuse_mismatch = 1 ;
				stat_app_first_arch_state_inuse_c = getClock() ;
				stat_app_first_arch_state_inuse_i = getRetInst() ;
                FAULT_RET_OUT("APP in-use mismatch\n");
			}
		}
	}

	// Prad - Memory state may mismatch without a mismatch
	// in the arch state (ie, registers may match, but memory
	// needn't match.
	 void setMemCycle() {
        if( getRetPriv() && stat_os_arch_state_mismatch ) { // Then, this is os mem state
            if( !stat_os_mem_state_mismatch ) {
                stat_os_mem_state_mismatch = 1 ;
                stat_os_first_mem_state_c = getClock() ;
                stat_os_first_mem_state_i = getRetInst() ;
            }
        } else if( stat_app_arch_state_mismatch ) { // Then, app mem state
            if( !stat_app_mem_state_mismatch ) {
                stat_app_mem_state_mismatch = 1 ;
                stat_app_first_mem_state_c = getClock() ;
                stat_app_first_mem_state_i = getRetInst() ;
            }
        }
		// We should also ensure that arch inuse cycle is set over here
		// as the MEM_STATE could've been flagged without a mismatch
		// in regs (mismatch in address only)
		setArchInuseCycle() ; // FIXME
	}

     bool getArchMismatch(void) { 
      return (stat_os_arch_state_mismatch | stat_app_arch_state_mismatch); 
    }

	 void setCtrlCycle() {
		 if(!ctrl_corrupted) {
			 ctrl_corrupted = true ;
			 stat_ctrl_corrupt_c = getClock() ;
			 stat_ctrl_corrupt_i = getRetInst() ;
		 }
	 }

	 int getCtrlCycle() { return stat_ctrl_corrupt_c ; }
	 int getCtrlInst() { return stat_ctrl_corrupt_i ; }

    // Track # cycles spent in OS or App
     void trackCycle(uint64 cur_cycle, uint64 seq_num, bool priv, bool isLoopInstr);

    // Track # branches
	 void incBranches() { } ;
	 void incStores() { store_branch_cnt ++ ; }

	/** Trap counters **/
	 int getTotalInj() { return stat_total_inject ; }
	 void incTotalInj() { stat_total_inject ++ ; }
	 void incTotalMask() { stat_total_mask ++ ; }
	 void incTotalTraps() { stat_total_traps ++ ; }
	 void incFatalTraps() { stat_fatal_traps ++ ; }
	 void incReadInj() { stat_read_inject ++ ; }
	 void incReadMask() { stat_read_mask ++ ; }

     void setTransInjected() { m_trans_injected = true ; }
     bool getTransInjected() { return m_trans_injected ; }

     void outSymptomInfo(uint64 cyc, uint64 inst, const char* trap_name, 
			 unsigned int tt, la_t tpc, int priv, la_t thpc, int tl, int coreId);
     void outSymptomInfo(uint64 cyc, uint64 inst, const char* trap_name, 
			    unsigned int tt, la_t tpc, int priv, la_t thpc, int tl, int coreId, int InvId);

/*      void outTrapInfo(uint64 cyc, unsigned int inst, const char* trap_name,  */
/* 			    unsigned int tt, la_t tpc, int priv, la_t thpc, int tl); */
     void markInjTransFault();

     uint64 getOSInstr() { return cur_os_instrs; }
     int getID() { return m_core_id; }

    void clearOSInstr() 
	{ 
	    last_switch_cycle=curr_cycle; 
	    last_switch_instr=ret_inst;
	    last_priv_mode=ret_priv;
	    os_start_instr=ret_inst;
	    cur_os_instrs=0;
	}

	// Functions to store the types of faulty instructions
	void addFaultyInstruction(const char *op) ;
	void printFaultInstList() ;

	void recordInstMix(const char* type, bool has_fault) ;
	void printInstMix() ;

	// Functions to manipulate list of targets of ctrl inst
	void addInstTargets(ireg_t pc, ireg_t target) ;
	void printInstTargets() ;

	// Functions to build ranges of mem addresses of mem inst
	void buildInstRange(int priv, ireg_t pc, ireg_t target) ;
	void printInstRanges() ;

	// Functions to doing the dynamic range checking
	struct ireg_cmp {
		bool operator() (ireg_t A, ireg_t B) {
			return A < B ;
		}
	} ;
	typedef pair<ireg_t, ireg_t> addr_range_t ;
	typedef map<ireg_t, addr_range_t, ireg_cmp> inst_addr_range_t ;
	void readInstRangesFile() ;
	bool rangeCheck(ireg_t pc, ireg_t addr) ;
	inst_addr_range_t golden_range ;

	// Invariant detectors
	typedef set<ireg_t, ireg_cmp> detector_list_t ;
	detector_list_t detector_list ;

	typedef pair<float64, float64> value_range_t ;
	typedef map<ireg_t, value_range_t, ireg_cmp> detector_range_t ;
	detector_range_t detector_range ;

	typedef vector<float64> value_list_t ;
	typedef map<ireg_t, value_list_t, ireg_cmp> detector_values_t ;
	detector_values_t detector_values ;

	void readDetectorList() ;
	bool hasDetector(ireg_t pc) ;
	void addToRange(ireg_t pc, ireg_t value) ;
	void addToRange(ireg_t pc, float32 value) ;
	void addToRange(ireg_t pc, float64 value) ;
	void printDetectorRanges() ;
	void readDetectorRanges() ;
	bool isInRange(ireg_t pc, ireg_t value) ;
	bool isInRange(ireg_t pc, float32 value) ;
	bool isInRange(ireg_t pc, float64 value) ;

	// Data-only values stuff
	detector_list_t data_only_values ;
	void readDataOnlyValuesList() ;
	bool isDataOnly(ireg_t pc) ;
	
	// Building object table based on the instrumentation of malloc, etc.
	typedef map<ireg_t, ireg_t, ireg_cmp> fake_object_table_t ;
	fake_object_table_t fake_object_table ;
	fake_object_table_t num_references ;
	typedef set<ireg_t> offsets_t ;
	typedef map<ireg_t, offsets_t, ireg_cmp> base_offsets_t ;
	base_offsets_t base_offsets ;
	typedef map<ireg_t, unsigned int, ireg_cmp> num_offsets_t ;
	num_offsets_t num_offsets ;

	void buildObjectTable(ireg_t base, ireg_t offset) ;
	ireg_t getObjectEnd(ireg_t addr) ;
	void pruneObjectTable() ;
	void makeRangesSane() ;
	void printObjectTable() ;

	// Manipulate list of mem addresses accessed by mem inst
	void addMemAccess(ireg_t pc, ireg_t target) ;
	void printMemAccess() ;

	// Functions for the stride prefetcher
	ireg_t getPrefetch(ireg_t pc, ireg_t addr) ;
	void printPrefetchTable() ;

	uint64 getFixAddr() {return fix_add; }
	uint64 getFixVal() {return fix_val; }
	int getFixAccessSize() {return fix_access_size; }
	void setFixAddr(uint64 v) {fix_add=v; }
	void setFixVal(uint64 v) {fix_val=v; }
	void setFixAccessSize(int v) {fix_access_size=v; }

 private:
	// Private Methods

	/// Private copy constructor
	 fault_stats(const fault_stats& obj);
	 // Private assignment operator
	 fault_stats& operator=(const fault_stats& obj);
	
	uint64 fix_add;
	int fix_access_size;
	uint64 fix_val;

   unsigned int m_faulttype ;
   unsigned int m_faultbit ;
   unsigned int m_fault_src_dst ;
   unsigned int m_faultstuckat ;
   unsigned int m_faultyreg ; 
   unsigned int m_srcdest_reg; 
   unsigned int m_start_logging_inst; 
   unsigned int m_compare_point; 

   unsigned int m_inf_loop_start ; 
   int m_fault_inj_inst ;
   la_t m_fault_inj_pc ;
   char m_fault_inj_op[20] ;
   bool m_trans_injected ;
   uint64 m_trans_inj_cyc ;
   uint64 m_fault_inj_cycle ;

	/// Stat variables
	 bool do_fault_injection ;
	 unsigned int m_core_id ;
	 uint64 curr_cycle ;
	 uint64 curr_inst ;
	 uint64 ret_inst ;
	 unsigned int ret_priv ;
	 unsigned int priv ;
	 unsigned int in_func_trap ; 
	 unsigned int non_ret_trap ;
	 unsigned int stat_non_ret_trap ;

	// Arch state mismatch info
	 unsigned int stat_app_arch_state_mismatch ;
	 uint64 stat_app_first_arch_state_c ;
	 uint64 stat_app_first_arch_state_i ;
	 unsigned int stat_os_arch_state_mismatch ;
	 uint64 stat_os_first_arch_state_c ;
	 uint64 stat_os_first_arch_state_i ;

	// Faulty register was in use and this caused arch state
	// mismatch
	 unsigned int stat_app_arch_state_inuse_mismatch ;
	 uint64 stat_app_first_arch_state_inuse_c ;
	 uint64 stat_app_first_arch_state_inuse_i ;
	 unsigned int stat_os_arch_state_inuse_mismatch ;
	 uint64 stat_os_first_arch_state_inuse_c ;
	 uint64 stat_os_first_arch_state_inuse_i ;

	// Mem state mismatch info
	 unsigned int stat_app_mem_state_mismatch ;
	 uint64 stat_app_first_mem_state_c ;
	 uint64 stat_app_first_mem_state_i ;
	 unsigned int stat_os_mem_state_mismatch ;
	 uint64 stat_os_first_mem_state_c ;
	 uint64 stat_os_first_mem_state_i ;


	bool floating_point_masked;

	 // Ctrl state mismatch info
	 bool ctrl_corrupted ;
	 uint64 stat_ctrl_corrupt_c ;
	 uint64 stat_ctrl_corrupt_i ;

	 uint64 stat_first_inject_c ;
	 uint64 stat_first_inject_i ;

	 unsigned int stat_total_inject ;
	 unsigned int stat_total_mask ;
	 unsigned int stat_total_traps ;

	 unsigned int stat_fatal_traps ;
	 unsigned int stat_read_inject ;
	 unsigned int stat_read_mask ;

     uint64 stat_app_cycles;
     uint64 stat_app_instr;
     uint64 stat_os_cycles;
     uint64 stat_os_instr;
     uint64 last_switch_cycle;
     uint64 last_switch_instr;
     uint64 max_os_instrs;
     uint64 cur_os_instrs;
	uint64 num_loop_instr;
     bool   last_priv_mode;
     bool   last_priv_mode_set;
     uint64 os_start_instr;

	 // Low level faults stats
	 unsigned int m_faulty_gate_id;
	 unsigned int m_faulty_fanout_id;
	 string m_faulty_machine ;
	 string m_stim_pipe ;
	 string m_response_pipe ;

     uint64 store_branch_cnt;

	 uint64 detection_cycle;
	 uint64 detection_inst;
	 unsigned int symptom;
         int inv_id;
	 la_t symptom_pc;
	 bool symptom_priv;
	 uint64 stat_os_instr_to_detection;
	 uint64 stat_os_app_cross;
	 uint64 stat_os_app_cross_inuse;

	// Map to record the types of instructions that have been been
	// affected by the fault
	struct const_char_cmp {
		bool operator() (const char* a, const char* b) const {
			return strcmp(a,b)<0 ;
		}
	} ;

	typedef map<const char*, int, const_char_cmp> inst_count_map_t ;
	inst_count_map_t fault_inst_list ;

	// maps to record the mix of instructions. inst_mix records all and
	// faulty_inst_mix records faulty ones
	inst_count_map_t inst_mix ;
	inst_count_map_t faulty_inst_mix ;

	// Map to record the targets of branches that we have seen
	typedef set<ireg_t, ireg_cmp> targets_t ;
	typedef map<ireg_t, targets_t, ireg_cmp> inst_targets_t ;
	inst_targets_t inst_targets ;

	// Map to record the addresses accessed by loads and stores
	typedef map<ireg_t, targets_t, ireg_cmp> mem_access_t ;
	mem_access_t mem_access ;
	ireg_t min_target ;
	ireg_t max_target ;
	
	// Map for min-max locations of addresses of loads and stores
	inst_addr_range_t app_addr_range ;
	inst_addr_range_t os_addr_range ;

	// Table to record the prefetches for given PC
	struct prefetch_data_t {
		ireg_t last_addr ;
		int stride ;
		int counter ;

		prefetch_data_t() { }

		prefetch_data_t(ireg_t addr) {
			last_addr = addr ;
			stride = 0 ;
			counter = 0 ;
		}

		ireg_t update(ireg_t addr) {
			ireg_t new_addr ;
			// counter is a sauturating counter.
			if(counter<PREF_CNT_THRESHOLD) {
				new_addr = 0x0 ;
			} else {
				new_addr = last_addr + stride ;
			}
			int new_stride = addr - last_addr ;
			if(new_stride == stride) {
				// Counter is a saturating counter
				if(counter<(PREF_CNT_SATURATION-1)) {
					counter += 1 ;
				}
				// if(counter==0) counter = 1 ;
			} else {
				stride = new_stride ;
				counter = 0 ;
			}
			last_addr = addr ;
			return new_addr ;
		}

		void print(char *s) {
			// DEBUG_OUT("Here\n") ;
			DEBUG_OUT("%s: 0x%llx, %d, %d\n", s, last_addr, stride, counter);
		}
	} ;
	typedef map<ireg_t, struct prefetch_data_t*, ireg_cmp> prefetch_table_t ;
	prefetch_table_t prefetch_table ;
};

/*************************************************************************
* class multi_core_diagnosis_t
*************************************************************************/
/*

class multi_core_diagnosis_t {
	private:


		//diagnosis information
		static bool diagnosis_started;
		static bool logging_step;
		static bool tmr_step;
		static bool trigger_logging_step[NUM_CORES];
		static bool trigger_tmr_step[NUM_CORES];
	public:
		/// Constructors
		multi_core_diagnosis_t();

		/// Destructor
		~multi_core_diagnosis_t();
		//@}

		static void startMulticoreDiagnosis();
		static bool hasDiagnosisStarted() {return diagnosis_started; }


		static bool isLoggingStep() { return logging_step; }
		static void setLoggingStep() { DEBUG_OUT("Logging Step started\n"); logging_step = true; }
		static void unsetLoggingStep(){ logging_step = false; }
		static void triggerLoggingStep();
		static void loggingStepTriggered(uint32 core_id) {trigger_logging_step[core_id] = false;}
		static bool isTriggerLoggingStep(uint32 core_id) {return trigger_logging_step[core_id];}


		static bool isTMRStep() { return tmr_step; }
		static void setTMRStep() { DEBUG_OUT("TRM Step started\n"); tmr_step = true; }
		static void unsetTMRStep() { tmr_step = false; }

		static void triggerTMRStep();
		static void TMRStepTriggered(uint32 core_id) {trigger_tmr_step[core_id] = false;}
		static bool isTriggerTMRStep(uint32 core_id) {return trigger_tmr_step[core_id];}


};
*/
	

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _DEBUGIO_H_ */
