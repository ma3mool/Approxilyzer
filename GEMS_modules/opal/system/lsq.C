
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
 * FileName:  lsq.C
 * Synopsis:  Implements the functionality of the load/store queue
 * Author:    cmauer
 * Version:   $Id: lsq.C 1.15 06/02/13 18:49:19-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "lsq.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void lsq_t::doInsert( memory_inst_t *memop ) {

  uint64          lsqe_seq_num = memop->getSequenceNumber();
  memory_inst_t  *trav = m_queue[ generateIndex(memop->getLineAddress()) ];
  
#ifdef EXPENSIVE_ASSERTIONS
  verifyQueues();
#endif

  ASSERT( !memop->getEvent(EVENT_LSQ_INSERT) );
#ifdef EXPENSIVE_ASSERTIONS
  m_pseq->out_info("lsq insert: %d 0x%0x\n", memop->getWindowIndex(), (int) memop);
#endif
  memop->markEvent(EVENT_LSQ_INSERT);
  
  /* insert into the queue -- the LSQ is 'perfectly disambiguated' by the
   * sequence number of the instruction.
   */

  // LSQ_FAULT - LSQ is organized as an array of size LSQ_HASH_SIZE,
  // where each index is a bucket of memops with max size of 2*IWINDOW_SIZE
  // Fault is now injected into a particular index and always in the
  // first entry in that bucket.
  if (trav == NULL || (trav->getSequenceNumber() <= lsqe_seq_num)) {
#ifdef EXPENSIVE_ASSERTIONS
    ASSERT( trav != memop );
    m_pseq->out_info("lsq head insert seq:%d address:0x%0llx index: 0x%0x\n",
                     memop->getWindowIndex(),
                     memop->getLineAddress(), 
                     generateIndex(memop->getLineAddress()));
#endif
    /* insert at head of list */
    memop->lsq_prev = NULL;
    memop->lsq_next = trav;
    if (trav != NULL) {
        trav->lsq_prev = memop;
    }
    m_queue[ generateIndex(memop->getLineAddress()) ] = memop;
    // memop is now at the head of the queue at this given index
	if( (m_pseq->GET_FAULT_TYPE()==LSQ_ADDRESS_FAULT) &&
			(generateIndex(memop->getLineAddress())==m_pseq->GET_FAULTY_REG()) ) {
		// Then inject an address fault into first 32 bits of physical address
		my_addr_t new_address ;
		// for lsq address injections, the bit is > 6, taken care of by genjobs.pl
		new_address = (my_addr_t) m_pseq->INJECT_FAULT( memop->getPhysicalAddress() ) ;

		if( new_address != memop->getPhysicalAddress() ) {
			// if( memop->getStaticInst()->getType() == DYN_STORE ) {
			// 	m_pseq->INC_STORE() ;
			// }
			if( PRADEEP_DEBUG_LSQ ) {
				DEBUG_OUT( "%d ", memop->getSequenceNumber() ) ;
				DEBUG_OUT( "(%s) <- Address fault -- ", 
						decode_opcode( (enum i_opcode)
							memop->getStaticInst()->getOpcode() )
						) ;
				DEBUG_OUT( "%llx->", memop->getPhysicalAddress() ) ;
				DEBUG_OUT( "%llx\n", new_address ) ;
			}
			// // Store the old data and get the old value and store it
			memop->setOldPhysicalAddress( memop->getPhysicalAddress() ) ;
			memop->setFault() ;
			memop->setMemFault() ;
			memop->setPhysicalAddress( new_address ) ;
			// memop->setPhysicalAddress( memop->getPhysicalAddress() ) ;
			// Check if the generated address is misaligned
			memop->checkAddressAlignment() ;
		}
	}
	if( (m_pseq->GET_FAULT_TYPE()==LSQ_DATA_FAULT) &&
			(generateIndex(memop->getLineAddress())==m_pseq->GET_FAULTY_REG()) &&
			(memop->getStaticInst()->getType() == DYN_STORE ) ) { // Inject data faults only in stores
		// Then insert into data values
		// We have to corrupt the data values that the LOAD writes to mem
		// or that the store gets from mem. However, these are not done at
		// this point. So, we just set the load/store as faulty and
		// change the data values appropriately when they get the data
		if( PRADEEP_DEBUG_LSQ ) {
			DEBUG_OUT( "(%s)-- ", 
					decode_opcode( (enum i_opcode)
						memop->getStaticInst()->getOpcode() )
					) ;
			DEBUG_OUT( "%d <- may get Data fault\n",
					memop->getSequenceNumber() ) ;
		}
		memop->setInjectFault() ;
	}
  } else {
    /* insert in middle of list (can happen as we are executing O-o-O),
     * or at the end of the list.
     */
    for ( ;; ) {
      ASSERT(trav != NULL);
      if ((trav->lsq_next == NULL) || 
          (trav->lsq_next->getSequenceNumber() < lsqe_seq_num)) {
        if (trav->lsq_next != NULL) { trav->lsq_next->lsq_prev = memop; }
        memop->lsq_prev = trav;
        memop->lsq_next = trav->lsq_next;
        trav->lsq_next = memop;
        break;
      }
      trav = trav->lsq_next;
    }
  }
}

