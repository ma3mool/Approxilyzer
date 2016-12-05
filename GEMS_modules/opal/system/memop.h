
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
#ifndef _MEMOP_H_
#define _MEMOP_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "dynamic.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// partial overlap: constant important (must be > 0)
const uint16  MEMOP_OVERLAP  =  1;
/// exact match: can bypass: constant important (must be > 0)
const uint16  MEMOP_EXACT    =  2;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* An "in-flight" memory instruction in the processor model.
*
* @see     dynamic_inst_t, waiter_t
* @author  zilles
* @version $Id: memop.h 1.56 06/02/13 18:49:19-06:00 mikem@maya.cs.wisc.edu $
*/
//**************************************************************************
class memory_inst_t : public dynamic_inst_t {
public:
  /** @name Constructor(s) / Destructor */
  //@{
  /** Constructor: uses default constructor */
  memory_inst_t( static_inst_t *s_inst, 
                 int32 window_index,
                 pseq_t *pseq,
                 abstract_pc_t *at, fault_stats *fault_stat );

  /** Destructor: uses default destructor. */
  virtual ~memory_inst_t();
  //@}

  /** squash an instruction in the pipe, after it has been renamed.
   *  As it rolls back the state it is implemented differently by control,
   *  exec, mem instrs. */
  void Squash();
  /** retire: change the machines in-order architected state according
   *  to an instruction.
   *  (implemented differently by control, exec, mem instrs).
   *  @param a   The next in-order pc, when this instruction is retired.
   */
  void Retire( abstract_pc_t *a );

  /** do the generic execution stuff for memory operations */
  void Execute();

  /** complete a memory transaction which has missed in the cache: */
  virtual void Complete( void ) = 0;

  /** This virtual function encapsulates the cache.
   */
  virtual bool accessCache( void ) = 0;

  /** Translate the ops register accesses to a logical, then physical
   *  address. Returns true if successful, false if not.  */
  trap_type_t  addressGenerate( OpalMemop_t accessType );
  void checkAddressAlignment( ) ;
  bool oldAddressAligned() {
      if( m_old_physical_addr & (m_access_size-1) != 0 ) {
    	  return false ;
      } else {
    	  return true ;
      }
  }
  /// returns the ASI for this instruction
  uint16       getASI( void ) {
    ASSERT( m_asi != (uint16) -1 );
    return (m_asi);
  }

  /// returns true if this ASI is cacheable
	  bool         isCacheable( void );
/*  { */
/* 	  if ( m_asi == (uint16) -1 || getPSEQ()->isRecovering()) { */
/*       return false; */
/*     } */
/*     return ( pstate_t::is_cacheable( m_asi ) ); */
/*   } */
  
  /** access the mmu (SFMMU currently), using the m_pseq's mmu interface */
  trap_type_t  mmuRegisterAccess( bool isaWrite );
  
  /** check if this memory instructions access overlaps a different one
   *  @return  0 if no overlap, MEMOP_OVERLAP, MEMOP_EXACT if the
   *  memory instructions are exactly. */
  uint16      addressOverlap( memory_inst_t *other );
  /** returns the virtual address for this load/store/stomic instruction */
  my_addr_t   getAddress( void ) const { return m_address; }
  /** returns if the m_data in this memop is valid:
   */
  bool        isDataValid( void ) const {
    return (m_data_valid);
  }
  
  // isIOAccess( physical_addr )
  static bool isIOAccess( pa_t phys_addr ) {
    ASSERT( phys_addr != (pa_t) -1 );
    return ( (phys_addr & 0x0000040000000000ULL) != 0 );
  }
  
  /// return a pointer to the data value loaded
  ireg_t     *getData( void ) {
	
    return ( &m_data_storage[0] );
  }
  
  ireg_t     *getOldData( void ) {
    return ( &m_old_data_storage[0] );
  }

  /// return the data (masked by the size of the load or store)
  ireg_t      getUnsignedData( void ) {
    if (m_access_size == 4) {
      return (m_data_storage[0] & MEM_WORD_MASK);
    } else if (m_access_size == 8) {
      return m_data_storage[0];
    } else if (m_access_size == 2) {
      return (m_data_storage[0] & MEM_HALF_MASK);
    } else if (m_access_size == 1) {
      return (m_data_storage[0] & MEM_BYTE_MASK);
    } else {
      ERROR_OUT("error: unable to mask access size: %d\n", m_access_size);
      return 0ULL;
    }      
  }
  
