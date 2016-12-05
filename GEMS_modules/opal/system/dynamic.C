
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
 * FileName:  dynamic.C
 * Synopsis:  Represents an in-flight instruction in the processor
 * Description: Contains information related to an instruction being executed,
 *              which is distinct from the generic decoded instruction.
 * Author:    zilles
 * Maintainer:cmauer
 * Version:   $Id: dynamic.C 1.75 06/02/13 18:49:16-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include "hfa.h"
#include "hfacore.h"

#include "lsq.h"
#include "exec.h"
#include "scheduler.h"
#include "dynamic.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/
//#define ALEX_RAT_DEBUG 

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

//-------------- GATE LEVEL SIMULATION --------------------------------------//
// FIXME - Where should these objects lie? memop.C also accesses these objects
#ifdef LL_INT_ALU
//structuralModule design;
#endif // LL_DECODER
extern faultSimulator fSim;

//----------------------------------------------------------------------------//

/// memory allocator for dynamic.C
listalloc_t dynamic_inst_t::m_myalloc;

/// The static jump table for the flow inst class
pmf_dynamicExecute dynamic_inst_t::m_jump_table[i_maxcount];

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/
/**
 *  Constructor:
 *     assumes the dynamic instruction is in the Fetch state,
 *     resets the counters for the object.
 */
//**************************************************************************
dynamic_inst_t::dynamic_inst_t( static_inst_t *s_inst, 
        int32 window_index,
        pseq_t *pseq,
        abstract_pc_t *at,
	fault_stats *fault_stat )
{
    this->m_pseq           = pseq;
    this->s                = s_inst;
    this->m_virtual_address= at->pc;
    this->m_windex         = window_index;
    this->seq_num          = pseq->stepSequenceNumber();
    this->m_fetch_cycle    = pseq->getLocalCycle();
    this->m_complete_cycle = 0;
    this->m_fs = fault_stat;

    /* instruction was just fetched */
    this->m_stage  = FETCH_STAGE;
    this->m_events = 0;
    this->m_traptype = (uint16) Trap_NoTrap;

	has_fault = false ;
	corrupted = false ;
	activate_fault = false ;


    // clear the event times
    for (int i = 0; i < (int) EVENT_TIME_NUM_EVENT_TIMES; i++) {
        m_event_times[i] = 0;
    }

    // increment the static instruction reference counter
    s_inst->incrementRefCount();

    // CM It should be possible to optimize the copying of register from
    //       the static instruction here (particularly "tofree")
    uint16 pstate  = at->pstate;
    uint16 cwp     = at->cwp;
    uint16 gset    = at->gset;
	
	

    // get priv info from pstate
    m_priv = ((pstate & 0x4) == 0x4);

    if ( !(cwp < NWINDOWS) ) {
        m_pseq->out_info("%d:dynamic_t:: constructor: warning: cwp is %d, set to %d\n",
                m_pseq->getID(), cwp, (cwp & (NWINDOWS - 1)));
        cwp &= (NWINDOWS - 1);
    }
    if ( !(gset <= REG_GLOBAL_INT) ) {
        m_pseq->out_info("%d:dynamic_t:: constructor: warning: gset is %d\n", m_pseq->getID(), gset);
        gset = REG_GLOBAL_INT;
    }

	// LXL: Without the following patch, the simulator seg faults, as
	// expected when you ask it to access bogus regfile
#define LXL_PATCH_CORRUPTED_REG
	

    reg_box_t &rbox = pseq->getRegBox();
    for (int i = 0; i < SI_MAX_SOURCE; i++) {
        reg_id_t &id = s_inst->getSourceReg(i);
        m_source_reg[i].copy( id, rbox );
        m_source_reg[i].setVanillaState( cwp, gset );

#if defined(LL_DECODER) && defined(LXL_PATCH_CORRUPTED_REG)
		if (m_source_reg[i].getARF()==NULL) {
			//DEBUG_OUT("source %d bad arf, illegal\n",i);
			setTrapType(Trap_Illegal_Instruction);
			id.setRtype(RID_NONE);
			m_source_reg[i].setRtype(RID_NONE);
			m_source_reg[i].copy( id, rbox );
			m_source_reg[i].setVanillaState( cwp, gset );
			assert(m_source_reg[i].getARF()!=NULL);
			has_fault=true;
		}
#endif

    }

    for (int i = 0; i < SI_MAX_DEST; i++) {
        reg_id_t &id = s_inst->getDestReg(i);
        m_dest_reg[i].copy( id, rbox );
        m_dest_reg[i].setVanillaState( cwp, gset );
        m_tofree_reg[i].copy( id, rbox );
        m_tofree_reg[i].setVanillaState( cwp, gset );
        m_dest_ready[i] = false;


#if defined(LL_DECODER) && defined(LXL_PATCH_CORRUPTED_REG)
		if (m_dest_reg[i].getARF()==NULL) {
			//DEBUG_OUT("dest %d bad arf, illegal\n",i);
			setTrapType(Trap_Illegal_Instruction);
			id.setRtype(RID_NONE);
			m_dest_reg[i].setRtype(RID_NONE);
			m_dest_reg[i].copy( id, rbox );
			m_dest_reg[i].setVanillaState( cwp, gset );
			assert(m_dest_reg[i].getARF()!=NULL);
			has_fault=true;
		}
#endif
    }

    if (m_fs->getFaultType()==DECODER_INPUT_FAULT && s_inst->getChangedDest() ) {
        for (int i = 0; i < SI_MAX_DEST; i++) {
            reg_id_t &id = s_inst->getOrigDestReg(i);
            m_orig_dest_reg[i].copy( id, rbox );
            m_orig_dest_reg[i].setVanillaState( cwp, gset );
        }

    }

    has_fault |= s_inst->getFault() ;
	corrupted |= s_inst->getFault() ;
    if( s_inst->getDecodeFail() ) {
        setTrapType(Trap_Illegal_Instruction);
    }

	m_control_fault = false ;
    m_faulty_rob = false;
    first_rat_err_on_read = false;
	m_agen_id=-1;
	m_result_bus_id=-1;
	m_is_stuck=false;
	stuck_src=-1;

	//squash_pipeline = false;

	reg_mismatch.clear();

#ifdef LL_INT_ALU
	this->infectedALU = false;
  	this->uMaskedALU = false;
	this->inst_arith = false;
	this->inst_logic = false;
	this->infectedResult = 0;
	this->goldenResult = 0; 

#endif

#ifdef SW_ROLLBACK
	if (m_pseq->m_op_mode==RECOVERING) {
		if (m_dest_reg[0].getRtype()==RID_INT) {
			m_dest_reg[0].setVanillaState(m_source_reg[1].getVanilla(),0);
			m_tofree_reg[0].setVanillaState(m_source_reg[1].getVanilla(),0);
			m_source_reg[1].setVanilla(0);
			m_source_reg[1].setRtype(RID_INT_GLOBAL);
		} else if (m_dest_reg[0].getRtype()==RID_INT_GLOBAL) {
			m_dest_reg[0].setVanillaState(0, m_source_reg[1].getVanilla());
			m_tofree_reg[0].setVanillaState(0, m_source_reg[1].getVanilla());
			m_source_reg[1].setVanilla(0);
		} 
	}
#endif
}

//**************************************************************************
dynamic_inst_t::~dynamic_inst_t()
{
     // if the static instruction is marked, free it
    s->decrementRefCount();
    if (s->getFlag( SI_TOFREE )) {
        if (s->getRefCount() == 0) {
            delete s;
        }
        // CM FIX: no longer trying to figure out if ref counting is a good idea
        //ERROR_OUT("warning: ref counting is a good idea!\n");
    }
}

//**************************************************************************
    void 
dynamic_inst_t::Decode( uint64 issueTime )
{
    /*
     * for each source, get the mapped register 
     */
    for (int i = 0; i < SI_MAX_SOURCE; i++) {
        reg_id_t &source = getSourceReg(i);
        source.getARF()->readDecodeMap( source );
    }

#ifdef RENAME_CHECKER
#ifdef VAR_TRACE
	reg_id_t &s1 = getSourceReg(0) ;
	reg_id_t &s2 = getSourceReg(1) ;
	if( !s1.isZero() && !s2.isZero() && (s1.getRtype()==s2.getRtype()) ) {
		TRACE_OUT("l1\n%d\n",s1.getFlatRegister());
		TRACE_OUT("p1\n%d\n",s1.getPhysical());
		TRACE_OUT("l2\n%d\n",s2.getFlatRegister());
		TRACE_OUT("p2\n%d\n",s2.getPhysical());
		TRACE_OUT("\n") ;
	}
#else //VAR_TRACE
	// Any given physical register can have exactly one logical mapping
	// That is, for this inst, if two logical registers are same, then
	// physical reg is same, else they are different.
	// The condition that should always hold true is
	// (s1 & s2 & (s1.type==s2.type))=>
	// ((s1.flatregister==s2.flatregister)&(s1.physical==s2.physical) |
	//	(s1.flatregister!=s2.flatregister)&(s1.physical!=s2.physical))
	// if(m_pseq->getArchMismatch()) {
		for(int i=0; i<SI_MAX_SOURCE; ++i) {
			reg_id_t &s1 = getSourceReg(i) ;
			if(s1.isZero()) {
				continue ;
			}
			for(int j=i+1; j<SI_MAX_SOURCE; ++j) {
				reg_id_t &s2 = getSourceReg(j) ;
				if(s2.isZero()) {
					continue ;
				}

#ifdef RENAME_CHECKER_TRACE
				FAULT_OUT("%d:",getSequenceNumber()) ;
				FAULT_OUT("%d\t", s1.getRtype()) ;
				FAULT_OUT("%d\t", s1.getFlatRegister()) ;
				half_t phys_reg = s1.getPhysical() ;
				if(s1.getRtype()!=6) { // double registers are two regs
					FAULT_OUT("%d:", phys_reg) ;
				} else { // print it as two regs
					half_t reg1 = (phys_reg >> 8) & 0xff;
					half_t reg2 = phys_reg & 0xff;
					FAULT_OUT("%d,", reg1) ;
					FAULT_OUT("%d:", reg2) ;
				}
				FAULT_OUT("%d\t", s2.getRtype()) ;
				FAULT_OUT("%d\t", s2.getFlatRegister()) ;
				phys_reg = s2.getPhysical() ;
				if(s2.getRtype()!=6) { // double registers are two regs
					FAULT_OUT("%d\n", phys_reg) ;
				} else {
					half_t reg1 = (phys_reg >> 8) & 0xff;
					half_t reg2 = phys_reg & 0xff;
					FAULT_OUT("%d,", reg1) ;
					FAULT_OUT("%d\n", reg2) ;
				}
#endif //RENAME_CHECKER_TRACE

				if( s1.getRtype()!=s2.getRtype() ) {
#ifdef DEBUG_RENAME_CHECKER
					// 	DEBUG_OUT("Diff type [%d] ", getSequenceNumber()) ;
					// 	DEBUG_OUT("s1 - %d, ", s1.getRtype()) ;
					// 	DEBUG_OUT("s2 - %d\n", s2.getRtype()) ;
#endif //  DEBUG_RENAME_CHECKER
					continue ;
				}
				if( s1.getFlatRegister() == s2.getFlatRegister() ) {
					// The logicals are same => physicals have to be same
					if(s1.getPhysical()!=s2.getPhysical()) {
#ifdef DEBUG_RENAME_CHECKER
						printRetireTrace("") ;
						DEBUG_OUT("Sources type = %d\n", s1.getRtype()) ;
						DEBUG_OUT("s1 : ") ;
						DEBUG_OUT("l:%d ", s1.getVanilla()) ;
						DEBUG_OUT("f:%d ", s1.getFlatRegister()) ;
						DEBUG_OUT("p:%d\n", s1.getPhysical()) ;

						DEBUG_OUT("s2 : ") ;
						DEBUG_OUT("l:%d ", s2.getVanilla()) ;
						DEBUG_OUT("f:%d ", s2.getFlatRegister()) ;
						DEBUG_OUT("p:%d\n", s2.getPhysical()) ;

						DEBUG_OUT("Same logical, diff phys\n" ) ;
#endif //DEBUG_RENAME_CHECKER
						getSequencer()->outSymptomInfo(
								(uint64) m_pseq->getLocalCycle(),
								getSequencer()->getRetInst(),
								"Rename_Checker",
								0x301, getVPC(),
								getPrivilegeMode(),0,0,0) ;
						HALT_SIMULATION ;
					}
				} else { // The logicals are diff => phy has to be diff
					if(s1.getPhysical()==s2.getPhysical()) {
#ifdef DEBUG_RENAME_CHECKER
						printRetireTrace("") ;
						DEBUG_OUT("Sources type = %d\n", s1.getRtype()) ;
						DEBUG_OUT("s1 : ") ;
						DEBUG_OUT("l:%d ", s1.getVanilla()) ;
						DEBUG_OUT("f:%d ", s1.getFlatRegister()) ;
						DEBUG_OUT("p:%d\n", s1.getPhysical()) ;

						DEBUG_OUT("s2 : ") ;
						DEBUG_OUT("l:%d ", s2.getVanilla()) ;
						DEBUG_OUT("f:%d ", s2.getFlatRegister()) ;
						DEBUG_OUT("p:%d\n", s2.getPhysical()) ;

						DEBUG_OUT("Diff logical, same phys\n" ) ;
#endif // DEBUG_RENAME_CHECKER
						getSequencer()->outSymptomInfo(
								(uint64) m_pseq->getLocalCycle(),
								getSequencer()->getRetInst(),
								"Rename_Checker",
								0x301, getVPC(),
								getPrivilegeMode(),0,0,0) ;
						HALT_SIMULATION ;
					}
				}
			}
		}
	// }
#endif //VAR_TRACE
#endif // RENAME_CHECKER

    /*
     * for each destination, 
     *     - get the currently mapped register (to free later)
     *     - allocate a new register to write into
     */
    for (int i = 0; i < SI_MAX_DEST; i++) {
        reg_id_t &dest    = getDestReg(i);

        if (!dest.isZero()) {

            reg_id_t &tofree = getToFreeReg(i);
            half_t logical = dest.getARF()->getLogical(dest);

#ifdef RENAME_EXPERIMENT
            // get the number of renames for this register (dest)
            half_t count   = dest.getARF()->renameCount(dest);
            m_pseq->logLogicalRenameCount( dest.getRtype(), logical, count );
#endif
            // get the previous mapping so we can free it later
            tofree.getARF()->readDecodeMap(tofree);

            // allocate a new physical register, and
            // set the logical mapping to point at it
            bool success = dest.getARF()->allocateRegister( dest );	//TODO:


            // We should mark the original fault-free register faulty.
            // On the other hand, no one will access that register.
            // Definitely want to mark the new faulty register faulty
            // to propagate the fault effect.
            if (m_fs->getFaultType()==RAT_FAULT && 
                dest.getARF()->getARFType() == INT_ARF ) {
                reg_map_t *m_decode_map = dest.getARF()->getDecodeMap() ;
                
                if (logical==m_fs->getFaultyReg()) {
					half_t old_phys = m_decode_map->getOldReg() ;
					// old_phys is set only if the fault caused a corruption
					if( old_phys != PSEQ_INVALID_REG ) {
						reg_box_t &rbox = m_pseq->getRegBox();
						m_orig_dest_reg[1].copy( dest, rbox ) ;
						m_orig_dest_reg[1].setPhysical( old_phys ) ;
						this->setFault() ;
						if (ALEX_RAT_DEBUG) {
							DEBUG_OUT("%lld dest decode faulty\n",getSequenceNumber());
						}
					}
                } else {
                    //DEBUG_OUT("bad phy %d\n");
                    if (m_decode_map->getMapping(m_fs->getFaultyReg()) == dest.getPhysical()) {
                        this->setFault();
                        if (ALEX_RAT_DEBUG) {
                            DEBUG_OUT("%d dest decode match faulty\n",getSequenceNumber());
                        }
                    }
                }
            }

            if (!success) {
                ERROR_OUT("dynamic_t:: decode: unable to allocate register:\n");
                printDetail();
            } 

            if ((m_fs->getFaultType() == DECODER_INPUT_FAULT) &&
                    (s->getChangedDest()) ) {
                for (int i=0; i<SI_MAX_DEST; i++) {
                    reg_id_t &odest= m_orig_dest_reg[i];
                    odest.getARF()->readDecodeMap(odest);
                }
            }
        }
    }

    //DEBUG_OUT("%d: decode: access_size=%d\n",m_pseq->getID(), getAccessSize());

    /* instruction scheduling priority is passed in a decode time
       (this can be updated later, but isn't CM 8/2002). */
    SetPriority( issueTime );
    SetStage(DECODE_STAGE);
}

//**************************************************************************
void 
dynamic_inst_t::testSourceReadiness() { 

    if (s->getType() == DYN_STORE) {
        // store instrs start waiting on source register 2, instead of 3 --
        //   This allows stores to issue as soon as address is available,
        //   instead of waiting for store data
        SetStage( WAIT_2ND_STAGE );

    } else if (s->getType() == DYN_ATOMIC) {
        /** Some replay traps are triggered by casa and casxa instructions.
         *  casa and casxa only use source1 for address generation, while
         *  swap(a) and ldstub(a) use both source 1 & 2 for a-gen.
         *  You could optimize the hardware this by starting cas(x)a instructions
         *  in WAIT_1ST_STAGE, and double checking that source 2&3 are ready
         *  in the memop function Execute().
         */
        if (s->getOpcode() == i_casa || s->getOpcode() == i_casxa) {
		    if ( maskBits32( s->getInst(), 13, 13 ) == 1 ) {
	
			// LXL: The simulation breaks if ASI REGISTER is not
			// ready!! Causing TLB misses incorrectly. Force it to
			// wait at 4TH stage
				SetStage( WAIT_4TH_STAGE );
			} else {
				SetStage( WAIT_1ST_STAGE );
			}
        } else {
            SetStage( WAIT_2ND_STAGE );
        }
    } else {
        SetStage( WAIT_4TH_STAGE );
    }
}