//**************************************************************************
void lsq_t::remove( memory_inst_t *memop ) {
  memory_inst_t *next = memop->lsq_next;

  ASSERT( memop->lsq_next != memop );

  // link the next element to our previous element
  if (next != NULL) {
    if (next->lsq_prev != memop) {
      m_pseq->out_info("lsq: warning: I am          0x%0x\n", (int) memop);
      memop->printDetail();
      m_pseq->out_info("lsq: warning: next is       0x%0x\n", (int) next);
      next->printDetail();
      m_pseq->out_info("lsq: warning: next->prev is 0x%0x\n", (int) next->lsq_prev);
      ASSERT(next->lsq_prev == memop);      
	  assert(0) ;
	  HALT_SIMULATION ;
    }
    next->lsq_prev = memop->lsq_prev;
  }

  // link the previous element to our next element
  if (memop->lsq_prev == NULL) {
      if (m_pseq->GET_FAULT_TYPE()==LSQ_ADDRESS_FAULT && memop->getMemFault() ) {
          // Know that it was indexed with old addr
          ASSERT( memop->getOldLineAddress() != (my_addr_t) -1 );
          m_queue[ generateIndex(memop->getOldLineAddress()) ] = next;
          
      } else {
          ASSERT( memop->getLineAddress() != (my_addr_t) -1 );
#ifdef EXPENSIVE_ASSERTIONS
          if (next != NULL)
              ASSERT( generateIndex(memop->getLineAddress()) ==
                      generateIndex(next->getLineAddress()) );
#endif
          m_queue[ generateIndex(memop->getLineAddress()) ] = next;
      }
  } else {
    memop->lsq_prev->lsq_next = next;
  }

  memop->lsq_prev = NULL;
  memop->lsq_next = NULL;
  
#ifdef EXPENSIVE_ASSERTIONS
  verifyQueues();
#endif

}