  ireg_t	getMaskedData(ireg_t *A) {
    if (m_access_size == 4) {
      return (A[0] & MEM_WORD_MASK);
    } else if (m_access_size == 8) {
      return A[0];
    } else if (m_access_size == 2) {
      return (A[0] & MEM_HALF_MASK);
    } else if (m_access_size == 1) {
      return (A[0] & MEM_BYTE_MASK);
    } else {
      ERROR_OUT("error: unable to mask access size: %d\n", m_access_size);
      return 0ULL;
    }      
  }


  /// return the sign extended data (masked by the size of the load or store)
  ireg_t      getSignedData( void ) {
    // This magic statement "m_access_size*8-1" selects the bit to do the
    // sign extension on (e.g. 63, 15, or 7).
    if (m_access_size == 4) {
      return sign_ext64(m_data_storage[0] & MEM_WORD_MASK, m_access_size*8-1);
    } else if (m_access_size == 8) {
      return m_data_storage[0];
    } else if (m_access_size == 2) {
      return sign_ext64(m_data_storage[0] & MEM_HALF_MASK, m_access_size*8-1);
    } else if (m_access_size == 1) {
      return sign_ext64(m_data_storage[0] & MEM_BYTE_MASK, m_access_size*8-1);
    } else {
      DEBUG_OUT("error: unable to mask access size: %d\n", m_access_size);
      return NULL;
    }      
  }

  /// returns the physical address
  my_addr_t getPhysicalAddress(void) const {
    return m_physical_addr;
  }

  /// returns the physical address
  my_addr_t* getPhysicalAddressAddress(void) {
    return &m_physical_addr;
  }

  /// sets the physical address
  void setPhysicalAddress(my_addr_t addr ) {
    m_physical_addr = addr ;
  }

  my_addr_t getOldPhysicalAddress() { return m_old_physical_addr; } 
  void setOldPhysicalAddress(my_addr_t addr ) { m_old_physical_addr = addr ; }

  /// returns the line address (shifted over by L1_BLOCK_BITS)
  my_addr_t getLineAddress(void) {
    if (m_physical_addr == (pa_t) -1) {
      DEBUG_OUT("error: getLineAddress(): invalid physical address index:%d.\n",
                getWindowIndex());
      printDetail();
      ASSERT(0);
    }
    return (m_physical_addr >> DL1_BLOCK_BITS);
  }

  /// returns the line address (shifted over by L1_BLOCK_BITS)
  my_addr_t getOldLineAddress(void) {
    if (m_old_physical_addr == (pa_t) -1) {
      DEBUG_OUT("error: getLineAddress(): invalid physical address index:%d.\n",
                getWindowIndex());
      printDetail();
      ASSERT(0);
    }
    return (m_old_physical_addr >> DL1_BLOCK_BITS);
  }

  /// returns the access size of the load or store
  byte_t    getAccessSize() const {
    return m_access_size;
  }

  /// print out detailed debugging information about this load or store
  void      printDetail( void );

  /// Functions to access the has_mem_fault
	  void      setMemFault() { has_mem_fault = true ; }
  void      clearMemFault() { has_mem_fault = false ; }
  bool      getMemFault() { return has_mem_fault ; }
  bool isEqual(ireg_t *a, ireg_t *b);
  
  /// Functions to access the should_inject_fault
  void      setInjectFault() { should_inject_fault = true ; }
  bool      getInjectFault() { return should_inject_fault; }

  bool hasPhysicalAddress() { return (m_physical_addr != -1); }

protected:  
  /** sets the access size of a load or store.
   *  warning: you should not use this ever to set a size larger than
   *           MEMOP_MAX_SIZE*8
   */
  void      setAccessSize( byte_t new_size )  {
    m_access_size = new_size;
    if ( m_access_size > MEMOP_MAX_SIZE*8 ) {
      ERROR_OUT("error: access size is greater than defined maximum.\n");
      ERROR_OUT("     : max size=%d  access size=%d\n",
                MEMOP_MAX_SIZE*8, m_access_size );
	  SIM_HALT;
    }
  }

