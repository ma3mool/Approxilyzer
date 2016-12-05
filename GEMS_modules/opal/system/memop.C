
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
 * FileName:  memop.C
 * Synopsis:  dynamic memory operations (loads / stores)
 * Author:    zilles
 * Version:   $Id: memop.C 1.87 06/02/13 18:49:19-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"

#include "scheduler.h"
#include "iwindow.h"
#include "lsq.h"
#include "pipestate.h"
#include "rubycache.h"
#include "memtrace.h"
#include "memop.h"

#ifdef LL_AGEN
//structuralModule design;
#endif // LL_AGEN
extern faultSimulator fSim;

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/
#define ALEX_DEBUG_MEMOP 0
#define ALEX_DEBUG_VALUE_BEFORE_SAVE 0
#define LXL_STORE_DEBUG 0
#define SIVA_LOAD_BUFFER_DEBUG 0
/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// memory allocator for memop.C
listalloc_t load_inst_t::m_myalloc;
listalloc_t store_inst_t::m_myalloc;
listalloc_t atomic_inst_t::m_myalloc;
listalloc_t prefetch_inst_t::m_myalloc;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* memory_inst                                                            */
/*------------------------------------------------------------------------*/

//***************************************************************************
memory_inst_t::memory_inst_t( static_inst_t *s_inst, 
        int32 window_index,
        pseq_t *pseq,
        abstract_pc_t *at, fault_stats *fault_stat )
: dynamic_inst_t( s_inst, window_index, pseq, at, fault_stat ) {

    has_mem_fault = false ;
    should_inject_fault = false ;
    // use default dynamic inst constructor
    lsq_prev = NULL;
    lsq_next = NULL;
    m_address       = (my_addr_t) -1;
    m_old_address   = (my_addr_t) -1;
    m_physical_addr = (pa_t) -1;
    m_old_physical_addr = (pa_t) -1;
    setAccessSize( s->getAccessSize() );
 //  DEBUG_OUT("%d: constructor: access size set =%d\n", pseq->getID(), m_access_size);
    m_data_valid    = false;
    m_old_data_valid = false;
    m_asi    = (uint16) -1;
    m_tl     = at->tl;
    m_pstate = at->pstate;
    m_check_at_retire = false;
    m_check_at_retire_value = -1;
    for( int i=0; i<MEMOP_MAX_SIZE ; i++ ) {
        m_data_storage[i] = (ireg_t) -1 ;
        m_old_data_storage[i] = (ireg_t) -1 ;
    }
}

//***************************************************************************
memory_inst_t::~memory_inst_t() {

}

//**************************************************************************
void
memory_inst_t::Squash() {
    ASSERT( !getEvent( EVENT_FINALIZED ) );
    ASSERT(m_stage != RETIRE_STAGE);
    if (getEvent(EVENT_LSQ_INSERT)) { 
        m_pseq->getLSQ()->remove( this );
    }
    if (getEvent(EVENT_MSHR_STALL)) { 
        m_pseq->getRubyCache()->squashInstruction( this );
    }

#ifdef EXPENSIVE_ASSERTIONS
    m_pseq->getLSQ()->verifyQueues();
#endif
    if (Waiting()) { RemoveWaitQueue(); }
    UnwindRegisters();
    m_pseq->decrementSequenceNumber();

    markEvent( EVENT_FINALIZED );
#ifdef PIPELINE_VIS
    m_pseq->out_log("squash %d\n", getWindowIndex());
#endif
}

//**************************************************************************
    void
memory_inst_t::Retire( abstract_pc_t *a )
{
    ASSERT( !getEvent( EVENT_FINALIZED ) );

	// printRetireTrace("memop: ") ;
	// static_inst_t *orig = getStaticInst()->getOriginalInstr() ;
	// if(orig) {
	// 	DEBUG_OUT("Original memop = %s\n", decode_opcode( (enum i_opcode) orig->getOpcode())) ;
	// }

#ifdef DEBUG_RETIRE
    m_pseq->out_log("## Memory Retire Stage\n");
    printDetail();
    m_pseq->out_log("   asi   : 0x%0x\n", m_asi );
    m_pseq->out_log("   pstate: 0x%0x\n", m_pstate );
    m_pseq->out_log("   tl    : 0x%0x\n", m_tl );
#endif

#ifdef LL_AGEN
	if( (m_fs->getFaultType() == AGEN_FAULT) &&
		(m_agen_id == m_fs->getFaultyReg()) &&
		(infectedAGEN) ) {
		if(m_old_address != m_address) {
			fSim.numBitFlipsRetire(m_old_address,m_address) ;
		}
		fSim.useBadALU++ ;
	}
#endif

    // record when execution takes place
    m_event_times[EVENT_TIME_RETIRE] = m_pseq->getLocalCycle() - m_fetch_cycle;

    // unstall fetch if this instruction is has in-order pipeline semantics
    if (s->getFlag( SI_FETCH_BARRIER ) ) {
        m_pseq->unstallFetch();
    }
    // This is related to the stxa instruction special case
    //      "stxa" instructions can be writing to the MMU to map or demap pages.
    //      They can't be executed in a pipelined manner-- if one executes past
    //      them (and speculates on "retry"), instructions that caused TLB misses
    //      will miss in the TLB again, instead of succeeding. By creating
    //      a dependency on the control registers, we block subsequent
    //      instructions from executing before the stxa is completed.
    //
    //      Here is where the stxa completes, hence the release of the
    //      control dependence.
    if ( getDestReg(1).getRtype() == RID_CONTROL ) {
        if (!getDestReg(1).getARF()->isReady( getDestReg(1) )) {
            getDestReg(1).getARF()->initializeControl( getDestReg(1) );
            getDestReg(1).getARF()->finalizeControl( getDestReg(1) );
        }
    }

    if (getEvent(EVENT_LSQ_INSERT)) { 
        m_pseq->getLSQ()->remove( this );
    }
#ifdef EXPENSIVE_ASSERTIONS
    m_pseq->getLSQ()->verifyQueues();
#endif
    retireRegisters();
    SetStage(RETIRE_STAGE);
    nextPC_execute( a );
    markEvent( EVENT_FINALIZED );

	// Track the ranges of addresses followed by the mem instructions
#ifdef RANGE_CHECK
	bool valid_access = m_fs->rangeCheck(getVPC(), getAddress()) ;
	if(!valid_access) {
		DEBUG_OUT("0x%llx access 0x%llx out of range!\n",getVPC(),getAddress());
		getSequencer()->outSymptomInfo(
				(uint64) m_pseq->getLocalCycle(),
				getSequencer()->getRetInst(),
				"Range_check_violation",
				0x206, getVPC(),
				getPrivilegeMode(),0,0,0) ;
		HALT_SIMULATION ;
	}
#endif // RANGE_CHECK

//TEMPORARY  FIX - remove this after 8/25/2011 
// if you dont remember what this is for, then just delete this.
	// if(!getPrivilegeMode() && !debugio_t::isValidObject(getAddress()) ) {
	// 	printRetireTrace("Memop No valid object: ") ;
	// 	int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
	// 	bool curr_priv = ((a->pstate & 0x4) == 0x4);
	// 	getSequencer()->outSymptomInfo(
	// 			(uint64) m_pseq->getLocalCycle(),
	// 			getSequencer()->getRetInst(),
	// 			"Range_check_violation",
	// 			0x206, getVPC(),
	// 			getPrivilegeMode(),0,0,0) ;
	// 	HALT_SIMULATION ;
	// }
	// for FFT


	// Do hard-bound mem check
#ifdef HARD_BOUND_BASE
	if(!getPrivilegeMode() && !debugio_t::isValidObject(getAddress()) ) {
		printRetireTrace("Memop No valid object: ") ;
		int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
		bool curr_priv = ((a->pstate & 0x4) == 0x4);
		getSequencer()->outSymptomInfo(
				(uint64) m_pseq->getLocalCycle(),
				getSequencer()->getRetInst(),
				"Range_check_violation",
				0x206, getVPC(),
				getPrivilegeMode(),0,0,0) ;
#ifdef LXL_SNET_RECOVERY
		m_pseq->symptomDetected(0x206);
#else // LXL_SNET_RECOVERY
		HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
	}
#endif // HARD_BOUND_BASE
	m_fs->buildInstRange(getPrivilegeMode(),getVPC(),getAddress()) ;

#ifdef BUILD_OBJECT_TABLE
	if(!getPrivilegeMode()) {
		ireg_t base = getSourceReg(0).getARF()->readInt64(getSourceReg(0)) ;
		ireg_t offset ;
		if(s->getFlag(SI_ISIMMEDIATE)) {
			offset = s->getImmediate() ;
		} else {
			offset = getSourceReg(1).getARF()->readInt64(getSourceReg(1)) ;
		}
		m_fs->buildObjectTable(base,offset) ;
	}
#endif // BUILD_OBJECT_TABLE

#ifdef FIND_MIN_STACK_ADDR
	// DEBUG_OUT("Recording 0x%llx\n", getAddress()) ;
	if(!getPrivilegeMode()) {
		debugio_t::recordMemAddr(getAddress()) ;
		if(getAddress() > 0xffc00000) {
			DEBUG_OUT("0x%llx\t0x%llx\n", getVPC(), getAddress()) ;
		}
	}
#endif // FIND_MIN_STACK_ADDR
}

//**************************************************************************
void
memory_inst_t::Execute( void )
{

    // record when execution takes place
    m_event_times[EVENT_TIME_EXECUTE_DONE] = m_pseq->getLocalCycle() - m_fetch_cycle;
    // LSQ_DATA_FAULT - Corrupt the m_data_storage[0] if the LSQ has a fault
    if( (getFaultStat()->getFaultType()==LSQ_DATA_FAULT) &&
            getInjectFault() ) {
        ireg_t new_data = getFaultStat()->injectFault( m_data_storage[0] ) ;
        if( new_data != m_data_storage[0] ) {
            if( PRADEEP_DEBUG_LSQ ) {
                printRetireTrace("Data fault ") ;
                DEBUG_OUT( "%llx->", m_data_storage[0] ) ;
                DEBUG_OUT( "%llx  ", new_data ) ;
                DEBUG_OUT( "@address %llx\n", m_address ) ;
            }
            if( PRADEEP_DEBUG_LSQ ) {
                DEBUG_OUT( "%d <- faulty from LSQ\n", getSequenceNumber() ) ;
            }
            setFault() ;
            setMemFault() ;
            m_data_storage[0] = new_data ;
        } 
    }

    // call the appropriate function
    static_inst_t *si = getStaticInst();
    // execute the instruction using the jump table
    pmf_dynamicExecute pmf = m_jump_table[si->getOpcode()];

    bool inject_me = false;
	bool write_to_int_rf=false;

	if ((s->getType()==DYN_LOAD)&&!m_dest_reg[0].isZero()) {
		write_to_int_rf = (m_dest_reg[0].getARF()->getARFType()==INT_ARF);
		if (write_to_int_rf) {
			m_result_bus_id = m_pseq->fetchIncWayCounter();
		} else {
			m_result_bus_id = m_pseq->fetchIncFWayCounter(); //FIXME: floating point data line
		}
	}

    if ((getFaultStat()->getFaultType() == REG_DATA_LINE_FAULT) && 
		(getFaultStat()->getFaultyReg() == m_result_bus_id) &&
        (s->getType()==DYN_LOAD) &&
        (!m_dest_reg[0].isZero()) ){
        inject_me = write_to_int_rf;
    }

    if (inject_me) {
        reg_id_t &dest=getDestReg(0);
        if (!dest.isZero()) {
            dest.getARF()->setRFWakeup(false);
        }
    }

    (this->*pmf)();

	// The other thing to check if is the dest ready. In the case of 
	// func trap, the dest value is not yet set (even in the case of
	// cache miss). Then, we should wait till the dests become ready before
	// injecting the fault and waking up deps.
	bool dest_ready = false ;
	for( int i=0; i< SI_MAX_DEST; i++ ) {
		reg_id_t &dest = getDestReg(i) ;
		if( !dest.isZero() ) {
			dest_ready |= dest.getARF()->getReady(dest) ;
		}
	}
	// If dest is not ready, unset inject me, and undo damange with the 
	// setRFWakeup function()
    if (!dest_ready) {
		inject_me = false ;
        reg_id_t &dest=getDestReg(0);
        if (!dest.isZero()) {
            dest.getARF()->setRFWakeup(true);
        }
    }

    if( !inject_me ) { 
        for( int i=0; i< SI_MAX_DEST ; i++ ) { //Changed from MAX_DST to 1
            reg_id_t &dest = getDestReg(i) ;
            if( !dest.isZero() ) { // Inject only if the destination is a valid register
				dest.getARF()->unSetCorrupted(dest) ;
				dest.getARF()->clearFault(dest);
				dest.getARF()->clearReadFault(dest);
			}
		}
    } else {
	    // Inject a fault in the output register
	    for( int i=0; i< SI_MAX_DEST; i++ ) { // Changed Max_dst to 1
		    reg_id_t &dest = getDestReg(i) ;
		    if( !dest.isZero() ) { // Inject only if the destination is a valid register
			    if (i==0) {
				    ireg_t result = dest.getARF()->readInt64( dest ) ;
				    getFaultStat()->setCurrInst( getSequenceNumber() ) ;
				    ireg_t inj_result = getFaultStat()->injectFault(result);
				    dest.getARF()->writeInt64( dest, inj_result ) ;

				    // If there is a fault, mark this instruction and the physical reg as faulty
				    if( result != inj_result ) {
					    //DEBUG_OUT("%llu gonna inject rbus %d isload %d\t",seq_num,m_result_bus_id,s->getType()==DYN_LOAD);
					    // 						DEBUG_OUT("%llu ",seq_num);
					    // 						DEBUG_OUT("PC:%llx\t%s\t%llx\t%llx\n",getVPC(),decode_opcode(s->getOpcode()),result,inj_result);

					    this->setFault() ;
					    dest.getARF()->writeInt64( dest, inj_result ) ;
					    dest.getARF()->setFault(dest);
					    if(INJECTED_INST) {
						    DEBUG_OUT("0x%llx\t%d\n",
								    getVPC(), getPrivilegeMode()) ;
					    }
					    setActivateFault() ;
				    } else {
					    dest.getARF()->unSetCorrupted(dest) ;
					    dest.getARF()->clearFault(dest);
					    dest.getARF()->clearReadFault(dest);
				    }
			    }
			    dest.getARF()->setRFWakeup(true);
			    dest.getARF()->wakeDependents(dest);
		    }
	    }
    }

    // check that this instruction has written its destination registers
    if ( getTrapType() == Trap_NoTrap ) {
        ASSERT( getDestReg(0).getARF()->isReady( getDestReg(0) ));

        // stxa's write the control registers at retire time (when they become 
        // non-speculative). So they need a conditional check.
        // See memory_inst_t::Retire() for more information (9/12/2002)
        if (getStaticInst()->getOpcode() != i_stxa) {
            ASSERT( getDestReg(1).getARF()->isReady( getDestReg(1) ));
        }
    }

    propagateFault();

    SetStage(COMPLETE_STAGE);

}

//**************************************************************************
    trap_type_t