//**************************************************************************
void 
dynamic_inst_t::Schedule() { 
    switch (m_stage) {
        case WAIT_4TH_STAGE:
            {
                reg_id_t &source = getSourceReg(3);
                if ( !source.getARF()->isReady( source ) ) {
                    source.getARF()->waitResult( source, this );
#ifdef LXL_WAKEDEP_DEBUG
					DEBUG_OUT("seq %d wait on ",getSequenceNumber());
					DEBUG_OUT("reg3 %d\n",source.getPhysical());
#endif
					break;
				}
				SetStage(WAIT_3RD_STAGE);
			}
			/* fall through */
		case WAIT_3RD_STAGE:
			{
				reg_id_t &source = getSourceReg(2);
				if ( !source.getARF()->isReady( source ) ) {
					source.getARF()->waitResult( source, this );
#ifdef LXL_WAKEDEP_DEBUG
					DEBUG_OUT("seq %d wait on ",getSequenceNumber());
					DEBUG_OUT("reg2 %d\n",source.getPhysical());
#endif
					break;
				}
				SetStage(WAIT_2ND_STAGE);
			}
			/* fall through */
		case WAIT_2ND_STAGE:
			{
				reg_id_t &source = getSourceReg(1);
				if ( !source.getARF()->isReady( source ) ) {
#ifdef LXL_WAKEDEP_DEBUG
					DEBUG_OUT("seq %d wait on ",getSequenceNumber());
					DEBUG_OUT("reg1 %d\n",source.getPhysical());
#endif
					source.getARF()->waitResult( source, this );
					break;
				}
				SetStage(WAIT_1ST_STAGE);
			}
			/* fall through */
		case WAIT_1ST_STAGE:
			{
				reg_id_t &source = getSourceReg(0);
				if ( !source.getARF()->isReady( source ) ) {
					source.getARF()->waitResult( source, this );
#ifdef LXL_WAKEDEP_DEBUG
					DEBUG_OUT("seq %d wait on ",getSequenceNumber());
					DEBUG_OUT("reg0 %d\n",source.getPhysical());
#endif
					break;
				}
				SetStage(READY_STAGE);
			}
			/* fall through */
		case READY_STAGE:
			{
				/* when all registers are ready, tell the scheduler that this
				   instruction is ready */
				m_pseq->getScheduler()->schedule(this);
				break;
			}
		case EXECUTE_STAGE:
			{
				Execute();
				break;
			}
		case LSQ_WAIT_STAGE:
			/* copy the value in */
			{
				(dynamic_cast<load_interface_t *> (this))->lsqBypass();
				break;
			}
		case ADDR_OVERLAP_STALL_STAGE:
			/* Loads enter this stage when it finds an older store in the LSQ with an overlapping address.
			 * Load will wait until the store retires (and removes itself from LSQ), so that load can read
			 * value from cache.
			 * Note we call different instances of continueExecution() based on whether LD or ATOMIC inst
			 */
			{
				if(s->getType() == DYN_LOAD){
					(static_cast<load_inst_t *> (this))->continueExecution();
				}
				else if(s->getType() == DYN_ATOMIC){
					(static_cast<atomic_inst_t *> (this))->continueExecution();
				}
				else{
					ASSERT(0);
				}
				break;
			}
		case EARLY_STORE_STAGE:
			// The store data is now available for the write...
			//    a prefetch has already been launched for the block
			//    calls storeDataToCache()
			//    calls store complete() if appropriate
			{
				for (int32 i = 0; i < SI_MAX_SOURCE; i++) {
					reg_id_t &source = getSourceReg(i);
					if ( !source.getARF()->isReady( source ) ) {
						source.getARF()->waitResult( source, this );
						return;
					}
				}
				(static_cast<store_inst_t *> (this))->storeDataWakeup();
				break;
			}

		case EARLY_ATOMIC_STAGE:
			{
				// The atomic memory operation either got its memory value from LSQ or not, but
				// The value to write is not ready yet... make sure all sources are ready
				// IMPORTANT: LIke EARLY_STORE_STAGE, need to make sure we have store permissions
				//     before calling Complete(), so need to call storeDataWakeup() instead of Complete() at
				//     this point!
				for (int32 i = 0; i < SI_MAX_SOURCE; i++) {
					reg_id_t &source = getSourceReg(i);
					if ( !source.getARF()->isReady( source ) ) {
						source.getARF()->waitResult( source, this );
						return;
					}
				}
				(static_cast<store_inst_t *> (this))->storeDataWakeup();
			}
			break;

		case CACHE_MISS_STAGE:
			{            // the cache line was not present in the cache ... we await its return
				//      (for stores, the data to be written is ready)
				//    calls load complete if it was a load
				//    calls store complete if it was a store ...
				static_cast<memory_inst_t *> (this)->Complete(); 
				break;
			}
		case CACHE_NOTREADY_STAGE:
			// the cache's resources are full, the access hasn't issued yet.
			//    poll to re-issue the request.
			//    calls load complete if it was a load
			//    calls store complete if it was a store ...
			{
				if (static_cast<memory_inst_t *>(this)->accessCache() == true) {
					// cache access succeed: now move to new state: complete
					static_cast<memory_inst_t *>(this)->Complete(); 
				}
				// default: retry the request until it completes
				break;
			}
		case RETIRE_STAGE:
			{
				ERROR_OUT("error: instruction woken up in retire stage.\n");
				printDetail();
				SIM_HALT;
			}

		default:
			/* should never reach this stage */
			{
				SIM_HALT;
			}
	}

}

//**************************************************************************
void 
dynamic_inst_t::beginExecution() { 
    // record when execution takes place
    m_event_times[EVENT_TIME_EXECUTE] = m_pseq->getLocalCycle() - m_fetch_cycle;

    SetStage(EXECUTE_STAGE);
    STAT_INC(m_pseq->m_stat_total_insts);
}

