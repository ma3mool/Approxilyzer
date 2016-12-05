
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
 * Version:   $Id: controlop.C 1.52 06/02/13 18:49:15-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include "hfa.h"
#include "hfacore.h"
#include "exec.h"
#include "controlop.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// memory allocator for controlop.C
listalloc_t control_inst_t::m_myalloc;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//***************************************************************************
control_inst_t::control_inst_t( static_inst_t *s_inst, 
        int32 window_index,
        pseq_t *pseq,
        abstract_pc_t *fetch_at,
	fault_stats *fault_stat )
: dynamic_inst_t( s_inst, window_index, pseq, fetch_at, fault_stat ) {

    m_isTaken =  false;
    m_actual  = *fetch_at;

    /* Task: update the cwp for saves and restores
     *  Save/Restores (1) read from sources in old windows
     *                (2) write to dest in new windows
     * The source registers already have the cwp epoch '1'. We calculate 
     * the cwp for '2', and update the destination registers here.
     */
    if (s->getFlag( SI_UPDATE_CWP )) {
        uint16 cwp  = fetch_at->cwp;
        uint16 gset = fetch_at->gset;
        if (s->getOpcode() == i_save) {
            cwp++;
            cwp &= (NWINDOWS - 1);
        } else if (s->getOpcode() == i_restore) {
            cwp--;
            cwp &= (NWINDOWS - 1);
        } else {
#ifdef LL_DECODER
			int trap_level = system_t::inst->m_state[m_pseq->getID()]->getControl(CONTROL_TL);
			bool curr_priv = ((fetch_at->pstate & 0x4) == 0x4);
			m_pseq->outSymptomInfo((uint64) m_pseq->getLocalCycle(),
					m_pseq->getRetiredICount(),
					pstate_t::trap_num_menomic((trap_type_t)0x10), 
					0x10,fetch_at->pc,
					curr_priv, 0,
					(uint16)trap_level, m_pseq->getID()) ;
			HALT_SIMULATION ;
#else // LL_DECODER
			SIM_HALT;
#endif //LL_DECODER
        }
        for (int i = 0; i < SI_MAX_DEST; i++) {
            m_dest_reg[i].setVanillaState( cwp, gset );
            m_tofree_reg[i].setVanillaState( cwp, gset );
        }
    }
}

//***************************************************************************
control_inst_t::~control_inst_t() {
}

//**************************************************************************
    void 