memory_inst_t::addressGenerate( OpalMemop_t accessType )
{ 
    //
    // calculate the virtual address being accessed
    //
#ifdef LL_AGEN

	la_t v_address ;
	uint64_t v_op1 = getSourceReg(0).getARF()->readInt64(getSourceReg(0)) ;
	uint64_t v_s1 = getSourceReg(1).getARF()->readInt64(getSourceReg(1)) ;
	uint64_t v_imm = s->getImmediate() ;
	int v_use_imm = s->getFlag(SI_ISIMMEDIATE) ;

	uint64_t orig_address = (v_use_imm) ? (v_op1+v_imm) : (v_op1+v_s1) ;

	char v_address_vec[REG_WIDTH] ;
	char v_op1_vec[REG_WIDTH] ;
	char v_s1_vec[REG_WIDTH] ;
	char v_imm_vec[REG_WIDTH] ;
	char *ptr ; // Need by strtoull

	m_agen_id = m_pseq->fetchIncAgenCounter();

#ifdef RESOURCE_RR_AGEN
	int next_agen = m_pseq->getNextAgenId() ;
	// DEBUG_OUT("Opal: %d RR: %d\n", m_agen_id, next_agen) ;
	m_agen_id = next_agen ;
#endif // RESOURCE_RR_AGEN
	fSim.lluToBinStr(v_op1_vec, v_op1, REG_WIDTH) ;
	fSim.lluToBinStr(v_s1_vec, v_s1, REG_WIDTH) ;
	fSim.lluToBinStr(v_imm_vec, v_imm, REG_WIDTH) ;

	if( (m_fs->getFaultType() != AGEN_FAULT) ||
		(m_agen_id != m_fs->getFaultyReg()) ||
		(!m_fs->getDoFaultInjection()) || // Only when we are supposed to inj
	 	(m_fs->getFaultInjInst() > m_fs->getRetInst())  ) { // only after preset inst
		v_address = orig_address ;
	} else {

		this->infectedAGEN = true ;
#if LXL_PROB_FM
		pattern_t fp = fSim.genPattern() ;
#if LXL_PROB_DIR_FM
		v_address = (orig_address&(~fp.mask))|fp.dir ;
#else
		v_address = orig_address^fp.mask ;
#endif

#else // LXL_PROB_FM - Then do vlog simulation
		if(fSim.counter == 0){
			char stimFIFO[NAME_SIZE];
			char responseFIFO[NAME_SIZE];
			string stim = m_fs->getStimPipe();
			string response = m_fs->getResponsePipe();

			strcpy(stimFIFO,stim.c_str());
			strcpy(responseFIFO,response.c_str());

			strcpy(fSim.writeFIFO,stimFIFO);
			strcpy(fSim.readFIFO,responseFIFO);

			fSim.fdWrite = open(fSim.writeFIFO, O_WRONLY|O_ASYNC);
			fSim.fdRead = open(fSim.readFIFO, O_RDWR|O_ASYNC);
		}
		fSim.counter++ ;

		if( DEBUG_LL_AGEN ) {
			DEBUG_OUT("ncsim - Inputs\n") ;
			DEBUG_OUT("%llx\n%llx\n%llx\n%d\n", v_op1, v_s1, v_imm, v_use_imm) ;
		}
		fSim.nwrite = write(fSim.fdWrite, &v_op1_vec, REG_WIDTH) ;
		fSim.nwrite = write(fSim.fdWrite, &v_s1_vec, REG_WIDTH) ;
		fSim.nwrite = write(fSim.fdWrite, &v_imm_vec, REG_WIDTH) ;
		fSim.nwrite = write(fSim.fdWrite, &v_use_imm, sizeof(int)) ;

		fSim.nread = read(fSim.fdRead, &v_address_vec, REG_WIDTH) ; // blocking read
		sanitizeAddr(v_address_vec, REG_WIDTH);
		v_address = strtoull(v_address_vec, &ptr, 2) ;
#endif // LXL_PROB_FM

		if( DEBUG_LL_AGEN ) {
			DEBUG_OUT("%llx\n%llx\n%llx\n%d\n", v_op1, v_s1, v_imm, v_use_imm) ;
			DEBUG_OUT("(orig,virt) = %llx - %llx\n\n", orig_address, v_address) ;
		}
		if( orig_address != v_address ) {
			if( DEBUG_LL_AGEN ) {
				// DEBUG_OUT("%llx\n%llx\n%llx\n%d\n", v_op1, v_s1, v_imm, v_use_imm) ;
				DEBUG_OUT("(orig,virt) = %llx - %llx\n\n", orig_address, v_address) ;
			}
			fSim.numBitFlips(orig_address,v_address);
			m_fs->incTotalInj();
			m_fs->injectSafFault();
			m_old_address = orig_address ;
			setFault() ;
			setMemFault() ;
		} else {
			m_fs->incTotalMask() ;
			clearFault() ;
			clearMemFault() ;

		}
	}
	m_address = v_address ;

#else // LL_AGEN
    if ( s->getFlag(SI_ISIMMEDIATE) ) {
        m_address = getSourceReg(0).getARF()->readInt64(getSourceReg(0)) + 
            s->getImmediate();
    } else {
        m_address = getSourceReg(0).getARF()->readInt64(getSourceReg(0)) + 
            getSourceReg(1).getARF()->readInt64(getSourceReg(1));
    }

	// DEBUG_OUT("%d\t0x%llx\n", getSequenceNumber(), m_address) ;
    m_agen_id = m_pseq->fetchIncAgenCounter();
#ifdef RESOURCE_RR_AGEN
	int next_agen = m_pseq->getNextAgenId() ;
	// DEBUG_OUT("Opal: %d RR: %d\n", m_agen_id, next_agen) ;
	m_agen_id = next_agen ;
#endif // RESOURCE_RR_AGEN

// we also do AGEN faults in ARCH_REGISTER_INJECTION. Inject fault when the AGEN
// is invoked by the specified instruction number

    if( getFaultStat()->getFaultType() == AGEN_FAULT ) {
#ifdef ARCH_REGISTER_INJECTION 
		//DEBUG_OUT("AGEN at %d:0x%llx; wanted %d\n", getSequenceNumber(), getVPC(), getFaultStat()->getFaultInjInst()); 
		if(getSequenceNumber()==getFaultStat()->getFaultInjInst()) {
#else // ARCH_REGISTER_INJECTION
	    if( m_agen_id == getFaultStat()->getFaultyReg() ) {
#endif // ARCH_REGISTER_INJECTION
		    my_addr_t new_address = getFaultStat()->injectFault( m_address ) ;
		    if( new_address != m_address ) {
			    // if( PRADEEP_DEBUG_AGEN ) {
				    DEBUG_OUT( "Address corrupted for %d ",
						    getSequenceNumber() ) ;
				    DEBUG_OUT( "(%s)-- ", 
						    decode_opcode( (enum i_opcode)
							    getStaticInst()->getOpcode() )
					     ) ;
				    DEBUG_OUT( "%llx -> ", m_address ) ;
				    DEBUG_OUT( "%llx\n", new_address ) ;
			    //} 
			    // Should store old virtual address somewhere and
			    // get mapping from the TLB and set the old physical address
			    m_old_address = m_address ;
			    setFault() ;
			    setMemFault() ;
			    m_address = new_address ;
				if(INJECTED_INST) {
					DEBUG_OUT("0x%llx\t%d\n",
							getVPC(), getPrivilegeMode()) ;
				}
				setActivateFault() ;
				DEBUG_OUT("AGEN: Injecting Transient fault\n"); 
				fflush(stdout);
				
		    }
		}
		// } to balance curly braces used at ARCH_REGISTER_INJECTION
    }
#endif // LL_AGEN

	// Ignoring last 8 bits of access, just like that!
	// getFaultStat()->addMemAccess((m_address&0xffffffffffffff00),getVPC()) ;
	// getFaultStat()->addMemAccess(getVPC(), (m_address&0xffffffffffffff00)) ;
	getFaultStat()->addMemAccess(m_address, getVPC()) ;
    // get the ASI for this memory operation
    setASI();

#ifdef SW_ROLLBACK
	if (m_pseq->isRecovering()) {
		m_asi=0x8c;
		return Trap_NoTrap;
	}
#endif

    // if a TLB access: return immediately
    //                : no corresponding physical address!
    if ( !isCacheable() ) {
        m_physical_addr = (my_addr_t) -1;
//         DEBUG_OUT("Address Generate: sn:%d type:%s asi:0x%x address:0x%0llx uncacheable\n",
//        getWindowIndex(), accessType?"ST":"LD", m_asi, m_address);
#ifdef DEBUG_LSQ
        m_pseq->out_log("Address Generate: index:%d type:%d asi:%d address:0x%0llx uncacheable\n",
                getWindowIndex(), accessType, m_asi, m_address);
#endif
        return Trap_NoTrap;
    }

//    DEBUG_OUT("%d:ADDRESS ALIGNMENT: 0x%0llx (%d) %d: %d: r%d=%llx r%d=%llx\n", m_pseq->getID(), m_address, m_access_size, getWindowIndex(), (m_address & (m_access_size - 1)), 
//			getSourceReg(0).getVanilla(), getSourceReg(0).getARF()->readInt64(getSourceReg(0)), 
//			getSourceReg(1).getVanilla(), getSourceReg(1).getARF()->readInt64(getSourceReg(1)));

    // check that this address is aligned with respect to the access size
    if ( (m_address & (m_access_size - 1)) != 0 ) {
#ifdef DEBUG_LSQ
        DEBUG_OUT("%d:ADDRESS NOT ALIGNED: 0x%0llx (%d) %d\n", m_pseq->getID(), m_address, m_access_size, getWindowIndex());
        if (s->getFlag(SI_ISIMMEDIATE)) {
            DEBUG_OUT("%d:ADDRESS: 0x%0llx 0x%0llx\n", m_pseq->getID(), getSourceReg(0).getARF()->readInt64(getSourceReg(0)), s->getImmediate());
        } else {
            DEBUG_OUT("%d:ADDRESS: 0x%0llx 0x%0llx\n", m_pseq->getID(), getSourceReg(0).getARF()->readInt64(getSourceReg(0)), getSourceReg(1).getARF()->readInt64(getSourceReg(1)) );
        }
#endif
        return Trap_Mem_Address_Not_Aligned;
    }


    /* Access the TLB */
    //**************************************************************************
    trap_type_t t = Trap_NoTrap;
    if ( CONFIG_IN_SIMICS ) {
        // access the mmu
        t = m_pseq->mmuAccess( m_address, m_asi, m_access_size, accessType,
                m_pstate, m_tl, &m_physical_addr );

        if( m_old_address != (my_addr_t)-1 ) {
            trap_type_t temp_t = m_pseq->mmuAccess(
                    m_old_address, m_asi, m_access_size, accessType,
                m_pstate, m_tl, &m_old_physical_addr );
        }
    } else {
        // if in stand alone mode: scan the transaction log for a hit
        memtrace_t *mt = m_pseq->getMemtrace();
        mt->scanTransaction( m_pseq->getRetiredICount() + 2,
                m_address, getVPC(),
                s->getInst(), m_physical_addr,
                m_access_size, NULL );
        if (m_physical_addr == (pa_t) -1) {
            return Trap_Data_Access_Mmu_Miss;
        }
    }

#ifdef ARCH_REGISTER_INJECTION 
	//DEBUG_OUT("AGEN at %d:0x%llx; wanted %d\n", getSequenceNumber(), getVPC(), getFaultStat()->getFaultInjInst()); 
 	if(getSequenceNumber()==getFaultStat()->getFaultInjInst()) {
 
  		// access tlb and get physical address for the correct address
  		pa_t m_new_physical_addr;

    	trap_type_t t = Trap_NoTrap;
  	    if ( CONFIG_IN_SIMICS ) {
  	        // access the mmu
  	        t = m_pseq->mmuAccess( m_old_address, m_asi, m_access_size, accessType,
  	                m_pstate, m_tl, &m_new_physical_addr );
  	
  	        if( m_old_address != (my_addr_t)-1 ) {
  	            trap_type_t temp_t = m_pseq->mmuAccess(
  	                    m_old_address, m_asi, m_access_size, accessType,
  	                m_pstate, m_tl, &m_old_physical_addr );
  	        }
  	    } else {
  	        // if in stand alone mode: scan the transaction log for a hit
  	        memtrace_t *mt = m_pseq->getMemtrace();
  	        mt->scanTransaction( m_pseq->getRetiredICount() + 2,
  	                m_old_address, getVPC(),
  	                s->getInst(), m_new_physical_addr,
  	                m_access_size, NULL );
  	        if (m_new_physical_addr == (pa_t) -1) {
   	        	DEBUG_OUT("Trap_Data_Access_Mmu_Miss for old address\n");
  	        }
  	    }

  	 	ireg_t old_value, new_value;
  		if(m_physical_addr != 0xffffffffffffffff ) {
  	 		m_pseq->readPhysicalMemory(m_physical_addr,
  	 					(int) m_access_size, &new_value) ;

  			double* n_val = (double*)&new_value;
 			// for single-precision
  			// uint32 new_val32 = new_value;
  			// float* n_val = (float*)&new_val32;
  	 		DEBUG_OUT("Faulty m_address=%llx, physical_addr=%llx, val=%llx (%.15le)\n", m_address, m_physical_addr, new_value, *n_val);
		}

  		if(m_new_physical_addr != 0xffffffffffffffff) {
  	 		// access memory for the 2 physical addresses and print their values
  	 		m_pseq->readPhysicalMemory(m_new_physical_addr,
  	 					(int) m_access_size, &old_value) ;
  	 
  			
 			// for double-precision
  			double* o_val = (double*)&old_value;
 			// for single-precision
  			// uint32 old_val32 = old_value;
  			// float* o_val = (float*)&old_val32;
  	 		DEBUG_OUT("Faulty m_address=%llx \n", m_address);
  	 		DEBUG_OUT("Correct m_address=%llx, physical_addr=%llx, val=%llx (%.15le)\n", m_old_address, m_new_physical_addr, old_value, *o_val);
			double source_reg ;
 			uint64 source_reg_val_raw ;
 
			// Test 1: dont corrupt the faulty address; just dont write the correct value in correct address.
			// How: Hard
			// Test 2: write the correct value in correct address and see if the SDC is caused by the corruption in the faulty address.

 			// for( int i = 0; i < SI_MAX_SOURCE; i++ ) {
 			// 	reg_id_t &reg = getSourceReg(i) ;
 			// 	if( !reg.isZero() ) {
 			// 		// DEBUG_OUT( "p%d ",reg.getPhysical());
 			// 		// DEBUG_OUT( "r%d", reg.getFlatRegister()) ;
 			// 		// if(MEM_PROPAGATION) {
 			// 		// 	DEBUG_OUT( "%s", (reg.getARF()->isCorrupted(reg))?"^":" ") ;
 			// 		// } else {
 			// 		// 	DEBUG_OUT( "%s", (reg.getARF()->getFault(reg))?"*":" ") ;
 			// 		// }
 			// 		// DEBUG_OUT( "%s", (reg.getARF()->getReadFault(reg))?"*":" ") ;
 			// 		// DEBUG_OUT( "(r%d)", reg.getVanilla()) ;
 			// 		// DEBUG_OUT( "(r%d)", reg.getFlatRegister()) ;
 			// 		if(!NO_VALUES) {
 			// 			if (reg.getPhysical() == PSEQ_INVALID_REG) {
 			// 				DEBUG_OUT( "(vND),");
 			// 			} else {
 			// 				DEBUG_OUT( "Source Reg: (v%llx),",reg.getARF()->readInt64(reg));
 			// 				uint64 reg_val_raw = reg.getARF()->readInt64(reg);
 			// 				double *reg_val = (double*)&reg_val_raw;
 			// 				DEBUG_OUT( "(v%.15le),",*reg_val);
			// 				source_reg = *reg_val;
 			// 			}
 			// 		}
 			// 	}
 			// }
 			//DEBUG_OUT("\n");

 			// Test 1: dont corrupt the faulty address; just dont write the correct value in correct address.
			// How: Hard
			// getFaultStat()->setFixAddr(m_physical_addr);
			// getFaultStat()->setFixVal(new_value);
			// getFaultStat()->setFixAccessSize(m_access_size);

			// Test 2: write the correct value in correct address and see if the SDC is caused by the corruption in the faulty address.
  	 		// m_pseq->writePhysicalMemory(m_new_physical_addr, // correct address
  	 		// 			(int) m_access_size, &source_reg_val_raw) ; // correct value
 
  			// FILE * hFile;
  			// hFile = fopen("/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer_test/perturbations.txt", "a");
  			// if(hFile!= NULL)  {
  	 		// 	fprintf(hFile,"%llx\t", new_value);
  	 		// 	fprintf(hFile,"%.15le\t", *n_val);
  	 		// 	fprintf(hFile,"%llx\t", old_value);
  	 		// 	fprintf(hFile,"%.15le\t", *o_val);
  	 		// 	fprintf(hFile,"%.15le\n", source_reg);
  			// }
  			// fclose(hFile);
  		}
 	}
#endif // ARCH_REGISTER_INJECTION



#ifdef DEBUG_LSQ
    m_pseq->out_log("Address Generate: index:%d type:%d v:0x%0llx p:0x%0llx 0x%0llx size=%d pstate=0x%0x asi=0x%0x result=0x%0x\n",
            getWindowIndex(), accessType, m_address, m_physical_addr,
            (m_physical_addr >> DL1_BLOCK_BITS), m_access_size,
            m_pstate, m_asi, (int) t);
#endif
    return t;
}

//**************************************************************************
    trap_type_t
memory_inst_t::mmuRegisterAccess( bool isaWrite )
{
    mmu_interface_t *asi_interface = m_pseq->getASIinterface();
    memory_transaction_t mem_op;
    exception_type_t e = Sim_PE_No_Exception;

    // sanity check the mmu address. All MMU registers are < 0x40
    // (possibly < 0x1F8)
    if ( m_address > 0x1F8 ) {
        return ( Trap_Unimplemented );
    }
    if ( asi_interface == NULL ) {
        return ( Trap_Unimplemented );
    }

    mem_op.s.logical_address = m_address;
    //        we presume loads and stores are only accessing data, not the
    //        instruction stream.
    mem_op.s.size = m_access_size;
    //        loads use read, and stores use Write here
    if (isaWrite) {
        mem_op.s.type = Sim_Trans_Store;
    } else {
        mem_op.s.type = Sim_Trans_Load;
    }

    conf_object_t *simics_cpu = system_t::inst->m_state[m_pseq->getID()]->getSimicsProcessor();
    conf_object_t *simics_mmu = system_t::inst->m_state[m_pseq->getID()]->getSimicsMMU();
    mem_op.s.inquiry        = 1;
    mem_op.s.inverse_endian = 0;
    mem_op.s.ini_type    = Sim_Initiator_CPU;
    mem_op.s.ini_ptr     = simics_cpu;
    mem_op.address_space = m_asi;

    /*
     * Task: get the current pstate bits
     */
    mem_op.priv = (m_pstate >> 2) & 0x1;

    if ( CONFIG_IN_SIMICS ) {

        // access the mmu register
        // DEBUG_OUT("asi %d ", m_asi);
        e = (*asi_interface->logical_to_physical)( simics_mmu, &mem_op );
        // DEBUG_OUT(" done %d\n", (int) e );

        // copy the memory op's data into our data structure
        if ( e == Sim_PE_No_Exception ) {
            m_data_storage[0] = mem_op.s.physical_address;
            m_data_valid = true;
            // dist13 should use this:: m_data_storage[0] = SIM_get_mem_op_value( &mem_op.s );
        }

    } else {

        // no access to the mmu: just return fail
        return (Trap_Unimplemented);
    }

    // check to see if simics is returning a real or pseudo exception...
    if ( (int) e < 1025 ) {
#ifdef PIPELINE_VIS
        m_pseq->out_log("mmuRegisterAccess: simics traps 0x%0x\n", (int) e);
#endif
        return ((trap_type_t) e);
    }

#ifdef PIPELINE_VIS
    if ( e != Sim_PE_No_Exception ) {
        m_pseq->out_log("warning: mmuRegisterAccess: simics exception 0x%0x\n",
                (int) e);
    }
#endif
    return Trap_NoTrap;
}

//***************************************************************************
    uint16
memory_inst_t::addressOverlap( memory_inst_t *other )
{
    uint16  isOverlap = 0;

    /* an access to the same cache line.  Check for overlap between accesses */
    my_addr_t  my_cacheline    = getLineAddress();
    my_addr_t  other_cacheline = other->getLineAddress();
    if (my_cacheline == other_cacheline) {

        int my_offset    = m_physical_addr & MEMOP_BLOCK_MASK;
        int other_offset = other->m_physical_addr & MEMOP_BLOCK_MASK;

#ifdef DEBUG_LSQ
        m_pseq->out_log("address overlap 0x%0llx ?= 0x%0llx\n",
                m_physical_addr, other->m_physical_addr );
        m_pseq->out_log("  -cacheline 0x%0llx offset 0x%0x\n",
                my_cacheline, my_offset);
        m_pseq->out_log("  -otherline 0x%0llx offset 0x%0x\n",
                other_cacheline, other_offset);
#endif
        // common case: access to same byte, word, etc.
        if (my_offset == other_offset) {

            if (m_access_size == other->m_access_size) {
                isOverlap = MEMOP_EXACT;
            } 
            else if( m_access_size < other->m_access_size){
                // LUKE - should be able to bypass if m_access_size < other->m_access_size
                isOverlap = MEMOP_EXACT;
            }
            else {
                // CM imprecise can do bypassing even when complete overlap,
                // not just partial overlap
#ifdef DEBUG_LSQ
                m_pseq->out_log("PO case 1:: sizes 0x%0x 0x%0x\n", m_access_size, other->m_access_size);
#endif
                isOverlap = MEMOP_OVERLAP;
            }

        } else if (my_offset < other_offset) {
            if (my_offset + m_access_size > other_offset) {
                // CM imprecise can do bypassing here too if m + size >= o + size
#ifdef DEBUG_LSQ
                m_pseq->out_log(".");
#endif
                isOverlap = MEMOP_OVERLAP;
            }
        } else {
            // (my_offset > other_offset)
#ifdef DEBUG_LSQ
            m_pseq->out_log("!");
#endif
            if (other_offset + other->m_access_size > my_offset) {
                isOverlap = MEMOP_OVERLAP;
            }
        }

    }
    return (isOverlap);
}

//***************************************************************************
void memory_inst_t::printDetail( void )
{
    dynamic_inst_t::printDetail();

    m_pseq->out_info("   access: %d\n", m_access_size );
    m_pseq->out_info("   addr  : 0x%0llx\n", m_address );
    m_pseq->out_info("   phys a: 0x%0llx\n", m_physical_addr );

    m_pseq->out_info("   pstate: 0x%x\n", m_pstate );
    m_pseq->out_info("   tl    : 0x%x\n", m_tl );
    m_pseq->out_info("   asi   : 0x%x\n", m_asi );
    m_pseq->out_info("   valid : %d\n", m_data_valid );
}

//**************************************************************************
void memory_inst_t::setASI( void )
{
    // if this instruction (may) affect an ASI, get the current %ASI register
    if ( s->getFlag( SI_ISASI ) ) {

        bool is_block_load = false;
        m_asi = memop_getASI( s->getInst(), getSourceReg(3), &is_block_load );
    if ( maskBits32( s->getInst(), 13, 13 ) == 1 ) {
        // uses the asi register

	//if(s->getOpcode() == i_casa) {
		//DEBUG_OUT(" setASI:m_asi = %d", m_asi);
		//DEBUG_OUT(" using reg: ");
		//DEBUG_OUT( "(r%d)", getSourceReg(3).getVanilla()) ;
		//DEBUG_OUT( "(f%d)", getSourceReg(3).getFlatRegister()) ;
		//DEBUG_OUT( "(s%d)", getSourceReg(3).getVanillaState()) ;
		//DEBUG_OUT( "(v%llx)\n",getSourceReg(3).getARF()->readInt64(getSourceReg(3))) ;
		m_check_at_retire = true;
		m_check_at_retire_value = getSourceReg(3).getARF()->readInt64(getSourceReg(3));
	//}
    } 
        if (s->getOpcode() == i_lddfa || s->getOpcode() == i_stdfa) {
            // if the asi is not a block load, and we've reserved 64 bytes,
            // the actual load or store size is 8 bytes
            if (!is_block_load) {
                if ( ((m_asi >> 4) & 0xf) == 0xd ) {
                    if ((m_asi & 0x2) == 0x2) {
                        // 16-bit load-store
                        setAccessSize( 2 );
                    } else {
                        // 8-bit load-store
                        setAccessSize( 1 );
                    }
                } else {
                    setAccessSize( 8 );
                }         

                // later we'll forward data from the 0th register using readInt64
                if (s->getOpcode() == i_stdfa) {
                    getSourceReg(2).setVanilla(0);
                }
            }
        }

    } else {

        int  inverse_endian = (m_pstate >> 9) & 0x1;
	
        if (!inverse_endian) {
            if ( m_tl == 0 ) {
                m_asi = ASI_PRIMARY;
            } else {
                m_asi = ASI_NUCLEUS;
            }
        } else {
            if ( m_tl == 0 ) {
                m_asi = ASI_PRIMARY_LITTLE;
            } else {
                m_asi = ASI_NUCLEUS_LITTLE;
            }
        }
    }

    // ASI reads to the MMU rely on the simics to get their values
    // ASI writes to the MMU must be retired ASAP (not bundled)
    if (PSEQ_MAX_UNCHECKED != 1) {
        if ( m_asi >= 0x50 && m_asi <= 0x5f ) {
            setTrapType( Trap_Use_Functional );
        }
    }
}

// Function to check if the memory address is aligned.
// Used when reassigning new address through agen/lsq fault
void memory_inst_t::checkAddressAlignment( )
{
    if ( (m_physical_addr & (m_access_size - 1)) != 0 ) {
        SetStage( COMPLETE_STAGE ) ;
        // ERROR_OUT( "Memory misaligned for %d\n", getSequenceNumber() ) ;
        setTrapType( Trap_Mem_Address_Not_Aligned ) ;
    }
}

bool memory_inst_t::isCacheable( void )
{
	//	if ( m_asi == (uint16) -1 || m_pseq->isRecovering()) SW_ROLLBACK
	if ( m_asi == (uint16) -1 ) {
		return false;
	}
	return ( pstate_t::is_cacheable( m_asi ) );
}



/*------------------------------------------------------------------------*/
/* load_inst                                                              */
/*------------------------------------------------------------------------*/

//***************************************************************************
load_inst_t::load_inst_t( static_inst_t *s_inst, 
        int32 window_index,
        pseq_t *pseq,
        abstract_pc_t *fetch_at, fault_stats* fault_stat )
: memory_inst_t( s_inst, window_index, pseq, fetch_at, fault_stat ) {

    // use default dynamic inst constructor
    m_value_pred = false;
    m_depend     = NULL;
    m_fetch_at   = *fetch_at;
	update_only = false;
	llb_read_pointer = 0;

    if(MULTICORE_DIAGNOSIS_ONLY) {
	    if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
		    //set the read pointer in the load instruciton
		    llb_read_pointer = m_pseq->getLLB()->getReadPointer();
		    //update the read pointer in the llb
		    //DEBUG_OUT("%d:%d ",m_pseq->getID(), getSequenceNumber());
		    //DEBUG_OUT("opcode=%s, read_pointer=%d, access_size=%d\n",decode_opcode(s->getOpcode()), m_pseq->getLLB()->getReadPointer(), getAccessSize());
		    int access_size = ((m_access_size+7)/8);
	    	    m_pseq->getLLB()->updateReadPointer(access_size) ;
		    //DEBUG_OUT("%d: read_pointer after update=%d\n",m_pseq->getID(), m_pseq->getLLB()->getReadPointer());

	    }
    }

}