//**************************************************************************
bool lsq_t::loadSearch( memory_inst_t *memop ) {

  // snoop for stores which match this address
  memory_inst_t *trav = m_queue[ generateIndex(memop->getLineAddress()) ];
  my_addr_t lsq_line_addr = memop->getLineAddress();
  uint64    lsq_seq_num   = memop->getSequenceNumber();
  int32     listcount = 0;

#ifdef DEBUG_LSQ
  m_pseq->out_info("DEBUG_LSQ: loadSearch: index:%d addr:0x%0llx line:0x%0x seq:%d\n",
                   memop->getWindowIndex(), memop->getAddress(),
                   lsq_line_addr, lsq_seq_num);
#endif
  
  while (trav != NULL) {
    /* only get a match if is a store to the same address and has
       retire or has an older sequencer number */
    bool match = ( (trav->getLineAddress() == lsq_line_addr) &&
                   (trav->getSequenceNumber() < lsq_seq_num) &&
                   (trav->getStaticInst()->getType() != DYN_LOAD) );
#ifdef DEBUG_LSQ
    m_pseq->out_info("DEBUG_LSQ: index:%d line:0x%0llx addr:0x%0llx match:%d\n",
                     trav->getWindowIndex(), trav->getLineAddress(),
                     trav->getAddress(), match);
#endif
    if ( match ) {
      /* Check for overlap between accesses */
      uint16 overlap = memop->addressOverlap(trav);
#ifdef DEBUG_LSQ
      m_pseq->out_info("DEBUG_LSQ: overlap myindex:%d index:%d line:0x%0llx %d\n",
                       memop->getWindowIndex(), trav->getWindowIndex(),
                       memop->getLineAddress(), overlap);
#endif
      load_interface_t  *bypass_interface;
      if (memop->getStaticInst()->getType() == DYN_LOAD) {
        bypass_interface = static_cast<load_interface_t *>(static_cast<load_inst_t *>(memop));
      } else if (memop->getStaticInst()->getType() == DYN_ATOMIC) {
        bypass_interface = static_cast<load_interface_t *>(static_cast<atomic_inst_t *>(memop));
      } else {
        SIM_HALT;
      }

      if (overlap == MEMOP_EXACT) {

        /* if there is a complete match, bypass the data */
        store_inst_t      *depend = static_cast<store_inst_t *>(trav);
        bypass_interface->setDepend( depend );
        if (depend->isDataValid()) {
#ifdef DEBUG_LSQ
          m_pseq->out_info("DEBUG_LSQ: bypassing index:%d -> %d\n",
                           depend->getWindowIndex(), memop->getWindowIndex() );
#endif
          bypass_interface->lsqBypass();
        } else {
          /* we found a match in the store queue, but it's value
             wasn't available, so wait for the store value */
#ifdef DEBUG_LSQ
          m_pseq->out_info("DEBUG_LSQ: !svr: waiting... index:%d -> %d\n",
                           depend->getWindowIndex(), memop->getWindowIndex() );
#endif
          bypass_interface->lsqWait();
        }
        return true;
        
      } else if (overlap == MEMOP_OVERLAP) {
        
#ifdef DEBUG_LSQ
        m_pseq->out_info("DEBUG_LSQ: replay trap triggered instr %d\n",
                         memop->getWindowIndex());
#endif
        /* partial overlap - put load on wait list for store to retire*/
        store_inst_t      *depend = static_cast<store_inst_t *>(trav);
        bypass_interface->addrOverlapWait(depend);
        return true;
      }
    }
    trav = trav->lsq_next;

    // check that the LSQ is not insanely large
    //    This could be a performance issue, as large capacity LSQs 
    //    imply lots of linear linked-list searches.
    listcount++;
    if ( listcount >= 2*IWINDOW_WIN_SIZE ) {
      ERROR_OUT("error: LSQ is far too large: %d\n", listcount);
      //printLSQ();
      SIM_HALT;
    }
  }
  return false;
}

//**************************************************************************
void lsq_t::storeSearch( store_inst_t *memop )
{
  // snoop for loads which match this address that are younger
  memory_inst_t *trav = m_queue[ generateIndex(memop->getLineAddress()) ];
  my_addr_t lsq_line_addr = memop->getLineAddress();
  uint64    lsq_seq_num   = memop->getSequenceNumber();
  int32     listcount = 0;

#ifdef DEBUG_LSQ
  m_pseq->out_info("DEBUG_LSQ: storeSearch: index:%d addr:0x%0llx line:0x%0x seq:%d\n",
                   memop->getWindowIndex(), memop->getAddress(),
                   lsq_line_addr, lsq_seq_num);
#endif
  
  while (trav != NULL) {
    
    bool match = ((trav->getLineAddress() == lsq_line_addr) &&
                  (trav->getSequenceNumber() > lsq_seq_num) &&
                  (trav->getStaticInst()->getType() != DYN_STORE));
#ifdef DEBUG_LSQ
    m_pseq->out_info("DEBUG_LSQ: index:%d line:0x%0llx addr:0x%0llx match:%d\n",
                     trav->getWindowIndex(), trav->getLineAddress(),
                     trav->getAddress(), match);
#endif
    if (match) {
      /* find all younger loads to the same address and Replay all that
       * data misspeculated (i.e. loaded values older than this store)
       */

      /* Check for overlap between accesses */
      if (memop->addressOverlap(trav)) {
        
#ifdef DEBUG_LSQ
        m_pseq->out_info("DEBUG_LSQ: overlap myindex:%d index:%d line:0x%0x %d\n",
                  memop->getWindowIndex(), trav->getWindowIndex(),
                  memop->getLineAddress());
#endif
        
        /* use the depend field in the load to avoid searching for
         * the younger store. */
        load_interface_t  *bypass_interface;
        if (trav->getStaticInst()->getType() == DYN_LOAD) {
          bypass_interface = static_cast<load_interface_t *>(static_cast<load_inst_t *>(trav));
        } else if (trav->getStaticInst()->getType() == DYN_ATOMIC) {
          bypass_interface = static_cast<load_interface_t *>(static_cast<atomic_inst_t *>(trav));
        } else {
          SIM_HALT;
        }
        
        store_inst_t *orig_store = bypass_interface->getDepend();
        if ((orig_store == NULL) || 
            (lsq_seq_num > orig_store->getSequenceNumber())) {
          /* no dependent store, or was older than this store, hence
             data misspeculation */
#ifdef DEBUG_LSQ
          m_pseq->out_info("DEBUG_LSQ: store search: replay trap triggered by instr %d on instr %d\n",
                           memop->getWindowIndex(), trav->getWindowIndex());
#endif
          bypass_interface->replay();
        }
      }
    }
    trav = trav->lsq_next;

    // check that the LSQ is not insanely large
    listcount++;
    if ( listcount >= 2*IWINDOW_WIN_SIZE ) {
      ERROR_OUT("error: LSQ is far too large: %d\n", listcount);
      //printLSQ();
      SIM_HALT;
    }
  }
}

