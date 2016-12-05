
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
#ifndef _RUBYCACHE_H_
#define _RUBYCACHE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
 *  Status for ruby accesses. The memory hierarchy can hit, miss or
 *  not be able to handle a memory request.
 */
enum ruby_status_t { MISS, HIT, NOT_READY };

/**
 * ruby_request is a doubly-linked list structure of misses (that
 * may or may not have issued) to the ruby memory cache hierarchy.
 * 
 * Note that some cache accesses wont create a request structure, 
 * as they can hit immediately in the cache. (This is the fast_path
 * hit optimization in the ruby_cache.)
 *
 * ruby_requests may also have not issued to the memory hierarchy
 * (if issued == false there are insufficient resources).
 */

class ruby_request_t : public pipestate_t {
public:
  /** @name Constructor(s) / Destructor */
  //@{
  /// Constructor: creates object
  ruby_request_t( waiter_t *waiter, pa_t address, OpalMemop_t requestType,
                  la_t vpc, bool priv, uint64 posttime );
  /// Destructor: frees object.
  ~ruby_request_t( );

  /** Allocates object throuh allocator interface */
  void *operator new( size_t size ) {
    return ( m_myalloc.memalloc( size ) );
  }
  /** frees object through allocator interface */
  void operator delete( void *obj ) {
    m_myalloc.memfree( obj );
  }
  //@}

  /** @name methods */
  //@{
  bool match( pa_t other_address ) {
    return ( other_address == m_physical_address );
  }
  
  /// get the address of the cache line miss
  pa_t getAddress( void ) {
    return m_physical_address;
  }

  /// The sorting order of these elements: based on their instruction #
  uint64      getSortOrder( void );

  /// A debugging print function
  void        print( void );
  //@}

  /** @name memory allocation:: */
  //@{
  static listalloc_t m_myalloc;
  //@}

  /// The physical address that missed in the cache (may be cache line address)
  pa_t        m_physical_address;
  
  /// The type of request that this miss is.
  OpalMemop_t m_request_type;
  
  /// virtual address of the program counter that caused this miss
  la_t        m_vpc;

  /// true if the instruction is privileged
  bool        m_priv;

  /// the time this instruction was inserted
  uint64      m_post_time;
  
  /// wait list of dynamic instructions
  wait_list_t m_wait_list;
};

/**
* Interface object between ruby and opal. Handles misses to caches.
*
* @see     rubymiss_t, ruby_request_t
* @author  cmauer
* @version $Id: rubycache.h 1.14 06/02/13 18:49:21-06:00 mikem@maya.cs.wisc.edu $
*/
class rubycache_t : public waiter_t {

public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /**
   * Constructor: creates object
   * @param id         The id of the sequencer that this cache belongs to
   * @param block_size The size of the cache lines (in bits) in the system
   * @param eventQueue The event queue (used when it is necessary to reschedule due to resource conflicts). If this is NULL, the scheduler relies on the caller to poll the 'm_is_scheduled' variable to detect when 'Wakeup() should be called.
   */
  rubycache_t( uint32 id, uint32 block_size, scheduler_t *eventQueue );

  /**
   * Destructor: frees object.
   */
  virtual ~rubycache_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /**
   * called to initiate a load / store access.
   * @param physical_address The address which has been loaded or stored
   * @param mode Indicates if this is a load or a store
   * @param waiter The instruction to wake up (only if the instruction misses) when the ld/st is complete
   * @param mshr_hit true indicates a hit in the mshr
   * @param mshr_stall true indicates miss wasn't missed from lack of resources
   * @return bool true indicates hit, false indicates miss
   */
  ruby_status_t access( pa_t physical_address, OpalMemop_t requestType, la_t vpc, bool priv, waiter_t *inst, bool &mshr_hit, bool &mshr_stall );

  /**
   * called to initiate a prefetch
   * @param physical_address The address which has been loaded or stored
   * @param mode Indicates if this is a load or a store
   */
  ruby_status_t prefetch( pa_t physical_address, OpalMemop_t requestType, la_t vpc, bool priv );

  /**
   * called to make a stale data request
   */
  bool  staleDataRequest( pa_t physical_address, char accessSize,
                          ireg_t *prediction );

  /**
   * called when a load / store is completed.
   * @param physical_address The address which has been loaded or stored
   */
  void complete( pa_t physical_address );

  /**
   *   Schedule the ruby cache to be woken up next cycle to handle delayed misses.
   */
  void scheduleWakeup( void );
  
  /**
   *   Wakeup to launch memory references stalled due to MSHR limits
   */
  void Wakeup( void );
  
  /**
   *   Notify the ruby cache of a squash of an instruction
   */
  void squashInstruction( dynamic_inst_t *inst );

  /**
   *  Debugging interface to advance timing: ticks the internal clock 
   *  of rubycache. After a number of cycles, any outstanding request 
   *  automatically complete.
   */
  void advanceTimeout( void );

  /**
   * Prints all outstanding memory references.
   */
  void print( void );

  /// get cache block size
  uint32 getBlockSize( void ) { return m_block_size; };
  //@}

  // Methods for SafetyNet
  void notifySNETRollback();
  void notifySNETLogging();
  void newChkpt();
  void printCLBSize();

private:
  /// read a character array as 16-bit memory value (converting endianess)
  void  read_array_as_memory16( int8 *buffer, uint64 *result, uint32 offset );

  /// read a character array as 32-bit memory value (converting endianess)
  void  read_array_as_memory32( int8 *buffer, uint64 *result, uint32 offset );

  /// read a character array as 64-bit memory value (converting endianess)
  void  read_array_as_memory64( int8 *buffer, uint64 *result, uint32 offset,
                                uint32 rindex );
  
  /// the sequencer's id for this processsor
  int32          m_id;
  /// the scheduler (used for rescheduling)
  scheduler_t   *m_event_queue;

  /// number of bytes in cacheline
  uint32         m_block_size;
  /// the clock for this module
  uint64         m_timeout_clock;

  /// memory request currently outstanding
  pa_t           m_fastpath_request;
  /// true if there is a 'fast path' request outstanding
  bool           m_fastpath_outstanding;
  /// hit status of last request
  bool           m_fastpath_hit;

  /// pool of instructions that are outstanding to memory
  pipepool_t    *m_request_pool;
  /// count of (non-icache) outstanding misses to the lower level of cache
  uint32         m_mshr_count;

  /// pool of instructions not currently being executed for lack of resources
  pipepool_t    *m_delayed_pool;

  /// boolean value: true if we are scheduled to run, false otherwise
  bool           m_is_scheduled;
  /// mask to strip off non-cache line bits
  pa_t           m_block_mask;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _RUBYCACHE_H_ */