#ifdef LL_INT_ALU
// Perform the low-level simulation for the integer alu alone
//***************************************************************************
void dynamic_inst_t::Execute( void ) {
	ireg_t result; // golden result
	ireg_t inj_result; // injected result [low level simulator return value]
	ireg_t op1;
	ireg_t op2;
	ireg_t source; // source register for immediate instructions 
	bool infected_instr;

	char * ptr; //strtoull
	char injResultVec[REG_WIDTH];
	char op1Vec[REG_WIDTH];
	char op2Vec[REG_WIDTH];
	char op3Vec[REG_WIDTH];
	char opShiftVec[REG_WIDTH];
	char rccVec[REG_WIDTH];
	int invert;
	int se;
	int cin;
	type opShiftVal;
	type op3Val;
	type rccVal;			
	int and_sel;
	int or_sel;
	int xor_sel;
	int move_sel;
	int null_32;
	int lsu;
	int scan_in;
	// Output mux select control
	int sel_sum; 
	int sel_shift; 
	int sel_rs3;	 
	int sel_logic;

	int ecl_shft_op32_e;
	int ecl_shft_shift4_e;
	int ecl_shft_shift1_e;
	int ecl_shft_enshift_e_l;
	int ecl_shft_extendbit_e;
	int ecl_shft_extend32bit_e_l;
	int ecl_shft_lshift_e_l;



	// record when execution takes place
	m_event_times[EVENT_TIME_EXECUTE_DONE] = m_pseq->getLocalCycle() - m_fetch_cycle;

	// call the appropriate function
	static_inst_t *si = getStaticInst();
	// execute the instruction using the jump table
	pmf_dynamicExecute pmf = m_jump_table[si->getOpcode()];

	// LXL: Check to see if dest are ready
	checkDestReadiness();

	// If the exec unit type matches the fault type and the exec unit num is 1, inject the fault
	// exec_n is the ALU number to corrupt (now set to 1)
	// exec_u is numbered from 0, look at config/config.defaults
	bool inject_me = false; 
	int32 exec_u = getExecUnit() ;
	byte_t exec_n = getExecUnitNum() ;

	// FIXME ONLY INT ALU FAULTS CONDIDERED, MORE FAULT TYPES TO BE INCLUDED
	inject_me =  ( (m_fs->getFaultType() == INTALU_OUTPUT_FAULT) &&
			(exec_u == 1) &&
			(exec_n == m_fs->getFaultyReg()) ); 

	bool andType=false ;
	bool nandType = false ;
	bool norType = false ;
	bool orType = false ;
	bool xorType = false ;
	bool xnorType = false ;
	bool addType = false ;
	bool addcType = false ;
	bool subType = false ;
	bool subcType= false ;
	bool logic = false ;
	bool arithmetic= false ;
	bool shift=false;

	switch( si->getOpcode() ) {

		case i_and : 
	    case i_andcc :
			andType = 1 ; logic = 1; break ;

		case i_or : 
	    case i_orcc :
			orType = 1 ; logic = 1 ; break ;

		case i_andn : 
	    case i_andncc :
			nandType = 1 ; logic = 1 ; break ;

		case i_orn : 
	    case i_orncc :
			norType = 1 ; logic = 1 ; break ;

		case i_xor : 
	    case i_xorcc :
			xorType = 1 ; logic = 1 ; break ;

		case i_xnor : 
	    case i_xnorcc :
			xnorType = 1 ; logic = 1 ; break ;

		case i_add : 
	    case i_addcc :
			addType = 1 ; arithmetic = 1 ; break ;

		case i_addc : 
	    case i_addccc :
			addcType = 1 ; arithmetic = 1 ; break ;

		case i_sub : 
	    case i_subcc :
			subType = 1 ; arithmetic = 1 ; break ;

		case i_subc : 
	    case i_subccc :
			subcType = 1 ; arithmetic = 1 ; break ;

	    case i_not:
	    case i_mov:
			logic = 1; break;

	    case i_sllx:
	    case i_sll:
	    case i_srlx:
	    case i_srl:
	    case i_srax:
	    case i_sra:
			shift = 1; break;
	}

	infected_instr = arithmetic | logic | shift;
	inject_me = inject_me & infected_instr;

	this->inst_arith = arithmetic; 
	this->inst_logic = logic;

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
	if( inject_me ) {

		// Inject a fault in the output register
		if( GSPOT_DEBUG ) { 
			DEBUG_OUT( "Injecting into " ) ;
			DEBUG_OUT( " instr %d\n",getSequenceNumber());
		}

		for(int i=0; i< SI_MAX_DEST; i++) { // LXL: keep upper limit to MAX_DEST, only inject 1 reg since i==0 
			reg_id_t &dest = getDestReg(i) ;            	
			if( !dest.isZero() ) { // Inject only if the destination is a valid register 

				if (i==0) {
					result = dest.getARF()->readInt64( dest ) ;	
					if( !m_fs->getDoFaultInjection() ){
						inj_result = result;
					}else if( m_fs->getFaultInjInst() >  m_fs->getRetInst() ){
						inj_result = result;
					} else { // Fault to be injected...
						// This just sets the current instruction (which is then to be used when 
						// collecting log info for the faulty instruction) stat_first_inject_i 
						m_fs->setCurrInst( getSequenceNumber() ) ;

						// Conduct custom saf simulation - this applies to i-add only!
						// I. Open pipes:
#if LXL_PROB_FM
						pattern_t fp = fSim.genPattern() ;
						if(PRADEEP_DEBUG_LL) {
							DEBUG_OUT("Generated pattern\n") ;
						}
						// DEBUG_OUT("Value = %llx ",result) ;
						// DEBUG_OUT("(%llx) (%llx)", fp.mask,fp.dir) ;
#if LXL_PROB_DIR_FM
						inj_result = (result&(~fp.mask))|fp.dir ;
#else
						inj_result = result^fp.mask ;
#endif
						// DEBUG_OUT(" -> %llx\n",inj_result) ;
#else //Vlog simulation
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

						// FIXME include all instrs applicable, filter redundancies

						// II. Depending on the instr at hand, prepare operands:
						op1 = getSourceReg(0).getARF()->readInt64( getSourceReg(0) );
						source = getSourceReg(1).getARF()->readInt64( getSourceReg(1) );
						op2 = (getStaticInst()->getFlag(SI_ISIMMEDIATE) ? getStaticInst()->getImmediate() : source);

						// Default values. Values over-written by instructions based on need
						invert = 0;
						cin = 0;							
						se = 0;
						opShiftVal = type(0);
						op3Val = type(0);
						rccVal = type(0);			
						and_sel = 0;
						or_sel = 0;
						xor_sel = 0;
						move_sel = 0;
						null_32 = 0;
						lsu = 0;
						scan_in = 0;	
						// Output mux select control
						// LXL: select signals are active low
						sel_sum = 1; 
						sel_shift = 1; 
						sel_rs3 = 1;	 
						sel_logic = 1;

						ecl_shft_op32_e=0;
						ecl_shft_shift4_e=0;
						ecl_shft_shift1_e=0;
						ecl_shft_enshift_e_l=1;
						ecl_shft_extendbit_e=0;
						ecl_shft_extend32bit_e_l=0;
						ecl_shft_lshift_e_l=0;


						if(addType){
							// invert op2
							// sparc ALU inverts the second operand automatically
							// xor with 64'hf basically
							invert = 0;
							and_sel = 1;
							sel_sum = 0; 
						} else if(addcType) { 
							// no cc change, use ICC_C 
							// ICC_C
							cin = (getSourceReg(2).getARF()->readInt64( getSourceReg(2) ) & 0x1 );
							invert = 0;						
							and_sel = 1;
							sel_sum = 0; 
						} else if(subType){ 
							// Subtraction conducted by 2's complement addition
							// since inversion of op2 is default, invert not to be set
							//LXL: to subtrace invert to 1, cin=0
							invert=1;
							cin = 1; // to enforce 2's complement add		
							and_sel = 1;
							sel_sum = 0; 
						} else if(subcType) {
							op2 += ( getSourceReg(2).getARF()->readInt64( getSourceReg(2) ) & 0x1 );
							// Subtraction conducted by 2's complement addition
							// since inversion of op2 is default, invert not to be set
							invert=1;
							cin = 1; // to enforce 2's complement add			
							and_sel = 1;
							sel_sum = 0; 
						} else if(andType) {
							// since inversion of op2 is default, invert to be set
							invert = 0;
							// Dummy 
							and_sel = 1;
							sel_logic = 0;	
						} else if(orType) {
							// since inversion of op2 is default, invert to be set
							invert = 0;
							or_sel = 1;
							sel_logic = 0;
						} else if(xorType) {
							// since inversion of op2 is default, invert to be set
							invert = 0;
							xor_sel = 1;
							sel_logic = 0;
						} else if(xnorType) {
							// since inversion of op2 is default, invert not to be set
							invert = 1;
							xor_sel = 1;
							sel_logic = 0;
						} if(si->getOpcode() == i_not) {
							// i_not is resolved as an xnor, having no immediate op2 and op2 = 0
							// since inversion of op2 is default, invert not to be set
							invert = 1;
							xor_sel = 1;
							sel_logic = 0;
						} else if(nandType) {
							// since inversion of op2 is default
							invert = 1;
							and_sel = 1;
							sel_logic = 0;		
						} else if(norType) {
							// since inversion of op2 is default
							invert = 1;
							or_sel = 1;
							sel_logic = 0;
						} else if(si->getOpcode() == i_mov) {
							invert = 1;
							move_sel = 1;
							sel_logic = 0;
						} else if(shift) {
							half_t opcode=si->getOpcode();
							bool arith_shift=0;
							bool extend64bit=0;

							ecl_shft_shift4_e= 1 << ((op2 & 0xc)>>2);
							ecl_shft_shift1_e= 1 << (op2&0x3);
							ecl_shft_enshift_e_l=0;

							if (!(opcode==i_sllx || opcode==i_sll)) {
								ecl_shft_lshift_e_l=1;
							}

							if (opcode==i_srax || opcode==i_sra) {
								arith_shift = 1;
							}

							if (opcode==i_sll || opcode==i_srl || opcode==i_sra) {
								ecl_shft_op32_e=1;
							} 
							
							ecl_shft_extend32bit_e_l = ~(ecl_shft_op32_e & ((op1>>31)&0x1) & 
														 arith_shift) & 0x1;

							extend64bit = arith_shift & ((op1>>63)&0x1) & ~ecl_shft_op32_e;
							ecl_shft_extendbit_e = (extend64bit | ~ecl_shft_extend32bit_e_l) & 0x1;
							sel_shift = 0;

						}

						// FIXME
						// regarding all operations altering conditions codes 
						// (all logical ops with index _cc; addcc and subcc eventually)
						// Following outputs of the ALU should be written to the corresponding bits of the 8 bit cc reg:
						// [bit 7: 64 bit result's MSB, bit 6: 64 bit result == 0, bit 3: 32 bit result's MSB, bit 2: 32 bit result == 0]
						//      assign  alu_ecl_zlow_e = ~(|zcomp_in[31:0]);
						//   	assign alu_ecl_zhigh_e = ~(|zcomp_in[63:32]);
						//   	assign   alu_ecl_add_n64_e = adder_out[63];
						//   	assign   alu_ecl_add_n32_e = adder_out[31];
						//   	assign   alu_ecl_log_n64_e = logic_out[63];
						//   	assign   alu_ecl_log_n32_e = logic_out[31];

						// FIXME the index of the instr should be included here
						// moreover, the table can be indexed by this index
						// multiple tables can be customized that refer to the same instr
						// The stimuli was not found in the LUT
						// wrap-up operands
						fSim.lluToBinStr(op1Vec, op1,REG_WIDTH);
						fSim.lluToBinStr(op2Vec, op2,REG_WIDTH);
						// DEBUG_OUT("Op1 in str = %s\n", op1Vec) ;
						// DEBUG_OUT("Op2 in str = %s\n", op2Vec) ;
						// DEBUG_OUT("add type = %d\n", addType) ;
						// DEBUG_OUT("or type = %d\n", orType) ;
						fSim.lluToBinStr(op3Vec, op3Val,REG_WIDTH);
						fSim.lluToBinStr(opShiftVec, opShiftVal,REG_WIDTH);
						fSim.lluToBinStr(rccVec, rccVal,REG_WIDTH);

						char shift4Vec[5], shift1Vec[5];
						fSim.lluToBinStr(shift4Vec, ecl_shft_shift4_e, 5);
						fSim.lluToBinStr(shift1Vec, ecl_shft_shift1_e, 5);
						// send stimuli over the write pipe				
						//fSim.nwrite = write(fSim.fdWrite, &se, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &op1Vec, REG_WIDTH);
						fSim.nwrite = write(fSim.fdWrite, &op2Vec, REG_WIDTH);
						fSim.nwrite = write(fSim.fdWrite, &cin, sizeof(int));
						//fSim.nwrite = write(fSim.fdWrite, &opShiftVec, REG_WIDTH);
						//fSim.nwrite = write(fSim.fdWrite, &op3Vec, REG_WIDTH);
						fSim.nwrite = write(fSim.fdWrite, &invert, sizeof(int));	
						fSim.nwrite = write(fSim.fdWrite, &and_sel, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &or_sel, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &xor_sel, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &move_sel, sizeof(int));	
						fSim.nwrite = write(fSim.fdWrite, &sel_sum, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &sel_shift, sizeof(int));
						//fSim.nwrite = write(fSim.fdWrite, &sel_rs3, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &sel_logic, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &null_32, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &ecl_shft_op32_e, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &shift4Vec, 5);
						fSim.nwrite = write(fSim.fdWrite, &shift1Vec, 5);
						fSim.nwrite = write(fSim.fdWrite, &ecl_shft_enshift_e_l, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &ecl_shft_extendbit_e, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &ecl_shft_extend32bit_e_l, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &ecl_shft_lshift_e_l, sizeof(int));

						//fSim.nwrite = write(fSim.fdWrite, &rccVec, REG_WIDTH);
						//fSim.nwrite = write(fSim.fdWrite, &lsu, sizeof(int));
						//fSim.nwrite = write(fSim.fdWrite, &scan_in, sizeof(int));
						// ncsim handles stimuli sent and starts simulation
						// it is already invoked as an independent process		
						// wait for results over the read pipe
						fSim.nread = read(fSim.fdRead, &injResultVec, REG_WIDTH);
						inj_result = strtoull(injResultVec, &ptr, 2);
#endif //LXL_PROB_FM
						// DEBUG_OUT("//send %s 0x%llx 0x%llx over pipe\n",decode_opcode(s->getOpcode()),op1,op2);
						// 						DEBUG_OUT("//****************************Got result 0x%llx !!!!!\n",inj_result);
										

#define GEN_TB 0
						if (GEN_TB) {
							DEBUG_OUT("byp_alu_rs1_data_e_latch = 64'h%llx;\n",op1);
							DEBUG_OUT("byp_alu_rs2_data_e_l_latch = 64'h%llx;\n ",op2);
							DEBUG_OUT("ecl_alu_cin_e_latch = 1'b%d;\n",cin);
							DEBUG_OUT("shft_alu_shift_out_e_latch = 64'h%llx;\n",opShiftVal);
							DEBUG_OUT("byp_alu_rs3_data_e_latch = 64'h%llx;\n",op3Val);
							DEBUG_OUT("ifu_exu_invert_d = 1'b%d;\n",invert);	
							DEBUG_OUT("ecl_alu_log_sel_and_e_latch = 1'b%d;\n",and_sel);
							DEBUG_OUT("ecl_alu_log_sel_or_e_latch = 1'b%d;\n",or_sel);
							DEBUG_OUT("ecl_alu_log_sel_xor_e_latch  = 1'b%d;\n    ",xor_sel);
							DEBUG_OUT("ecl_alu_log_sel_move_e_latch  = 1'b%d;\n   ",move_sel);	
							DEBUG_OUT("ecl_alu_out_sel_sum_e_l_latch  = 1'b%d;\n  ",sel_sum);
							DEBUG_OUT("ecl_alu_out_sel_shift_e_l_latch  = 1'b%d;\n",sel_shift);
							DEBUG_OUT("ecl_alu_out_sel_rs3_e_l_latch  = 1'b%d;\n  ",sel_rs3);
							DEBUG_OUT("ecl_alu_out_sel_logic_e_l_latch  = 1'b%d;\n",sel_logic);
							DEBUG_OUT("byp_alu_rcc_data_e_latch   = 64'h%llx;\n",rccVec);
							DEBUG_OUT("ifu_lsu_casa_e_latch = 1'b%d;\n",lsu);
							DEBUG_OUT("#clkPeriod  rclk = ~rclk;\n");
							DEBUG_OUT("#clkPeriod  rclk = ~rclk;\n");
							DEBUG_OUT("$display(\"rs1 %%x rs2 %%x\",byp_alu_rs1_data_e,byp_alu_rs2_data_e_l);\n");
							DEBUG_OUT("$display(\"result %%x\",alu_byp_rd_data_e);\n\n");
						}
						// keep number of invocations
						fSim.counter++;						
						// TO BE USED FOR DEBUGGING
#ifdef COMPARE
						ireg_t golden = result;
						ireg_t hls_result;
						int bit = 0;
						if ( bit > (sizeof(uint64)<<3) ) {
							hls_result = golden;	
						} else {
							hls_result = golden & (~((uint64)(1)<<bit));
						}
						if( result != inj_result ) {
							//DEBUG_OUT("priv %d seq_num %lld\t",m_priv,seq_num);
							DEBUG_OUT("//send %s 0x%llx 0x%llx over pipe\n",decode_opcode(s->getOpcode()),op1,op2);
 							DEBUG_OUT("Golden value = %llx\n", result) ;
 							DEBUG_OUT("low level = %llx\n", inj_result) ;
// 							DEBUG_OUT("High level = %llu\n", hls_result) ;

							// HALT_SIMULATION;
						} else {
							//DEBUG_OUT("-------------------	PASSED!\n\n", golden);
						}
#endif // end_compare
						this->infectedResult = inj_result;
						this->goldenResult = result; 

						// If there is a fault, mark this instruction and the physical reg as faulty
						if( result != inj_result ) {
							if( PRADEEP_DEBUG_LL ) {
								DEBUG_OUT("dest t%d flat%d phys%d\n",dest.getRtype(),dest.getFlatRegister(),dest.getPhysical());

								DEBUG_OUT("%llx\t", result) ;
								DEBUG_OUT("%llx\n", inj_result) ;
							}
							// FIXME: Correlation btw bits to be accounted for here
							fSim.numBitFlips(result,inj_result);
							//FAULT_STAT_OUT("I:%llx\n", getVPC()) ;
							m_fs->incTotalInj(); // an injection-counter gets updated only
							m_fs->injectSafFault();
#ifndef ACT_STATS	
							this->setFault() ;
							dest.getARF()->writeInt64( dest, inj_result ) ;
							dest.getARF()->setFault(dest);

							this->infectedALU = true;
#endif // ACT_STATS
						} else {
							dest.getARF()->unSetCorrupted(dest) ;
							dest.getARF()->clearFault(dest);
							dest.getARF()->clearReadFault(dest);

// 							if( !( (!m_fs->getDoFaultInjection()) ||
// 								   (m_fs->getFaultInjInst()> m_fs->getRetInst()))){
							m_fs->incTotalMask(); // a masked-counter gets incremented only

								//------------- TO DIFFERENTIATE BTW RETIRING AND SQUASHED INSTR
							this->uMaskedALU = true;
							this->infectedALU = true;
								//}
						}
					}
				}
				dest.getARF()->setRFWakeup(true);
				dest.getARF()->wakeDependents(dest);
			} // end: destination was a valid reg
		}
	} else {// end if(inject_me)
        for( int i=0; i< SI_MAX_DEST ; i++ ) { //Changed from MAX_DST to 1
            reg_id_t &dest = getDestReg(i) ;
            if( !dest.isZero() ) { // Inject only if the destination is a valid register
				dest.getARF()->unSetCorrupted(dest) ;
				dest.getARF()->clearFault(dest);
				dest.getARF()->clearReadFault(dest);
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

#else // LL_INT_ALU if no low-level ALU, then do the default injection
//***************************************************************************
void
dynamic_inst_t::Execute( void ) {
    // record when execution takes place
    m_event_times[EVENT_TIME_EXECUTE_DONE] = m_pseq->getLocalCycle() - m_fetch_cycle;

    // call the appropriate function
    static_inst_t *si = getStaticInst();
    // execute the instruction using the jump table
    pmf_dynamicExecute pmf = m_jump_table[si->getOpcode()];

    // LXL: Check to see if dest are ready
    checkDestReadiness();

    // If the exec unit type matches the fault type and the exec unit num is
	// the faulty unit, inject the fault
    // exec_n is the ALU number to corrupt 
    // exec_u is numbered from 0. Look at config/config.defaults for numbers
    bool inject_me = false;
    int32 exec_u = getExecUnit() ;
    byte_t exec_n = getExecUnitNum() ;
	bool write_to_int_rf=false;

	if (!m_dest_reg[0].isZero()) {
		write_to_int_rf = (m_dest_reg[0].getARF()->getARFType()==INT_ARF);
		if (write_to_int_rf) {
			m_result_bus_id = m_pseq->fetchIncWayCounter();
#ifdef RESOURCE_RR_REGBUS
			// Make the reg dbus allocation round-robin
			int next_bus = m_pseq->getNextBusId() ;
			// DEBUG_OUT("Opal: %d RR: %d\n", m_result_bus_id, next_bus) ;
			m_result_bus_id = next_bus ;
#endif // RESOURCE_RR_REGBUS
		} else {
			m_result_bus_id = m_pseq->fetchIncFWayCounter(); //FIXME: floating point data line
		}
	}


    if ((m_fs->getFaultType() == REG_DATA_LINE_FAULT) && 
		(m_result_bus_id==m_fs->getFaultyReg()) &&
		(!m_dest_reg[0].isZero()) ){
        inject_me = write_to_int_rf;
    }

    inject_me = inject_me || 
                ((m_fs->getFaultType() == INTALU_OUTPUT_FAULT) && (exec_u == 1) && 
                 (exec_n == m_fs->getFaultyReg())) ||
                ((m_fs->getFaultType() == FPALU_OUTPUT_FAULT) && (exec_u == 4) && 
                 (exec_n == m_fs->getFaultyReg()));

#ifdef LL_INT_ALU_COMPARE
	bool logic = 0 ;
	bool arithmetic = 0 ;
	bool shift = 0 ;
	bool infected_instr = 0 ;
	switch( si->getOpcode() ) {

		case i_and : 
	    case i_andcc :
		case i_or : 
	    case i_orcc :
		case i_andn : 
	    case i_andncc :
		case i_orn : 
	    case i_orncc :
		case i_xor : 
	    case i_xorcc :
		case i_xnor : 
	    case i_xnorcc :
	    case i_not:
	    case i_mov:
			logic = 1; break ;

		case i_add : 
	    case i_addcc :
		case i_addc : 
	    case i_addccc :
		case i_sub : 
	    case i_subcc :
		case i_subc : 
	    case i_subccc :
			arithmetic = 1 ; break ;

	    case i_sllx:
	    case i_sll:
	    case i_srlx:
	    case i_srl:
	    case i_srax:
	    case i_sra:
			shift = 1; break;
	}

	infected_instr = arithmetic | logic | shift;
	inject_me = inject_me & infected_instr;
#endif // LL_INT_ALU_COMPARE

    if ( inject_me) {
        reg_id_t &dest=getDestReg(0);
        if (!dest.isZero()) {
            dest.getARF()->setRFWakeup(false);
        }
    }

    (this->*pmf)();

	if(getFaultStat()->getFaultType()==DECODER_INPUT_FAULT) {
		static_inst_t *original = getStaticInst()->getOriginalInstr() ;
		if(original) {
			if(original->getType() == DYN_STORE || original->getType()==DYN_ATOMIC) {
				DEBUG_OUT("st morph: [%d]\t[0x%llx]\t%s->%s\n",
						getSequenceNumber(),
						getVPC(),
						decode_opcode( (enum i_opcode)original->getOpcode()),
						decode_opcode( (enum i_opcode)getStaticInst()->getOpcode())
						) ;
			}
		}
	}
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
	// If dest is not ready, unset inject me, and undo damage with the 
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
        if( GSPOT_DEBUG ) { 
            DEBUG_OUT( "Injecting into " ) ;
#ifdef OMIT_LOW_LEVEL
            printRetireTrace("") ;
#endif
		    DEBUG_OUT( " instr %d\n",getSequenceNumber());
	}

        for( int i=0; i< SI_MAX_DEST ; i++ ) { 
            reg_id_t &dest = getDestReg(i) ;
			if( !dest.isZero() ) {
				// Inject only if the dest is valid & ready
				// check here if the destination register is ready, and
				// then inject the fault
				if (i==0) {
					ireg_t result = dest.getARF()->readInt64( dest ) ;
					m_fs->setCurrInst( getSequenceNumber() ) ;
					ireg_t inj_result = m_fs->injectFault(result);
					// If there is a fault, mark this instruction
					// and the physical reg as faulty
					if( result != inj_result ) {
						this->setFault() ;
						dest.getARF()->writeInt64( dest, inj_result ) ;
						dest.getARF()->setFault(dest);
						if(INJECTED_INST) {
							DEBUG_OUT("0x%llx\t%d\n",
									getVPC(), getPrivilegeMode()) ;
						}
						setActivateFault() ;
						// DEBUG_OUT("%llx\n", getVPC()) ;
					} else {
						dest.getARF()->unSetCorrupted(dest) ;
						dest.getARF()->clearFault(dest);
						dest.getARF()->clearReadFault(dest);
					}
				}
				// 1st: check whether above conditions are right.
				// 2nd: if right, find ways to change dest reg (inject fault)
				// FIXME - Moving this up because we should do wakeup
				// only when there is a new value because of fault injection
			    dest.getARF()->setRFWakeup(true);
			    dest.getARF()->wakeDependents(dest);
		    }
	    }
    }

#ifdef FP_CHECKER
	if(exec_u == 4) { // Then this is an FP instruction
		reg_id_t &s1 = getSourceReg(0) ;
		reg_id_t &s2 = getSourceReg(1) ;
		reg_id_t &d = getDestReg(0) ;
		int type ;
		double f1, f2, dest ;
		half_t op = s->getOpcode() ;
		// Right now the checker deals only with add instructions
		if( (op==i_fadds || op==i_faddd) && //  || op==i_fsubs || op==i_fsubd) &&
			(!s1.isZero() && !s2.isZero() && !d.isZero()) &&
			((s1.getRtype()==s2.getRtype())) ) {
			if(op ==i_fadds) {
				type = 0 ;
				f1 = s1.getARF()->readFloat32(s1) ;
				f2 = s2.getARF()->readFloat32(s2) ;
				dest = d.getARF()->readFloat32(d) ;
			} else if(op == i_faddd) {
				type = 0 ;
				f1 = s1.getARF()->readFloat64(s1) ;
				f2 = s2.getARF()->readFloat64(s2) ;
				dest = d.getARF()->readFloat64(d) ;
			} else if(op ==i_fsubs) {
				type = 1 ;
				f1 = s1.getARF()->readFloat32(s1) ;
				f2 = s2.getARF()->readFloat32(s2) ;
				dest = d.getARF()->readFloat32(d) ;
			} else if(op == i_fsubd) {
				type = 1 ;
				f1 = s1.getARF()->readFloat64(s1) ;
				f2 = s2.getARF()->readFloat64(s2) ;
				dest = d.getARF()->readFloat64(d) ;
			}
#ifdef VAR_TRACE
			// TRACE_OUT("type\n%d\n", type) ;
			TRACE_OUT("s1\n%f\n", f1) ;
			TRACE_OUT("s2\n%f\n", f2) ;
			TRACE_OUT("dest\n%f\n\n", dest) ;
#else // VAR_TRACE
			// This will be the actual FP checker that we derive
			// use fabs to avoid the problem of -0.0
			// (s1!=0<=>s2!=dest) && (s2!=0<=>s1!=dest)
			// (s1=0<=>s2=dest) && (s2=0<=>s1=dest)
			// Both the above are the same because of the iff relation
			// FIXME - Just trying to compare the results with
			// if( ((f1==0)&&(f2!=dest)) || ((f2==0)&&(f1!=dest)) ) {
			// s1=0 & s2=0 => dest=0 to get some ideas
			// if( (f1==0)&&(f2==0)&&(dest!=0) ) {
			// s1!=0 & s1=s2 => dest!=0
			// if( (f1!=0)&&(f2==0)&&(dest==0) ) {
			// s2=0=>s1=dest
			 if( (f2==0)&&(f1!=dest) ) {
#ifdef DEBUG_FP_CHECKER
				DEBUG_OUT("Violation on:"); printRetireTrace("") ;
				DEBUG_OUT( "%s: ", decode_opcode((enum i_opcode)op)) ;
				DEBUG_OUT("%f + ", f1) ;
				DEBUG_OUT("%f = ", f2) ;
				DEBUG_OUT("%f\n", dest) ;
#endif // DEBUG_FP_CHECKER
				getSequencer()->outSymptomInfo(
						(uint64) m_pseq->getLocalCycle(),
						getSequencer()->getRetInst(),
						"FP_Checker",
						0x302, getVPC(),
						getPrivilegeMode(),0,0,0) ;
				HALT_SIMULATION ;
			}
#endif // VAR_TRACE
		}
	}
#endif // FP_CHECKER


#ifdef MEASURE_FP
    if (exec_u==4) {
        m_pseq->incFP(4,exec_n-1);
    } else if (exec_u==5) {
        m_pseq->incFP(5,exec_n-1);
    } else if (exec_u==6) {
        m_pseq->incFP(6,exec_n-1);
    }

#endif

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
#endif // end LL_INT_ALU

// TODO - The propagate fault for control registers is not exactly correct.
// The setFault() has to write in to the decoder map, and when the retireRegisters()
// happens, this fault must be copied in to the retire map of the ctrl regisers!
// Right now, this is not done.
    void
dynamic_inst_t::propagateFault()
{
	// if( getSequenceNumber()==967012) {
	// 	DEBUG_OUT("begin propagateFault() for %d. Has fault = %d\n",
	// 			getSequenceNumber(), isCorrupted()) ;
	// }

    if( m_fs->getFaultType() == INST_WINDOW_FAULT) {
        // This instruction activated the ROB fault
        if (m_faulty_rob && getFault()) { 
            reg_id_t &dest = getDestReg(0);//(debugio_t::getFaultSrcDst()-4)&0x1) ;
            if( !dest.isZero() ) {
                dest.getARF()->setFault(dest);
				// dest.getARF()->setReadFault(dest); // Pradeep - 2009-05-14.
                dest.setPhysical(old_dest_physical);
                dest.getARF()->setFault(dest);
				// dest.getARF()->setReadFault(dest); // Pradeep - 2009-05-14.
                dest.setPhysical(new_dest_physical);
            }
        }
        if (m_faulty_rob && isCorrupted()) { 
            reg_id_t &dest = getDestReg(0);//(debugio_t::getFaultSrcDst()-4)&0x1) ;
            if( !dest.isZero() ) {
                dest.getARF()->setCorrupted(dest);
                dest.setPhysical(old_dest_physical);
                dest.getARF()->setCorrupted(dest);
                dest.setPhysical(new_dest_physical);
            }
        }
    }

    if( m_fs->getFaultType() == RAT_FAULT) {

        for( int i=0; i<SI_MAX_DEST; i++ ) {
            reg_id_t &dest = getDestReg(i) ;
            if (dest.getARF()->getARFType()==INT_ARF&&!dest.isZero()) {
				// I'm the last one who wrote to this physical register
                dest.getARF()->setSyncTarget(dest);
                if (m_dest_ready[i]) {
                // all I care is, when I write this register, is it already ready?
                // if so, aliasing occur
                    dest.getARF()->setFault(dest);
                    setFault();
                    if (ALEX_RAT_DEBUG) {
						DEBUG_OUT("propagateFault dest ready %lld %d:p%d\n",
								getSequenceNumber(),dest.getFlatRegister(),dest.getPhysical());
                    }
					// Pradeep - 2009-05-14.
					// At this point, the other reg that is mapped into this reg
					// is also faulty! Should propagate to simics, so should find its mapping
					// and set it as faulty
					// dest.getARF()->setReadFault(dest) ;
                }
                
            }
        }
    }

    for (int i = 0; i < SI_MAX_SOURCE; i ++) {
        reg_id_t &source= getSourceReg(i);
        if (!source.isZero()) {
            if( source.getARF()->getFault(source)) { 
                // this source has fault
                this->setFault() ;
                if (ALEX_RAT_DEBUG) {
                    DEBUG_OUT("123 %lld src faulty\n",getSequenceNumber());
                }
            }

            if( source.getARF()->isCorrupted(source)) { 
                // this source has fault
                this->setCorrupted() ;
                if (ALEX_RAT_DEBUG) {
                    DEBUG_OUT("456 %lld src faulty\n",getSequenceNumber());
                }
            }
        }
    }

    for( int i=0; i<SI_MAX_DEST; i++ ) {
        reg_id_t &dest = getDestReg(i) ;
        if( !dest.isZero() ) {
            if( getFault()) {
                // Instruction was faulty becuase it read faulty source
                dest.getARF()->setFault(dest) ;
                if (ALEX_RAT_DEBUG) {
                    DEBUG_OUT("%d ",getSequenceNumber());
                    DEBUG_OUT("p%d ",dest.getPhysical());
                    DEBUG_OUT("fault:%d\n",dest.getARF()->getFault(dest));
                }
            } else {
                if (dest.getARF()->getFault(dest)) {
					if (ALEX_RAT_DEBUG) {
						DEBUG_OUT("instr setfault t%d r%d\n",dest.getRtype(), dest.getPhysical());
						printDetail();
						assert(0);
					}
					this->setFault();
				} else {
                    dest.getARF()->clearFault(dest) ;    
                    dest.getARF()->clearReadFault(dest) ;
				}
            }
            if( isCorrupted()) {
                // Instruction was faulty becuase it read faulty source
				if(getSequenceNumber()==967012) {
					DEBUG_OUT("Setting dest faulty\n") ;
				}
                dest.getARF()->setCorrupted(dest) ;
            } else {
                if (dest.getARF()->isCorrupted(dest)) {
					this->setCorrupted();
				} else {
                    dest.getARF()->unSetCorrupted(dest) ;    
				}
            }
        }
    }    

    if ((m_fs->getFaultType() == DECODER_INPUT_FAULT)  &&
            (s->getChangedDest())) {
        // original reg was not written, need to update to
        // set them as faulty

        for( int i=0; i<SI_MAX_DEST; i++ ) {
            reg_id_t &odest = m_orig_dest_reg[i] ;
            if( !odest.isZero() ) {
                if( getFault() ) { // Instruction was faulty becuase it read faulty source
                    odest.getARF()->setFault(odest) ;
                }
                if( isCorrupted() ) { // Instruction was faulty becuase it read faulty source
					// if(getSequenceNumber() == 666769) {
					// 	DEBUG_OUT("Other : Exec: %d, corrupted r%d\n",
					// 			getSequenceNumber(), odest.getFlatRegister()) ;
					// }
                    odest.getARF()->setCorrupted(odest) ;
                }
            }
        }
    }

    if( m_fs->getFaultType() == RAT_FAULT ) {
        if( getFault() && !m_orig_dest_reg[1].isZero() ) {
            m_orig_dest_reg[1].getARF()->setFault( m_orig_dest_reg[1] ) ;
        }
        if( isCorrupted() && !m_orig_dest_reg[1].isZero() ) {
            m_orig_dest_reg[1].getARF()->setCorrupted( m_orig_dest_reg[1] ) ;
        }
    }
	// if( getSequenceNumber()==666757 || getSequenceNumber()==666769) {
	// 	DEBUG_OUT("done propagateFault() for %d. Has fault = %d\n",
	// 			getSequenceNumber(), isCorrupted()) ;
	// }
}

//**************************************************************************
    void
dynamic_inst_t::Retire( abstract_pc_t *a )
{
	// record when execution takes place
	m_event_times[EVENT_TIME_RETIRE] = m_pseq->getLocalCycle() - m_fetch_cycle;

	ASSERT( !getEvent( EVENT_FINALIZED ) );
	ASSERT( !s->getFlag( SI_FETCH_BARRIER ) );

#ifdef LL_INT_ALU
	if (infectedALU) {
		if (goldenResult!=infectedResult) {
			fSim.numBitFlipsRetire(goldenResult,infectedResult);
		}
		fSim.useBadALU++;
	}
#endif

#ifdef DEBUG_RETIRE
		m_pseq->out_info("## Integer Retire Stage\n");
		printDetail();
#endif


#ifdef SW_ROLLBACK
	if (m_pseq->m_op_mode==RECOVERING && s->getOpcode()==i_nop) {
		m_pseq->m_op_mode=RECOVERED;
	}
#endif
	SetStage(RETIRE_STAGE);
	retireRegisters();

	// advance the program counter by calling member function "nextPC"
	pmf_nextPC pmf = s->nextPC;
	(this->*pmf)( a );
	markEvent( EVENT_FINALIZED );

#ifdef DETECT_INV_VIOLATION
#ifdef FAULT_LOG
	//Assuming no false positive invariants

	if (s->getExitCode() > 0) {
		FAULT_OUT("Invariant:%d got violated\n", s->getExitCode() - 100 ); 
		m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
				getSequenceNumber(),
				"Assert Fail", 
				0x200,getVPC(),
				getPrivilegeMode(), 0,
				(uint16)0,  m_pseq->getID(), s->getExitCode() - 100 );    
		m_pseq->setNonRetTrap();
#ifdef LXL_SNET_RECOVERY
		//symptomDetected(0x200);
		ERROR_OUT("Recovery not implemented for invariant violation");
#else
		HALT_SIMULATION ;          
#endif //LXL_SNET_RECOVERY
	}
#endif //FAULT_LOG
#endif // DETECT_INV_VIOLATION
	// Record the type of instruction as faulty
	if(RECORD_FAULTY_INST) {
		if(isCorrupted()) {
			m_fs->addFaultyInstruction(decode_opcode((enum i_opcode)s->getOpcode())) ;
		}
	}
	if(INST_MIX) {
		m_fs->recordInstMix("other",isCorrupted()) ;
	}
	
#ifdef HARD_BOUND
	// Do the memory tracking for the objects
	propagateObjects() ;
#endif // HARD_BOUND
}

#ifdef HARD_BOUND
void dynamic_inst_t::propagateObjects(int in_trap)
{
	reg_id_t &src1 = getSourceReg(0) ;
	reg_id_t &src2 = getSourceReg(1) ;
	reg_id_t &dest = getDestReg(0) ;
	half_t src1_flat, src2_flat, dest_flat ;
	if(src1.getRtype()!=RID_INT && src1.getRtype()!=RID_INT_GLOBAL) {
		src1_flat = PSEQ_FLATREG_NWIN ;
	} else {
		src1_flat = src1.getFlatRegister() ;
	}
	if(src2.getRtype()!=RID_INT && src2.getRtype()!=RID_INT_GLOBAL) {
		src2_flat = PSEQ_FLATREG_NWIN ;
	} else {
		src2_flat = src2.getFlatRegister() ;
	}
	if(dest.getRtype()!=RID_INT && dest.getRtype()!=RID_INT_GLOBAL) {
		dest_flat = PSEQ_FLATREG_NWIN ;
	} else {
		dest_flat = dest.getFlatRegister() ;
	}
	// if(DEBUG_HARD_BOUND) {
	// 	printRetireTrace("Obj propagate ") ;
	// }
	if(canDoPtrArithmetic()) {
		if(debugio_t::handleObjectPropagation(dest_flat, src1_flat, src2_flat, getPrivilegeMode())!=0) {
			// Something wrong
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
	}
}

// Subset of instructions that are allowed to do ptr arithmetic
bool dynamic_inst_t::canDoPtrArithmetic() {
	return true ; // TODO - make this a proper set
}
#endif // HARD_BOUND

void dynamic_inst_t::collectCompareLog()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		for (int i = 0; i < SI_MAX_DEST; i++) {
			reg_id_t &dest    = getDestReg(i);

			if (!dest.isZero()) {
				m_pseq->getMulticoreDiagnosis()->collectCompareLog(getVPC(), dest.getARF()->readInt64(dest), m_pseq->getID(), true);
				//DEBUG_OUT( "%d: collecting pc=%llx, dest=%llx\n", m_pseq->getID(), getVPC(), dest.getARF()->readInt64(dest));

			}
		}
	}
}
//**************************************************************************
void 
dynamic_inst_t::retireRegisters() {
	bool siva_debug = false;
	if(siva_debug)
    		DEBUG_OUT( "Retire: %s\n", decode_opcode( (enum i_opcode) s->getOpcode() ) );
	for (int i = 0; i < SI_MAX_DEST; i++) {
		reg_id_t &dest    = getDestReg(i);

        // if the register isn't the %g0 register --
        if (!dest.isZero()) {
            // For the control registers, you must first writeRetireMap then
            // free the registers. writeRetire does a copy of unused registers
            // at this point.

			bool dest_corrupted = false;
			bool src_corrupted = false;
			for (int i = 0; i < SI_MAX_SOURCE; i++) {
				reg_id_t &srcReg = getSourceReg(i);
				if (!srcReg.isZero()) {
					src_corrupted = src_corrupted | srcReg.getARF()->isCorrupted(srcReg);

					if(siva_debug) {	
						DEBUG_OUT( "src:%d",srcReg.getPhysical());
						DEBUG_OUT( "%s", (srcReg.getARF()->getFault(srcReg))?"*":" ") ;
						DEBUG_OUT( "%s", (srcReg.getARF()->getReadFault(srcReg))?"*":" ") ;
						DEBUG_OUT( "%s", (srcReg.getARF()->isCorrupted(srcReg))?"^":" ") ;
						DEBUG_OUT( "(r%d)", srcReg.getVanilla()) ;
						DEBUG_OUT( "(v%llx),",srcReg.getARF()->readInt64(srcReg));
					}

				}
			}

			// dest_corrupted <- op has fault or if the sources are corrupted
			dest_corrupted = src_corrupted | getFault() | isCorrupted() ;
			if(siva_debug) {	
				DEBUG_OUT( "dest: %d",dest.getPhysical());
				DEBUG_OUT( "%s", (dest.getARF()->getFault(dest))?"*":" ") ;
				DEBUG_OUT( "%s", (dest.getARF()->getReadFault(dest))?"*":" ") ;
				DEBUG_OUT( "%s", (dest_corrupted)?"^":" ") ;
				DEBUG_OUT( "(r%d)", dest.getVanilla()) ;
				DEBUG_OUT( "(v%llx)\n",dest.getARF()->readInt64(dest));
			}

			if(dest_corrupted) {
				if(siva_debug)
					DEBUG_OUT(" RetireRegesters : Dynamic.C: if dest_corrupted\n");
				dest.getARF()->setCorrupted(dest);
			} else {

				//if the fault is not injected in dest reg
				dest.getARF()->unSetCorrupted(dest); 	// TODO: is this correct, this might over write the value from store inst
				//if the fault is in the dest register

				//I  think that this case is taken care of by
				//has_fault. has_fault will be set when the
				//wrong value is read. and it will
				//automatically mark the reg corrupted. Here I
				//dont know if the value is really corrupted by
				//the injected fault on the dest reg or not.

				//if(m_fs->getFaultType() == RAT_FAULT && dest.getARF()->getLogical(dest) == m_fs->getFaultyReg()) {
				//	dest.setCorrupted();
				//}
			}


			// update the logical->physical mapping in the "retired" map
			dest.getARF()->writeRetireMap( dest );

			// free the old physical register: old_reg      
			reg_id_t &tofree = getToFreeReg(i);

			//tofree.getARF()->clearFault( tofree );
			tofree.getARF()->clearReadFault( tofree );
			tofree.getARF()->freeRegister( tofree );


			assert(dest.getPhysical()!=PSEQ_INVALID_REG);
		}
	}
}

#define LXL_SQUASH_DEBUG 0

//**************************************************************************
void 
dynamic_inst_t::FetchSquash() { 
    ASSERT(!Waiting());
    ASSERT(m_stage <= FETCH_STAGE);
#ifdef PIPELINE_VIS
    m_pseq->out_log("squash %d\n", getWindowIndex());
#endif
    m_pseq->decrementSequenceNumber();
	if (LXL_SQUASH_DEBUG && m_pseq->recovered) 
		FAULT_OUT("%d fsquashed\n",seq_num);
}

//**************************************************************************
void 
dynamic_inst_t::Squash() { 
    ASSERT( !getEvent( EVENT_FINALIZED ) );
    ASSERT(m_stage != RETIRE_STAGE);
    if (Waiting()) {
        RemoveWaitQueue();
    }
    UnwindRegisters( );
    m_pseq->decrementSequenceNumber();
#ifdef PIPELINE_VIS
    m_pseq->out_log("squash %d\n", getWindowIndex());
#endif
	if (LXL_SQUASH_DEBUG && m_pseq->recovered) 
		FAULT_OUT("%d squashed\n",seq_num);
    markEvent( EVENT_FINALIZED );
}

//**************************************************************************
void dynamic_inst_t::Wakeup( void )
{
    Schedule();
}

/// sets the stage of execution
//**************************************************************************
void dynamic_inst_t::SetStage(enum stage_t stage)
{
#ifdef PIPELINE_VIS
    if (stage != WAIT_2ND_STAGE && stage != WAIT_3RD_STAGE)
        m_pseq->out_log("stage %d %s\n", getWindowIndex(),
                printStageAbbr(stage) );
#endif
    this->m_stage = stage;

    // if (stage==RETIRE_STAGE)
    //     m_pseq->out_info("   PC    : 0x%0llx\n", getVPC());


    /* if we are finished this instruction: */
    if (m_stage == COMPLETE_STAGE) {
        /* ready to retire RETIRE_STAGES cycles later */
        m_complete_cycle = m_pseq->getLocalCycle() + RETIRE_STAGES;
    }
}

//***************************************************************************
const char *dynamic_inst_t::print( void ) const
{
    return ( decode_opcode( (enum i_opcode) s->getOpcode() ) );
}

//***************************************************************************
bool dynamic_inst_t::isRetireReady() const {
    /* make sure we have set m_complete_cycle */
    if (m_stage == COMPLETE_STAGE)
        ASSERT(m_complete_cycle != 0);

    return (m_stage == COMPLETE_STAGE) &&
        (m_pseq->getLocalCycle() >= m_complete_cycle);
}

//***************************************************************************
void dynamic_inst_t::printDetail( void )
{
    m_pseq->out_info(" dynamic instruction: [%s]\n", 
            decode_opcode( (enum i_opcode) s->getOpcode() ));
    m_pseq->out_info("   window: %d\n", m_windex);
    m_pseq->out_info("   stage : %s\n", printStage(m_stage) );
    m_pseq->out_info("   PC    : 0x%0llx\n", getVPC());
    m_pseq->out_info("   inst  : 0x%0x\n", s->getInst());
    m_pseq->out_info("   seq # : %lld\n", seq_num);
    m_pseq->out_info("   Priv  : %lld\n", getPrivilegeMode());
    m_pseq->out_info("   A_type: %d\n", exec_unit);
    m_pseq->out_info("   A_num : %d\n", exec_unit_num);
    m_pseq->out_info("   TL    : %d\n", m_pseq->getControlARF()->getRetireRF()->getInt( CONTROL_TL ) ) ;
    if (getTrapType() != Trap_NoTrap) {
        m_pseq->out_info("   TRAP: 0x%0x\n", getTrapType() );
    }
    if (m_events != 0) {
        m_pseq->out_info("   EVENTS: 0x%0x\n", m_events);
    }
    if (s->getFlag(SI_ISIMMEDIATE)) {
        m_pseq->out_info("   Immediate: 0x%0llx\n", s->getImmediate());
    } else {
        m_pseq->out_info("   No Immediate\n");
    }
    m_pseq->out_info("   Offset: 0x%0llx\n", s->getOffset());
    printRegs( true ); //LXL_TODO: change back
    m_pseq->out_info("   has_fault : %d\n", getFault() );
    m_pseq->out_info(" decode_fail : %d\n", getStaticInst()->getDecodeFail() );
}

//***************************************************************************
void dynamic_inst_t::printRegs( bool print_values )
{
    m_pseq->out_info(" dynamic source regs:   \n");
    for (int i = 0; i < SI_MAX_SOURCE; i ++) {
        reg_id_t &reg = getSourceReg(i);
        if (!reg.isZero()) {
            m_pseq->out_info("  %2.2d ", i);
            abstract_rf_t *arf = reg.getARF();
            if (arf != NULL) {
                arf->print( reg );
            } else {
                m_pseq->out_info("  NULL ARF ");
            }
            if (print_values) {
                if (reg.getPhysical() == PSEQ_INVALID_REG)
                    m_pseq->out_info("not decoded\n");
                else if (reg.getARF()->isReady( reg ))
                    m_pseq->out_info("value = 0x%0llx\n",
                            reg.getARF()->readInt64( reg ));
                else
                    m_pseq->out_info("not ready\n");
            }
        }
    }

    m_pseq->out_info(" dynamic dest regs:   \n");
    for (int i = 0; i < SI_MAX_DEST; i ++) {
        reg_id_t &reg = getDestReg(i);
        if (!reg.isZero()) {
            m_pseq->out_info("  %2.2d ", i);
            abstract_rf_t *arf = reg.getARF();
            if (arf != NULL) {
                arf->print( reg );
            } else {
                m_pseq->out_info("  NULL ARF ");
            }
            if (print_values) {
                if (reg.getPhysical() == PSEQ_INVALID_REG)
                    m_pseq->out_info("not decoded\n");
                else if (reg.getARF()->isReady( reg ))
                    m_pseq->out_info("value = 0x%0llx\n",
                            reg.getARF()->readInt64( reg ));
                else
                    m_pseq->out_info("not ready\n");
            }
        }
    }

    m_pseq->out_info(" to free dest regs:   \n");
    for (int i = 0; i < SI_MAX_DEST; i ++) {
        reg_id_t &reg = getToFreeReg(i);
        if (!reg.isZero()) {
            m_pseq->out_info("  %2.2d ", i);
            abstract_rf_t *arf = reg.getARF();
            if (arf != NULL) {
                arf->print( reg );
            } else {
                m_pseq->out_info("  NULL ARF ");
            }
            if (print_values) {
                if (reg.getPhysical() == PSEQ_INVALID_REG)
                    m_pseq->out_info("not decoded\n");
                else if (reg.getARF()->isReady( reg ))
                    m_pseq->out_info("value = 0x%0llx\n",
                            reg.getARF()->readInt64( reg ));
                else
                    m_pseq->out_info("not ready\n");
            }
        }
    }
    m_pseq->out_info("\n");
}

// This function prints out a trace of the retiring instruction, with
// mem address, immediate, if applicable. The fault status is based on
// the corrupt bit (and not the fault bit) to account for memory-level
// fault propagation
void dynamic_inst_t::printRetireTrace(char *str)
{
	// DEBUG_OUT( "[0x%llx]", getVPC() ) ;
	// DEBUG_OUT( "(%d) - ", getPrivilegeMode() ) ;
	// DEBUG_OUT( "%s", decode_opcode( (enum i_opcode) s->getOpcode() ) ) ;
	// DEBUG_OUT( "\n" ) ;
	// return ;
	// print target address for ctrl and mem inst
	// memory_inst_t* mem_inst = dynamic_cast<memory_inst_t*>(this) ;
	// if(mem_inst) {
	// 	// Print virtual address, value
	// 	my_addr_t addr = mem_inst->getAddress() ;
	// 	DEBUG_OUT("[0x%llx]\t0x%llx\n", getVPC(), addr) ;
	// }
	// return ;
	if(MEM_ONLY_TRACE) {
		memory_inst_t* mem_inst = dynamic_cast<memory_inst_t*>(this) ;
		if(!mem_inst) {
			return ;
		}
	}

	if(ASSEMBLY_TRACE) {
		DEBUG_OUT( "%s", str) ;
		DEBUG_OUT( "[0x%llx] ", getVPC() ) ;
		char disasm[100];
		int count = s->printDisassemble(disasm) ;
		DEBUG_OUT("%s\n", disasm) ;
	} else if(SLICE_TRACE) { // Print a trace that can be used for slicing
		DEBUG_OUT("%d", getSequenceNumber()) ;
		DEBUG_OUT( "\t0x%llx", getVPC() ) ;
		DEBUG_OUT( "\t%s", decode_opcode( (enum i_opcode) s->getOpcode() ) ) ;
		for( int i = 0; i < SI_MAX_SOURCE; i++ ) {
			reg_id_t &reg = getSourceReg(i) ;
			if( reg.isZero() ) {
				DEBUG_OUT( "\trI") ;
			} else {
				DEBUG_OUT( "\tr%d", reg.getFlatRegister()) ;
			}
		}
		for (int i = 0; i < SI_MAX_DEST; i ++) {
			reg_id_t &reg = getDestReg(i);
			if (reg.isZero()) {
				DEBUG_OUT( "\tr65535") ;
			} else {
				DEBUG_OUT( "\tr%d", reg.getFlatRegister()) ;
			}
		}
		// print target address for mem inst
		memory_inst_t* mem_inst = dynamic_cast<memory_inst_t*>(this) ;
		if(mem_inst) {
			// Print virtual address, value
			my_addr_t addr = mem_inst->getAddress() ;
			DEBUG_OUT("\t0x%llx\n", addr) ;
		} else {
			DEBUG_OUT("\t0x0\n") ;
		}
	} else if(SHORT_TRACE) {
		DEBUG_OUT( "%s", str) ;
		DEBUG_OUT( "[%d]", m_pseq->getLocalCycle() ) ;
		DEBUG_OUT( "[%d]", getSequenceNumber()) ;
		DEBUG_OUT( "[0x%llx]", getVPC() ) ;
		DEBUG_OUT( "(%d) - ", getPrivilegeMode() ) ;
		DEBUG_OUT( "%s", decode_opcode( (enum i_opcode) s->getOpcode() ) ) ;
		DEBUG_OUT( " <- " ) ;
		for( int i = 0; i < SI_MAX_SOURCE; i++ ) {
			reg_id_t &reg = getSourceReg(i) ;
			if( !reg.isZero() ) {
				// DEBUG_OUT( "%d",reg.getPhysical());
				DEBUG_OUT( "r%d", reg.getFlatRegister()) ;
				if(!NO_VALUES) {
					if (reg.getPhysical() == PSEQ_INVALID_REG) {
						DEBUG_OUT( "(vND)");
					} else {
						DEBUG_OUT( "(v%llx)",reg.getARF()->readInt64(reg));
					}
				}
				DEBUG_OUT(",") ;
			}
		}
		if(s->getFlag(SI_ISIMMEDIATE)) {
			DEBUG_OUT("imm") ;
			if(!NO_VALUES) {
				DEBUG_OUT( "(0x%llx),",s->getImmediate()) ;
			}
		}
		DEBUG_OUT( "-> " ) ;

		for (int i = 0; i < SI_MAX_DEST; i ++) {
			reg_id_t &reg = getDestReg(i);
			if (!reg.isZero()) {
				DEBUG_OUT( "r%d", reg.getFlatRegister()) ;
				if(!NO_VALUES) {
					if (reg.getPhysical() == PSEQ_INVALID_REG) {
						DEBUG_OUT( "(vND)");
					} else {
						DEBUG_OUT( "(v%llx)",reg.getARF()->readInt64(reg));
					}
				}
				DEBUG_OUT(",") ;
			}
		}
		memory_inst_t* mem_inst = dynamic_cast<memory_inst_t*>(this) ;
		if(mem_inst) {
			// Print virtual address, value
			my_addr_t addr = mem_inst->getAddress() ;
			DEBUG_OUT(" VA=0x%llx,", addr) ;
			DEBUG_OUT(" size=%d,", mem_inst->getAccessSize()) ;
			if(!NO_VALUES) {
				DEBUG_OUT(" Val=0x%llx,", mem_inst->getUnsignedData()) ;
				// XXX - this isn't giving correct data-value output.
				// Bit value changed in 32nd bit is not showing up in print
				// although the fault is there! Why??
			}
		} else {
			// Print target
			control_inst_t *ctrl_inst = dynamic_cast<control_inst_t*>(this) ;
			if(ctrl_inst) {
				DEBUG_OUT(" target: 0x%llx", getPSEQ()->getSimicsNPC()) ;
				// physical_file_t *control_rf = getPSEQ()->getControlARF()->getRetireRF();
				// DEBUG_OUT(" Target: 0x%llx", control_rf->getInt(CONTROL_NPC)) ;
				// DEBUG_OUT(" target: 0x%llx", ctrl_inst->getActualTarget()->npc) ;
			}
		}

		DEBUG_OUT( "\n" ) ;
	} else { // Print the regular full trace with values and all
		DEBUG_OUT( "%s", str) ;
		DEBUG_OUT( "[%d]", m_pseq->getID() ) ;
		DEBUG_OUT( "[%d]", m_pseq->getLocalCycle() ) ;
		DEBUG_OUT( "[%d]", getSequenceNumber() ) ;
		DEBUG_OUT( "[0x%llx]", getVPC() ) ;
		DEBUG_OUT( "(%d) - ", getPrivilegeMode() ) ;
		DEBUG_OUT( "%s", decode_opcode( (enum i_opcode) s->getOpcode() ) ) ;
		if(MEM_PROPAGATION) {
			DEBUG_OUT( "%s ", (isCorrupted())?"^":" ") ;
		} else {
			DEBUG_OUT( "%s ", (getFault())?"*":" ") ;
		}
		DEBUG_OUT( " <- " ) ;
		for( int i = 0; i < SI_MAX_SOURCE; i++ ) {
			reg_id_t &reg = getSourceReg(i) ;
			if( !reg.isZero() ) {
				DEBUG_OUT( "p%d ",reg.getPhysical());
				DEBUG_OUT( "r%d", reg.getFlatRegister()) ;
				if(MEM_PROPAGATION) {
					DEBUG_OUT( "%s", (reg.getARF()->isCorrupted(reg))?"^":" ") ;
				} else {
					DEBUG_OUT( "%s", (reg.getARF()->getFault(reg))?"*":" ") ;
				}
				// DEBUG_OUT( "%s", (reg.getARF()->getReadFault(reg))?"*":" ") ;
				// DEBUG_OUT( "(r%d)", reg.getVanilla()) ;
				// DEBUG_OUT( "(r%d)", reg.getFlatRegister()) ;
				if(!NO_VALUES) {
					if (reg.getPhysical() == PSEQ_INVALID_REG) {
						DEBUG_OUT( "(vND),");
					} else {
						DEBUG_OUT( "(v%llx),",reg.getARF()->readInt64(reg));
					}
				}
			}
		}

		if(s->getFlag(SI_ISIMMEDIATE)) {
			DEBUG_OUT("imm") ;
			if(!NO_VALUES) {
				DEBUG_OUT( "(0x%llx),",s->getImmediate()) ;
			}
		}
		DEBUG_OUT( "-> " ) ;

		for (int i = 0; i < SI_MAX_DEST; i ++) {
			reg_id_t &reg = getDestReg(i);
			if (!reg.isZero()) {
				DEBUG_OUT( "p%d ", reg.getPhysical()) ;
				DEBUG_OUT( "r%d", reg.getFlatRegister()) ;
				if(MEM_PROPAGATION) {
					DEBUG_OUT( "%s", (reg.getARF()->isCorrupted(reg))?"^":" ") ;
				} else {
					DEBUG_OUT( "%s", (reg.getARF()->getFault(reg))?"*":" ") ;
				}
				// DEBUG_OUT( "%s", (reg.getARF()->getReadFault(reg))?"*":" ") ;
				// DEBUG_OUT( "(r%d)", reg.getVanilla()) ;
				// DEBUG_OUT( "(r%d)", reg.getFlatRegister()) ;
				if(!NO_VALUES) {
					if (reg.getPhysical() == PSEQ_INVALID_REG) {
						DEBUG_OUT( "(vND),");
					} else {
						DEBUG_OUT( "(v%llx),",reg.getARF()->readInt64(reg));
						DEBUG_OUT( "(v%f),",reg.getARF()->readFloat32(reg));
					}
				}
			}
		}
		memory_inst_t* mem_inst = dynamic_cast<memory_inst_t*>(this) ;
		if(mem_inst) {
			my_addr_t addr = mem_inst->getAddress() ;

			DEBUG_OUT(" VA") ;
			DEBUG_OUT("%s",(debugio_t::isCorruptAddress(addr,getPrivilegeMode()))?"^":" ") ;
			DEBUG_OUT("=0x%llx,", addr) ;
			DEBUG_OUT(" size=%d, ", mem_inst->getAccessSize()) ;
			if(!NO_VALUES) {
				DEBUG_OUT(" Val=%llx,", mem_inst->getUnsignedData()) ;
				DEBUG_OUT(" PA:0x%llx, ", mem_inst->getPhysicalAddress()) ;
			}
		} else {
			// Print target
			control_inst_t *ctrl_inst = dynamic_cast<control_inst_t*>(this) ;
			if(ctrl_inst) {
				DEBUG_OUT(" target: 0x%llx", getPSEQ()->getSimicsNPC()) ;
				// DEBUG_OUT(" target: 0x%llx", ctrl_inst->getActualTarget()->npc) ;
                // physical_file_t *control_rf = getPSEQ()->getControlARF()->getRetireRF();
                // DEBUG_OUT(" Target: 0x%llx", control_rf->getInt(CONTROL_NPC)) ;
			}
		}

		DEBUG_OUT( "\n" ) ;
	}
}

void dynamic_inst_t::printDestReg()
{
	for (int i = 0; i < SI_MAX_DEST; i ++) {
		reg_id_t &reg = getDestReg(i);
		if (!reg.isZero()) {
			//DEBUG_OUT( "p%d ", reg.getPhysical()) ;
			DEBUG_OUT( "r%d", reg.getFlatRegister()) ;
			if(MEM_PROPAGATION) {
				DEBUG_OUT( "%s", (reg.getARF()->isCorrupted(reg))?"^":" ") ;
			} else {
				DEBUG_OUT( "%s", (reg.getARF()->getFault(reg))?"*":" ") ;
			}
			if(!NO_VALUES) {
				if (reg.getPhysical() == PSEQ_INVALID_REG) {
					DEBUG_OUT( "(vND),");
				} else {
					DEBUG_OUT( "(v%llx),",reg.getARF()->readInt64(reg));
					if(reg.getARF()->readFloat32(reg) != -1.0)
						DEBUG_OUT( "(v%f),",reg.getARF()->readFloat32(reg));
					if(reg.getARF()->readFloat64(reg) != -1.0)
						DEBUG_OUT( "(v%lf),",reg.getARF()->readFloat64(reg));
				}
			}
		}
	}
}

// Return a string that prints the given inst
char* dynamic_inst_t::printRetireString(char *str)
{
	char output[1000] = "" ;
	char temp[100] = "" ;
	strcat(output, str) ;

	sprintf(temp, "[0x%llx](%d) - %s <- ",
			getVPC(),
			getPrivilegeMode(),
			decode_opcode( (enum i_opcode) s->getOpcode() ) ) ;

	strcat(output, temp) ;
	strcmp(temp, "") ;

	for( int i = 0; i < SI_MAX_SOURCE; i++ ) {
		reg_id_t &reg = getSourceReg(i) ;
		if( !reg.isZero() ) {
			sprintf(temp, "r%d,", reg.getFlatRegister()) ;
			strcat(output, temp) ;
			strcpy(temp, "") ;
		}
	}
	strcat(output, "-> " ) ;

	for (int i = 0; i < SI_MAX_DEST; i ++) {
		reg_id_t &reg = getDestReg(i);
		if (!reg.isZero()) {
			sprintf(temp, "r%d,", reg.getFlatRegister()) ;
			strcat(output, temp) ;
			strcpy(temp, "") ;
		}
	}
	memory_inst_t* mem_inst = dynamic_cast<memory_inst_t*>(this) ;
	if(mem_inst) {
		// Print virtual address, value
		my_addr_t addr = mem_inst->getAddress() ;
		sprintf(temp, " VA=0x%llx,", addr) ;
		strcat(output, temp);
		strcpy(temp, "") ;
	} else {
		// Print target
		control_inst_t *ctrl_inst = dynamic_cast<control_inst_t*>(this) ;
		if(ctrl_inst) {
			sprintf(temp, " target: 0x%llx", getPSEQ()->getSimicsNPC()) ;
			strcat(output, temp) ;
			strcpy(temp, "") ;
		}
	}
	return output ;
}

//**************************************************************************
    void
dynamic_inst_t::UnwindRegisters( void )
{
    for (int i = 0; i < SI_MAX_DEST; i++) {
        reg_id_t &dest    = getDestReg(i);
        // if the register isn't the %g0 register --
        if (!dest.isZero()) {

            reg_id_t &tofree = getToFreeReg(i);
            // revert the logical->physical mapping in the "decode" map
            tofree.getARF()->writeDecodeMap( tofree );

            // free the physical register
            // LXL: do not clearFault here, centralize at exec stage
            //dest.getARF()->clearFault( dest ); 
            dest.getARF()->freeRegister( dest );
        }
    }
    clearReadFaults();
}

//***************************************************************************
const char *dynamic_inst_t::printStage( stage_t stage )
{
    switch (stage) {

        case FETCH_STAGE:
            return ("FETCH_STAGE");
        case DECODE_STAGE:
            return ("DECODE_STAGE");
        case WAIT_4TH_STAGE:
            return ("WAIT_4TH_STAGE");
        case WAIT_3RD_STAGE:
            return ("WAIT_3RD_STAGE");
        case WAIT_2ND_STAGE:
            return ("WAIT_2ND_STAGE");
        case WAIT_1ST_STAGE:
            return ("WAIT_1ST_STAGE");
        case READY_STAGE:
            return ("READY_STAGE");
        case EXECUTE_STAGE:
            return ("EXECUTE_STAGE");
        case LSQ_WAIT_STAGE:
            return ("LSQ_WAIT_STAGE");
        case ADDR_OVERLAP_STALL_STAGE:
            return ("ADDR_OVERLAP_STALL_STAGE");
        case EARLY_STORE_STAGE:
            return ("EARLY_STORE_STAGE");
        case EARLY_ATOMIC_STAGE:
            return ("EARLY_ATOMIC_STAGE");
        case CACHE_MISS_STAGE:
            return ("CACHE_MISS_STAGE");
        case CACHE_NOTREADY_STAGE:
            return ("CACHE_NOTREADY_STAGE");
        case COMPLETE_STAGE:
            return ("COMPLETE_STAGE");
        case RETIRE_STAGE:
            return ("RETIRE_STAGE");
        default:
            return ("Unknown");    
    }
    return ("Unknown");    
}

//***************************************************************************
const char *dynamic_inst_t::printStageAbbr( stage_t stage )
{
    switch (stage) {

        case FETCH_STAGE:
            return ("F");
        case DECODE_STAGE:
            return ("D");
        case WAIT_4TH_STAGE:
        case WAIT_3RD_STAGE:
        case WAIT_2ND_STAGE:
        case WAIT_1ST_STAGE:
        case LSQ_WAIT_STAGE:
        case EARLY_STORE_STAGE:
        case EARLY_ATOMIC_STAGE:
        case CACHE_MISS_STAGE:
        case CACHE_NOTREADY_STAGE:
        case ADDR_OVERLAP_STALL_STAGE:
            return ("S");
        case READY_STAGE:
            return ("Y");
        case EXECUTE_STAGE:
            return ("E");
        case COMPLETE_STAGE:
            return ("C");
        case RETIRE_STAGE:
            return ("R");
        default:
            return ("Unknown");    
    }
    return ("Unknown");    
}


/*------------------------------------------------------------------------*/
/* Next PC Methods                                                        */
/*------------------------------------------------------------------------*/

/**  nextPC: Given the current PC, nPC, these functions
 **  compute the next PC and next nPC. This modifies the state of
 **  the branch predictors, so they must be used accordingly.
 */

/// non-CTI instructions: PC = nPC, nPC = nPC + 4
//**************************************************************************
void   dynamic_inst_t::nextPC_execute( abstract_pc_t *a )
{
    // NOTE: NO prediction is allowed here. setTarget_uncond calls this

    // advance the program counter (PC = nPC)
    a->pc  = a->npc;
    a->npc = a->npc + sizeof(uint32);

    /** Note: 11/6/01 CM
     *        No speculation is permitted in this function. see controlop.h */
}

/// taken non annulled CTI: PC = nPC, nPC = EA
//**************************************************************************
void   dynamic_inst_t::nextPC_taken( abstract_pc_t *a )
{
    predictor_state_t *specBPS = m_pseq->getSpecBPS();

    // In the future, this could be speculative, given BTB design
    la_t targetPC = (int64) a->pc + (int64) s->getOffset();

    // LXL: An attempt to corrupt Branch target. This can be thought
    // of as output latch of the BTB
    if( m_fs->getFaultType() == BRANCH_TARGET_FAULT && a->fetch_id==m_fs->getFaultyReg() ) {
        la_t new_targetPC = m_fs->injectFault( targetPC );

        if( targetPC != new_targetPC ) {
            this->setFault() ;
            setControlFault();
        }

        targetPC = new_targetPC ;
    }

    a->pc  = a->npc;
    a->npc = targetPC;

    /* store the predicted target (a->pc after the jump) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);
}

/// taken annulled CTI: PC = EA, nPC = EA + 4
//**************************************************************************
void   dynamic_inst_t::nextPC_taken_a( abstract_pc_t *a )
{
    predictor_state_t *specBPS = m_pseq->getSpecBPS();


    // In the future, this could be speculative, given BTB design
    a->pc  = (int64) a->pc + (int64) s->getOffset();

    // LXL: An attempt to corrupt Branch target. This can be thought
    // of as output latch of the BTB
    if( m_fs->getFaultType() == BRANCH_TARGET_FAULT && a->fetch_id==m_fs->getFaultyReg() ) {
        la_t new_targetPC = m_fs->injectFault( a->pc);

        if( a->pc != new_targetPC ) {
            this->setFault() ;
            setControlFault();
        }

        a->pc = new_targetPC ;
    }

    a->npc = a->pc + sizeof(uint32);

    /* store the predicted target (a->pc after the jump) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);
}

/// untaken annulled CTI: PC = nPC + 4, nPC = nPC + 8
//**************************************************************************
void   dynamic_inst_t::nextPC_untaken_a( abstract_pc_t *a )
{
    predictor_state_t *specBPS = m_pseq->getSpecBPS();

    // In the future, this could be speculative, given BTB design
    a->pc  = a->npc + sizeof(uint32);
    a->npc = a->npc + (2*sizeof(uint32));

    /* store the predicted target (a->pc after the jump) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);
}

/// return from trap (done, retry) instructions
//**************************************************************************
void   dynamic_inst_t::nextPC_trap_return( abstract_pc_t *a )
{
    predictor_state_t *specBPS = m_pseq->getSpecBPS();
    tlstack_t         *tlstack = m_pseq->getTLstack();

    // read the future tl off of the stack
    ireg_t tpc  = 0;
    ireg_t tnpc = 0;
    // NOTE: normally TL is decremented at end of retry. Our trap state array
    //       has trap level 1 at array index 0, so we decrement it here.

    // decrement tl
    if ( a->tl <= 0 ) {
        a->tl = 0;
    } else {
        a->tl = a->tl - 1;
    }

    tlstack->pop( tpc, tnpc, a->pstate, a->cwp, a->tl );
    a->gset = pstate_t::getGlobalSet( a->pstate );

    // Trap return speculation can cause the fetch unit to retry instructions
    // that miss in the I-TLB, before the new translation is placed in the I-TLB.
    // Now that the instruction executed by the trap handler (stxa) 
    // that updated the I-TLB stalls fetch, this isn't a problem.
    if (s->getOpcode() == i_done) {
        a->pc  = tnpc;
        a->npc = tnpc + sizeof(uint32);
    } else if (s->getOpcode() == i_retry) {
        a->pc  = tpc;
        a->npc = tnpc;
    } else {
        ERROR_OUT("dx: nextPC_trap_return: called for unknown opcode\n");
#ifdef LL_DECODER
        a->pc  = tpc;
        a->npc = tnpc;
#else
        SIM_HALT;
#endif
    }

    /* store the predicted target (a->pc after the jump) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);
}

/** CALL control transfer: functionally similar to nextPC_indirect, but
 *       * updates the RAS predictor as well.
 *  Now use indirect predictor to predict call target.
 */
//**************************************************************************
void   dynamic_inst_t::nextPC_call( abstract_pc_t *a )
{
    cascaded_indirect_t   *bpred          = m_pseq->getIndirectBP();
    predictor_state_t     *specBPS        = m_pseq->getSpecBPS();
    ras_t                 *ras            = m_pseq->getRAS();

    // make a branch prediction
    STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][2] );
    STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][m_priv? 1:0] );

    my_addr_t predNPC = bpred->Predict( a->pc, &specBPS->indirect_state,
            &m_pseq->m_branch_except_stat[s->getBranchType()]);

    la_t targetPC = (int64) a->pc + (int64) s->getOffset();

    a->pc = a->npc;
    if(s->getOpcode() == i_call) {
        // call instruction: get the target address directly
        a->npc = targetPC;
    } else {
        // jmpl instruction: using indirect predictor
        a->npc = predNPC;
    }

    // LXL: An attempt to corrupt Branch target. This can be thought
    // of as output latch of the BTB
    if( m_fs->getFaultType() == BRANCH_TARGET_FAULT && a->fetch_id==m_fs->getFaultyReg() ) {
        la_t new_targetNPC = m_fs->injectFault( a->npc );

        if( a->npc != new_targetNPC ) {
            this->setFault() ;
            setControlFault();
        }

        a->npc = new_targetNPC ;
    }

    // push the return address on the RAS
    //          + 4 is the delay slot
    //          + 8 is the return address
    if(ras_t::PRECISE_CALL) {
        // exclude call to <pc+8>
        if( (getVPC() + 8) != a->npc ) {
            ras->push( getVPC() + 8, a->npc, &specBPS->ras_state );
        }
    } else {
        ras->push( getVPC() + 8, a->npc, &specBPS->ras_state );
    }


    /* store the predicted target (a->pc after the jump) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);
}

/** RET  control transfer: functionally similar to nextPC_indirect, but
 *       * uses the RAS predictor as well.
 *       * taken non annulled CTI: PC = nPC, nPC = EA */
