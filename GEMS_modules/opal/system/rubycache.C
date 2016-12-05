
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
 * FileName:  rubycache.C
 * Synopsis:  interface to ruby (MP cache simulator)
 * Author:    cmauer
 * Version:   $Id: rubycache.C 1.22 06/02/13 18:49:21-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "pipestate.h"
#include "pipepool.h"
#include "rubycache.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// memory allocator for rubycache.C
listalloc_t ruby_request_t::m_myalloc;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

const char *memop_menomic( OpalMemop_t memop ) {
  switch (memop) {
  case OPAL_LOAD:
    return ("LOAD");
  case OPAL_STORE:
    return ("STORE");
  case OPAL_IFETCH:
    return ("IFETCH");
  case OPAL_ATOMIC:
    return ("ATOMIC");
  default:
    return ("UNKNOWN");
  }
  return ("UNKNOWN");
} 

/*------------------------------------------------------------------------*/
/* ruby_request_t                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
ruby_request_t::ruby_request_t( waiter_t *waiter, pa_t address, 
                                OpalMemop_t requestType, la_t vpc, bool priv,
                                uint64 posttime ) :
  pipestate_t( waiter )
{
  m_physical_address = address;
  m_request_type = requestType;
  m_vpc = vpc;
  m_priv = priv;
  m_post_time = posttime;
}

//**************************************************************************
ruby_request_t::~ruby_request_t( void )
{
}

//**************************************************************************
uint64 ruby_request_t::getSortOrder( void )
{
  return (m_post_time);
}

//**************************************************************************
void ruby_request_t::print( void )
{
  DEBUG_OUT("[0x%0x] element: 0x%0x posted: %lld  ", this, getElement(),
            m_post_time);
  getElement()->print();

#if 0
  DEBUG_OUT("rubylink: [0x%0x]\n", this);
  DEBUG_OUT("   element: 0x%x\n", getElement());
  DEBUG_OUT("   address: 0x%llx\n", m_physical_address );
  DEBUG_OUT("   type   : 0x%d\n", m_request_type);
  DEBUG_OUT("   vpc    : 0x%llx\n", m_vpc);
  DEBUG_OUT("   priv   : 0x%d\n", m_priv);
  DEBUG_OUT("   posted :   %lld\n", m_post_time);
  pipestate_t::print();
#endif
}

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
rubycache_t::rubycache_t( uint32 id, uint32 block_bits,
                          scheduler_t *eventQueue )
{
  m_id = id;
  m_event_queue = eventQueue;
  m_block_size = 1 << block_bits;
  m_block_mask = ~(m_block_size - 1);
  m_timeout_clock = 0;
  m_request_pool = new pipepool_t();
  m_delayed_pool = new pipepool_t();
  m_mshr_count   = 0;
  m_is_scheduled = false;
  // DEBUG_OUT("  rubycache: block bits: %d\n", block_bits );
}

//**************************************************************************
rubycache_t::~rubycache_t( void )
{
}

//**************************************************************************
ruby_status_t rubycache_t::access( pa_t physical_address,
                                   OpalMemop_t requestType, la_t vpc,
                                   bool priv, waiter_t *inst, bool &mshr_hit,
                                   bool &mshr_stall )
{
  ruby_request_t *matchingmiss = NULL;
  pa_t            lineaddr = physical_address & m_block_mask;
  mshr_hit   = false;
  mshr_stall = false;

  // Luke - in order to maintain SC, we cannot have a MSHR that merges write requests.  
  //           However, we can merge read requests to a cache line.
  if(requestType != OPAL_STORE && requestType != OPAL_ATOMIC){
    // search the linked list for an outstanding miss with this address
    //     This is a required invariant for the ruby interface.
    ruby_request_t *miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
    while (miss != NULL) {
      if ( miss->match( lineaddr ) ) {
        matchingmiss = miss;
        break;
      }
      miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
    }
  }
  
  // IFETCH - we should NOT merge IFETCH requests with LD/ST/ATOMIC to the same cache line, bc
  //  the request goes to a different cache. (I-cache vs D-cache)
  if(matchingmiss != NULL && requestType == OPAL_IFETCH){
    if(matchingmiss->m_request_type != OPAL_IFETCH){
      //we cannot merge IFETCH with any other outstanding request types
      matchingmiss = NULL;
    }
    //otherwise we can merge with outstanding IFETCH to same addr
  }
  
  // LD - we should not merge LD requests with IFETCH to the same cache line, bc the request goes to
  //  a different cache. (D-cache vs. I-cache)
  if(matchingmiss != NULL && requestType == OPAL_LOAD){
    if(matchingmiss->m_request_type == OPAL_IFETCH){
      //we cannot merge LD with outstanding IFETCH to same addr
      matchingmiss = NULL;
    }
    //else it is a LD/ST/ATOMIC to same addr, this is OK
  }

  // if not found, see if ruby can accept more requests
  //       This models a finite number of MSHRs.
  if (matchingmiss == NULL) {
#ifndef FAKE_RUBY
    // get a pointer to the ruby api call
    mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
    // issue a miss to ruby
    if ( ruby_api == NULL || ruby_api->makeRequest == NULL ) {
      ERROR_OUT("error: ruby api is called but ruby is not installed.\n");
      SYSTEM_EXIT;
    }

    // if ruby does not accept requests, return
    if (!(*ruby_api->isReady)( m_id, lineaddr, requestType )) {
      return NOT_READY;
    }
#endif

    // check the number of outstanding misses: if too great, wait
    // one hardware context, the sequencer, has a dedicated MSHR reserved
    // as it impacts fetch
    if ( requestType != OPAL_IFETCH &&
         m_mshr_count >= MEMORY_OUTSTANDING_REQUESTS ) {
      
      // add this waiter to a list waiting on MSHR resources
#ifdef DEBUG_RUBY
      DEBUG_OUT("adding memory waiter %d >= %d...\n",
                m_request_pool->getCount(), MEMORY_OUTSTANDING_REQUESTS );
#endif
      
      // add the 'waiter' to the pipeline pool
      // allocate a new ruby miss object
      matchingmiss = new ruby_request_t( inst, lineaddr,
                                         requestType, vpc, priv,
                                         system_t::inst->m_seq[m_id]->getLocalCycle()
                                        );
      m_delayed_pool->insertOrdered( matchingmiss );
      mshr_stall = true;
      return MISS;
    }
    
    // if ruby does accept requests, try for a "fast path hit"
    m_fastpath_request     = lineaddr;
    m_fastpath_outstanding = true;
    m_fastpath_hit         = false;
    
    // call make request
#ifndef FAKE_RUBY
#ifdef  DEBUG_RUBY
    if (requestType != OPAL_IFETCH) {
      DEBUG_OUT("[%d] rubycache:: access %s 0x%0llx\n", m_id,
                memop_menomic(requestType), lineaddr );
      print();
    }
#endif

    // ruby may call opal api to change m_fastpath_hit's value
    (*ruby_api->makeRequest)( m_id, lineaddr, m_block_size,
                              requestType, vpc, priv );
#else
    // complete the request about 25% of the time
    if ( random() % 4 == 0 ) {
      complete( lineaddr );
    }
#endif
    
    if ( m_fastpath_hit ) {
      // hit in fast path
      return HIT;
    }

    // not a fastpath hit
    m_fastpath_outstanding = false;

    // do not increment mshr count until after "fast path hit"
    // only increment count on non-ifetch miss
    if ( requestType != OPAL_IFETCH ) {
      m_mshr_count++;
    }

    // allocate a new miss, add to list
    matchingmiss = new ruby_request_t( inst, lineaddr,
                                       requestType, vpc, priv,
                                       system_t::inst->m_seq[m_id]->getLocalCycle()
                                       );
    m_request_pool->insertOrdered( matchingmiss );
  } else {
    // found miss entry in MSHR
    mshr_hit = true;
#ifdef DEBUG_RUBY
    DEBUG_OUT("rubycache outstanding request: 0x%0llx\n", lineaddr);
#endif
  }
  // missed in cache (either already outstanding or miss in fastpath)
  // add the waiter to the waiter list of this miss
  inst->InsertWaitQueue( matchingmiss->m_wait_list );
  return MISS;
}

void rubycache_t::notifySNETRollback()
{
    mf_ruby_api_t *ruby_api    = system_t::inst->m_ruby_api;   
    (*ruby_api->makeRequest)(m_id, 0, 0,OPAL_SNET_ROLLBACK,0,0);
}

void rubycache_t::notifySNETLogging()
{
    mf_ruby_api_t *ruby_api    = system_t::inst->m_ruby_api;   
    (*ruby_api->makeRequest)(m_id, 0, 0,OPAL_SNET_START_LOGGING,0,0);
}

void rubycache_t::newChkpt()
{
    mf_ruby_api_t *ruby_api    = system_t::inst->m_ruby_api;   
    (*ruby_api->makeRequest)(m_id, 0, 0,OPAL_SNET_NEW_CHKPT,0,0);
}

void rubycache_t::printCLBSize()
{
    mf_ruby_api_t *ruby_api    = system_t::inst->m_ruby_api;   
    (*ruby_api->makeRequest)(m_id, 0, 0,OPAL_SNET_PRINT_CLBSIZE,0,0);
}

//**************************************************************************
ruby_status_t rubycache_t::prefetch( pa_t physical_address,
                                     OpalMemop_t requestType, la_t vpc,
                                     bool priv )
{
#ifdef FAKE_RUBY
  return HIT;
#endif
  // get a pointer to the ruby api call
  pa_t           lineaddr = physical_address & m_block_mask;  
  mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
  
  // issue a miss to ruby
  if ( ruby_api == NULL || ruby_api->makeRequest == NULL ) {
    ERROR_OUT("error: ruby is not installed.\n");
    SYSTEM_EXIT;
  }

  // if ruby does not accept requests, return
  if (!(*ruby_api->isReady)( m_id, lineaddr, requestType )) {
    return NOT_READY;
  }
  (*ruby_api->makePrefetch)( m_id, lineaddr, m_block_size,
                             requestType, vpc, priv );
  // always hit for prefetch
  return HIT;
}

//**************************************************************************
bool rubycache_t::staleDataRequest( pa_t physical_address, char accessSize,
                                    ireg_t *prediction )
{
#ifdef DEBUG_RUBY
  DEBUG_OUT("rubycache: stale data request 0x%0llx\n", m_id, physical_address);
#endif

  if (accessSize == 0)
    return false;

  mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
  int8 *buffer = new int8[m_block_size];
  int   found = (*ruby_api->staleDataRequest)( m_id, physical_address,
                                               (int) accessSize,
                                               buffer );
  
  // if there is no data... return immediately (no formatting to do)
  if (!found) {
    delete [] buffer;
    return found;
  }

  // copy bytes from cache line into correct locations
  // NOTE: the "predicted_storage" (prediction) array is in 'target endianess'
  //       We need to value in 'host endianess' so we do byte swaps

  //   a) calculate the byte offset in the array
  uint32  offset  = physical_address & ~m_block_mask;
  ASSERT( offset < m_block_size );

  //   b) read the memory, possibly correcting the endianess
  if (accessSize == 4) {
    read_array_as_memory32( buffer, prediction, offset );
  } else if (accessSize == 2) {
    read_array_as_memory16( buffer, prediction, offset );
  } else if (accessSize == 1) {
    prediction[0] = buffer[offset];
  } else if (accessSize == 8) {
    read_array_as_memory64( buffer, prediction, offset, 0 );
  } else if (accessSize == 16) {
    read_array_as_memory64( buffer, prediction, offset, 0 );
    read_array_as_memory64( buffer, prediction, offset + 8, 1 );
  } else if (accessSize == 64) {
    read_array_as_memory64( buffer, prediction, offset,     0 );
    read_array_as_memory64( buffer, prediction, offset +  8, 1 );
    read_array_as_memory64( buffer, prediction, offset + 16, 2 );
    read_array_as_memory64( buffer, prediction, offset + 24, 3 );
    read_array_as_memory64( buffer, prediction, offset + 32, 4 );
    read_array_as_memory64( buffer, prediction, offset + 40, 5 );
    read_array_as_memory64( buffer, prediction, offset + 48, 6 );
    read_array_as_memory64( buffer, prediction, offset + 56, 7 );
  } else {
    ERROR_OUT("error: rubycache: unknown access size == %d\n", accessSize );
    delete [] buffer;
    return false;
  }
  
  /* DEBUG_OUT("prediction (%d): 0x%0llx 0x%llx\n",
     accessSize, physical_address, prediction[0]); */