//***************************************************************************
load_inst_t::~load_inst_t() {
}

//**************************************************************************
void 
load_inst_t::Execute() {

    checkDestReadiness();

	// First off, clear the faults of the destination registers.
	for( int i=0; i< SI_MAX_DEST ; i++ ) { //Changed from MAX_DST to 1
		reg_id_t &dest = getDestReg(i) ;
		if( !dest.isZero() ) { // Inject only if the destination is a valid register
			dest.getARF()->unSetCorrupted(dest) ;
			dest.getARF()->clearFault(dest);
			dest.getARF()->clearReadFault(dest);
		}
	}
    STAT_INC( m_pseq->m_stat_loads_exec );
    trap_type_t t = addressGenerate( OPAL_LOAD );
	propagateFault();
    if (t != Trap_NoTrap) {
        /* couldn't correctly generate address - fired off exception */
        SetStage( COMPLETE_STAGE );
        setTrapType( (trap_type_t) t );
        return;
    }
#ifdef SW_ROLLBACK
	if (m_pseq->m_op_mode==RECOVERING) {
		goto cont_exec;
	}
#endif

    /* can't complete I/O accesses, and moreover they are uncacheable */
    if ( m_physical_addr != (pa_t) -1 &&
            isIOAccess( m_physical_addr ) ) {
        SetStage( COMPLETE_STAGE );
        setTrapType( Trap_Use_Functional );
        STAT_INC( m_pseq->m_stat_count_io_access );
#ifdef DEBUG_LSQ
        m_pseq->out_log("   load I/O access: index: %d\n", getWindowIndex());
#endif
        return;
    }

    if ( isCacheable() ) {
        ASSERT( m_physical_addr != (pa_t) -1 );

        // insert this memory instruction into the load/store queue.
        if (m_pseq->getLSQ()->insert( this )) {
            /* we got a hit in the LSQ or some other event so we should stop
               handling it like a normal load */
#ifdef DEBUG_LSQ
            m_pseq->out_log("***** load LSQ exception cycle %lld\n",
                    m_pseq->getLocalCycle() );
#endif
            return;
        }
    } // end isCacheable()

 cont_exec:
    // if no hit in LSQ (for data bypassing or waiting until dependent store retires)
    //           move on to 2nd phase of execution
    continueExecution();
	
	// For decoder input faults, look at whether the original instruction was a store.
	// If so, record the old value at that location to later undo the store
	// TODO - Should make such a fix for other instructions too. What if the store gets
	// morphed into a non-load?
	if(getFaultStat()->getFaultType() == DECODER_INPUT_FAULT) {
		static_inst_t *original = getStaticInst()->getOriginalInstr() ;
		if(original) {
			if(original->getType() == DYN_STORE || original->getType() == DYN_ATOMIC) {
				if(getTrapType() != Trap_Mem_Address_Not_Aligned) {
					m_old_data_valid = m_pseq->readPhysicalMemory(m_physical_addr,
							(int) m_access_size, getOldData()) ;
					// DEBUG_OUT("[%d] [0x%llx] other data = 0x%llx\n", getSequenceNumber(), getVPC(), getOldData()) ;
				}
			}
		}
	}
}

//************************************************************************
    void
load_inst_t::continueExecution() 
{
    if ( isCacheable() ) {
        // access the cache
        if (!accessCache())
            return;
    } // is cacheable

    Complete();
}


//**************************************************************************
void 
load_inst_t::replay( void ) {
    m_pseq->raiseException( EXCEPT_MEMORY_REPLAY_TRAP,
            m_pseq->getIwindow()->iwin_decrement(getWindowIndex()),
            (enum i_opcode) s->getOpcode(),
            &m_fetch_at, 0 , /* penalty */ 0 );
}

//**************************************************************************
void
load_inst_t::lsqBypass( void ) {

    STAT_INC( m_pseq->m_stat_bypasses );
    ASSERT(m_depend->getStoreValueReady());

    // get the maximum amount of data that can be bypassed from the other
    // instruction.
    byte_t  min_access_size = MIN( m_access_size, m_depend->getAccessSize() );

    // convert the access size (in bytes) to register size (8 byte) quantities
    min_access_size = ((min_access_size + 7)/ 8);
    for ( int i = 0; i < min_access_size; i++ ) {
        m_data_storage[i] = m_depend->getData()[i];
    }
    //LXL: The lower address word is actually at the upper half
	if (m_depend->getAccessSize()>m_access_size) {
		if (m_depend->getAccessSize()==8) {
			m_data_storage[0] = m_data_storage[0] >> ((8-m_access_size)*8);
		} else if (m_depend->getAccessSize()==4) {
			m_data_storage[0] = m_data_storage[0] >> ((4-m_access_size)*8);
		} else if (m_depend->getAccessSize()==2) {
			m_data_storage[0] = m_data_storage[0] >> ((2-m_access_size)*8);
		}
    }

    m_data_valid = true;
    markEvent( EVENT_LSQ_BYPASS );

    /* the lsq copied the memory bits into the instruction; format the
       value and write it into a register */
    memory_inst_t::Execute(); 
}

//**************************************************************************
void
load_inst_t::lsqWait( void ) {
    STAT_INC( m_pseq->m_stat_early_store_bypass );
    SetStage( LSQ_WAIT_STAGE );

    /* put ourselves in the wait list for the store value register */
    m_depend->addDependentLoad( this );
}

//*************************************************************************
void
load_inst_t::addrOverlapWait(store_inst_t * depend){
    //wait until Store retires
    SetStage( ADDR_OVERLAP_STALL_STAGE );
    depend->addOverlapLoad(this);
}

//**************************************************************************
bool
load_inst_t::accessCache( void ) {

	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
			//pretend that its a hit
			return true;
		}
	}

    if (CONFIG_WITH_RUBY) {

#ifdef DEBUG_RUBY
        m_pseq->out_log("PC: 0x%0llx DATALOAD: 0x%0llx\n",
                getVPC(), m_physical_addr );
#endif
        bool          mshr_hit   = false;
        bool          mshr_stall = false;
        rubycache_t  *rcache = m_pseq->getRubyCache();
        ruby_status_t status = rcache->access( m_physical_addr, OPAL_LOAD,
                getVPC(), (m_pstate >> 2) & 0x1,
                this, mshr_hit, mshr_stall );
        if (mshr_hit == true) {
            markEvent( EVENT_MSHR_HIT );
        }
        if (mshr_stall == true) {
            markEvent(EVENT_MSHR_STALL );
        }
        if ( status == HIT ) {
            // do nothing
        } else if ( status == MISS ) {
            if ( MEMOP_STALE_DATA ) {
                // try to get stale data under the miss
                bool staleAvailable = rcache->staleDataRequest( m_physical_addr,
                        m_access_size,
                        m_predicted_storage );

                // current using oracle-like prediction for value prediction
                if (staleAvailable) {
                    // perform oracle memory access
#if 0
                    m_pseq->readPhysicalMemory( m_physical_addr,
                            (int) m_access_size,
                            getData() );
                    bool use_value_predictor = validateValuePrediction();
#endif
                    bool use_value_predictor = true;
                    if (use_value_predictor) {
                        // perform value speculation using the stale data
                        m_value_pred = true;
                        STAT_INC(m_pseq->m_stat_stale_predictions);

                        //   a) copy predicted into actual, set data to be valid
                        for (int32 i = 0; i < (m_access_size + 7)/8; i++) {
                            m_data_storage[i] = m_predicted_storage[i];
                        }
                        m_data_valid = true;

                        //   b) format the value and write it into a register 
                        memory_inst_t::Execute();
                    }
                }
            }
            markEvent( EVENT_DCACHE_MISS );
            STAT_INC(m_pseq->m_stat_num_dcache_miss);
            SetStage(CACHE_MISS_STAGE);

            return false;
        } else if ( status == NOT_READY ) {
            // NOT_READY cache status causes polling using event queue
#if 0
            m_pseq->out_log("NOTREADY: cycle:0x%lld PC: 0x%0llx DATALOAD: 0x%0llx\n",
                    m_pseq->getLocalCycle(),
                    getVPC(), m_physical_addr );
#endif
            m_pseq->postEvent( this, 1 );
            STAT_INC(m_pseq->m_num_cache_not_ready);
            SetStage(CACHE_NOTREADY_STAGE);
            return false;
        } else {
            SIM_HALT;
        }
    } else {
        /* we missed in the load/store queue, check to see if we hit in
         * the cache-- if so get the value from architected memory,
         * otherwise request the line from the next level cache.
         */
        cache_t *dcache = m_pseq->getDataCache();
        bool primary_bool = false;
        if ( !dcache->Read( m_physical_addr, this, true, &primary_bool )) {
            markEvent( EVENT_DCACHE_MISS );
            STAT_INC(m_pseq->m_stat_num_dcache_miss);
            SetStage(CACHE_MISS_STAGE);
            return false;
        }
    }
    return true;
}

//**************************************************************************
void
load_inst_t::Retire( abstract_pc_t *a ) {
	// update statistics
	STAT_INC( m_pseq->m_stat_loads_retired );

	if(getVPC() > 0x10000486c && getVPC() < 0x100004978) {	
		// if(getAddress() > 0x10312a068 && getAddress() != 0xffffffffffffffffULL)
		if(getAddress() != 0xffffffffffffffffULL) {
    		DEBUG_OUT ( "Mem\t" ) ;
    		DEBUG_OUT ( "pc:0x%0llx", getVPC() ) ;
    		DEBUG_OUT ( "(%d)\t", getPrivilegeMode() ) ;
    		DEBUG_OUT ( "Addr:0x%0llx\n", getAddress() ) ;
		}
	}


	if( ALEX_DEBUG_MEMOP && ALEX_SNET_DEBUG ) {
		DEBUG_OUT( "%d:", m_pseq->getID() ) ;
		DEBUG_OUT( "Ld - %d ", getVPC() ) ;
		DEBUG_OUT( "%d ", getSequenceNumber() ) ;
		DEBUG_OUT( "0x%x ", m_events ) ;
		DEBUG_OUT( "%d ", getPrivilegeMode() ) ;
		DEBUG_OUT( "Mem[%llx] ", m_physical_addr ) ;
		DEBUG_OUT( "= %llx\n", m_data_storage[0] ) ;
	}
#ifdef LLB_ANALYSIS
	m_pseq->num_values += m_access_size;
	m_pseq->find_and_add_in_directory(m_data_storage, m_access_size);
#endif

#ifdef LXL_MEM_REC_VERIFIER

#if MULTICORE_DIAGNOSIS_ONLY
	if (m_pseq->getMulticoreDiagnosis()->isLoggingPhase()) {
#else //MULTICORE_DIAGNOSIS_ONLY
	if (!m_pseq->recovered) {
#endif //MULTICORE_DIAGNOSIS_ONLY
		ireg_t tmp = m_data_storage[0];
		if (m_access_size < 8) {
			tmp = tmp&(0xffffffffffffffffULL>>(64-8*m_access_size));
		}
		m_pseq->addMemRec(getVPC(),true,m_physical_addr,tmp);

#if MULTICORE_DIAGNOSIS_ONLY
	} else if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
#else //MULTICORE_DIAGNOSIS_ONLY
	} else { 
#endif //MULTICORE_DIAGNOSIS_ONLY

			mem_rec_t &verify=m_pseq->m_mem_trace[m_pseq->mem_trace_cnt++];
			ireg_t tmp = m_data_storage[0];
			if (m_access_size < 8) {
				tmp = tmp&(0xffffffffffffffffULL>>(64-8*m_access_size));
			}

			if ( (verify.PC != getVPC()) ||
					!verify.isLoad ||
					(verify.address != m_physical_addr) ||
					(verify.data != tmp)) {
				DEBUG_OUT("ERROR: %d mismatch load!!\t", getSequenceNumber());
				DEBUG_OUT("PC %llx %llx \n", getVPC(),verify.PC);
				DEBUG_OUT("trace cnt %d \n", m_pseq->mem_trace_cnt);
				DEBUG_OUT("isLoad %d\n",verify.isLoad);
				DEBUG_OUT("addr %llx %llx \n", m_physical_addr,verify.address);
				DEBUG_OUT("data %llx %llx \n", tmp,verify.data);
				HALT_SIMULATION;
			}
		}
	}