//**************************************************************************
void lsq_t::verify(memory_inst_t *lsq_slice, bool halt_on_error) {
  memory_inst_t *trav = lsq_slice;
  memory_inst_t *prev = NULL;
  
  while (trav != NULL) {
    
    m_pseq->out_info("lsq: %d: 0x%0x: index:0x%0x address: 0x%0llx\n",
                     trav->getWindowIndex(),
                     (int) trav, generateIndex(trav->getLineAddress()),
                     trav->getLineAddress());
    switch (trav->getStaticInst()->getType()) {
    case DYN_LOAD:
      static_cast<load_inst_t *>(trav)->printDetail();
      break;
    case DYN_STORE:
      static_cast<store_inst_t *>(trav)->printDetail();
      break;
    case DYN_ATOMIC:
      static_cast<atomic_inst_t *>(trav)->printDetail();
      break;
    default:
      static_cast<memory_inst_t *>(trav)->printDetail();
    }

    if (m_queue[ generateIndex(trav->getLineAddress()) ] != lsq_slice) {
      m_pseq->out_info( "error: LSQ index not in slice: 0x%0llx %d\n",
                        trav->getLineAddress(),
                        generateIndex(trav->getLineAddress()) );
      if (halt_on_error) {
        SIM_HALT;
		HALT_SIMULATION ;
      }
    }
    
    if ( trav->lsq_prev != prev ) {
      m_pseq->out_info( "error: LSQ is not a doubly-linked list.\n" );
      if (halt_on_error) {
        SIM_HALT;
		HALT_SIMULATION ;
      }
    }

    if (prev != NULL) {
      if (prev->getSequenceNumber() <= trav->getSequenceNumber()) {
        m_pseq->out_info( "error: LSQ is not in increasing order!\n");
        m_pseq->out_info( "error: prev seq %lld, current seq %lld\n",
                          prev->getSequenceNumber(), 
                          trav->getSequenceNumber() );
        if (halt_on_error) {
          SIM_HALT;
		HALT_SIMULATION ;
        }
      }
    }
    
    if (trav->getEvent(EVENT_LSQ_INSERT) != true ) {
      m_pseq->out_info( "error: memory instruction not marked as LSQ insert\n");
      trav->printDetail();
      if (halt_on_error) {
        SIM_HALT;
		HALT_SIMULATION ;
      }
    }
    
    // Verify the Instruction is at the correct WindowPosition
    if ( !(trav->getEvent(EVENT_IWINDOW_REMOVED)) ) {
      if (!m_pseq->getIwindow()->isAtPosition(trav, trav->getWindowIndex())) {
        m_pseq->out_info("lsq window violation: index:%d\n",
                         trav->getWindowIndex());
        trav->printDetail();
        if (halt_on_error) {
          SIM_HALT;
		  HALT_SIMULATION ;
        }
      }
    }

    prev = trav;
    trav = trav->lsq_next;
  }
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/


/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************