  /** sets the asi based on control registers */
  void      setASI( void );

  /** data location for memory accesses */
  ireg_t     m_data_storage[MEMOP_MAX_SIZE];

  /** The virtual address of the memory being accessed by this instruction. */
  my_addr_t  m_address;

  /** The physical location in memory to access. */
  pa_t       m_physical_addr;

  /** the pstate of this instruction at issue */
  uint16     m_pstate;
  /** the tl of this instruction at issue */
  uint16     m_tl;
  /** the address space identifier for this instruction */
  uint16     m_asi;
  /** true if this instructions 'm_data_storage' field is valid */
  bool       m_data_valid;

  /** Address/data line corrupted. used for mem_state */
  bool       has_mem_fault;

  /** Should we inject into this mem insts for lsq fault? */
  bool       should_inject_fault ;

  ireg_t     m_old_data_storage[MEMOP_MAX_SIZE];
  my_addr_t  m_old_address;
  pa_t       m_old_physical_addr;
  bool       m_old_data_valid;
  bool m_check_at_retire;
  uint64 m_check_at_retire_value;

public:
  /** Load/Store queue fields: prev entry */
  memory_inst_t *lsq_prev;
  /**  Load/Store queue fields: next entry */
  memory_inst_t *lsq_next;
  /** The size of the memory access.
   *  The access size should really only depend on the static instruction.
   *  However, thanks to 64-byte ld/st ASIs, it does not. And since the ASI
   *  may not be determined at fetch, or even at decode, you may change this
   *  access size _late_ in the game (in the setASI() function in memop.C).
   */
  byte_t         m_access_size;

  // ireg_t     m_old_data_storage[MEMOP_MAX_SIZE];
  // my_addr_t  m_old_address;
  // pa_t       m_old_physical_addr;
  // bool       m_old_data_valid;
};

/**
* A virtualized interface for loads used by the load/store queue.
*
* @author  cmauer
* @version $Id: memop.h 1.56 06/02/13 18:49:19-06:00 mikem@maya.cs.wisc.edu $
*/
//**************************************************************************
class load_interface_t {
public:
  /** Raise an exception due to data misspeculation:
   *    the load/store queue forwarded an invalid data item.
   *    (this is called by the offending store instruction on the load. */
  virtual void replay(void) = 0;
  /** read the for this load from the load/store queue: 
   *      a store instruction in the lsq exactly matches this load */
  virtual void lsqBypass(void) = 0;
  /** When the store data is not ready for this load, we wait for it
   *      to become ready */
  virtual void lsqWait(void) = 0;
  /** puts this instr on a Store's wait list due to overlapping addresses */
  virtual void          addrOverlapWait(store_inst_t * depend) = 0;

  /** get the store which this load depends on
   *      The value from this store will be forwarded by lsqbypass() */
  virtual store_inst_t *getDepend( void ) const = 0;
  /** sets the store that this load depends on. */
  virtual void          setDepend( store_inst_t *depend ) = 0;

};

/**
* A load instruction in the processor model.
*
* @see     memory_inst_t, dynamic_inst_t, waiter_t
* @author  zilles
* @version $Id: memop.h 1.56 06/02/13 18:49:19-06:00 mikem@maya.cs.wisc.edu $
*/
//**************************************************************************
class load_inst_t : public memory_inst_t, public load_interface_t {

public:
  /** @name Constructor(s) / Destructor */
  //@{
  /** Constructor: uses default constructor */
  load_inst_t( static_inst_t *s_inst, 
               int32 window_index,
               pseq_t *pseq,
               abstract_pc_t *fetch_at , fault_stats *fault_stat );

  /** Destructor: uses default destructor. */
  virtual ~load_inst_t();

  /** Allocates object throuh allocator interface */
  void *operator new( size_t size ) {
    return ( m_myalloc.memalloc( size ) );
  }
  
  /** frees object through allocator interface */
  void operator delete( void *obj ) {
    m_myalloc.memfree( obj );
  }
  //@}