#endif //LXL_MEM_REC_VERIFIER
	if (getMemFault()) {
		// 		DEBUG_OUT("%lld has mem fault\n",seq_num);
		// 		printDetail();
		getFaultStat()->setArchCycle() ;
		getFaultStat()->setMemCycle() ;
	}
	if( PRADEEP_LSQ_BADADDR ) {
		DEBUG_OUT("Ld%s ", getFault()?"*":" " ) ;
		DEBUG_OUT(" <- %llx\n", m_physical_addr ) ;
	}

	// Set that this load's dest is faulty is address is faulty
	if(debugio_t::isCorruptAddress(getAddress(),getPrivilegeMode())) {
		// DEBUG_OUT("ld: %llx corrupted\n", getAddress()) ;
		this->setCorrupted() ;
		// Record the type of instruction as faulty
		if(RECORD_FAULTY_INST) {
			m_fs->addFaultyInstruction(decode_opcode((enum i_opcode)s->getOpcode())) ;
		}
	} else {
		// DEBUG_OUT("ld: Not corrupted\n") ;
		// printRetireTrace() ;
	}
	// If instruction or source regs are corrupt, set dest reg corrupt
	bool corrupts_memory = isCorrupted() | false ;
	for (int i = 0; i < SI_MAX_SOURCE; i ++) {
		reg_id_t &reg = getSourceReg(i);
		if (!reg.isZero()) {
			corrupts_memory |= reg.getARF()->isCorrupted(reg) ;
		}
	}
	if(corrupts_memory) {
		for (int i = 0; i < SI_MAX_DEST; i ++) {
			reg_id_t &reg = getDestReg(i);
			if (!reg.isZero()) {
				reg.getARF()->setCorrupted(reg) ;
			}
		}
	}

	memory_inst_t::Retire(a);

	// For decoder input faults, look at whether the original instruction was a store.
	// If so, restore the old value at that location to undo the store
	if(getFaultStat()->getFaultType() == DECODER_INPUT_FAULT) {
		static_inst_t *original = getStaticInst()->getOriginalInstr() ;
		if(original) {
			if(original->getType() == DYN_STORE || original->getType() == DYN_ATOMIC) {
				if(m_old_data_valid && getTrapType() != Trap_Mem_Address_Not_Aligned) {
					// ireg_t  curr_data[MEMOP_MAX_SIZE];
					// bool curr_data_valid = m_pseq->readPhysicalMemory(m_physical_addr,
					// 		(int) m_access_size,
					// 		&curr_data[0] );
					m_pseq->writePhysicalMemory(m_physical_addr,
							(int) m_access_size,
							getOldData()) ;
					// DEBUG_OUT("[%d] [0x%llx] restored data: 0x%llx->0x%llx\n",
					// 		getSequenceNumber(), getVPC(), curr_data[0], getOldData()) ;
				}
			}
		}
	}

#if TRACK_MEM_SHARING
	if(debugio_t::isShared(m_physical_addr, getPrivilegeMode())) {
		uint32 faulty_core_id = debugio_t::getMappedCoreID(m_physical_addr,getPrivilegeMode());
		if(faulty_core_id == debugio_t::getFaultyCore() && getSequencer()->getID() != debugio_t::getFaultyCore()) {
			struct addr_info temp;
			temp.core_id = getSequencer()->getID();
			temp.rw = 0;
			temp.trap_level = m_tl;
			temp.pc = getVPC();

			debugio_t::add_shared_address(m_physical_addr, getPrivilegeMode(), temp);
			//debugio_t::out_info("%d:%d:%llx\n", getSequencer()->getID(), getPrivilegeMode(), m_physical_addr);
		}
	}
#endif // TRACK_MEM_SHARING

	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_traptype != Trap_Use_Functional) {
			if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
				m_pseq->getMulticoreDiagnosis()->reportLoadInstruction(m_pseq->getID());

				for (int i = 0; i < SI_MAX_DEST; i++) {
					reg_id_t &dest = getDestReg(i);
					if (!dest.isZero()) {
						if (dest.getPhysical() != PSEQ_INVALID_REG) {
							check_result_t result;
							result.update_only = false;
#ifdef LXL_SNET_RECOVERY
							result.recovered = false;
#endif
							result.patch_dest = true;
							//result.verbose = true;
							dest.getARF()->check(dest, system_t::inst->m_state[m_pseq->getID()], &result, false);
							//DEBUG_OUT("%d: check done\n", m_pseq->getID());
						}
					}
				}
			}
		}
	}

	// Loads seem way harder to predict than stores! Don't do for now.
	// Prefetch learning. Learn this address.
	// if(!getPrivilegeMode()) { // Don't do this for OS as its hard to predict
	// 	ireg_t prefetch_addr = m_fs->getPrefetch(getVPC(),m_address) ;
	// 	if(prefetch_addr!=0x0 && prefetch_addr!=m_address) {
	// 		DEBUG_OUT("Ld: 0x%llx: 0x%llx\t0x%llx\n",
	// 				getVPC(), prefetch_addr, m_address) ;
	// 	}
	// }
#ifdef HARD_BOUND
	// Do the memory tracking for the objects
	propagateObjects() ;
#endif // HARD_BOUND
	if(INST_MIX) {
		m_fs->recordInstMix("load",isCorrupted()) ;
	}
	return;
}

#ifdef HARD_BOUND
void load_inst_t::propagateObjects(int in_trap)
{
	ireg_t addr = getAddress() ;
	reg_id_t &src1 = getSourceReg(0) ;
	half_t src1_flat ;
	if(src1.getRtype()!=RID_INT && src1.getRtype()!=RID_INT_GLOBAL) {
		src1_flat = PSEQ_FLATREG_NWIN ;
	} else {
		src1_flat = src1.getFlatRegister() ;
	}

	reg_id_t &src2 = getSourceReg(1) ; 
	half_t src2_flat ;
	if(s->getFlag(SI_ISIMMEDIATE) || (src1.getRtype()!=RID_INT && src1.getRtype()!=RID_INT_GLOBAL)) {
		src2_flat = PSEQ_FLATREG_NWIN ;
	} else {
		src2_flat = src2.getFlatRegister() ;
		if(src1_flat !=PSEQ_FLATREG_NWIN) {
			// How to find out what the base is. Pass in src1 as the one with
			// the higher value. Perhaps that is the base
			if(src1.getARF()->readInt64(src1) < src1.getARF()->readInt64(src2)) {
				half_t temp = src1_flat ;
				src1_flat = src2_flat ;
				src2_flat = temp ;
			}
		}
	}

	reg_id_t &dest = getDestReg(0) ;
	half_t dest_flat ;
	if(dest.getRtype()!=RID_INT && dest.getRtype()!=RID_INT_GLOBAL) {
		dest_flat = PSEQ_FLATREG_NWIN ;
	} else {
		dest_flat = dest.getFlatRegister() ;
	}

	// Do the sophisticated hard bound checks
	if(debugio_t::handleObjectLoad(dest_flat, src1_flat, src2_flat, addr, getPrivilegeMode(), in_trap)!=0) { // Something wrong
		if(strcmp(debugio_t::getAppName(), "gcc")==0 &&
				(getVPC()>=0x7fffffff7ff00ff4 && getVPC()<=0x7fffffff7ff0135c)) {
			// This is known to be false positive! TODO - Fix this!
		} else {
			printRetireTrace("Fail on: ") ;
			int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
			m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
					getSequenceNumber(),
					"Range_violation",
					0x207,getVPC(),
					getPrivilegeMode(), 0,
					(uint16)trap_level, m_pseq->getID()) ;
#ifdef LXL_SNET_RECOVERY
			m_pseq->symptomDetected(0x206);
#else // LXL_SNET_RECOVERY
			HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
		}
	}
	// if( !debugio_t::isRegValueinObj(dest_flat, dest.getARF()->readInt64(dest)) ) {
	// 	printRetireTrace("Fail on: ") ;
	// 	int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
	// 	m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
	// 			getSequenceNumber(),
	// 			"Range_violation",
	// 			0x208,getVPC(),
	// 			getPrivilegeMode(), 0,
	// 			(uint16)trap_level, m_pseq->getID()) ;
	// 	HALT_SIMULATION ;
	// }

	if(!m_pseq->getSymptomSeen()) {
		// Do the basic hard bound check
		if(!getPrivilegeMode() &&
				(getAddress()>=0xffffffff80000000ULL || // Last 4GB is reserved
				 getAddress()<0x100000000) ){ // Below this is not used!
			// This check is there here because you may call this when this inst is about
			// to take a trap
			DEBUG_OUT("Address in code segment: 0x%llx\n", getAddress()) ;
			int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
			m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
					getSequenceNumber(),
					"Address_out_of_range",
					0x206,getVPC(),
					getPrivilegeMode(), 0,
					(uint16)trap_level, m_pseq->getID()) ;
#ifdef LXL_SNET_RECOVERY
			m_pseq->symptomDetected(0x206);
#else // LXL_SNET_RECOVERY
			HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
		}
		if(!getPrivilegeMode() && debugio_t::isHeapAddress(getAddress()) ) {// Application does heap access
			bool valid_access = debugio_t::isValidObject(getAddress()) ;
			if(!valid_access) {
				printRetireTrace("No valid object: " ) ;
				getSequencer()->outSymptomInfo(
						(uint64) m_pseq->getLocalCycle(),
						getSequencer()->getRetInst(),
						"Range_check_violation",
						0x206, getVPC(),
						getPrivilegeMode(),0,0,m_pseq->getID()) ;
#ifdef LXL_SNET_RECOVERY
				m_pseq->symptomDetected(0x206);
#else // LXL_SNET_RECOVERY
				HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
			}
		}
	}
}
#endif // HARD_BOUND

bool load_inst_t::collectLLB()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_pseq->getMulticoreDiagnosis()->isLoggingPhase()) {

			for (int i = 0; i < SI_MAX_DEST; i++) {
				reg_id_t &dest = getDestReg(i);

				// if the register isn't the %g0 register --
				if (!dest.isZero()) {
					if (dest.getPhysical() != PSEQ_INVALID_REG) {
						//log in the load buffer
						bool llb_full = m_pseq->addLLBEntry(m_physical_addr, dest.getARF()->readInt64(dest));
						//DEBUG_OUT("%d: address=%llx value=%llx\n", m_pseq->getID(), m_physical_addr, dest.getARF()->readInt64(dest));
						if(llb_full)
							return false;
					}
				}
			}
		} 
	}
	return true;

}

bool load_inst_t::setLLBReadPointer(int read_pointer)
{
	llb_read_pointer = read_pointer;
	return true;
}

void load_inst_t::collectLoadSources()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		for (int i = 0; i < SI_MAX_SOURCE; i++) {
			reg_id_t &src = getSourceReg(i);
			if(!src.isZero()) {
				m_pseq->getMulticoreDiagnosis()->collectCompareLog(getVPC(), src.getARF()->readInt64(src), m_pseq->getID(), true);
			}
		}
	}
}

bool load_inst_t::matchesLoadBuffer()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {

		if(m_traptype != Trap_Use_Functional) {
			int access_size = ((m_access_size+7)/8);

			if(m_pseq->getMulticoreDiagnosis()->isLoggingPhase()) {
				ireg_t value[MEMOP_MAX_SIZE];
				for(int i=0; i<MEMOP_MAX_SIZE; i++) 
					value[i] = -1;
				//if inst is correctly implemented
				m_pseq->readPhysicalMemory( m_physical_addr,
						(int) m_access_size,
						&value[0] );

				if(0) {
					DEBUG_OUT("%d:LD: address = %llx",m_pseq->getID(), m_physical_addr);
					DEBUG_OUT(" access_size = %d ", access_size);
					DEBUG_OUT(" m_data_storage = %llx\n", m_data_storage[0]);
					DEBUG_OUT(" value = %llx\n", value[0]);
				}
				//log in the load buffer
				//DEBUG_OUT("%d:physical address = %llx\n", m_pseq->getID(), m_physical_addr);
				bool is_full = m_pseq->addLLBEntry(m_physical_addr, &value[0], access_size);

				if(is_full)
					return false;
				else 
					return true;

			} else if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
				ireg_t value[MEMOP_MAX_SIZE];
				for(int i=0; i<MEMOP_MAX_SIZE; i++) 
					value[i] = -1;
				//if inst is correctly implemented
				m_pseq->readPhysicalMemory( m_physical_addr,
						(int) m_access_size,
						&value[0] );


				//DEBUG_OUT("%d:physical address = %llx\n", m_pseq->getID(), m_physical_addr);
				//if(SIVA_LOAD_BUFFER_DEBUG) {
				if(0) {
					DEBUG_OUT("%d:LD: address = %llx",m_pseq->getID(), m_physical_addr);
					DEBUG_OUT(" access_size = %d ", access_size);
					DEBUG_OUT(" m_data_storage = %llx\n", m_data_storage[0]);
					DEBUG_OUT(" value = %llx\n", value[0]);
					DEBUG_OUT(" llb_read_pointer = %d ", llb_read_pointer);
					DEBUG_OUT(" llb_front = %d ",  m_pseq->getLLB()->getFront());
					DEBUG_OUT(" llb_front_value = %llx \n",  m_pseq->getLLB()->readFront());
				}
				//}

				ireg_t value_buf[MEMOP_MAX_SIZE];
				for(int i=0; i<MEMOP_MAX_SIZE; i++) {
					value_buf[i] = -1;
				}

				//if hit in loadBuffer
				if(m_pseq->getLLB()->isLLBHit(m_physical_addr, m_pseq->getID())) {

					if(SIVA_LOAD_BUFFER_DEBUG)
						DEBUG_OUT("%d: LLB Hit\n",m_pseq->getID());

					int num_llb_entries_removed = 0;
					// read the value from buffer
					//DEBUG_OUT("\n %d: access_size=%d value=", m_pseq->getID(), m_access_size);

					for(int i=0; i<access_size; i++) {

						num_llb_entries_removed++;
						value_buf[i] = m_pseq->getLLB()->readFront(i);

						if(SIVA_LOAD_BUFFER_DEBUG) {
							DEBUG_OUT(" (%llx, ", value_buf[i]);
							DEBUG_OUT("%llx), ", m_data_storage[i]);
						}
					}

					//compare the value of memory and pipeline reg
					bool is_equal = true;
					is_equal = (llb_read_pointer == m_pseq->getLLB()->getFront());
					for(int i=0; i<access_size; i++) {
						if(value_buf[0] != m_data_storage[0]) {
						//	if(is_equal) {
						//		DEBUG_OUT("%d:%d See this case!!!", m_pseq->getID(), getSequenceNumber());
						//		DEBUG_OUT(" (%llx", value_buf[0]);
						//		DEBUG_OUT(", %llx", m_data_storage[0]);
						//		DEBUG_OUT(", %llx) \n", m_pseq->getLLB()->readFromPointer(llb_read_pointer));
						//	}
							is_equal = false;
						}
					}
					//DEBUG_OUT("%d:%d ", m_pseq->getID(), getSequenceNumber());
					//DEBUG_OUT("llb_read_pointer=%d, llb_front=%d\n", llb_read_pointer, m_pseq->getLLB()->getFront());
					if(is_equal) {
						if(SIVA_LOAD_BUFFER_DEBUG)
							DEBUG_OUT("%d:Value in buffer matched the value from memory\n", m_pseq->getID());
						//perfect the load has matched the value from the LoggingStep

						// remove the front element from the buffer
						for(int i=0; i<access_size; i++) {
							m_pseq->getLLB()->pop();
						}

						return true;
					} else {
						if(SIVA_LOAD_BUFFER_DEBUG)
							DEBUG_OUT(" reverting back the read pointer\n");
						// if no one is squashing on this address, then patch it to memory
						m_pseq->getLLB()->resetReadPointer();
					}
				} else {
					// Load miss in buffer: Continue execution (continue retiring instrucitons)
					// during the comparison point at the end of the TMR step, this miss would result in 
					// divergence. Hence, report address not found (miss) and conitnue
					m_pseq->m_multicore_diagnosis->reportAddressNotFound(m_pseq->getID(), m_physical_addr);
					if(SIVA_LOAD_BUFFER_DEBUG)
						DEBUG_OUT("%d: reporting a miss on address=%llx\n", m_pseq->getID(), m_physical_addr);

					return true;
				}
				return false;

			} else	//isTMRPhase()
				return true;
		}
	}
	return true;
}