//**************************************************************************
void   dynamic_inst_t::nextPC_return( abstract_pc_t *a )
{
    predictor_state_t *specBPS = m_pseq->getSpecBPS();
    ras_t *ras = m_pseq->getRAS();

    // make a return address prediction
    STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][2] );
    STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][m_priv? 1:0] );

    a->pc  = a->npc;
    a->npc = ras->pop( &specBPS->ras_state );

    /* store the predicted target (a->pc after the jump) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);
}

//**************************************************************************
void   dynamic_inst_t::nextPC_cwp( abstract_pc_t *a )
{
    predictor_state_t *specBPS = m_pseq->getSpecBPS();

    // advance the program counter (PC = nPC)
    a->pc  = a->npc;
    a->npc = a->npc + sizeof(uint32);

    // Task: update the cwp for saves and restores

    /* 
     *  Save/Restores (1) read from sources in old windows
     *                (2) write to dest in new windows
     *  The control_inst_t constructor helps us out in this case.
     */
    if (s->getOpcode() == i_save) {
        a->cwp = (a->cwp + 1) & (NWINDOWS - 1);
    } else if (s->getOpcode() == i_restore) {
        a->cwp = (a->cwp - 1) & (NWINDOWS - 1);
    } else {
#ifdef LL_DECODER
		int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
		bool curr_priv = ((a->pstate & 0x4) == 0x4);
		m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
				m_pseq->getRetiredICount(),
				pstate_t::trap_num_menomic((trap_type_t)0x10), 
				0x10,getVPC(),
				curr_priv, 0,
				(uint16)trap_level, m_pseq->getID()) ;
		HALT_SIMULATION ;