  /** execute: modify the machine's state by executing this instruction
   *  implemented differently by control, exec, mem instrs */
  void          Execute();
  /** execute (2nd half): called after no LSQ conflicts are detected
   * Used to finish execution after being put on wait list*/
  void          continueExecution();
  /** @name lsq interfaces */
  //@{
  /** Raise an exception due to data misspeculation:
   *    the load/store queue forwarded an invalid data item.
   *    (this is called by the offending store instruction on the load. */
  void          replay( void );
  /** read the for this load from the load/store queue: 
   *      a store instruction in the lsq exactly matches this load */
  void          lsqBypass( void );
  /** When the store data is not ready for this load, we wait for it
   *      to become ready */
  void          lsqWait( void );
  /** get the store which this load depends on
   *      The value from this store will be forwarded by lsqbypass() */
  store_inst_t *getDepend( void ) const { return m_depend; }
  /** sets the store that this load depends on. */
  void          setDepend( store_inst_t *depend ) {
    m_depend = depend;
  }
  /** puts this load on a Store's wait list due to overlapping addresses */
  void          addrOverlapWait(store_inst_t * depend); 
  //@}

  /** @name dynamic.C interfaces */
  //@{
  /// issue this memory transaction to the cache
  bool accessCache( void );
  /// retires an instruction
  void Retire( abstract_pc_t *a );

  bool matchesLoadBuffer();
  bool collectLLB();
  void collectLoadSources();
  void collectCompareLog() {}
  bool setLLBReadPointer(int);
  /// complete the execution of a load which misses in the cache
  void Complete();
  //@}

  /** @name memory allocation:: */
  //@{
  static listalloc_t m_myalloc;
  //@}

  /// print out detailed debugging information about this load or store
  void          printDetail( void );
#ifdef HARD_BOUND
  void propagateObjects(int in_trap=0) ;
#endif //HARD_BOUND

	bool updateOnly() { return update_only;}

private:
  /** compare the predicted value to the actual value
   *  @return bool  True if the values match, false if not. */
  bool          validateValuePrediction( void );

  /** true if this load made a value speculation */
  bool          m_value_pred;

  /** the predicted value for this load instruction, if any */
  ireg_t        m_predicted_storage[MEMOP_MAX_SIZE];

  /** pointer to the store which fed this load */
  store_inst_t *m_depend;

  /** next program counter: used for replay traps to correct fetch */
  abstract_pc_t m_fetch_at;

	bool update_only;

	int llb_read_pointer;
};

/**
* A store instruction in the processor model.
*
* @see     memory_inst_t, dynamic_inst_t, waiter_t
* @author  zilles
* @version $Id: memop.h 1.56 06/02/13 18:49:19-06:00 mikem@maya.cs.wisc.edu $
*/
//**************************************************************************
class store_inst_t : public memory_inst_t {

public:
  /** @name Constructor(s) / Destructor */
  //@{
  /** Constructor: uses default constructor */
  store_inst_t( static_inst_t *s_inst, 
                int32 window_index,
                pseq_t *pseq,
                abstract_pc_t *at , fault_stats *fault_stat );

  /** Destructor: uses default destructor. */
  virtual ~store_inst_t();

  /** Allocates object throuh allocator interface */
  void *operator new( size_t size ) {
    return ( m_myalloc.memalloc( size ) );
  }
  
  /** frees object through allocator interface */
  void operator delete( void *obj ) {
    m_myalloc.memfree( obj );
  }
  //@}

  /** this function gets called when the store's address operand is
   *    available and it gets executed.  We can compute the virtual
   *    address, do translation, and enter the store in the load/store
   *    queue.  If the store value is ready we can call Complete();
   *    otherwise we just put this dynamic instruction back on the
   *    waiting list for the store value */
  void Execute();
 /** execute (2nd half): called after no LSQ conflicts are detected
  * Used to finish execution after being put on wait list*/
  void          continueExecution();

  /** this function is called when the store data is available --
   *     It performs a demand write to the cache.
   *  @return bool True if the cache hits, false if misses.
   */
  bool storeDataToCache( void );

  /** returns true when the store value is ready.
   */
  bool getStoreValueReady() const {
    return (m_stage == CACHE_NOTREADY_STAGE ||
            m_stage == CACHE_MISS_STAGE ||
            m_stage == COMPLETE_STAGE ||
            m_stage == RETIRE_STAGE);
  }
  