control_inst_t::Execute()
{
    STAT_INC( m_pseq->m_stat_control_exec );
    m_event_times[EVENT_TIME_EXECUTE_DONE] = m_pseq->getLocalCycle() - m_fetch_cycle;

	// First off, clear the faults of the destination registers.
	for( int i=0; i< SI_MAX_DEST ; i++ ) { //Changed from MAX_DST to 1
		reg_id_t &dest = getDestReg(i) ;
		if( !dest.isZero() ) { // Inject only if the destination is a valid register
			dest.getARF()->unSetCorrupted(dest) ;
		}
	}
    // call the appropriate function
    static_inst_t *si = getStaticInst();
    if (true) { // What the hell!!!
        // execute the instruction using the jump table
        pmf_dynamicExecute pmf = m_jump_table[si->getOpcode()];

        checkDestReadiness();
        (this->*pmf)();

        propagateFault();
    } else {
        dp_control_t dp_value;
        dp_value.m_at      = &m_actual;
        dp_value.m_taken   = false;
        dp_value.m_annul   = si->getAnnul();
        dp_value.m_offset  = si->getOffset();

        // do the operation
        exec_fn_execute( (i_opcode) si->getOpcode(), (dp_int_t *) &dp_value );

        // write result back to this dynamic instruction
        m_isTaken = dp_value.m_taken;
    }
    SetStage(COMPLETE_STAGE);

    if ((m_pseq->GET_FAULT_TYPE() == BRANCH_TARGET_FAULT) &&
        getControlFault() && m_isTaken) {

        m_actual.pc = m_predicted.pc;
        m_actual.npc = m_predicted.npc;
//         m_actual.cwp = m_predicted.cwp;
//         m_actual.tl = m_predicted.tl;
//         m_actual.pstate = m_predicted.pstate;
    }

    // All control op should be checked, all of them can mis-predict
    // if the predicted PC, nPC pairs don't match with actual, cause exeception
    if ( (m_predicted.pc     != m_actual.pc ||
                m_predicted.npc    != m_actual.npc ||
                m_predicted.cwp    != m_actual.cwp ||
                m_predicted.tl     != m_actual.tl ||
                m_predicted.pstate != m_actual.pstate) ) {

        /* There was a branch mis-prediction --
         *    patch up branch predictor state */

        // This preformatted debugging information is left for your convenience
        /*
           if (m_predicted.cwp != m_actual.cwp) {
           m_pseq->out_info("CONTROLOP: CWP mispredict: %d %d type=%d\n",
           m_predicted.cwp, m_actual.cwp, s->getBranchType());
           }
           if (m_predicted.tl != m_actual.tl) {
           m_pseq->out_info("CONTROLOP: TL mispredict: %d %d type=%d\n",
           m_predicted.tl, m_actual.tl, s->getBranchType());
           }
           if (m_predicted.pstate != m_actual.pstate) {
           m_pseq->out_info("CONTROLOP: PSTATE mispredict: 0x%0x 0x%0x type=%d\n",
           m_predicted.pstate, m_actual.pstate, s->getBranchType());
           }
         */

        markEvent(EVENT_BRANCH_MISPREDICT);
        if (s->getBranchType()      == BRANCH_COND ||
                s->getBranchType()      == BRANCH_PCOND ) {
            // flip the last bit of the history to correct the misprediction
            m_pred_state.cond_state = m_pred_state.cond_state ^ 1;

        } else if (s->getBranchType() == BRANCH_INDIRECT ||
                (s->getBranchType() == BRANCH_CALL && s->getOpcode() != i_call) ) {
            /*
               m_pseq->out_info(" INDIRECT:  predicted 0x%0llx, 0x%0llx\n",
               m_predicted.pc, m_predicted.npc);
               m_pseq->out_info(" INDIRECT:  actual    0x%0llx, 0x%0llx\n",
               m_actual.pc, m_actual.npc);
             */
            m_pseq->getIndirectBP()->FixupState(&(m_pred_state.indirect_state), 
                    getVPC());
        }
        // no predictor fixup on PRIV, TRAP or TRAP_RETURN right now

        // TASK: should update the BTB here

        // TASK: determine the fetch location (and state)

        m_actual.gset = pstate_t::getGlobalSet( m_actual.pstate );

        /* cause a branch misprediction exception */
        m_pseq->raiseException(EXCEPT_MISPREDICT, getWindowIndex(),
                (enum i_opcode) s->getOpcode(),
                &m_actual, 0, BRANCHPRED_MISPRED_PENALTY );
    }
}

//**************************************************************************
void 
control_inst_t::Squash() { 
    ASSERT( !getEvent( EVENT_FINALIZED ) );
    ASSERT(m_stage != RETIRE_STAGE);
    if (Waiting()) {
        RemoveWaitQueue();
    }
    UnwindRegisters( );
    m_pseq->decrementSequenceNumber();

    markEvent( EVENT_FINALIZED );
#ifdef PIPELINE_VIS
    m_pseq->out_log("squash %d\n", getWindowIndex());
#endif
}

//**************************************************************************
    void