#else // LL_DECODER
		SIM_HALT;
#endif //LL_DECODER
    }

    // Note: you could speculatively enter the trap handler here too

    /* store the predicted target (a->pc after the jump) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);
}

//**************************************************************************
void   dynamic_inst_t::nextPC_nop( abstract_pc_t *a )
{
    // special case handler which does nothing to pc&npc
}

/** predicted conditional: 
 **    use predicated result to determine PC, nPC result
 **    Currently predicated branches are treated as a normal, unpredicated br
 */
//**************************************************************************
void   dynamic_inst_t::nextPC_predicated_branch( abstract_pc_t *a )
{
    direct_predictor_t  *bpred   = m_pseq->getDirectBP();
    predictor_state_t   *specBPS = m_pseq->getSpecBPS();
    bool prediction;

    // make a branch prediction
    STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][2] );
    STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][m_priv? 1:0] );

    prediction = bpred->Predict(a->pc, specBPS->cond_state,
            s->getFlag(SI_STATICPRED) );
    if (prediction) {
        // (cond, taken, annulled) and (cond, taken, not annulled) have
        // the same behavoir
        la_t targetPC = (int64) a->pc + (int64) s->getOffset();

        // LXL: An attempt to corrupt Branch target. This can be thought
        // of as output latch of the BTB
        if( m_fs->getFaultType() == BRANCH_TARGET_FAULT && a->fetch_id==m_fs->getFaultyReg() ) {
            la_t new_targetPC = m_fs->injectFault( targetPC );

            if( targetPC != new_targetPC ) {
                this->setFault() ;
                setControlFault();
            }

            targetPC = new_targetPC ;
        }

        a->pc  = a->npc;
        a->npc = targetPC;
    } else {

        if (s->getAnnul()) {
            // cond, not taken, annulled
            a->pc  = a->npc + sizeof(uint32);
            a->npc = a->npc + (2*sizeof(uint32));
        } else {
            // cond, not taken, not annulled
            a->pc  = a->npc;
            a->npc = a->npc + sizeof(uint32);
        }
    }

    /* speculatively update the branch predictor's history */
    specBPS->cond_state = ((specBPS->cond_state << 1) |
            (prediction & 0x1));

    /* store the predicted target (a->pc after the branch) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);
}

/** unpredicted conditional: use branch prediction to determine
 **    PC, nPC result
 */