  /** Makes the load instruction "l" depend on this store. */
  void addDependentLoad(memory_inst_t *l) {
    l->InsertWaitQueue(m_dependent_loads);
  }

  /** Makes the load instruction "I" depend on this store, because of partial 
   * address overlap. */
  void addOverlapLoad(memory_inst_t * i){
    i->InsertWaitQueue(m_overlap_loads);
  }

  /** @name dynamic.C interfaces */
  //@{
  /** this function is called when the store data is available from dynamic.C
   *     This calles storeDataToCache, then complete if appropriate.
   */
  void storeDataWakeup( void );

  /** wakeup any loads which are waiting for this value to become available.
   *     This should be called as soon as the data is available from
   *     the register.
   */
  void wakeupDependentLoads();


  /** Wakes up any dependent loads which have overlapping addresses */
  void wakeupOverlapLoads();

  /// issue this memory transaction to the cache
  bool accessCache( void );

  /// retires an instruction
  void Retire( abstract_pc_t *a );

  /* Complete the execution of a store which misses in the cache:
   *    This function should be called when both the store's address, 
   *    data and cache line are available.
   * */
  void Complete();

  /// accessor to determine if this atomic operation wrote memory
  bool atomicMemoryWrite( void ) {
    return m_atomic_swap;
  }

  /** Writes a value to memory: used by swap, casa, casxa instructions */
  void writeValue( ireg_t value ) {
//	DEBUG_OUT(" pc=%llx: seq_num=%llx, writeValue is = %llx\n", getVPC(), getSequenceNumber(), value);
    m_data_storage[0] = value;
    m_data_valid = true;
    m_atomic_swap = true;
  }
  //@}

  /** @name memory allocation:: */
  //@{
  static listalloc_t m_myalloc;
  //@}

  /// print out detailed debugging information about this load or store
  void          printDetail( void );

  bool corrupting_store;
  bool isCorruptingStore() {return corrupting_store;};
#ifdef HARD_BOUND
  void propagateObjects(int in_trap=0) ;
#endif //HARD_BOUND
 
  void copyStoreValue();
  void writeBackStoreValue();

private:

  /** list of loads waiting for this store to complete */
  wait_list_t m_dependent_loads;

  /** list of loads whose memory address overlaps with this store.  Must wait until this store
   * finishes before releasing dependent load */
  wait_list_t m_overlap_loads;

  /** true if this (atomic) did a memory write */
  bool        m_atomic_swap;
  /** Multicore Diagnosis: patch the store value during TMR phase **/
  bool patch_store_value;
  ireg_t previous_value[MEMOP_MAX_SIZE];
};


/**
* A atomic / memory operation instruction in the processor model.
*
* @see     memory_inst_t, dynamic_inst_t, waiter_t
* @author  cmauer
* @version $Id: memop.h 1.56 06/02/13 18:49:19-06:00 mikem@maya.cs.wisc.edu $
*/
//**************************************************************************
class atomic_inst_t : public store_inst_t, public load_interface_t {

public:
  /** @name Constructor(s) / Destructor */
  //@{
  /** Constructor: uses default constructor */
  atomic_inst_t( static_inst_t *s_inst, 
                 int32 window_index,
                 pseq_t *pseq,
                 abstract_pc_t *at, fault_stats* fault_stat );

  /** Destructor: uses default destructor. */
  virtual ~atomic_inst_t();

  /** Allocates object throuh allocator interface */
  void *operator new( size_t size ) {
    return ( m_myalloc.memalloc( size ) );
  }
  
  /** frees object through allocator interface */
  void operator delete( void *obj ) {
    m_myalloc.memfree( obj );
  }
  //@}

  /** @name dynamic.C interfaces */
  //@{
  //  accessCache is implemented in parent class: store_inst_t
  /// retires an instruction
  void Retire( abstract_pc_t *a );

  /* Complete the execution of an atomic which misses in the cache:
   *    This function can get called when both an atomic's address and data
   *    are available, and the cache line is in write mode in the cache.
   *    Atomics are just like stores, except they copy data back. */  
  void Complete();
  //@}