#ifdef DEBUG_RUBY
  DEBUG_OUT("... hit = %d\n", found);
#endif
  delete [] buffer;
  return found;
}

//**************************************************************************
void rubycache_t::complete( pa_t physical_address )
{
  bool        found = false;
  pa_t        lineaddr = physical_address & m_block_mask;

  // check for fast path completion
  if ( m_fastpath_outstanding && physical_address == m_fastpath_request ) {
#ifdef DEBUG_RUBY
    DEBUG_OUT("[%d] rubycache:: fastpath complete 0x%0llx\n", m_id, physical_address);
#endif
    m_fastpath_hit = true;
    //Luke - reset the oustanding flag so that other non-fastpath outstanding requests to same line can be
    //           removed from MSHR
    m_fastpath_outstanding = false;
    return;
  }

  if ( lineaddr != physical_address ) {
    ERROR_OUT("error: rubycache: ruby returns pa 0x%0llx which is not a cache line: 0x%0llx\n", physical_address, lineaddr );
  }
  
  // search the linked list for the address
  ruby_request_t *miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
  while (miss != NULL) {
    if ( miss->match( lineaddr ) ) {
      found = true;
      break;
    }
    miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
  }

  if (found) {
    // first remove the rubymiss from the linked list
    bool found = m_request_pool->removeElement( miss );
    ASSERT( found == true );
    
    // only decrement count on non-ifetch miss
    if ( miss->m_request_type != OPAL_IFETCH ) {
      // decrement the number of (non-icache) outstanding misses
      m_mshr_count--;
    }
    if ( m_delayed_pool->getCount() > 0 ) {
      scheduleWakeup();
    }
    miss->m_wait_list.WakeupChain();
    
#ifdef DEBUG_RUBY
    DEBUG_OUT( "[%d] COMPLETE 0x%0llx 0x%0llx (count:%d) (delay:%d)\n",
               m_id, lineaddr, miss->m_physical_address,
               m_mshr_count, m_delayed_pool->getCount() );
#endif
    delete miss;
  } else {
    // if not found, report a warning
    ERROR_OUT("[%d] error: rubycache: at complete, address 0x%0llx not found.\n", m_id, lineaddr);
    ERROR_OUT("rubycache:: complete FAILS\n");
    print();
    SIM_HALT;
  }
}