//**************************************************************************
void   dynamic_inst_t::nextPC_predict_branch( abstract_pc_t *a )
{
    // get the branch predictor from the sequencer
    direct_predictor_t  *bpred   = m_pseq->getDirectBP();
    predictor_state_t   *specBPS = m_pseq->getSpecBPS();

    // make a branch prediction
    STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][2] );
    STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][m_priv? 1:0] );

    bool prediction = bpred->Predict(a->pc, specBPS->cond_state,
            s->getFlag(SI_STATICPRED) );
    if (prediction) {
        // (cond, taken, annulled) and (cond, taken, not annulled) have
        // the same behavoir
        la_t targetPC = (int64) a->pc + (int64) s->getOffset();

        // LXL: An attempt to corrupt Branch target. This can be thought
        // of as output latch of the BTB
        if( m_fs->getFaultType() == BRANCH_TARGET_FAULT && a->fetch_id==m_fs->getFaultyReg() ) {
            la_t new_targetPC = m_fs->injectFault( targetPC );

            if( targetPC != new_targetPC ) {
                this->setFault() ;
                setControlFault();
            }

            targetPC = new_targetPC ;
        }

        a->pc  = a->npc;
        a->npc = targetPC;
    } else {

        if (s->getAnnul()) {
            // cond, not taken, annulled
            a->pc  = a->npc + sizeof(uint32);
            a->npc = a->npc + (2*sizeof(uint32));
        } else {
            // cond, not taken, not annulled
            a->pc  = a->npc;
            a->npc = a->npc + sizeof(uint32);
        }
    }

    /* speculatively update the branch predictor's history */
    specBPS->cond_state = ((specBPS->cond_state << 1) |
            (prediction & 0x1));

    /* store the predicted target (a->pc after the branch) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);
}

/** register indirect delayed control transfer to effective address */
//**************************************************************************
void   dynamic_inst_t::nextPC_indirect( abstract_pc_t *a )
{
    // get the branch predictor from the sequencer
    cascaded_indirect_t     *bpred     = m_pseq->getIndirectBP();
    predictor_state_t       *specBPS   = m_pseq->getSpecBPS();

    // make a branch prediction
    STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][2] );
    STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][m_priv? 1:0] );

    my_addr_t predNPC = bpred->Predict( a->pc, &specBPS->indirect_state,
            &m_pseq->m_branch_except_stat[s->getBranchType()]);

    // LXL: An attempt to corrupt Branch target. This can be thought
    // of as output latch of the BTB
    if( m_fs->getFaultType() == BRANCH_TARGET_FAULT && a->fetch_id==m_fs->getFaultyReg() ) {
        la_t new_targetNPC = m_fs->injectFault( predNPC ); 

        if( predNPC != new_targetNPC ) {
            predNPC = new_targetNPC ;
        }
    }

    a->pc  = a->npc;
    a->npc = predNPC;

    /* store the predicted target (a->pc after the jump) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);

}

/// take a trap instruction (Tcc)
//**************************************************************************
void   dynamic_inst_t::nextPC_trap( abstract_pc_t *a )
{
    // get the branch predictor state from the sequencer
    // NOTE: this is not restored ever, it just enable us to call setpredtarget
    predictor_state_t *specBPS = m_pseq->getSpecBPS();

    // CM FIX should insert mechanism to speculate into traps successfully
    // However, since trap instructions are so _rare_ (almost never occurring)
    // this isn't a high priority.
#if 0
    ireg_t trapnum = getSourceReg(0).readInt64( m_pseq->getRegBox() );
    if ( s->getFlag(SI_ISIMMEDIATE) ) {
        trapnum += s->getImmediate();
    } else {
        trapnum += getSourceReg(1).readInt64( m_pseq->getRegBox() );
    }
#endif

    // predict the trap is NOT taken
    a->pc  = a->npc;
    a->npc = a->npc + sizeof(uint32);

    /* store the predicted target (a->pc after the branch) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);
}

//**************************************************************************
void   dynamic_inst_t::nextPC_priv( abstract_pc_t *a )
{
    // get the branch predictor state from the sequencer
    // NOTE: this is not restored ever, it just enable us to call setpredtarget
    predictor_state_t *specBPS = m_pseq->getSpecBPS();

    // advance the program counter (PC = nPC)
    a->pc  = a->npc;
    a->npc = a->npc + sizeof(uint32);

    // Three types of predictions are made here :PSTATE, TL, and CWP
    ireg_t value;
    if ( getDestReg(1).getVanilla() == CONTROL_PSTATE ) {
        if ( s->getFlag(SI_ISIMMEDIATE) ) {
            value = s->getImmediate();
        } else {
            value = 0x1a;
        }
        a->pstate = value;
        a->gset = pstate_t::getGlobalSet( a->pstate );
    } else if ( getDestReg(1).getVanilla() == CONTROL_TL ) {
        if ( s->getFlag(SI_ISIMMEDIATE) ) {
            value = s->getImmediate();
        } else {
            if (a->tl == 0) {
                value = 1;
            } else {
                value = a->tl - 1;
            }
        }
        a->tl = value;
    } else if ( getDestReg(1).getVanilla() == CONTROL_CWP ) {
        if ( s->getFlag(SI_ISIMMEDIATE) ) {
            value = s->getImmediate();
        } else {
            value = (a->cwp + 1) & (NWINDOWS - 1);
        }
        a->cwp = value;
    } else {
        ERROR_OUT("dynamic_inst: nextPC_priv: unknown instruction type\n");
    }

    /* store the predicted target (a->pc after the branch) with the 
     * dynamic instruction, so we can later verify the prediction */ 
    control_inst_t *control_p = static_cast<control_inst_t *>( this );
    control_p->setPredictedTarget(a);
    control_p->setPredictorState(*specBPS);
}