  /** @name lsq interfaces */
  //@{
  /** Raise an exception due to data misspeculation:
   *    the load/store queue forwarded an invalid data item.
   *    (this is called by the offending store instruction on the load. */
  void          replay( void );
  /** read the for this load from the load/store queue: 
   *      a store instruction in the lsq exactly matches this load */
  void          lsqBypass( void );
  /** When the store data is not ready for this load, we wait for it
   *      to become ready */
  void          lsqWait( void );
  /** get the store which this load depends on
   *      The value from this store will be forwarded by lsqbypass() */
  store_inst_t *getDepend( void ) const { return m_depend; }
  /** sets the store that this load depends on. */
  void          setDepend( store_inst_t *depend ) {
    m_depend = depend;
  }
  /** puts this load on a Store's wait list due to overlapping addresses */
  void          addrOverlapWait(store_inst_t * depend); 
  //@}

  /** @name memory allocation:: */
  /// allocator is not needed until atomics size != stores size, anyways...
  //@{
  static listalloc_t m_myalloc;
  //@}

  /// print out detailed debugging information about this load or store
  void          printDetail( void );
#ifdef HARD_BOUND
  void propagateObjects(int in_trap=0) ;
#endif //HARD_BOUND

  bool matchesLoadBuffer();
  bool checkReadRegValue();
  bool collectLLB();
  void copyStoreValue();
  void writeBackStoreValue();
  bool updateOnly() { return update_only;}

  bool setLLBReadPointer(int);

private:
  /** pointer to the store which fed this atomic */
  store_inst_t *m_depend;
  
  /** next program counter: used for replay traps to correct fetch */
  abstract_pc_t m_fetch_at;

  /** Multicore Diagnosis: patch the store value during TMR phase **/
  bool patch_store_value;
	bool update_only;
  ireg_t previous_value[MEMOP_MAX_SIZE];

	int llb_read_pointer;
};

/**
 * An non-faulting memory prefetch.
 *
 * @see     dynamic_inst_t, waiter_t
 * @author  cmauer
 * @version $Id: memop.h 1.56 06/02/13 18:49:19-06:00 mikem@maya.cs.wisc.edu $
 */
//**************************************************************************
class prefetch_inst_t : public memory_inst_t {
public:
  /** @name Constructor(s) / Destructor */
  //@{
  /** Constructor: uses default constructor */
  prefetch_inst_t( static_inst_t *s_inst, 
                   int32 window_index,
                   pseq_t *pseq,
                   abstract_pc_t *at , fault_stats *fault_stat );
  
  /** Destructor: uses default destructor. */
  virtual ~prefetch_inst_t();

  /** Allocates object throuh allocator interface */
  void *operator new( size_t size ) {
    return ( m_myalloc.memalloc( size ) );
  }
  
  /** frees object through allocator interface */
  void operator delete( void *obj ) {
    m_myalloc.memfree( obj );
  }
  //@}

  /** @name dynamic.C interfaces */
  //@{
  /** this function gets called when the prefetches address operand is
   *    available and it gets executed.  We can compute the virtual
   *    address, do translation, and enter the store in the load/store
   *    queue.
   */
  void Execute();

  ///  This function encapsulated the cache access for this memory transaction
  bool accessCache( void );   

  /// retires an instruction
  void Retire( abstract_pc_t *a );

  /* Complete the execution of an atomic which misses in the cache:
   *    This function can get called when both an atomic's address and data
   *    are available, and the cache line is in write mode in the cache.
   *    Atomics are just like stores, except they copy data back. */  
  void Complete();
  //@}

  /** @name memory allocation:: */
  /// allocator is not needed until atomics size != stores size, anyways...
  //@{
  static listalloc_t m_myalloc;
  //@}

  /// print out detailed debugging information about this load or store
  void printDetail( void );
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/**
 *  memop_getASI( ): gets, munges, and returns the ASI for memory operations
 *  
 *  @param  rid    The control register for the %ASI register
 *  @param  bool   (returns)True if this instruction is a 64-byte block load
 *  @return uint32 The value of the %asi from the static instruction
 */
uint16 memop_getASI( uint32 inst, reg_id_t asi_reg, bool *is_block_load );

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/


#endif  /* _MEMOP_H_ */