//**************************************************************************
void rubycache_t::scheduleWakeup( void )
{
  if (!m_is_scheduled) {
    // wake up next cycle, and handle outstanding miss
    if (m_event_queue != NULL) {
      // if we are running using the event queue, schedule ourselves
      // otherwise rely on caller to poll the is_scheduled variable
      tick_t current_cycle = m_event_queue->getCycle();
      m_event_queue->queueInsert( this, current_cycle, 1 );
    }
    m_is_scheduled = true;
  }
}

//**************************************************************************
void rubycache_t::Wakeup( void )
{
  // The event queue must not be NULL: and will not be for timing simulation
  ASSERT( m_event_queue != NULL );
  m_is_scheduled = false;
  if ( m_mshr_count < MEMORY_OUTSTANDING_REQUESTS &&
       m_delayed_pool->getCount() > 0 ) {

    // select the oldest miss (from decending ordered list)
    ruby_request_t   *miss = (ruby_request_t *) m_delayed_pool->removeHead();
    if (miss != NULL) {
      // re-issue the oldest memory reference in the list
#ifdef DEBUG_RUBY
      DEBUG_OUT("*** issuing [0x%0x] 0x%0x\n", miss, miss->getElement());
#endif
      mf_ruby_api_t *ruby_api    = system_t::inst->m_ruby_api;
      pa_t           re_lineaddr = miss->m_physical_address & m_block_mask;  
        
      // if ruby does not accept requests, halt simulation
      if (!(*ruby_api->isReady)( m_id, re_lineaddr, miss->m_request_type )) {
#ifdef DEBUG_RUBY
        DEBUG_OUT( "warning: rubycache_t: wakeup: unable to reissue request\n");
#endif
        // repost miss to the mshr list, reschedule for next cycle
        m_delayed_pool->insertOrdered( miss );
        scheduleWakeup();
        return;
      }

      bool mshr_hit = false;
      bool mshr_stall = false;
      ruby_status_t status = access( miss->m_physical_address,
                                     miss->m_request_type,
                                     miss->m_vpc,
                                     miss->m_priv,
                                     miss->getElement(),
                                     mshr_hit,
                                     mshr_stall );
      // we just freed up resources, so this shouldn't stall
      ASSERT( !mshr_stall );

      if (status == HIT) {
#ifdef DEBUG_RUBY
        DEBUG_OUT( "*** HIT [0x%0x] 0x%0x\n", miss, miss->getElement());
#endif
        tick_t current_cycle = m_event_queue->getCycle();
        m_event_queue->queueInsert( miss->getElement(), current_cycle, 1 );

      } else if (status == MISS) {
#ifdef DEBUG_RUBY
        DEBUG_OUT( "*** MISS [0x%0x] 0x%0x\n", miss, miss->getElement());
#endif
      } else if (status == NOT_READY) {
        DEBUG_OUT( "*** NOTREADY [0x%0x] 0x%0x\n", miss, miss->getElement());
        SIM_HALT;
      } else {
        DEBUG_OUT( "rubycache:wakeup: error: unknown status return type in wakeup.\n");
        SIM_HALT;
      }
      
      delete miss;
    } else {
      ERROR_OUT( "rubycache:wakeup: error: delayed == %d, but none found on list.\n",
                 m_delayed_pool->getCount() );
      SIM_HALT;
    }
    
    // if there are guys remaining, reschedule for next cycle
    if (m_delayed_pool->getCount() > 0)
      scheduleWakeup();
  }
}

