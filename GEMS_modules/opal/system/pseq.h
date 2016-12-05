
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
#ifndef _PSEQ_H_
#define _PSEQ_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include "diagnosis.h"
#include "inv-defs.h" //Invariant definitions
#include "ddg.h" 

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define LOOP_HASH_SIZE 100
#define INF_LOOP_THRESHOLD 500000
#define RANGE_THRESHOLD 200
#define TAGS_IDX 0
#define CNT_IDX 1
#define LAST_SEQ_IDX 2
#define RANGE_IDX 3
#define VALUE_START 8

// If the execution spends more than X instructions in the OS, we
// deem that it has something wrong.
// #define OS_INSTR_THRESHOLD 20000 // This is for SPEC
#define OS_INSTR_THRESHOLD 500000 // This is for SPEC test
#define OS_INSTR_THRESHOLD_MULTI 50000

/// If we see more than TRAP_RATE traps in every TRAP_RATE_RESET
/// instructions committed, we say trap rate is too high. The curr_trap_rate
/// counter is reset every TRAP_RATE_RESET number of commits
#define TRAP_RATE 50
#define TRAP_RATE_RESET 1000
/// The status of the fetch unit: ready to fetch instructions, or stalled
enum pseq_fetch_status_t {
  PSEQ_FETCH_READY = 0,
  PSEQ_FETCH_ICACHEMISS,
  PSEQ_FETCH_SQUASH,
  PSEQ_FETCH_ITLBMISS,
  PSEQ_FETCH_WIN_FULL,
  PSEQ_FETCH_BARRIER,
  PSEQ_FETCH_MAX_STATUS
};

/// The status of the retire unit
enum pseq_retire_status_t {
  PSEQ_RETIRE_READY = 0,
  PSEQ_RETIRE_UPDATEALL,
  PSEQ_RETIRE_SQUASH,
  PSEQ_RETIRE_LIMIT,
  PSEQ_RETIRE_MAX_STATUS
};

enum operation_mode_t {
	NORMAL=0,
	RECOVERING,
	RECOVERED
};

/// The number of table entries in the instruction page map.
const int PSEQ_IPAGE_TABLESIZE       = 16384;

/// The number of retire opportunities before raising the "deadlock" flag
const int PSEQ_MAX_FWD_PROGRESS_TIME = 1024*64;

/// The number of instruction to fastforward past (when skipping miss handlers)
const uint32 PSEQ_MAX_FF_LENGTH      = 256;

// LXL: structure to record trap info

typedef struct {
  uint64 trap_cycle;
  uint64 instr_seq_num;
  la_t   instr_pc;
  bool   instr_priv;
  uint16 trap_type;
  ireg_t trap_level;
  la_t   trap_handler_pc;
} trap_info;

typedef struct {
    uint64 PC;
    bool isLoad;
    uint64 address;
    uint64 data;
} mem_rec_t;

enum tlb_reg_t {
    ctxt_nucleus, 
    ctxt_primary, 
    ctxt_secondary, 
    dcu_ctrl, 
    dsfar,
    dsfsr, 
    dtag_access, 
    dtag_access_ex, 
    dtag_target, 
    dtlb_2w_daccess,
    dtlb_2w_tagread, 
    dtlb_fa_daccess, 
    dtlb_fa_tagread, 
    dtsb, 
    dtsb_nx, 
    dtsb_px,
    dtsb_sx, dtsbp64k, dtsbp8k, dtsbpd, ec_control, isfsr,
    itag_access, itag_target, itlb_2w_daccess, itlb_2w_tagread, itlb_fa_daccess, 
    itlb_fa_tagread, itsb, itsb_nx, itsb_px, itsbp64k, itsbp8k,
    last_etag_write, lfsr, lsu_ctrl, madr_1, madr_2, madr_3, madr_4,
    mem_address_control, mem_tmg_1, mem_tmg_2, mem_tmg_3, mem_tmg_4, mem_tmg_5,
    pa_watchpoint, trace, va_watchpoint, num_tlb_regs
};

typedef struct {
    attr_value_t reg[num_tlb_regs];
} tlb_data_t;

typedef struct {
    la_t VPC;
    half_t opcode;
    int fetch_id;
    uint16 win_id;
    int32 exec_unit;
    byte_t exec_unit_id;
    int res_bus;
} old_instr_diag_info_t;

typedef struct {
    uint64 address;
    int access_size; 
    ireg_t data[MEMOP_MAX_SIZE];
} mem_event_t;



