
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
 * FileName:  iwindow.C
 * Synopsis:  instruction window (container)
 * Author:    cmauer
 * Version:   $Id: iwindow.C 1.37 06/02/13 18:49:19-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "iwindow.h"

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

//**************************************************************************
iwindow_t::iwindow_t(uint32 rob_size, uint32 win_size)
{
	ASSERT(rob_size >= win_size && win_size > 0);

	m_rob_size = rob_size;
	m_win_size = win_size;
	m_window   = (dynamic_inst_t **) malloc( sizeof(dynamic_inst_t *) * m_rob_size );
	initialize();
}

//**************************************************************************
iwindow_t::~iwindow_t()
{
	if (m_window) {
		free( m_window );
		m_window = NULL;
	}
}

//**************************************************************************
void iwindow_t::initialize( void ) {
	for (uint32 i=0; i < m_rob_size; i++) {
		m_window[i] = NULL;
	}

	m_last_fetched   = m_rob_size - 1;
	m_last_decoded   = m_rob_size - 1;
	m_last_scheduled = m_rob_size - 1;
	m_last_retired   = m_rob_size - 1;

	rob_head = 0;
	rob_tail = 0;
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//***************************************************************************
uint32 iwindow_t::getInsertIndex( void )
{
	// calculate the next slot in the window (wrap around)
	uint32  nextslot = iwin_increment(m_last_fetched);
	return (nextslot);
}

//**************************************************************************
void  iwindow_t::insertInstruction( dynamic_inst_t *instr )
{
	byte_t new_dest_vanilla ;
	// calculate the next slot in the window (wrap around)
	uint32  nextslot = iwin_increment(m_last_fetched);

	// check that we insert into an empty slot
	ASSERT( m_window[nextslot] == NULL );

	// insert instruction, increment last fetched instruction (wrap around)
	m_window[nextslot]   = instr;

	m_last_fetched       = nextslot;
}

//**************************************************************************
dynamic_inst_t *iwindow_t::decodeInstruction( void )
{
	// calculate the next slot in the window (wrap around)
	m_last_decoded = iwin_increment(m_last_decoded);

	// return instruction
	return (m_window[m_last_decoded]);
}

#define ROB_FAULT_DEBUG 0

//**************************************************************************
dynamic_inst_t *iwindow_t::scheduleInstruction( void )
{
	// make sure instruction window is not full
	if( rangeSubtract(m_last_scheduled, m_last_retired) >= (int32) m_win_size)
		return NULL;

	// calculate the next slot in the window (wrap around)
	m_last_scheduled = iwin_increment(m_last_scheduled);

	dynamic_inst_t *instr=m_window[m_last_scheduled];

	if (ROB_FAULT_DEBUG && instr->getPSEQ()->recovered) {
		FAULT_OUT("%llx:%llx ",instr->getSequenceNumber(),instr->getVPC());
		for (int i=0;i<SI_MAX_DEST;i++) {
			FAULT_OUT("d %d ",instr->getDestReg(i).getPhysical());
		}
		FAULT_OUT("\n");
	}

	// LXL: Fault injection into the instruction Window
	if( (instr!=NULL) && (m_fs->getFaultType()==INST_WINDOW_FAULT) && 
		(m_last_scheduled==m_fs->getFaultyReg()) ) {

		int src_dst_idx = m_fs->getFaultSrcDst();

		if (src_dst_idx >= 4) {
			src_dst_idx= 0;//(src_dst_idx-4)&0x1;
			half_t new_dest_physical;
			half_t old_dest_physical ;
			reg_id_t &dest = instr->getDestReg(src_dst_idx) ;
			if (dest.getRtype() == RID_INT || dest.getRtype()==RID_INT_GLOBAL) {
				if( !dest.isZero() ) {
					old_dest_physical = dest.getPhysical() ;
					new_dest_physical = m_fs->injectFault( old_dest_physical);

					if( new_dest_physical != old_dest_physical ) {
						if ((ROB_FAULT_DEBUG)) { // && instr->getPSEQ()->recovered)) {
							DEBUG_OUT("%lld:%llx ",instr->getSequenceNumber(),instr->getVPC());
							DEBUG_OUT("dest %d changed to %d\n",old_dest_physical,new_dest_physical);
						} 
						dest.setPhysical(new_dest_physical) ;
						instr->setFault() ;
						instr->setCorruptedPair(old_dest_physical, new_dest_physical);
					} 
				}
			}
		} else { // injection at the source
			src_dst_idx= (src_dst_idx&0x1);

			reg_id_t &src = instr->getSourceReg(src_dst_idx);

			if (src.getRtype() == RID_INT || src.getRtype()==RID_INT_GLOBAL) {
				if( !src.isZero() ) {
					half_t old_src_physical = src.getPhysical();
					half_t new_src_physical = m_fs->/*instr->getFaultStat()->*/injectFault/*INJECT_FAULT*/(old_src_physical);

					if( new_src_physical != old_src_physical ) {      
						if ((ROB_FAULT_DEBUG)) { // && instr->getPSEQ()->recovered)) {
							DEBUG_OUT("%d:",instr->getSequenceNumber());
							DEBUG_OUT("src %d changed to %d\n",old_src_physical,new_src_physical);
						}
						src.setPhysical(new_src_physical) ;
						instr->setFault() ;
					} 
				}
			}
		}
	}

	// return instruction
	return (m_window[m_last_scheduled]);
}

//***************************************************************************
dynamic_inst_t *iwindow_t::retireInstruction( void )
{
	// calculate the next retired slot
	uint32  nextslot      = iwin_increment(m_last_retired);
	dynamic_inst_t *dinst = m_window[nextslot];

	// check if this dynamic instruction is ready to retire...
	//#ifdef DEBUG_RETIRE
	// 
	//if (dinst) {
	//dinst->m_pseq->out_info("## retire check %d %d (%d)\n", m_last_retired,
	//dinst->isRetireReady(), (int) dinst );
	//dinst->printDetail();
	//}
	//#endif

	if (dinst && dinst->isRetireReady()) {
		m_last_retired = nextslot;
		m_window[m_last_retired] = NULL;
		// mark "d" as removed from the iwin, so lsq->verify doesn't get confused
		dinst->markEvent( EVENT_IWINDOW_REMOVED );
		return (dinst);
	}
	return (NULL);
}

//**************************************************************************
dynamic_inst_t *iwindow_t::getNextInline( void )
{
	// calculate the next retired slot
	uint32  nextslot      = iwin_increment(m_last_retired);
	dynamic_inst_t *dinst = m_window[nextslot];

	return dinst;

}

//**************************************************************************
bool   iwindow_t::isInWindow( uint32 testNum )
{
	bool result;

	result = rangeOverlap( m_last_retired, m_last_decoded, testNum );
	return ( result );
}

//**************************************************************************
void iwindow_t::squash( pseq_t* the_pseq, int32 last_good, int32 &num_decoded )
{
	// window index in the m_window
	uint32      windex;

	// check that last good is between the last_retired && the last_fetched
	if ( !rangeOverlap( m_last_retired, m_last_fetched, last_good ) ) {
		DEBUG_OUT( "squash: warning: last_good = %d. last_fetched = %d. last_retired = %d. [%0llx]\n",
				last_good, m_last_fetched, m_last_retired,
				the_pseq->getLocalCycle() );
	}

	if ( ( iwin_increment(last_good) == m_last_retired ) &&
			m_window[last_good] == NULL ) {
		DEBUG_OUT( "squash: warning: last_good = %d. last_retired = %d. [%0llx]\n",
				last_good, m_last_retired, the_pseq->getLocalCycle() );
	}

	uint32 stage_squash[dynamic_inst_t::MAX_INST_STAGE];
	for (uint32 i = 0; i < dynamic_inst_t::MAX_INST_STAGE; i++) {
		stage_squash[i] = 0;
	}
	windex = m_last_fetched;
	// squash all the fetched instructions (in reverse order)
	while (windex != m_last_decoded) {

		if (m_window[windex]) {
			// FetchSquash double checks this is OK to squash
			stage_squash[m_window[windex]->getStage()]++;
			m_window[windex]->FetchSquash();
			delete m_window[windex];
			m_window[windex] = NULL;
		} else {
			ERROR_OUT("error: iwindow: fetchsquash(): index %d is NULL\n", windex);
			SIM_HALT;
		}
		windex = iwin_decrement(windex);
	}

	// we squash instructions in the opposite order they were fetched,
	// the process of squashing restores the decode register map.
	windex = m_last_decoded;
	// squash all the decoded instructions (in reverse order)
	while (windex != (uint32) last_good) {

		// squash modifies the decode map to restore the original mapping
		if (m_window[windex]) {
			// if last_good is last_retired, windex may point to NULL entry
			stage_squash[m_window[windex]->getStage()]++;
			m_window[windex]->Squash();
			delete m_window[windex];
			m_window[windex] = NULL;
		} else {
			ERROR_OUT("error: iwindow: squash(): index %d is NULL\n", windex);
			ERROR_OUT("last_good %d m_last_retired %d m_last_decoded %d m_last_fetched %d\n", 
					  last_good,m_last_retired, m_last_decoded, m_last_fetched);
			SIM_HALT;
		}
		windex = iwin_decrement(windex);
	}

	// assess stage-based statistics on what was squashed
	for (uint32 i = 0; i < dynamic_inst_t::MAX_INST_STAGE; i++) {
		if (stage_squash[i] >= (uint32) IWINDOW_ROB_SIZE) {
			ERROR_OUT("lots of instructions (%d) squashed from stage: %s\n",
					stage_squash[i],
					dynamic_inst_t::printStage( (dynamic_inst_t::stage_t) i ));
			stage_squash[i] = IWINDOW_ROB_SIZE - 1;
		}
		STAT_INC( the_pseq->m_hist_squash_stage[i][stage_squash[i]] );
	}

	// look back through the in-flight instructions,
	// restoring the most recent "good" branch predictors state
	windex = last_good;
	while (windex != m_last_retired) {
		if (m_window[windex] == NULL) {
			ERROR_OUT("error: iwindow: inflight: index %d is NULL\n", windex);
			SIM_HALT;
		} else {
			if (m_window[windex]->getStaticInst()->getType() == DYN_CONTROL) {
				predictor_state_t good_spec_state = (static_cast<control_inst_t*> (m_window[windex]))->getPredictorState();
				the_pseq->setSpecBPS(good_spec_state);
				break;
			}
		}
		windex = iwin_decrement(windex);
	}
	if (windex == m_last_retired) {
		/* no inflight branch, restore from retired "architectural" state */
		the_pseq->setSpecBPS(*(the_pseq->getArchBPS()));
	}

	m_last_fetched   = last_good;
	m_last_decoded   = last_good;

	// if we squash in execute, we can flush the decode pipeline, with no trouble
	// check if logically (m_last_scheduled > last_good)
	if ( rangeOverlap( m_last_retired, m_last_scheduled, last_good ) ) {

		m_last_scheduled = last_good;
		num_decoded = 0;
		ASSERT( rangeSubtract( m_last_decoded, m_last_scheduled ) <= 0 );

	} else {

		// else, last_good instruction is in decode ... so we need to pass
		// the number of currently decoded instructions back...
		num_decoded = (uint32) rangeSubtract( m_last_decoded, m_last_scheduled );

	}

	if (num_decoded > 8) {
		SIM_HALT;
	}
}

//**************************************************************************
void  iwindow_t::print( void )
{
	DEBUG_OUT("|");
	for (uint32 i = 0; i < m_rob_size; i++) {
		if (m_window[i] != NULL) {
			DEBUG_OUT("X|");
		} else {
			DEBUG_OUT(" |");
		}
	}
	DEBUG_OUT("\n");

	DEBUG_OUT("|");
	for (uint32 i = 0; i < m_rob_size; i++) {
		int  overlap = 0;
		char output  = ' ';
		if ( i == m_last_retired ) {
			overlap++;
			output = 'R';
		}
		if ( i == m_last_scheduled ) {
			overlap++;
			output = 'E';
		}
		if ( i == m_last_decoded ) {
			overlap++;
			output = 'D';
		}
		if ( i == m_last_fetched ) {
			overlap++;
			output = 'F';
		}
		if (overlap > 1) {
			DEBUG_OUT("%.1d|", overlap);
		} else {
			DEBUG_OUT("%c|", output);
		}
	}
	DEBUG_OUT("\n");
}

//***************************************************************************
void  iwindow_t::printDetail( void )
{
	DEBUG_OUT("last fetched:    %d\n", m_last_fetched);
	DEBUG_OUT("last decoded:    %d\n", m_last_decoded);
	DEBUG_OUT("last scheduled:  %d\n", m_last_scheduled);
	DEBUG_OUT("last retired:    %d\n", m_last_retired);
	uint32  loc = iwin_increment(m_last_retired);
	while (loc != m_last_retired) {
		if (m_window[loc] != NULL) {
			DEBUG_OUT("window: %d\n", loc);
			m_window[loc]->printDetail();
		}
		loc = iwin_increment(loc);
	}

	if (m_window[m_last_retired] != NULL) {
		DEBUG_OUT("window: %d\n", m_last_retired);
		m_window[m_last_retired]->printDetail();
	}
}

//***************************************************************************
dynamic_inst_t *iwindow_t::peekWindow( int &index )
{
	// return the current dynamic instr
	dynamic_inst_t *d = m_window[index];

	// calculate the next slot in the window (wrap around)
	index = iwin_increment(index);
	return (d);
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

//***************************************************************************
uint32  iwindow_t::getLastFetched( void )
{
	return (m_last_fetched);
}

//***************************************************************************
uint32  iwindow_t::getLastDecoded( void )
{
	return (m_last_decoded);
}

//***************************************************************************
uint32  iwindow_t::getLastScheduled( void )
{
	return (m_last_scheduled);
}

//***************************************************************************
uint32  iwindow_t::getLastRetired( void )
{
	return (m_last_retired);
}

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//***************************************************************************
bool iwindow_t::rangeOverlap( uint32 lowerNum, uint32 upperNum, uint32 testNum )
{
	bool result;
	if ( lowerNum <= upperNum ) {
		// regular check
		result = ( (lowerNum <= testNum) && (testNum <= upperNum) );
	} else {
		// wrapped check
		result = !( (upperNum < testNum) && (testNum < lowerNum) );
	}
	return (result);
}

//***************************************************************************
int32 iwindow_t::rangeSubtract( uint32 index1, uint32 index2 )
{
	// offset of index 1 relative to the last_retired instruction
	uint32  offseti1;

	if ( m_last_retired <= index1 ) {
		offseti1 = index1 - m_last_retired;
	} else {
		offseti1 = (m_rob_size + index1) - m_last_retired;
	}

	// offset of index 2 relative to the last_retired instruction
	uint32  offseti2;

	if ( m_last_retired <= index2 ) {
		offseti2 = index2 - m_last_retired;
	} else {
		offseti2 = (m_rob_size + index2) - m_last_retired;
	}

	return ( offseti1 - offseti2 );
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