//**************************************************************************
void dynamic_inst_t::initialize( void )
{
    for (uint32 i = 0; i < i_maxcount; i++) {
        m_jump_table[i] = &dynamic_inst_t::dx_ill;
    }

    m_jump_table[i_add] = &dynamic_inst_t::dx_add;
    m_jump_table[i_addcc] = &dynamic_inst_t::dx_addcc;
    m_jump_table[i_addc] = &dynamic_inst_t::dx_addc;
    m_jump_table[i_addccc] = &dynamic_inst_t::dx_addccc;
    m_jump_table[i_and] = &dynamic_inst_t::dx_and;
    m_jump_table[i_andcc] = &dynamic_inst_t::dx_andcc;
    m_jump_table[i_andn] = &dynamic_inst_t::dx_andn;
    m_jump_table[i_andncc] = &dynamic_inst_t::dx_andncc;
    m_jump_table[i_fba] = &dynamic_inst_t::dx_fba;
    m_jump_table[i_ba] = &dynamic_inst_t::dx_ba;
    m_jump_table[i_fbpa] = &dynamic_inst_t::dx_fbpa;
    m_jump_table[i_bpa] = &dynamic_inst_t::dx_bpa;
    m_jump_table[i_fbn] = &dynamic_inst_t::dx_fbn;
    m_jump_table[i_fbpn] = &dynamic_inst_t::dx_fbpn;
    m_jump_table[i_bn] = &dynamic_inst_t::dx_bn;
    m_jump_table[i_bpn] = &dynamic_inst_t::dx_bpn;
    m_jump_table[i_bpne] = &dynamic_inst_t::dx_bpne;
    m_jump_table[i_bpe] = &dynamic_inst_t::dx_bpe;
    m_jump_table[i_bpg] = &dynamic_inst_t::dx_bpg;
    m_jump_table[i_bple] = &dynamic_inst_t::dx_bple;
    m_jump_table[i_bpge] = &dynamic_inst_t::dx_bpge;
    m_jump_table[i_bpl] = &dynamic_inst_t::dx_bpl;
    m_jump_table[i_bpgu] = &dynamic_inst_t::dx_bpgu;
    m_jump_table[i_bpleu] = &dynamic_inst_t::dx_bpleu;
    m_jump_table[i_bpcc] = &dynamic_inst_t::dx_bpcc;
    m_jump_table[i_bpcs] = &dynamic_inst_t::dx_bpcs;
    m_jump_table[i_bppos] = &dynamic_inst_t::dx_bppos;
    m_jump_table[i_bpneg] = &dynamic_inst_t::dx_bpneg;
    m_jump_table[i_bpvc] = &dynamic_inst_t::dx_bpvc;
    m_jump_table[i_bpvs] = &dynamic_inst_t::dx_bpvs;
    m_jump_table[i_call] = &dynamic_inst_t::dx_call;
    m_jump_table[i_casa] = &dynamic_inst_t::dx_casa;
    m_jump_table[i_casxa] = &dynamic_inst_t::dx_casxa;
    m_jump_table[i_done] = &dynamic_inst_t::dx_doneretry;
    m_jump_table[i_retry] = &dynamic_inst_t::dx_doneretry;
    m_jump_table[i_jmpl] = &dynamic_inst_t::dx_jmpl;
    m_jump_table[i_fabss] = &dynamic_inst_t::dx_fabss;
    m_jump_table[i_fabsd] = &dynamic_inst_t::dx_fabsd;
    m_jump_table[i_fabsq] = &dynamic_inst_t::dx_fabsq;
    m_jump_table[i_fadds] = &dynamic_inst_t::dx_fadds;
    m_jump_table[i_faddd] = &dynamic_inst_t::dx_faddd;
    m_jump_table[i_faddq] = &dynamic_inst_t::dx_faddq;
    m_jump_table[i_fsubs] = &dynamic_inst_t::dx_fsubs;
    m_jump_table[i_fsubd] = &dynamic_inst_t::dx_fsubd;
    m_jump_table[i_fsubq] = &dynamic_inst_t::dx_fsubq;
    m_jump_table[i_fcmps] = &dynamic_inst_t::dx_fcmps;
    m_jump_table[i_fcmpd] = &dynamic_inst_t::dx_fcmpd;
    m_jump_table[i_fcmpq] = &dynamic_inst_t::dx_fcmpq;
    m_jump_table[i_fcmpes] = &dynamic_inst_t::dx_fcmpes;
    m_jump_table[i_fcmped] = &dynamic_inst_t::dx_fcmped;
    m_jump_table[i_fcmpeq] = &dynamic_inst_t::dx_fcmpeq;
    m_jump_table[i_fmovs] = &dynamic_inst_t::dx_fmovs;
    m_jump_table[i_fmovd] = &dynamic_inst_t::dx_fmovd;
    m_jump_table[i_fmovq] = &dynamic_inst_t::dx_fmovq;
    m_jump_table[i_fnegs] = &dynamic_inst_t::dx_fnegs;
    m_jump_table[i_fnegd] = &dynamic_inst_t::dx_fnegd;
    m_jump_table[i_fnegq] = &dynamic_inst_t::dx_fnegq;
    m_jump_table[i_fmuls] = &dynamic_inst_t::dx_fmuls;
    m_jump_table[i_fmuld] = &dynamic_inst_t::dx_fmuld;
    m_jump_table[i_fmulq] = &dynamic_inst_t::dx_fmulq;
    m_jump_table[i_fdivs] = &dynamic_inst_t::dx_fdivs;
    m_jump_table[i_fdivd] = &dynamic_inst_t::dx_fdivd;
    m_jump_table[i_fdivq] = &dynamic_inst_t::dx_fdivq;
    m_jump_table[i_fsmuld] = &dynamic_inst_t::dx_fsmuld;
    m_jump_table[i_fdmulq] = &dynamic_inst_t::dx_fdmulq;
    m_jump_table[i_fsqrts] = &dynamic_inst_t::dx_fsqrts;
    m_jump_table[i_fsqrtd] = &dynamic_inst_t::dx_fsqrtd;
    m_jump_table[i_fsqrtq] = &dynamic_inst_t::dx_fsqrtq;
    m_jump_table[i_retrn] = &dynamic_inst_t::dx_retrn;
    m_jump_table[i_brz] = &dynamic_inst_t::dx_brz;
    m_jump_table[i_brlez] = &dynamic_inst_t::dx_brlez;
    m_jump_table[i_brlz] = &dynamic_inst_t::dx_brlz;
    m_jump_table[i_brnz] = &dynamic_inst_t::dx_brnz;
    m_jump_table[i_brgz] = &dynamic_inst_t::dx_brgz;
    m_jump_table[i_brgez] = &dynamic_inst_t::dx_brgez;
    m_jump_table[i_fbu] = &dynamic_inst_t::dx_fbu;
    m_jump_table[i_fbg] = &dynamic_inst_t::dx_fbg;
    m_jump_table[i_fbug] = &dynamic_inst_t::dx_fbug;
    m_jump_table[i_fbl] = &dynamic_inst_t::dx_fbl;
    m_jump_table[i_fbul] = &dynamic_inst_t::dx_fbul;
    m_jump_table[i_fblg] = &dynamic_inst_t::dx_fblg;
    m_jump_table[i_fbne] = &dynamic_inst_t::dx_fbne;
    m_jump_table[i_fbe] = &dynamic_inst_t::dx_fbe;
    m_jump_table[i_fbue] = &dynamic_inst_t::dx_fbue;
    m_jump_table[i_fbge] = &dynamic_inst_t::dx_fbge;
    m_jump_table[i_fbuge] = &dynamic_inst_t::dx_fbuge;
    m_jump_table[i_fble] = &dynamic_inst_t::dx_fble;
    m_jump_table[i_fbule] = &dynamic_inst_t::dx_fbule;
    m_jump_table[i_fbo] = &dynamic_inst_t::dx_fbo;
    m_jump_table[i_fbpu] = &dynamic_inst_t::dx_fbpu;
    m_jump_table[i_fbpg] = &dynamic_inst_t::dx_fbpg;
    m_jump_table[i_fbpug] = &dynamic_inst_t::dx_fbpug;
    m_jump_table[i_fbpl] = &dynamic_inst_t::dx_fbpl;
    m_jump_table[i_fbpul] = &dynamic_inst_t::dx_fbpul;
    m_jump_table[i_fbplg] = &dynamic_inst_t::dx_fbplg;
    m_jump_table[i_fbpne] = &dynamic_inst_t::dx_fbpne;
    m_jump_table[i_fbpe] = &dynamic_inst_t::dx_fbpe;
    m_jump_table[i_fbpue] = &dynamic_inst_t::dx_fbpue;
    m_jump_table[i_fbpge] = &dynamic_inst_t::dx_fbpge;
    m_jump_table[i_fbpuge] = &dynamic_inst_t::dx_fbpuge;
    m_jump_table[i_fbple] = &dynamic_inst_t::dx_fbple;
    m_jump_table[i_fbpule] = &dynamic_inst_t::dx_fbpule;
    m_jump_table[i_fbpo] = &dynamic_inst_t::dx_fbpo;
    m_jump_table[i_bne] = &dynamic_inst_t::dx_bne;
    m_jump_table[i_be] = &dynamic_inst_t::dx_be;
    m_jump_table[i_bg] = &dynamic_inst_t::dx_bg;
    m_jump_table[i_ble] = &dynamic_inst_t::dx_ble;
    m_jump_table[i_bge] = &dynamic_inst_t::dx_bge;
    m_jump_table[i_bl] = &dynamic_inst_t::dx_bl;
    m_jump_table[i_bgu] = &dynamic_inst_t::dx_bgu;
    m_jump_table[i_bleu] = &dynamic_inst_t::dx_bleu;
    m_jump_table[i_bcc] = &dynamic_inst_t::dx_bcc;
    m_jump_table[i_bcs] = &dynamic_inst_t::dx_bcs;
    m_jump_table[i_bpos] = &dynamic_inst_t::dx_bpos;
    m_jump_table[i_bneg] = &dynamic_inst_t::dx_bneg;
    m_jump_table[i_bvc] = &dynamic_inst_t::dx_bvc;
    m_jump_table[i_bvs] = &dynamic_inst_t::dx_bvs;
    m_jump_table[i_fstox] = &dynamic_inst_t::dx_fstox;
    m_jump_table[i_fdtox] = &dynamic_inst_t::dx_fdtox;
    m_jump_table[i_fqtox] = &dynamic_inst_t::dx_fqtox;
    m_jump_table[i_fstoi] = &dynamic_inst_t::dx_fstoi;
    m_jump_table[i_fdtoi] = &dynamic_inst_t::dx_fdtoi;
    m_jump_table[i_fqtoi] = &dynamic_inst_t::dx_fqtoi;
    m_jump_table[i_fstod] = &dynamic_inst_t::dx_fstod;
    m_jump_table[i_fstoq] = &dynamic_inst_t::dx_fstoq;
    m_jump_table[i_fdtos] = &dynamic_inst_t::dx_fdtos;
    m_jump_table[i_fdtoq] = &dynamic_inst_t::dx_fdtoq;
    m_jump_table[i_fqtos] = &dynamic_inst_t::dx_fqtos;
    m_jump_table[i_fqtod] = &dynamic_inst_t::dx_fqtod;
    m_jump_table[i_fxtos] = &dynamic_inst_t::dx_fxtos;
    m_jump_table[i_fxtod] = &dynamic_inst_t::dx_fxtod;
    m_jump_table[i_fxtoq] = &dynamic_inst_t::dx_fxtoq;
    m_jump_table[i_fitos] = &dynamic_inst_t::dx_fitos;
    m_jump_table[i_fitod] = &dynamic_inst_t::dx_fitod;
    m_jump_table[i_fitoq] = &dynamic_inst_t::dx_fitoq;
    m_jump_table[i_fmovfsn] = &dynamic_inst_t::dx_fmovfsn;
    m_jump_table[i_fmovfsne] = &dynamic_inst_t::dx_fmovfsne;
    m_jump_table[i_fmovfslg] = &dynamic_inst_t::dx_fmovfslg;
    m_jump_table[i_fmovfsul] = &dynamic_inst_t::dx_fmovfsul;
    m_jump_table[i_fmovfsl] = &dynamic_inst_t::dx_fmovfsl;
    m_jump_table[i_fmovfsug] = &dynamic_inst_t::dx_fmovfsug;
    m_jump_table[i_fmovfsg] = &dynamic_inst_t::dx_fmovfsg;
    m_jump_table[i_fmovfsu] = &dynamic_inst_t::dx_fmovfsu;
    m_jump_table[i_fmovfsa] = &dynamic_inst_t::dx_fmovfsa;
    m_jump_table[i_fmovfse] = &dynamic_inst_t::dx_fmovfse;
    m_jump_table[i_fmovfsue] = &dynamic_inst_t::dx_fmovfsue;
    m_jump_table[i_fmovfsge] = &dynamic_inst_t::dx_fmovfsge;
    m_jump_table[i_fmovfsuge] = &dynamic_inst_t::dx_fmovfsuge;
    m_jump_table[i_fmovfsle] = &dynamic_inst_t::dx_fmovfsle;
    m_jump_table[i_fmovfsule] = &dynamic_inst_t::dx_fmovfsule;
    m_jump_table[i_fmovfso] = &dynamic_inst_t::dx_fmovfso;
    m_jump_table[i_fmovfdn] = &dynamic_inst_t::dx_fmovfdn;
    m_jump_table[i_fmovfdne] = &dynamic_inst_t::dx_fmovfdne;
    m_jump_table[i_fmovfdlg] = &dynamic_inst_t::dx_fmovfdlg;
    m_jump_table[i_fmovfdul] = &dynamic_inst_t::dx_fmovfdul;
    m_jump_table[i_fmovfdl] = &dynamic_inst_t::dx_fmovfdl;
    m_jump_table[i_fmovfdug] = &dynamic_inst_t::dx_fmovfdug;
    m_jump_table[i_fmovfdg] = &dynamic_inst_t::dx_fmovfdg;
    m_jump_table[i_fmovfdu] = &dynamic_inst_t::dx_fmovfdu;
    m_jump_table[i_fmovfda] = &dynamic_inst_t::dx_fmovfda;
    m_jump_table[i_fmovfde] = &dynamic_inst_t::dx_fmovfde;
    m_jump_table[i_fmovfdue] = &dynamic_inst_t::dx_fmovfdue;
    m_jump_table[i_fmovfdge] = &dynamic_inst_t::dx_fmovfdge;
    m_jump_table[i_fmovfduge] = &dynamic_inst_t::dx_fmovfduge;
    m_jump_table[i_fmovfdle] = &dynamic_inst_t::dx_fmovfdle;
    m_jump_table[i_fmovfdule] = &dynamic_inst_t::dx_fmovfdule;
    m_jump_table[i_fmovfdo] = &dynamic_inst_t::dx_fmovfdo;
    m_jump_table[i_fmovfqn] = &dynamic_inst_t::dx_fmovfqn;
    m_jump_table[i_fmovfqne] = &dynamic_inst_t::dx_fmovfqne;
    m_jump_table[i_fmovfqlg] = &dynamic_inst_t::dx_fmovfqlg;
    m_jump_table[i_fmovfqul] = &dynamic_inst_t::dx_fmovfqul;
    m_jump_table[i_fmovfql] = &dynamic_inst_t::dx_fmovfql;
    m_jump_table[i_fmovfqug] = &dynamic_inst_t::dx_fmovfqug;
    m_jump_table[i_fmovfqg] = &dynamic_inst_t::dx_fmovfqg;
    m_jump_table[i_fmovfqu] = &dynamic_inst_t::dx_fmovfqu;
    m_jump_table[i_fmovfqa] = &dynamic_inst_t::dx_fmovfqa;
    m_jump_table[i_fmovfqe] = &dynamic_inst_t::dx_fmovfqe;
    m_jump_table[i_fmovfque] = &dynamic_inst_t::dx_fmovfque;
    m_jump_table[i_fmovfqge] = &dynamic_inst_t::dx_fmovfqge;
    m_jump_table[i_fmovfquge] = &dynamic_inst_t::dx_fmovfquge;
    m_jump_table[i_fmovfqle] = &dynamic_inst_t::dx_fmovfqle;
    m_jump_table[i_fmovfqule] = &dynamic_inst_t::dx_fmovfqule;
    m_jump_table[i_fmovfqo] = &dynamic_inst_t::dx_fmovfqo;
    m_jump_table[i_fmovsn] = &dynamic_inst_t::dx_fmovsn;
    m_jump_table[i_fmovse] = &dynamic_inst_t::dx_fmovse;
    m_jump_table[i_fmovsle] = &dynamic_inst_t::dx_fmovsle;
    m_jump_table[i_fmovsl] = &dynamic_inst_t::dx_fmovsl;
    m_jump_table[i_fmovsleu] = &dynamic_inst_t::dx_fmovsleu;
    m_jump_table[i_fmovscs] = &dynamic_inst_t::dx_fmovscs;
    m_jump_table[i_fmovsneg] = &dynamic_inst_t::dx_fmovsneg;
    m_jump_table[i_fmovsvs] = &dynamic_inst_t::dx_fmovsvs;
    m_jump_table[i_fmovsa] = &dynamic_inst_t::dx_fmovsa;
    m_jump_table[i_fmovsne] = &dynamic_inst_t::dx_fmovsne;
    m_jump_table[i_fmovsg] = &dynamic_inst_t::dx_fmovsg;
    m_jump_table[i_fmovsge] = &dynamic_inst_t::dx_fmovsge;
    m_jump_table[i_fmovsgu] = &dynamic_inst_t::dx_fmovsgu;
    m_jump_table[i_fmovscc] = &dynamic_inst_t::dx_fmovscc;
    m_jump_table[i_fmovspos] = &dynamic_inst_t::dx_fmovspos;
    m_jump_table[i_fmovsvc] = &dynamic_inst_t::dx_fmovsvc;
    m_jump_table[i_fmovdn] = &dynamic_inst_t::dx_fmovdn;
    m_jump_table[i_fmovde] = &dynamic_inst_t::dx_fmovde;
    m_jump_table[i_fmovdle] = &dynamic_inst_t::dx_fmovdle;
    m_jump_table[i_fmovdl] = &dynamic_inst_t::dx_fmovdl;
    m_jump_table[i_fmovdleu] = &dynamic_inst_t::dx_fmovdleu;
    m_jump_table[i_fmovdcs] = &dynamic_inst_t::dx_fmovdcs;
    m_jump_table[i_fmovdneg] = &dynamic_inst_t::dx_fmovdneg;
    m_jump_table[i_fmovdvs] = &dynamic_inst_t::dx_fmovdvs;
    m_jump_table[i_fmovda] = &dynamic_inst_t::dx_fmovda;
    m_jump_table[i_fmovdne] = &dynamic_inst_t::dx_fmovdne;
    m_jump_table[i_fmovdg] = &dynamic_inst_t::dx_fmovdg;
    m_jump_table[i_fmovdge] = &dynamic_inst_t::dx_fmovdge;
    m_jump_table[i_fmovdgu] = &dynamic_inst_t::dx_fmovdgu;
    m_jump_table[i_fmovdcc] = &dynamic_inst_t::dx_fmovdcc;
    m_jump_table[i_fmovdpos] = &dynamic_inst_t::dx_fmovdpos;
    m_jump_table[i_fmovdvc] = &dynamic_inst_t::dx_fmovdvc;
    m_jump_table[i_fmovqn] = &dynamic_inst_t::dx_fmovqn;
    m_jump_table[i_fmovqe] = &dynamic_inst_t::dx_fmovqe;
    m_jump_table[i_fmovqle] = &dynamic_inst_t::dx_fmovqle;
    m_jump_table[i_fmovql] = &dynamic_inst_t::dx_fmovql;
    m_jump_table[i_fmovqleu] = &dynamic_inst_t::dx_fmovqleu;
    m_jump_table[i_fmovqcs] = &dynamic_inst_t::dx_fmovqcs;
    m_jump_table[i_fmovqneg] = &dynamic_inst_t::dx_fmovqneg;
    m_jump_table[i_fmovqvs] = &dynamic_inst_t::dx_fmovqvs;
    m_jump_table[i_fmovqa] = &dynamic_inst_t::dx_fmovqa;
    m_jump_table[i_fmovqne] = &dynamic_inst_t::dx_fmovqne;
    m_jump_table[i_fmovqg] = &dynamic_inst_t::dx_fmovqg;
    m_jump_table[i_fmovqge] = &dynamic_inst_t::dx_fmovqge;
    m_jump_table[i_fmovqgu] = &dynamic_inst_t::dx_fmovqgu;
    m_jump_table[i_fmovqcc] = &dynamic_inst_t::dx_fmovqcc;
    m_jump_table[i_fmovqpos] = &dynamic_inst_t::dx_fmovqpos;
    m_jump_table[i_fmovqvc] = &dynamic_inst_t::dx_fmovqvc;
    m_jump_table[i_fmovrsz] = &dynamic_inst_t::dx_fmovrsz;
    m_jump_table[i_fmovrslez] = &dynamic_inst_t::dx_fmovrslez;
    m_jump_table[i_fmovrslz] = &dynamic_inst_t::dx_fmovrslz;
    m_jump_table[i_fmovrsnz] = &dynamic_inst_t::dx_fmovrsnz;
    m_jump_table[i_fmovrsgz] = &dynamic_inst_t::dx_fmovrsgz;
    m_jump_table[i_fmovrsgez] = &dynamic_inst_t::dx_fmovrsgez;
    m_jump_table[i_fmovrdz] = &dynamic_inst_t::dx_fmovrdz;
    m_jump_table[i_fmovrdlez] = &dynamic_inst_t::dx_fmovrdlez;
    m_jump_table[i_fmovrdlz] = &dynamic_inst_t::dx_fmovrdlz;
    m_jump_table[i_fmovrdnz] = &dynamic_inst_t::dx_fmovrdnz;
    m_jump_table[i_fmovrdgz] = &dynamic_inst_t::dx_fmovrdgz;
    m_jump_table[i_fmovrdgez] = &dynamic_inst_t::dx_fmovrdgez;
    m_jump_table[i_fmovrqz] = &dynamic_inst_t::dx_fmovrqz;
    m_jump_table[i_fmovrqlez] = &dynamic_inst_t::dx_fmovrqlez;
    m_jump_table[i_fmovrqlz] = &dynamic_inst_t::dx_fmovrqlz;
    m_jump_table[i_fmovrqnz] = &dynamic_inst_t::dx_fmovrqnz;
    m_jump_table[i_fmovrqgz] = &dynamic_inst_t::dx_fmovrqgz;
    m_jump_table[i_fmovrqgez] = &dynamic_inst_t::dx_fmovrqgez;
    m_jump_table[i_mova] = &dynamic_inst_t::dx_mova;
    m_jump_table[i_movfa] = &dynamic_inst_t::dx_movfa;
    m_jump_table[i_movn] = &dynamic_inst_t::dx_movn;
    m_jump_table[i_movfn] = &dynamic_inst_t::dx_movfn;
    m_jump_table[i_movne] = &dynamic_inst_t::dx_movne;
    m_jump_table[i_move] = &dynamic_inst_t::dx_move;
    m_jump_table[i_movg] = &dynamic_inst_t::dx_movg;
    m_jump_table[i_movle] = &dynamic_inst_t::dx_movle;
    m_jump_table[i_movge] = &dynamic_inst_t::dx_movge;
    m_jump_table[i_movl] = &dynamic_inst_t::dx_movl;
    m_jump_table[i_movgu] = &dynamic_inst_t::dx_movgu;
    m_jump_table[i_movleu] = &dynamic_inst_t::dx_movleu;
    m_jump_table[i_movcc] = &dynamic_inst_t::dx_movcc;
    m_jump_table[i_movcs] = &dynamic_inst_t::dx_movcs;
    m_jump_table[i_movpos] = &dynamic_inst_t::dx_movpos;
    m_jump_table[i_movneg] = &dynamic_inst_t::dx_movneg;
    m_jump_table[i_movvc] = &dynamic_inst_t::dx_movvc;
    m_jump_table[i_movvs] = &dynamic_inst_t::dx_movvs;
    m_jump_table[i_movfu] = &dynamic_inst_t::dx_movfu;
    m_jump_table[i_movfg] = &dynamic_inst_t::dx_movfg;
    m_jump_table[i_movfug] = &dynamic_inst_t::dx_movfug;
    m_jump_table[i_movfl] = &dynamic_inst_t::dx_movfl;
    m_jump_table[i_movful] = &dynamic_inst_t::dx_movful;
    m_jump_table[i_movflg] = &dynamic_inst_t::dx_movflg;
    m_jump_table[i_movfne] = &dynamic_inst_t::dx_movfne;
    m_jump_table[i_movfe] = &dynamic_inst_t::dx_movfe;
    m_jump_table[i_movfue] = &dynamic_inst_t::dx_movfue;
    m_jump_table[i_movfge] = &dynamic_inst_t::dx_movfge;
    m_jump_table[i_movfuge] = &dynamic_inst_t::dx_movfuge;
    m_jump_table[i_movfle] = &dynamic_inst_t::dx_movfle;
    m_jump_table[i_movfule] = &dynamic_inst_t::dx_movfule;
    m_jump_table[i_movfo] = &dynamic_inst_t::dx_movfo;
    m_jump_table[i_movrz] = &dynamic_inst_t::dx_movrz;
    m_jump_table[i_movrlez] = &dynamic_inst_t::dx_movrlez;
    m_jump_table[i_movrlz] = &dynamic_inst_t::dx_movrlz;
    m_jump_table[i_movrnz] = &dynamic_inst_t::dx_movrnz;
    m_jump_table[i_movrgz] = &dynamic_inst_t::dx_movrgz;
    m_jump_table[i_movrgez] = &dynamic_inst_t::dx_movrgez;
    m_jump_table[i_ta] = &dynamic_inst_t::dx_ta;
    m_jump_table[i_tn] = &dynamic_inst_t::dx_tn;
    m_jump_table[i_tne] = &dynamic_inst_t::dx_tne;
    m_jump_table[i_te] = &dynamic_inst_t::dx_te;
    m_jump_table[i_tg] = &dynamic_inst_t::dx_tg;
    m_jump_table[i_tle] = &dynamic_inst_t::dx_tle;
    m_jump_table[i_tge] = &dynamic_inst_t::dx_tge;
    m_jump_table[i_tl] = &dynamic_inst_t::dx_tl;
    m_jump_table[i_tgu] = &dynamic_inst_t::dx_tgu;
    m_jump_table[i_tleu] = &dynamic_inst_t::dx_tleu;
    m_jump_table[i_tcc] = &dynamic_inst_t::dx_tcc;
    m_jump_table[i_tcs] = &dynamic_inst_t::dx_tcs;
    m_jump_table[i_tpos] = &dynamic_inst_t::dx_tpos;
    m_jump_table[i_tneg] = &dynamic_inst_t::dx_tneg;
    m_jump_table[i_tvc] = &dynamic_inst_t::dx_tvc;
    m_jump_table[i_tvs] = &dynamic_inst_t::dx_tvs;
    m_jump_table[i_sub] = &dynamic_inst_t::dx_sub;
    m_jump_table[i_subcc] = &dynamic_inst_t::dx_subcc;
    m_jump_table[i_subc] = &dynamic_inst_t::dx_subc;
    m_jump_table[i_subccc] = &dynamic_inst_t::dx_subccc;
    m_jump_table[i_or] = &dynamic_inst_t::dx_or;
    m_jump_table[i_orcc] = &dynamic_inst_t::dx_orcc;
    m_jump_table[i_orn] = &dynamic_inst_t::dx_orn;
    m_jump_table[i_orncc] = &dynamic_inst_t::dx_orncc;
    m_jump_table[i_xor] = &dynamic_inst_t::dx_xor;
    m_jump_table[i_xorcc] = &dynamic_inst_t::dx_xorcc;
    m_jump_table[i_xnor] = &dynamic_inst_t::dx_xnor;
    m_jump_table[i_xnorcc] = &dynamic_inst_t::dx_xnorcc;
    m_jump_table[i_mulx] = &dynamic_inst_t::dx_mulx;
    m_jump_table[i_sdivx] = &dynamic_inst_t::dx_sdivx;
    m_jump_table[i_udivx] = &dynamic_inst_t::dx_udivx;
    m_jump_table[i_sll] = &dynamic_inst_t::dx_sll;
    m_jump_table[i_srl] = &dynamic_inst_t::dx_srl;
    m_jump_table[i_sra] = &dynamic_inst_t::dx_sra;
    m_jump_table[i_sllx] = &dynamic_inst_t::dx_sllx;
    m_jump_table[i_srlx] = &dynamic_inst_t::dx_srlx;
    m_jump_table[i_srax] = &dynamic_inst_t::dx_srax;
    m_jump_table[i_taddcc] = &dynamic_inst_t::dx_taddcc;
    m_jump_table[i_taddcctv] = &dynamic_inst_t::dx_taddcctv;
    m_jump_table[i_tsubcc] = &dynamic_inst_t::dx_tsubcc;
    m_jump_table[i_tsubcctv] = &dynamic_inst_t::dx_tsubcctv;
    m_jump_table[i_udiv] = &dynamic_inst_t::dx_udiv;
    m_jump_table[i_sdiv] = &dynamic_inst_t::dx_sdiv;
    m_jump_table[i_umul] = &dynamic_inst_t::dx_umul;
    m_jump_table[i_smul] = &dynamic_inst_t::dx_smul;
    m_jump_table[i_udivcc] = &dynamic_inst_t::dx_udivcc;
    m_jump_table[i_sdivcc] = &dynamic_inst_t::dx_sdivcc;
    m_jump_table[i_umulcc] = &dynamic_inst_t::dx_umulcc;
    m_jump_table[i_smulcc] = &dynamic_inst_t::dx_smulcc;
    m_jump_table[i_mulscc] = &dynamic_inst_t::dx_mulscc;
    m_jump_table[i_popc] = &dynamic_inst_t::dx_popc;
    m_jump_table[i_flush] = &dynamic_inst_t::dx_flush;
    m_jump_table[i_flushw] = &dynamic_inst_t::dx_flushw;
    m_jump_table[i_rd] = &dynamic_inst_t::dx_rd;
    m_jump_table[i_rdcc] = &dynamic_inst_t::dx_rdcc;
    m_jump_table[i_wr] = &dynamic_inst_t::dx_wr;
    m_jump_table[i_wrcc] = &dynamic_inst_t::dx_wrcc;
    m_jump_table[i_save] = &dynamic_inst_t::dx_save;
    m_jump_table[i_restore] = &dynamic_inst_t::dx_restore;
    m_jump_table[i_saved] = &dynamic_inst_t::dx_saved;
    m_jump_table[i_restored] = &dynamic_inst_t::dx_restored;
    m_jump_table[i_sethi] = &dynamic_inst_t::dx_sethi;
    m_jump_table[i_ldf] = &dynamic_inst_t::dx_ldf;
    m_jump_table[i_lddf] = &dynamic_inst_t::dx_lddf;
    m_jump_table[i_ldqf] = &dynamic_inst_t::dx_ldqf;
    m_jump_table[i_ldfa] = &dynamic_inst_t::dx_ldfa;
    m_jump_table[i_lddfa] = &dynamic_inst_t::dx_lddfa;
    m_jump_table[i_ldblk] = &dynamic_inst_t::dx_ldblk;
    m_jump_table[i_ldqfa] = &dynamic_inst_t::dx_ldqfa;
    m_jump_table[i_ldsb] = &dynamic_inst_t::dx_ldsb;
    m_jump_table[i_ldsh] = &dynamic_inst_t::dx_ldsh;
    m_jump_table[i_ldsw] = &dynamic_inst_t::dx_ldsw;
    m_jump_table[i_ldub] = &dynamic_inst_t::dx_ldub;
    m_jump_table[i_lduh] = &dynamic_inst_t::dx_lduh;
    m_jump_table[i_lduw] = &dynamic_inst_t::dx_lduw;
    m_jump_table[i_ldx] = &dynamic_inst_t::dx_ldx;
    m_jump_table[i_ldd] = &dynamic_inst_t::dx_ldd;
    m_jump_table[i_ldsba] = &dynamic_inst_t::dx_ldsba;
    m_jump_table[i_ldsha] = &dynamic_inst_t::dx_ldsha;
    m_jump_table[i_ldswa] = &dynamic_inst_t::dx_ldswa;
    m_jump_table[i_lduba] = &dynamic_inst_t::dx_lduba;
    m_jump_table[i_lduha] = &dynamic_inst_t::dx_lduha;
    m_jump_table[i_lduwa] = &dynamic_inst_t::dx_lduwa;
    m_jump_table[i_ldxa] = &dynamic_inst_t::dx_ldxa;
    m_jump_table[i_ldda] = &dynamic_inst_t::dx_ldda;
    m_jump_table[i_ldqa] = &dynamic_inst_t::dx_ldqa;
    m_jump_table[i_stf] = &dynamic_inst_t::dx_stf;
    m_jump_table[i_stdf] = &dynamic_inst_t::dx_stdf;
    m_jump_table[i_stqf] = &dynamic_inst_t::dx_stqf;
    m_jump_table[i_stb] = &dynamic_inst_t::dx_stb;
    m_jump_table[i_sth] = &dynamic_inst_t::dx_sth;
    m_jump_table[i_stw] = &dynamic_inst_t::dx_stw;
    m_jump_table[i_stx] = &dynamic_inst_t::dx_stx;
    m_jump_table[i_std] = &dynamic_inst_t::dx_std;
    m_jump_table[i_stfa] = &dynamic_inst_t::dx_stfa;
    m_jump_table[i_stdfa] = &dynamic_inst_t::dx_stdfa;
    m_jump_table[i_stblk] = &dynamic_inst_t::dx_stblk;
    m_jump_table[i_stqfa] = &dynamic_inst_t::dx_stqfa;
    m_jump_table[i_stba] = &dynamic_inst_t::dx_stba;
    m_jump_table[i_stha] = &dynamic_inst_t::dx_stha;
    m_jump_table[i_stwa] = &dynamic_inst_t::dx_stwa;
    m_jump_table[i_stxa] = &dynamic_inst_t::dx_stxa;
    m_jump_table[i_stda] = &dynamic_inst_t::dx_stda;
    m_jump_table[i_ldstub] = &dynamic_inst_t::dx_ldstub;
    m_jump_table[i_ldstuba] = &dynamic_inst_t::dx_ldstuba;
    m_jump_table[i_prefetch] = &dynamic_inst_t::dx_prefetch;
    m_jump_table[i_prefetcha] = &dynamic_inst_t::dx_prefetcha;
    m_jump_table[i_swap] = &dynamic_inst_t::dx_swap;
    m_jump_table[i_swapa] = &dynamic_inst_t::dx_swapa;
    m_jump_table[i_ldfsr] = &dynamic_inst_t::dx_ldfsr;
    m_jump_table[i_ldxfsr] = &dynamic_inst_t::dx_ldxfsr;
    m_jump_table[i_stfsr] = &dynamic_inst_t::dx_stfsr;
    m_jump_table[i_stxfsr] = &dynamic_inst_t::dx_stxfsr;
    m_jump_table[i__trap] = &dynamic_inst_t::dx__trap;
    m_jump_table[i_impdep1] = &dynamic_inst_t::dx_impdep1;
    m_jump_table[i_impdep2] = &dynamic_inst_t::dx_impdep2;
    m_jump_table[i_membar] = &dynamic_inst_t::dx_membar;
    m_jump_table[i_stbar] = &dynamic_inst_t::dx_stbar;
    m_jump_table[i_cmp] = &dynamic_inst_t::dx_cmp;
    m_jump_table[i_jmp] = &dynamic_inst_t::dx_jmp;
    m_jump_table[i_mov] = &dynamic_inst_t::dx_mov;
    m_jump_table[i_nop] = &dynamic_inst_t::dx_nop;
    m_jump_table[i_not] = &dynamic_inst_t::dx_not;
    m_jump_table[i_rdpr] = &dynamic_inst_t::dx_rdpr;
    m_jump_table[i_wrpr] = &dynamic_inst_t::dx_wrpr;
    m_jump_table[i_faligndata] = &dynamic_inst_t::dx_faligndata;
    m_jump_table[i_alignaddr] = &dynamic_inst_t::dx_alignaddr;
    m_jump_table[i_alignaddrl] = &dynamic_inst_t::dx_alignaddrl;
    m_jump_table[i_fzero] = &dynamic_inst_t::dx_fzero;
    m_jump_table[i_fzeros] = &dynamic_inst_t::dx_fzeros;
    m_jump_table[i_fnor] = &dynamic_inst_t::dx_fnor;
    m_jump_table[i_fnors] = &dynamic_inst_t::dx_fnors;
    m_jump_table[i_fandnot2] = &dynamic_inst_t::dx_fandnot2;
    m_jump_table[i_fandnot2s] = &dynamic_inst_t::dx_fandnot2s;
    m_jump_table[i_fnot2] = &dynamic_inst_t::dx_fnot2;
    m_jump_table[i_fnot2s] = &dynamic_inst_t::dx_fnot2s;
    m_jump_table[i_fandnot1] = &dynamic_inst_t::dx_fandnot1;
    m_jump_table[i_fandnot1s] = &dynamic_inst_t::dx_fandnot1s;
    m_jump_table[i_fnot1] = &dynamic_inst_t::dx_fnot1;
    m_jump_table[i_fnot1s] = &dynamic_inst_t::dx_fnot1s;
    m_jump_table[i_fxor] = &dynamic_inst_t::dx_fxor;
    m_jump_table[i_fxors] = &dynamic_inst_t::dx_fxors;
    m_jump_table[i_fnand] = &dynamic_inst_t::dx_fnand;
    m_jump_table[i_fnands] = &dynamic_inst_t::dx_fnands;
    m_jump_table[i_fone] = &dynamic_inst_t::dx_fone;
    m_jump_table[i_fones] = &dynamic_inst_t::dx_fones;
    m_jump_table[i_fand] = &dynamic_inst_t::dx_fand;
    m_jump_table[i_fands] = &dynamic_inst_t::dx_fands;
    m_jump_table[i_fnxor] = &dynamic_inst_t::dx_fnxor;
    m_jump_table[i_fnxors] = &dynamic_inst_t::dx_fnxors;
    m_jump_table[i_fsrc1] = &dynamic_inst_t::dx_fsrc1;
    m_jump_table[i_fsrc1s] = &dynamic_inst_t::dx_fsrc1s;
    m_jump_table[i_fornot2] = &dynamic_inst_t::dx_fornot2;
    m_jump_table[i_fornot2s] = &dynamic_inst_t::dx_fornot2s;
    m_jump_table[i_fsrc2] = &dynamic_inst_t::dx_fsrc2;
    m_jump_table[i_fsrc2s] = &dynamic_inst_t::dx_fsrc2s;
    m_jump_table[i_fornot1] = &dynamic_inst_t::dx_fornot1;
    m_jump_table[i_fornot1s] = &dynamic_inst_t::dx_fornot1s;
    m_jump_table[i_for] = &dynamic_inst_t::dx_for;
    m_jump_table[i_fors] = &dynamic_inst_t::dx_fors;
    m_jump_table[i_fcmple16] = &dynamic_inst_t::dx_fcmple16;
    m_jump_table[i_fcmpne16] = &dynamic_inst_t::dx_fcmpne16;
    m_jump_table[i_fcmple32] = &dynamic_inst_t::dx_fcmple32;
    m_jump_table[i_fcmpne32] = &dynamic_inst_t::dx_fcmpne32;
    m_jump_table[i_fcmpgt16] = &dynamic_inst_t::dx_fcmpgt16;
    m_jump_table[i_fcmpeq16] = &dynamic_inst_t::dx_fcmpeq16;
    m_jump_table[i_fcmpgt32] = &dynamic_inst_t::dx_fcmpgt32;
    m_jump_table[i_fcmpeq32] = &dynamic_inst_t::dx_fcmpeq32;
    m_jump_table[i_bshuffle] = &dynamic_inst_t::dx_bshuffle;
	m_jump_table[i_bmask] = &dynamic_inst_t::dx_bmask;
	m_jump_table[i_edge8] = &dynamic_inst_t::dx_edge8;
	m_jump_table[i_edge8n] = &dynamic_inst_t::dx_edge8n;
	m_jump_table[i_edge8l] = &dynamic_inst_t::dx_edge8l;
	m_jump_table[i_edge8ln] = &dynamic_inst_t::dx_edge8ln;
	m_jump_table[i_edge16] = &dynamic_inst_t::dx_edge16;
	m_jump_table[i_edge16n] = &dynamic_inst_t::dx_edge16n;
	m_jump_table[i_edge16l] = &dynamic_inst_t::dx_edge16l;
	m_jump_table[i_edge16ln] = &dynamic_inst_t::dx_edge16ln;
	m_jump_table[i_edge32] = &dynamic_inst_t::dx_edge32;
	m_jump_table[i_edge32n] = &dynamic_inst_t::dx_edge32n;
	m_jump_table[i_edge32l] = &dynamic_inst_t::dx_edge32l;
	m_jump_table[i_edge32ln] = &dynamic_inst_t::dx_edge32ln;
	m_jump_table[i_array8] = &dynamic_inst_t::dx_array8;
	m_jump_table[i_array16] = &dynamic_inst_t::dx_array16;
	m_jump_table[i_array32] = &dynamic_inst_t::dx_array32;
	m_jump_table[i_fpadd16] = &dynamic_inst_t::dx_fpadd16;
	m_jump_table[i_fpadd16s] = &dynamic_inst_t::dx_fpadd16s;
	m_jump_table[i_fpadd32] = &dynamic_inst_t::dx_fpadd32;
	m_jump_table[i_fpadd32s] = &dynamic_inst_t::dx_fpadd32s;
	m_jump_table[i_fpsub16] = &dynamic_inst_t::dx_fpsub16;
	m_jump_table[i_fpsub16s] = &dynamic_inst_t::dx_fpsub16s;
	m_jump_table[i_fpsub32] = &dynamic_inst_t::dx_fpsub32;
	m_jump_table[i_fpsub32s] = &dynamic_inst_t::dx_fpsub32s;
	m_jump_table[i_fmul8x16] = &dynamic_inst_t::dx_fmul8x16;
	m_jump_table[i_fmul8x16au] = &dynamic_inst_t::dx_fmul8x16au;
	m_jump_table[i_fmul8x16al] = &dynamic_inst_t::dx_fmul8x16al;
	m_jump_table[i_fmul8sux16] = &dynamic_inst_t::dx_fmul8sux16;
	m_jump_table[i_fmul8ulx16] = &dynamic_inst_t::dx_fmul8ulx16;
	m_jump_table[i_fmuld8sux16] = &dynamic_inst_t::dx_fmuld8sux16;
	m_jump_table[i_fmuld8ulx16] = &dynamic_inst_t::dx_fmuld8ulx16;
	m_jump_table[i_fpack32] = &dynamic_inst_t::dx_fpack32;
	m_jump_table[i_fpack16] = &dynamic_inst_t::dx_fpack16;
	m_jump_table[i_fpackfix] = &dynamic_inst_t::dx_fpackfix;
	m_jump_table[i_pdist] = &dynamic_inst_t::dx_pdist;
	m_jump_table[i_fpmerge] = &dynamic_inst_t::dx_fpmerge;
	m_jump_table[i_fexpand] = &dynamic_inst_t::dx_fexpand;
	m_jump_table[i_shutdown] = &dynamic_inst_t::dx_shutdown;
	m_jump_table[i_siam] = &dynamic_inst_t::dx_siam;
    m_jump_table[i_ill] = &dynamic_inst_t::dx_ill;
}

