
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
#ifndef _LSQ_H_
#define _LSQ_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "memop.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// size of load/store hash table  (internal only)
const uint32 LSQ_HASH_SIZE = 64;
/// mask for load/store hash table (used for wrap around)
const uint32 LSQ_HASH_MASK = LSQ_HASH_SIZE - 1;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Implements a load/store queue.
*
* @see     memory_inst_t
* @author  zilles
* @version $Id: lsq.h 1.13 03/01/06 19:40:56-00:00 cmauer@cottons.cs.wisc.edu $
*/
class lsq_t {

public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param pseq  A pointer to the owning sequencer.
   */
  lsq_t( pseq_t *seq ) {
    m_pseq = seq;
    initialize();
  }

  void initialize( void ) {
    for (uint32 i = 0 ; i < LSQ_HASH_SIZE ; i ++) {
      m_queue[i] = NULL;
    }
  }

  /**
   * Destructor: frees object. (default)
   */
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{

  /** search for stores that conflict with this load in the lsq.
   *  @param  memop  The load to search for a conflict with
   *  @return bool   true if there was an LSQ exception
   */
  bool        loadSearch( memory_inst_t *memop );

  /** search for loads that conflict with this store in the lsq (& replay them)
   */
  void        storeSearch( store_inst_t *memop );

  /** insert a load into the load/store queue
   *  @param  memop The load to insert into the queue.
   *  @return bool  true if there was an LSQ exception
   */
  bool        insert( memory_inst_t *memop ) {
    doInsert(memop);
    return (loadSearch( memop ) );
  }
  
  /** insert a store into the load/store queue
   *  @param  memop The store to insert into the queue.
   */
  bool        insert( store_inst_t *memop ) {
    doInsert(memop);
    storeSearch( memop );
    if (memop->getStaticInst()->getType() == DYN_ATOMIC) {
      return (loadSearch( memop ));
    }
    return (false);
  }

  /** removes a memory instruction from the load/store queue.
   */
  void        remove( memory_inst_t *memop );
  //@}

  /** Verifies the load/store queues by traversing each outstanding
   *  memory operation, and verifying that the instruction window
   *  still contains an outstanding memory reference
   */
  void verifyQueues( void ) {
    for (int i = 0 ; i < (int)LSQ_HASH_SIZE ; i ++) {
      verify(m_queue[i]);
    }
  }

  /** Prints out the contents of the load/store data structure. */
  void printLSQ( void ) {
    for (int i = 0 ; i < (int)LSQ_HASH_SIZE ; i ++) {
      verify(m_queue[i], false);
    }
  }
  
private:
  /// Verify the integrity of one address bucket in the load/store queue
  void        verify(memory_inst_t *lsq_slice, bool halt_on_error=true );

  /** insert a memory instruction into the load/store queue
   *  called by insert( load/store ) methods.
   *  @return bool  true if there was an LSQ exception
   */
  void        doInsert( memory_inst_t *memop );

  /// wraps around in a load/store queue window
  uint32      generateIndex(my_addr_t addr) const { 
    return (((uint32) addr) & LSQ_HASH_MASK);
  }

  /** The sequencer this load/store queue is associated with:
   *  used for validation and for debugging output
   */
  pseq_t        *m_pseq;

  /** The load/store queue: memory instructions waiting for transactions */
  memory_inst_t *m_queue[LSQ_HASH_SIZE];
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _LSQ_H_ */