//**************************************************************************
void 
load_inst_t::Complete( void ) {

    // update the ASI access statistics in the sequencer
    if (s->getFlag( SI_ISASI )) {
        m_pseq->m_asi_rd_stat[m_asi]++;
    }

    /* the value wasn't in the load/store queue, so get the value from
       the architected memory state */
    if ( CONFIG_IN_SIMICS ) {
        if ( isCacheable() ) {

            checkAddressAlignment() ;
            // read the value from physical memory
	    if( getTrapType() != Trap_Mem_Address_Not_Aligned ) {

		    m_data_valid = m_pseq->readPhysicalMemory( m_physical_addr,
				    (int) m_access_size,
				    getData() );

		    if(MULTICORE_DIAGNOSIS_ONLY) {
			    //DEBUG_OUT("\n %d: access_size=%d\n", m_pseq->getID(), m_access_size);
			    if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
				    int access_size = (m_access_size+7)/8;
				    //DEBUG_OUT("\n %d:%d", m_pseq->getID(), getSequenceNumber());
				    //DEBUG_OUT(" access_size=%d llb_pointer=%d, value=", access_size, llb_read_pointer);
				    if(access_size == 0) {
					DEBUG_OUT(" the reason of the bug should be this\n");
				    }
				    for(int i=0; i<access_size; i++) {
					    m_data_storage[i] = m_pseq->getLLB()->readFromPointer(llb_read_pointer+i);
				    	    //DEBUG_OUT("%llx, ", m_data_storage[i]);
				    }
				    //DEBUG_OUT("\n");
                                    m_data_valid = true;
			    } 
		    }

		    if( PRADEEP_DEBUG_LSQ ) {
			    DEBUG_OUT( "%d:", m_pseq->getID() ) ;
			    DEBUG_OUT( "Ld - %d ", getSequenceNumber() ) ;
			    DEBUG_OUT( "%s ", m_data_valid?"valid":"invalid" ) ;
			    DEBUG_OUT( "%d ", getPrivilegeMode() ) ;
			    DEBUG_OUT( "size %d ", m_access_size ) ;
			    DEBUG_OUT( "Mem[%llx] ", m_physical_addr ) ;
			    DEBUG_OUT( "= %llx\n", m_data_storage[0] ) ;
		    }
	    }

            if (m_value_pred) {
                // compare the predicted value to the one from memory
                if (validateValuePrediction()) {
                    if (log_base_two(m_access_size) <= log_base_two(MEMOP_MAX_SIZE*8))
                        m_pseq->m_stat_stale_histogram[log_base_two(m_access_size)]++;
                    else
                        m_pseq->out_info("warning: load_t complete: out of range: m_access_size\n");
                    m_pseq->m_stat_stale_success++;
#ifdef STALE_DEBUG
                    m_pseq->out_info("VSUCC: PC: 0x%0llx ADDR: 0x%0llx SIZE: %d inst: 0x%0x\n", getVPC(), m_physical_addr, m_access_size, s->getInst());
                    m_pseq->out_info("load_inst: value (%d): 0x%0llx\n",
                            m_access_size, m_predicted_storage[0]);
                    m_pseq->out_info("load_inst: value prediction succeeded!\n");
#endif
                    SetStage(COMPLETE_STAGE);
                } else {
                    // trigger a squash on this instruction
#ifdef STALE_DEBUG
                    m_pseq->out_info("VFAIL: PC: 0x%0llx ADDR: 0x%0llx SIZE: %d inst: 0x%0x\n", getVPC(), m_physical_addr, m_access_size, s->getInst());
                    m_pseq->out_info("load_inst: mispredict (%d): 0x%0llx\n",
                            m_access_size, m_predicted_storage[0]);
                    m_pseq->out_info("load_inst: actual        : 0x%0llx\n",
                            getData()[0]);
                    m_pseq->out_info("load_inst: time %0lld\n",
                            m_pseq->getLocalCycle());
#endif
                    m_pseq->raiseException( EXCEPT_VALUE_MISPREDICT,
                            m_pseq->getIwindow()->iwin_decrement( getWindowIndex() ),
                            (enum i_opcode) s->getOpcode(),
                            &m_fetch_at, 0 , /* penalty */ 0 );
                }
                return;
            }

#ifdef DEBUG_LSQ
            m_pseq->out_log( "load_complete: index:%d 0x%0llx\n",
                    getWindowIndex(), m_data_storage[0] );
#endif
        } else {
            // do the MMU access (using a backdoor in simics) if not bundling retires
            // By not bundling retires, the MMU state reflects the architected state
            // some errors may still be caused by the out-of-order execution in
            // non-bundled mode, but they are few / minor.
            if (PSEQ_MAX_UNCHECKED == 1) {
#ifdef DEBUG_LSQ
                m_pseq->out_log("   MMU load access: index: %d\n", getWindowIndex());
#endif
                if ( m_asi >= 0x50 && m_asi <= 0x5f )
                    mmuRegisterAccess( false );

#ifdef SW_ROLLBACK
				if (m_pseq->isRecovering() && m_asi==0x8c) {
					m_data_storage[0] = m_pseq->getCheckpointReg(m_dest_reg[0]);
					m_data_valid = true;
				}
#endif
            }
        }
    } else {

        // read the value from the memory trace
        memtrace_t *mt = m_pseq->getMemtrace();
        //  m_pseq->out_log("local time %d = %d address 0x%0llx\n", m_pseq->getRetiredICount(), mytime, m_address);
        // plus 2 as the time is at the retire time ... not the current time :)
        m_data_valid = mt->scanTransaction( m_pseq->getRetiredICount() + 2,
                m_address, getVPC(),
                s->getInst(), m_physical_addr,
                m_access_size, getData() );
    }

    /* format the value and write it into a register */
    memory_inst_t::Execute();
}

//***************************************************************************
bool load_inst_t::validateValuePrediction( void )
{
    bool  match = true;
    char *pred   = (char *) m_predicted_storage;
    char *actual = (char *) getData();

    for (int i=0; i < m_access_size; i++) {
        if (pred[i] != actual[i]) {
#ifdef STALE_DEBUG
            m_pseq->out_info("(%i) SIZE: %d  ADDR: 0x%0llx PRED: 0x%x ACTUAL: 0x%x\n",
                    i, (int) m_access_size, getVPC(),
                    (int) pred[i], (int) actual[i] );
#endif
            match = false;
        }
    }
    return match;
}

//***************************************************************************
void load_inst_t::printDetail( void )
{
    m_pseq->out_info("load_inst_t\n");
    memory_inst_t::printDetail();
    if (m_depend) {
        m_pseq->out_info("   depend: %d\n", m_depend->getWindowIndex());
    }
}

/*------------------------------------------------------------------------*/
/* store_inst                                                             */
/*------------------------------------------------------------------------*/

//***************************************************************************
store_inst_t::store_inst_t( static_inst_t *s_inst, 
        int32 window_index,
        pseq_t *pseq,
        abstract_pc_t *at, fault_stats* fault_stat )
: memory_inst_t( s_inst, window_index, pseq, at, fault_stat ) {
	// use default dynamic inst constructor
	m_atomic_swap = false;
	corrupting_store = false;
	patch_store_value = false;
}

//***************************************************************************
store_inst_t::~store_inst_t() {
}

//***************************************************************************
bool
store_inst_t::storeDataToCache( void ) {

    reg_id_t &source = getSourceReg(2);
    if ( !source.getARF()->isReady(source) ) {
        // not ready quite yet...
        ERROR_OUT("warning: storeDataToCache: stalling: %d\n", getWindowIndex());
        printDetail();
        source.getARF()->waitResult( source, this );
        return (false);
    }

    // read the store value from the register file (source 3)

    // convert the access size (in bytes) to register size (8 byte) quantities
    byte_t access_size = ((m_access_size + 7)/ 8);
    for (int i = 0; i < access_size; i++) {
        // if copying more than one register-- set a register selector
        // this is used for 64-byte load stores
        if (access_size > 1)
            source.setVanilla( i );
        m_data_storage[i] = source.getARF()->readInt64( source );
#ifdef DEBUG_LSQ 
        m_pseq->out_log("storeData2Cache: index:%d size:%d stored data: 0x%0llx\n",
                getWindowIndex(), i, m_data_storage[i]);
        DEBUG_OUT("s2 %d ",source.getPhysical());
        DEBUG_OUT("fault %d\n",source.getARF()->getFault(source));
#endif
    }
    m_data_valid = true;  

    if (!accessCache())
        return false;
    return true;
}

//**************************************************************************
void 
store_inst_t::Execute() {
    //    DEBUG_OUT("seq %d Execute\n",getSequenceNumber());

	for( int i=0; i< SI_MAX_DEST ; i++ ) { //Changed from MAX_DST to 1
		reg_id_t &dest = getDestReg(i) ;
		if( !dest.isZero() ) { // Inject only if the destination is a valid register
			dest.getARF()->unSetCorrupted(dest) ;
			dest.getARF()->clearFault(dest);
			dest.getARF()->clearReadFault(dest);
		}
	}
    if ( s->getType() == DYN_ATOMIC ) {
        STAT_INC( m_pseq->m_stat_atomics_exec );
    } else {
        STAT_INC( m_pseq->m_stat_stores_exec );
    }

    trap_type_t t = addressGenerate( OPAL_STORE );
	propagateFault() ;
    if (t != Trap_NoTrap) {
        /* couldn't correctly generate address - fired off exception */
        SetStage( COMPLETE_STAGE );
        setTrapType( (trap_type_t) t );
        return;
    }

    /* can't complete I/O accesses, and moreover they are uncacheable */
    if ( m_physical_addr != (pa_t) -1 &&
            isIOAccess( m_physical_addr ) ) {
        SetStage( COMPLETE_STAGE );
        setTrapType( Trap_Use_Functional );
        STAT_INC( m_pseq->m_stat_count_io_access );
#ifdef DEBUG_LSQ
        m_pseq->out_log("   store I/O access: index: %d\n", getWindowIndex());
#endif
        return;
    }

    if ( isCacheable() ) {
        ASSERT( m_physical_addr != (pa_t) -1 );
        // insert this store instruction into the load/store queue.
        if (m_pseq->getLSQ()->insert( this )) {
            /* we got a hit in the LSQ or some other event so we should stop
               handling it like a normal load */
#ifdef DEBUG_LSQ
            m_pseq->out_log("***** (atomic) load LSQ exception cycle %lld\n",
                    m_pseq->getLocalCycle() );
#endif
            return;
        }
    }  //end isCacheable()

    // If ST or ATOMIC succeeds in inserting into LSQ, continue onto next phase of execution
    continueExecution();
}

//*****************************************************************
void 
store_inst_t::continueExecution(){
    if( isCacheable() ){
        // are all registers ready
        bool       all_ready = true;
        for (int32 i = 0; i < SI_MAX_SOURCE; i++) {
            reg_id_t &source = getSourceReg(i);
            if ( !source.getARF()->isReady( source ) ) {
                all_ready = false;

                if (s->getType() == DYN_STORE) {
                    STAT_INC(m_pseq->m_stat_num_early_stores);
                    SetStage( EARLY_STORE_STAGE );
                } else if (s->getType() == DYN_ATOMIC) {
                    STAT_INC(m_pseq->m_stat_num_early_atomics);
                    SetStage( EARLY_ATOMIC_STAGE );
                }
                // wait for the source register to be written
                source.getARF()->waitResult( source, this );
                break;
            }
        }

        if ( all_ready ) {
            // read the register, and do a demand write to the cache
            //    if the cache misses, return immediately
            if ( !storeDataToCache() ) {
                return;
            }
        } else {
            if (CONFIG_WITH_RUBY) {
                rubycache_t *rcache = m_pseq->getRubyCache();
                // initiate a prefetch... if it fails because ruby isn't ready,
                //                        don't retry it, we'll issue soon enough.
                //Differentiate between Stores and Atomics
                OpalMemop_t requestType = OPAL_STORE;
                if(s->getType() == DYN_ATOMIC){
                    requestType = OPAL_ATOMIC;
                }
                rcache->prefetch( m_physical_addr, requestType,
                        getVPC(), (m_pstate >> 2) & 0x1 );
            } else {
                // early store: if its not already in our cache, exclusive prefetch block
                cache_t *dcache = m_pseq->getDataCache();
                if ( !dcache->TagCheck(m_physical_addr) )
                    dcache->Prefetch( m_physical_addr);
            }
            // scheduled wakeup during "all register ready" loop
            return;
        }
    }  // end isCacheable()

    Complete();
}

//**************************************************************************
void 
store_inst_t::storeDataWakeup( void ) {
    if ( storeDataToCache() ) {
        // hit in cache -- we can complete
        //   be careful: this could be atomic_inst_t complete!
        Complete();    
    }
    // miss in cache -- we are already placed in cache miss stage...
}

//**************************************************************************
void 
store_inst_t::wakeupDependentLoads() {
    m_dependent_loads.WakeupChain();
}

//************************************************************************
void
store_inst_t::wakeupOverlapLoads() {
    m_overlap_loads.WakeupChain();
}


// accessCache() for both store and atomic instructions!
//**************************************************************************
bool
store_inst_t::accessCache( void ) {

	if(MULTICORE_DIAGNOSIS_ONLY) 
		return true;

	if (CONFIG_WITH_RUBY) {
#ifdef DEBUG_RUBY
		m_pseq->out_log("PC: 0x%0llx DATASTORE: 0x%0llx\n",
				getVPC(), m_physical_addr );
#endif
		bool          mshr_hit = false;
		bool          mshr_stall = false;
		rubycache_t  *rcache   = m_pseq->getRubyCache();
		//Differentiate between Stores and Atomics
		OpalMemop_t requestType = OPAL_STORE;
		if(s->getType() == DYN_ATOMIC){
			requestType = OPAL_ATOMIC;
		}

		if (ALEX_DEBUG_VALUE_BEFORE_SAVE&&ALEX_SNET_DEBUG) {
			ireg_t old_value;
			if (m_pseq->readPhysicalMemory( m_physical_addr,
						(int) m_access_size,
						&old_value)) {
				cout << getSequenceNumber() << ": old_value is " << hex << old_value << endl;
			}
		}

		// printRetireTrace("st_access_cache:") ;
		ruby_status_t status   = rcache->access( m_physical_addr, requestType,
				getVPC(), (m_pstate >> 2) & 0x1,
				this, mshr_hit, mshr_stall );
		if (mshr_hit == true) {
			markEvent( EVENT_MSHR_HIT );
		}
		if (mshr_stall == true) {
			markEvent(EVENT_MSHR_STALL );
		}
		if ( status == HIT ) {
			// do nothing
		} else if ( status == MISS ) {
			markEvent( EVENT_DCACHE_MISS );
			STAT_INC(m_pseq->m_stat_num_dcache_miss);
			SetStage(CACHE_MISS_STAGE);
			return false;
		} else if ( status == NOT_READY ) {
			// NOT_READY cache status causes polling using event queue
			m_pseq->postEvent( this, 1 );
			STAT_INC(m_pseq->m_num_cache_not_ready);
			SetStage(CACHE_NOTREADY_STAGE);
			return false;
		} else {
			SIM_HALT;
		}
	} else {
		// check the cache for presence of this line
		cache_t *dcache = m_pseq->getDataCache();

		if ( !dcache->Write( m_physical_addr, this ) ) {
			/* we missed: the cache is requesting a fill,
			 *            we'll wait in CACHE_MISS_STAGE for the wakeup,
			 *            update statistics, return */
			SetStage( CACHE_MISS_STAGE );
			STAT_INC(m_pseq->m_stat_num_dcache_miss);
			return false;
		}
	}

	return true;
}