//**************************************************************************
void rubycache_t::squashInstruction( dynamic_inst_t *inst )
{
  ruby_request_t *miss  = static_cast<ruby_request_t*>(m_delayed_pool->walkList(NULL));
  ruby_request_t *match = NULL;
  while (miss != NULL) {
    // FIXME: This seems to be a bug. Shouldn't we be checking the m_wait_list
    // for the instruction that are waiting? Right now, I think, the
    // m_wait_list always has only one element which is what miss->getElement()
    // returns. Therefore, no problem is observed. However, if we ever have
    // more than one element in m_wait_list, what we should do here is:
    //   1. check the m_wait_list for a match
    //   2. remove the matching instruction from the m_wait_list
    //   3. if the matchin instruction is what miss->getElement returns, update
    //      *miss object so getElement() returns one instruction that is still
    //      on the m_wait_list
    //   4. remove the miss structure is all instructions are squashed.
    if ( miss->getElement() == inst ) {
      if (match != NULL) {
        ERROR_OUT("error: rubycache: squashInstruction: inst 0x%0x has multiple instances in list!\n", inst);
        SIM_HALT;
      }
      match = miss;
    }
    miss = static_cast<ruby_request_t*>(m_delayed_pool->walkList( miss ));
  }
  
  if (match != NULL) {
    m_delayed_pool->removeElement( match );
  }
  // match may be NULL as delayed instructions can be issued, or completed
}