control_inst_t::Retire( abstract_pc_t *a )
{
    ASSERT( !getEvent( EVENT_FINALIZED ) );
    STAT_INC( m_pseq->m_stat_control_retired );
    STAT_INC( m_pseq->m_branch_seen_stat[s->getBranchType()][2] );
    STAT_INC( m_pseq->m_branch_seen_stat[s->getBranchType()][m_priv? 1:0] );

    // record when execution takes place
    m_event_times[EVENT_TIME_RETIRE] = m_pseq->getLocalCycle() - m_fetch_cycle;

    // update dynamic branch prediction (predicated) instruction statistics
    static_inst_t *s_instr = getStaticInst();
    if (s_instr->getType() == DYN_CONTROL) {
        uint32 inst;
        int    op2;
        int    pred;
        switch (s_instr->getBranchType()) {
            case BRANCH_COND:
                // conditional branch
                STAT_INC( m_pseq->m_nonpred_retire_count_stat );
                break;

            case BRANCH_PCOND:
                // predictated conditional
                inst = s_instr->getInst();
                op2  = maskBits32( inst, 22, 24 );
                pred = maskBits32( inst, 19, 19 );
                if ( op2 == 3 ) {
                    // integer register w/ predication
                    STAT_INC( m_pseq->m_pred_reg_retire_count_stat );
                    if (pred) {
                        STAT_INC( m_pseq->m_pred_reg_retire_taken_stat );
                    } else {
                        STAT_INC( m_pseq->m_pred_reg_retire_nottaken_stat );
                    }
                } else {
                    STAT_INC( m_pseq->m_pred_retire_count_stat );
                    if (pred) {
                        STAT_INC( m_pseq->m_pred_retire_count_taken_stat );
                    } else {
                        STAT_INC( m_pseq->m_pred_retire_count_nottaken_stat );
                    }
                }
                if (pred == true && m_isTaken == false ||
                        pred == false && m_isTaken == true) {
                    STAT_INC( m_pseq->m_branch_wrong_static_stat );
                }
                break;

            default:
                ;      // ignore non-predictated branches
        }
    }

    //LXL: debugpriv
    //     if (0&&getPrivilegeMode() && getFault())
    //  	printDetail();
 	
#ifdef DEBUG_RETIRE
    m_pseq->out_info("## Control Retire Stage\n");
    printDetail();
    m_pseq->printPC( &m_actual );
#endif

    bool mispredict = (m_events & EVENT_BRANCH_MISPREDICT);
    if (mispredict) {
        // incorrect branch prediction
        STAT_INC( m_pseq->m_branch_wrong_stat[s->getBranchType()][2] );
        STAT_INC( m_pseq->m_branch_wrong_stat[s->getBranchType()][m_priv? 1:0] );

        // train ras's exception table
        if (ras_t::EXCEPTION_TABLE && s->getBranchType() == BRANCH_RETURN) {
            my_addr_t tos;
            ras_t *ras = m_pseq->getRAS();
            ras->getTop(&(m_pred_state.ras_state), &tos);
            if(tos == m_actual.npc) {
                ras->markException(m_predicted.npc);
                // update RAS state
                ras->pop(&(m_pred_state.ras_state));
                m_pseq->setSpecBPS(m_pred_state);
                if (ras_t::DEBUG_RAS) m_pseq->out_info("*********** DEBUG_RAS ***********\n");
            } else {
                ras->unmarkException(m_actual.npc);
            }
        }
    } else {
        // correct or no prediction
        STAT_INC( m_pseq->m_branch_right_stat[s->getBranchType()][2] );
        STAT_INC( m_pseq->m_branch_right_stat[s->getBranchType()][m_priv? 1:0] );
    }

    /* update branch predictor tables at retirement */
    if (s->getBranchType() == BRANCH_COND ||
            s->getBranchType() == BRANCH_PCOND) {
        m_pseq->getDirectBP()->Update(getVPC(),
                (m_pseq->getArchBPS()->cond_state),
                s->getFlag( SI_STATICPRED ),
                m_isTaken);

    } else if (s->getBranchType() == BRANCH_INDIRECT ||
            (s->getBranchType() == BRANCH_CALL && s->getOpcode() != i_call) ) {
        // m_actual.npc is the indirect target
        m_pseq->getIndirectBP()->Update( getVPC(),
                &(m_pseq->getArchBPS()->indirect_state),
                m_actual.npc );
    }
    m_pseq->setArchBPS(m_pred_state);

    // no need to update call&return, since we used checkpointed RAS
    // no update on traps right now
    SetStage(RETIRE_STAGE);

    // Pradeep - Now, take stat of this branch retiring
    m_pseq->INC_BRANCH();
#ifdef ALEX_FAULT_RET_OUT
    FAULT_RET_OUT( "Branch\t") ;
//     FAULT_RET_OUT( "[%u]\t", m_pseq->getLocalCycle() ) ;
//     FAULT_RET_OUT( "[%u]\t", getSequenceNumber() ) ;
    FAULT_RET_OUT( "pc:0x%0llx", getVPC() ) ;
    FAULT_RET_OUT( "(%d)\t", getPrivilegeMode() ) ;
    FAULT_RET_OUT( "target:0x%0llx\n", m_actual.npc ) ;
#endif

	// if(getSequenceNumber()==666769) {
	// 	DEBUG_OUT("Pre retireRegisters() %d. Has fault = %d\n",
	// 			getSequenceNumber(), isCorrupted()) ;
	// }

    /* retire any register overwritten by link register */
    retireRegisters(); 

	// if(getSequenceNumber()==666769) {
	// 	DEBUG_OUT("post retireRegisters() %d. Has fault = %d\n",
	// 			getSequenceNumber(), isCorrupted()) ;
	// }
    /* return pc, npc pair which are the results of execution */
    ASSERT( m_actual.pc != (my_addr_t) -1 );
    a->pc  = m_actual.pc;
    a->npc = m_actual.npc;
    a->has_fault = this->getFault() || getControlFault();

//     if( this->getFault() ) {
// 		if(LXL_DEBUG) {
// 			DEBUG_OUT( "%d Control inst faulty\n", getSequenceNumber() ) ;
// 			printDetail();
// 		}
//         m_pseq->ARCH_STATE() ;
//         m_pseq->MEM_STATE() ;
// #if 0
//     FAULT_RET_OUT( "Branch\t") ;
//     FAULT_RET_OUT( "[%u]\t", m_pseq->getLocalCycle() ) ;
//     FAULT_RET_OUT( "[%u]\t", getSequenceNumber() ) ;
//     FAULT_RET_OUT( "pc:0x%0llx", getVPC() ) ;
//     FAULT_RET_OUT( "(%d)\t", getPrivilegeMode() ) ;
//     FAULT_RET_OUT( "target:0x%0llx\n", m_actual.npc ) ;
// #endif
//     }

    markEvent( EVENT_FINALIZED );
	// Record the type of instruction as faulty
	if(RECORD_FAULTY_INST) {
		if(isCorrupted()) {
			m_fs->addFaultyInstruction(decode_opcode((enum i_opcode)s->getOpcode())) ;
		}
	}
#ifdef DETECT_INF_LOOP
    if (!addressMasked()) {
        m_pseq->isLoop(this);
    }
#endif

    if(MULTICORE_DIAGNOSIS_ONLY) {

	    // collect control op info, NOTE: also check ta instructions
	    m_pseq->getMulticoreDiagnosis()->collectCompareLog(getVPC(), (uint64)m_actual.npc, m_pseq->getID());
	    m_pseq->getMulticoreDiagnosis()->reportSequenceNumber(m_pseq->getID());

	    if(0) {
		    DEBUG_OUT( "Branch ") ;
		    DEBUG_OUT( "pc:0x%0llx ", getVPC() ) ;
		    DEBUG_OUT( "(%d) ", getPrivilegeMode() ) ;
		    DEBUG_OUT( "target:0x%0llx\n", m_actual.npc ) ;
	    }
    }

	// DEBUG_OUT("0x%llx\t%s%s\t0x%llx\n",
	// 		getVPC(), 
	// 		decode_opcode((enum i_opcode)s->getOpcode()),
	// 		(isCorrupted())?"^":" ",
	// 		getPSEQ()->getSimicsNPC()) ;
	// DEBUG_OUT("Ctrl: ") ;
	// printRetireTrace() ;
	m_fs->addInstTargets(getVPC(), getPSEQ()->getSimicsNPC()) ;
#ifdef CTRL_TARGET
	if(getPrivilegeMode()) {
		DEBUG_OUT("0x%llx\n", getPSEQ()->getSimicsNPC()) ;
	}
#endif // CTRL_TARGET

#ifdef HARD_BOUND
	propagateObjects() ;
#endif
	if(INST_MIX) {
		m_fs->recordInstMix("ctrl",isCorrupted()) ;
	}
}

//***************************************************************************
    void
control_inst_t::printDetail( void )
{
    dynamic_inst_t::printDetail();

    m_pseq->out_info( "control_inst_t\n");
    m_pseq->out_info( "type    : %s\n",
            pstate_t::branch_type_menomic[s->getBranchType()] );
    m_pseq->out_info("offset  : 0x%0llx\n", s->getOffset());
    m_pseq->out_info("isTaken : %d\n", m_isTaken);
    m_pseq->out_info("Predicted Target :\n");
    m_pseq->printPC( &m_predicted );
    m_pseq->out_info("Actual Target :\n");
    m_pseq->printPC( &m_actual );
}