void dynamic_inst_t::setReadFaults()
{
	for (int i = 0; i < SI_MAX_SOURCE; i++) {
		reg_id_t &source = getSourceReg(i);
		if (source.getARF()->getFault(source)) {
			source.getARF()->setReadFault(source);
		}
	}
}

void dynamic_inst_t::clearReadFaults()
{
    for (int i = 0; i < SI_MAX_SOURCE; i++) {
        reg_id_t &source = getSourceReg(i);
        source.getARF()->clearReadFault( source );
    }
}

bool dynamic_inst_t::inIdleLoop()
{
/*    return (( m_virtual_address>=0x104868c &&
                m_virtual_address<=0x10486b8 ) ||
            (m_virtual_address>=0x104a070 &&
             m_virtual_address<=0x104a074));
*/


	if((m_virtual_address>=0x1049a7c && m_virtual_address<=0x1049b7c) 
		|| (m_virtual_address>=0x1039cc0 && m_virtual_address<=0x1039cdc) 
		|| (m_virtual_address>=0x1049b84 && m_virtual_address<=0x1049b8c) 
		|| (m_virtual_address>=0x1048748 && m_virtual_address<=0x1048758) 
		|| (m_virtual_address>=0x1039c80 && m_virtual_address<=0x1039c94) 
		|| m_virtual_address == 0x1048788 || m_virtual_address == 0x1048a070 
		|| m_virtual_address == 0x1048794 || m_virtual_address == 0x10486ec 
		|| m_virtual_address == 0x1048708 || m_virtual_address == 0x104810 
		|| m_virtual_address == 0x1048734)
	{
		debugio_t::out_info(" check pc=%llx ",m_virtual_address);
		return true;
	}
	else
	{
		debugio_t::out_info(" check else pc=%llx ",m_virtual_address);
		return false;
	}

}

void dynamic_inst_t::checkDestReadiness()
{
    if (m_fs->getFaultType()==RAT_FAULT ) {
        for (int i=0; i<SI_MAX_DEST; i++) {
            reg_id_t &dest= getDestReg(i);
            if (!dest.isZero()) {
                if (dest.getARF()->isReady( dest )) {
                    //Ready before it is even written!!
                    m_dest_ready[i]=true;
                }
            }
        }
    }
}
#define LXL_DEBUG_MISMATCH_INFO 0

void dynamic_inst_t::insertMismatchInfo(reg_mismatch_info_t &mismatch)
{
	reg_mismatch.push_back(mismatch);
	if (LXL_DEBUG_MISMATCH_INFO) {
		DEBUG_OUT("mismatch reg info %llu\n",seq_num);
		DEBUG_OUT("id %d rt %d lr %d pr %d bv %llx gv %llx\n",
				  mismatch.id,    mismatch.reg_type,
				  mismatch.logical_reg,
				  mismatch.physical_reg,
				  mismatch.bad_value,
				  mismatch.good_value);
		//printDetail();
	}

}

//Deprecated
void dynamic_inst_t::transferMismatchInfo(instr_diag_info_t &i_info)
{
	for (int i=0;i<reg_mismatch.size();i++) {
		i_info.mismatch_regs.push_back(reg_mismatch[i]);
	}
}

// In pseq, after checkchanged state, the mismatch dest regs are saved
// in this instruction. We are inserting into instr_info so that the
// diagnosis module would know.
//
// in instr_info, there are register_info_t, which should have info
// about the reg type and logical/phys reg numbers,
void dynamic_inst_t::transferMismatchInfo(instruction_information_t &i_info)
{
	for (int i=0;i<reg_mismatch.size();i++) {
		int id=reg_mismatch[i].id;

		i_info.dest[id].mismatch=(reg_mismatch[i].good_value!=reg_mismatch[i].bad_value);
		i_info.dest[id].good_value=reg_mismatch[i].good_value;

		if (LXL_DEBUG_MISMATCH_INFO) {
			DEBUG_OUT("dest[%d] rt %d phy %d\n",id,i_info.dest[id].rid.getRtype(),
					  i_info.dest[id].rid.getPhysical());
		
			if (i_info.dest[id].value!=reg_mismatch[i].bad_value) {
				DEBUG_OUT("v %llx bv %llx gv %llx\n",i_info.dest[id].value, 
						  reg_mismatch[i].bad_value,reg_mismatch[i].good_value);
				assert(0);
			}
		}
		assert(i_info.dest[id].value==reg_mismatch[i].bad_value);
	}
	reg_mismatch.clear();
}

void dynamic_inst_t::sanitizeAddr(char *in_addr, int length)
{

	for (int i=0;i<length;i++) {
		if (in_addr[i]!='1' && in_addr[i]!='0' && in_addr[i]!='\0') {
			printf("in address %s\n",in_addr);
			FAULT_STAT_OUT("I see '%c', scared, goodbye!\n",in_addr[i]);
			HALT_SIMULATION;
			break;
		}
	}

}

int dynamic_inst_t::getSourceNum(int arch_reg_no)
{
	for( int i = 0; i < SI_MAX_SOURCE; i++ ) {
		reg_id_t &reg = getSourceReg(i) ;
		if( !reg.isZero() ) 
			if(reg.getVanilla() == arch_reg_no)
				return i;
	}
	return -1;
}

int dynamic_inst_t::getDestNum(int arch_reg_no)
{
	for( int i = 0; i < SI_MAX_DEST; i++ ) {
		reg_id_t &reg = getDestReg(i) ;
		if( !reg.isZero() ) 
			if(reg.getVanilla() == arch_reg_no)
				return i;
	}
	return -1;
}