//**************************************************************************
void
store_inst_t::Retire( abstract_pc_t *a ) {
    // update statistics
    STAT_INC( m_pseq->m_stat_stores_retired );
	
    memory_inst_t::Retire(a);

#ifdef DEBUG_AMBER_CHKPT
	printRetireTrace("St retire: ") ;
#endif // DEBUG_AMBER_CHKPT

	// Doing this for only store instructions for now - FIXME
	// m_fs->buildInstRange(getPrivilegeMode(), getVPC(),getAddress()) ;

	// If the store is corrupted, assume it is writing a bad value to memory
	// It not, it is overwriting a previously bad memory value.
	bool corrupts_memory = isCorrupted() | false ;
	for (int i = 0; i < SI_MAX_SOURCE; i ++) {
		reg_id_t &reg = getSourceReg(i);
		if (!reg.isZero()) {
			corrupts_memory |= reg.getARF()->isCorrupted(reg) ;
		}
	}
	if(corrupts_memory) {
		// printRetireTrace("") ;
		debugio_t::addCorruptAddress(getAddress(),getPrivilegeMode()) ;
		this->setCorrupted() ;
		// Record the type of instruction as faulty
		if(RECORD_FAULTY_INST) {
			m_fs->addFaultyInstruction(decode_opcode((enum i_opcode)s->getOpcode())) ;
		}
	} else {
		debugio_t::removeCorruptAddress(getAddress(),getPrivilegeMode()) ;
	}

    checkAddressAlignment() ;

#if TRACK_MEM_SHARING
	bool updating = false;
	if(m_fs->getArchMismatch()) {
		struct addr_info temp;
		temp.core_id = getSequencer()->getID();
		temp.rw = 1;
		temp.trap_level = m_tl;
		temp.pc = getVPC();
		temp.corrupted = true;
		temp.val = value;

		debugio_t::add_shared_address(m_physical_addr,getPrivilegeMode(), temp);

		//debugio_t::updateMap(m_physical_addr, dynamic_inst_t::getSequencer()->getID(), getPrivilegeMode());
	// } else {
	// 	if(debugio_t::getMappedCoreID(m_physical_addr) != dynamic_inst_t::getSequencer()->getID()) {
	// 		debugio_t::updateMap(m_physical_addr, dynamic_inst_t::getSequencer()->getID());
	// 		updating = true;
	// 	}
	}
	// if(0&updating) {
	// 	debugio_t::out_info("coreID:%d ",dynamic_inst_t::getSequencer()->getID());
	// 	debugio_t::out_info("Address:%llx\n",m_physical_addr);
	// }
	
#endif // TRACK_MEM_SHARING

	if( (getTrapType() != Trap_Mem_Address_Not_Aligned ) && m_data_valid ) {
		// If the retiring store is faulty, then flush this data
		// into memory. Also, for address faults (AGEN/LSQ), make sure
		// that we re-write the old value into the old address as simics
		// would've now over-written it with teh correct value
		if( getFault() ) {
		    ireg_t  data_new_loc[MEMOP_MAX_SIZE];
		    bool data_new_valid = m_pseq->readPhysicalMemory(m_physical_addr,
				    (int) m_access_size,
				    &data_new_loc[0] );
		    if (!isEqual(data_new_loc,m_data_storage)) {
			    if (LXL_STORE_DEBUG) {
				    DEBUG_OUT("store corrupt s %d n %llx o %llx\n",
							m_access_size,data_new_loc[0],m_data_storage[0]);
					DEBUG_OUT("instr opcode %s\n",decode_opcode(s->getOpcode()));
			    }
			    corrupting_store=true;
			    setMemFault() ;
		    }
#if defined( LXL_SNET_RECOVERY) && USE_TBFD
		    if (!m_pseq->recovered || (!LXL_NO_CORRUPT && m_pseq->recovered)) {
			    m_pseq->writePhysicalMemory( m_physical_addr,
					    (int) m_access_size,
					    getData() ) ;
		    }
#else //define(LXL_SNET_RECOVERY) && USE_TBFD
#define LXL_TEST_CODE 1
#if LXL_TEST_CODE
			ireg_t  data_new_loc_test[MEMOP_MAX_SIZE];
			bool data_new_valid_test = m_pseq->readPhysicalMemory(m_physical_addr,
					(int) m_access_size,
					&data_new_loc_test[0] );
			if (!isEqual(data_new_loc_test,m_data_storage)) {
				if (LXL_STORE_DEBUG) {
					DEBUG_OUT("store corrupt s %d n %llx o %llx\n",m_access_size,data_new_loc_test[0],m_data_storage[0]);
				}
				corrupting_store=true;
			}
#endif //LXL_TEST_CODE

			if(MULTICORE_DIAGNOSIS_ONLY) { 
				if(m_pseq->getMulticoreDiagnosis()->dont_run_core_id() != m_pseq->getID()) {
					if(m_pseq->getMulticoreDiagnosis()->isLoggingPhase()) {
						m_pseq->writePhysicalMemory( m_physical_addr,
								(int) m_access_size,
								getData() ) ;
					}
				}
			} else {
				m_pseq->writePhysicalMemory( m_physical_addr,
						(int) m_access_size,
						getData() ) ;
				m_pseq->logMemEvent(m_physical_addr, m_access_size, getData());
			}

			
#endif //define(LXL_SNET_RECOVERY) && USE_TBFD

			if( PRADEEP_LSQ_BADADDR ) {
				DEBUG_OUT("St%s", getFault()?"*":" " ) ;
				DEBUG_OUT(" -> %llx but ", m_old_physical_addr ) ;
				DEBUG_OUT("to %llx\n", m_physical_addr ) ;
			}
			if( PRADEEP_DEBUG_LSQ ) {
				ireg_t  m_temp_data_storage[MEMOP_MAX_SIZE];
				bool m_temp_data_valid = m_pseq->readPhysicalMemory(
						m_physical_addr,
						(int) m_access_size,
						&m_temp_data_storage[0] );

				DEBUG_OUT( "St - %d -- ", getSequenceNumber() ) ;
				if( m_temp_data_valid ) {
					DEBUG_OUT( "Corrupt data - Mem[%llx]", m_physical_addr ) ;
					DEBUG_OUT( " = %llx\n", m_temp_data_storage[0] ) ;
				} else {
					DEBUG_OUT( "Write to Mem[%llx] failed\n", m_physical_addr) ;
				}
			}
			if( m_old_data_valid  && (m_old_data_valid!=((my_addr_t)-1) )) {
				if( oldAddressAligned() ) {
					ireg_t  data_old_loc[MEMOP_MAX_SIZE];
					bool data_old_valid = m_pseq->readPhysicalMemory(m_old_physical_addr,
							(int) m_access_size,
							&data_old_loc[0] );
					if (!isEqual(data_old_loc,m_old_data_storage)) {
						if (LXL_STORE_DEBUG) {
							DEBUG_OUT("store corrupt b %llx a %llx\n",data_old_loc[0],m_old_data_storage[0]);
						}
						corrupting_store=true;
						setMemFault() ;
					}
#if defined(LXL_SNET_RECOVERY) && USE_TBFD
					if (!m_pseq->recovered || (!LXL_NO_CORRUPT && m_pseq->recovered)) {
						m_pseq->writePhysicalMemory( m_old_physical_addr,
								(int) m_access_size,
								getOldData() ) ;
					}
#else // defined(LXL_SNET_RECOVERY) && USE_TBFD
#if LXL_TEST_CODE
					ireg_t  data_old_loc_test[MEMOP_MAX_SIZE];
					bool data_old_valid_test = m_pseq->readPhysicalMemory(m_old_physical_addr,
							(int) m_access_size,
							&data_old_loc_test[0] );
					if (!isEqual(data_old_loc_test,m_old_data_storage)) {
						if (LXL_STORE_DEBUG) {
							DEBUG_OUT("store corrupt b %llx a %llx\n",data_old_loc_test[0],m_old_data_storage[0]);
						}
						corrupting_store=true;
					}
#endif //LXL_TEST_CODE

					if(MULTICORE_DIAGNOSIS_ONLY) { 
						if(m_pseq->getMulticoreDiagnosis()->dont_run_core_id() != m_pseq->getID())
							m_pseq->writePhysicalMemory( m_old_physical_addr,
									(int) m_access_size,
									getOldData() ) ;
					} else
						m_pseq->writePhysicalMemory( m_old_physical_addr,
								(int) m_access_size,
								getOldData() ) ;

#endif // defined(LXL_SNET_RECOVERY) && USE_TBFD
					if( PRADEEP_DEBUG_LSQ ) {
						ireg_t  m_temp_data_storage[MEMOP_MAX_SIZE];
						bool m_temp_data_valid = m_pseq->readPhysicalMemory(
								m_old_physical_addr,
								(int) m_access_size,
								&m_temp_data_storage[0] );

						DEBUG_OUT( "St - %d -- ", getSequenceNumber() ) ;
						if( m_temp_data_valid ) {
							DEBUG_OUT( "Old data - Mem[%llx]", m_old_physical_addr ) ;
							DEBUG_OUT( " = %llx\n", m_temp_data_storage[0] ) ;
						} else {
							DEBUG_OUT( "Write to Mem[%llx] failed\n",
									m_old_physical_addr) ;
						}
					}
				} else {
					if( PRADEEP_DEBUG_LSQ ) {
						DEBUG_OUT( "%d old addr not aligned ", getSequenceNumber() ) ;
						DEBUG_OUT( "(Mem[%llx]\n", getOldPhysicalAddress() ) ;
					}
				}
			}
		}
	} else {
        // It can come here because we may be using functional trap to
        // update the data and hence the valid bit is not set.
        if( PRADEEP_DEBUG_LSQ ) {
            printRetireTrace( "Data not ready when instruction retiring\n" ) ;
        }
    }


    // Pradeep - Take stats of retiring store
    // m_pseq->INC_STORE();
#ifdef ALEX_FAULT_RET_OUT
    FAULT_RET_OUT( "Store\t" ) ;
    //    FAULT_RET_OUT( "[%u]\t", m_pseq->getLocalCycle() ) ;
    //    FAULT_RET_OUT( "[%u]\t", getSequenceNumber() ) ;
    FAULT_RET_OUT( "pc:0x%0llx", getVPC() ) ;
    FAULT_RET_OUT( "(%d)\t", getPrivilegeMode() ) ;
    FAULT_RET_OUT( "Addr:0x%0llx\t", getAddress() ) ;
    FAULT_RET_OUT( "Data:0x%0llx\n", getUnsignedData() ) ;

#endif
    // Store retired. So, at this junction, if one of the sources
    // is faulty, a faulty store retired
    // FIXME - But this doesnt mean the data was wrong. It is still
    // possible that the data was correct.

    //if( getFault() || getMemFault() ) 
    if( isCorrupted() || getMemFault() || corrupting_store) {
        if(PRADEEP_DEBUG_AGEN||LXL_DEBUG) {
            DEBUG_OUT( "%d Store inst faulty\n", getSequenceNumber() ) ;
        }
        getFaultStat()->setArchCycle() ;
        getFaultStat()->setMemCycle() ;
#if 0
	    FAULT_RET_OUT( "Store\t" ) ;
	    FAULT_RET_OUT( "[%u]\t", m_pseq->getLocalCycle() ) ;
	    FAULT_RET_OUT( "[%u]\t", getSequenceNumber() ) ;
	    FAULT_RET_OUT( "pc:0x%0llx", getVPC() ) ;
	    FAULT_RET_OUT( "(%d)\t", getPrivilegeMode() ) ;
	    FAULT_RET_OUT( "Addr:0x%0llx\t", getAddress() ) ;
	    FAULT_RET_OUT( "Data:0x%0llx\n", getUnsignedData() ) ;

	    if ( s->getFlag(SI_ISIMMEDIATE) ) {
		    reg_id_t &rid=getSourceReg(0);
		    FAULT_RET_OUT( "addr s0:%d\t", reg_box_t::iregWindowMap( rid.getVanilla(),
					    rid.getVanillaState() ));
		    FAULT_RET_OUT( "imm:0x%0llx\t", s->getImmediate());
	    } else {
		    reg_id_t &rid=getSourceReg(0);
		    reg_id_t &rid1=getSourceReg(1);

		    FAULT_RET_OUT( "addr s0:%d\t", reg_box_t::iregWindowMap( rid.getVanilla(),
					    rid.getVanillaState() ));
		    FAULT_RET_OUT( "s1:%d\t", reg_box_t::iregWindowMap( rid1.getVanilla(),
					    rid1.getVanillaState() ));
	    }

	    reg_id_t &rid2=getSourceReg(2);

	    FAULT_RET_OUT( "data s2:%d\n", reg_box_t::iregWindowMap( rid2.getVanilla(),
				    rid2.getVanillaState() ));

#endif
    }

    if (ALEX_DEBUG_MEMOP&&ALEX_SNET_DEBUG) {
	    ireg_t new_value;
	    if (m_pseq->readPhysicalMemory( m_physical_addr,
				    (int) m_access_size,
				    &new_value)) {
		    cout << hex << getVPC() << ": new_value @ " << m_physical_addr << " is " 
			    << hex << new_value << endl;
		    //      cout << hex << getVPC() << endl;
	    }
    }

#ifdef LXL_MEM_REC_VERIFIER
	if (!m_pseq->recovered) {
		ireg_t tmp = m_data_storage[0];
		if (m_access_size < 8) {
			tmp = tmp & (0xffffffffffffffffULL>>(64-8*m_access_size));
		}
		m_pseq->addMemRec(getVPC(),false,m_physical_addr,tmp);
	} else {
		if (m_pseq->mem_trace_cnt < m_pseq->m_mem_trace.size()) {

			mem_rec_t &verify=m_pseq->m_mem_trace[m_pseq->mem_trace_cnt++];
			ireg_t tmp = m_data_storage[0];
			if (m_access_size < 8) {
				tmp = tmp & (0xffffffffffffffffULL>>(64-8*m_access_size));
			}

			if ( (verify.PC != getVPC()) ||
					verify.isLoad ||
					(verify.address != m_physical_addr) ||
					(verify.data != tmp)) {
				DEBUG_OUT("ERROR: %llx mismatch store!!\t",getSequenceNumber());
				DEBUG_OUT("PC %llx %llx \n", getVPC(),verify.PC);
				DEBUG_OUT("trace cnt %d \n", m_pseq->mem_trace_cnt);
				DEBUG_OUT("isLoad %d\n",verify.isLoad);
				DEBUG_OUT("addr %llx %llx \n", m_physical_addr,verify.address);
				DEBUG_OUT("data %llx %llx \n", tmp,verify.data);
				HALT_SIMULATION;
			}

		} else {
			//DEBUG_OUT("new stores\n");
		}
	}
#endif

	// if(debugio_t::isCorruptAddress(m_address,getPrivilegeMode())) {
	// 	DEBUG_OUT("0x%llx\n", m_address) ;
	// }
    // Need to wakeup any loads that have overlapping addresses 
    wakeupOverlapLoads();

    if(MULTICORE_DIAGNOSIS_ONLY) {
	    if(m_pseq->getMulticoreDiagnosis()->isLoggingPhase()) {
		    ireg_t temp_value[MEMOP_MAX_SIZE];
		    m_pseq->readPhysicalMemory( m_physical_addr, (int) m_access_size, &temp_value[0]);
		    ireg_t b_masked = temp_value[0]&(0xffffffffffffffffULL>>(64-8*m_access_size));

		    if(m_physical_addr == -1) {
			    //pa_t phys_mem;
			    //DEBUG_OUT("\n m_address=%llx", m_address);
			    //bool excep = m_pseq->translateInstruction(m_address, m_access_size, phys_mem); 
			    //DEBUG_OUT(": %llx\n", phys_mem);
#ifndef EXECUTE_STORES
			    if(CONCISE_VERBOSE)
				    DEBUG_OUT("%d: Logging: Physical address is -1\n", m_pseq->getID());
			    m_pseq->getMulticoreDiagnosis()->noPhysicalAddress(m_pseq->getID());
#endif
		    } else 
			    m_pseq->getMulticoreDiagnosis()->collectCompareLog(getVPC(), m_physical_addr, temp_value[0], m_pseq->getID());
	    }
	    if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {

		    //collecting CompareLog elements
		    //if(m_physical_addr != -1) {
			    ireg_t temp_value[MEMOP_MAX_SIZE];
			    m_pseq->readPhysicalMemory( m_physical_addr, (int) m_access_size, &temp_value[0]);
			    ireg_t b_masked = temp_value[0]&(0xffffffffffffffffULL>>(64-8*m_access_size));

			    if(SIVA_LOAD_BUFFER_DEBUG ) {
				    DEBUG_OUT("%d:St: ", m_pseq->getID());
				    DEBUG_OUT("addr=%llx, ", m_physical_addr);
				    DEBUG_OUT("size=%llx, ", m_access_size);
				    DEBUG_OUT("value=%llx \n", temp_value[0]);
				    DEBUG_OUT("value=%llx \n", m_data_storage[0]);
				    DEBUG_OUT("b_mask=%llx \n", b_masked);
			    }

			    if(m_physical_addr != -1)  {
				    m_pseq->getMulticoreDiagnosis()->collectCompareLog(getVPC(), m_physical_addr, temp_value[0], m_pseq->getID());

				    // patch the store value with the value just before the store instruction during TMR phase
				    // This is a hack for not letting store propagate to memory
				    // this SHOULD done after collecting the log, because the compareLog should have use the old value
				    writeBackStoreValue();
			    }
		    //} else {
			    //m_pseq->out_info("   access: %d", m_access_size );
			    //m_pseq->out_info("   addr  : 0x%0llx", m_address );
			    //m_pseq->out_info("   phys a: 0x%0llx\n", m_physical_addr );
				
			    //DEBUG_OUT(" Could not patch: NEED TO PATCH!!!!!\n");
		    //}
	    }
    }

	// Prefetch learning. Learn this address.
	// if(!getPrivilegeMode()) { // Don't do this for OS as its hard to predict
	// 	ireg_t prefetch_addr = m_fs->getPrefetch(getVPC(),m_address) ;
	// 	if(prefetch_addr!=0x0 && prefetch_addr!=m_address) {
	// 		DEBUG_OUT("St: 0x%llx: 0x%llx\t0x%llx\n",
	// 				getVPC(), prefetch_addr, m_address) ;
	// 	}
	// }
#ifdef HARD_BOUND
	// Do the memory tracking for the objects
	propagateObjects() ;
#endif // HARD_BOUND
	if(INST_MIX) {
		m_fs->recordInstMix("store",isCorrupted()) ;
	}
}

#ifdef HARD_BOUND
void store_inst_t::propagateObjects(int in_trap)
{
	ireg_t addr = getAddress() ;
	reg_id_t &src1 = getSourceReg(0) ;
	half_t src1_flat ;
	if(src1.getRtype()!=RID_INT && src1.getRtype()!=RID_INT_GLOBAL) {
		src1_flat = PSEQ_FLATREG_NWIN ;
	} else {
		src1_flat = src1.getFlatRegister() ;
	}

	reg_id_t &src2 = getSourceReg(1) ; 
	half_t src2_flat ;
	if(s->getFlag(SI_ISIMMEDIATE) || (src1.getRtype()!=RID_INT && src1.getRtype()!=RID_INT_GLOBAL)) {
		src2_flat = PSEQ_FLATREG_NWIN ;
	} else {
		src2_flat = src2.getFlatRegister() ;
		if(src1_flat !=PSEQ_FLATREG_NWIN) {
			// How to find out what the base is. Pass in src1 as the one with
			// the higher value. Perhaps that is the base
			if(src1.getARF()->readInt64(src1) < src1.getARF()->readInt64(src2)) {
				half_t temp = src1_flat ;
				src1_flat = src2_flat ;
				src2_flat = temp ;
			}
		}
	}

	reg_id_t &dest = getDestReg(0) ;
	half_t dest_flat ;
	if(dest.getRtype()!=RID_INT && dest.getRtype()!=RID_INT_GLOBAL) {
		dest_flat = PSEQ_FLATREG_NWIN ;
	} else {
		dest_flat = dest.getFlatRegister() ;
	}

	// Do the sophisticated hard bound checks
	if(debugio_t::handleObjectStore(dest_flat, src1_flat, src2_flat, addr, getPrivilegeMode(), in_trap)!=0) { // Something wrong
		if(strcmp(debugio_t::getAppName(), "gcc")==0 &&
				(getVPC()>=0x7fffffff7ff00ff4 && getVPC()<=0x7fffffff7ff0135c)) {
			// This is known to be false positive! TODO - Fix this!
		} else {
			printRetireTrace("Fail on: ") ;
			int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
			m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
					getSequenceNumber(),
					"Range_violation",
					0x207,getVPC(),
					getPrivilegeMode(), 0,
					(uint16)trap_level, m_pseq->getID()) ;
			HALT_SIMULATION ;
		}
	} 
	// if( !debugio_t::isMemValueinObj(getAddress(), dest.getARF()->readInt64(dest)) ) {
	// 	printRetireTrace("Fail on: ") ;
	// 	int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
	// 	m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
	// 			getSequenceNumber(),
	// 			"Range_violation",
	// 			0x208,getVPC(),
	// 			getPrivilegeMode(), 0,
	// 			(uint16)trap_level, m_pseq->getID()) ;
	// 	HALT_SIMULATION ;
	// }

	if(!m_pseq->getSymptomSeen()) {
		// Do the basic hard bound check
		if(!getPrivilegeMode() &&
				(getAddress()>=0xffffffff80000000ULL || // Last 4GB is reserved
				 getAddress()<0x100000000) ){ // Below this is not used!
			// This check is there here because you may call this when this inst is about
			// to take a trap
			DEBUG_OUT("Address in code segment: 0x%llx\n", getAddress()) ;
			int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
			m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
					getSequenceNumber(),
					"Address_out_of_range",
					0x206,getVPC(),
					getPrivilegeMode(), 0,
					(uint16)trap_level, m_pseq->getID()) ;
			HALT_SIMULATION ;
		}
		if(!getPrivilegeMode() && debugio_t::isHeapAddress(getAddress()) ) {// Application does heap access
			bool valid_access = debugio_t::isValidObject(getAddress()) ;
			if(!valid_access) {
				printRetireTrace("No valid object: " ) ;
				getSequencer()->outSymptomInfo(
						(uint64) m_pseq->getLocalCycle(),
						getSequencer()->getRetInst(),
						"Range_check_violation",
						0x206, getVPC(),
						getPrivilegeMode(),0,0,m_pseq->getID()) ;
				HALT_SIMULATION ;
			}
		}
	}
}
#endif // HARD_BOUND