/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Processor Sequencer - one processor's sequencer - represents a single
* thread of execution on a SMT machine.
*
* Each sequencer can wait on the L1 i-cache. if instruction fetch stalls,
* the sequencer waits for the cache to return with the data (e.g. a call
* to "Wakeup()". The next cycle the sequencer resumes fetching.
*
* @see     iwindow_t, waiter_t
* @author  cmauer
* @version $Id: pseq.h 1.101 06/02/13 18:49:20-06:00 mikem@maya.cs.wisc.edu $
*/

class pseq_t : public waiter_t, public out_intf_t {

public:
  /// @name debug constants
  //@{
  /// debug simple front end
  static const bool DEBUG_SIMPLE_FE = false;
  /// debug multicycle front end
  //static const bool DEBUG_MULTICYCLE_FE = false;
  static const bool DEBUG_MULTICYCLE_FE = true;
  //@}

  /// @name constants
  //@{
  /// invalid line address used in the multicycle I$ frontend
  static const pa_t PSEQ_INVALID_LINE_ADDRESS = ~(pa_t)0;
  //@}

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param id    Integer idenfier for this sequencer
   */
  pseq_t( int32 id, multicore_diagnosis_t *m_multicore_diagnosis );

  /**
   * Destructor: frees object.
   */
  virtual ~pseq_t();
  //@}

  /**
   * @name Methods
   */
  //@{

  /** advances one clock cycle for this processor.
   *
   *  Includes fetching, decoding, scheduling, executing, and retiring
   *  instructions. Each of these sub-tasks is broken out as private
   *  methods of this class.
   */
  void advanceCycle( void );

  /// Fetch a number of instructions past the current PC
  void fetchInstruction( void );
  /// Decodes instructions which have been fetched.
  void decodeInstruction( void );
  /** Schedules instructions which have been decoded on the functional
   *  units in the processor. */
  void scheduleInstruction( void );
  /** Retires instructions from the instruction window. */
  void retireInstruction( void );

  /** Cause an exception on this sequencer. This schedules an exception
   *  if there are no outstanding exceptions more recent.
   *
   *  @param type      The type of exception
   *  @param win_index The point in the program where the exception occurred.
   *  @param offender  The type of instruction causing the exception
   *  @param continue_at  The program counter to start fetch at
   *  @param access_addr If a memory operation caused the exception, the
   *                     address of the memory location being accessed.
   */
  void raiseException(exception_t type, uint32 win_index,
                      enum i_opcode offender,
                      abstract_pc_t *continue_at,
                      my_addr_t access_addr,
                      uint32 execption_penalty);
  /** clears the exception fields. */
  void clearException( void );

  /** Takes a trap.
   *  @param  instruction  The instruction that causes the trap
   *  @param  inorder_at   Contains a cache of the inorder fetch/pstate info
   *  @param  is_timing    True if called from the timing context
   *  @return bool         True if the instruction must be checked
   *                       (e.g. unimplemented instruction) */
  bool takeTrap( dynamic_inst_t *instruction, 
                 abstract_pc_t  *inorder_at,
                 bool is_timing );

  /** Squash execution back to a last_good instruction. Start executing
   *  with a new program counter (abstract pc).
   *  @param last_good  The last good instruction in the instr window to exec
   *  @param offender   The instruction causing the exception
   *  @param fetch_at   The PC to start executing at.
   */
  void partialSquash( uint32 last_good, abstract_pc_t *fetch_at,
                      enum i_opcode offender );
  
  /** Squash all instructions in the window. The offending instruction
   *  is required for statistics. Fetch restarts at the inorder pc & npc
   *  of the architected state.
   *  @param offender   The instruction causing the exception
   */
  void fullSquash( enum i_opcode offender );

  /** postEvent is called to schedule a wakeup sometime in the future.
   *  This interface calls to the 'm_scheduler' that owns the event queue.
   *  @param waiter The waiter to be woken up in the future
   *  @param cyclesInFuture The number of cycles in the future when the
   *                        wakeup should occur.
   */
  void postEvent( waiter_t *waiter, uint32 cyclesInFuture );

  /** Wakeup(): is called when the L1 i-cache returns with a the next
   *  instruction to be fecthed cache line.
   */
  void    Wakeup( void );

  /** unstallFetch: is called by retiring instructions to restart
   *   instruction fetch. This allows instructions to have 'in-order'
   *   barrier-like semantics in an out-of-order machine. Like, I am
   *   going to modify the control register, and there is a dependence
   *   between what I do and the next instruction fetched. Another
   *   example is writes to the MMU-- translation can't complete until
   *   the store that maps a page has been executed.
   */
  void    unstallFetch( void );

  /** prints out the control flow graph for debugging */
  void    printCFG( void );

  /** Get an idealized (in-order) register file for this processor */
  reg_box_t &getIdealBox( void ) {
    return m_inorder.regs;
  }

  /** Simulates an in-order functional processor. */
  void                stepInorder( void );
  /** Runs the inorder functional processor up to a certain inorder index */
  pseq_fetch_status_t idealRunTo( int64 runto_seq_num );
  /** provides an oracle branch prediction */
  pseq_fetch_status_t oraclePredict( dynamic_inst_t *d, bool *taken, 
                                     abstract_pc_t *inorder_at );

  /** re-synchronize between the oracle predictor and simics */
  void    idealResync( void );

  /** provides an interface for oracle fetch */
  void    oracleFetch( int64 seq_num);
  /** Runs the inorder functional processor ahead of the timing model */
  void    runInorderAhead( void );
  /** synchronize the checked state with simics's */
  void    idealCheckTo( int64 seq_num );

  /** warmup the cache.
   *  This interface takes a memory transaction from simics, and inserts
   *  the address into the cache hierarchy. It does not provide any
   *  additional timing information, it just "busts" the data in there.
   */
  void    warmupCache( memory_transaction_t *mem_op );

  /** start the wall-clock performance timing */
  void    startTime( void );
  /** stop  the wall-clock performance timing */
  void    stopTime( void );

  /** log the number of logical */
  void    logLogicalRenameCount( rid_type_t rid, half_t logical, 
                                 uint32 count );
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  
  /** returns the id of this sequencer */
  int32          getID( void ) {
    return m_id;
  }
  /** Gets a pointer to the load/store queue -- callers may modify the lsq.
   *  @returns A pointer to the load/store queue */
  lsq_t       *getLSQ( void ) {
    return m_lsq; }
  /** Gets a pointer to the instruction window */
  iwindow_t   *getIwindow( void ) {
    return &m_iwin; }
  /** Gets a pointer to the scheduler */
  scheduler_t *getScheduler( void ) {
    return m_scheduler; }
  /** Gets a pointer to the Miss Status Holding Register */
  mshr_t      *getMSHR( void ) {
    return m_mshr; }

  /** Gets a pointer to direct branch predictor */
  direct_predictor_t  *getDirectBP( void ) {
    return m_bpred;
  }
  /** Gets a pointer to indirect branch predictor */
  cascaded_indirect_t *getIndirectBP( void ) {
    return m_ipred;
  }
  /** get a pointer to the speculative branch predictor state */
  predictor_state_t *getSpecBPS( void ) {
    return (m_spec_bpred);
  }
  /** get a pointer to the architected branch predictor state */
  predictor_state_t *getArchBPS( void ) {
    return (m_arch_bpred);
  }
  /** set the architected branch predictor state */
  void setArchBPS(predictor_state_t &new_arch_bpred) {
    *m_arch_bpred = new_arch_bpred;
  }
  /** set the architected branch predictor state */
  void setSpecBPS(predictor_state_t &new_arch_bpred) {
    *m_spec_bpred = new_arch_bpred;
  }
  /** Gets a return address stack pointer */
  ras_t  *getRAS( void ) {
    return m_ras;
  }
  /** Gets the trap level stack predictor */
  tlstack_t *getTLstack( void ) {
    return m_tlstack;
  }

  /** gets the memory management unit's ASI interface 
   *       This interface deals with reading and writing to the MMU's 
   *       internal registers, so use with care. */
  mmu_interface_t *getASIinterface( void ) {
    return m_mmu_asi;
  }

  /** Abstract interface to all register files */
  reg_box_t &getRegBox( void ) {
    return m_ooo.regs;
  }

  /** get the memory operation trace */
  memtrace_t *getMemtrace( void ) {
    return m_memtracefp;
  }
  /** get the l1 data cache interface */
  cache_t *getDataCache( void ) {
    return (l1_data_cache);
  }
  /** get the l1 instruction cache interface */
  cache_t *getInstructionCache( void ) {
    return (l1_inst_cache);
  }
  /** get the ruby cache interface */
  rubycache_t *getRubyCache( void ) {
    return (m_ruby_cache);
  }

  /** get the number of instructions retired on this processor */
  uint64       getRetiredICount( void ) {
    return (m_local_icount);
  }
  /** get the local cycle count */
  tick_t       getLocalCycle( void ) {
    return (m_local_cycles);
  }
  /** step the sequence number */
  uint64       stepSequenceNumber( void ) {
    uint64 number = m_local_sequence_number;
    m_local_sequence_number = m_local_sequence_number + 1;
    return (number);
  }
  /** unstep the sequence number::
   *  This should only be called when instructions are squashed.
   *  Otherwise, the LSQ state will get corrupted (relies on sequence ordering)
   */
  void         decrementSequenceNumber( void ) {
    m_local_sequence_number = m_local_sequence_number - 1;
  }
  /** step the sequence number */
  uint64       stepInorderNumber( void ) {
    uint64 number = m_local_inorder_number;
    m_local_inorder_number = m_local_inorder_number + 1;
    return (number);
  }
  
  /** increment the local cycle counter */
  void         localCycleIncrement( void ) {
    m_local_cycles++;
  }

  /** statistics object holding per dynamic instruction stats */
  decode_stat_t* getOpstat( void ) {
    return ( &m_opstat );
  }
  //@}

  /**
   * @name Simics specific interfaces
   */
  //@{
  /// Simics Specific: get/set interfaces (specifically the mmu)
  void   installInterfaces( void );

  /// Simics Specific: remove interfaces on uninstall
  void   removeInterfaces( void );

  /// Advance Simics (or a trace) a given number of steps
  bool   advanceSimics( void );

  /// Fastforward Simics until a retry statement is hit
  void   fastforwardSimics( void );

  /** context switch at a processor.
   *  @param context    The new context for this processor
   */
  void   contextSwitch( context_id_t new_context );

  /** RUBY specific: called when a memory request is completed */
  void   completedRequest( pa_t physicalAddr );
  
  /** read a physical memory location
   *
   * @param phys_addr The physical address to read from.
   * @param size      The number of bytes to read
   * @param result_reg A pointer to where the result should be written
   * @return bool true if the read succeeds, false if TBE access fails
   */
  bool   readPhysicalMemory( pa_t phys_addr, int size, ireg_t *result_reg );

  /** write a physical memory location
  */
  bool   writePhysicalMemory( pa_t phys_addr, int size, ireg_t *result_reg );

  bool   translateInstruction( ireg_t log_addr, int size, pa_t &phys_addr);

  /** read an instruction from simics memory
   *
   * @param cur_pc       The virtual (logical) PC to read the instruction from.
   * @param physical_pc  The physical address of this program counter.
   * @param traplevel    The interrupt level of the processor
   * @param pstate       The PSTATE register of the processor
   * @param next_instr_p A 32-bit integer where the next instruction will
   *       be written
   * @return bool true if the get succeeds, false if it fails
   */
  bool   getInstr( la_t cur_pc, int32 traplevel, int32 pstate,
                   pa_t *physical_pc, unsigned int *next_instr_p );

  /** read an instruction from memory, using simics's current trap level
   *  and processor state as an implicit parameter.  Otherwise,
   *  same as getInstr().
   */
  bool getInstr( la_t cur_pc, 
                 pa_t *physical_pc, unsigned int *next_instr_p );

  /** access the memory management unit to do address translation */
  trap_type_t mmuAccess( la_t address, uint16 asi, uint16 accessSize,
                         OpalMemop_t op, uint16 pstate, uint16 tl,
                         pa_t *physAddress);

  /** post an exception to this processor */
  void postException( uint32 exception );

  /** find the pid of this processor */
  int32 getPid( void );
  /** given a logical address to start with, find the PID */
  int32 getPid( la_t thread_p );

  /** get the thread pointer (of the currently running thread on this proc) */
  la_t getCurrentThread( void );
  
  /** set the physical address of this processors 'thread' pointer */
  void setThreadPhysAddress( la_t thread_phys_addr );

  /** get the physical address of this processors 'thread' pointer */
  la_t getThreadPhysAddress( void );
  //@}

  /**
   * @name Configuration interfaces: used to save/restore state.
   */
  //@{
  /// register checkpoint interfaces
  void  registerCheckpoint( void );

  /// begin writing new checkpoint
  void  writeCheckpoint( char *checkpointName );

  /// open a checkpoint for reading
  void  readCheckpoint( char *checkpointName );
  //@}

  /**
   * @name Trace recording interfaces
   */
  //@{
  /** open a trace file for writing
   *  @param traceFileName The name of the trace file.
   */
  void    openTrace( char *traceFileName );
  /** Log the PC, instruction, and register information to the trace file */
  void    writeTraceStep( void );
  /** Log the just the PC ... to the trace file */
  void    writeSkipTraceStep( void );
  /** close the trace */
  void    closeTrace( void );

  /** attach to a trace file for reading
   *  @param traceFileName The name of the trace file.
   *  @param withImap      True if the imap is saved in a file, false if not
   */
  void    attachTrace( char *traceFileName, bool withImap );
  /** attach to a memory trace file for reading */
  void    attachMemTrace( char *traceFileName );
  /** attach to a TLB map file */
  void    attachTLBTrace( char *traceFileName );
  /** log a memory transaction to the trace file */
  void    writeTraceMemop( transaction_t *trans );

  /** open a branch trace file for writing
   *  @param traceFileName The name of the trace file.
   */
  void    openBranchTrace( char *traceFileName );
  /** write branch step: write the next instruction in the trace */
  void    writeBranchStep( void );
  /** write next file: write next sequential file in branch trace */
  bool    writeBranchNextFile( void );
  /** close the trace */
  void    closeBranchTrace( void );
  
  /** load the next PC, instruction in the trace.
   *  @param  a       Contains virtual pc, and tl of this instruction
   *  @param  ppc     reference to the next physical pc in the trace
   *  @param  instr   reference to the next instruction in the trace
   *  @return bool true if valid, false if at the end of a trace
   */
  bool    readTraceStep( la_t &vpc, pa_t &ppc, unsigned int &instr );

  /** query for an instruction in the instruction maps.
   */
  bool    queryInstruction( pa_t fetch_ppc, static_inst_t * &s_instr );
  
  /** inserts an instruction into the instruction cache.
   *  @param  ppc     The physical pc of ths instruction
   *  @param  instr   The instruction at the virtual address "vpc"
   *  @return static_inst_t A pointer to the new static instruction.
   */
  static_inst_t *insertInstruction( pa_t fetch_ppc, unsigned int instr );

  /** Invalidates instruction in the decoded instruction cache.
   *  This refers to the Opal's (software simulator) cache of decoded
   *  instructions, not invalidating entries in the i-cache.
   *  @param  address   The physical address to be invalidated.
   */
  void    invalidateInstruction( pa_t address );

  /** Write out the (flat) instruction file (to enable prefetching) */
  void    writeInstructionTable( char *imapFileName );

  /** Read in the (flat) instruction file (to enable prefetching)
   *  @return true if successful, false if not */
  bool    readInstructionTable( char *imapFileName, int context );

  /** Allocate and initialize the ideal in-order processor register file */
  void    allocateFlatRegBox( mstate_t &inorder );
  /** Clear flat register dependencies */
  void    clearFlatRegDeps( mstate_t &inorder, flow_inst_t *predecessor );
  //@}

  /**
   * @name Statistics functions
   */
  //@{
  /// initialize statistics
  void    initializeStats( void );
  //@}

  /**
   * @name Debugging / Testing
   */
  //@{
  /** Formats and print out an abstract pc to the log */
  void    printPC( abstract_pc_t *a );
  /** Print out the state of the entire microprocessor */
  void    print( void );
  /** print the in flight instructions */
  void    printInflight( void );
  /** print statistics */
  void    printStats( void );
  /** validate the trace (print out its contents). */
  void    validateTrace( void );
  /** print out the contents of the instruction pages
   *  @param  verbose False if instructions should only be counted not printed
   *  @return uint32  The number of instructions printed
   */
  uint32  printIpage( bool verbose );

  /** translates a fetch status into a string */
  static const char* fetch_menomic( pseq_fetch_status_t status );

  /** translates a retire status into a string */
  static const char* retire_menomic( pseq_retire_status_t status );
  //@}

	/** get the retire rf **/
  arf_control_t* getControlARF() { return m_control_arf; }
  
  bool isInjectionInstr(dynamic_inst_t * d);
  void checkAndInjectTransientFault(dynamic_inst_t *d, int dest_src);
  
#ifdef ARCH_REGISTER_INJECTION
  void injectTransientFault(half_t reg_no);
#else
  void injectTransientFault();
#endif

#ifdef DETECT_INF_LOOP
  void isLoop(dynamic_inst_t* cinst);
#endif

  int getWayCounter() { return way_counter; } 
  void incWayCounter() { way_counter++; } 
  int fetchIncWayCounter() { return way_counter++; }
  int fetchIncFWayCounter() { return fway_counter++; }
  int fetchIncAgenCounter() { return agen_counter++; }

  int getTrapRate() { return curr_trap_rate ; } 

#ifdef MEASURE_FP
  void incFP(int i, int j) { FP_util[i-4][j]++; }
  int getFP(int i, int j) {return FP_util[i-4][j]; }
#endif

  bool inIdleLoop(la_t virtual_address);

 bool compareState(core_state_t *ps);

/****************************************************
Wrapper functions
****************************************************/
  void clearDoFaultInjection()
  {
	fault_stat->clearDoFaultInjection();
  }
  void setDoFaultInjection()
  {
	fault_stat->setDoFaultInjection(); 
  }
  bool getDoFaultInjection()
  {
	return fault_stat->getDoFaultInjection();
  }
  int getFaultType()
  {
	return fault_stat->getFaultType();
  }
  int getFaultBit()
  {
	return fault_stat->getFaultBit();
  }
  int getFaultStuckat()
  {
	return fault_stat->getFaultStuckat();
  }
  int getFaultyReg()
  {
	return fault_stat->getFaultyReg();
  }
  void setFaultyReg(int r)
  {
	fault_stat->setFaultyReg(r);
  }
  void setStartLogging(int r)
  {
	fault_stat->setStartLogging(r);
  }
  void setComparePoint(int r)
  {
	fault_stat->setComparePoint(r);
  }

  void setSrcDestReg(int r)
  {
	fault_stat->setSrcDestReg(r);
  }


  //----------------------------------------------- GATE LEVEL SAFSIM ----------------------------------------------//
  /// Sets the faulty gateId
  void  setFaultyGateId( int s )
  {
	  fault_stat->setFaultyGateId(s);
  }

  /// Sets the faulty fanoutId
  void  setFaultyFanoutId( int s )
  {
	  fault_stat->setFaultyFanoutId(s);
  }

  /// Print saf statistics
  /*void printSafFaultStats( )
	{
	fault_stat->printSafFaultStats() ;
	}
   */
  // NCVLOG RELATED

  /// Sets the faulty machine (faulty net) name
  void  setFaultyMachine( string s )
  {
	  fault_stat->setFaultyMachine(s);
  }
  void  setStimPipe( string s )
  {
	  fault_stat->setStimPipe(s);
  }
  void  setResponsePipe( string s )
  {
	  fault_stat->setResponsePipe(s);
  }

  // ------------GATE LEVEL commands ----------//
  byte_t injectFault(byte_t value)
  {
	return fault_stat->injectFault(value);
  }

  half_t injectFault(half_t value)
  {
	return fault_stat->injectFault(value);
  }

  uint32 injectFault(uint32 value)
  {
	return fault_stat->injectFault(value);
  }

  uint64 injectFault(uint64 value)
  {
	return fault_stat->injectFault(value);
  }


  void setClock(uint64 c)
  {
	fault_stat->setClock(c);
  }


  void setRetInst(uint64 i)
  {
	fault_stat->setRetInst(i);
  }

  uint64 getRetInst()
  {
	return fault_stat->getRetInst();
  }

  void setRetPriv(unsigned int i)
  {
	fault_stat->setRetPriv(i);
  }

  unsigned int getRetPriv()
  {
	return fault_stat->getRetPriv();
  }

  void setCurrInst(uint64 i)
  {
	fault_stat->setCurrInst(i);
  }

  void setPriv(int p)
  {
	fault_stat->setPriv(p);
  }

  int getPriv()
  {
	return fault_stat->getPriv();
  }

  void inFuncTrap()
  {
	fault_stat->inFuncTrap();
  }

  void clearFuncTrap()
  {
	fault_stat->clearFuncTrap();
  }

  void setNonRetTrap()
  {
	fault_stat->setNonRetTrap();
  }

  void setArchCycle()
  {
	fault_stat->setArchCycle();
  }

  void setArchInuseCycle()
  {
	fault_stat->setArchInuseCycle();
  }

  void setMemCycle()
  {
	fault_stat->setMemCycle();
  }

  uint64 getArchCycle()
  {
	  return fault_stat->getArchCycle();
  }

  bool getArchMismatch(void) {
    return fault_stat->getArchMismatch() ;
  }

  void incTotalInj()
  {
	fault_stat->incTotalInj();
  }

  void incTotalMask()
  {
	fault_stat->incTotalMask();
  }

  void incTotalTraps()
  {
	fault_stat->incTotalTraps();
  }

  void incFatalTraps()
  {
	fault_stat->incFatalTraps();
  }

  void incReadInj()
  {
	fault_stat->incReadInj();
  }

  void incReadMask()
  {
	fault_stat->incReadMask();
  }

  uint64 getInfLoopCycle()
  {
	return fault_stat->getInfLoopCycle();
  }

  void trackCycle(uint64 cur_cycle, uint64 seq_num, bool priv, bool isLoopInstr)
  {
	fault_stat->trackCycle(cur_cycle, seq_num, priv, isLoopInstr);
  }

  uint64 getClock()
  {
	return fault_stat->getClock();
  }

  void incBranches()
  {
	fault_stat->incBranches();
  }

  void incStores()
  {
	fault_stat->incStores();
  }

  void outSymptomInfo(uint64 cyc, uint64 inst, const char* trap_name, unsigned int tt, la_t tpc, int priv, la_t thpc, int tl, int coreId)
  {
	symptomSeen() ;
	fault_stat->outSymptomInfo(cyc, inst, trap_name, tt, tpc, priv, thpc, tl, coreId);
  }

  void outSymptomInfo(uint64 cyc, uint64 inst, const char* trap_name, unsigned int tt, la_t tpc, int priv, la_t thpc, int tl, int coreId, int InvId)
  {
	symptomSeen() ;
	fault_stat->outSymptomInfo(cyc, inst, trap_name, tt, tpc, priv, thpc, tl, coreId, InvId);
  }

  bool symptom_seen ;
  void symptomSeen() { symptom_seen = true ; }
  bool getSymptomSeen() { return symptom_seen ; }
/*   void outTrapInfo(uint64 cyc, unsigned int inst, const char* trap_name, unsigned int tt, la_t tpc, int priv, la_t thpc, int tl) */
/*   { */
/* 	fault_stat->outTrapInfo( cyc, inst, trap_name, tt, tpc, priv, thpc, tl); */
/*   } */

  void setFaultInjInst(int value)
  {
	fault_stat->setFaultInjInst(value);
  }


//extra wrapper functions 

  void setFaultType(int t)
  {
	fault_stat->setFaultType(t);
  }

  void setFaultBit(int t)
  {
	fault_stat->setFaultBit(t);
  }

  void setFaultStuckat(int t)
  {
	fault_stat->setFaultStuckat(t);
  }

  void setInfLoopCycle(uint64 cycle)
  {
	fault_stat->setInfLoopCycle(cycle);
  }

  void setFaultInjCycle(int cycle)
  {
	fault_stat->setFaultInjCycle(cycle);
  }

  void printFaultStats()
  {
	fault_stat->printFaultStats();
  }

  int getFaultSrcDst()
  {
	return fault_stat->getFaultSrcDst();
  }

  int getOSInstr()
  {
	return fault_stat->getOSInstr();
  }

  void clearOSInstr()
  {
	fault_stat->clearOSInstr();
  }

  ireg_t getCheckpointReg(reg_id_t&rid);

#ifdef SW_ROLLBACK
  bool isRecovering() { return m_op_mode==RECOVERING;}
#endif
  
   bool isAppAbort(la_t pc) ;

   la_t panicPC();

  llb_t* getLLB() { return llb; }
  bool addLLBEntry(uint64 addr, uint64 *value, int num_values);
  bool addLLBEntry(uint64 addr, uint64 value);

	void log_func_inst_info(dynamic_inst_t *d);	
	void restore_func_inst_info(dynamic_inst_t *d);
	void readMemory(ireg_t log_addr, int size, ireg_t *result_reg);

#ifdef LLB_ANALYSIS
  vector<uint64> dummy_directory;
  uint64 num_values;
  void find_and_add_in_directory(uint64 * values, int size);
  void print_llb_analysis();
#endif

   bool isKernelPanic(la_t pc) ;
   void setStartPC() { start_pc = m_fetch_at->pc ; }
   void setStopPC() { stop_pc = m_fetch_at->pc ; }
   char *getStartStopPC() {
	   char pcs[100] ;
	   int n = sprintf(pcs,"0x%llx -> 0x%llx\n", start_pc, stop_pc) ;
	   return pcs ;
   }
  //**************************************************************************
 
private:

  /**
   * @name Fetch Helper functions
   */
  /** Implements a simple (easily idealizable) front end */
  void            fetchInstrSimple( void );

  ///// lookup a static instruction during fetch, used by fetchInstruction
  //void            lookupInstruction( const abstract_pc_t *apc,
  //                                   static_inst_t** s_instr_ptr,
  //                                   pa_t* fetchPhysicalPC_ptr );
  
  /// lookup a static instruction during fetch, used by fetchInstruction
  /// Overloaded for fault injection. Inject only when fetch_num == 0
  void            lookupInstruction( const abstract_pc_t *apc,
                                     static_inst_t** s_instr_ptr,
                                     pa_t* fetchPhysicalPC_ptr,
		 							 uint32 fetch_num );

  /** Allocate a dynamic instruction during fetch, used by fetchInstruction.
   *  This function uses the cwp/gset information and modifies 'fetch_at'.
   *  Task: Fetch an instruction, insert it in the instruction window,
   *        Tag this instruction as Fetched,
   *        Advance PC/NPC state contained in fetch_at speculatively.
   */
  dynamic_inst_t *createInstruction( static_inst_t *s_instr, 
                                     abstract_pc_t *fetch_at );
  
  /**
   * @name Check functions for validating execution.
   */
  //@{

  /** analyze statistics for the last run
   */
  void            threadStatAnalyze( void );

  /** commitObserver: This method observes the committed instruction stream.
   */
  void            commitObserver( dynamic_inst_t *dinstr );

  /** uncheckedRetire: stores an instruction until simics is stepped past it
   *  @param dinst         The instruction to retire
   *  @return bool         true of retirement must happen immediately
   *                       otherwise unchanged.
   */
  bool            uncheckedRetire( dynamic_inst_t *dinstr );

  /** reads unchecked retires out of the instruction buffer.
   *  Used at check time to update statistics. You must free
   *  these instructions!!
   */
  dynamic_inst_t *getNextUnchecked( void );

  void clearUnchecked( void );

  /** returns a memory instruction if there is an unchecked store instruction
   *  which has written to the same location.
   */
  memory_inst_t  *uncheckedValueForward( pa_t phys_addr, int my_size );

  /** Prints out unchecked instructions.
   */
  void            uncheckedPrint( void );

  /** push a recently retired instruction to a debugging buffer */
  void            pushRetiredInstruction( dynamic_inst_t *dinstr );

  /** print out the buffer of recently retired instructions */
  void            printRetiredInstructions( void );

  /** Helper function for checkAll and checkChanged state functions.
   *  This checks the critial portions of the state, such as PC and PSTATE
   *  registers.
   *  @param  result      contains the result of the comparison
   *  @param  mstate      Machine state (register box, abstract PC) to check.
   */
  void    checkCriticalState( check_result_t *result, mstate_t *mstate, bool init );

  /** Compare and update the simics architectural state with this sequencer's.
   *  This updates all of our registers, and returns if everything matches.
   *  @param  result      contains the result of the comparison
   *  @param  mstate      Machine state (register box, abstract PC) to check.
   */
  void    checkAllState( check_result_t *result, mstate_t *mstate, bool init );
  void    syncAllState( check_result_t *result, mstate_t *mstate, bool init );

  /** Compare and update changes we think we have made of a dynamic
   *  instruction. This updates only the registers we think have changed.
   *  @param  result     Contains the check result ('critical', 'verbose', etc)
   *  @param  mstate     The state after this instructions execution
   *  @param  d_instr    The dynamic instruction to be checked.
   */
  void    checkChangedState( check_result_t *result, mstate_t *mstate,
                             dynamic_inst_t *d_instr, bool init );

  /** Writes the destination registers of dinstr with the results from simics
   */
  void    updateInstructionState( dynamic_inst_t *dinstr, bool init );
  /** Comes in two varieties: dynamic and flow instructions. */
  void    updateInstructionState( flow_inst_t *flow_inst, bool init );

  /** allocate and initialize the out-of-order processor state */
  void    allocateRegBox( mstate_t &ooo );
  /** Allocate and initialize the ideal in-order processor state */
  void    allocateIdealState( void );
  //@}

  // LXL: is trap fatal?
  bool    isTrapFatal(uint16 traptype);

  void    resetWayCounter() { way_counter = 0 ; agen_counter=0; fway_counter=8;}

  void rollbackRecovery();
  void notifyAmberRollback() ;
  void checkRegisterReady();

#ifdef SW_ROLLBACK
  void initializeRecoveryInstrMem();
  unsigned int getNextRecoveryInstr();
#endif

  ireg_t readFuncSimValue(reg_id_t&rid);

  /** @name Private Variables */

  /// identifier for this sequencer
  int32      m_id;

	int32 current;
	int32 previous;
	int32 cpu_os;
	int32 cpu_os_idle;
	int32 cpu_app;
	int32 previous_os;
	int32 previous_op;
	trap_type_t previous_trap_type;
	uint64 previous_trap_level;
	uint64 previous_pc;
	bool dont_count;
	bool track;
	uint64 current_inst_num;

  /// local time for this processor
  tick_t     m_local_cycles;
  /// local dynamic (issue) sequence number for this processor:
  ///     # of instructions decoded
  uint64     m_local_sequence_number;
  /// local sequence number for inorder model for this processor:
  uint64     m_local_inorder_number;
  /// local instruction count for this processor: # of instr's completed.
  uint64     m_local_icount;
  /// simics pc step time for last event
  pc_step_t  m_simics_time;

  /// instruction window:
  iwindow_t  m_iwin;

  /// miss status holding register
  mshr_t    *m_mshr;

  /**
   * @name  Core state (registers, fetch state)
   */
  //@{
  /** scheduler: Contains the event queue for instructions.
   *             Wakes up dynamic instructions for execution. */
  scheduler_t     *m_scheduler;

  // The out-of-order registers, fetch at state, and critical regs
  mstate_t         m_ooo;

  /** The non-speculative in-order program counter -- 
   *  Only the PC and NPC fields of this structure should be used.
   *  This is maintained separately from the control register information,
   *  because if it were the control PC & NPCs would need to be renamed
   *  for each instruction.
   */
  abstract_pc_t   *m_inorder_at;

  /** The speculative abstract program counter. This structure contains
   *  the PC and NPC, both of which are needed to determine the program's
   *  control flow. The PC points one instruction past the current retired
   *  program counter. (i.e. at the next instruction to fetch)
   *
   *  NOTE: this redundantly points to the PC in the m_ooo structure.
   *  I kept it here because its name is much more descriptive to me.
   */
  abstract_pc_t   *m_fetch_at;
  
  /// CM FIX: I'd like to remove both the control rf, and the cc rf pointers,
  //          and only leave access through the "arf" interface.
  /// condition-code registers
  /// physical condition code registers
  physical_file_t *m_cc_rf;
  /// condition code map at retire time
  reg_map_t       *m_cc_retire_map;

  /// physical control state register register files
  physical_file_t **m_control_rf;
  /// control register file abstract interface
  arf_control_t    *m_control_arf;

  /** debugging variable: shadow pstate:
   *  detects when the processor state changes, so we can minimize debugging */
  uint16     m_shadow_pstate;
  
  /** The insertion point into the recent retires buffer. */
  int32      m_recent_retire_index;

  /** debugging variable: recent_retires
   *  A buffer recording state about recently retired instructions.
   */
  dynamic_inst_t **m_recent_retire_instr;
  //@}

  /** LSQ: pointer to the load/store queue for this processor */
  lsq_t       *m_lsq;

  /** @name Cache support: L1 I/D caches, L2 caches */
  //@{
  /// L1 miss status holding register structure
  mshr_t      *il1_mshr;
  mshr_t      *dl1_mshr;

  /// L1 instruction cache
  generic_cache_template<generic_cache_block_t> *l1_inst_cache;
  /// L1 data cache
  generic_cache_template<generic_cache_block_t> *l1_data_cache;

  /// L2 miss status holding register structure
  mshr_t      *l2_mshr;
  /// L2 (unified) cache
  generic_cache_template<generic_cache_block_t> *l2_cache;
  /// alternative to (uniprocessor cache structure): the ruby cache
  rubycache_t *m_ruby_cache;
  //@}

  /** @name MMU support: I/D-TLB, contexts */
  //@{

  /// MMU: memory management unit interface (may model a Spitfire MMU)
  mmu_interface_t *m_mmu_access;

  /// MMU: memory management unit's ASI interface
  mmu_interface_t *m_mmu_asi;

  conf_object_t * m_mmu_object;

  /** primary context for the current process running on the computer
   */
  context_id_t     m_primary_ctx;

  /** A cached translation of logical to physical address for the ITLB.
   *  This single entry gives the physical address for the current page
   *  instruction fetch is being performed on. The physical address for
   *  the itlb mini-cache.
   */
  pa_t          m_itlb_physical_address;
  /** The logical address for the itlb mini-cache */
  la_t          m_itlb_logical_address;

  /** A TLB- used in stand-alone runs- to examine the effects of different
   *  virtual->physical memory mappings
   */
  dtlb_t       *m_standalone_tlb;

  /** the mmu hap handler */
  hap_handle_t  m_mmu_haphandle;
  
  /** the physical address of this sequencers thread id */
  pa_t          m_thread_physical_address;
  //@}

  /** control flow predictors - direct branch predictor
   * (bpred and ipred can share resources with other sequencers) */
  direct_predictor_t   *m_bpred;
  /** control flow predictors - indirect branch predictor */
  cascaded_indirect_t  *m_ipred;

  /** pointer to the return address stack - */
  ras_t     *m_ras;
  /** pointer to the trap return stack */
  tlstack_t *m_tlstack;

  /** speculative predictor state -- */
  predictor_state_t *m_spec_bpred;
  /** architected predictor state -- */
  predictor_state_t *m_arch_bpred;
  
  /** @name Fetch unit fields. */
  //@{
  /** The fetch unit status. PSEQ_FETCH_* constants only
   */
  pseq_fetch_status_t m_fetch_status;

  /// The future time when the fetch will resume after the squash
  tick_t     m_fetch_ready_cycle;

  /// The next line address used to access multicycle I$
  pa_t       m_next_line_address;

  /// flag to tell line buffer fetch to stall
  bool       m_line_buffer_fetch_stall;

  /// requested line address by the fetch engine
  pa_t       m_fetch_requested_line;

  /// flag to tell if the engine is in waiting mode
  bool       m_wait_for_request;

  /** last physical address fetched from the I$, used to emulate
   *  the line buffer, which means each cache line is fetch only
   *  once if !same_line(pc1, pc2)
   *
   *  In the case of multicycle I$ frontend, this is the current
   *  line buffer address
   */
  pa_t       m_last_fetch_physical_address;

  /// In multicycle I$, this is the line buffer address of last cycle
  pa_t       m_last_line_buffer_address;

  /** Indicates the number of instructions retired, since stepping simics.
   */
  uint32           m_unchecked_retires;
  /** top of the unchecked retire stack: lets retires be fetched in order. */
  uint32           m_unchecked_retire_top;
  /** a map from store address to a dynamic instruction for unchecked retires*/

  /** A buffer recording state about retired instructions, which haven't
   *  yet been commited by simics.
   */
  dynamic_inst_t **m_unchecked_instr;

  /** a static instruction which is fetched when an i-tlb miss happens
   */
  static_inst_t   *m_fetch_itlbmiss;

  /** A circular buffer holding the address of I$ lines successfully
   *  fetched each cycle. */
  FiniteCycle<pa_t>   m_i_cache_line_queue;

  /** A circular buffer holding the number of instructions successfully
   *  fetched each cycle. */
  FiniteCycle<uint32> m_fetch_per_cycle;

  /** A circular buffer holding the number of instructions successfully
   *  decoded each cycle. */
  FiniteCycle<uint32> m_decode_per_cycle;

  // FIXFIXFIX: put in a better place

  /// The scheduling actor: waits for inputs to become ready, then executes the instruction
  actor_t    *m_act_schedule;
  //@}

  /**
   * @name Retire related members -- includes exceptions
   */
  //@{  
  /** The number of instruction retire opportunities, before asserting that
   *  something bad happened (like dead or live lock). */
  uint64      m_fwd_progress_cycle;
  
  /// If simics reports an interrupt occurs on this processor it is recorded here
  trap_type_t m_posted_interrupt;
  /** pointer to last good instruction */
  uint32      m_next_exception;
  /** The instruction which caused this exception */
  enum i_opcode m_except_offender;
  /** program counter to fetch at after exception */
  abstract_pc_t m_except_continue_at;
  /** memory address for TLB miss */
  my_addr_t   m_except_access_addr;
  /** type of exception */
  exception_t m_except_type;
  /** cycle penalty of this exception */
  uint32      m_except_penalty;
  //@}

  /**
   * @name Cache of static (decoded) instructions
   */
  //@{
  /// current instruction map of decoded static instructions by physical PC
  ipagemap_t   *m_imap;
  //@}

  /** @name Configuration support: reading / writing state to disk */
  //@{
  confio_t     *m_conf;
  //@}

  /// A file interface object to the trace being read or written
  tracefile_t    *m_tracefp;

  /// A file interface object to a branch trace trace being read or written
  branchfile_t   *m_branch_trace;

  /// A file interface object to a memory trace being read or written
  memtrace_t     *m_memtracefp;

  /**
   * @name  Idealized fields for the CPU
   */
  //@{
  /// number of instructions retired in "ideal mode" by opal
  int64               m_ideal_retire_count;
  /// sequence number of last ideal instruction checked by simics
  int64               m_ideal_last_checked;

  /** sequence number of first instruction that is predictable.
   *  This marks the boundary between "freed" flow_insts and those that are
   *  live.
   */
  int64               m_ideal_last_freed;

  /// sequence number of the first instruction that is predictable
  int64               m_ideal_first_predictable;

  /// sequence number of the last instruction that is predictable
  int64               m_ideal_last_predictable;

  /// machine state (future file) for the in-order functional processor
  mstate_t            m_inorder;

  /// machine state (architected file) for the in-order functional processor
  mstate_t            m_check;

  /// ideal predecessor register
  flow_inst_t        *m_cfg_pred;

  /// The physical register file (touches internal regfile of m_inorder struct)
  physical_file_t    *m_ideal_control_rf;

  /// ideal opstat statistics
  decode_stat_t       m_ideal_opstat;

  /// ideal fetch unit status (possibly stalled on ITLB miss)
  pseq_fetch_status_t m_ideal_status;

  /// control flow graph index: one node for each sequence number
  CFGIndex            m_cfg_index;

  /// memory   dependence chain
  mem_dependence_t   *m_mem_deps;
  //@}

  /**
   * @name Statistics related functions
   */
  //@{
  
  /// opcode statistics
  decode_stat_t      m_opstat;

  /// opal internal squash statistics
  uint64             m_exception_stat[EXCEPT_NUM_EXCEPT_TYPES];
  /// opal internal register use statistics
  uint32           **m_reg_use[RID_NUM_RID_TYPES];

  /// timer to measure how long simulation takes
  utimer_t           m_simulation_timer;
  /// timer to measure how long retirement checking (stepping simics) takes
  utimer_t           m_retirement_timer;

  /// number of times the functional simulator is used to retire instructions
  uint64             m_stat_count_functionalretire;
  /// number of times squashing due to retirement problems
  uint64             m_stat_count_badretire;
  /// number of times squashing due to done/retries
  uint64             m_stat_count_retiresquash;

  // Start and stop pcs
  my_addr_t start_pc ;
  my_addr_t stop_pc ;
#ifdef RESOURCE_RR_REGBUS
  int m_last_result_bus_id ;
#endif // RESOURCE_RR_REGBUS
  
#ifdef RESOURCE_RR_AGEN
  int m_last_agen_id ;
#endif // RESOURCE_RR_AGEN

public:
  /// per cpu stopwatch counting simulated time
  stopwatch_t       *m_overall_timer;
  /// per cpu stopwatch for current thread's simulated time
  stopwatch_t       *m_thread_timer;
  /// histogram of instructions retired by the current thread
  histogram_t       *m_thread_histogram;
  /// count of atomic instructions excluded from the lock structure
  uint64             m_exclude_count;
  /// number of instructions executed by this thread
  uint64             m_thread_count;
  /// number of idle instructions executed by this thread
  uint64             m_thread_count_idle;

  /// # of times each trap occurs (in opal)
  uint64             m_trapstat[TRAP_NUM_TRAP_TYPES];
  /// # of times each trap occurs (in simics)
  uint64             m_simtrapstat[TRAP_NUM_TRAP_TYPES];
  /// asi read-access statistics
  uint64             m_asi_rd_stat[MAX_NUM_ASI];
  /// asi write-access statistics
  uint64             m_asi_wr_stat[MAX_NUM_ASI];
  /// asi atomic-access statistics
  uint64             m_asi_at_stat[MAX_NUM_ASI];

  /// histogram of fetch, decode, dispatch, and retired instructions per cycle
  uint64            *m_hist_fetch_per_cycle;
  uint64            *m_hist_decode_per_cycle;
  uint64            *m_hist_schedule_per_cycle;
  uint64            *m_hist_retire_per_cycle;

  uint64           **m_hist_squash_stage;
  uint64            *m_hist_decode_return;
  
  /// histogram of fetch stalls
  uint64            *m_hist_fetch_stalls;
  /// histogram of retire stalls
  uint64            *m_hist_retire_stalls;
  /// histogram of fastforward lengths
  uint64            *m_hist_ff_length;
  /// histogram of ideal predictor coverage
  uint64            *m_hist_ideal_coverage;

  /// count of times that fetch stalls across branch boundaries
  uint64             m_stat_no_fetch_taken_branch;
  /// count of times that fetch stalls across cache-line boundaries
  uint64             m_stat_no_fetch_across_lines;

  /// count of functional unit utilization
  uint64            *m_stat_fu_utilization;
  /// count of functional unit utilization that retired
  uint64            *m_stat_fu_util_retired;
  
  /// user, kernel, tatal, all three stats are recorded
  static const uint32 TOTAL_INSTR_MODE = 3;
  /// branch predictor statistics
  /// number of times the branch is predicted
  uint64             m_branch_pred_stat[BRANCH_NUM_BRANCH_TYPES][TOTAL_INSTR_MODE];
  /// number of times each type of branch is seen at retire time
  uint64             m_branch_seen_stat[BRANCH_NUM_BRANCH_TYPES][TOTAL_INSTR_MODE];
  /// number of times the branch predictor is correct
  uint64             m_branch_right_stat[BRANCH_NUM_BRANCH_TYPES][TOTAL_INSTR_MODE];
  /// number of times the branch predictor is wrong
  uint64             m_branch_wrong_stat[BRANCH_NUM_BRANCH_TYPES][TOTAL_INSTR_MODE];

  /// number of times a static branch predictor predicts incorrectly
  uint64             m_branch_wrong_static_stat;

  /// number of times the branch hits in the exception table of indirect pred
  uint64             m_branch_except_stat[BRANCH_NUM_BRANCH_TYPES];

  /// number of predictated branches decoded
  uint64             m_pred_count_stat;
  /// number of branches predicting "taken"
  uint64             m_pred_count_taken_stat;
  /// number of branches predicting "not-taken"
  uint64             m_pred_count_nottaken_stat;
  /// number of non-predicated branches decoded
  uint64             m_nonpred_count_stat;

  /// number of predictated branches on a register decoded
  uint64             m_pred_reg_count_stat;
  /// number of branches predicting "taken"
  uint64             m_pred_reg_taken_stat;
  /// number of branches predicting "not-taken"
  uint64             m_pred_reg_nottaken_stat;

  /// number of predictated branches retired
  uint64             m_pred_retire_count_stat;
  /// number of branches predicting "taken"
  uint64             m_pred_retire_count_taken_stat;
  /// number of branches predicting "not-taken"
  uint64             m_pred_retire_count_nottaken_stat;
  /// number of non-predicated branches retired
  uint64             m_nonpred_retire_count_stat;

  /// number of predictated branches on a register retired
  uint64             m_pred_reg_retire_count_stat;
  /// number of branches predicting "taken"
  uint64             m_pred_reg_retire_taken_stat;
  /// number of branches predicting "not-taken"
  uint64             m_pred_reg_retire_nottaken_stat;

  /// register use statistics
  /// count of cycles when a decode stall is due to insufficient registers
  uint64             m_reg_stall_count_stat;
  /// count of number of instructions for decode after a register stall
  uint64             m_decode_stall_count_stat;
  /// count of cycles when a schedule stall due to instruction window full
  uint64             m_iwin_stall_count_stat;
  /// count of number of instructions for decode after a register stall
  uint64             m_schedule_stall_count_stat;
  /// number of times a load requests a bypass value, before it is ready
  uint64             m_stat_early_store_bypass;

  //
  //
  //

  /// number of instructions read from trace
  uint64   m_stat_trace_insn;
  /// total number of instructions committed
  uint64   m_stat_committed;
  /// total number of times squash is called (includes branch mispredicts)
  uint64   m_stat_total_squash;
  /// total number of instructions squashing on commit (only commit squashes)
  uint64   m_stat_commit_squash;
  /// number of times an asi store causes a squash
  uint64   m_stat_count_asistoresquash;
  /// total number of instructions committing successfully
  uint64   m_stat_commit_good;
  /// total number of instructions committing unsuccessfully
  uint64   m_stat_commit_bad;
  /// total number of unimplemented instructions committing
  uint64   m_stat_commit_unimplemented;
  /// total number of exceptions
  uint64   m_stat_count_except;

  /// total number of loads retired
  uint64   m_stat_loads_retired;
  /// total number of stores retired
  uint64   m_stat_stores_retired;
  /// total number of atomics retired
  uint64   m_stat_atomics_retired;
  /// total number of prefetches retired
  uint64   m_stat_prefetches_retired;
  /// total number of control insts committed
  uint64   m_stat_control_retired;

  /// total number of instructions fetched
  uint64   m_stat_fetched;
  /// total number of instructions that miss in mini-itlb
  uint64   m_stat_mini_itlb_misses;
  /// total number of instructions decoded
  uint64   m_stat_decoded;
  /// total number of instructions executed
  uint64   m_stat_total_insts;

  /// total number of loads executed
  uint64   m_stat_loads_exec;
  /// total number of stores executed
  uint64   m_stat_stores_exec;
  /// total number of atomics executed
  uint64   m_stat_atomics_exec;
  /// total number of prefetches executed
  uint64   m_stat_prefetches_exec;
  /// total number of control insts executed
  uint64   m_stat_control_exec;

  /// loads which are found in the trace
  uint64   m_stat_loads_found;
  /// loads which are not found in the trace
  uint64   m_stat_loads_notfound;
  /// total number of spill traps
  uint64   m_stat_spill;
  /// total number of fill traps
  uint64   m_stat_fill;

  /* cache stats */
  /// number fetches which miss in icache
  uint64   m_stat_num_icache_miss;
  /// number of fetches that hit in the mhsr (after missing in i-cache)
  uint64   m_stat_icache_mshr_hits;
  /// number instructions which miss in dcache
  uint64   m_stat_num_dcache_miss;
  /// number of instructions that retire (having missed in the dcache)
  uint64   m_stat_retired_dcache_miss;
  /// number of instructions with long latency misses
  uint64   m_stat_retired_memory_miss;
  /// number of instructions that hit in the mshr structure
  uint64   m_stat_retired_mshr_hits;
  /// number of load/store instructions to I/O space
  uint64   m_stat_count_io_access;
  /// number of times the cache stalls when trying to issue a memory request
  uint64   m_num_cache_not_ready;

  /// number of main memory misses currently outstanding
  uint64   m_stat_miss_count;
  /// the last miss to main memory's sequence number
  uint64   m_stat_last_miss_seq;
  /// the last miss to main memory's fetch time
  tick_t   m_stat_last_miss_fetch;
  /// the last miss to main memory's issue time
  tick_t   m_stat_last_miss_issue;
  /// the last miss to main memory's retire time
  tick_t   m_stat_last_miss_retire;
  
  /// count of instructions that are effective (independent or not)
  uint64   m_stat_miss_effective_ind;
  uint64   m_stat_miss_effective_dep;
  /// count of instructions that are inter-cluster instructions
  uint64   m_stat_miss_inter_cluster;

  /// histogram of effective, independent instructions
  histogram_t *m_stat_hist_effective_ind;
  /// histogram of effective, dependent instructions
  histogram_t *m_stat_hist_effective_dep;
  /// histogram of cache inter-arrival times (# of instructions, not cycles)
  histogram_t *m_stat_hist_inter_cluster;
  /// histogram of counts of outstanding cache misses
  histogram_t *m_stat_hist_misses;
  /// histogram of cache miss "inter-arrival" cycles
  histogram_t *m_stat_hist_interarrival;
  /// histogram of dependent memory operations
  histogram_t *m_stat_hist_dep_ops;

  /* lsq stats */
  /// number of loads satisfied by store queue
  uint64   m_stat_bypasses;
  /// number of stores scheduled before value
  uint64   m_stat_num_early_stores;
  /// number of loads waiting for early store resolution
  uint64   m_stat_num_early_store_bypasses;
  /// number of atomics scheduled before value
  uint64   m_stat_num_early_atomics;

  /// number of stale data predictions made
  uint64   m_stat_stale_predictions;
  /// number of stale data predictions successful
  uint64   m_stat_stale_success;
  /// a histogram of successful stale data speculations
  uint64  *m_stat_stale_histogram;

  /// number of times simics is stepped
  uint64   m_stat_continue_calls;
  /// number of instructions overwritten
  uint64   m_stat_modified_instructions;

  /// The number of times the ideal processor was not able to reach the end of the window
  uint64   m_inorder_partial_success;
  //@}
  // Boolean added to make sure simics can crash...
  // for the first call to checkAllState(), we should do nothing

  // LXL: statistics for traps
  uint64   m_stat_exception_count;
  uint64   m_stat_trap_count;
  uint64   m_stat_fatal_trap_count;
  bool     seen_fatal_trap;
  trap_info first_fatal_trap;

  bool     cur_priv_mode;
  bool     patched_pc;
  uint64   patched_seq_num;

  int      way_counter;
  int      fway_counter;
  int      agen_counter;

  bool fault_cause_fatal;
  

  bool recovered;
  int  recover_peek;
  bool detected_again;

  uint64 last_rollback_time;
  uint64 time_from_rollback;
  int steady_state;

  int print_trace;

  uint64 squash_inst_no;
#ifdef SW_ROLLBACK
  operation_mode_t m_op_mode;
#endif

  int mem_trace_cnt;
  vector<mem_rec_t> m_mem_trace;
  void addMemRec(uint64 pc, bool isLoad, uint64 address, uint64 data);

  tlb_data_t *tlb_data[NUM_CHKPTS];
  int getCurrChkptNum();
  void takeChkpt(dynamic_inst_t *instruction) ;
  void printArchState();
  void checkpointTLB();
  void recoverTLB();
  void printTLB();
  void updateInstrInfo(instruction_information_t &instr_info, dynamic_inst_t *d);


  void simicsDetection() ;
  void symptomDetected(int);
  void oldSymptomDetected(int);
  void clearAllFaults();
  bool m_trigger_recovery;
  bool do_not_advance;

  unsigned int *recovery_instr_mem;
  unsigned int recovery_instr_mem_size;
  unsigned int  recovery_instr_ptr;

  void validateRecovery();
  void recoveryModuleExec();
  void startCheckpointing();
  int getChkptInterval() ;
  void recoveryModule();
  void takeProcessorCheckpoint();
  void prepareRollback();
  void postRollback();
  void checkpointTLBAfterLogging();
	
  multicore_diagnosis_t * getMulticoreDiagnosis() { return m_multicore_diagnosis;}

  diagnosis_t* m_diagnosis;
  multicore_diagnosis_t* m_multicore_diagnosis;

  trap_type_t m_last_symptom;

#ifdef MEASURE_FP
  int FP_util[3][4];
#endif

#ifdef DETECT_INF_LOOP
  uint64     loop_info[LOOP_HASH_SIZE][VALUE_START+2];
#endif

  int      curr_trap_rate ;

  fault_stats *fault_stat;
  int hw_hang_cnt;
  bool trans_fault_injected;

  char m_tlbfilename[256];

  llb_t *llb;
  func_inst_buffer_t func_inst_buffer;
  bool m_report_interrupt_to_diagnosis;
  bool squash_next_instr;

  bool done_squash_trans;
 


  // Functions to track enter/exit of interrupt to disable high OS
  bool in_interrupt ;
  int interrupt_priv ;

  void takingInterrupt(int p) { in_interrupt = true ; interrupt_priv = p ; }
  void exitInterrupt() { in_interrupt = false ; interrupt_priv = -1 ; }
  bool isHandlingInterrupt() { return in_interrupt ; }
  int getInterruptPriv() { return interrupt_priv ; } 

  bool m_start_logging;
  vector<mem_event_t> mem_event_log;
  void logMemEvent(uint64 addr, int access_size, ireg_t * data);
  void printMemEvents();

#ifdef DETECT_INV_VIOLATION

  /****** Variable Declarations for Invariant detection ********/
  bool allow_inv_violation_msg;
  bool invariant_faulty_run ;
  set<int64> invariant_falpos_ids ;
  int64 invariant_failed_id ;
  uint64 invariant_last_seq_number ;
  uint64 invariant_org_fault_injection_pt;

  uint64 app_inst;
  uint64 last_app_inst;
  uint64 no_of_inst_to_simulate;

  bool readInvariantsFlag;
  InvMap aggressiveInvMap, conservativeInvMap;  
  MulRangeInvMap conservativeMulRangeInvMap; 
  SVAAddrMap svaAddrMap;
  std::ifstream *aggressiveInvInpFile, *conservativeInvInpFile, *conservativeMulRangeInvInpFile;
  std::ifstream *SVAAddrFile;
  void readPidAAddr(void);
  void readSVAAddr(void);
  void readConservMulRangeInvariants(void);
  void readMulRangeInvariants(MulRangeInvMap &iMap, std::ifstream *); 
  void printMulRangeInvariants(MulRangeInvMap &iMap);
  void readConservInvariants(void);
  void readAggrInvariants(void);
  void readInvariants(InvMap &iMap, std::ifstream *); 
  void printInvariants(InvMap &iMap);

  /****** Variables for invariant/SDC analysis ********/
  uint64 total_num_SVA_access;
  uint64 total_num_SVA_access_faulty; 
  uint64 faulty_store_value_num;
  uint64 faulty_store_addr_num; 
  bool allow_arch_state_mismatch_msg;
  /*************************************************************/

#endif //DETECT_INV_VIOLATION

	ireg_t getSimicsNPC() ;

#ifdef RESOURCE_RR_REGBUS
	int getNextBusId() {
		m_last_result_bus_id = (m_last_result_bus_id+1) % MAX_FETCH ;
		return m_last_result_bus_id ;
	}
#endif // RESOURCE_RR_REGBUS
#ifdef RESOURCE_RR_AGEN
	int getNextAgenId() {
		m_last_agen_id = (m_last_agen_id+1) % MAX_FETCH ;
		return m_last_agen_id ;
	}
#endif // RESOURCE_RR_AGEN

	void readInstRangesFile()  { fault_stat->readInstRangesFile() ; }
	void readDetectorList()    { fault_stat->readDetectorList() ; }
	void printDetectorRanges() { fault_stat->printDetectorRanges() ; }
	void readDetectorRanges()  { fault_stat->readDetectorRanges() ; }

	void readDataOnlyValuesList() { fault_stat->readDataOnlyValuesList() ; }

	struct uint64_cmp {
		bool operator() (uint64 A, uint64 B) {
			return A < B ;
		}
	} ;

#ifdef FIND_UNIQUE_PCS
	typedef map<uint64, int, uint64_cmp> retire_pcs_t ;
	retire_pcs_t retire_pcs ;
	void addRetirePC(la_t pc) ;
	void printRetirePCs() ;
#endif // FIND_UNIQUE_PCS


#ifdef BUILD_SLICE
#define SLICE_FAULTS_TILL 1000000
#define MAX_PROPAGATION_LATENCY 10000000000 // Set to 10B so that propagation continues forever
#define INVALID_REG 65535

	typedef vector<half_t> reg_list_t ;
	typedef vector<la_t> addr_list_t ;
	typedef vector<uint64> number_list_t ;

	struct half_cmp {
		bool operator() (half_t A, half_t B) {
			return A < B ;
		}
	} ;
	typedef map<half_t, uint64, half_cmp> producer_t ;
	// static producer_t last_producer ;
	typedef map<uint64, half_t, uint64_cmp> deleted_instructions_t ;
	deleted_instructions_t deleted_instructions ;

	struct slice_inst {
		uint64 seq ;
		la_t pc ;
		reg_list_t src_regs ;
		// uint64 src_producer[SI_MAX_SOURCE] ;
		uint64 src_producer_fault[SI_MAX_SOURCE] ;
		reg_list_t dest_regs ;
		la_t address ;
		dyn_execute_type_t type ;
		half_t opcode ;
		uint64 addr_producer_fault ;
		
		slice_inst(dynamic_inst_t *d) {
			seq = d->getSequenceNumber() ;
			pc = d->getVPC() ;

			src_regs.clear() ;
			for(int i=0; i<SI_MAX_SOURCE; i++) {
				reg_id_t &reg = d->getSourceReg(i) ;
				if(reg.isZero()) {
					src_regs.push_back(INVALID_REG) ;
					// src_producer[i] = 0 ;
					src_producer_fault[i] = 0 ;
				} else {
					half_t src = reg.getFlatRegister() ;
					src_regs.push_back(src) ;
					// uint64 inst = 0  ;
					// if(last_producer.find(src)!=last_producer.end()) {
					// 	inst = last_producer[src] ;
					// }
					// // DEBUG_OUT("[%lld] produces r%d\n", inst, src) ;
					// src_producer[i] = inst ;
					uint64 fault_id = 0  ;
					if(last_reg_fault.find(src)!=last_reg_fault.end()) {
						fault_id = last_reg_fault[src] ;
					}
					src_producer_fault[i] = fault_id ;
				}
			}
			dest_regs.clear() ;
			for(int i=0; i<SI_MAX_DEST; i++) {
				reg_id_t &reg = d->getDestReg(i) ;
				if(reg.isZero()) {
					dest_regs.push_back(INVALID_REG) ;
				} else {
					half_t dest = reg.getFlatRegister() ;
					dest_regs.push_back(dest) ;
				}
			}
			type = d->getStaticInst()->getType() ;
			opcode = d->getStaticInst()->getOpcode() ;
			address = 0x0 ;
			addr_producer_fault = 0 ;
			memory_inst_t *mem = dynamic_cast<memory_inst_t*>(d) ;
			if(mem) {
				address = mem->getAddress() ;
				if(last_addr_fault.find(address)!=last_addr_fault.end()) {
					addr_producer_fault = last_addr_fault[address] ;
				}
			}
			// DEBUG_OUT("Recording %d's last_addr_fault[0x%llx] = %d\n", seq, address, addr_producer_fault) ;
		}

		~slice_inst() {
			src_regs.clear() ;
			dest_regs.clear() ;
	   	}

		void print(char *s) {
			if(strcmp(s,"")!=0) {
				DEBUG_OUT("%s:",s) ;
			}
			DEBUG_OUT("[%d] [0x%llx] - %s <- ",
					seq, pc, decode_opcode( (enum i_opcode) opcode)) ;
			int pos = -1 ;
			for(reg_list_t::iterator I=src_regs.begin(), E=src_regs.end() ;
					I!=E; I++) {
				pos ++ ;
				half_t reg = *I ;
				if(reg != INVALID_REG) {
					// DEBUG_OUT("r%d(p%lld, i%d),", reg, src_producer[pos],pos) ;
					DEBUG_OUT("r%d,", reg) ;
					// DEBUG_OUT("r%d,", reg) ;
				}
			}
			DEBUG_OUT("-> ") ;
			for(reg_list_t::iterator I=dest_regs.begin(), E=dest_regs.end() ;
					I!=E; I++) {
				half_t reg = *I ;
				if(reg != INVALID_REG) {
					DEBUG_OUT("r%d,", reg) ;
				}
			}
			if(address != 0x0) {
				DEBUG_OUT(" VA=0x%llx", address) ;
			}
			DEBUG_OUT("\n") ;
		}
	} ;
	typedef struct slice_inst slice_inst_t ;
		
	static uint64 pseq_fault_id ;
	uint64 stepFaultId() { pseq_fault_id++ ; return pseq_fault_id ;} 

	struct fault {
		uint64 fault_id ;
		slice_inst *inst ;
		tick_t cycle ;
		int reg_valid ;
		half_t reg ;
		half_t phys_reg ;
		int reg_type ;
		la_t address ;
		int del_mark ;

		fault(dynamic_inst_t *d, half_t r, half_t phys, int type) {
			fault_id = ++pseq_fault_id ;
			inst = new slice_inst(d) ;
			cycle = 0 ;
			reg = r ;
			if(reg==INVALID_REG) {
				reg_valid = 0 ;
				phys_reg = 0 ;
				reg_type = -1 ;
			} else {
				reg_valid = 1 ;
				phys_reg = phys ;
				reg_type = type ;
			}
			address = inst->address ;
			del_mark = 0 ;
		}

		~fault() {
			delete inst ;
		}

		uint64 getFaultId() { return fault_id; }

		void setCycle(tick_t c) { cycle = c ; }

		void print(char *s) {
			if(strcmp(s,"")!=0) {
				DEBUG_OUT("%s:",s) ;
			}
			// DEBUG_OUT("%d:: ", fault_id) ;
			if(reg_valid) {
				DEBUG_OUT("r%d @%lld ", reg, cycle) ;
			} else {
				DEBUG_OUT("0x%llx ", address) ;
			}
			inst->print("in") ;
		}

		void markToDel() { del_mark = 1 ; } 
		bool isMarked() { return (del_mark==1) ; }
	} ;
	typedef struct fault fault_t;
	struct fault_cmp {
		bool operator() (fault_t *A, fault_t *B) {
			return A->fault_id < B->fault_id ;
		}
	} ;
	typedef map<uint64,fault_t*,uint64_cmp> fault_list_t ;
	fault_list_t fault_list ;

	typedef set<uint64,uint64_cmp> deleted_faults_t ;
	deleted_faults_t deleted_faults ;


	typedef map<half_t, uint64, half_cmp> last_reg_fault_t ;
	static last_reg_fault_t last_reg_fault ;
	typedef map<uint64, uint64, uint64_cmp> last_addr_fault_t ;
	static last_addr_fault_t last_addr_fault ;

	unsigned long all_reg_faults ;
	reg_list_t reg_list ;
	addr_list_t addr_list ;
	reg_list_t unread_regs ;
	addr_list_t unread_addr ;

	void slice(dynamic_inst_t *inst) ;

	// Find instrutions who's dest regs are dynamically dead.
	// Faults in these instructions don't affect outputs.
	bool findDeadInstructions(dynamic_inst_t *inst) ;
	bool findDeadInstructions(slice_inst_t *inst) ;

	// Recursively mark the producers of the sources of this instruction
	// that are currently only the slice
	bool markProducers(dynamic_inst_t* inst) ;
	bool markProducers(slice_inst_t* inst) ;
	bool markDataProducers(slice_inst_t *inst) ;

	/**********************************
	** XXX Deprecated functions
	** void markToDel(fault_t *fault) ;
	** void deleteMarkedFromSlice() ;

	** // for a given instruction, add its sources to backward slice.
	** // Remember that for stores, s2 is used for data. So, only when
	** // memory tracking identifies that they are dep, add those regs
	** bool addRegsToSlice(slice_inst_t *inst) ;
	** bool addRegsToSlice(fault_t *fault) ;
	** bool addAddrToSlice(fault_t *fault) ;
	** bool addDataRegsToSlice(fault_t *fault) ;
	***********************************/

	void printDataSlice() ;

	// Techniques to reduce the set of faults to inject
	typedef set<fault_t*, fault_cmp> fault_set_t ;
	fault_set_t reduced_fault_set ;

	typedef vector<uint64> uint64_list_t ;
	typedef set<uint64, uint64_cmp> uint64_set_t ;
	typedef map<uint64, uint64_list_t, uint64_cmp> seq_num_set_t ;
	seq_num_set_t reduced_fault_set_ids ;
	void addToReducedFaultSet(fault_t *fault) ;

#ifdef BUILD_DDG
	// Methods for building and keeping track of the Data Flow Graph
	ddg_node_t *ddg_root ;
	typedef map<uint64, ddg_node_t*, uint64_cmp> ddg_node_map_t ;
	ddg_node_map_t ddg_node_map ;

	struct ddg_a_stats {
		uint64 a_fanout ;
		uint64 a_size ;
		uint64 a_lifetime ;

		ddg_a_stats() {
			a_fanout = 0 ;
			a_size = 0 ;
			a_lifetime = 0 ;
		}

		ddg_a_stats(uint64 f, uint64 s, uint64 l) {
			a_fanout = f ;
			a_size = s ;
			a_lifetime = l ;
		}
		~ddg_a_stats() { }
	} ;
	typedef struct ddg_a_stats ddg_a_stats_t ;
	typedef map<uint64, ddg_a_stats_t*, uint64_cmp> ddg_stats_map_t ;
	ddg_stats_map_t ddg_stats_map ;

	typedef vector<ddg_node_t*> ddg_node_list_t ;
	struct addr_cmp {
		bool operator() (la_t A, la_t B) {
			return A<B ;
		}
	} ;
	typedef map<la_t, ddg_node_t*, addr_cmp> last_store_t ;
	static last_store_t ddg_last_store ;
	static producer_t ddg_last_producer ; // Separate as I am tracking DDG for entire exec

	void printDDG() ;
	void printReverseDDG() ;
	void updateDDGStats(uint64 pc, uint64_list_t &L) ;
	void addSubTreeNodes(ddg_node_t *N, uint64_set_t &subtree_nodes) ;
#endif // BUILD_DDG

#endif // BUILD_SLICE
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/


#define NO_FAULT_INJECTION \
	clearDoFaultInjection 

#define DO_FAULT_INJECTION \
	setDoFaultInjection 

#define GET_FAULT_INJECTION \
	getDoFaultInjection 

#define GET_FAULT_TYPE \
	getFaultType

//no body is using this function
#define GET_FAULT_BIT \
	getFaultBit

//this is done
#define GET_FAULT_STUCKAT \
	getFaultStuckat


//probles with this func at iwindow and other places
//it seems that this function hs to be taken care
#define GET_FAULTY_REG \
	getFaultyReg

#define INJECT_FAULT \
	injectFault

#define FAULT_CLOCK \
	setClock

#define FAULT_RET_INST\
	setRetInst

#define GET_RET_INST\
	getRetInst

#define FAULT_RET_PRIV\
	setRetPriv

#define GET_RET_PRIV\
	getRetPriv

#define FAULT_CURR_INST \
	setCurrInst

#define FAULT_SET_PRIV \
	setPriv

#define GET_PRIV \
	getPriv

#define FAULT_IN_FUNC_TRAP \
	inFuncTrap

#define FAULT_CLEAR_FUNC_TRAP \
	clearFuncTrap

#define FAULT_SET_NON_RET_TRAP \
	setNonRetTrap

#define ARCH_STATE \
	setArchCycle

#define ARCH_INUSE_STATE \
	setArchInuseCycle

#define MEM_STATE\
	setMemCycle

#define INC_TOTAL_INJ \
	incTotalInj

#define INC_TOTAL_MASK \
	incTotalMask

#define INC_TOTAL_TRAPS \
	incTotalTraps

#define INC_FATAL_TRAPS \
	incFatalTraps

#define INC_READ_INJ \
	incReadInj

#define INC_READ_MASK \
	incReadMask

#define GET_INF_LOOP_START \
	getInfLoopCycle

#define TRACK_CYCLE \
    trackCycle

#define GET_CLOCK \
    getClock

#define INC_BRANCH \
    incBranches

#define INC_STORE \
    incStores


// dont know if i should keep this here
#define OUT_SYMPTOM_INFO \
    outSymptomInfo


//so far good...
#define SET_FAULT_INJ_INST \
    setFaultInjInst

#endif  /* _PSEQ_H_ */