//**************************************************************************
void rubycache_t::advanceTimeout( void )
{
  m_timeout_clock++;
  if ( m_timeout_clock % 20 == 0 ) {
    // complete the load or store at the head of the queue
    ruby_request_t *cur = (ruby_request_t *) m_request_pool->removeHead();

    // DEBUG_OUT("    advanceTime completing: 0x%0llx\n", cur->getAddress());
    complete( cur->getAddress() );
  }
}

//**************************************************************************
void rubycache_t::print( void )
{
  DEBUG_OUT("[%d] rubycache_t: delayed: %d outstanding: %d (%d).\n", m_id,
            m_delayed_pool->getCount(), m_mshr_count, m_is_scheduled);

  ruby_request_t *miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
  if (miss == NULL) {
    // DEBUG_OUT("[%d] rubycache_t: no outstanding\n", m_id);
  } else {
    DEBUG_OUT("[%d] rubycache_t: outstanding memory transactions.\n", m_id);
  }
  while (miss != NULL) {
    DEBUG_OUT("[%d]    rubycache_t %s: 0x%0llx ( posttime: %lld )\n", m_id,
              memop_menomic( miss->m_request_type ), miss->getAddress(), miss->m_post_time);
    miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
  }

  // walk the pipeline pool, printing out all elements
  ruby_request_t *link = NULL;
  while (link != NULL) {
    link->print();
    link = (ruby_request_t *) m_delayed_pool->walkList( link );
  }
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//**************************************************************************
void rubycache_t::read_array_as_memory16( int8 *buffer, uint64 *result,
                                           uint32 offset )
{
  if (!ENDIAN_MATCHES) {
    result[0]   =  buffer[offset] & 0xff;
    result[0] <<= 8;
    offset++;
    result[0]  |= buffer[offset] & 0xff;
  } else {
    result[0]  = (buffer[offset] & 0xff);
    offset++;
    result[0] |= (buffer[offset] & 0xff) << 8;
  }
  if ( !(offset < m_block_size) ) {
    ERROR_OUT( "error: read_array_as_memory: final offset = %d\n", offset );
  }
  ASSERT( offset < m_block_size );
}

//**************************************************************************
void rubycache_t::read_array_as_memory32( int8 *buffer, uint64 *result,
                                           uint32 offset )
{
  if (!ENDIAN_MATCHES) {
    result[0]   =  buffer[offset] & 0xff;
    result[0] <<= 8;
    offset++;
    result[0]  |= buffer[offset] & 0xff;
    result[0] <<= 8;
    offset++;
    result[0]  |= buffer[offset] & 0xff;
    result[0] <<= 8;
    offset++;
    result[0]  |= buffer[offset] & 0xff;
  } else {
    result[0]  = ((uint64) buffer[offset] & 0x00ff);
    offset++;
    result[0] |= ((uint64) buffer[offset] & 0x00ff) << 8;
    offset++;
    result[0] |= ((uint64) buffer[offset] & 0x00ff) << 16;
    offset++;
    result[0] |= ((uint64) buffer[offset] & 0x00ff) << 24;
  }
  if ( !(offset < m_block_size) ) {
    ERROR_OUT( "error: read_array_as_memory: final offset = %d\n", offset );
  }
  ASSERT( offset < m_block_size );
}

//**************************************************************************
void rubycache_t::read_array_as_memory64( int8 *buffer, uint64 *result,
                                          uint32 offset, uint32 rindex )
{
  if (!ENDIAN_MATCHES) {
    result[rindex]   =  buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;

#if 0
    result[rindex]  = ((uint64) buffer[offset] & 0x00ff) << 56;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 48;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 40;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 32;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 24;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 16;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 8;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff);
#endif
  } else {
    result[rindex]  = ((uint64) buffer[offset] & 0x00ff);
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 8;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 16;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 24;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 32;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 40;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 48;
    offset++;
    result[rindex]  = ((uint64) buffer[offset] & 0x00ff) << 56;
  }
  if ( !(offset < m_block_size) ) {
    ERROR_OUT( "error: read_array_as_memory: final offset = %d\n", offset );
  }
  ASSERT( offset < m_block_size );
}

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