//**************************************************************************
void store_inst_t::copyStoreValue()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
			if(m_physical_addr == -1) {
				DEBUG_OUT(" Physical Address is -1\n");
				DEBUG_OUT("%d:St: ", m_pseq->getID());
				DEBUG_OUT("addr=%llx, ", m_physical_addr);
				DEBUG_OUT("size=%llx, ", m_access_size);
				DEBUG_OUT("value=%llx \n", m_data_storage[0]);

				m_pseq->getMulticoreDiagnosis()->stopDiagnosis();
			} else {
				m_pseq->readPhysicalMemory( m_physical_addr,
						(int) m_access_size,
						&previous_value[0] );
				patch_store_value = true;
			}
		}
	}
}

//**************************************************************************
void store_inst_t::writeBackStoreValue()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
			if(patch_store_value) {
				if(SIVA_LOAD_BUFFER_DEBUG) {
					DEBUG_OUT(" Patching Store value\n");
					DEBUG_OUT("%d:St: ", m_pseq->getID());
					DEBUG_OUT("addr=%llx, ", m_physical_addr);
					DEBUG_OUT("size=%llx, ", m_access_size);
					DEBUG_OUT("prev_value=%llx \n", previous_value[0]);
				}

				m_pseq->writePhysicalMemory( m_physical_addr,
						(int) m_access_size,
						&previous_value[0] ) ;
			}
			patch_store_value = false;
		}
	}
}

//**************************************************************************
void 
store_inst_t::Complete() {
    // update the ASI access statistics in the sequencer
    if (s->getFlag( SI_ISASI )) {
        m_pseq->m_asi_wr_stat[m_asi]++;
    }

    /* this function can get called when both a store's address and data
       are available */
    memory_inst_t::Execute(); 

    if( getTrapType() != Trap_Mem_Address_Not_Aligned ) {
        if( getMemFault() && (m_old_physical_addr!=((my_addr_t)-1) )) {
            m_old_data_valid = m_pseq->readPhysicalMemory( m_old_physical_addr,
                    (int) m_access_size,
                    getOldData() );
            m_old_data_valid = true ;
        }
    }
	// propagateFault() ;

    /* must call wakeup _after_ execute:
     *      wakeup checks the store stage (must be completed)
     *      before bypassing the value */
    wakeupDependentLoads();
}

//***************************************************************************
void store_inst_t::printDetail( void )
{
    m_pseq->out_info("store_inst_t\n");
    memory_inst_t::printDetail();
    m_pseq->out_info( "  waitlist: \n");
    m_dependent_loads.print( cout );
    m_pseq->out_info( "  atomic swap: %d\n", m_atomic_swap );
}

bool memory_inst_t::isEqual(ireg_t* A, ireg_t* B)
{
	if (m_access_size<=8) {
		// ireg_t a_masked=A[0]&(0xffffffffffffffffULL>>(64-8*m_access_size));
		// ireg_t b_masked=B[0]&(0xffffffffffffffffULL>>(64-8*m_access_size));
// 		if (a_masked!=b_masked) {
// 			DEBUG_OUT("a %llx b %llx\n",a_masked,b_masked);
// 		}
		// return (a_masked==b_masked);
		return(getMaskedData(A)==getMaskedData(B)) ;
	}
	else  {
		ireg_t a0_masked=A[0]&(0xffffffffffffffffULL>>(64-8*m_access_size));
		ireg_t b0_masked=B[0]&(0xffffffffffffffffULL>>(64-8*m_access_size));
		ireg_t a1_masked=A[1]&(0xffffffffffffffffULL>>(64-8*m_access_size));
		ireg_t b1_masked=B[1]&(0xffffffffffffffffULL>>(64-8*m_access_size));

		return (a0_masked==b0_masked && a1_masked==b1_masked);
	}

	return true; //TODO: sloppy, but this case should be rare
}

#if 0 
bool atomic_inst_t::isEqual(ireg_t* A, ireg_t* B)
{
	if (m_access_size<=8) {
		ireg_t a_masked=A[0]&(0xffffffffffffffffULL>>(64-8*m_access_size));
		ireg_t b_masked=B[0]&(0xffffffffffffffffULL>>(64-8*m_access_size));
// 		if (a_masked!=b_masked) {
// 			DEBUG_OUT("a %llx b %llx\n",a_masked,b_masked);
// 		}
		return (a_masked==b_masked);
	}
	return true; //TODO: sloppy, but this case should be rare
}

bool load_inst_t::isEqual(ireg_t* A, ireg_t* B)
{
	if (m_access_size<=8) {
		ireg_t a_masked=A[0]&(0xffffffffffffffffULL>>(64-8*m_access_size));
		ireg_t b_masked=B[0]&(0xffffffffffffffffULL>>(64-8*m_access_size));
// 		if (a_masked!=b_masked) {
// 			DEBUG_OUT("a %llx b %llx\n",a_masked,b_masked);
// 		}
		return (a_masked==b_masked);
	}
	return true; //TODO: sloppy, but this case should be rare
}
#endif

/*------------------------------------------------------------------------*/
/* atomic_inst                                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
atomic_inst_t::atomic_inst_t( static_inst_t *s_inst, 
        int32 window_index,
        pseq_t *pseq,
        abstract_pc_t *at, fault_stats* fault_stat )
: store_inst_t( s_inst, window_index, pseq, at, fault_stat ) {

	// use default dynamic inst constructor
	m_depend   = NULL;
	m_fetch_at = *at;
	patch_store_value = false;
	update_only = false;
	llb_read_pointer = 0;

    if(MULTICORE_DIAGNOSIS_ONLY) {
	    if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
		    //set the read pointer in the load instruciton
		    llb_read_pointer = m_pseq->getLLB()->getReadPointer();
		    //update the read pointer in the llb
		    //DEBUG_OUT("%d:%d ",m_pseq->getID(), getSequenceNumber());
		    //DEBUG_OUT("opcode=%s, read_pointer=%d, access_size=%d\n",decode_opcode(s->getOpcode()), m_pseq->getLLB()->getReadPointer(), getAccessSize());
		    int access_size = ((m_access_size+7)/8);
		    m_pseq->getLLB()->updateReadPointer(access_size) ;
		    //DEBUG_OUT("%d: read_pointer after update=%d\n",m_pseq->getID(), m_pseq->getLLB()->getReadPointer());

	    }
    }


}

//***************************************************************************
atomic_inst_t::~atomic_inst_t() {
}

//**************************************************************************
void 
atomic_inst_t::replay( void ) {
    m_pseq->raiseException( EXCEPT_MEMORY_REPLAY_TRAP,
            m_pseq->getIwindow()->iwin_decrement( getWindowIndex() ),
            (enum i_opcode) s->getOpcode(),
            &m_fetch_at, 0 , /* penalty */ 0 );
}

//**************************************************************************
void
atomic_inst_t::lsqBypass( void ) {

    STAT_INC( m_pseq->m_stat_bypasses );

#ifdef DEBUG_LSQ
    m_pseq->out_info("Atomic lsqBypass: index:%d\n", getWindowIndex() );
#endif
    ASSERT(m_depend->getStoreValueReady());

    // get the maximum amount of data that can be bypassed from the other
    // instruction.
    byte_t  min_access_size = MIN( m_access_size, m_depend->getAccessSize() );

    // convert the access size (in bytes) to register size (8 byte) quantities
    min_access_size = ((min_access_size + 7)/ 8);
    for ( int i = 0; i < min_access_size; i++ ) {
        m_data_storage[i] = m_depend->getData()[i];
    }
    //LXL: The lower address word is actually at the upper half
	if (m_depend->getAccessSize()>m_access_size) {
		if (m_depend->getAccessSize()==8) {
			m_data_storage[0] = m_data_storage[0] >> ((8-m_access_size)*8);
		} else if (m_depend->getAccessSize()==4) {
			m_data_storage[0] = m_data_storage[0] >> ((4-m_access_size)*8);
		} else if (m_depend->getAccessSize()==2) {
			m_data_storage[0] = m_data_storage[0] >> ((2-m_access_size)*8);
		}
    }
    // don't set m_data_valid here because the atomic instruction can write
    //           values on execution
    markEvent( EVENT_LSQ_BYPASS );

    // Make sure all registers are ready before calling complete
    for (int32 i = 0; i < SI_MAX_SOURCE; i++) {
        reg_id_t &source = getSourceReg(i);
        if ( !source.getARF()->isReady( source ) ) {
            source.getARF()->waitResult( source, this );
            SetStage( EARLY_ATOMIC_STAGE );
            return;
        }
    }

    Complete();
}

//**************************************************************************
void
atomic_inst_t::lsqWait( void ) {
    STAT_INC( m_pseq->m_stat_early_store_bypass );
    SetStage( LSQ_WAIT_STAGE );

    /* put ourselves in the wait list for the store value register */
    m_depend->addDependentLoad( this );
}

//*************************************************************************
void
atomic_inst_t::addrOverlapWait(store_inst_t * depend){
    //wait until Store retires
    SetStage( ADDR_OVERLAP_STALL_STAGE );
    depend->addOverlapLoad(this);
}

//**************************************************************************
void
atomic_inst_t::Retire( abstract_pc_t *a ) {
	// update statistics
	STAT_INC( m_pseq->m_stat_atomics_retired );
	memory_inst_t::Retire(a);

	// If the atomis inst is corrupted, assume it is writing a
	// bad value to memory. If not, it is overwriting a previously
	// bad memory value.
	bool corrupts_memory = false ;
	for (int i = 0; i < SI_MAX_SOURCE; i ++) {
		reg_id_t &reg = getSourceReg(i);
		if (!reg.isZero()) {
			corrupts_memory |= reg.getARF()->isCorrupted(reg) ;
		}
	}
	if(corrupts_memory) {
		// printRetireTrace("") ;
		debugio_t::addCorruptAddress(getAddress(),getPrivilegeMode()) ;
		this->setCorrupted() ;
		for (int i = 0; i < SI_MAX_DEST; i ++) {
			reg_id_t &reg = getDestReg(i);
			if (!reg.isZero()) {
				reg.getARF()->setCorrupted(reg) ;
			}
		}
		// Record the type of instruction as faulty
		if(RECORD_FAULTY_INST) {
			m_fs->addFaultyInstruction(decode_opcode((enum i_opcode)s->getOpcode())) ;
		}
	} else {
		debugio_t::removeCorruptAddress(getAddress(),getPrivilegeMode()) ;
	}

#if TRACK_MEM_SHARING
	int priv = getPrivilegeMode();
	if(debugio_t::isShared(m_physical_addr,priv )) {
		uint32 mapped_value = debugio_t::getMappedCoreID(m_physical_addr,getPrivilegeMode());
		if(mapped_value == debugio_t::getFaultyCore() && getSequencer()->getID() != debugio_t::getFaultyCore()) {
			struct addr_info temp;
			temp.core_id = getSequencer()->getID();
			temp.rw = 1;
			temp.trap_level = m_tl;
			temp.pc = getVPC();
			temp.corrupted = false;
			temp.val = value;

			debugio_t::add_shared_address(m_physical_addr, getPrivilegeMode(), temp);

		}
	}
#endif // TRACK_MEM_SHARING

	if( (getTrapType() != Trap_Mem_Address_Not_Aligned ) && m_data_valid ) {
		// If the retiring store is faulty, then flush this data
		// into memory. Also, for address faults (AGEN/LSQ), make sure
		// that we re-write the old value into the old address as simics
		// would've now over-written it with teh correct value
		if( getFault() ) {
		    ireg_t  data_new_loc[MEMOP_MAX_SIZE];
		    bool data_new_valid = m_pseq->readPhysicalMemory(m_physical_addr,
				    (int) m_access_size,
				    &data_new_loc[0] );
		    if (!isEqual(data_new_loc,m_data_storage)) {
			    if (LXL_STORE_DEBUG) {
				    DEBUG_OUT("store corrupt s %d n %llx o %llx\n",
							m_access_size,data_new_loc[0],m_data_storage[0]);
					DEBUG_OUT("instr opcode %s\n",decode_opcode(s->getOpcode()));
			    }
			    corrupting_store=true;
			    setMemFault() ;
		    }
#if defined( LXL_SNET_RECOVERY) && USE_TBFD
		    if (!m_pseq->recovered || (!LXL_NO_CORRUPT && m_pseq->recovered)) {
			    m_pseq->writePhysicalMemory( m_physical_addr,
					    (int) m_access_size,
					    getData() ) ;
		    }
#else //define(LXL_SNET_RECOVERY) && USE_TBFD
			ireg_t  data_new_loc_test[MEMOP_MAX_SIZE];
			bool data_new_valid_test = m_pseq->readPhysicalMemory(m_physical_addr,
					(int) m_access_size,
					&data_new_loc_test[0] );
			if (!isEqual(data_new_loc_test,m_data_storage)) {
				if (LXL_STORE_DEBUG) {
					DEBUG_OUT("store corrupt s %d n %llx o %llx\n",m_access_size,data_new_loc_test[0],m_data_storage[0]);
				}
				corrupting_store=true;
			}

			if(MULTICORE_DIAGNOSIS_ONLY) { 
				if(m_pseq->getMulticoreDiagnosis()->dont_run_core_id() != m_pseq->getID()) {
					if(m_pseq->getMulticoreDiagnosis()->isLoggingPhase()) {
						m_pseq->writePhysicalMemory( m_physical_addr,
								(int) m_access_size,
								getData() ) ;
					}
				}
			} else {
				m_pseq->writePhysicalMemory( m_physical_addr,
						(int) m_access_size,
						getData() ) ;
			}

#endif //define(LXL_SNET_RECOVERY) && USE_TBFD

			if( m_old_data_valid  && (m_old_data_valid!=((my_addr_t)-1) )) {
				if( oldAddressAligned() ) {
					ireg_t  data_old_loc[MEMOP_MAX_SIZE];
					bool data_old_valid = m_pseq->readPhysicalMemory(m_old_physical_addr,
							(int) m_access_size,
							&data_old_loc[0] );
					if (!isEqual(data_old_loc,m_old_data_storage)) {
						if (LXL_STORE_DEBUG) {
							DEBUG_OUT("store corrupt b %llx a %llx\n",data_old_loc[0],m_old_data_storage[0]);
						}
						corrupting_store=true;
						setMemFault() ;
					}
#if defined(LXL_SNET_RECOVERY) && USE_TBFD
					if (!m_pseq->recovered || (!LXL_NO_CORRUPT && m_pseq->recovered)) {
						m_pseq->writePhysicalMemory( m_old_physical_addr,
								(int) m_access_size,
								getOldData() ) ;
					}
#else // defined(LXL_SNET_RECOVERY) && USE_TBFD
#if LXL_TEST_CODE
					ireg_t  data_old_loc_test[MEMOP_MAX_SIZE];
					bool data_old_valid_test = m_pseq->readPhysicalMemory(m_old_physical_addr,
							(int) m_access_size,
							&data_old_loc_test[0] );
					if (!isEqual(data_old_loc_test,m_old_data_storage)) {
						if (LXL_STORE_DEBUG) {
							DEBUG_OUT("store corrupt b %llx a %llx\n",data_old_loc_test[0],m_old_data_storage[0]);
						}
						corrupting_store=true;
					}
#endif //LXL_TEST_CODE

					if(MULTICORE_DIAGNOSIS_ONLY) { 
						if(m_pseq->getMulticoreDiagnosis()->dont_run_core_id() != m_pseq->getID())
							m_pseq->writePhysicalMemory( m_old_physical_addr,
									(int) m_access_size,
									getOldData() ) ;
					} else {
						m_pseq->writePhysicalMemory( m_old_physical_addr,
								(int) m_access_size,
								getOldData() ) ;
					}

#endif // defined(LXL_SNET_RECOVERY) && USE_TBFD
					if( PRADEEP_DEBUG_LSQ ) {
						ireg_t  m_temp_data_storage[MEMOP_MAX_SIZE];
						bool m_temp_data_valid = m_pseq->readPhysicalMemory(
								m_old_physical_addr,
								(int) m_access_size,
								&m_temp_data_storage[0] );

						DEBUG_OUT( "St - %d -- ", getSequenceNumber() ) ;
						if( m_temp_data_valid ) {
							DEBUG_OUT( "Old data - Mem[%llx]", m_old_physical_addr ) ;
							DEBUG_OUT( " = %llx\n", m_temp_data_storage[0] ) ;
						} else {
							DEBUG_OUT( "Write to Mem[%llx] failed\n",
									m_old_physical_addr) ;
						}
					}
				} else { // Doesn't have a fault. Dont do anything
				}
			}
		}
	} else {
        // It can come here because we may be using functional trap to
        // update the data and hence the valid bit is not set.
        if( PRADEEP_DEBUG_LSQ ) {
            printRetireTrace( "Data not ready when instruction retiring\n" ) ;
        }
    }

	// Need to wakeup any loads that have overlapping addresses 
	wakeupOverlapLoads();

	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {

			m_pseq->getMulticoreDiagnosis()->reportLoadInstruction(m_pseq->getID());
			if(m_physical_addr != -1) {
				// patch the store value with the value just before the store instruction during TMR phase
				// This is a hack for not letting store propagate to memory
				// this is done after collecting the log, because the compareLog should have use the old value
				writeBackStoreValue();
			} else
				DEBUG_OUT(" Could not patch: NEED TO PATCH!!!!!\n");


			//dest register patching
			if(m_traptype != Trap_Use_Functional) {

				for (int i = 0; i < SI_MAX_DEST; i++) {
					reg_id_t &dest = getDestReg(i);
					if (!dest.isZero()) {
						if (dest.getPhysical() != PSEQ_INVALID_REG) {
							check_result_t result;
							result.update_only = false;
#ifdef LXL_SNET_RECOVERY
							result.recovered = false;
#endif
							result.patch_dest = true;
							//result.verbose = true;
							dest.getARF()->check(dest, system_t::inst->m_state[m_pseq->getID()], &result, false);
							//DEBUG_OUT("%d: check done\n", m_pseq->getID());
						}
					}
				}
			}
		}
	}

