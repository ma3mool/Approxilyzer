
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
 * FileName:  pfile.C
 * Synopsis:  Physical register file (integer or floating point)
 * Author:    zilles
 * Version:   $Id: regfile.C 1.18 06/02/13 18:49:21-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "regfile.h"

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
physical_file_t::physical_file_t( uint16 num_physical, pseq_t *seq, fault_stats* fault_stat ) {

//printf("=======-------====== physical_file_t constructor %d \n", num_physical);
  m_num_physical = num_physical;
  m_pseq = seq;
  m_fs = fault_stat;
  m_reg = (physical_reg_t *) malloc( sizeof(physical_reg_t) * num_physical );
  
  /* initialize all registers as not ready */
  for (uint16 i = 0 ; i < m_num_physical ; i ++) {
    m_reg[i].wait_list.wl_reset();
    m_reg[i].isReady = false;
    m_reg[i].isFree  = true;
    m_reg[i].as_int  = 0;
  }

  m_watch = (uint16) -1;
  patched = new bool[num_physical]; 
  has_fault = new bool[num_physical]; 
  corrupted = new bool[num_physical]; 
  read_fault = new bool[num_physical];
  ref_count = new int[num_physical];
  sync_target = new uint16[num_physical];
  for( int i=0;i<num_physical;i++ ) {
	  has_fault[i] = false;
	  read_fault[i] = false;
	  ref_count[i] = 0 ;
	  sync_target[i] = PSEQ_INVALID_REG ;
	  corrupted[i] = false;
	  patched[i] = false;
  }

  reg_file_type = UNKNOWN_REG_FILE; // Type unknown for now
  wake_dep =true;
}

physical_file_t::physical_file_t( uint16 num_physical, pseq_t *seq, int rf_type, fault_stats* fault_stat ) {

  m_num_physical = num_physical;
  m_pseq = seq;
  m_fs = fault_stat;
  m_reg = (physical_reg_t *) malloc( sizeof(physical_reg_t) * num_physical );
  
  /* initialize all registers as not ready */
  for (uint16 i = 0 ; i < m_num_physical ; i ++) {
    m_reg[i].wait_list.wl_reset();
    m_reg[i].isReady = false;
    m_reg[i].isFree  = true;
    m_reg[i].as_int  = 0;
  }

  m_watch = (uint16) -1;

  patched = new bool[num_physical]; 
  has_fault = new bool[num_physical]; 
  corrupted = new bool[num_physical]; 
  read_fault = new bool[num_physical];
  ref_count = new int[num_physical];
  sync_target = new uint16[num_physical];
  for( int i=0;i<num_physical;i++) {
	  has_fault[i] = false; 
	  read_fault[i] = false;
      ref_count[i] = 0;
      sync_target[i] = PSEQ_INVALID_REG ;
	  corrupted[i] = false;
	  patched[i] = false;
  }

  reg_file_type = rf_type;
  wake_dep =true;

}

//***************************************************************************
physical_file_t::~physical_file_t() {
  free( m_reg );
}

//***************************************************************************
void
physical_file_t::dependenceError( uint16 reg_no ) const
{
#if 0
  ERROR_OUT("dependence error: register %d is not ready.\n", reg_no);
  FAULT_OUT("dependence error: register %d is not ready.\n", reg_no);
  FAULT_OUT("checking states? %d\n",debugio_t::getChecking());
	FAULT_OUT("[%d]\t", m_pseq->GET_CLOCK() ) ;
	FAULT_OUT("[%d]\t\tDependence_Error", m_pseq->GET_RET_INST() ) ;
	FAULT_OUT("(%d)\n",m_pseq->GET_PRIV());

	m_pseq->FAULT_SET_NON_RET_TRAP() ;
  m_pseq->out_info("error: register %d is not ready.\n", reg_no);
  m_pseq->out_info("error: cycle = %lld\n", m_pseq->getLocalCycle());
	HALT_SIMULATION ;
  m_pseq->printInflight();
  ASSERT(0);
#endif
}

//***************************************************************************
void 
physical_file_t::print(uint16 reg_no)
{
  DEBUG_OUT("Register: %d\n", reg_no);
  DEBUG_OUT("intvalue= 0x%0llx\n", m_reg[reg_no].as_int);
  DEBUG_OUT("isReady = %d\n", m_reg[reg_no].isReady);
  DEBUG_OUT("isFree  = %d\n", m_reg[reg_no].isFree);
  DEBUG_OUT("wl      = ");
  cout << m_reg[reg_no].wait_list << endl;
}

/** @name Accessors / Mutators */

/// wait on this register to be written (i.e. to become ready)
//***************************************************************************
void
physical_file_t::waitResult( uint16 reg_no, dynamic_inst_t *d_instr )
{
    if (reg_no >= m_num_physical) {
        return;
    }
  ASSERT(reg_no < m_num_physical);
  ASSERT( m_reg[reg_no].isFree == false );
  d_instr->InsertWaitQueue( m_reg[reg_no].wait_list );
}

//***************************************************************************
bool
physical_file_t::isWaitEmpty(uint16 reg_no)
{
  return (m_reg[reg_no].wait_list.Empty());
}

bool 
physical_file_t::isReady(uint16 reg_no) const {

	if(SIVA_DEBUG)
	{
		printf("----------- isReady():regfile.C 1a----- %d %d \n", reg_no, m_num_physical);
		fflush(stdout);
	}

    if (reg_no >= m_num_physical) {
        return false;
    }
  ASSERT(reg_no < m_num_physical);
  return ( m_reg[reg_no].isReady );
}