#ifdef HARD_BOUND
	// Do the memory tracking for the objects
	propagateObjects() ;
#endif // HARD_BOUND

	if(INST_MIX) {
		m_fs->recordInstMix("store",isCorrupted()) ;
	}
}

#ifdef HARD_BOUND
void atomic_inst_t::propagateObjects(int in_trap)
{
	ireg_t addr = getAddress() ;
	reg_id_t &src1 = getSourceReg(0) ;
	half_t src1_flat ;
	if(src1.getRtype()!=RID_INT && src1.getRtype()!=RID_INT_GLOBAL) {
		src1_flat = PSEQ_FLATREG_NWIN ;
	} else {
		src1_flat = src1.getFlatRegister() ;
	}

	reg_id_t &src2 = getSourceReg(1) ; 
	half_t src2_flat ;
	if(s->getFlag(SI_ISIMMEDIATE) || (src1.getRtype()!=RID_INT && src1.getRtype()!=RID_INT_GLOBAL)) {
		src2_flat = PSEQ_FLATREG_NWIN ;
	} else {
		src2_flat = src2.getFlatRegister() ;
		if(src1_flat !=PSEQ_FLATREG_NWIN) {
			// How to find out what the base is. Pass in src1 as the one with
			// the higher value. Perhaps that is the base
			if(src1.getARF()->readInt64(src1) < src1.getARF()->readInt64(src2)) {
				half_t temp = src1_flat ;
				src1_flat = src2_flat ;
				src2_flat = temp ;
			}
		}
	}

	reg_id_t &dest = getDestReg(0) ;
	half_t dest_flat ;
	if(dest.getRtype()!=RID_INT && dest.getRtype()!=RID_INT_GLOBAL) {
		dest_flat = PSEQ_FLATREG_NWIN ;
	} else {
		dest_flat = dest.getFlatRegister() ;
	}

	// Do the sophisticated hard bound check
	if(debugio_t::handleObjectStore(dest_flat, src1_flat, src2_flat, addr, getPrivilegeMode(), in_trap)!=0){ // Something wrong
		if(strcmp(debugio_t::getAppName(), "gcc")==0 &&
				(getVPC()>=0x7fffffff7ff00ff4 && getVPC()<=0x7fffffff7ff0135c)) {
			// This is known to be false positive! TODO - Fix this!
		} else {
			printRetireTrace("Fail on: ") ;
			int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
			m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
					getSequenceNumber(),
					"Range_violation",
					0x207,getVPC(),
					getPrivilegeMode(), 0,
					(uint16)trap_level, m_pseq->getID()) ;
			HALT_SIMULATION ;
		}
	} 
	// if( !debugio_t::isMemValueinObj(getAddress(), dest.getARF()->readInt64(dest)) ) {
	// 	printRetireTrace("Fail on: ") ;
	// 	int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
	// 	m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
	// 			getSequenceNumber(),
	// 			"Range_violation",
	// 			0x208,getVPC(),
	// 			getPrivilegeMode(), 0,
	// 			(uint16)trap_level, m_pseq->getID()) ;
	// 	HALT_SIMULATION ;
	// }

	if(!m_pseq->getSymptomSeen()) {
		// Do the basic hard bound check
		if(!getPrivilegeMode() &&
				(getAddress()>=0xffffffff80000000ULL || // Last 4GB is reserved
				 getAddress()<0x100000000) ){ // Below this is not used!
			// This check is there here because you may call this when this inst is about
			// to take a trap
			DEBUG_OUT("Address in code segment: 0x%llx\n", getAddress()) ;
			int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
			m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
					getSequenceNumber(),
					"Address_out_of_range",
					0x206,getVPC(),
					getPrivilegeMode(), 0,
					(uint16)trap_level, m_pseq->getID());
			HALT_SIMULATION ;
		}
		if(!getPrivilegeMode() && debugio_t::isHeapAddress(getAddress()) ) { // Application does heap access
			bool valid_access = debugio_t::isValidObject(getAddress()) ;
			if(!valid_access) {
				printRetireTrace("No valid object: ") ;
				getSequencer()->outSymptomInfo(
						(uint64) m_pseq->getLocalCycle(),
						getSequencer()->getRetInst(),
						"Range_check_violation",
						0x206, getVPC(),
						getPrivilegeMode(),0,0,m_pseq->getID()) ;
				HALT_SIMULATION ;
			}
		}
	}
}
#endif // HARD_BOUND

bool atomic_inst_t::collectLLB()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_pseq->getMulticoreDiagnosis()->isLoggingPhase()) {

			for (int i = 0; i < SI_MAX_DEST; i++) {
				reg_id_t &dest    = getDestReg(i);

				// if the register isn't the %g0 register --
				if (!dest.isZero()) {
					if (dest.getPhysical() != PSEQ_INVALID_REG) {
						//log in the load buffer
						int llb_full = m_pseq->addLLBEntry(m_physical_addr, dest.getARF()->readInt64(dest));
						if(llb_full)
							return false;
						//DEBUG_OUT("%d: address=%llx\n", m_pseq->getID(), m_physical_addr);
					}
				}
			}
		} 
	}
	return true;
}

//**************************************************************************
void atomic_inst_t::copyStoreValue()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
			if(m_physical_addr == -1) {
				m_pseq->out_info("   access: %d", m_access_size );
				m_pseq->out_info("   addr  : 0x%0llx", m_address );
				m_pseq->out_info("   phys a: 0x%0llx\n", m_physical_addr );

				DEBUG_OUT(" Physical Address is -1\n");
				m_pseq->getMulticoreDiagnosis()->stopDiagnosis();
			} else {
				m_pseq->readPhysicalMemory( m_physical_addr,
						(int) m_access_size,
						&previous_value[0] );
				patch_store_value = true;
			}
		}
	}
}

void atomic_inst_t::writeBackStoreValue()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
			if(patch_store_value) {
				if(SIVA_LOAD_BUFFER_DEBUG) {
					DEBUG_OUT(" Patching Store value\n");
					DEBUG_OUT("%d:St: ", m_pseq->getID());
					DEBUG_OUT("addr=%llx, ", m_physical_addr);
					DEBUG_OUT("size=%llx, ", m_access_size);
					DEBUG_OUT("prev_value=%llx \n", previous_value[0]);
				}

				m_pseq->writePhysicalMemory( m_physical_addr,
						(int) m_access_size,
						&previous_value[0] ) ;
				patch_store_value = false;
			}
		}
	}
}


bool atomic_inst_t::setLLBReadPointer(int read_pointer)
{
	llb_read_pointer = read_pointer;
	return true;
}
//this function does much more than just matching load buffer with the memory value
//it does the work to keep the execution livelock free by checking and reporting to multicore_diagnosis_t
bool atomic_inst_t::matchesLoadBuffer()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {

		if(m_traptype != Trap_Use_Functional) {
			int access_size = ((m_access_size+7)/8);

			if(m_pseq->getMulticoreDiagnosis()->isLoggingPhase()) {
				ireg_t value[MEMOP_MAX_SIZE];
				//if inst is correctly implemented
				m_pseq->readPhysicalMemory( m_physical_addr,
						(int) m_access_size,
						&value[0] );

				// if inst is wrongly implemented and if there is no fault
				// check if there is no fault and get the value from the simics and log it
				// needed? i dont think so..

				//log in the load buffer
				bool is_full = m_pseq->addLLBEntry(m_physical_addr, &value[0], access_size);

				if(is_full)
					return false;
				else 
					return true;

			} else if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {

				if(SIVA_LOAD_BUFFER_DEBUG) {
					DEBUG_OUT("%d:LD: address = %llx",m_pseq->getID(), m_physical_addr);
					DEBUG_OUT(" value = %llx\n", m_data_storage[0]);
				}
				ireg_t value_buf[MEMOP_MAX_SIZE];
				for(int i=0; i<MEMOP_MAX_SIZE; i++) {
					value_buf[i] = -1;
				}

				//if hit in loadBuffer
				if(m_pseq->getLLB()->isLLBHit(m_physical_addr, m_pseq->getID())) {

					if(SIVA_LOAD_BUFFER_DEBUG)
						DEBUG_OUT("%d: LLB Hit\n",m_pseq->getID());

					int num_llb_entries_removed = 0;
					// read the value from buffer
					for(int i=0; i<access_size; i++) {
						num_llb_entries_removed++;
						value_buf[i] = m_pseq->getLLB()->readFront(i);

						if(SIVA_LOAD_BUFFER_DEBUG) 
							DEBUG_OUT(" value_buf[%d]=%llx ",i, value_buf[i]);
					}

					bool is_equal = (llb_read_pointer == m_pseq->getLLB()->getFront());
					//if(value_buf[0] != m_data_storage[0]) {
					//	is_equal = false;
					//}


					//compare the value of memory and pipeline reg
					//if(isEqual(value_buf,m_data_storage)) {
					if(is_equal) {
						if(SIVA_LOAD_BUFFER_DEBUG)
							DEBUG_OUT("%d:Value in buffer matched the value from memory\n", m_pseq->getID());
						//perfect the load has matched the value from the LoggingStep

						// remove the front element from the buffer
						for(int i=0; i<access_size; i++) {
							m_pseq->getLLB()->pop();
						}

						return true;
					} else {

						if(SIVA_LOAD_BUFFER_DEBUG)
							DEBUG_OUT(" reverting back the read buffer\n");

						// if no one is squashing on this address, then patch it to memory
						m_pseq->getLLB()->resetReadPointer();
					}

				} else {
					// Load miss in buffer: Continue execution (continue retiring instrucitons)
					// during the comparison point at the end of the TMR step, this miss would result in 
					// divergence. Hence, report address not found (miss) and conitnue
					m_pseq->m_multicore_diagnosis->reportAddressNotFound(m_pseq->getID(), m_physical_addr);
					if(SIVA_LOAD_BUFFER_DEBUG)
						DEBUG_OUT("%d: reporting a miss on address=%llx\n", m_pseq->getID(), m_physical_addr);

					return true;
				}
				return false;

				} else	//isTMRPhase()
					return true;
			}
		}
	return true;

}

bool atomic_inst_t::checkReadRegValue()
{
	if(m_check_at_retire) {
		if(m_check_at_retire_value != getSourceReg(3).getARF()->readInt64(getSourceReg(3))) {
			DEBUG_OUT(" Squashing because different value read during Execute %llx, %llx\n", m_check_at_retire_value, getSourceReg(3).getARF()->readInt64(getSourceReg(3)));
			return false;
		}
	}
	return true;

}

//**************************************************************************
void 
atomic_inst_t::Complete() {

    // update the ASI access statistics in the sequencer
    if (s->getFlag( SI_ISASI )) {
        m_pseq->m_asi_at_stat[m_asi]++;
    }

    /* the value wasn't in the load/store queue, so get the value from
       the architected memory state */
	if ( CONFIG_IN_SIMICS ) {
		// we may have already gotten the data through the lsq
		if ( isCacheable() ) {

			if (getEvent(EVENT_LSQ_BYPASS)) {
				// already got data through lsq ... don't read from memory
				m_data_valid = true;
			} else {
				// read the value from physical memory
				if( getTrapType() != Trap_Mem_Address_Not_Aligned ) {

					m_data_valid = m_pseq->readPhysicalMemory( m_physical_addr,
							(int) m_access_size,
							getData() );

					if(MULTICORE_DIAGNOSIS_ONLY) {
						//DEBUG_OUT("\n %d: access_size=%d\n", m_pseq->getID(), m_access_size);
						if(m_pseq->getMulticoreDiagnosis()->isTMRPhase()) {
							//DEBUG_OUT("\n %d: access_size=%d value=", m_pseq->getID(), m_access_size);
							int access_size = ((m_access_size+7)/8);
							for(int i=0; i<access_size; i++) {
								m_data_storage[i] = m_pseq->getLLB()->readFromPointer(llb_read_pointer+i);
								//DEBUG_OUT("%llx, ", m_data_storage[i]);
							}
							m_data_valid = true;
							//DEBUG_OUT("\n");
						}
					}
				}
			}
		}
	} else {

		// read the value from the memory trace
		memtrace_t *mt = m_pseq->getMemtrace();
		//  m_pseq->out_log("local time %d = %d address 0x%0llx\n", m_pseq->getRetiredICount(), mytime, m_address);
		// plus 2 as the time is at the retire time ... not the current time :)
		m_data_valid = mt->scanTransaction( m_pseq->getRetiredICount() + 2,
				m_address, getVPC(),
				s->getInst(), m_physical_addr,
				m_access_size, getData() );
	}

    memory_inst_t::Execute(); 

	// Pradeep - testing whether the atomic instructions' write is undone
    if( getTrapType() != Trap_Mem_Address_Not_Aligned ) {
        if( getMemFault() && (m_old_physical_addr!=((my_addr_t)-1) )) {
            m_old_data_valid = m_pseq->readPhysicalMemory( m_old_physical_addr,
                    (int) m_access_size,
                    getOldData() );
			// DEBUG_OUT("[0x%llx] old_data - 0x%llx\n", getVPC(), getOldData()) ;
            // m_old_data_valid = true ;
        }
    }
    /* must call wakeup _after_ execute:
     *      wakeup checks the store stage (must be completed)
     *      before bypassing the value */
    wakeupDependentLoads();
}

//***************************************************************************
void atomic_inst_t::printDetail( void )
{
    m_pseq->out_info("atomic_inst_t\n");
    store_inst_t::printDetail();
    if (m_depend) {
        m_pseq->out_info("   depend: %d\n", m_depend->getWindowIndex());
    }
}

/*------------------------------------------------------------------------*/
/* prefetch_inst                                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
prefetch_inst_t::prefetch_inst_t( static_inst_t *s_inst, 
        int32 window_index,
        pseq_t *pseq,
        abstract_pc_t *at, fault_stats *fault_stat )
: memory_inst_t( s_inst, window_index, pseq, at, fault_stat ) {
}

//***************************************************************************
prefetch_inst_t::~prefetch_inst_t() {
}

//**************************************************************************
void 
prefetch_inst_t::Execute( void ) {
    STAT_INC( m_pseq->m_stat_prefetches_exec );
    // CM FIX: should possibly prefetch in exclusive
    trap_type_t t = addressGenerate( OPAL_LOAD );

    if (t != Trap_NoTrap) {
        /* couldn't correctly generate address -- we're done! */
        SetStage( COMPLETE_STAGE );
        return;
    }

    /* can't complete I/O accesses, and moreover they are uncacheable */
    if ( m_physical_addr != (pa_t) -1 &&
            isIOAccess( m_physical_addr ) ) {
        SetStage( COMPLETE_STAGE );
        STAT_INC( m_pseq->m_stat_count_io_access );
        return;
    }

    if ( isCacheable() ) {
        ASSERT( m_physical_addr != (pa_t) -1 );
        if (!accessCache())
            return;
    } // is cacheable

    Complete();
}

//**************************************************************************
bool
prefetch_inst_t::accessCache( void ) {
    if (CONFIG_WITH_RUBY) {
#ifdef DEBUG_RUBY
        m_pseq->out_log("PC: 0x%0llx PREFETCH: 0x%0llx\n",
                getVPC(), m_physical_addr );
#endif
        rubycache_t *rcache = m_pseq->getRubyCache();

        // CM fix: OPAL_LOAD should depend on type of prefetch
        ruby_status_t status = rcache->prefetch( m_physical_addr, OPAL_LOAD,
                getVPC(), (m_pstate >> 2) & 0x1 );
        if (status == NOT_READY) {
            // NOT_READY cache status causes polling using event queue
            m_pseq->postEvent( this, 1 );
            STAT_INC(m_pseq->m_num_cache_not_ready);
            SetStage(CACHE_NOTREADY_STAGE);
            return false;
        }
    } else {
        /* do a demand prefetch to the cache hierarchy */
        cache_t *dcache = m_pseq->getDataCache();
        if ( !dcache->TagCheck(m_physical_addr) )
            dcache->Prefetch( m_physical_addr);
    }
    return true;
}

//**************************************************************************
void
prefetch_inst_t::Retire( abstract_pc_t *a ) {
    // update statistics
    STAT_INC( m_pseq->m_stat_prefetches_retired );
    memory_inst_t::Retire(a);
}

//***************************************************************************
void 
prefetch_inst_t::Complete( void ) {
    // update the ASI access statistics in the sequencer
    if (s->getFlag( SI_ISASI )) {
        // CM FIX: not tracking prefetch ASI stats yet.
        // m_pseq->m_asi_rd_stat[m_asi]++;
    }

    // execute the prefetch instruction (change to retire-ready instruction)
    memory_inst_t::Execute(); 
}

//***************************************************************************
void prefetch_inst_t::printDetail( void )
{
    m_pseq->out_info("prefetch_inst_t\n");
    memory_inst_t::printDetail();
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

//***************************************************************************
uint16 memop_getASI( uint32 inst, reg_id_t asi_reg, bool *is_block_load )
{
    uint16 asi;
    // casa, casxa must set isimmediate for address generation
    //    so (s->getFlag(SI_ISIMMEDIATE)) doesn't work here
    if ( maskBits32( inst, 13, 13 ) == 1 ) {
        /// uses the asi register
        asi = asi_reg.getARF()->readInt64( asi_reg );
    } else {
        /// read the asi from the static instruction
        asi = maskBits32( inst, 12, 5 );
    }

    // determine if the ASI is a block load ASI or not.
    char upper_asi = (asi >> 4) & 0xf;
    *is_block_load = ( (upper_asi == 0x7) ||
            (upper_asi == 0xf) ||
            (upper_asi == 0xe) );
    return (asi);
}
