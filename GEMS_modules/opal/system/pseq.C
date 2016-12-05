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
 * FileName:  pseq.C
 * Synopsis:  Implements an out-of-order sequencer
 * Author:    cmauer
 * Version:   $Id: pseq.C 1.140 06/02/13 18:49:20-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"

// branch prediction schemes
#include "gshare.h"
#include "agree.h"
#include "yags.h"
#include "igshare.h"
#include "mlpredict.h"

#include "utimer.h"
#include "lsq.h"
#include "ipagemap.h"
#include "tracefile.h"
#include "branchfile.h"
#include "memtrace.h"
#include "fileio.h"
#include "confio.h"
#include "debugio.h"
#include "checkresult.h"
#include "pipestate.h"
#include "rubycache.h"
#include "mf_api.h"
#include "amber_api.h"
#include "histogram.h"
#include "stopwatch.h"
#include "sysstat.h"
#include "dtlb.h"

#include "flow.h"
#include "actor.h"
#include "flatarf.h"
#include "dependence.h"
#include "ptrace.h"
#include "pseq.h"

// The Simics API Wrappers
#include "interface.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define ALEX_SNET_DEBUG 0
#define ALEX_MMU_DEBUG 0
#define PRINT_INSTR_AFTER_RECOVERY 1
#define VERBOSE_IN_CHECK 0

#define SRC_REG 0
#define DEST_REG 1

#define ROLLBACK_AT_SYMPTOM 0
// #define ROLLBACK_AT_SYMPTOM 1
#define HALT_AT_ROLLBACK 1

// AM64:  AddressMask 64-bits. This corresponds to the AM bit in the PSTATE
//        register word, that makes all PC accesses only use the lower 32-bits.
#define AM64( address )       \
    ( (int32) address & 0xffffffff )

// ISADDRESS32:  Strips the AM bit out of the PSTATE register word.
#define ISADDRESS32( PSTATE ) \
    ( ((PSTATE) & 0x8) == 0x8 )

// defines the OS page size, used for performance reasons in the ITLB mini-
// cache (8K pages in solaris == 13 bits of mask)
const la_t PSEQ_OS_PAGE_MASK = 0xffffffffffffe000ULL;

// M_PSTATE:  pstate used to be a member variable, now it is system-wide.
#define M_PSTATE \
    system_t::inst->m_state[m_id]

/// size of register use histogram fields
const uint32  PSEQ_REG_USE_HIST       = 200;

/// size of recently retired instruction buffer
///   This is like the "flight data recorder" for the simulation.
const int32   PSEQ_RECENT_RETIRE_SIZE = 40;

/// size of instructions returned for more decoding
const int32   PSEQ_HIST_DECODE        = 30;

const char *cheetah_register_names[] = {
    "ctxt_nucleus", 
    "ctxt_primary", 
    "ctxt_secondary", 
    "dcu_ctrl",
    "dsfar",
    "dsfsr",
    "dtag_access",
    "dtag_access_ex", 
    "dtag_target",
    "dtlb_2w_daccess",
    "dtlb_2w_tagread", 
    "dtlb_fa_daccess", 
    "dtlb_fa_tagread", 
    "dtsb",
    "dtsb_nx",
    "dtsb_px",
    "dtsb_sx", "dtsbp64k", "dtsbp8k", "dtsbpd", "ec_control", "isfsr",
    "itag_access", "itag_target", "itlb_2w_daccess", "itlb_2w_tagread", "itlb_fa_daccess",
    "itlb_fa_tagread", "itsb", "itsb_nx", "itsb_px", "itsbp64k", "itsbp8k",
    "last_etag_write", "lfsr", "lsu_ctrl", "madr_1", "madr_2", "madr_3", "madr_4",
    "mem_address_control", "mem_tmg_1", "mem_tmg_2", "mem_tmg_3", "mem_tmg_4", "mem_tmg_5",
    "pa_watchpoint", "trace", "va_watchpoint"
};

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

// handles mmu context switches
static void pseq_mmu_reg_handler( void *pseq_obj, void *ptr,
        uint64 regindex, uint64 newcontext );

#ifdef BUILD_SLICE
// pseq_t::producer_t pseq_t::last_producer ;
pseq_t::last_reg_fault_t pseq_t::last_reg_fault ;
pseq_t::last_addr_fault_t pseq_t::last_addr_fault ;
uint64 pseq_t::pseq_fault_id ;
#ifdef BUILD_DDG
pseq_t::producer_t pseq_t::ddg_last_producer ;
pseq_t::last_store_t pseq_t::ddg_last_store ;
#endif // BUILD_DDG
#endif // BUILD_SLICE

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
pseq_t::pseq_t( int32 id, multicore_diagnosis_t * m_diag ):out_intf_t( id ), m_iwin(IWINDOW_ROB_SIZE, IWINDOW_WIN_SIZE) {

    m_lsq                 = NULL;
    m_bpred               = NULL;
    m_ipred               = NULL;
    m_spec_bpred          = NULL;
    m_arch_bpred          = NULL;
    m_ras                 = NULL;
    m_tlstack             = NULL;
    m_conf                = NULL;
    m_imap                = NULL;
    m_tracefp             = NULL;
    m_branch_trace        = NULL;
    m_memtracefp          = NULL;
    m_control_rf          = NULL;
    m_control_arf         = NULL;
    l2_mshr               = NULL;
    l2_cache              = NULL;
    il1_mshr              = NULL;
    dl1_mshr              = NULL;
    l1_inst_cache         = NULL;
    l1_data_cache         = NULL;
    m_ruby_cache          = NULL;

    print_trace = 0;
    /*
     * core initialization
     */
    m_id     = id;
    m_local_cycles = 0;
    m_local_inorder_number = 0;
    m_local_sequence_number = 0;
    m_local_icount = 0;
    m_lsq    = new lsq_t( this );

    m_multicore_diagnosis = m_diag;
    llb = new llb_t();

    current = 0;
    previous = 0;
    previous_os = 0;
    previous_op = 0;
    previous_trap_type = Trap_NoTrap;
    previous_trap_level = 0;
    previous_pc = 0;
    cpu_os = 0;
    cpu_app = 0;
    cpu_os_idle = 0;
    dont_count = false;
    track = false;
    squash_inst_no = -1;
    squash_next_instr = false;
    done_squash_trans = false;
    current_inst_num = 0;
    trans_fault_injected = false;


    fault_stat = new fault_stats(m_id);
    m_iwin.setFaultStat(fault_stat);

#ifdef LLB_ANALYSIS
    num_values = 0;
#endif

#ifdef SESE_POT
    m_start_logging = false;
#endif

    m_report_interrupt_to_diagnosis = false;

    branch_predictor_type_t  predictor_type;
    if ( !strcmp( BRANCHPRED_TYPE, "GSHARE" ) ) {
        predictor_type = BRANCHPRED_GSHARE;
    } else if ( !strcmp( BRANCHPRED_TYPE, "AGREE" ) ) {
        predictor_type = BRANCHPRED_AGREE;
    } else if ( !strcmp( BRANCHPRED_TYPE, "YAGS" ) ) {
        predictor_type = BRANCHPRED_YAGS;
    } else if ( !strcmp( BRANCHPRED_TYPE, "IGSHARE" ) ) {
        predictor_type = BRANCHPRED_IGSHARE;
    } else if ( !strcmp( BRANCHPRED_TYPE, "MLPREDICT" ) ) {
        predictor_type = BRANCHPRED_MLPRED;
    } else if ( !strcmp( BRANCHPRED_TYPE, "EXTREME" ) ) {
        predictor_type = BRANCHPRED_EXTREME;
    } else {
        DEBUG_OUT("Unknown branch predictor type: %s\n", BRANCHPRED_TYPE );
        DEBUG_OUT("   Defaulting to YAGS branch predictor\n");
        predictor_type = BRANCHPRED_YAGS;
    }

    switch (predictor_type) {
        case BRANCHPRED_GSHARE:
            m_bpred = new gshare_t( BRANCHPRED_PHT_BITS );
            break;
        case BRANCHPRED_AGREE:
            m_bpred = new agree_t( BRANCHPRED_PHT_BITS );
            break;
        case BRANCHPRED_YAGS:
            m_bpred = new yags_t( BRANCHPRED_PHT_BITS, BRANCHPRED_EXCEPTION_BITS,
                    BRANCHPRED_TAG_BITS );
            break;
        case BRANCHPRED_IGSHARE:
            m_bpred = new igshare_t( BRANCHPRED_PHT_BITS, BRANCHPRED_USE_GLOBAL );
            break;
        case BRANCHPRED_MLPRED:
            m_bpred = new mlpredict_t( BRANCHPRED_PHT_BITS, 32, 32 );
            ((mlpredict_t *) m_bpred)->readTags( NULL );
            break;
        case BRANCHPRED_EXTREME:
            SIM_HALT;
            break;
        default:
            ERROR_OUT("Unknown branch predictor type: %d\n", BRANCHPRED_TYPE);
            SIM_HALT;
    }
    m_ipred = new cascaded_indirect_t();
    m_spec_bpred = new predictor_state_t();
    m_arch_bpred = new predictor_state_t();
    memset( (void *) m_spec_bpred, 0, sizeof(predictor_state_t) );
    memset( (void *) m_arch_bpred, 0, sizeof(predictor_state_t) );
    m_ras = new ras_t( this, (1 << RAS_BITS), RAS_EXCEPTION_TABLE_BITS );
    m_tlstack = new tlstack_t( (MAXTL + 2) );

    // initialize branch predictor's architected and speculative state
    m_bpred->InitializeState( &m_arch_bpred->cond_state );
    m_ipred->InitializeState( &m_arch_bpred->indirect_state );
    m_ras->InitializeState( &m_arch_bpred->ras_state );

    m_bpred->InitializeState( &m_spec_bpred->cond_state );
    m_ipred->InitializeState( &m_spec_bpred->indirect_state );
    m_ras->InitializeState( &m_spec_bpred->ras_state );

    ASSERT( m_spec_bpred->ras_state.TOS      == 0 );
    ASSERT( m_spec_bpred->ras_state.next_free == 1 );

    m_conf  = new confio_t();
    m_imap  = new ipagemap_t(PSEQ_IPAGE_TABLESIZE);

    // initialize scheduler
    m_scheduler = new scheduler_t( this );

    /*
     * register file initialization
     */
    allocateRegBox( m_ooo );
    /*
     * cache initialization
     */
    if (!CONFIG_WITH_RUBY) {
        /* Second level mshr, backed up by main memory */
        l2_mshr = new mshr_t("L2.mshr", /* memory */ NULL, m_scheduler,
                L2_BLOCK_BITS, MEMORY_DRAM_LATENCY, L2_FILL_BUS_CYCLES,
                L2_MSHR_ENTRIES, L2_STREAM_BUFFERS);

        /* Second cache backs up first level MSHR */
        l2_cache = new generic_cache_template<generic_cache_block_t>(
                "L2.unified", l2_mshr, m_scheduler, L2_SET_BITS, L2_ASSOC,
                L2_BLOCK_BITS, (L2_IDEAL != 0));

        /* First level mshr, backed up by L2 cache */
        il1_mshr = new mshr_t("IL1.mshr", l2_cache, m_scheduler,
                IL1_BLOCK_BITS, L2_LATENCY, L1_FILL_BUS_CYCLES,
                IL1_MSHR_ENTRIES, IL1_STREAM_BUFFERS);

        dl1_mshr = new mshr_t("DL1.mshr", l2_cache, m_scheduler,
                DL1_BLOCK_BITS, L2_LATENCY, L1_FILL_BUS_CYCLES,
                DL1_MSHR_ENTRIES, DL1_STREAM_BUFFERS);

        /* first level instruction caches */  
        l1_inst_cache = new generic_cache_template<generic_cache_block_t>(
                "L1.inst", il1_mshr, m_scheduler, IL1_SET_BITS,
                IL1_ASSOC, IL1_BLOCK_BITS, (IL1_IDEAL != 0) );

        /* first level data caches */  
        l1_data_cache = new generic_cache_template<generic_cache_block_t>(
                "L1.data", dl1_mshr, m_scheduler, DL1_SET_BITS,
                DL1_ASSOC, DL1_BLOCK_BITS, (DL1_IDEAL != 0) );

        m_standalone_tlb = new dtlb_t( TLB_NUM_ENTRIES, TLB_NUM_PAGE_SIZES,
            TLB_PAGE_SIZES );
    } else {
        /* CONFIG_WITH_RUBY  */
        m_ruby_cache = new rubycache_t( m_id, L2_BLOCK_BITS, m_scheduler );
    }

    // set MMU related fields
    m_primary_ctx = 0;
    m_shadow_pstate = 0;
    m_mmu_access    = NULL;
    m_mmu_object    = NULL;
    m_mmu_asi       = NULL;
    if (CONFIG_IN_SIMICS) {
        char mmuname[200];
        pstate_t::getMMUName( m_id, mmuname, 200 );
        conf_object_t *mmu = SIMICS_get_object( mmuname );
        if (mmu == NULL) {
            ERROR_OUT("error: unable to locate object: %s\n", mmuname);
            SIM_HALT;
        }
        m_mmu_access = (mmu_interface_t *) SIMICS_get_interface( mmu, "mmu" );
		m_mmu_object = mmu;
		//DEBUG_OUT("Init: %s: %d\n", mmuname, m_mmu_object);
        if (m_mmu_access == NULL) {
            ERROR_OUT("error: object does not implement mmu interface: %s\n",
                    mmuname);
            SIM_HALT;
        }
    }

	// set fetch related fields
	m_fetch_status = PSEQ_FETCH_READY;
	m_fetch_ready_cycle = 0;
	m_next_line_address = 0;
	m_line_buffer_fetch_stall = false;
	m_fetch_requested_line = 0;
	m_wait_for_request = false;
	m_last_fetch_physical_address = 0;
	m_last_line_buffer_address = 0;
	m_unchecked_retires = 0;
	m_unchecked_retire_top = 0;
	m_unchecked_instr = (dynamic_inst_t **) malloc( (PSEQ_MAX_UNCHECKED + 1)*
			sizeof(dynamic_inst_t *) );
	m_recent_retire_index = 0;
	m_recent_retire_instr = (dynamic_inst_t **) malloc(PSEQ_RECENT_RETIRE_SIZE*
			sizeof(dynamic_inst_t *) );
	for ( int32 i = 0; i < PSEQ_RECENT_RETIRE_SIZE; i++ ) {
		m_recent_retire_instr[i] = NULL;
	}
	m_fetch_itlbmiss = new static_inst_t( ~(my_addr_t)0, STATIC_INSTR_MOP );
	// itlbmiss is not free()'d
	m_fetch_itlbmiss->incrementRefCount();

	// init the I$ pipeline
	m_i_cache_line_queue.setSize( min_power_of_two(ICACHE_CYCLE) );
	m_i_cache_line_queue.init( PSEQ_INVALID_LINE_ADDRESS );
	// init the fetch pipeline
	m_fetch_per_cycle.setSize( min_power_of_two(FETCH_STAGES) );
	m_fetch_per_cycle.init( 0 );
	// init the decode pipeline
	m_decode_per_cycle.setSize( min_power_of_two(DECODE_STAGES) );
	m_decode_per_cycle.init( 0 );

	m_act_schedule = new act_schedule_t( this, 64 );

	// initialize retire related members
	m_fwd_progress_cycle = 2*PSEQ_MAX_FWD_PROGRESS_TIME;
	m_posted_interrupt   = Trap_NoTrap;
	clearException();

	// check that for each ALU type, there exists at least one resource
	for (uint32 i = 0; i < FU_NUM_FU_TYPES; i++) {
		if (CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[i]] == 0) {
			DEBUG_OUT("error: ALU resource %s has no functional units allocated\n",
					pstate_t::fu_type_menomic( (fu_type_t) i ));
			ERROR_OUT("error: check CONFIG_ALU_MAPPING for map errors. (zero functional units).\n");
			SIM_HALT;
		}
	}

	//
	// Statistics
	// 
	initializeStats();
#ifdef RENAME_EXPERIMENT
	for (uint32 i = 0; i < RID_NUM_RID_TYPES; i++ ) {

		uint32 logical;
		switch (i) {
			case RID_INT:
				logical = CONFIG_IREG_LOGICAL;
				break;

			case RID_SINGLE:
				logical = CONFIG_FPREG_LOGICAL;
				break;

			case RID_CC:
				logical = CONFIG_CCREG_LOGICAL;
				break;

			default:
				// we don't track rename statistics on all other registers
				continue;
		}

		m_reg_use[i] = (uint32 **) malloc( logical * sizeof(uint32 *) );
		for (uint32 j = 0; j < logical; j++) {
			m_reg_use[i][j] = (uint32 *) malloc( PSEQ_REG_USE_HIST*sizeof(uint32) );
			for (uint32 k = 0; k < PSEQ_REG_USE_HIST; k++) {
				m_reg_use[i][j][k] = 0;
			}
		}
	}
#endif

#ifdef DETECT_INF_LOOP
	for (uint32 j=0; j<LOOP_HASH_SIZE; j++) {
		for (uint32 k=0; k<(4+SI_MAX_SOURCE);k++)
			loop_info[j][k]=0;
	}
#endif

#ifdef MEASURE_FP
	for (int i=0; i<3; i++) {
		for (int j=0; i<4; i++) {
			FP_util[i][j]=0;
		}
	}
#endif

#ifdef SW_ROLLBACK
	m_op_mode = NORMAL;
#endif

	curr_trap_rate = 0 ;
	m_mem_trace.clear();
	recovered=false;
	detected_again = false;
	mem_trace_cnt=0;
	m_trigger_recovery=false;
	for(int i=0;i<NUM_CHKPTS;i++) {
		tlb_data[i]=new tlb_data_t;
	}

	m_last_symptom=Trap_NoTrap;

#ifdef LXL_SNET_RECOVERY
	if(!MULTICORE_DIAGNOSIS_ONLY)
		m_diagnosis=new diagnosis_t;
	else
		m_diagnosis = NULL;
#else
	m_diagnosis=NULL;
#endif

	reg_box_t &rbox=m_ooo.regs;
#ifdef SW_ROLLBACK
	initializeRecoveryInstrMem();
#endif
	last_rollback_time=0;
	time_from_rollback=0;
	steady_state=0;

	hw_hang_cnt=0;

	ASSERT( m_spec_bpred->ras_state.TOS      == 0 );
	ASSERT( m_spec_bpred->ras_state.next_free == 1 );

	ASSERT( m_arch_bpred->ras_state.TOS      == 0 );
	ASSERT( m_arch_bpred->ras_state.next_free == 1 );

	// set fields to indicate if this core is in interrupt
	in_interrupt = false ;
	interrupt_priv = -1 ;

#ifdef RESOURCE_RR
	m_last_result_bus_id = -1 ;
#endif 

	symptom_seen = false ;
#ifdef BUILD_SLICE
	fault_list.clear() ;
	last_reg_fault.clear() ;
	last_addr_fault.clear() ;
	deleted_faults.clear() ;
	pseq_fault_id = 0 ;
	reg_list.clear() ;
	addr_list.clear() ;
	unread_regs.clear() ;
	unread_addr.clear() ;
	all_reg_faults = 0 ;
	deleted_instructions.clear() ;
	// last_producer.clear() ;
#ifdef BUILD_DDG
	ddg_last_producer.clear() ;
	ddg_last_store.clear() ;
	ddg_root = new ddg_node_t(0,0,0) ;
	ddg_node_map.clear() ;
	ddg_node_map[0] = ddg_root ;
	ddg_stats_map.clear() ;
#endif BUILD_DDG
#endif // BUILD_SLICE
}

//**************************************************************************
	pseq_t::~pseq_t() {
		if (m_lsq)
			delete m_lsq;
		if (m_bpred)
			delete m_bpred;
		if (m_ipred)
			delete m_ipred;
		if (m_spec_bpred)
			delete m_spec_bpred;
		if (m_arch_bpred)
			delete m_arch_bpred;
		if (m_ras)
			delete m_ras;

		if (m_control_rf) {
			for ( uint32 i = 0; i < CONFIG_NUM_CONTROL_SETS; i++ ) {
				delete m_control_rf[i];
			}
			free( m_control_rf );
		}
		if (m_control_arf)
			delete m_control_arf;

		// free the cache!
		if (l1_inst_cache)
			delete l1_inst_cache;
		if (l1_data_cache)
			delete l1_data_cache;
		if (l2_cache)
			delete l2_cache;
		if (il1_mshr)
			delete il1_mshr;
		if (dl1_mshr)
			delete dl1_mshr;
		if (l2_mshr)
			delete l2_mshr;

		if (m_imap)
			delete m_imap;
		if (m_conf)
			delete m_conf;
		if (m_tracefp)
			delete m_tracefp;
		if (m_memtracefp)
			delete m_memtracefp;

		// Free statistics
		if (m_overall_timer)
			delete m_overall_timer;
		if (m_thread_timer)
			delete m_thread_timer;
		if (m_thread_histogram)
			delete m_thread_histogram;

#ifdef RENAME_EXPERIMENT
		for (uint32 i = 0; i < RID_NUM_RID_TYPES; i++ ) {
			uint32 logical;
			switch (i) {
				case RID_INT:
					logical = CONFIG_IREG_LOGICAL;
					break;

				case RID_SINGLE:
					logical = CONFIG_FPREG_LOGICAL;
					break;

				case RID_CC:
					logical = CONFIG_CCREG_LOGICAL;
					break;

				default:
					// we don't track rename statistics on all other registers
					continue;
			}

			for (uint32 j = 0; j < logical; j++) {
				if (m_reg_use[i][j])
					free (m_reg_use[i][j]);
			}
			if (m_reg_use[i] != NULL)
				free( m_reg_use[i] );
		}
#endif
		if (m_hist_fetch_per_cycle)
			free(m_hist_fetch_per_cycle);
		if (m_hist_decode_per_cycle)
			free(m_hist_decode_per_cycle);
		if (m_hist_schedule_per_cycle)
			free(m_hist_schedule_per_cycle);
		if (m_hist_retire_per_cycle)
			free(m_hist_retire_per_cycle);
		if (m_hist_fetch_stalls)
			free(m_hist_fetch_stalls);
		if (m_hist_decode_return)
			free(m_hist_decode_return);
		if (m_hist_retire_stalls)
			free(m_hist_retire_stalls);
		if (m_hist_ff_length)
			free(m_hist_ff_length);
		if (m_hist_ideal_coverage)
			free(m_hist_ideal_coverage);
		if (m_stat_fu_utilization)
			free(m_stat_fu_utilization);
		if (m_stat_fu_util_retired)
			free(m_stat_fu_util_retired);
	}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/** advances one clock cycle for this processor.
 *
 *  Includes fetching, decoding, scheduling, executing, and retiring
 *  instructions. Each of these sub-tasks is broken out as private
 *  methods of this class.
 *
 */
//**************************************************************************
void pseq_t::advanceCycle( void )
{
	// If Opal has been made into a perfect processor
	// Dumbly step simics without simulation
	if(OPAL_IS_NULL) {
		assert( OPAL_NULL_IPC >= 1);
		m_unchecked_retires = OPAL_NULL_IPC;
		advanceSimics();
		m_stat_committed+=OPAL_NULL_IPC;
		localCycleIncrement();
		return;
	}

#ifdef PIPELINE_VIS
	out_log( "cycle %u\n", (uint32) getLocalCycle());
	ireg_t pstate    = m_control_arf->getRetireRF()->getInt( CONTROL_PSTATE );
	if ( m_shadow_pstate != (uint16) pstate ) {
		out_log( "set pstate 0x%0llx\n", pstate );
		m_shadow_pstate = pstate;
	}
	printPC( m_fetch_at );
#endif

	FAULT_CLOCK( getLocalCycle() ) ;

	if(DEBUG_PRINT_CYCLE) {
		out_info("*********************************************\n") ;
		out_info("-------------- cycle %d ------------\n", getLocalCycle()) ;
		out_info("*********************************************\n") ;
	}

	// New cycle-based injection method 
	// if ( GET_FAULT_STUCKAT()==4 &&
	//         getLocalCycle() == fault_stat->getFaultInjCycle() &&
	//         !(fault_stat->getTransInjected()) ) {
	//     injectTransientFault();
	// }
#ifndef ARCH_TRANS_FAULT
	injectTransientFault();
#endif // ARCH_TRANS_FAULT

	// Reset way counter
	resetWayCounter();

	// I. Fetch instructions
	fetchInstruction();

	// II. Decode instructions in the pipe
	decodeInstruction();

	// III. Schedule decoded instructions
	scheduleInstruction();

	// (Executing scheduled instructions on functional units takes place
	//  implicitly after their dependencies become resolved)
	if (ALEX_ACCURATE_DEBUG) out_info("---- cycle %d ----\n",getLocalCycle());
	// IV. Retire completed instructions

	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_multicore_diagnosis->hasDiagnosisStarted()) {
			M_PSTATE->disableInterrupts();
		}
	}
	retireInstruction();

#ifdef CHECK_REGFILE
	// verify the free lists
	m_ooo.regs.validateMapping( m_id, m_iwin );
#endif


	// tell the sequencer to do its action on the executing instructons
	m_scheduler->Execute( getLocalCycle() );

	//
	// If running a uniprocessor simulation, or running in 'fake ruby' mode
	// this processor needs to advance the time of its local caches.
	//
	if (CONFIG_WITH_RUBY) {
#ifdef FAKE_RUBY
		m_ruby_cache->advanceTimeout();
#endif
	} else {
		// Tick L2 first
		l2_mshr->Tick();

		// then tick L1
		il1_mshr->Tick();
		dl1_mshr->Tick();
	}

#ifdef LXL_SNET_RECOVERY
	recoveryModuleExec() ;
	if(MULTICORE_DIAGNOSIS_ONLY) {
		m_multicore_diagnosis->runMulticoreDiagnosisAlgorithm(m_id);
	}
#endif

	// advance local time one cycle
	localCycleIncrement();
	if(DEBUG_PRINT_CYCLE) {
		out_info("*********************************************\n") ;
	}
}

/** Fetch a number of instructions past the current PC.
 *  This is where the "fetch policy" is implemented.
 *  Currently, the policy can be simple (idealizing some features),
 *  or multi-cycle (a more detailed model).
 */
//**************************************************************************
void pseq_t::fetchInstruction( void )
{
	fetchInstrSimple();
}

/** Fetch a number of instructions past the current PC.
 *  This simple front-end uses some idealizations that are not present
 *  in the complex "multi-cycle" front-end.
 */
//**************************************************************************
void pseq_t::fetchInstrSimple( void )
{

	dynamic_inst_t  *d_instr = NULL;
	static_inst_t   *s_instr = NULL;
	pa_t             fetchPhysicalPC;
	uint32           num_fetched=0;

	// XU DEBUG
	static bool last_mode = false;
	static dynamic_inst_t* last_d_call_instr = NULL;
	static bool last_fetch_is_call = false;

	// check the fetch status before fetching: 

	// check if we have waited long enough for squash penalty
	if (m_fetch_status == PSEQ_FETCH_SQUASH) {
		ASSERT(m_fetch_ready_cycle >= getLocalCycle());

		if (m_fetch_ready_cycle == getLocalCycle()) {
			m_fetch_status = PSEQ_FETCH_READY;
		}
	}

	static const bool fetch_pass_cache_line = (FETCH_PASS_CACHE_LINE > 0);
	static const bool fetch_pass_taken_branch = (FETCH_PASS_TAKEN_BRANCH > 0);

	if (m_fetch_status != PSEQ_FETCH_READY) {
		// if not ready, skip fetching

		//if (DEBUG_SIMPLE_FE) 
		//out_info("fetch is not ready: %s\n", fetch_menomic(m_fetch_status));

		ASSERT( m_fetch_status < PSEQ_FETCH_MAX_STATUS );
#ifdef FETCH_CHECKER
#ifdef VAR_TRACE
		TRACE_OUT("fetch_ready\n0\n") ;
		TRACE_OUT("PC\n%d\n", fetchPhysicalPC) ;
		TRACE_OUT("inst\n0\n") ;
		TRACE_OUT("\n") ;
#endif // VAR_TRACE
#endif // FETCH_CHECKER
	} else {
		// fetch engine is ready
		bool done = false;
		for (num_fetched = 0; !done && num_fetched < MAX_FETCH; num_fetched++) {

			//  Check to see that the instruction window has space for an instruction
			if (!m_iwin.isSlotAvailable()) {
				if(DEBUG_SIMPLE_FE) out_info("stalling fetch: no slots available\n");
				m_fetch_status = PSEQ_FETCH_WIN_FULL;
				break;
			}
			//lookupInstruction(m_fetch_at, &s_instr, &fetchPhysicalPC);
			lookupInstruction(m_fetch_at, &s_instr, &fetchPhysicalPC, num_fetched);

#ifdef SW_ROLLBACK
			if (m_op_mode ==RECOVERING) {
				goto create_dyn_inst;
			}
#endif

			if ( s_instr == m_fetch_itlbmiss ) {
				// ITLB miss

				/** When the fetch unit is unable to perform an address translation,
				 *  the pseq.C fetch state is changed to FETCH_ITLBMISS. Fetch is
				 *  restarted when any instruction squashes. So when the TLB
				 *  exception is handled the fetch is restarted.
				 */
				m_fetch_status = PSEQ_FETCH_ITLBMISS;
				done = true;
			} else {
				// ITLB hit, now access cache
				bool hit = false;
				if (CONFIG_WITH_RUBY) {
#ifdef DEBUG_RUBY
					out_info("IFETCH: PC 0x%0llx\n", fetchPhysicalPC);
#endif
					bool mshr_hit = false;
					bool mshr_stall = false;
					hit = getRubyCache()->access( fetchPhysicalPC, OPAL_IFETCH, 0ULL,
							(m_fetch_at->pstate >> 2) & 0x1,
							this, mshr_hit, mshr_stall );
					if (mshr_hit) {
						STAT_INC( m_stat_icache_mshr_hits );
					}
					if (mshr_stall) {
						// sequencer fetch context will never stall: it has a dedicated
						// MSHR resource assigned to it.
						SIM_HALT;
					}

					// if not in L1 cache: stall fetch pending i-cache return
					if (!hit) {
						STAT_INC( m_stat_num_icache_miss );
						if(DEBUG_SIMPLE_FE) out_info("l1 cache miss ... 0x%0llx\n",
								fetchPhysicalPC);
						m_fetch_status = PSEQ_FETCH_ICACHEMISS;
						break;
					}
				} else {
					/** check the ifetch line buffer
					 *  This include the case where pc1 == pc2, when last fetch was
					 *  a miss and it is then fetched again after woken up.
					 */
					if(! l1_inst_cache->same_line(m_last_fetch_physical_address, fetchPhysicalPC)) {
						hit = l1_inst_cache->Read( fetchPhysicalPC, this, false );
						m_last_fetch_physical_address = fetchPhysicalPC;
						// if not in L1 cache: stall fetch pending i-cache return
						if (!hit) {
							STAT_INC( m_stat_num_icache_miss );
							if (DEBUG_SIMPLE_FE) out_info("l1 cache miss ... 0x%0llx\n",
									fetchPhysicalPC);
							m_fetch_status = PSEQ_FETCH_ICACHEMISS;
							break;
						}

						// check shall we fetch pass cache line? (first fetch this cycle to new line is allowed)
						if(!fetch_pass_cache_line && num_fetched != 0) {
							if(DEBUG_SIMPLE_FE) out_info("do not pass cache line ... 0x%0llx\n", fetchPhysicalPC);
							break;
						}
					} else {
						/* fetch from the line buffer */
						hit = true;
						if(DEBUG_SIMPLE_FE) out_info("line buffer hit ... 0x%0llx\n", fetchPhysicalPC);
					}
				} // ! ruby cache
			} // !tlbmiss

create_dyn_inst:
			abstract_pc_t old_pc = *m_fetch_at;
			//   allocate a new dynamic instruction
			//            if (ALEX_TRACE_PC) {
			//                out_info("%d\t",getLocalCycle());
			//                out_info("m_fetch_at %llx m_ooo %llx\n",m_fetch_at->pc,m_ooo.at.pc);
			//            }

			m_fetch_at->fetch_id=num_fetched;

			d_instr = createInstruction( s_instr, m_fetch_at );
			//            if (ALEX_TRACE_PC) 
			//                out_info("m_fetch_at %llx m_ooo %llx\n",m_fetch_at->pc,m_ooo.at.pc);
			//
			//   add the dynamic instruction to the instruction window
			m_iwin.insertInstruction( d_instr );

			// if this is a barrier-type instruction, stall fetch
			if (s_instr->getFlag( SI_FETCH_BARRIER )) {
				m_fetch_status = PSEQ_FETCH_BARRIER;
				done = true;
			}
			// whether this is the delay slot of a taken branch?
			bool taken  = (m_fetch_at->pc - old_pc.pc) != sizeof(uint32);
			if (!fetch_pass_taken_branch && taken) {
				if(DEBUG_SIMPLE_FE) out_info("do not pass branch from V:0x%0llx to V:0x%0llx ... \n", old_pc.pc, m_fetch_at->pc);
				done = true;
			}

			// XU DEBUG: print control inst
			// FIXME - What a great if statment!!!
			if ( 0 && s_instr && s_instr->getType() == DYN_CONTROL) {
				char buf[128];
				ASSERT(d_instr);
				s_instr->printDisassemble(buf);
				out_info("%s [VPC 0x%llx]: %s\n", d_instr->getPrivilegeMode()? "kernel":"user", old_pc.pc, buf);
			}

			// excluding call&restore combination as CALL instr
			if (ras_t::PRECISE_CALL) {
				if(s_instr->getBranchType() == BRANCH_CALL) {
					last_fetch_is_call = true;
					last_d_call_instr = d_instr;
				} else {
					// call after restore is annulled
					if(last_fetch_is_call && s_instr->getOpcode() == i_restore) {
						// undo the push of last call
						m_ras->pop(&getSpecBPS()->ras_state);
						ASSERT(last_d_call_instr);
						((control_inst_t *) last_d_call_instr)->setPredictorState(*getSpecBPS());
					}
					last_fetch_is_call = false;
					last_d_call_instr = NULL;
				}
			} // ras_t::PRECISE_CALL

		} // end for
	} // end else (fetch is not ready)

	if (d_instr) last_mode = d_instr->getPrivilegeMode();

	/** insert the number of instructions fetched this cycle into the pipeline
	 *  these instructions become ready in FETCH_STAGES cycles
	 */
	m_fetch_per_cycle.insertItem( getLocalCycle(), FETCH_STAGES, num_fetched );

	ASSERT( num_fetched <= MAX_FETCH );
	// record the fetch histogram
	m_hist_fetch_per_cycle[num_fetched]++;
	// record the stall reason
	if (num_fetched == 0) m_hist_fetch_stalls[m_fetch_status]++;
	// reset status if it was a window full
	if (m_fetch_status == PSEQ_FETCH_WIN_FULL) m_fetch_status = PSEQ_FETCH_READY;

#ifdef DEBUG_PSEQ
	out_info("%d:num fetched PC 0x%0llx NPC 0x%0llx %d\n",
			m_id, m_fetch_at->pc, m_fetch_at->npc, num_fetched);
#endif

}

/** Task: check to see if this instruction is cached in the ipagemap
 *        (if it was seen before it is cached there). This results
 *        in the staticly decoded instruction pointer being set.
 */
//**************************************************************************
void pseq_t::lookupInstruction( const abstract_pc_t *apc,
		static_inst_t **s_instr_ptr,
		pa_t* fetchPhysicalPC_ptr,
		uint32 fetch_num) 
{
	bool            isValid = false;
	bool            inTLB = false;
	uint32          next_instr = 0;

	*s_instr_ptr = NULL;
	if ( CONFIG_IN_SIMICS ) {
		// (1)  perform address translation
#ifdef USE_MINI_ITLB
		// if enabled, use a mini-tlb (last TLB returned by simics) to a-translate
		// Note:
		//     Caching decoded instructions can impact correctness especially
		//     in the case of self-modifying code. If using the mini-TLB cache
		//     causes your correctness to go down, the cache of instructions
		//     (in the ipage/ipagemap) may not be getting properly invalidated.
		//     Historically, there was a problem with simics not notifying us
		//     on context changes which would also cause this instruction cache
		//     to get polluted. Uncomment code around 'CTXT_SWITCH' identifiers
		//     to see if we are notified of context switches. Finally, it should
		//     be noted the SPARC instruction 'FLUSH' is not properly implemented
		//     (and is used for self modifying code).
		bool miniTLB_hit = false;
		if ( (apc->pc & PSEQ_OS_PAGE_MASK) == m_itlb_logical_address ) {
			*fetchPhysicalPC_ptr = ( m_itlb_physical_address | 
					(apc->pc & ~PSEQ_OS_PAGE_MASK) );
			inTLB = true;
			miniTLB_hit = true;
#ifdef PIPELINE_VIS
			out_info("mini-itlb hit: itlb:0x%0llx bits:0x%0llx == 0x%0llx\n",
					m_itlb_physical_address, (apc->pc & ~PSEQ_OS_PAGE_MASK),
					*fetchPhysicalPC_ptr);
#endif
		} else {
			inTLB   = getInstr( apc->pc, apc->tl, apc->pstate,
					fetchPhysicalPC_ptr, &next_instr);
			if (inTLB) {
				m_itlb_logical_address  = apc->pc & PSEQ_OS_PAGE_MASK;
				m_itlb_physical_address = *fetchPhysicalPC_ptr & PSEQ_OS_PAGE_MASK;
				STAT_INC( m_stat_mini_itlb_misses );
			}
		}
#else
		// not USE_MINI_ITLB
		bool miniTLB_hit = false;
		inTLB   = getInstr( apc->pc, apc->tl, apc->pstate,
				fetchPhysicalPC_ptr, &next_instr);
#endif // USE_MINI_ITLB

		// (2)  find or construct decoded instruction
		if (!inTLB) {
			// instruction tlb miss

			// PC is not mapped by the current TLB ...
			// cause a trap on this instruction by fetching
			// a nop, and dynamically setting its trap flag
			*s_instr_ptr = m_fetch_itlbmiss;

		} else {
			// TLB hit
#ifndef REDECODE_EACH
			// read pre-decoded instruction from our cache
			isValid = queryInstruction( *fetchPhysicalPC_ptr, *s_instr_ptr );
			if (isValid) {
				// instr is in tlb and is valid (in opal's ipage cache)
				//   if we read from simics, verify instruction hasn't changed.
				if ( !miniTLB_hit &&
						next_instr != (*s_instr_ptr)->getInst() ) {
					STAT_INC( m_stat_modified_instructions );
					// INSTRUCTION_OVERWRITE
					out_info("overwrite vpc:0x%0llx pc:0x%0llx i:0x%0x new:0x%0x\n",
							apc->pc, *fetchPhysicalPC_ptr,
							(*s_instr_ptr)->getInst(), next_instr );

					*s_instr_ptr = insertInstruction( *fetchPhysicalPC_ptr,
							next_instr );
				}
			} else {
				// hit in ITLB cache, but missing a cached decoded instruction.
				if (miniTLB_hit) {
					// read next instruction from simics
					inTLB = getInstr( apc->pc, apc->tl, apc->pstate,
							fetchPhysicalPC_ptr, &next_instr);
				}
				// add the instruction to opal's ipage cache
				*s_instr_ptr = insertInstruction( *fetchPhysicalPC_ptr,
						next_instr);
			} // end if valid
#else
			miniTLB_hit = true;     // useless assignment to prevent warnings
			if (!inTLB) {
				*s_instr_ptr = m_fetch_itlbmiss;
			} else {
				//static_inst_t *decoded_instr = new static_inst_t( *fetchPhysicalPC_ptr,
				//                                                  next_instr );
				FAULT_SET_PRIV((apc->pstate & 0x4) == 0x4) ;
				//std::cout<<"m_id = "<<m_id<<endl;
				static_inst_t *decoded_instr = new static_inst_t( *fetchPhysicalPC_ptr,
						next_instr,
						fetch_num, m_id, fault_stat);
#ifdef FETCH_CHECKER
#ifdef VAR_TRACE
				TRACE_OUT("fetch_ready\n1\n") ;
				TRACE_OUT("PC\n%d\n", *fetchPhysicalPC_ptr) ;
				TRACE_OUT("inst\n%d\n", decoded_instr->getInst()) ;
				TRACE_OUT("\n") ;
#else // VAR_TRACE
				// (fetchready!=0 => inst!=0)
				if(decoded_instr->getInst()==0) {
					// checker detection
					bool curr_priv = ((apc->pstate & 0x4) == 0x4);
					int trap_level = m_control_arf->getRetireRF()->getInt( CONTROL_TL );
					OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
							m_local_sequence_number,
							"fetch_checker",
							0x303,*fetchPhysicalPC_ptr,
							curr_priv, 0,
							(uint16)trap_level, m_id);
					HALT_SIMULATION ;
				}
#endif // VAR_TRACE
#endif // FETCH_CHECKER

				// Register-based injection, we want to find out if
				// the destination regs have been mutated.

				if (decoded_instr->getFault()) {

					static_inst_t *orig_instr = new static_inst_t( *fetchPhysicalPC_ptr,
							next_instr);
					if (!decoded_instr->compareDestRegs(orig_instr)) {
						// Not good, dest reg has been mutated. 
						decoded_instr->setChangedDest(true);
						decoded_instr->getOrigDest(orig_instr);
					}
					// #if !defined(LL_DECODER) && defined(LXL_FIND_BITFLIP)
					decoded_instr->setOriginalInstr(orig_instr);
					// #endif
					// delete orig_instr;
				}

				decoded_instr->setFlag( SI_TOFREE, true );
				*s_instr_ptr = decoded_instr;
			}
#endif // ifndef REDECODE_EACH
		} // end if TLB miss
	} else {
		//
		// CONFIG_IN_SIMICS == false: running stand-alone
		//

		// translate virtual to physical address using (idealized) D-TLB
		bool inTLB = m_standalone_tlb->translateAddress( apc->pc, m_primary_ctx, 4,
				false,
				fetchPhysicalPC_ptr );

		if (!inTLB) {
			// no translation information: fetch the infamous nop instruction
			*s_instr_ptr = m_fetch_itlbmiss;
		} else {

			isValid = queryInstruction( *fetchPhysicalPC_ptr, *s_instr_ptr );
			// if the fetch PC and the retire PC are the same...
			// fetch the "MOP" instruction
			if (!isValid) {

				// NOTE: in stand-alone, may want to report error (as all 
				//       non-speculative instructions should be in map)
				//       However, this could be a speculative instruction.
				*s_instr_ptr = m_fetch_itlbmiss;
			}
		}
	} // end not in simics
	ASSERT( *s_instr_ptr != NULL );
}

/** Task: Fetch an instruction, insert it in the instruction window,
 *        Tag this instruction as Fetched,
 *        Advance PC/NPC state contained in fetch_at speculatively.
 */
//**************************************************************************
dynamic_inst_t* pseq_t::createInstruction( static_inst_t *s_instr,
		abstract_pc_t *fetch_at ) {
	dynamic_inst_t *d_instr = NULL;
	int32 index = m_iwin.getInsertIndex();
	STAT_INC( m_stat_fetched );

	// create a new dynamic instruction ... of the correct type
	switch ( s_instr->getType() ) {
		case DYN_EXECUTE:
			d_instr = new dynamic_inst_t(s_instr, index, this, fetch_at, fault_stat);
			// an i-tlb miss occured on this instruction ...
			if (s_instr == m_fetch_itlbmiss ) {
				d_instr->setTrapType( Trap_Fast_Instruction_Access_MMU_Miss );
			}
			break;

		case DYN_CONTROL:
			d_instr = new control_inst_t(s_instr, index, this, fetch_at, fault_stat);
			break;

		case DYN_LOAD:
			d_instr = new load_inst_t(s_instr, index, this, fetch_at, fault_stat);
			break;

		case DYN_STORE:
			d_instr = new store_inst_t(s_instr, index, this, fetch_at, fault_stat);
			break;

		case DYN_PREFETCH:
			d_instr = new prefetch_inst_t(s_instr, index, this, fetch_at, fault_stat);
			break;

		case DYN_ATOMIC:
			d_instr = new atomic_inst_t(s_instr, index, this, fetch_at, fault_stat);
			break;

		default:
#ifdef LL_DECODER
			bool curr_priv = ((fetch_at->pstate & 0x4) == 0x4);
			int trap_level = m_control_arf->getRetireRF()->getInt( CONTROL_TL );
			OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
					m_local_sequence_number,
					pstate_t::trap_num_menomic((trap_type_t)0x10), 
					0x10,fetch_at->pc,
					curr_priv, 0,
					(uint16)trap_level, m_id);
			HALT_SIMULATION ;
#else // LL_DECODER
			SIM_HALT;
#endif // LL_DECODER 
			break;
	}

	ASSERT( d_instr != NULL );

	if( s_instr->getFault()  && !s_instr->getLLFail()) {
		d_instr->setFault() ;
	}

#ifdef DECODER_CHECKER
	if(s_instr->getAssertFail()) {
#ifdef DEBUG_DECODER_CHECKER
		DEBUG_OUT("Checker failed on: ") ;
		d_instr->printRetireTrace() ;
#endif // DEBUG_DECODER_CHECKER
		OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
				GET_RET_INST(),
				"Decoder_checker",
				0x304,d_instr->getVPC(),
				d_instr->getPrivilegeMode(), 0x0, 0, m_id);
		HALT_SIMULATION ;
	}
#endif // DECODER_CHECKER

#ifdef PIPELINE_VIS
	char            buffer[128];
	s_instr->printDisassemble( buffer );
	// print out the instruction for later vis
	out_log("instr %d 0x%12.12llx 0x%8.8x \"%s\"\n",
			index, m_fetch_at->pc, s_instr->getInst(), buffer);
	out_log("cwp %d gset %d\n", m_fetch_at->cwp, m_fetch_at->gset);
	/* out_info("instr %d 0x%12.12llx 0x%8.8x \"%s\"\n",
	 *          index, m_fetch_at->pc, s_instr->getInst(), buffer);
	 */
#endif

	// advance the program counter by calling member function "nextPC"
	pmf_nextPC pmf = s_instr->nextPC;

	(d_instr->*pmf)( fetch_at );
	// CM AddressMask64 AM64 ?
	return d_instr;
}

/** Decodes instructions which have been fetched.
 *
 *  Decode does the register renaming on the source and destination registers,
 *  set its sequence number (a unique number-- a.k.a. priority).
 */
//**************************************************************************
void pseq_t::decodeInstruction( void )
{
	// decode avail is the current # of instrs ready for decode
	uint32  decode_avail = m_fetch_per_cycle.peekCurrent( getLocalCycle() );

#ifdef DEBUG_PSEQ
	out_info("Decoding %u instructions available\n", decode_avail ) ;
#endif

	uint32 num_decoded = 0;
	while ( (num_decoded < decode_avail) &&
			(num_decoded < MAX_DECODE) &&
			m_ooo.regs.registersAvailable() ) {

		// decode the appropriate instructions in the instruction window
		dynamic_inst_t *d_instr = m_iwin.decodeInstruction();
		STAT_INC( m_stat_decoded );
		d_instr->Decode( getLocalCycle() );
		num_decoded++;

#if defined(ARCH_TRANS_FAULT) && defined(ARCH_REGISTER_INJECTION)
		checkAndInjectTransientFault(d_instr, SRC_REG); 
#endif
	}

	// did not decode all available instructions, add remaining to next cycle
	if (num_decoded < decode_avail) {
		int leftover = decode_avail - num_decoded;
		ASSERT( leftover >= 0 );

		/* get number of instruction ready to be decoded next cycle
		 * add leftover number up to that
		 */
		uint32 numfetched = m_fetch_per_cycle.peekItem( getLocalCycle(), 1 );
		m_fetch_per_cycle.insertItem( getLocalCycle(), 1, numfetched + leftover );

		m_reg_stall_count_stat++;
		m_decode_stall_count_stat += leftover;
	}

	// insert the number of instructions not decoded this cycle to next cycle
	m_decode_per_cycle.insertItem( getLocalCycle(), DECODE_STAGES,
			num_decoded );

	ASSERT( num_decoded <= MAX_DECODE );
	m_hist_decode_per_cycle[num_decoded]++;

#ifdef DEBUG_PSEQ
	out_info("num decoded %d\n", num_decoded);
#endif
}

/** Schedules instructions which have been decoded on the functional
 *  units in the processor.
 *
 *  This function takes instructions from the decoder and identifies
 *  whether they are ready for execution.  If they are ready, they
 *  are passed on to the (centralized) scheduler associated with this
 *  sequencer.  Otherwise they are stuck in a waiting list for one of
 *  their un-ready register values.  (When the register value becomes
 *  ready they are rescheduled.
 */
//**************************************************************************
void pseq_t::scheduleInstruction( void )
{
	// schedule avail is the current # of instrs ready for schedule
	uint32  schedule_avail = m_decode_per_cycle.peekCurrent( getLocalCycle() );

#ifdef DEBUG_PSEQ  
	//DEBUG_OUT << "scheduling ... " << schedule_avail 
	//<< " instructions available" << endl;
	out_info( "Scheduling %u instructions available\n", schedule_avail ) ;
#endif

	/* This function schedules exactly the number of decoded instruction
	 * available. It is equivalent to the "issue/dispatch" stage in the
	 * processor. Instructions are executed OoO with m_schedule.Execute().
	 * So their state in i_window changes from E->C OoO. And C->O in retire
	 * stage.
	 *
	 * m_iwin.scheduleInstruction() return a valid instruction if
	 * the instruction window is not full.(Even ROB entries still
	 * available.
	 */
	// get instructions which can be scheduled now... see if they are ready
	uint32 num_scheduled;
	for ( num_scheduled = 0;
			num_scheduled < schedule_avail && num_scheduled < MAX_DISPATCH;
			num_scheduled++ ) {
		dynamic_inst_t *d_instr = m_iwin.scheduleInstruction();
		if (!d_instr)
			break;
		d_instr->testSourceReadiness();

		d_instr->Schedule();
	}

	// did not schedule all available instructions, add remaining to next cycle
	if (num_scheduled < schedule_avail) {
		int leftover = schedule_avail - num_scheduled;
		ASSERT( leftover >= 0 );

		/* get number of instruction ready to be scheduled next cycle
		 * add leftover number up to that
		 */
		uint32 numdecoded = m_decode_per_cycle.peekItem( getLocalCycle(), 1 );
		m_decode_per_cycle.insertItem( getLocalCycle(), 1, numdecoded + leftover );

		m_iwin_stall_count_stat++;
		m_schedule_stall_count_stat += leftover;
	}

	ASSERT( num_scheduled <= MAX_DISPATCH );
	m_hist_schedule_per_cycle[num_scheduled]++;

#ifdef DEBUG_PSEQ
	out_info("num scheduled %d\n", num_scheduled);
#endif
}

/** Retires instructions from the instruction window.
 */
//**************************************************************************
void pseq_t::retireInstruction( void )
{

	uint32         retire_count = 0;  // the number of instructions retiring

	// control variables for the retirement check
	check_result_t check_result;      // contains the result of retirement check
	bool           squash_pipeline;   // true if the pipeline must be squashed
	bool           must_check;        // true if the retired instr must be
	// checked (unimplemented instructions
	// for example)
	bool           must_update_all = false; // true if non-perfect instructions
	// will cause a complete state reload
	bool           step_ok = false;           // true if the next step was readable
	// status of the retire stage
	pseq_retire_status_t status =  PSEQ_RETIRE_LIMIT;
	bool           force_read_from_simics = false;
#if defined(LXL_SNET_RECOVERY) 
#if USE_TBFD
	static_inst_t *good_instr=NULL;
	//ireg_t good_pc=0;
	//uint32 good_iword=0;
	func_inst_info_t good_instr_info;
	uint16 func_tl=0;
	bool fatal_trap=false;
#endif //USE_TBFD

	do_not_advance=false;
	m_last_symptom=Trap_NoTrap;
#endif //LXL_SNET_RECOVERY

	/* if there is an outstanding exception, squash and refetch */
	if (m_except_type != EXCEPT_NONE) {
		STAT_INC( m_stat_exception_count );

#ifdef PIPELINE_VIS
		out_log("exception 0x%0llx 0x%0llx %s\n", 
				m_except_continue_at.pc, m_except_continue_at.npc,
				pstate_t::async_exception_menomic(m_except_type) );
#endif
		partialSquash(m_next_exception, &m_except_continue_at, m_except_offender);
		clearException();
	}

	// detect deadlock- issue a forward progress error if it takes too long
	// between retiring instructions
	if ( getLocalCycle() > m_fwd_progress_cycle ) {

		//DEBUG_OUT( "*** Lack of forward progress detected ***\n" );
		//DEBUG_OUT( "%d:    Cycle = %d, m_fwd_progress_cycle = %d \n", m_id, getLocalCycle(), m_fwd_progress_cycle );
		//out_info("*** Lack of forward progress detected ***\n");
		//// end debug filtering if there is an error
		//setDebugTime( 0 );

#ifdef FAULT_LOG
		OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
				GET_RET_INST(),
				pstate_t::trap_num_menomic(Trap_Watchdog_Reset), 
				0x2,0,
				GET_RET_PRIV(), 0x0, 0, m_id);
		FAULT_SET_NON_RET_TRAP() ;
#ifdef LXL_SNET_RECOVERY
		symptomDetected(0x2);
		m_fwd_progress_cycle = getLocalCycle() + 2*PSEQ_MAX_FWD_PROGRESS_TIME;

		// if (recovered || m_op_mode==RECOVERING) SW_ROLLBACK
		return;
#else // LXL_SNET_RECOVERY
		HALT_SIMULATION;
#endif //LXL_SNET_RECOVERY
#endif // FAULT_LOG
#ifndef LL_DECODER
		print();
#endif
		return;
	}

	/* Retire as many instructions which are ready to retire */
	dynamic_inst_t *d  = m_iwin.retireInstruction();
	enum i_opcode   op = i_mop;

	while ( d != NULL ) {

#ifdef LXL_SNET_RECOVERY
		if(d->getSequenceNumber()%getChkptInterval()==0) {
			takeChkpt(d) ;
			break ;
		}
#endif // LXL_SNET_RECOVERY
		//fake symptom for golden run to test Logging and TMR step for diagnosis
		//if((d->getSequenceNumber() == 50000) && (m_id == 0) && !getMulticoreDiagnosis()->isReplayPhase())  {
		//    symptomDetected(0x300);
		//}

#if defined (LXL_SNET_RECOVERY) && USE_TBFD
		if (recovered) {
			fatal_trap=false;
		}
#endif

		// if(d->getSequenceNumber()%100000 == 0) {
		//     DEBUG_OUT("Bit flips = %d\n", fault_stat->getTotalInj()) ;
		// }

		ireg_t trap_level = m_control_arf->getRetireRF()->getInt( CONTROL_TL );
#ifdef FAULT_LOG
		if( trap_level == MAXTL - 1 ) {
			// System in RED state
			OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
					d->getSequenceNumber(),
					pstate_t::trap_num_menomic(Trap_Red_State_Exception), 
					Trap_Red_State_Exception,d->getVPC(),
					d->getPrivilegeMode(), 0x0bad0bad,
					(uint16)trap_level, m_id);

			FAULT_SET_NON_RET_TRAP() ;
#ifdef LXL_SNET_RECOVERY
			symptomDetected(Trap_Red_State_Exception);
#else// LXL_SNET_RECOVERY
			HALT_SIMULATION ;          

#endif// LXL_SNET_RECOVERY
		} else if ( trap_level >= MAXTL ) {
			// Now, we are in ERROR state

			OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
					d->getSequenceNumber(),
					"Error_state", 
					0x29,d->getVPC(),
					d->getPrivilegeMode(), 0x0badbeef,
					(uint16)trap_level, m_id);

			FAULT_SET_NON_RET_TRAP() ;
#ifdef LXL_SNET_RECOVERY
			symptomDetected(0x29);
#else // LXL_SNET_RECOVERY
			HALT_SIMULATION ;

#endif // LXL_SNET_RECOVERY
		}

#endif // FAULT_LOG

		force_read_from_simics = false;
		FAULT_RET_INST( d->getSequenceNumber() ) ;
		FAULT_RET_PRIV( d->getPrivilegeMode() ) ;
		squash_pipeline  = false;
		must_check       = false;
		static_inst_t *s = d->getStaticInst();
		op = (enum i_opcode) s->getOpcode();

		// LXL: Track cycles and instrs in priv/non-priv mode

		// Identify if the interrupt handler has been exitted
		// Using IE, PIL From simics for this as opal's control registers
		// may mismatch with simics' values
		// FIXME - PIL is not set in the subsequent instruction, but takes
		// some time to get set. So, this doesn't work. Using hack for now.
		// if(isHandlingInterrupt() &&
		//         (M_STATE->isIE()) &&
		//         (M_PSTATE->getPIL()==0)) {
		//     exitInterrupt() ;
		//     DEBUG_OUT("Clearing Interrupt in %d\n",
		//             d->getSequenceNumber()) ;
		// }
		if(isHandlingInterrupt()) {
			// Note - When in interrupt handler, TL to TL-1 doesn't mean
			// interrupt handler has exitted. 
			if(getInterruptPriv()==0) {
				if (d->getPrivilegeMode()==0) {
					exitInterrupt() ;
					// DEBUG_OUT("Clearing Interrupt in %d\n",
					//         d->getSequenceNumber()) ;
				}
			} else {
				// This means that the interrupt was taken in priv mode 1.
				// Have to do some smart way of identifying how to see exit.
				// For now, just disable high os till you see priv mode 0 again
				// FIXME - Perhaps can use PIL to take care of this problem
				if (d->getPrivilegeMode()==0) {
					exitInterrupt() ;
					// DEBUG_OUT("Clearing Interrupt in %d\n",
					//         d->getSequenceNumber()) ;
				}
			} 
		}

		bool isIdleLoopInst = false;
		// if(op == i_ta) {
		//     //dont_count = true;
		//     debugio_t::out_info("TA inst ta=%s, priv=%d, trap_level=%d, trap_type=%llx\n",
		//             decode_opcode(op), d->getPrivilegeMode(), trap_level, d->getTrapType());
		// }

		isIdleLoopInst = inIdleLoop(d->getVPC());
#ifdef USE_APP_ABORT
		if(isIdleLoopInst) {
			OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
					d->getSequenceNumber(),
					"App_abort",
					0x204,d->getVPC(),
					d->getPrivilegeMode(), 0,
					(uint16)trap_level, m_id);
#ifdef LXL_SNET_RECOVERY
			symptomDetected(0x204);
#else // LXL_SNET_RECOVERY
			HALT_SIMULATION ;
#endif 
		}
#endif    // USE_APP_ABORT

		current_inst_num = d->getSequenceNumber();

		// Do not count OS instructions when it is in idle loop,
		// or is handling interrupts
		if(d->getPrivilegeMode() == 1) {
			TRACK_CYCLE(getLocalCycle(),
					d->getSequenceNumber(),
					d->getPrivilegeMode(),
					(isIdleLoopInst || dont_count || isHandlingInterrupt())) ;
			dont_count = false;
		}
		else {
			TRACK_CYCLE(getLocalCycle(),
					d->getSequenceNumber(),
					d->getPrivilegeMode(),
					false);
		}

#ifdef USE_HIGH_OS

		int threshold ;
#ifdef MULTI_CORE
		threshold = OS_INSTR_THRESHOLD_MULTI ;
#else // MULTI_CORE
		threshold = OS_INSTR_THRESHOLD ;
#endif // MULTI_CORE
		if ( d->getPrivilegeMode() &&
				!isHandlingInterrupt() && 
				getOSInstr()>threshold) {
			if(RETIRE_TRACE) {
				d->printRetireTrace("Trap=0x202: ") ;
			}
			OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
					d->getSequenceNumber(),
					"OS Instr Threshold Exceeded", 
					0x202,d->getVPC(),
					d->getPrivilegeMode(), 0,
					(uint16)trap_level, m_id);
			FAULT_SET_NON_RET_TRAP() ;
#ifdef LXL_SNET_RECOVERY
			if(MULTICORE_DIAGNOSIS_ONLY) {
				if(!m_multicore_diagnosis->hasDiagnosisStarted())  {
					symptomDetected(0x202);
					clearOSInstr();
				} 
			} else {
				symptomDetected(0x202);
				fault_stat->clearOSInstr();
			}

#else // LXL_SNET_RECOVERY
			HALT_SIMULATION ;

#endif // LXL_SNET_RECOVERY
		}
#endif // USE_HIGH_OS


		//for the following two cases, we dont need to have m_id in the
		//call. It doesnt matter which core calls this funciton,
		//onlything that matters is that is the system making forward
		//progress or is it a CPU panic

		//         if( isAppAbort(d->getVPC())) {
		//             OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
		//                     d->getSequenceNumber(),
		//                     "App abort exit",
		//                     0x204,d->getVPC(),
		//                     d->getPrivilegeMode(), 0,
		//                     (uint16)trap_level, m_id);
		//             FAULT_SET_NON_RET_TRAP() ;
		// #ifdef LXL_SNET_RECOVERY
		//             symptomDetected(0x204);
		// #else // LXL_SNET_RECOVERY
		//             HALT_SIMULATION ;
		// #endif // LXL_SNET_RECOVERY
		//         }

		//You might want to have panic as a symptom, but then, you have
		//to identify the pc
		//This pc should occur only where there is a panic
		// Hardcoded for multiple processors

		//if( d->getVPC() == panicPC()) 
		if( isKernelPanic(d->getVPC())) {
			OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
					d->getSequenceNumber(),
					"CPU Panic", 
					0x203,d->getVPC(),
					d->getPrivilegeMode(), 0,
					(uint16)trap_level, m_id);
			FAULT_SET_NON_RET_TRAP() ;
#ifdef LXL_SNET_RECOVERY
			symptomDetected(0x203);

			if(MULTICORE_DIAGNOSIS_ONLY) {
				if(m_multicore_diagnosis->isTMRPhase())
					break;
			}
#else //LXL_SNET_RECOVERY
			HALT_SIMULATION ;
#endif //LXL_SNET_RECOVERY
		}

#ifdef MULTI_CORE
		//check if all the cores are making forward progress. This
		//funtion checks if all the cores are retiring OS instrucitons
		//(idle/non-idle) for FORWARD_PROGRESS_THRESHOLD instrucitons
		//continuously
		if( !debugio_t::makingForwardProgress()) {
			OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
					d->getSequenceNumber(),
					"No Forward Progress", 
					0x204,d->getVPC(),
					d->getPrivilegeMode(), 0,
					(uint16)trap_level, m_id);
			FAULT_SET_NON_RET_TRAP() ;
#ifdef LXL_SNET_RECOVERY
			if(MULTICORE_DIAGNOSIS_ONLY) {
				if(!m_multicore_diagnosis->hasDiagnosisStarted())  {
					symptomDetected(0x204);
					debugio_t::clearForwardProgress();
				}
			} else {
				symptomDetected(0x204);
				debugio_t::clearForwardProgress();
			}

#else //LXL_SNET_RECOVERY
			HALT_SIMULATION ;
#endif //LXL_SNET_RECOVERY

		}
#endif //MULTI_CORE

		if(MULTICORE_DIAGNOSIS_ONLY) {
			//squash only in TMR phase, when the reg value doesnt match with llb
			//this should happend before takeTrap
			if(squash_next_instr) {
				//DEBUG_OUT("%d: Squashing next instr\n", m_id);
				fullSquash(op);
				squash_next_instr = false;
				d->Squash();
				m_multicore_diagnosis->reportSquash(m_id);    //for stats collection
				break;
			}
		}
#ifdef ARCH_REGISTER_INJECTION


		// if(d->getSequenceNumber()==fault_stat->getFaultInjInst()+1) {
		// 	
		//	// see memop.C
 		// 	// Test 1: dont corrupt the faulty address; just dont write the correct value in correct address.
		// 	// Fix faulty address with its original value;
		// 	uint64 fix_val = fault_stat->getFixVal();
  	 	// 	writePhysicalMemory(fault_stat->getFixAddr(), // correct address
  	 	// 	 			(int) fault_stat->getFixAccessSize(), &fix_val) ; // correct value
		// }
		if(d->getSequenceNumber()==fault_stat->getFaultInjInst()-5) {
			print_trace = 1;
		}
		if(d->getSequenceNumber()==fault_stat->getFaultInjInst()+500) {
			//print_trace = 0;
		}

		// This code is for dest register injections
		if(fault_stat->getFaultType() == REG_FILE_FAULT) {

			// squash instruction just before the injection point
			// squash instruction just after the injection point

			// This code is for source register injections

			if(isInjectionInstr(d)) {
				DEBUG_OUT("Squashing inj instr %lld\n", d->getSequenceNumber());
				fullSquash(op);
				d->Squash();
				done_squash_trans = true;
				break;
			}
			if(squash_next_instr && (d->getSequenceNumber()!=fault_stat->getFaultInjInst()) ) {
			 	DEBUG_OUT("Squashing next instr %lld\n", d->getSequenceNumber());
			 	fullSquash(op);
			 	squash_next_instr = false;
			 	d->Squash();
			 	break;
			}
		}
#ifdef SESE_POT
		if(d->getSequenceNumber()==fault_stat->getComparePoint()) {
			printArchState();
			printMemEvents();
			HALT_SIMULATION ;
		}
		if(d->getSequenceNumber()==fault_stat->getStartLogging()) {
			m_start_logging = true;
		}
#endif
#endif 


		/*
		 * Task: retire the instruction, unless it causes a trap.
		 */
		// The traptype of the retiring instruction
		trap_type_t traptype = d->getTrapType();
		if ( traptype == Trap_NoTrap ) {
			// Retire the instruction, unless single stepping simics
			// (we detect interrupts, and retire if single stepping)
			if (PSEQ_MAX_UNCHECKED != 1) {
				//    assert(0);
				//    d->Retire( m_inorder_at );
			}
		} else {
			if ( traptype == Trap_Use_Functional ) {

				// LXL: Functional trap, golden runs throw them, too
#ifdef FAULT_LOG
				if (ALEX_ACCURATE_DEBUG) { 
					out_info( "Functional_trap %d\n",d->getSequenceNumber() ) ;
				}
#endif
				force_read_from_simics = true;
				//FAULT_IN_FUNC_TRAP() ;
				squash_pipeline = true;
				must_check = true;
				m_opstat.functionalOp( op );
				STAT_INC( m_stat_count_functionalretire );
			} else {
				if (PSEQ_MAX_UNCHECKED != 1) {
					must_check = takeTrap( d, m_inorder_at, true );
					squash_pipeline = true;
				} // else UNCHECKED == 1 (must_check is irrelevant,as it will be checked)
			}
		}

		if(MULTICORE_DIAGNOSIS_ONLY) {
			//check if the value from the memory matches the value from LLB *before* advance simics
			//here, simics is not advanced
			if(!must_check) {
				if (PSEQ_MAX_UNCHECKED == 1) {
					if ( m_posted_interrupt == Trap_NoTrap ) {
						if (traptype == Trap_NoTrap ) {
							//DEBUG_OUT(" just before matching\n");
							if(!d->matchesLoadBuffer()) {
								if(0&&CONCISE_VERBOSE) {
									DEBUG_OUT("%d: squashing instr ", m_id);
									DEBUG_OUT(", opcode = %s\n", decode_opcode(op));
								}
								//partialSquash(m_iwin.getLastRetired(), m_inorder_at, op);
								fullSquash(op);
								d->Squash();
								m_multicore_diagnosis->reportSquash(m_id);    //for stats collection
								break;
							}


							//DEBUG_OUT(" after matching and before copying\n");
							//during TMR phase, store instructions should not write to the memory.
							//Sol: Copy the value before advanceSimics and then patch the value back
							//    after advance simics
							if(m_multicore_diagnosis->isTMRPhase()) {
								if(d->hasPhysicalAddress()) {
									d->copyStoreValue();
								} else {
									if(op != i_stxa || d->getPrivilegeMode() == 0) {
										DEBUG_OUT("%d:%dCheck this store Instr:%s\n", m_id, d->getPrivilegeMode(),  decode_opcode(op));
									}
#ifndef EXECUTE_STORES
									if(m_multicore_diagnosis->reasonForTMR() != SYMPTOM_IN_LOGGING_PHASE) {
										if(CONCISE_VERBOSE)
											DEBUG_OUT("%d: squashing and stopping core\n", m_id);
										fullSquash(op);
										d->Squash();
										m_multicore_diagnosis->stopCore(m_id);
										break;
									}
#endif
								}
							}
							//DEBUG_OUT(" after matching and copying\n");
						}
						if(!d->checkReadRegValue()) {
							if(0&&CONCISE_VERBOSE) {
								DEBUG_OUT("%d: read reg value mismatch squashing instr ", m_id);
								DEBUG_OUT(", opcode = %s\n", decode_opcode(op));
							}
							fullSquash(op);
							d->Squash();
							break;
						}
					}
				}
			}
		}

		// Add the instruction to the list of 'unchecked retires'
		// and update the forward progress cycle.
		must_check = must_check | uncheckedRetire( d );
		m_fwd_progress_cycle = getLocalCycle() + 2*PSEQ_MAX_FWD_PROGRESS_TIME;

		/*
		 * TASK: compare our results with SimIcs's results
		 */

		// If the retiring instruction has no injected fault, or was
		// a functional trap, or trap unimplemented, dont update from
		// simics
		// FIXME - What about decoder faults? Does this take care of it?
		if( force_read_from_simics ) {
			NO_FAULT_INJECTION() ;
		} else {
			DO_FAULT_INJECTION() ;
		}

		if (must_check) {
			// LXL: Here, we are catching fatal trap before Simics
			// does anything.  The reason for doing this is because
			// once Simics is in exception-handling mode, opal needs
			// to wait for it to get done before it can change the
			// processor state of Simics. For details, go to Simics
			// Forum and search for clearing excpetion. The thread was
			// started by Naveen from Craig's group.

#ifdef LXL_SNET_RECOVERY        
#if USE_TBFD
			if (recovered) {
				fatal_trap=false;
			}
#endif // USE_TBFD

			if (traptype!=Trap_NoTrap && isTrapFatal(traptype)) {
				// LXL: I tried initiating rollback from
				// here. However, it makes Simics do weird
				// stuff. E.g., restart PC=X and after
				// advanceSimics PC=X+8 always.
				//
				// Thus, a variable is used to initiate rollback outside
				symptomDetected(traptype);
				if(MULTICORE_DIAGNOSIS_ONLY) {
					if(m_multicore_diagnosis->isTMRPhase())
						break;
				}

				if( RETIRE_TRACE  || print_trace) {
					d->printRetireTrace( "Trap: " ) ;
				}

#if USE_TBFD
				if (recovered) {
					if (LXL_NO_CORRUPT) {
						assert(0);
						d->setTrapType(Trap_NoTrap);
						if (recovered) m_trigger_recovery=false;

					} else {
						fatal_trap=true;
					}
				}
#endif // USE_TBFD
			}

#if USE_TBFD
			if (recovered) {
				// This mean that we are doing diagnosis
				ireg_t good_pc=M_PSTATE->getControl(CONTROL_PC);
				uint32 good_iword;


				uint16 cwp = M_PSTATE->getControl(CONTROL_CWP);
				uint16 gset = pstate_t::getGlobalSet( M_PSTATE->getControl(CONTROL_PSTATE));

				pa_t phys_pc;
				unsigned int cur_inst;
				getInstr(good_pc, &phys_pc, &cur_inst);
				good_iword = cur_inst;
				good_instr = new static_inst_t( phys_pc,cur_inst);

				good_instr_info.VPC=good_pc;
				good_instr_info.iword=good_iword;
				good_instr_info.opcode=good_instr->getOpcode();
				good_instr_info.imm=good_instr->getImmediate();

				for (int i = 0; i < SI_MAX_SOURCE; i++) {
					reg_id_t &id = good_instr->getSourceReg(i);
					id.setVanillaState( cwp, gset );
					good_instr_info.source[i].rid =id;
					good_instr_info.source[i].value = readFuncSimValue(id);

				}

				for (int i = 0; i < SI_MAX_DEST; i++) {
					reg_id_t &id = good_instr->getDestReg(i);
					id.setVanillaState( cwp, gset );
				}
			}
#endif //USE_TBFD
#endif // LXL_SNET_RECOVERY


#ifdef LXL_SNET_RECOVERY
			//if (!m_trigger_recovery && m_op_mode!=RECOVERING) SW_ROLLBACK
			//            if (!m_trigger_recovery ) 
			if (!do_not_advance ) {
				step_ok = advanceSimics();

#if USE_TBFD
				if (recovered) {
					func_tl = M_PSTATE->getControl(CONTROL_TL);
				}
#endif //USE_TBFD

			} else {
				DEBUG_OUT(" step_ok\n");
				step_ok = true;
			}

#else // LXL_SNET_RECOVERY
			step_ok = advanceSimics();
#endif //LXL_SNET_RECOVERY

			if (step_ok) {
				if (PSEQ_MAX_UNCHECKED == 1) {
					// if no interrupts were posted, retire 'd' normally
					if ( m_posted_interrupt == Trap_NoTrap ) {
						if (traptype == Trap_NoTrap) {
							d->Retire( m_inorder_at );
							if( RETIRE_TRACE  || print_trace) {
								d->printRetireTrace( "Ret: " ) ;
							}
#ifdef BUILD_SLICE
							slice(d) ;
#endif // BUILD_SLICE

#ifdef FIND_UNIQUE_PCS
#ifdef BUILD_SLICE
							if(d->getSequenceNumber()<=SLICE_FAULTS_TILL)
#endif // BUILD_SLICE
								addRetirePC(d->getVPC()) ;
#endif // FIND_UNIQUE_PCS

#ifdef ARCH_TRANS_FAULT
							checkAndInjectTransientFault(d, DEST_REG);

#endif // ARCH_TRANS_FAULT

#ifdef GEN_DATA_VALUE_INV
							// if(d->getVPC()==0x1001c31c0) {
							// 	DEBUG_OUT("Gen value for %d\n", d->getSequenceNumber()) ;
							// }
							if(fault_stat->hasDetector(d->getVPC())) {
								reg_id_t &reg = d->getDestReg(0) ;
								if(!reg.isZero()) {
									int reg_type = reg.getARF()->getARFType() ;
									if(reg_type==INT_ARF) {
										// if(d->getVPC()==0x1001c31c0) {
										// 	DEBUG_OUT("Val(0x%llx) = %lld\n",
										// 			d->getVPC(), reg.getARF()->readInt64(reg)) ;
										// }
										fault_stat->addToRange(d->getVPC(), reg.getARF()->readInt64(reg)) ;
									} else if(reg_type==SINGLE_ARF) {
										fault_stat->addToRange(d->getVPC(), reg.getARF()->readFloat32(reg)) ;
									} else if(reg_type == DOUBLE_ARF) {
										fault_stat->addToRange(d->getVPC(), reg.getARF()->readFloat64(reg)) ;
									}
								}
							}
#endif // GEN_DATA_VALUE_INV 


#ifdef CHECK_DATA_VALUE_INV
							if(fault_stat->hasDetector(d->getVPC())) {
								if(DEBUG_CHECK_DATA_VALUE_INV) {
									DEBUG_OUT("0x%llx has detector\n", d->getVPC()) ;
								}
								reg_id_t &reg = d->getDestReg(0) ;
								if(!reg.isZero()) {
									int reg_type = reg.getARF()->getARFType() ;
									bool in_range = true ;
									if(reg_type==INT_ARF) {
										in_range = fault_stat->isInRange(d->getVPC(), reg.getARF()->readInt64(reg)) ;
									} else if(reg_type == SINGLE_ARF) {
										in_range = fault_stat->isInRange(d->getVPC(), reg.getARF()->readFloat32(reg)) ;
									} else if(reg_type ==DOUBLE_ARF) {
										in_range = fault_stat->isInRange(d->getVPC(), reg.getARF()->readFloat64(reg)) ;
									}

									if(!in_range) {
										OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
												d->getSequenceNumber(),
												"Value_Range_Violation",
												0x208,d->getVPC(),
												d->getPrivilegeMode(), 0,
												(uint16)trap_level, m_id);
										FAULT_SET_NON_RET_TRAP() ;
#ifdef LXL_SNET_RECOVERY
										if(MULTICORE_DIAGNOSIS_ONLY) {
											if(!m_multicore_diagnosis->hasDiagnosisStarted())  {
												symptomDetected(0x208);
											} 
										} else {
											symptomDetected(0x208);
										}

#else // LXL_SNET_RECOVERY
										HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
									}
								}
							}
#endif // CHECK_DATA_VALUE_INV 

#ifdef CHECK_DATA_ONLY_VALUES
							if(fault_stat->isDataOnly(d->getVPC())) {
								if(d->isCorrupted()) {
									OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
											d->getSequenceNumber(),
											"Faulty_Data_Only_Value",
											0x209,d->getVPC(),
											d->getPrivilegeMode(), 0,
											(uint16)trap_level, m_id);
									FAULT_SET_NON_RET_TRAP() ;
#ifdef LXL_SNET_RECOVERY
									if(MULTICORE_DIAGNOSIS_ONLY) {
										if(!m_multicore_diagnosis->hasDiagnosisStarted())  {
											symptomDetected(0x209);
										} 
									} else {
										symptomDetected(0x209);
									}

#else // LXL_SNET_RECOVERY
									HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
								}
							}
#endif // CHECK_DATA_ONLY_VALUES

							if(MULTICORE_DIAGNOSIS_ONLY) {
								if(op == i_ta) {
									m_multicore_diagnosis->collectCompareLog(d->getVPC(), -1, m_id);
								}

								if(m_multicore_diagnosis->isTMRPhase() && d->updateOnly()) {
									squash_next_instr = true;
									squash_pipeline = true;
									//DEBUG_OUT("%d: squash ", m_id);
									//DEBUG_OUT("opcode = %s\n", decode_opcode(op));
								}
								if(m_multicore_diagnosis->isTMRPhase() && m_multicore_diagnosis->isCoreStoppedInTMR(m_id)) {
									//dont try to retire the subsequent insturctions if the core is stopped in tmr
									squash_pipeline = true;
								}
							}

						} else if (traptype == Trap_Use_Functional) {

							// LXL: Functional trap, golden runs throw them, too
#ifdef FAULT_LOG
							//FAULT_OUT( "Functional_trap\t" ) ;
							if (LXL_DEBUG && !force_read_from_simics)
								FAULT_OUT("expecting force_read_simics true\n");
							if (ALEX_ACCURATE_DEBUG)
								out_info("func trap %lld\n",d->getSequenceNumber());
#endif
							if(MULTICORE_DIAGNOSIS_ONLY) {
								m_multicore_diagnosis->reportSequenceNumber(m_id);

								if(m_multicore_diagnosis->isLoggingPhase()) {
									updateInstructionState( d, force_read_from_simics );
									log_func_inst_info(d);    //should follow update instruction state
								} else if(m_multicore_diagnosis->isTMRPhase()) {
									restore_func_inst_info(d);
									d->writeBackStoreValue();
								} else {
									// read register from simics
									updateInstructionState( d, force_read_from_simics );
								}
							} else {
								updateInstructionState( d, force_read_from_simics );
							}
							// FIXME - flush for func trap!
							// must_update_all = true ;

							d->Retire( m_inorder_at );
							//DEBUG_OUT("%d:func took trap 0x%x\n",m_id,traptype);
							if( RETIRE_TRACE  || print_trace) {
								d->printRetireTrace( "Func: " ) ;
							}
#ifdef BUILD_SLICE
							slice(d) ;
#endif // BUILD_SLICE


#ifdef FIND_UNIQUE_PCS
#ifdef BUILD_SLICE
							if(d->getSequenceNumber()<=SLICE_FAULTS_TILL)
#endif // BUILD_SLICE
								addRetirePC(d->getVPC()) ;
#endif // FIND_UNIQUE_PCS

#ifdef ARCH_TRANS_FAULT
							checkAndInjectTransientFault(d, DEST_REG);
#endif // ARCH_TRANS_FAULT

#ifdef GEN_DATA_VALUE_INV
							// if(d->getVPC()==0x1001c31c0) {
							// 	DEBUG_OUT("Gen value for %d\n", d->getSequenceNumber()) ;
							// }
							if(fault_stat->hasDetector(d->getVPC())) {
								reg_id_t &reg = d->getDestReg(0) ;
								if(!reg.isZero()) {
									int reg_type = reg.getARF()->getARFType() ;
									if(reg_type==INT_ARF) {
										// if(d->getVPC()==0x1001c31c0) {
										// 	DEBUG_OUT("Val(0x%llx) = %lld\n",
										// 			d->getVPC(), reg.getARF()->readInt64(reg)) ;
										// }
										fault_stat->addToRange(d->getVPC(), reg.getARF()->readInt64(reg)) ;
									} else if(reg_type==SINGLE_ARF) {
										fault_stat->addToRange(d->getVPC(), reg.getARF()->readFloat32(reg)) ;
									} else if(reg_type == DOUBLE_ARF) {
										fault_stat->addToRange(d->getVPC(), reg.getARF()->readFloat64(reg)) ;
									}
								}
							}
#endif // GEN_DATA_VALUE_INV 

#ifdef CHECK_DATA_VALUE_INV
							if(fault_stat->hasDetector(d->getVPC())) {
								reg_id_t &reg = d->getDestReg(0) ;
								if(!reg.isZero()) {
									int reg_type = reg.getARF()->getARFType() ;
									bool in_range = true ;
									if(reg_type==INT_ARF) {
										in_range = fault_stat->isInRange(d->getVPC(), reg.getARF()->readInt64(reg)) ;
									} else if(reg_type == SINGLE_ARF) {
										in_range = fault_stat->isInRange(d->getVPC(), reg.getARF()->readFloat32(reg)) ;
									} else if(reg_type ==DOUBLE_ARF) {
										in_range = fault_stat->isInRange(d->getVPC(), reg.getARF()->readFloat64(reg)) ;
									}

									if(!in_range) {
										OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
												d->getSequenceNumber(),
												"Value_Range_Violation",
												0x208,d->getVPC(),
												d->getPrivilegeMode(), 0,
												(uint16)trap_level, m_id);
										FAULT_SET_NON_RET_TRAP() ;
#ifdef LXL_SNET_RECOVERY
										if(MULTICORE_DIAGNOSIS_ONLY) {
											if(!m_multicore_diagnosis->hasDiagnosisStarted())  {
												symptomDetected(0x208);
											} 
										} else {
											symptomDetected(0x208);
										}

#else // LXL_SNET_RECOVERY
										HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
									}
								}
							}
#endif // CHECK_DATA_VALUE_INV 

#ifdef CHECK_DATA_ONLY_VALUES
							if(fault_stat->isDataOnly(d->getVPC())) {
								if(d->isCorrupted()) {
									OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
											d->getSequenceNumber(),
											"Faulty_Data_Only_Value",
											0x209,d->getVPC(),
											d->getPrivilegeMode(), 0,
											(uint16)trap_level, m_id);
									FAULT_SET_NON_RET_TRAP() ;
#ifdef LXL_SNET_RECOVERY
									if(MULTICORE_DIAGNOSIS_ONLY) {
										if(!m_multicore_diagnosis->hasDiagnosisStarted())  {
											symptomDetected(0x209);
										} 
									} else {
										symptomDetected(0x209);
									}

#else // LXL_SNET_RECOVERY
									HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
								}
							}
#endif // CHECK_DATA_ONLY_VALUES

						} else {
							if (TLB_IS_IDEAL &&
									((traptype == Trap_Fast_Instruction_Access_MMU_Miss) ||
									 (traptype == Trap_Fast_Data_Access_MMU_Miss))) {
								// fastforward through this trap
								fastforwardSimics();
								// squash the pipeline
								partialSquash(m_iwin.getLastRetired(), m_inorder_at, op);
								d->Squash();
								squash_pipeline = true;
								must_update_all = true;
							} else {
								if(RETIRE_TRACE || print_trace) {
									char a[100] = "" ;
									sprintf(a,"Trap=0x%x: ",d->getTrapType()) ; 
									d->printRetireTrace(a) ;
								}
								// Need ot check ranges for instructions that cause traps, else we lose
								// opportuinty.
								if(MULTICORE_DIAGNOSIS_ONLY)
									if(CONCISE_VERBOSE)
										DEBUG_OUT( "%d: Trap:0x%x \n", m_id, traptype ) ;
#ifdef RANGE_CHECK
								// Now, if the address of an application store/load instruction is
								// out of address space, then this is a symptom!
								memory_inst_t* mem_inst = dynamic_cast<memory_inst_t*>(d) ;
								if( mem_inst) {
									my_addr_t addr = mem_inst->getAddress() ;
									if(!(d->getPrivilegeMode())) {
										if( (addr>=0xffffffff80000000ULL) || // The last 4GB is reserved! 
												(addr<0x100000000) ) { // Below this is not used!
											DEBUG_OUT("Address is invalid: 0x%llx\n", addr) ;
											OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
													d->getSequenceNumber(),
													"Address_out_of_range",
													0x206,d->getVPC(),
													d->getPrivilegeMode(), 0,
													(uint16)trap_level, m_id);
#ifdef LXL_SNET_RECOVERY
											symptomDetected(0x204);
#else // LXL_SNET_RECOVERY
											HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
										}
									}
									bool valid_access = fault_stat->rangeCheck(d->getVPC(), addr) ;
									if(!valid_access) {
										DEBUG_OUT("[%d] 0x%llx access 0x%llx out of range!\n",
												d->getSequenceNumber(), d->getVPC(),addr) ;
										d->printRetireTrace("Out of bounds: ") ;
										OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
												d->getSequenceNumber(),
												"Range_check_violation",
												0x206,d->getVPC(),
												d->getPrivilegeMode(), 0,
												(uint16)trap_level, m_id);
#ifdef LXL_SNET_RECOVERY
										symptomDetected(0x206);
#else // LXL_SNET_RECOVERY
										HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
									}
								}
#endif //RANGE_CHECK

#ifdef HARD_BOUND_BASE
								// Now, if the address of an application store/load instruction is
								// out of address space, then this is a symptom!
								memory_inst_t* mem_inst = dynamic_cast<memory_inst_t*>(d) ;
								if( mem_inst) {
									my_addr_t addr = mem_inst->getAddress() ;
#ifdef BINARY_32_BIT
									ireg_t top = 0x10000 ;
#else // BINARY_32_BIT
									ireg_t top = 0x100000000 ;
#endif // BINARY_32_BIT
									if(!d->getPrivilegeMode() && addr<top){ // Below this is not used!
										DEBUG_OUT("Address out of range: 0x%llx\n", addr) ;
										OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
												d->getSequenceNumber(),
												"Address_out_of_range",
												0x206,d->getVPC(),
												d->getPrivilegeMode(), 0,
												(uint16)trap_level, m_id);
#ifdef LXL_SNET_RECOVERY
										symptomDetected(0x206);
#else // LXL_SNET_RECOVERY
										HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
									}
									if(!d->getPrivilegeMode() && !debugio_t::isValidObject(addr)) { 
										d->printRetireTrace("pseq No valid object: " ) ;
										OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
												d->getSequenceNumber(),
												"Range_check_violation",
												0x206,d->getVPC(),
												d->getPrivilegeMode(), 0,
												(uint16)trap_level, m_id);
#ifdef LXL_SNET_RECOVERY
										symptomDetected(0x206);
#else // LXL_SNET_RECOVERY
										HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
									}
								}
#endif //HARD_BOUND_BASE

#ifdef HARD_BOUND
								load_inst_t* load_inst = dynamic_cast<load_inst_t*>(d) ;
								store_inst_t* store_inst = dynamic_cast<store_inst_t*>(d) ;
								atomic_inst_t* atomic_inst = dynamic_cast<atomic_inst_t*>(d) ;
								// TODO - will it be a problem if the destination reg/mem gets
								// the object before this instruction retires? Should we undo just
								// that part?
								if(load_inst) {
									load_inst->propagateObjects(1) ;
								} else if(store_inst) {
									store_inst->propagateObjects(1) ;
								} else if(atomic_inst) {
									atomic_inst->propagateObjects(1) ;
								} else {
									// Just a regular instruction. Don't care for now.
								}
#endif // HARD_BOUND
								// take the traP
								force_read_from_simics = takeTrap( d, m_inorder_at, true );
								squash_pipeline = true;
								if( force_read_from_simics ) { // Unimplemented instruction
									if(CONCISE_VERBOSE)
										DEBUG_OUT("%d: Unimplemented :%d \n", m_id, m_multicore_diagnosis->getCurrentInstNum(m_id));
									NO_FAULT_INJECTION();

								}
#ifdef LXL_SNET_RECOVERY
								if (recovered) {
									// DEBUG_OUT("took trap 0x%x\n",traptype);
									trap_level = m_control_arf->getRetireRF()->getInt( CONTROL_TL );
								}
#endif
							}
						}
					} else {
						// interrupt was posted

						if(RETIRE_TRACE || print_trace) {
							d->printRetireTrace("Interrupt: ") ;
						}

						d->setTrapType( m_posted_interrupt );

						out_log("\n[%d]: Interrupt posted at %d: ", m_id, getLocalCycle()) ;
						out_log( ": priv=%d",d->getPrivilegeMode());
						out_log( ": type=0x%x\n",d->getTrapType() ) ;

						//reset the counter when you see an interrupt
						dont_count = true;
						m_report_interrupt_to_diagnosis = true;
						//track= true;

						//if(d->getTrapType() == 0x4d) {
						//HALT_SIMULATION;
						//out_log("\n start tracking");
						//track = true;
						//out_log("\n stop counting");
						//dont_count = true;
						//}


						force_read_from_simics = takeTrap( d, m_inorder_at, true );
						squash_pipeline = true;
						if( force_read_from_simics ) { // Unimplemented instruction
							NO_FAULT_INJECTION();
						}
#ifdef LXL_SNET_RECOVERY
						if (recovered) {
							trap_level = m_control_arf->getRetireRF()->getInt( CONTROL_TL );
						}
#endif

						// This final else clause used to cause a small number of
						// retiring (stores mostly) to fail. If the store
						// experiences a TLB miss, _and_ an interrupt occurs,
						// the machine model specifies that the interrupt wins.
						// However, we would have squashed and taken the trap
						// (i.e. the interrupt is not detected until after retirement)
						// so in this case, this instruction will deviate.
						// I corrected this by making "takeTrap" not execute above
						// when MAX_UNCHECKED == 1.

						// reset posted interrupt
						m_posted_interrupt = Trap_NoTrap;
					}

				} else {  // PSEQ_MAX_UNCHECKED != 1
					// for bundled retires, update state after stepping simics
					if (traptype == Trap_Use_Functional) {
						// read register from simics
						updateInstructionState( d, false );
					}
				}

				//   do the commit check for the important (PC, PSTATE) registers
				check_result.perfect_check = true;
				check_result.critical_check = true;

#ifdef LXL_SNET_RECOVERY
#if MULTICORE_DIAGNOSIS_ONLY
				check_result.recovered = false;
#else
				check_result.recovered = recovered;
#endif
				check_result.retry_mismatch = false;
				check_result.do_not_sync = false;

				checkCriticalState( &check_result, &m_ooo, force_read_from_simics );

				check_result.retry_mismatch = false;
#else

				checkCriticalState( &check_result, &m_ooo, force_read_from_simics );

#endif // LXL_SNET_RECOVERY
				if (!check_result.critical_check) {
					STAT_INC( m_stat_count_badretire );
					must_update_all = true;
				}

				// do the commit check each unchecked instruction
				dynamic_inst_t *dcheck = getNextUnchecked();

#ifdef DEBUG_FUNCTIONALITY
				if (!check_result.critical_check) {
					out_info("critial check fails time: %lld\n", getLocalCycle() );
					dcheck->printDetail();
				}
#endif
				while (dcheck != NULL) {
					op = (enum i_opcode) dcheck->getStaticInst()->getOpcode();

					// if stat committed isn't incremented, the simulation will never
					// terminate (other stats should be able to be turned off however)
					// STAT_INC( m_stat_committed );
					m_stat_committed++;

#ifdef CHECK_TRAP_RATE
					if( m_stat_committed%TRAP_RATE_RESET== 0 ) {
						curr_trap_rate = 0 ;
					}
#endif

#ifdef CHECK_HEART_BEAT 
					if ((m_stat_committed % 1000000)==0)
						DEBUG_OUT("[%d] committed instr: %d\n",
								m_id, m_stat_committed);
#endif // CHECK_HEART_BEAT
					ASSERT( dcheck->getEvent( EVENT_FINALIZED ) == true ) ;

					// send this instruction to the committed instruction stream proc
					commitObserver( dcheck );

					// LXL: set src reg to be committed to Simics
					dcheck->setReadFaults() ;

					// do a commit check for all unchecked instructions
#ifdef PIPELINE_VIS
					checkAllState( &check_result, &m_ooo, force_read_from_simics );
#else

#if defined (LXL_SNET_RECOVERY) && USE_TBFD
					//SW_ROLLBACK
					//if (LXL_CHECK_RESULT_ONLY && (m_op_mode==RECOVERING || recovered)) 
					if (LXL_CHECK_RESULT_ONLY && recovered) {
						assert(dcheck==d);

						// Get retiring instruction info from faulty machine
						instruction_information_t instr_info;

						updateInstrInfo(instr_info, d);

						for (int i = 0; i < SI_MAX_DEST; i++) {
							reg_id_t &id = good_instr->getDestReg(i);
							good_instr_info.dest[i].rid=id;
							good_instr_info.dest[i].value=readFuncSimValue(id);

						}


						if (VERBOSE_IN_CHECK) {
							check_result.verbose=true;
						}

						if (PRINT_INSTR_AFTER_RECOVERY) 
							d->printDetail();

						// We just want to check the result when we are diagnosing
						checkChangedState( &check_result, &m_ooo, dcheck, force_read_from_simics );

						store_inst_t*store_inst=dynamic_cast<store_inst_t*>(d);

						if (store_inst!=NULL) {
							if (store_inst->isCorruptingStore()) {
								check_result.retry_mismatch=true;
							}
						}

						d->transferMismatchInfo(instr_info);
						instr_info.mismatch = check_result.retry_mismatch;
						instr_info.good_inst = good_instr_info;


						if (fatal_trap) {
							instr_info.isTrap=fault_cause_fatal;
						} else {
							instr_info.isTrap=(traptype!=Trap_NoTrap) && (trap_level != func_tl) &&
								(traptype!=Trap_Unimplemented) && (traptype!=Trap_Use_Functional);
						}

						m_diagnosis->insertInstrInfo(instr_info);

						if (check_result.retry_mismatch) {
							// Whenever there's a mismatch, roll back
							//m_trigger_recovery=true;
							if (LXL_NO_CORRUPT)
								m_last_symptom=Trap_NoTrap;

							//d->printDetail();

						}
						if ((m_last_symptom!=Trap_NoTrap) && (m_last_symptom!=0x202)) {
							m_diagnosis->fatalTrap();
							m_trigger_recovery=true;
						}

						bool changed_perfect_check=check_result.perfect_check;

						// Prevent check all state below to sync up timing and functional sim
						check_result.do_not_sync=true;

						// Why do we do check all state here? LXL
						//check_result.verbose=false;
						checkAllState( &check_result, &m_ooo, force_read_from_simics );
						//check_result.verbose=false;

						check_result.perfect_check=changed_perfect_check;

						assert(good_instr!=NULL);
						if (good_instr!=NULL) {
							// prevent memory leak
							delete good_instr;
							good_instr=NULL;
						}

					} else {
						// LXL: Before, we are checking the entire
						// arch state. Now, we are just checking the
						// produced value. If we want to take a look
						// at the entire arch state, the above code
						// can be copied and MODIFIED.

						checkAllState( &check_result, &m_ooo, force_read_from_simics );
					}
#else // !LXL_SNET_RECOVERY || !USE_TBFD
					//checkChangedState( &check_result, &m_ooo, dcheck, force_read_from_simics );

					//if(getLocalCycle() == 46585 || getLocalCycle() == 41151) {
					//    d->printDetail();
					//}

					//if(m_multicore_diagnosis->getNumLoggingPhases() >315) {
					//DEBUG_OUT("%d: seq_number=%d\n",m_id, d->getSequenceNumber());
					//    check_result.verbose=true;
					// }
					check_result.patch_dest = false;
					if (VERBOSE_IN_CHECK) {
						check_result.verbose=true;
					}

					// if(dcheck->getVPC() == 0x1008bac) {
					// 	DEBUG_OUT("Calling checkAllState() with init = %d\n", force_read_from_simics) ;
					// }
					checkAllState( &check_result, &m_ooo, force_read_from_simics );

#endif // ( def(LXL_SNET_RECOVERY) && USE_TBFD )

#endif // PIPELINE_VIS

					if (check_result.critical_check && check_result.perfect_check) {
						// force updates on unimplemented instructions
						if (dcheck->getTrapType() == Trap_Unimplemented) {
#ifdef PIPELINE_VIS
							out_log("dy %d unimpl %s\n", dcheck->getWindowIndex(),
									decode_opcode( op ));
#endif
							STAT_INC( m_stat_commit_unimplemented );
							must_update_all = true;
							//debugio_t::out_info(" trap_unimplemented\n");
						} else {
#ifdef PIPELINE_VIS
							out_log("dy %d success %s in cycle %d\n", dcheck->getWindowIndex(),
									decode_opcode( op ), getLocalCycle() );
#endif
							STAT_INC( m_stat_commit_good );
							m_opstat.successOp( op );
						}
					} else {
						// print out which instruction fails
#ifdef PIPELINE_VIS 
						out_log("dy %d fail %s in cycle %d\n", dcheck->getWindowIndex(),
								decode_opcode( op ), getLocalCycle() );

#endif
						// Changing to make it retire even on mismatch
#ifdef RETIRE_ZERO_TOLERANCE
						must_update_all = true;
#endif
						//// CM FIX: should re-read and re-decode this instruction
						STAT_INC( m_stat_commit_bad );
					}

					pushRetiredInstruction( dcheck );
					dcheck = getNextUnchecked();
				}
				if(MULTICORE_DIAGNOSIS_ONLY ) {
					if (PSEQ_MAX_UNCHECKED == 1) {
						// if no interrupts were posted, retire 'd' normally
						if ( m_posted_interrupt == Trap_NoTrap ) {
							if (traptype == Trap_NoTrap) {
								if(!M_PSTATE->getAdvanceSimicsException()) {

									//if(previous_pc == d->getVPC()) {
									//    DEBUG_OUT("%d: Not reporting sequence number\n", m_id);
									//    m_multicore_diagnosis->notReportingSequenceNumber(m_id, d->getVPC());
									//}
									d->collectCompareLog();
									//if(previous_pc != d->getVPC()) {
									m_multicore_diagnosis->reportSequenceNumber(m_id, d->getVPC());
									//}
								}
								previous_pc = d->getVPC();
							} else
								if(traptype == Trap_Unimplemented) {
									m_multicore_diagnosis->reportSequenceNumber(m_id);

									if(m_multicore_diagnosis->isLoggingPhase()) {
										//log_func_inst_info(d);
									} else if(m_multicore_diagnosis->isTMRPhase()) {
										//TODO: Need to patch the destination 
										//registers of unimplemented instrucitons
										//restore_func_inst_info(d);
										d->writeBackStoreValue();
										squash_next_instr = true;
										squash_pipeline = true;
									}

								}
						}
					}
				}
			} else { // end step_ok
				// the step will only 'fail' if we're running a trace. this signals
				// the end of simulation. Note: there may be some unchecked
				// instructions if the step size is > 1.

				HALT_SIMULATION;
				return;
			}

			ASSERT( m_unchecked_retires == 0 );
		} // if (must_check) results

		//if (must_update_all && !recovered)  THIS LINE IS FOR !NO_CORRUPT
		if (must_update_all) {
			// update all state
#ifdef PIPELINE_VIS
			checkAllState( &check_result, &m_ooo, force_read_from_simics );
#else
			check_result.update_only = false;
			checkAllState( &check_result, &m_ooo, force_read_from_simics );
			check_result.update_only = false;
#endif
			// the last op retired (if bundling retires) is charged for the squash

			fullSquash( op );
			status = PSEQ_RETIRE_UPDATEALL;
			break;
		}


		/* limit the number of instructions which can retire in one opportunity */
		retire_count++;
		if (retire_count >= MAX_RETIRE) {
			break;
		}


		// if the pipeline was squashed -- stop retiring instrs
		if (squash_pipeline) {
			STAT_INC( m_stat_commit_squash );
			status = PSEQ_RETIRE_SQUASH;
			break;
		}

		// fetch the next instruction to retire
		//DEBUG_OUT(" retiring inst\n");
		d = m_iwin.retireInstruction();
	}

	/** Note: the main loop breaks out to this point, so nothing should be 
	 *        added, if it is NOT shared by all cases below:
	 *                    No more inst to retire;
	 *                    reached retire_max;
	 *                    pipeline squash;
	 *                    fullSquash()
	 */
	ASSERT( retire_count <= MAX_RETIRE );
	m_hist_retire_per_cycle[retire_count]++;
	if (retire_count != PSEQ_RETIRE_LIMIT) {
		m_hist_retire_stalls[status]++;
	}


	if(MULTICORE_DIAGNOSIS_ONLY) {
		if(m_multicore_diagnosis->hasDiagnosisStarted()) {
			if(m_report_interrupt_to_diagnosis) {
				DEBUG_OUT(" reporting Interrupt\n");    
				if(m_multicore_diagnosis->hasDiagnosisStarted()) {
					for(int i=0; i<10; i++) {
						step_ok = advanceSimics();
						if(!step_ok)
							break;
					}
					m_multicore_diagnosis->reportInterrupt(m_id);
				}
				m_report_interrupt_to_diagnosis = false;
			}
		}
	}
	//DEBUG_OUT(" %d: end of retireInstr\n",m_id);
}

//**************************************************************************
	void 
pseq_t::raiseException(exception_t type, 
		uint32 win_index,
		enum i_opcode offender,
		abstract_pc_t *continue_at,
		my_addr_t access_addr,
		uint32 exception_penalty)
{
	ASSERT( m_iwin.isInWindow(win_index) );
	if (type < EXCEPT_NUM_EXCEPT_TYPES)
		m_exception_stat[type]++;

	/* if this exeception is sooner than any other one outstanding, keep it */
	if ( (m_except_type == EXCEPT_NONE) ||
			(m_iwin.rangeSubtract( win_index, m_next_exception ) < 0) ||
			((win_index == m_next_exception) && (type == EXCEPT_MISPREDICT)) ) {

		// out_info("cycle %lld: new E %d %d 0x%0llx\n", getLocalCycle(), type, win_index, m_except_continue_at.pc );

#if 0
		// CM FIX
		// This condition is when you're trying to squash an instruction that
		// has already retired. It only occurs when stale data speculation is
		// turned on, but should generate a warning!
		if ( m_iwin.getLastRetired() == m_iwin.iwin_increment(win_index) ) {
			out_info("exception: last window: %0lld 0x%0llx\n",
					getLocalCycle(), m_except_continue_at.pc );
		}
#endif

#ifdef PIPELINE_VIS
		out_log("raisedException\n");
		out_log("cycle %lld: new E %d %d 0x%0llx\n",
				getLocalCycle(), type, win_index, m_except_continue_at.pc );
		printPC( continue_at );
#endif
		m_except_type = type;
		m_next_exception     = win_index;
		m_except_offender    = offender;
		m_except_continue_at = *continue_at;
		m_except_access_addr = access_addr;
		m_except_penalty     = exception_penalty;
	}
}

//**************************************************************************
	void 
pseq_t::clearException( void )
{
	m_next_exception     =  0;
	m_except_offender    =  i_nop;
	m_except_continue_at.init();
	m_except_access_addr =  0;
	m_except_type        = EXCEPT_NONE;
}

//**************************************************************************
	bool
pseq_t::takeTrap( dynamic_inst_t *instruction, abstract_pc_t *inorder_at,
		bool is_timing )
{
	trap_type_t      traptype = Trap_NoTrap;
	physical_file_t *control_state_rf = NULL;

	if (is_timing) {
		enum i_opcode op = (enum i_opcode) instruction->getStaticInst()->getOpcode();
		traptype = instruction->getTrapType();

		// CM don't use full squash here: m_fetch_at won't be reset correctly
		partialSquash(m_iwin.getLastRetired(), inorder_at, op);
		// NOTE: due to rollback of register renaming, must squash this
		//       instruction AFTER calling squash (always reverse order)
		instruction->Squash();

		// NOTE: increment lsq index. Squash decrements it, but simics
		//       will be retiring an instruction. keep in step with it. :)
		stepSequenceNumber();

		control_state_rf = m_control_arf->getRetireRF();

	} else {
		// please forgive me, my light day hacker friend -- The Night Hacker!
		flow_inst_t *my_flow = (flow_inst_t *) instruction;
		traptype = my_flow->getTrapType();
		control_state_rf = m_ideal_control_rf;
	}

	// if this instruction is not implemented, merely tell the caller
	// that the complete state must be checked before retiring it.
	if (traptype == Trap_Unimplemented) {
		// fake trap, don't need to print
		//FAULT_OUT( "\ttrappc:0x00(%d)\n", control_state_rf->getInt( CONTROL_TL )) ;
		if (ALEX_ACCURATE_DEBUG) out_info("unimplemented instr\n");
		return true;
	}

#ifdef DEBUG_RETIRE
	out_info("## Trapped Retire Stage\n");
	instruction->printDetail();
#endif

	// The following implements trap handling in accordance to SPARC V9 docs
	ireg_t tba     = control_state_rf->getInt( CONTROL_TBA );
	ireg_t tl      = control_state_rf->getInt( CONTROL_TL );
	ireg_t pstate  = control_state_rf->getInt( CONTROL_PSTATE );
	ireg_t cwp     = control_state_rf->getInt( CONTROL_CWP );
	uint16 gset    = REG_GLOBAL_NORM;
	uint16 ccr     = 0;

	if(instruction->getTrapType() == m_posted_interrupt) {
		takingInterrupt(instruction->getPrivilegeMode()) ;
		// DEBUG_OUT("Taking Interrupt in %d\n", instruction->getSequenceNumber()) ;
	}

	if (is_timing) {
		ccr = m_cc_rf->getInt( m_cc_retire_map->getMapping( REG_CC_CCR ) );
	} else {
		ccr = control_state_rf->getInt( CONTROL_CCR );
	}

	// write the trap type to the TT register
	int tt = (int) traptype;
	if ( tt < TRAP_NUM_TRAP_TYPES ) {
		m_trapstat[tt]++;
	}

	bool arch_mismatch = fault_stat->getArchMismatch() || instruction->getFault(); 

	control_state_rf->setInt( CONTROL_TT1 + tl, tt );
	if (arch_mismatch) {
		control_state_rf->setFault( CONTROL_TT1 + tl );
	}

#ifdef PIPELINE_VIS
	out_log("trap 0x%0x\n", tt);
#endif

#ifdef DEBUG_PSEQ
	out_log("TAKING TRAP 0x%0x 0x%0x 0x%0x\n", tt, (tt << 5),
			((tl != 0) << 14));
#endif

	// preserve the existing state
	ireg_t trapstate = 0;
	uint16 asi    = control_state_rf->getInt( CONTROL_ASI ) & 
		makeMask64( reg_box_t::controlRegSize(CONTROL_ASI) - 1, 0 );

	trapstate |=  cwp;
	trapstate |= ((ireg_t) pstate << 8);
	trapstate |= ((ireg_t) asi << 24);
	trapstate |= ((ireg_t) ccr << 32);

	control_state_rf->setInt( CONTROL_TSTATE1 + tl, trapstate );
	control_state_rf->setInt( CONTROL_TPC1 + tl, inorder_at->pc );
	control_state_rf->setInt( CONTROL_TNPC1 + tl, inorder_at->npc );

	// LXL: added to corrupt simics
	if (arch_mismatch) {
		control_state_rf->setFault(CONTROL_TSTATE1 + tl);
		control_state_rf->setFault( CONTROL_TPC1 + tl );
		control_state_rf->setFault( CONTROL_TNPC1 + tl );
	}

	if (is_timing) {
		// also: push the old trap address (PC,NPC, etc.) on to a stack (predictor)
		m_tlstack->push( inorder_at->pc, inorder_at->npc, (uint16) pstate,
				(uint16) cwp, (uint16) tl );
	}

	/*
	 *  Task:  update the processor state PSTATE
	 */
	// mask off everything except TLE and MM
	pstate  =  pstate & 0x1c0;
	// move TLE over to CLE, mask back on
	pstate |= ((pstate << 1) & 0x200);
	// set architecturally defined (known) mode bits
	// 0x10 == PEF  floating point enabled
	// 0x04 == PRIV privileged mode
	pstate |= 0x14;

	// set global bits
	if (tt == Trap_Interrupt_Vector) {
		// IG == 11 bits over
		pstate |= (1 << 11);
	} else if (tt == Trap_Fast_Instruction_Access_MMU_Miss ||
			tt == Trap_Fast_Data_Access_MMU_Miss ||
			tt == Trap_Fast_Data_Access_Protection ||
			tt == Trap_Instruction_Access_Exception ||
			tt == Trap_Data_Access_Exception) {
		// MG == 10 bits over
		pstate |= (1 << 10);
	} else {
		// AG == 0 bits over (alternate globals)
		pstate |= 1;
	}
	control_state_rf->setInt( CONTROL_PSTATE, pstate );

	if (arch_mismatch) {
		control_state_rf->setFault( CONTROL_PSTATE );
	}
	//
	// immediately find the new global set (in architected state)
	//
	gset = pstate_t::getGlobalSet( pstate );


	/*
	 * Task: update the CWP (if a spill / fill trap)
	 *       I tried to do this in the execute logic, but the wrong CWP was saved
	 */
	if (tt == Trap_Clean_Window) {
		// CLEAN WINDOW TRAP
		// increment cwp (for trap handler)
		cwp++;
		cwp &= (NWINDOWS - 1);
		control_state_rf->setInt( CONTROL_CWP, cwp );
		if (arch_mismatch) 
			control_state_rf->setFault( CONTROL_CWP );

	} else if ( (uint32) tt > (uint32) 0x80  &&  (uint32) tt < (uint32) 0xbf ) {
		// WINDOW SPILL TRAP
		// set cwp (for trap handler) to be CWP + CANSAVE + 2
		ireg_t cansave = control_state_rf->getInt( CONTROL_CANSAVE );
		cwp  = cwp + cansave + 2;
		cwp &= (NWINDOWS - 1);
		control_state_rf->setInt( CONTROL_CWP, cwp );
		if (arch_mismatch) 
			control_state_rf->setFault( CONTROL_CWP );

	} else if ( (uint32) tt > (uint32) 0xc0  &&  (uint32) tt < (uint32) 0xff ) {
		// WINDOW FILL TRAP
		// set cwp (for trap handler) to be CWP - 1
		cwp--;
		cwp &= (NWINDOWS - 1);
		control_state_rf->setInt( CONTROL_CWP, cwp );
		if (arch_mismatch) 
			control_state_rf->setFault( CONTROL_CWP );
	}

	if ( control_state_rf->getInt( CONTROL_CWP ) > NWINDOWS ) {
		out_info("error: pseq: trap: cwp set to %lld\n",
				control_state_rf->getInt( CONTROL_CWP ));
	}

	// compute and set the program counter based on the trap number
	la_t eff_addr = tba | (tt << 5) | ((tl != 0) << 14);

	// Increment Trap Level register TL
	tl++;
	if (tl > MAXTL) {
		out_info("tl out of range: 0x%0llx\n", tl);
		tl = MAXTL;
	}
#ifdef DEBUG_PSEQ
	out_info("writing trap level: %lld\n", tl);
#endif
	control_state_rf->setInt( CONTROL_TL, tl );
	if (arch_mismatch) {
		control_state_rf->setFault( CONTROL_TL );
		inorder_at->has_fault = true; // make sure to corrupt path
	}

	// CM AddressMask64 AM64
	inorder_at->pc     = eff_addr;
	inorder_at->npc    = eff_addr + sizeof(uint32);
	inorder_at->tl     = tl;
	inorder_at->pstate = pstate;
	inorder_at->cwp    = cwp;
	inorder_at->gset   = gset;

	if (recovered && isTrapFatal(traptype) ) {
		if (ROLLBACK_AT_SYMPTOM) advanceSimics();
		fault_cause_fatal=(M_PSTATE->getControl(CONTROL_PC)!=eff_addr);
	}

	// re-start the timing model's fetch at the inorder PC
	if (is_timing) {
		*m_fetch_at = *inorder_at;
	}
#ifdef FAULT_LOG
#define PRINT_ALL_TRAPS 0

#if PRINT_ALL_TRAPS
	debugio_t::out_info("\n trap: ");
	debugio_t::out_trap_info((uint64) getLocalCycle(),
			instruction->getSequenceNumber(),
			pstate_t::trap_num_menomic(traptype), 
			traptype,instruction->getVPC(),
			instruction->getPrivilegeMode(), eff_addr,
			(uint16)tl,m_id);
#endif

	if (isTrapFatal(traptype)) {
		OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
				instruction->getSequenceNumber(),
				pstate_t::trap_num_menomic(traptype), 
				traptype,instruction->getVPC(),
				instruction->getPrivilegeMode(), eff_addr,
				(uint16)tl, m_id);

#ifndef LXL_SNET_RECOVERY
		HALT_SIMULATION;
#endif
	}
#endif

#ifdef FAULT_CORRUPT_SIMICS
	if( tl == MAXTL - 1 ) {
		// System in RED state
		OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
				instruction->getSequenceNumber(),
				pstate_t::trap_num_menomic(Trap_Red_State_Exception), 
				Trap_Red_State_Exception,instruction->getVPC(),
				instruction->getPrivilegeMode(), eff_addr,
				(uint16)tl, m_id);
#ifdef LXL_SNET_RECOVERY
		symptomDetected(Trap_Red_State_Exception);
#else
		HALT_SIMULATION ;
#endif
	} else if ( tl >= MAXTL ) {
		// Now, we are in ERROR state
		OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
				instruction->getSequenceNumber(),
				"Error_state", 
				0x29,instruction->getVPC(),
				instruction->getPrivilegeMode(), eff_addr,
				(uint16)tl, m_id);
#ifdef LXL_SNET_RECOVERY
		symptomDetected(0x29);
#else
		HALT_SIMULATION ;

#endif
	}
#endif

#ifdef CHECK_TRAP_RATE
	curr_trap_rate ++ ;

	if( curr_trap_rate > TRAP_RATE ) {
		OUT_SYMPTOM_INFO( (uint64) getLocalCycle(),
				instruction->getSequenceNumber(),
				"High Trap rate",
				0x200,
				instruction->getVPC(),
				instruction->getPrivilegeMode(),
				eff_addr,
				(uint16)tl, m_id ) ;
		HALT_SIMULATION ;
	}
#endif

	// FIXME: changing MMU register
	if (LXL_NO_CORRUPT && recovered ) return false;

	if ((arch_mismatch  && CONFIG_IN_SIMICS && 
				traptype==Trap_Fast_Data_Access_MMU_Miss)) {
		char mmuname[200];
		pstate_t::getMMUName( m_id, mmuname, 200 );
		conf_object_t *mmu = SIMICS_get_object( mmuname );
		attr_value_t ret=SIM_get_attribute(mmu,"dsfar");
		if (ret.u.integer!=((memory_inst_t*)instruction)->getAddress()) {
			attr_value_t context_id;
			ret.u.integer=((memory_inst_t*)instruction)->getAddress();
			SIM_set_attribute(mmu,"dsfar",&ret);
			if ((tl-1)>0) {
				context_id=SIM_get_attribute(mmu,"ctxt_nucleus");
			} else {
				context_id=SIM_get_attribute(mmu,"ctxt_primary");
			}

			ret.u.integer = (ret.u.integer & PSEQ_OS_PAGE_MASK) | (context_id.u.integer & 0x1fff);
			SIM_set_attribute(mmu,"dtag_access",&ret);
			ret.u.integer = (ret.u.integer >> 22) | ((context_id.u.integer & 0x1fff) << 48);
			SIM_set_attribute(mmu,"dtag_target",&ret);


			if (ALEX_MMU_DEBUG) {
				// DEBUG_OUT("mmu addr doesn't match\n");
				ret=SIM_get_attribute(mmu,"dsfar"); 
				DEBUG_OUT("data miss addr 0x%llx\n",ret.u.integer);
				ret=SIM_get_attribute(mmu,"dtag_access"); 
				DEBUG_OUT("data miss tag  0x%llx\n",ret.u.integer);
				ret=SIM_get_attribute(mmu,"dtag_target"); 
				DEBUG_OUT("data tag targ  0x%llx\n",ret.u.integer);
				ret=SIM_get_attribute(mmu,"ctxt_primary"); 
				DEBUG_OUT("data context   0x%llx\n",ret.u.integer);
				DEBUG_OUT("my d context   0x%llx\n",context_id.u.integer);
			}

		}
	} 

	return false;
}

void pseq_t::prepareRollback()
{

	previous_pc = -1;
	TRACK_CYCLE(getLocalCycle(), current_inst_num, 0, false);
	check_result_t result;
	syncAllState(&result, &m_ooo, false);
	fullSquash(i_nop);
	clearException();
	clearAllFaults();
	clearUnchecked();

#ifdef TRACK_MEM_SHARING
	debugio_t::clear_corrupted_address_map();
#endif

#ifdef REPLAY_PHASE
	recoverTLB();
#endif
}

void pseq_t::postRollback()
{
	check_result_t check_result;      // contains the result of retirement check
	uint64 before_recovery=fault_stat->getFaultInjInst();

	fault_stat->setFaultInjInst(400*1000*1000);

	reg_id_t        rid;
	abstract_rf_t  *arf;
	mstate_t       *mstate = &m_ooo;
	pstate_t       *pstate = M_PSTATE;

	// reset all windowed integer registers
	rid.setRtype( RID_INT );
	arf = mstate->regs.getARF( rid.getRtype() );
	arf->rollbackReset();

	// reset the global registers
	rid.setRtype( RID_INT_GLOBAL );
	arf = mstate->regs.getARF( rid.getRtype() );
	arf->rollbackReset();

	// reset the fp registers
	rid.setRtype( RID_SINGLE );
	arf = mstate->regs.getARF( rid.getRtype() );
	arf->rollbackReset();

	// reset the fp registers
	rid.setRtype( RID_CONTROL );
	arf = mstate->regs.getARF( rid.getRtype() );
	arf->rollbackReset();

#ifdef SW_ROLLBACK
	m_op_mode=RECOVERED;
#endif

	checkAllState( &check_result, &m_ooo, true );

	//No need for memory rollback during multicore diagnosis (Logging phase or TMR phase)
	//Rollback memory state only after symptom detection during faulty run, before diagnosis
	if(m_multicore_diagnosis->isFirstTimeRollback()) {
		if(DIAGNOSIS_THRESHOLD >= 1) {
			m_ruby_cache->notifySNETRollback();
		}
		m_multicore_diagnosis->clearFirstTimeRollback();
	}
#ifdef REPLAY_PHASE
	m_ruby_cache->notifySNETRollback();
#endif

	*m_fetch_at = *m_inorder_at;

	fault_stat->setFaultInjInst(before_recovery);

	if (ALEX_SNET_DEBUG) {
		DEBUG_OUT("checkpointed state: %d\n", m_id);
		DEBUG_OUT("m_fetch_at pc is %llx\n",m_fetch_at->pc);
		DEBUG_OUT("m_inorder_at pc is %llx\n",m_inorder_at->pc);

		ireg_t simreg = M_PSTATE->getControl( CONTROL_PSTATE );
		bool mask = ISADDRESS32(simreg);

		ireg_t simregnew = M_PSTATE->getControl( CONTROL_PC ) ;
		if( mask ) simregnew = AM64( simregnew ) ;
		DEBUG_OUT( "PC: After patching, value = %llx mask:%d\n", simregnew,mask ) ;
		DEBUG_OUT( "SIMICS PC: %llx\n", M_PSTATE->getPC());
	}
}

	void
pseq_t::rollbackRecovery()
{
	check_result_t check_result;      // contains the result of retirement check

	fullSquash(i_nop);
	clearException();

	int curr_chkpt = M_PSTATE->getCurrChkptNum()-1 ;
	if(curr_chkpt < 0 ) {
		curr_chkpt = NUM_CHKPTS-1 ;
	}
	DEBUG_OUT("old chkpt %d, curr_num = %d\n", M_PSTATE->getCurrChkptNum(), curr_chkpt) ;
	M_PSTATE->setChkptNum(curr_chkpt) ;
	// The curr_chkpt is now chkpt-1
	DEBUG_OUT("Rolling back to chkpt %d\n", M_PSTATE->getCurrChkptNum()) ;
	M_PSTATE->recoverState();

	// Registers should not be tagged faulty after rollback
	clearAllFaults();
#if TRACK_MEM_SHARING
	debugio_t::clear_corrupted_address_map();
#endif
	uint64 before_recovery=fault_stat->getFaultInjInst();

	fault_stat->setFaultInjInst(400*1000*1000);

	reg_id_t        rid;
	abstract_rf_t  *arf;
	pstate_t       *pstate = M_PSTATE;
	mstate_t       *mstate = &m_ooo;

	// reset all windowed integer registers
	rid.setRtype( RID_INT );
	arf = mstate->regs.getARF( rid.getRtype() );
	arf->rollbackReset();

	// reset the global registers
	rid.setRtype( RID_INT_GLOBAL );
	arf = mstate->regs.getARF( rid.getRtype() );
	arf->rollbackReset();

	// reset the fp registers
	rid.setRtype( RID_SINGLE );
	arf = mstate->regs.getARF( rid.getRtype() );
	arf->rollbackReset();

	// reset the fp registers
	rid.setRtype( RID_CONTROL );
	arf = mstate->regs.getARF( rid.getRtype() );
	arf->rollbackReset();

#ifdef SW_ROLLBACK
	m_op_mode=RECOVERED;
#endif

	checkAllState( &check_result, &m_ooo, true );

	*m_fetch_at = *m_inorder_at;

	recoverTLB();
	m_ruby_cache->notifySNETRollback();
	notifyAmberRollback() ;
	fault_stat->setFaultInjInst(before_recovery);
	if (ALEX_SNET_DEBUG) {
		DEBUG_OUT("checkpointed state\n");
		DEBUG_OUT("m_fetch_at pc is %llx\n",m_fetch_at->pc);
		DEBUG_OUT("m_inorder_at pc is %llx\n",m_inorder_at->pc);

		ireg_t simreg = M_PSTATE->getControl( CONTROL_PSTATE );
		bool mask = ISADDRESS32(simreg);

		ireg_t simregnew = M_PSTATE->getControl( CONTROL_PC ) ;
		if( mask ) simregnew = AM64( simregnew ) ;
		DEBUG_OUT( "PC: After patching, value = %llx mask:%d\n", simregnew,mask ) ;
		DEBUG_OUT( "SIMICS PC: %llx\n", M_PSTATE->getPC());
	}
}

void pseq_t::notifyAmberRollback()
{
	system_t::inst->notifyAmberRecovery() ;
}

	void
pseq_t::checkRegisterReady()
{
	reg_id_t        rid;
	abstract_rf_t  *arf;
	mstate_t *mstate = &m_ooo;

	// compare all windowed integer registers
	rid.setRtype( RID_INT );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (uint32 cwp = 0; cwp < NWINDOWS; cwp++ ) {
		rid.setVanillaState( cwp, 0 );
		// compare the IN and LOCAL registers
		for (int reg = 31; reg >= 16; reg --) {
			rid.setVanilla( reg );
			arf->checkRegReadiness( rid );
		}
	}

	// compare the global registers
	rid.setRtype( RID_INT_GLOBAL );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (uint16 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset++) {
		rid.setVanillaState( 0, gset );
		// ignore register %g0
		for (int reg = 1; reg < 8; reg ++) {
			rid.setVanilla( reg );
			arf->checkRegReadiness( rid );
		}
	}
}


//**************************************************************************
	void
pseq_t::partialSquash(uint32 last_good, abstract_pc_t *fetch_at,
		enum i_opcode offender )
{
	int32 num_decoded = 0;

#ifdef DEBUG_PSEQ 
	out_log("partial squash: %lld 0x%0llx %d\n", getLocalCycle(), fetch_at->pc, last_good);
#endif

	STAT_INC( m_stat_total_squash );
	m_opstat.squashOp( offender );

	// squash the instructions out of the iwindow
	m_iwin.squash(this, last_good, num_decoded);

	/******************** Fetch-related fields ********************/ 

	/* if this sequencer is waiting for an i-cache miss, stop waiting */
	if (Waiting()) RemoveWaitQueue();

	// flush the fetch pipe
	m_fetch_per_cycle.init(0);
	if (m_except_type == EXCEPT_MISPREDICT && m_except_penalty != 0) {
		// if mis-prediction, charge some penalty
		m_fetch_status = PSEQ_FETCH_SQUASH;
		m_fetch_ready_cycle = getLocalCycle() + m_except_penalty;
	} else {
		// otherwise, immediately resume fetch
		m_fetch_status = PSEQ_FETCH_READY;
	}

	// reset the fetch control fields to reflect the 'future' architected state
	// where the squash is occuring
	*m_fetch_at = *fetch_at;

	// for multicycle I$, reset waiting flag, can set waiting addr cause TLB
	if (ICACHE_CYCLE > 1 && m_wait_for_request) m_wait_for_request = false;

	/******************** Decode-related fields *******************/ 

	// flush the decode pipe
	m_decode_per_cycle.init(0);
	m_decode_per_cycle.insertItem( getLocalCycle(), 1, num_decoded);
	if (num_decoded < IWINDOW_WIN_SIZE) {
		m_hist_decode_return[num_decoded]++;
	} else {
		m_hist_decode_return[IWINDOW_WIN_SIZE - 1]++;
	}

	/************** Schedule/Execute-related fields ***************/ 
	/* force a recount of inflight instructions?? */

	if(MULTICORE_DIAGNOSIS_ONLY) {
		llb->resetReadPointer();
	}

}

//**************************************************************************
	void
pseq_t::fullSquash( enum i_opcode offender )
{
	int32         last_good = m_iwin.getLastRetired();

	// restart fetch based on the architected control state
	abstract_pc_t fetch_at;
	partialSquash( last_good, &fetch_at, offender );


	physical_file_t *control_rf = m_control_arf->getRetireRF();
	m_fetch_at->pc     = control_rf->getInt( CONTROL_PC );
	m_fetch_at->npc    = control_rf->getInt( CONTROL_NPC );
	m_fetch_at->tl     = control_rf->getInt( CONTROL_TL );
	m_fetch_at->pstate = control_rf->getInt( CONTROL_PSTATE );
	m_fetch_at->cwp    = control_rf->getInt( CONTROL_CWP );

	m_fetch_at->gset = pstate_t::getGlobalSet( m_fetch_at->pstate );
	if ( ISADDRESS32(m_fetch_at->pstate) ) {
		//assert(0);
		m_fetch_at->pc  = AM64( m_fetch_at->pc );
		m_fetch_at->npc = AM64( m_fetch_at->npc );
	}
#ifdef PIPELINE_VIS
	out_info("AFTER full squash\n");
	printPC( m_fetch_at );
#endif
}


//**************************************************************************
	void
pseq_t::postEvent( waiter_t *waiter, uint32 cyclesInFuture )
{
	m_scheduler->queueInsert( waiter, m_local_cycles, cyclesInFuture - 1 );
}

//**************************************************************************
void pseq_t::Wakeup( void )
{
#if defined DEBUG_PSEQ || defined DEBUG_RUBY
	out_info("pseq: wakeup(): cycle: %lld\n", m_local_cycles);
#endif

	// we only woken up by the I$, don't reset fetch_status if not waiting
	if( m_fetch_status == PSEQ_FETCH_ICACHEMISS)
		m_fetch_status = PSEQ_FETCH_READY;
}

//**************************************************************************
void pseq_t::unstallFetch( void )
{
	ASSERT( m_fetch_status == PSEQ_FETCH_BARRIER );
	m_fetch_status = PSEQ_FETCH_READY;
}

/*------------------------------------------------------------------------*/
/* Functional Inorder Processor Model                                     */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pseq_t::printCFG( void )
{
	// walk the current CFG, printing each node
	// print CFG starting at start index
	DEBUG_OUT( "pseq_t:: printCFG called\n");
	m_cfg_pred->printGraph( 0 );
}

//**************************************************************************
pseq_fetch_status_t pseq_t::idealRunTo( int64 runto_seq_num )
{
	static_inst_t   *s_instr = NULL;
	pa_t             fetchPhysicalPC;

	// check to find if this sequence number is ready or not
	//    if its to low < last_checked, we have no information on it
	if (runto_seq_num < m_ideal_last_checked) {
		ERROR_OUT("error: idealRunTo: unable to run to: too far in past [%lld < %lld]!\n", runto_seq_num, m_ideal_last_checked );
		return (PSEQ_FETCH_BARRIER);
	}

	//    if the number is between last_checked and last_executed, we results now
	if (runto_seq_num > m_ideal_first_predictable &&
			m_ideal_last_predictable <= runto_seq_num ) {

		DEBUG_OUT("  searching for predictable\n");

		// find the index in the hash table and return the result
		flow_inst_t *node = m_cfg_index[runto_seq_num];
		if (node == NULL) {
			ERROR_OUT( "error: idealRunTo: unable to find node. (%lld)\n",
					runto_seq_num );
			return (PSEQ_FETCH_BARRIER);
		} else {
			// return found!
			return (PSEQ_FETCH_READY);
		}
	}

	// if the number is ahead of last_predictable, run until it is predictable
	while ( m_ideal_last_predictable < runto_seq_num ) {
		// fetch an instruction
		//#ifdef DEBUG_IDEAL
		DEBUG_OUT("pseq_t: predictable %lld runto %lld. running.\n",
				m_ideal_last_predictable < runto_seq_num );
		printPC( &m_inorder.at );
		//#endif
		lookupInstruction(&m_inorder.at, &s_instr, &fetchPhysicalPC, MAX_FETCH+1);

		// allocate a new node in the control flow graph
		// connects it to its sources and destination dependencies
		flow_inst_t *node = new flow_inst_t( s_instr, this,
				&m_inorder,
				m_mem_deps );

		if ( s_instr == m_fetch_itlbmiss ) {
			// take the in-order trap
			node->setTrapType( Trap_Fast_Instruction_Access_MMU_Miss );

			// NOTE: After this ITLB miss is completed Simics's MMU state
			//   may still not contain the translation.
		} else {
			// "Execute them!" "Bogus!" (name the movie this is from)
			node->readMemory();
			node->execute();
		}

		/*
		 * Task: retire the instruction, unless it causes a trap.
		 */
		// The traptype and window index of the retiring instr (if any)
		trap_type_t traptype = node->getTrapType();
		if ( traptype == Trap_NoTrap ) {
			// Retire the instruction
			node->advancePC();

		} else if ( traptype == Trap_Use_Functional ||
				traptype == Trap_Unimplemented ) {
			// stall the in-order model until the processor advances
			node->advancePC();

			// return that we were not able to execute this instruction
			// stall the in-order model until the processor advances
			STAT_INC( m_inorder_partial_success );
			return (PSEQ_FETCH_BARRIER);
		} else {
			// functionally take the trap
			takeTrap( (dynamic_inst_t *) node, node->getActualTarget(), false );
		}

		// advance PC, update where we are fetching / executing from
		m_inorder.at = *node->getActualTarget();
		//#if DEBUG_IDEAL
		DEBUG_OUT("pseq_t: stepInorder: retirement\n");
		node->print();
		printPC( &m_inorder.at );
		//#endif

		// increment the last_predictable index
		m_ideal_last_predictable++;
	}

	return (PSEQ_FETCH_READY);
}

//**************************************************************************
pseq_fetch_status_t pseq_t::oraclePredict( dynamic_inst_t *d,
		bool *taken, 
		abstract_pc_t *inorder_at )
{
	pseq_fetch_status_t status = PSEQ_FETCH_READY;
	uint64 seq_num = d->getSequenceNumber();

	DEBUG_OUT("oracle predict. called\n");
	// look up the sequence number in the ideal table
	if (m_cfg_index.find( seq_num ) == m_cfg_index.end()) {
		// not found: run inorder processor until it is found
		status = idealRunTo( seq_num );
		if (status != PSEQ_FETCH_READY) {
			// if not able to execute to this instruction, return
			status = PSEQ_FETCH_SQUASH;
		}
	}

	// query the mapping to get the flow instruction
	if (m_cfg_index.find( seq_num ) != m_cfg_index.end()) {
		flow_inst_t *flow_inst  = m_cfg_index[seq_num];

		// if the instruction is found OK, read / compare the instruction
		if ( flow_inst->getStaticInst()->getInst() ==
				d->getStaticInst()->getInst() ) {
			// make the prediction based on flow instruction
			DEBUG_OUT("oracle predict. making prediction!\n");
			*taken = flow_inst->getTaken();
			*inorder_at = *flow_inst->getActualTarget();
		} else {
			// not the same instruction
			status = PSEQ_FETCH_SQUASH;
		}
	}

	// return successful (ready)
	DEBUG_OUT("oracle predict. returns: %d\n", status);
	m_hist_ideal_coverage[status]++;
	return (status);
}

//**************************************************************************
void pseq_t::idealCheckTo( int64 seq_num )
{
	// if m_ideal_last_checked is less than seq_num, check each instruction
	// until synchronized
	DEBUG_OUT("idealCheckTo. checking inorder processor: %lld\n", seq_num);

#if 0
	while ( m_ideal_last_checked < seq_num ) {

		enum i_opcode op = (enum i_opcode) s_instr->getOpcode();
		m_ideal_opstat.seenOp( op );

		// here we actually need to check a value from the _past_

		check_result_t result;
		result.verbose = true;
		checkAllState( &result, &m_inorder, true );

#if defined( DEBUG_FUNCTIONALITY ) && defined( DEBUG_IDEAL )
		if (!result.perfect_check) {
			out_info("perfect check fails\n");
			node->print();
		}
#endif

		if (result.perfect_check) {
#ifdef PIPELINE_VIS
			out_log("dy %lld success %s\n", node->getID(), decode_opcode( op ));
#endif
			m_ideal_opstat.successOp( op );
		} else {
			node->print();
#ifdef PIPELINE_VIS
			out_log("dy %lld fail %s\n", node->getID(), decode_opcode( op ) );
#endif
		}
	}
#endif
}

//**************************************************************************
void pseq_t::stepInorder( void )
{
	static_inst_t   *s_instr = NULL;
	pa_t             fetchPhysicalPC;

#if 0
	if (m_ideal_status != PSEQ_FETCH_READY) {
		return false;
	}
#endif

	// fetch an instruction
#ifdef DEBUG_IDEAL
	DEBUG_OUT("pseq_t: stepInorder: fetch\n");
	printPC( &m_inorder.at );
#endif
	lookupInstruction(&m_inorder.at, &s_instr, &fetchPhysicalPC,MAX_FETCH+1);

	enum i_opcode op = (enum i_opcode) s_instr->getOpcode();
	m_ideal_opstat.seenOp( op );

	// allocate a new node in the control flow graph
	// connects it to its sources and destination dependencies
	flow_inst_t *node = new flow_inst_t( s_instr, this,
			&m_inorder, m_mem_deps );

	if ( s_instr == m_fetch_itlbmiss ) {
		// take the in-order trap
		node->setTrapType( Trap_Fast_Instruction_Access_MMU_Miss );
		// stall the in-order model until the processor advances
		//m_ideal_status = PSEQ_FETCH_ITLBMISS;
	} else {
		// "Execute them!" "Bogus!" (name the movie this is from)
		node->readMemory();
		node->execute();
	}

	/*
	 * Task: retire the instruction, unless it causes a trap.
	 */

	// The traptype and window index of the retiring instr (if any)
	trap_type_t traptype = node->getTrapType();
	if ( traptype == Trap_NoTrap ) {
		// Retire the instruction
		node->advancePC();

	} else if ( traptype == Trap_Use_Functional ||
			traptype == Trap_Unimplemented ) {
		// stall the in-order model until the processor advances
		node->advancePC();

	}

	// print out the PC, instruction that is retiring
#if DEBUG_IDEAL
	DEBUG_OUT("pseq_t: stepInorder: retirement\n");
	node->print();
	printPC( node->getActualTarget() );
#endif

	// check this instruction
	m_unchecked_retires = 1;
	advanceSimics();
	STAT_INC( m_ideal_retire_count );

	if (m_posted_interrupt == Trap_NoTrap) {
		if ( !( traptype == Trap_NoTrap ||
					traptype == Trap_Use_Functional ||
					traptype == Trap_Unimplemented) ) {
			// if we should have taken a trap before, functionally take it now
			takeTrap( (dynamic_inst_t *) node, &m_inorder.at, false );
			*node->getActualTarget() = m_inorder.at;
		}
	} else {
		// trap posted, get it then reset
		node->setTrapType( m_posted_interrupt );
		m_posted_interrupt = Trap_NoTrap;

		// take the trap here (if any), as execute may have modified
		takeTrap( (dynamic_inst_t *) node, &m_inorder.at, false );
		*node->getActualTarget() = m_inorder.at;
	}

	if (traptype == Trap_Use_Functional) {
		// read the result register out of simics
		updateInstructionState( node, true );
		//TODO - Here we are.
	}

	// update/advance the PC of this instruction
	m_inorder.at = *node->getActualTarget();

	// check all state
	check_result_t result;

	checkAllState( &result, &m_inorder, true );

#if defined( DEBUG_FUNCTIONALITY ) && defined( DEBUG_IDEAL )
	if (!result.perfect_check) {
		out_info("perfect check fails\n");
		node->print();
	}
#endif

	if (result.perfect_check) {
#ifdef PIPELINE_VIS
		out_log("dy %lld success %s\n", node->getID(), decode_opcode( op ));
#endif
		m_ideal_opstat.successOp( op );
	} else {
		node->print();
#ifdef PIPELINE_VIS
		out_log("dy %lld fail %s\n", node->getID(), decode_opcode( op ) );
#endif
	}

	// add this flow instruction to the map (id->instruction)
	m_cfg_index[node->getID()] = node;

	// CM FIX: use flow instruction ref counting to free the node appropriately
	//         (e.g. after retirement count has passed it & no one depends onit)
	// delete node;
}

//**************************************************************************
void pseq_t::warmupCache( memory_transaction_t *mem_op )
{
	cache_t *l1_cache;
	pa_t     physaddr = mem_op->s.physical_address;

	// case instruction or data cache  
	if ( mem_op->s.type == Sim_Trans_Instr_Fetch ) {
		l1_cache = l1_inst_cache;
	} else {
		l1_cache = l1_data_cache;
	}

	// read or write to the 1l caches, r/w the l2's on a miss
	if ( SIMICS_mem_op_is_write( &mem_op->s ) ) {
		if (! l1_cache->Write( physaddr, NULL ) )
			l2_cache->Write( physaddr, NULL );
	} else {
		if (! l1_cache->Read( physaddr, NULL, true, NULL ) )
			l2_cache->Read( physaddr, NULL, true, NULL );
	}

	localCycleIncrement();

	// Warm up the instruction or data cache, depending on type of reference
	if ( SIMICS_mem_op_is_instruction( &mem_op->s ) ) {
		l1_inst_cache->Warmup( mem_op->s.physical_address);
	} else {
		// mem_op->s.type is data Load or Store
		l1_data_cache->Warmup( mem_op->s.physical_address);
	}
	l2_cache->Warmup( mem_op->s.physical_address);
}

/** log the number of logical renames while we've been in flight */
//**************************************************************************
void pseq_t::logLogicalRenameCount( rid_type_t rid, half_t logical, 
		uint32 count )
{
#ifdef RENAME_EXPERIMENT
	uint32  rindex;
	switch (rid) {
		case RID_INT:
		case RID_INT_GLOBAL:
			rindex = RID_INT;
			break;
		case RID_SINGLE:
		case RID_DOUBLE:
		case RID_QUAD:
			rindex = RID_SINGLE;
			break;
		case RID_CC:
			rindex = RID_CC;
			break;
		case RID_CONTROL:
			// ignore control registers as they are not renamed currently.
			return;
		default:
			ERROR_OUT("unknown logical register type: %d\n", rid);
			return;
	}

	if ( count >= PSEQ_REG_USE_HIST) {
		ERROR_OUT("logicalrenamecount: trimming %d\n", count);
		count = PSEQ_REG_USE_HIST - 1;
	}
	m_reg_use[rindex][logical][count]++;
#endif
}

/** close the trace */
//**************************************************************************
void pseq_t::startTime( void )
{
	m_simulation_timer.startTimer();
}

/** close the trace */
//**************************************************************************
void pseq_t::stopTime( void )
{
	m_simulation_timer.stopTimer();
	m_simulation_timer.accumulateTime();
}

/*------------------------------------------------------------------------*/
/* Simics Interfaces                                                      */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pseq_t::installInterfaces( void )
{
	if (CONFIG_IN_SIMICS) {
		ASSERT( m_mmu_access != NULL );
		char mmuname[200];
		pstate_t::getMMUName( m_id, mmuname, 200 );
		conf_object_t *mmu = SIMICS_get_object( mmuname );
		if (mmu == NULL) {
			ERROR_OUT("error: unable to locate object: %s\n", mmuname);
			SIM_HALT;
		}

		// FIXFIXFIX
		// Currently the MMU ASI is broken (as of 1.2.5 and 1.4.4).
		m_mmu_asi = NULL;
#if 0  
		m_mmu_asi = (mmu_interface_t *) SIMICS_get_interface( mmu, "asi" );
		if (m_mmu_asi == NULL) {
			ERROR_OUT("error: object does not implement \"asi\" interface: %s\n",
					mmuname);
			ERROR_OUT("     : you need to compile the spitfire-mmu object from the multifacet tree\n");
			SYSTEM_EXIT;
		}
#endif
		hfa_checkerr("install Interfaces");

		// get the primary context register of the sfmmu
		attr_value_t   index;
		index.kind       = Sim_Val_String;
		index.u.string   = "ctxt_primary";
		attr_value_t   ctxt_reg = SIMICS_get_attribute_idx(mmu, "registers", &index);
		ireg_t         ctxt_id  = 0;
		if (ctxt_reg.kind == Sim_Val_Integer) {
			ctxt_id = ctxt_reg.u.integer;
		} else {
			ERROR_OUT("error: unable to read attribute on %s: registers[]\n",
					mmuname);
			SYSTEM_EXIT;
		}

		hap_type_t mmu_register_write = SIMICS_hap_get_number("MMU_Register_Write");
#ifndef SIMICS_3_0
		// verify the type safety of this call
		callback_arguments_t args      = SIMICS_get_hap_arguments( mmu_register_write, 0 );
		const char          *paramlist = SIMICS_get_callback_argument_string( args );
		if (strcmp(paramlist, "nocII" )) {
			ERROR_OUT("error: system_t::installHapHandlers: expect hap to take parameters %s. Current simics executable takes: %s\n",
					"nocII", paramlist );
			SYSTEM_EXIT;
		}
#endif

		m_mmu_haphandle = SIMICS_hap_add_callback_index( "MMU_Register_Write", 
				(obj_hap_func_t) &pseq_mmu_reg_handler, 
				(void *) this, ctxt_id );

		if (m_mmu_haphandle <= 0) {
			ERROR_OUT("error: installHapHandlers: mmu hap handle <= 0\n");
			SYSTEM_EXIT;
		}

		// copy all of simics's state into the pstate data structure
		M_PSTATE->checkpointState();

		// get the current context
		m_primary_ctx = M_PSTATE->getContext();
	}

	// copy all simics's state into the in-order register files
	allocateIdealState();

	// reset the timers using the current instruction counts
	m_overall_timer->reset();
	m_thread_timer->reset();

	// set the initial PC, nPC pair
	m_fetch_at->pc  = M_PSTATE->getControl(CONTROL_PC);
	m_fetch_at->npc = M_PSTATE->getControl(CONTROL_NPC);
	m_fetch_at->tl  = M_PSTATE->getControl(CONTROL_TL);
	m_fetch_at->pstate = M_PSTATE->getControl( CONTROL_PSTATE );
	m_fetch_at->cwp = M_PSTATE->getControl(CONTROL_CWP);

	if(getID() == 1 && getLocalCycle() >= 17500 && getLocalCycle() <= 17700) 
		DEBUG_OUT(" initialInterface: %d\n", m_fetch_at->cwp);

	m_fetch_at->gset = pstate_t::getGlobalSet( m_fetch_at->pstate );
	if ( ISADDRESS32(m_fetch_at->pstate) ) {
		//assert(0);
		m_fetch_at->pc  = AM64( m_fetch_at->pc );
		m_fetch_at->npc = AM64( m_fetch_at->npc );
	}
	m_ooo.at     = *m_fetch_at;
	m_inorder.at = *m_fetch_at;

	// perform address translation (warm the itlb mini-cache)
	if (CONFIG_IN_SIMICS) {
		uint32 next_instr;
		getInstr( m_fetch_at->pc, m_fetch_at->tl, m_fetch_at->pstate,
				&m_itlb_physical_address, &next_instr );
		m_itlb_logical_address  = m_fetch_at->pc & PSEQ_OS_PAGE_MASK;
		m_itlb_physical_address = m_itlb_physical_address & PSEQ_OS_PAGE_MASK;
	}
	// out_info("[%d]\tPC 0x%0llx\tNPC 0x%0llx\tctx 0x%x\n",
	//         m_id, m_fetch_at->pc, m_fetch_at->npc, m_primary_ctx );

	// copy from the pstate structure into our data structures
	check_result_t  result;

	checkAllState( &result, &m_ooo, true );
	checkAllState( &result, &m_inorder, true );
}

//**************************************************************************
void pseq_t::removeInterfaces( void )
{
	if (CONFIG_IN_SIMICS) {
		hap_type_t mmu_register_write = SIMICS_hap_get_number("MMU_Register_Write");
		if (mmu_register_write == 0) {
			ERROR_OUT("error: removeHapHandlers: system unable to get mmu hap event\n");
		}
		SIMICS_hap_delete_callback_id( "MMU_Register_Write", m_mmu_haphandle );
		hfa_checkerr("check: removeHapHandlers");
	}
}

//***************************************************************************
void pseq_t::fastforwardSimics( void )
{
	bool   hit_retry = false;
	uint32 count     = 0;
	bool   found;
	uint32 next_instr;
	ireg_t pc;
	pa_t   physical_pc;
	ireg_t traplevel;
	ireg_t pstate;

	// PSEQ_MAX_FF_LENGTH == arbitrary limit on how far this should go on
	while (hit_retry == false && count < PSEQ_MAX_FF_LENGTH - 3) {
		advanceSimics();
		count++;

		// decode the current instruction
		pc        = M_PSTATE->getControl( CONTROL_PC );
		traplevel = M_PSTATE->getControl( CONTROL_TL );
		pstate    = M_PSTATE->getControl( CONTROL_PSTATE );  

		if ( ISADDRESS32(pstate) ) {
			pc = AM64( pc );
		}

		// out_info("FF: 0x%0llx ", pc );
		found = getInstr( pc, traplevel, pstate,
				&physical_pc, &next_instr );
		if (found) {
			static_inst_t decoded_instr( physical_pc, next_instr );
			decoded_instr.setFlag( SI_TOFREE, true );
			if (decoded_instr.getOpcode() == i_retry) {
				hit_retry = true;
			}
		}
	}

	// step past the retry instruction itself
	advanceSimics();
	count++;
	if ( count < PSEQ_MAX_FF_LENGTH ) {
		m_hist_ff_length[count]++;
	}
	/* out_info("fastforward: ending after %d instructions (%d)\n",
	   count, hit_retry );
	 */
}

//***************************************************************************
bool pseq_t::advanceSimics( void )
{
	bool read_ok = true;
	if (CONFIG_IN_SIMICS == true) {
		// step simics some number of steps
		STAT_INC( m_stat_continue_calls );

		m_retirement_timer.startTimer();
		if (CONFIG_MULTIPROCESSOR) {
			M_PSTATE->simcontinue( m_unchecked_retires );
		} else {
			SIMICS_continue( m_unchecked_retires );
		}
		if ( SIMICS_get_pending_exception() ) {
			// clear the exception
			SIMICS_clear_exception();
			DEBUG_OUT( "retire instruction error message: %s\n",
					SIMICS_last_error() );
			// Sometimes RED_state ends up here. In that case, say
			// that it is a detection
			string error = string(SIMICS_last_error()) ;
			if( error.find("RED_state", 0) != string::npos ) {
				DEBUG_OUT("Error is red state\n") ;
				OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
						GET_RET_INST(),
						pstate_t::trap_num_menomic(Trap_Red_State_Exception), 
						Trap_Red_State_Exception,0x0bad0bad,
						1, 0x0bad0bad,
						MAXTL-1, getID());
			} else {
				DEBUG_OUT("ERror not found\n") ;
			}
			DEBUG_OUT(" ftl %lld ",M_PSTATE->getControl(CONTROL_TL));
			DEBUG_OUT("ps 0x%llx\n",M_PSTATE->getControl(CONTROL_PSTATE));

			OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
					GET_RET_INST(),
					pstate_t::trap_num_menomic(Trap_Red_State_Exception), 
					Trap_Red_State_Exception,0x0bad0bad,
					1, 0x0bad0bad,
					MAXTL-1, getID());

			system_t::inst->breakSimulation();
		}

		// end retirement check timing
		m_retirement_timer.stopTimer();
		m_retirement_timer.accumulateTime();
	} else {
		// variables for reading the trace
		la_t  program_counter;   // the PC of the instruction
		pa_t  physical_pc;       // the address of the instruction
		unsigned int instr;      // the correct instruction at a given PC

		// read the next trace step
		for (uint32 i = 0; i < m_unchecked_retires; i++) {
			bool success = readTraceStep( program_counter, physical_pc, instr );
			if (!success) {
				read_ok = false;
			}


			// If the instruction is different update the ipage map
			if ( success &&
					m_unchecked_instr[i]->getStaticInst()->getInst() != instr ) {
				// INSTRUCTION_OVERWRITE
				/* out_info("overwriting vpc:0x%0llx pc:0x%0llx i:0x%0x new:0x%0x\n",
				   program_counter, physical_pc, 
				   m_unchecked_instr[i]->getStaticInst()->getInst(), instr );
				 */
				// overwrite the existing instruction in the ipagemap

				abstract_pc_t  pc;
				pc.pc = program_counter;
				pc.tl = m_control_arf->getRetireRF()->getInt( CONTROL_TL );
				insertInstruction( physical_pc, instr );
			}
		}
	}
	return (read_ok);
}

//**************************************************************************
void pseq_t::contextSwitch( context_id_t new_context )
{
	// CTXT_SWITCH
	// out_info("seq: context: %u\n", new_context);
	if (new_context != m_primary_ctx) {

		// if we're taking a trace, emit the context switch...
		if (m_tracefp) {
			m_tracefp->writeContextSwitch( new_context );
		}

		if (m_branch_trace) {
			m_branch_trace->writeContextSwitch( new_context );
		}

		// update the primary context
		m_primary_ctx = new_context;
	} 
}

/// remove memory timing interface
//***************************************************************************
void pseq_t::completedRequest( pa_t physicalAddr )
{
	m_ruby_cache->complete( physicalAddr );
}

//**************************************************************************
bool pseq_t::readPhysicalMemory( pa_t phys_addr, int size, ireg_t *result_reg )
{
	// read memory ...
	if (PSEQ_MAX_UNCHECKED > 1) {
		memory_inst_t *uncheckedRetire = uncheckedValueForward( phys_addr, size );
		if ( uncheckedRetire != NULL ) {
			if ( uncheckedRetire->isDataValid() ) {
				// try to forward the value from this instruction
				byte_t  length = uncheckedRetire->getAccessSize();

				// forward value from store to load, by copying byte-by-byte

				// get the proper offset into the data
				int index1 = (int) (uncheckedRetire->getPhysicalAddress() - phys_addr);
				if ( index1 < 0 ) {
					ERROR_OUT( "readPhysicalMemory(): warning: negative offset: %d\n",
							index1);
					return false;
				}

				char *p1 = (char *) uncheckedRetire->getData();
				char *p2 = (char *) result_reg;
				// out_info("copying: size %d, index1 %d\n", size, index1);
				while ( size > 0 ) {
					if ( index1 >= length ) {
						ERROR_OUT("readPhysicalMemory(): warning: array bounds read sz=%d (%d on %d).\n", size, index1, length);
						return (false);
					}
					*p2++ = p1[index1++];
					size--;
				}
				return (true);
			} else {
				// since we know memory is written, but we're not sure of the value
				// just give up on this load.
				DEBUG_OUT("%d:data not valid\n",m_id);
				return (false);
			}
		}
	}
	return ( M_PSTATE->readPhysicalMemory( phys_addr, size, result_reg ) );
}

bool pseq_t::writePhysicalMemory( pa_t phys_addr, int size, ireg_t *result_reg )
{
	return ( M_PSTATE->writeMemory( phys_addr, size, result_reg ) ) ;
}

bool pseq_t::translateInstruction( ireg_t log_addr, int size, pa_t &phys_addr)
{
	return M_PSTATE->translateInstruction(log_addr, size, phys_addr);
}

//**************************************************************************
trap_type_t pseq_t::mmuAccess( la_t address, uint16 asi,
		uint16 accessSize, OpalMemop_t op,
		uint16 pstate, uint16 tl, pa_t *physAddress)
{
	memory_transaction_t mem_op;
	exception_type_t e = Sim_PE_No_Exception;

	memset( &mem_op, 0, sizeof(mem_op) );
	mem_op.s.logical_address = address;
	mem_op.s.physical_address = 0;
	mem_op.s.size = accessSize;

	switch (op) {
		case OPAL_LOAD:
			mem_op.s.type = Sim_Trans_Load;
			break;
		case OPAL_STORE:
			mem_op.s.type = Sim_Trans_Store;
			break;
		case OPAL_IFETCH:
			mem_op.s.type = Sim_Trans_Instr_Fetch;
			break;
		case OPAL_ATOMIC:
			mem_op.s.type = Sim_Trans_Store;
			break;
		default:
			ERROR_OUT("mmuAccess: Wrong type of memory access!");
	}

	mem_op.s.inquiry        = 1;
	mem_op.s.inverse_endian = 0;
	mem_op.s.ini_type    = Sim_Initiator_CPU;
	mem_op.s.ini_ptr     = M_PSTATE->getSimicsProcessor();

	/*
	 * Task: get the current pstate, am, priv, cle bits
	 */

	mem_op.priv             = (pstate >> 2) & 0x1;
	mem_op.s.inverse_endian = (pstate >> 9) & 0x1;
	mem_op.address_space    = asi;
	int address_mask        = (pstate >> 3) & 0x1;
#ifdef CHECK_ADDR_MASK
	if (mem_op.priv)
		DEBUG_OUT("addr mask is %d\n",address_mask);
#endif
	if (address_mask) {
		//assert(0);
		mem_op.s.logical_address = AM64( mem_op.s.logical_address );
	}

	// The ASI must be set at this point (not uninit or IMPLICIT)
	ASSERT( asi != (uint16) -1 && asi != 0x100 );
	ASSERT( m_mmu_access != NULL );

	// inside simics, call interface to query physical address
	// Pradeep - FIXME
	//DEBUG_OUT( "Simics MMU Access with PC : %0llx\n", mem_op.s.logical_address ) ;

	e = (*m_mmu_access->logical_to_physical)( M_PSTATE->getSimicsMMU(),&mem_op );
	if ( e == Sim_PE_No_Exception ) {
		*physAddress = mem_op.s.physical_address;
		//DEBUG_OUT( "No trap. Physical address = %0llx\n", mem_op.s.physical_address ) ;
		return ( Trap_NoTrap );
	}
	if ( (int) e > (int) Sim_PE_No_Exception ) {
#ifdef DEBUG_LSQ
		// else: an exception occured during translation -- cause an exception
		// check to see if simics is returning a real or pseudo exception...
		out_info("mmuAccess: pseudo exception: addr 0x%0llx pa 0x%0llx asi 0x%0x err=0x%x\n",
				address, mem_op.s.physical_address, asi, (int) e);
#endif
		return Trap_NoTrap;
	}

#ifdef DEBUG_LSQ
	// out_info("mmuAccess: exception=0x%0x: va:0x%0llx ma:0x%0llx %c %c pa:0x%0llx pstate:0x%0x tl:0x%x asi:0x%0x result:0x%0x\n",
	// (int) e, address, mem_op.s.logical_address,
	// (mem_op.s.type == Sim_Trans_Load || mem_op.s.type == Sim_Trans_Store) ? 'D' : 'I',
	// (op == OPAL_STORE) ? 'W' : 'R',
	// mem_op.s.physical_address, pstate, tl, asi, e);
	out_info( "mmuAccess: exception=0x%0x ", (int) e ) ;
	out_info("va:0x%0llx ", mem_op.s.logical_address ) ;
	out_info("pa:0x%0llx\n", mem_op.s.physical_address ) ;
#endif

	return ((trap_type_t) e);
}

//**************************************************************************
bool pseq_t::getInstr( la_t cur_pc, int32 traplevel, int32 pstate,
		pa_t *physical_pc, unsigned int *next_instr_p )
{

#ifdef SW_ROLLBACK
	if (m_op_mode==RECOVERING) {
		*next_instr_p = getNextRecoveryInstr();
		*physical_pc = cur_pc;

		return true;
	}
#endif
	// translate from logical to physical memory
	uint16 asi;

	if (traplevel == 0) {
		asi = ASI_PRIMARY;
	} else {
		asi = ASI_NUCLEUS;
	}
	trap_type_t t = mmuAccess( cur_pc, asi, 4, OPAL_IFETCH, pstate, traplevel,
			physical_pc );

	if (t == Trap_NoTrap) {
		//DEBUG_OUT( "Reading physical memory %0llx\n", *physical_pc ) ;
		*next_instr_p = SIMICS_read_phys_memory( (processor_t *) M_PSTATE->getSimicsProcessor(),
				*physical_pc, 4 );
	} else {
		// emit a special token at this point
		int isexcept = SIMICS_get_pending_exception();
		if ( isexcept != 0 ) {
			sim_exception_t except_code = SIMICS_clear_exception();
			// ignore Memory exception at this point -- no translation
			if (except_code != SimExc_Memory) {
				ERROR_OUT( "getInstr: Exception error message: %s\n",
						SIMICS_last_error() );
			}
		}
		*next_instr_p = STATIC_INSTR_MOP;
		return (false);
	}
	// hfa_checkerr("getInstr: after SIMICS_read_phys_memory");
	return (true);
}

#ifdef SW_ROLLBACK
// Injecting instruction for recovery
unsigned int pseq_t::getNextRecoveryInstr()
{
	if (recovery_instr_ptr<recovery_instr_mem_size) 
		return recovery_instr_mem[recovery_instr_ptr++];
	else
		return (unsigned int) 0x4<<22; //This is nop
}
#endif


// Read value from the good machine/functional simulator
ireg_t pseq_t::readFuncSimValue(reg_id_t&rid) 
{
	switch (rid.getRtype()) {
		case RID_NONE:                // No register
			return 0;

		case RID_INT:                 // integer identifier (windowed)
			return M_PSTATE->getIntWp( rid.getVanilla(), rid.getVanillaState() );

		case RID_INT_GLOBAL:          // integer identifier (global)
			return M_PSTATE->getIntGlobal( rid.getVanilla(), rid.getVanillaState() );

		case RID_CC:                  // condition code identifier
			{
				ireg_t simreg;
				int32 reg=rid.getVanilla();
				if (reg == REG_CC_CCR) {
					simreg  = M_PSTATE->getControl( CONTROL_CCR );
				} else {
					ireg_t fsr = M_PSTATE->getControl( CONTROL_FSR );    
					if (reg == REG_CC_FCC0)
						simreg  = maskBits64( fsr, 11, 10 );
					else if (reg == REG_CC_FCC1)
						simreg  = maskBits64( fsr, 33, 32 );
					else if (reg == REG_CC_FCC2)
						simreg  = maskBits64( fsr, 35, 34 );
					else if (reg == REG_CC_FCC3)
						simreg  = maskBits64( fsr, 37, 36 );
					else {
						DEBUG_OUT( "arf_cc: check fails: unknown rid %d\n", reg);
						return 0;
					}
				}
				return simreg;
			}

		case RID_CONTROL:             // control register
			return M_PSTATE->getControl( rid.getVanilla() );


		case RID_SINGLE:              // floating point: single precision
			{
				half_t reg = rid.getVanilla();
				// get the double value for the register (rounded to the nearest 2)
				freg_t fsimreg = M_PSTATE->getDouble( (reg / 2)*2 );
				ireg_t simreg32;
				uint32 *int_ptr = (uint32 *) &fsimreg;

				// ENDIAN_MATTERS
				if (ENDIAN_MATCHES) {
					if ( reg % 2 == 1 ) {
						// check target upper 32 bits
						simreg32 = int_ptr[1];
					} else {
						// check target lower 32 bits
						simreg32 = int_ptr[0];
					}
				} else {
					if ( reg % 2 == 1 ) {
						// check target upper 32 bits
						simreg32 = int_ptr[0];
					} else {
						// check target lower 32 bits
						simreg32 = int_ptr[1];
					}
				}
				return simreg32;
			}

		case RID_DOUBLE:              // floating point: double precision
			return M_PSTATE->getDouble( rid.getVanilla() );

		case RID_QUAD:                // floating point: quad   precision
		case RID_CONTAINER:           // container class
			return 0;
		default:
			ERROR_OUT("error: reg_id_t: unidentified rid type: %d\n", rid.getRtype());
#ifdef LL_DECODER
			bool curr_priv = fault_stat->getRetPriv() ;
			int trap_level = m_control_arf->getRetireRF()->getInt( CONTROL_TL );
			OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
					m_local_sequence_number,
					pstate_t::trap_num_menomic((trap_type_t)0x10), 
					0x10,0x0,
					curr_priv, 0,
					(uint16)trap_level, m_id);
			HALT_SIMULATION ;
#else // LL_DECODER
			SIM_HALT;
#endif // LL_DECODER 
	}
	return 0;

}

/**
 * read an instruction from a given current program counter
 */
//**************************************************************************
bool pseq_t::getInstr( la_t cur_pc, 
		pa_t *physical_pc, unsigned int *next_instr_p )
{
	// implicitly get the trap level, and pstate from simics
	ireg_t traplevel = M_PSTATE->getControl( CONTROL_TL );
	ireg_t pstate    = M_PSTATE->getControl( CONTROL_PSTATE );
	return (getInstr( cur_pc, traplevel, pstate, physical_pc, next_instr_p ));
}

//**************************************************************************
void pseq_t::postException( uint32 exception )
{
	m_simtrapstat[exception]++;
	// if this exception is one that opal can't model, set the interrupt flag
	//    The interrupt flag is checked as instructions retire
	if ( exception >= Trap_Interrupt_Level_1 &&
			exception <= Trap_Interrupt_Vector ) {
#ifdef PIPELINE_VIS
		if (system_t::inst->isSimulating()) {
			out_info("postedException: exception 0x%0x\n", exception );
		}
#endif
		m_posted_interrupt = (trap_type_t) exception;
	}
}

//**************************************************************************
int32 pseq_t::getPid( void )
{
	// now: "thread" is the virtual address address of the thread pointer--
	la_t thread = getCurrentThread();
	return getPid( thread );
}

//**************************************************************************
int32 pseq_t::getPid( la_t thread_p )
{
	// dereference virtual addresses plus offset (Solaris 9)
	la_t procp    = M_PSTATE->dereference( thread_p + 0x110, 8 );
	la_t pidp     = M_PSTATE->dereference( procp + 0xb0, 8 );
	int32 pid     = M_PSTATE->dereference( pidp + 0x4, 4 );

#if 0
	// Solaris 8
	la_t procp    = M_PSTATE->dereference( thread + 0x130, 8 );
	la_t pidp     = M_PSTATE->dereference( procp + 0xb0, 8 );
	int32 pid     = M_PSTATE->dereference( pidp + 0x4, 4 );
#endif

	// can find the hardware context too...
#if 0    
	la_t as   = M_PSTATE->dereference( procp + 0x8, 8 );
	la_t hat  = M_PSTATE->dereference( as + 0x10, 8 );
	la_t ctx  = M_PSTATE->dereference( hat + 0x2e, 2 );
#endif
	return pid;
}

//**************************************************************************
la_t  pseq_t::getCurrentThread( void )
{
	// previously: read thread from cpu_list offset
	// la_t   thread   = dereference( cpu_list + 0x10, 8 );

	// now: read thread pointer directly from physical memory
	la_t thread = 0;
	bool success = M_PSTATE->readPhysicalMemory( m_thread_physical_address,
			8, (ireg_t *) &thread );
	if (!success) {
		out_info( "getCurrentThread: read physical memory failed: 0x%0llx\n",
				thread );
	}
	return thread;
}

//**************************************************************************
void pseq_t::setThreadPhysAddress( la_t thread_phys_addr )
{
	m_thread_physical_address = thread_phys_addr;
}

//**************************************************************************
la_t pseq_t::getThreadPhysAddress( void )
{
	return m_thread_physical_address;
}


/*------------------------------------------------------------------------*/
/* Configuration Interfaces                                               */
/*------------------------------------------------------------------------*/

/// register checkpoint interfaces
//**************************************************************************
void  pseq_t::registerCheckpoint( void )
{
	/* caches: */
	if(!CONFIG_WITH_RUBY) {
		l1_data_cache->registerCheckpoint( m_conf );
		l1_inst_cache->registerCheckpoint( m_conf );
		l2_cache->registerCheckpoint( m_conf );
	}

	/* branch predictors */
	if ( !strcmp( BRANCHPRED_TYPE, "YAGS" ) ) {
		((yags_t *) m_bpred)->registerCheckpoint( m_conf );
	}
	m_ipred->registerCheckpoint( m_conf );
}

/// begin writing new checkpoint
//**************************************************************************
void  pseq_t::writeCheckpoint( char *checkpointName )
{
	int rc = m_conf->writeConfiguration( checkpointName  );
	if ( rc < 0 )
		ERROR_OUT("error: unable to write checkpoint: %s\n", checkpointName);
}

/// open a checkpoint for reading
//**************************************************************************
void  pseq_t::readCheckpoint( char *checkpointName )
{
	int rc = m_conf->readConfiguration( checkpointName, OPAL_RELATIVE_PATH );
	if ( rc < 0 )
		ERROR_OUT("error: unable to read checkpoint: %s\n", checkpointName);
}

/*------------------------------------------------------------------------*/
/* Trace Interfaces                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pseq_t::openTrace( char *traceFileName )
{
	// read the primary context from the mmu
	int primary_ctx = M_PSTATE->getContext();
	m_tracefp = new tracefile_t( traceFileName, M_PSTATE->getProcessorState(),
			primary_ctx
#ifdef LXL_CTRL_MAP
			, M_PSTATE->getControlMap()
#endif
			);
	m_memtracefp = new memtrace_t( traceFileName, true );

	char   testname[FILEIO_MAX_FILENAME];
	int32 index=0;

	sprintf(m_tlbfilename, "%s%s", "tlb-", traceFileName);
	if (!findSeqFilename( m_tlbfilename, testname, &index )) {
		ERROR_OUT("error opening tlbtrace file: aborting!\n");
		exit(1);
	}
	strcpy( m_tlbfilename, testname );


}

//**************************************************************************
void pseq_t::writeTraceStep( void )
{
	bool         rc;             // Return Code
	la_t         curPC = 0;
	pa_t         physicalPC = 0;
	unsigned int instr;

	// read current PC
	curPC = M_PSTATE->getControl( CONTROL_PC );
	// read instruction from memory
	rc    = getInstr(curPC, &physicalPC, &instr);

	if (rc) {
		// read simics's state changes
		M_PSTATE->checkpointState();

		// write the PC and any difference between the current state and this state
		//timecheck  pc_step_t    newtime;
		//timecheck  newtime = M_PSTATE->getTime();
		//timecheck  m_local_icount = newtime - m_simics_time;
		m_tracefp->writeTraceStep( curPC, physicalPC, instr, m_local_icount );

		m_local_icount++;
		//timecheck  m_simics_time = newtime;

	} else {
		// else our last instruction read failed (e.g. probably an itlb miss)
		// ERROR_OUT("error: pc read failed\n");
	}
}


//**************************************************************************
void pseq_t::writeSkipTraceStep( void )
{
	bool         rc;             // Return Code
	la_t         curPC  = 0;
	la_t         curNPC = 0;
	pa_t         physicalPC = 0;
	pa_t         physicalNPC = 0;
	unsigned int instr;

	// read current PC
	curPC = M_PSTATE->getControl( CONTROL_PC );
	// read instruction from memory
	rc    = getInstr(curPC, &physicalPC, &instr);

	if (rc) {

		// just write the PC
		m_tracefp->writeTracePC( curPC, physicalPC, instr, m_local_icount );
		m_local_icount++;
	} else {
		// else our last instruction read failed (e.g. probably an itlb miss)
		// ERROR_OUT("error: pc read failed (1) 0x%0llx\n", curPC);
	}

	// read "next" instruction
	curNPC = M_PSTATE->getControl( CONTROL_NPC );
	rc     = getInstr(curNPC, &physicalNPC, &instr);

	if (rc) {
		// read simics's state changes
		M_PSTATE->checkpointState();

		// write the PC and any difference between the current state and this state
		m_tracefp->writeTraceStep( curNPC, physicalNPC, instr, m_local_icount );
		m_local_icount++;

	} else {
		// else our last instruction read failed (e.g. probably an itlb miss)
		// ERROR_OUT("error: pc read failed (2) 0x%0llx\n", curNPC);
	}
}

//**************************************************************************
void pseq_t::closeTrace( void )
{
	if (m_tracefp) {
		delete m_tracefp;
		m_tracefp = NULL;
	} 
	if (m_memtracefp) {
		delete m_memtracefp;
		m_memtracefp = NULL;
	}
	if (m_standalone_tlb) {
		if (CONFIG_IN_SIMICS) {
			// This means we are tracing, copy the iTLB
			FILE *fp = fopen( m_tlbfilename, "w" );
			char mmuname[200];
			pstate_t::getMMUName( m_id, mmuname, 200 );
			conf_object_t *mmu = SIMICS_get_object( mmuname );
			attr_value_t i2w_daccess,i2w_tagread,fa_daccess,fa_tagread;

			DEBUG_OUT("closeTrace: getting attribute\n");
			// LXL: Copy all mmu regs and tlb entries
			fa_daccess = SIM_get_attribute(mmu,"itlb_fa_daccess");
			fa_tagread = SIM_get_attribute(mmu,"itlb_fa_tagread");
			i2w_daccess = SIM_get_attribute(mmu,"itlb_2w_daccess");
			i2w_tagread = SIM_get_attribute(mmu,"itlb_2w_tagread");
			DEBUG_OUT("closeTrace: done getting attribute\n");

			if (fa_daccess.kind==Sim_Val_List) {
				int size = fa_daccess.u.list.size;
				for (int i=0; i<size; i++) {
					assert(fa_daccess.u.list.vector[i].kind==Sim_Val_Integer);
					m_standalone_tlb->addTranslation(fa_tagread.u.list.vector[i].u.integer&PSEQ_OS_PAGE_MASK,
							m_primary_ctx,
							(uint64)(fa_daccess.u.list.vector[i].u.integer<<21)>>34<<13,
							8*1024);
				}
				size = i2w_daccess.u.list.size;
				for (int i=0; i<size; i++) {
					assert(i2w_daccess.u.list.vector[i].kind==Sim_Val_Integer);
					m_standalone_tlb->addTranslation(i2w_tagread.u.list.vector[i].u.integer&PSEQ_OS_PAGE_MASK,
							m_primary_ctx,
							(uint64)(i2w_daccess.u.list.vector[i].u.integer<<21)>>34<<13,
							8*1024);
				}
				DEBUG_OUT("done add translation\n");
			}

			m_standalone_tlb->writeTranslation(fp);
			fclose(fp);
		}
	}
}

//**************************************************************************
void pseq_t::attachTrace( char *traceFileName, bool withImap )
{
	pa_t   physical_pc;       // the address of the instruction
	la_t   pc;
	uint32 instr;

	// must attach to memory trace first!
	if (withImap) {
		ASSERT( m_memtracefp != NULL );
	}

	m_tracefp = new tracefile_t( traceFileName, M_PSTATE->getProcessorState(),
			&m_primary_ctx 
#ifdef LXL_CTRL_MAP
			, M_PSTATE->getControlMap()
#endif
			);
	if (m_tracefp) {
		out_info("Initial primary context: 0x%0x\n", m_primary_ctx);

		// step the trace one cycle
		if (!readTraceStep( pc, physical_pc, instr )) {
			ERROR_OUT("pseq_t: error: unexpected end of trace.\n");
			ERROR_OUT("        Is the trace attached correctly?\n");
			SYSTEM_EXIT;
		}

		// set the initial PC, nPC pair
		m_fetch_at->pc  = M_PSTATE->getControl(CONTROL_PC);
		m_fetch_at->npc = M_PSTATE->getControl(CONTROL_NPC);
		m_fetch_at->tl  = M_PSTATE->getControl(CONTROL_TL);
		m_fetch_at->pstate = M_PSTATE->getControl(CONTROL_PSTATE);
		m_fetch_at->cwp = M_PSTATE->getControl(CONTROL_CWP);
		m_fetch_at->gset = pstate_t::getGlobalSet( m_fetch_at->pstate );
		out_info("PC 0x%0llx NPC 0x%0llx\n", m_fetch_at->pc, m_fetch_at->npc );


		if(getID() == 1 && getLocalCycle() >= 17500 && getLocalCycle() <= 17700)
			DEBUG_OUT(" attachTrace: %d\n", m_fetch_at->cwp);

		// write all simics's registers into our registers
		check_result_t result;
		checkAllState( &result, &m_ooo, true );

	} else { // end if tracing
		ERROR_OUT("error: unable to attach to trace %s.\n",
				traceFileName);
	}
}

//***************************************************************************
void pseq_t::attachMemTrace( char *traceFilename )
{
	m_memtracefp = new memtrace_t( traceFilename );
}

//***************************************************************************
void pseq_t::attachTLBTrace( char *traceFileName )
{
	m_standalone_tlb = new dtlb_t( TLB_NUM_ENTRIES, TLB_NUM_PAGE_SIZES,
			TLB_PAGE_SIZES );

	FILE *fp = fopen( traceFileName, "r" );
	if (fp == NULL) {
		ERROR_OUT("error: unable to attach to TLB file: %s\n",
				traceFileName);
		SYSTEM_EXIT;
	}
	DEBUG_OUT("Reading TLB translation information...\n");
	m_standalone_tlb->readTranslation( fp );
}

//**************************************************************************
void pseq_t::writeTraceMemop( transaction_t *trans )
{
	bool         rc;             // Return Code
	pa_t         physicalPC = 0;
	la_t         curPC = 0;
	unsigned int instr;

	// read current PC
	curPC = M_PSTATE->getControl( CONTROL_PC );
	// read instruction from memory
	rc    = getInstr(curPC, &physicalPC, &instr);

	if (m_memtracefp) {
		m_memtracefp->writeTransaction( curPC, instr, m_local_icount, trans );
	}
}

//**************************************************************************
void pseq_t::openBranchTrace( char *traceFileName )
{
	// read the primary context from the mmu
	int primary_ctx = M_PSTATE->getContext();
	m_branch_trace = new branchfile_t( traceFileName, primary_ctx );
}

//**************************************************************************
void pseq_t::writeBranchStep( void )
{
	abstract_pc_t at;
	pa_t          ppc;
	branch_type_t branch_type;
	static_inst_t *s_instr;

	at.pc      = M_PSTATE->getControl( CONTROL_PC );
	at.npc     = M_PSTATE->getControl( CONTROL_NPC );
	at.tl      = M_PSTATE->getControl( CONTROL_TL );
	at.pstate  = M_PSTATE->getControl( CONTROL_PSTATE );

	ASSERT( CONFIG_IN_SIMICS == true );

	// check last branch, if any
	m_branch_trace->checkLastBranch( at.pc, at.npc );

	lookupInstruction( &at, &s_instr, &ppc, MAX_FETCH+1 );
	if (s_instr != m_fetch_itlbmiss) {
		branch_type = s_instr->getBranchType();
		if (branch_type == BRANCH_COND ||
				branch_type == BRANCH_PCOND) {
			// write instruction
			m_branch_trace->writeBranch( at.pc, at.npc, at.tl, s_instr );
		}
	}
}

//**************************************************************************
bool pseq_t::writeBranchNextFile( void )
{
	return (m_branch_trace->writeNextFile());
}

//**************************************************************************
void pseq_t::closeBranchTrace( void )
{
	if (m_branch_trace) {
		delete m_branch_trace;
		m_branch_trace = NULL;
	} 
}

//**************************************************************************
bool pseq_t::readTraceStep( la_t &vpc, pa_t &ppc, unsigned int &instr )
{
	bool          success;
	uint32        context;
	uint32        localtime;

	if (m_tracefp) {
		context = m_primary_ctx;
		success = m_tracefp->readTraceStep( vpc, ppc, instr, localtime, context );

		// check to see if context has changed
		if (context != m_primary_ctx) {
			m_primary_ctx = context;
		}

		// update the local time
		m_local_icount = localtime;

		// If the instruction is different update the ipage map
		static_inst_t *s_instr = NULL;
		abstract_pc_t  pc;
		pc.pc   = vpc;
		pc.tl   = M_PSTATE->getControl(CONTROL_TL);
		bool inMap = queryInstruction( ppc, s_instr );
		if ( success && inMap &&
				s_instr->getInst() != instr ) {
			out_info("overwriting vpc:0x%0llx pc:0x%0llx i:0x%0x new:0x%0x\n",
					vpc, ppc, s_instr->getInst(), instr );

			// overwrite the existing instruction in the ipagemap
			insertInstruction( ppc, instr );
		}

		// scan in the memory trace
		if (m_memtracefp) {
#ifdef PIPELINE_VIS
			out_log("set localtime %d\n", localtime);
#endif
			m_memtracefp->advanceToTime( localtime );
		}

	} else {
		success = false;
	}
	return (success);
}

//***************************************************************************
bool pseq_t::queryInstruction( pa_t fetch_ppc, static_inst_t * &s_instr )
{
	return ( m_imap->queryInstr( fetch_ppc, s_instr ));
}

//***************************************************************************
static_inst_t *pseq_t::insertInstruction( pa_t fetch_ppc, unsigned int instr )
{
	uint16         status;
	static_inst_t *s_instr;

	// out_info("inserting:: 0x%0llx 0x%0x\n", a->pc, instr );

	// insert this instruction into the physical page map
	status = m_imap->insertInstr( fetch_ppc, instr, s_instr );

	// observe this static instruction
	system_t::inst->m_sys_stat->observeStaticInstruction( this, s_instr );

	return (s_instr);
}

//**************************************************************************
void pseq_t::invalidateInstruction( pa_t address )
{
	// invalidate the decoded instructions in this physical address
	m_imap->invalidateInstr( address );
}

//**************************************************************************
void pseq_t::writeInstructionTable( char *imapFileName )
{
	char     filename[FILEIO_MAX_FILENAME];

	// save the instruction map
	sprintf(filename, "%s.map", imapFileName );
	bool success = m_imap->saveFile( filename );
	if (!success) {
		ERROR_OUT("sequencer: error writing the imap table: %s\n", filename);
	}
}

//**************************************************************************
bool pseq_t::readInstructionTable( char *imapFileName, int context )
{
	char        filename[FILEIO_MAX_FILENAME];

	sprintf(filename, "%s.map", imapFileName);
	ipagemap_t *imap = new ipagemap_t(PSEQ_IPAGE_TABLESIZE);
	bool        success = imap->readFile(filename);
	if (success) {
		m_imap = imap;
	} else {
		ERROR_OUT("error reading imap file\n");
		return false;
	}
	return true;
}

//***************************************************************************
void pseq_t::allocateFlatRegBox( mstate_t &inorder )
{
	reg_box_t &rbox = inorder.regs;
	// Here is one location where the pstate object must be initialized
	// (in system, you should NOT be trying to create pseq before pstate objs).

	// The problem here is that now flat register files are allowed
	// to access the M_PSTATE object, but it hasn't be initialized yet.
	rbox.initializeMappings( );
	rbox.addRegisterHandler( RID_NONE, 
			new arf_none_t( NULL, NULL, NULL, fault_stat ) );

	// NOTE: creating flat register file with # of logical registers

	// Pradeep - The flat ARF is used only for the ideal inorder register
	// file. We shouldnt be corrupting it now, so I'm not setting the
	// type of the type of the register file => its not INT_REG_FILE =>
	// fault will not be injected

	physical_file_t *int_file_rf = new physical_file_t( CONFIG_IREG_LOGICAL,
			this, fault_stat ) ;
	flat_int_t *fit = new flat_int_t( int_file_rf, M_PSTATE, fault_stat );
	rbox.addRegisterHandler( RID_INT, fit );
	rbox.addRegisterHandler( RID_INT_GLOBAL,
			new flat_int_global_t( int_file_rf, M_PSTATE, fault_stat ) );

	physical_file_t *fp_file_rf = new physical_file_t( CONFIG_FPREG_LOGICAL,
			this, fault_stat );
	flat_single_t *flat_single_arf = new flat_single_t( fp_file_rf, M_PSTATE, fault_stat );
	rbox.addRegisterHandler( RID_SINGLE, flat_single_arf );

	flat_double_t *flat_double_arf = new flat_double_t( fp_file_rf, M_PSTATE,
			flat_single_arf->getLastWriter(), fault_stat );
	rbox.addRegisterHandler( RID_DOUBLE, flat_double_arf );

	// make a flat control register
	m_ideal_control_rf = new physical_file_t( MAX_CTL_REGS, this, fault_stat );
	flat_control_t *flat_control_arf = new flat_control_t( m_ideal_control_rf,
			M_PSTATE , fault_stat);
	rbox.addRegisterHandler( RID_CONTROL, flat_control_arf );    
	rbox.addRegisterHandler( RID_CC, flat_control_arf );

	//
	// define some "container" register types
	//
	flat_container_t *flat_container = new flat_container_t(fault_stat);
	flat_container->openRegisterType( CONTAINER_BLOCK, 8 );
	// 64-byte block renames 8 double registers,
	//    each with an offset of 2 from the original register specifier
	for ( int32 i = 0; i < 8; i++ ) {
		flat_container->addRegister( RID_DOUBLE, 2*i, flat_double_arf );
	}
	flat_container->closeRegisterType();

	flat_container->openRegisterType( CONTAINER_YCC, 2 );
	// YCC register type writes both CC registers and control registers
	flat_container->addRegister( RID_CC, REG_CC_CCR, flat_control_arf );
	flat_container->addRegister( RID_CONTROL, CONTROL_Y,  flat_control_arf );
	flat_container->closeRegisterType();

	flat_container->openRegisterType( CONTAINER_QUAD, 4 );

	// quad block renames 4 single-precision registers,
	//    each with an offset of 1 from the original register specifier
	for ( int32 i = 0; i < 4; i++ ) {
		flat_container->addRegister( RID_SINGLE, i, flat_single_arf );
	}
	flat_container->closeRegisterType();
	rbox.addRegisterHandler( RID_CONTAINER, flat_container );  
}

//***************************************************************************
void pseq_t::clearFlatRegDeps( mstate_t &inorder, flow_inst_t *predecessor )
{
	// for each register type, add this register as a predecessor
	reg_id_t        rid;
	abstract_rf_t  *arf;

	// clear all windowed integer registers
	rid.setRtype( RID_INT );
	arf = inorder.regs.getARF( rid.getRtype() );
	for (uint32 cwp = 0; cwp < NWINDOWS; cwp++ ) {
		rid.setVanillaState( cwp, 0 );
		// compare the IN and LOCAL registers
		for (int reg = 31; reg >= 16; reg --) {
			rid.setVanilla( reg );
			arf->setDependence( rid, predecessor );
		}
	}

	// clear the global registers
	rid.setRtype( RID_INT_GLOBAL );
	arf = inorder.regs.getARF( rid.getRtype() );
	for (uint16 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset++) {
		rid.setVanillaState( 0, gset );
		// no dependence on register %g0
		rid.setVanilla( 0 );
		arf->setDependence( rid, NULL );
		for (int reg = 1; reg < 8; reg ++) {
			rid.setVanilla( reg );
			arf->setDependence( rid, predecessor );
		}
	}

	// clear the dependences on single registers
	rid.setRtype( RID_SINGLE );
	arf = inorder.regs.getARF( rid.getRtype() );
	// FLOAT_REGS is in terms of doubles, so clear 2 regs at a time
	for (uint32 reg = 0; reg < MAX_FLOAT_REGS; reg++) {
		rid.setVanilla( reg*2 );
		arf->setDependence( rid, predecessor );
		rid.setVanilla( reg*2 + 1 );
		arf->setDependence( rid, predecessor );
	}

	// clear the dependences on control registers
	rid.setRtype( RID_CONTROL );
	arf = inorder.regs.getARF( rid.getRtype() );
	for (uint32 reg = 0; reg < CONTROL_NUM_CONTROL_PHYS; reg++) {
		rid.setVanilla( reg );
		arf->setDependence( rid, predecessor );
	}

	// clear the dependences on condition code registers
	rid.setRtype( RID_CC );
	arf = inorder.regs.getARF( rid.getRtype() );
	for (int reg = REG_CC_CCR; reg <= REG_CC_FCC3; reg++) {
		rid.setVanilla( reg );
		arf->setDependence( rid, predecessor );
	}
}

/*------------------------------------------------------------------------*/
/* Check Interfaces                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pseq_t::commitObserver( dynamic_inst_t *dinstr )
{
	system_t::inst->m_sys_stat->observeInstruction( this, dinstr );
}

/** Buffers retirement information for a given instruction
 */
//**************************************************************************
bool pseq_t::uncheckedRetire( dynamic_inst_t *dinstr )
{
	// if this instruction is a store, we must check the results immediately
	dyn_execute_type_t mytype = dinstr->getStaticInst()->getType();
	if (mytype == DYN_STORE || mytype == DYN_ATOMIC) {
		dinstr->markEvent( EVENT_VALUE_PRODUCER );
	}
	m_unchecked_instr[m_unchecked_retires++] = dinstr;

	ASSERT( m_unchecked_retires <= PSEQ_MAX_UNCHECKED );
	return (m_unchecked_retires >= PSEQ_MAX_UNCHECKED);
}

/** reads unchecked retires out of the instruction buffer.
 *  Used at check time to update statistics. You must free
 *  these instructions!!
 */
//**************************************************************************
dynamic_inst_t *pseq_t::getNextUnchecked( void )
{
	if ( m_unchecked_retires <= 0 )
		return NULL;

	dynamic_inst_t *d = m_unchecked_instr[m_unchecked_retire_top];
	m_unchecked_instr[m_unchecked_retire_top] = NULL;
	m_unchecked_retires--;
	m_unchecked_retire_top++;
	if (m_unchecked_retires == 0)
		m_unchecked_retire_top = 0;
	return (d);
}

void pseq_t::clearUnchecked()
{
	if ( m_unchecked_retires <= 0 )
		return;

	for(int i=0; i<m_unchecked_retire_top; i++) {
		m_unchecked_instr[m_unchecked_retire_top] = NULL;
		m_unchecked_retires--;
		m_unchecked_retire_top++;
		if (m_unchecked_retires == 0)
			m_unchecked_retire_top = 0;
	}
}

//***************************************************************************
memory_inst_t *pseq_t::uncheckedValueForward( pa_t phys_addr, int my_size )
{
	int       index = (int) m_unchecked_retires - 1;
	my_addr_t my_cacheline = phys_addr & ~MEMOP_BLOCK_MASK;
	int       my_offset    = phys_addr &  MEMOP_BLOCK_MASK;

	// out_info("searching: 0x%0llx\n", my_cacheline );
	while ( index >= (int) m_unchecked_retire_top ) {
		dynamic_inst_t *d = m_unchecked_instr[index];
		if ( d->getEvent( EVENT_VALUE_PRODUCER ) ) {
			memory_inst_t *m = (memory_inst_t *) d;
			my_addr_t other_cacheline = m->getPhysicalAddress() & ~MEMOP_BLOCK_MASK;
			int       other_offset    = m->getPhysicalAddress() &  MEMOP_BLOCK_MASK;
			int       other_size      = m->getAccessSize();

			// out_info("   0x%0llx\n", other_cacheline );
			if (my_cacheline == other_cacheline) {
				/*
				   out_info("address overlap 0x%0llx ?= 0x%0llx\n",
				   my_cacheline, other_cacheline);
				   out_info(" my line  0x%0llx 0x%0x\n",
				   my_cacheline, my_offset);
				   out_info(" other line  0x%0llx 0x%0x\n",
				   other_cacheline, other_offset);
				 */

				if ( my_offset == other_offset &&
						other_size >= my_size ) {
					// exact match -- can forward for sure
					return m;
				} // CM FIX: can forward in other situations too

				// FIXFIXFIX
				// CM FIX: CAN match in other situations
			} // end if cache line matches
		}
		index--;
	}
	return NULL;
}

//**************************************************************************
void pseq_t::uncheckedPrint( void )
{
	int index = (int) m_unchecked_retires - 1;
	while ( index >= (int) m_unchecked_retire_top ) {
		dynamic_inst_t *d = m_unchecked_instr[index];
		out_info("unchecked instruction: %d\n", index );
		d->printDetail();
		index--;
	}
}

//***************************************************************************
void pseq_t::pushRetiredInstruction( dynamic_inst_t *dinstr )
{
	// check to see if insertion point is null
	if ( m_recent_retire_instr[m_recent_retire_index] != NULL ) {
		delete m_recent_retire_instr[m_recent_retire_index];
	}
	m_recent_retire_instr[m_recent_retire_index] = dinstr;
	m_recent_retire_index = m_recent_retire_index + 1;
	if (m_recent_retire_index >= PSEQ_RECENT_RETIRE_SIZE) {
		m_recent_retire_index = 0;
	}
}

//***************************************************************************
void pseq_t::printRetiredInstructions( void )
{
	int32 count = 0;
	int32 index = m_recent_retire_index - 1;
	if (index < 0) {
		index = PSEQ_RECENT_RETIRE_SIZE - 1;
	}
	while (count < PSEQ_RECENT_RETIRE_SIZE) {
		dynamic_inst_t *d = m_recent_retire_instr[index];
		if (d == NULL) {
			out_info("recently retired instruction: %d is NULL.\n", count );
		} else {
			out_info("recently retired instruction: %d\n", count );
			d->printDetail();
		}
		// decrement index, wrapping it around in the retire window size
		index = index - 1;
		if (index < 0) {
			index = PSEQ_RECENT_RETIRE_SIZE - 1;
		}
		count++;
	}
}

//***************************************************************************
void pseq_t::checkCriticalState( check_result_t *result, mstate_t *mstate, bool init )
{
	ireg_t simreg;
	bool   mask = false;
	if( GSPOT_DEBUG ) { out_info( "checkCriticalState() with init = %d\n", init ) ;
	}

#ifdef SW_ROLLBACK
	if (m_op_mode==RECOVERING) return;
#endif

	//result->verbose=true;
	if (ALEX_ACCURATE_DEBUG)
		result->verbose=true;

	result->critical_check = true;
	simreg = M_PSTATE->getControl( CONTROL_PSTATE );
	if ( ISADDRESS32(simreg) ) {
		mask = true;
	}

	//if (!LXL_NO_CORRUPT && recovered) mstate->at.has_fault=true;
	// CM AddressMask64
	simreg = M_PSTATE->getControl( CONTROL_PC );
	if (mask) {
		//assert(0);
		simreg = AM64( simreg );
	}

	if (simreg != mstate->at.pc) {
#ifdef FAULT_CORRUPT_SIMICS
		if (result->verbose)
			out_info("patch  PC: 0x%0llx 0x%0llx\n", mstate->at.pc, simreg);
		if (!init && GET_FAULT_INJECTION() && mstate->at.has_fault) {
			if(ALEX_DEBUG)
				DEBUG_OUT( "PC: Before patching, value = %llx\n", simreg ) ;
			ARCH_STATE() ;
			MEM_STATE() ;

#ifdef ALEX_PRINT_MISMATCHES
			FAULT_RET_OUT("%d:",GET_RET_INST());
			FAULT_RET_OUT("c%d:",reg);
			FAULT_RET_OUT("%llx:",mstate->at.pc);
			FAULT_RET_OUT("%llx\n",simreg);
#endif

#if defined(LXL_SNET_RECOVERY)
			if (LXL_NO_CORRUPT) {
				if (recovered) {
					mstate->at.pc = simreg;
					result->retry_mismatch=true;
					result->perfect_check = false;
					result->critical_check = false;
				}
			} else {
				if (recovered) {
					result->perfect_check = false;
					result->critical_check = false;
				}
			}
#endif
			M_PSTATE->setPC(mstate->at.pc);

			if(ALEX_DEBUG) {
				ireg_t simregnew = M_PSTATE->getControl( CONTROL_PC ) ;
				if( mask ) simregnew = AM64( simregnew ) ;
				DEBUG_OUT( "PC: After patching, value = %llx\n", simregnew ) ;
			}
			//out_info("Successfully corrupt PC cur_pc %llx bad_pc %llx last_pc %llx\n",
			//dummy, mstate->at.pc, simreg);
		} else {
			mstate->at.pc = simreg;
			result->perfect_check = false;
			result->critical_check = false;
		}

#else
		if (result->verbose)
			out_info("patch  PC: 0x%0llx 0x%0llx\n", mstate->at.pc, simreg);
		mstate->at.pc = simreg;
		result->perfect_check = false;
		result->critical_check = false;
#endif
	}

	if (!recovered)
		mstate->at.has_fault = false;


	// Explicitly patch the NPC because we want to corrupt it
	simreg = M_PSTATE->getControl( CONTROL_NPC );
	if (mask)
		simreg = AM64( simreg );
	if (simreg != mstate->at.npc) {
#ifdef FAULT_CORRUPT_SIMICS
		// out_info("[0x%llx] 0x%0llx 0x%0llx\n",
		//         mstate->at.pc, mstate->at.npc, simreg);
		if (result->verbose)
			out_info("patch  NPC: 0x%0llx 0x%0llx\n", mstate->at.npc, simreg);
		if (!init && GET_FAULT_INJECTION() && mstate->at.has_fault) {
			if(ALEX_DEBUG)
				DEBUG_OUT( "NPC: Before patching, value = %llx\n", simreg ) ;
			//ARCH_STATE() ;
#if LXL_NO_CORRUPT && defined(LXL_SNET_RECOVERY)
			if (recovered) {
				mstate->at.npc = simreg;
				result->retry_mismatch=true;
				result->perfect_check = false;
				result->critical_check = false;
			}
#endif
			M_PSTATE->setControl(CONTROL_NPC, mstate->at.npc);
			// At this state, we are patching simics and opal to take a new
			// path that was not the path computed by the correct instruction
			// I have a separate mark for this, but this is NOT divergence
			// from golden run, if mem state happens before this!
			// FIXME - This doesn't work correctly!

			if(ALEX_DEBUG) {
				ireg_t simregnew = M_PSTATE->getControl( CONTROL_NPC ) ;
				if( mask ) simregnew = AM64( simregnew ) ;
				DEBUG_OUT( "NPC: After patching, value = %llx\n", simregnew ) ;
			}
		}
		else {
			// CM AddressMask64
			mstate->at.npc = simreg;
			result->perfect_check = false;
			result->critical_check = false;
		}
#else

		out_info("PC = 0x%0llx\n", mstate->at.pc) ;
		out_info("patch  NPC: 0x%0llx", mstate->at.npc);
		out_info("sim = 0x%0llx\n", simreg ) ;
		if (result->verbose)
			out_info("patch  NPC: 0x%0llx 0x%0llx\n", mstate->at.npc, simreg);
		// CM AddressMask64
		mstate->at.npc = simreg;
		result->perfect_check = false;
		result->critical_check = false;

#endif
	}
	mstate->at.has_fault = false;

	// Walk the list of registers to check in the abstract register file.
	for (uint32 i = 0; i < PSEQ_CRITICAL_REG_COUNT; i++) {
		mstate->critical_regs[i].getARF()->check( mstate->critical_regs[i],
				M_PSTATE, result, init );
	}

	if ( result->perfect_check == false ) {
		result->critical_check = false;

		// update all the fields in the fetch_at structure
		physical_file_t *control_state_rf = NULL;
		if ( mstate == &m_ooo ) {
			// checking the criticalness of the timing ('inorder') registers
			control_state_rf = m_control_arf->getRetireRF();
		} else if ( mstate == &m_inorder ) {
			// check the criticalness of the functional ('ideal') registers
			control_state_rf = m_ideal_control_rf;
		} else {
			ERROR_OUT("error: pseq_t: check \"critical\" state fails: invariant violated\n");
			SIM_HALT;
		}

		mstate->at.pstate = control_state_rf->getInt( CONTROL_PSTATE );
		mstate->at.cwp    = control_state_rf->getInt( CONTROL_CWP );
		mstate->at.tl     = control_state_rf->getInt( CONTROL_TL );
		mstate->at.gset   = pstate_t::getGlobalSet( mstate->at.pstate );
		mstate->at.asi    = control_state_rf->getInt( CONTROL_ASI );
	}

#if defined(LXL_SNET_RECOVERY) &&  !LXL_NO_CORRUPT
	if (0&&recovered) {
		result->perfect_check=true;
		result->critical_check=true;
	}
#endif

	if( GSPOT_DEBUG ) { out_info( "done checkCriticalState\n" ) ;
	}

}

//***************************************************************************
void pseq_t::checkAllState( check_result_t *result, mstate_t *mstate, bool init )
{
	if (ALEX_ACCURATE_DEBUG) result->verbose=true; //FIXME: UNDO
	if (ALEX_ACCURATE_DEBUG) out_info("all state enter\n");

#ifdef SW_ROLLBACK
	if (m_op_mode==RECOVERING) return;
#endif

	result->critical_check = true;
	result->perfect_check = true;

	// check the ultimately important (PC, PSTATE) state 
	checkCriticalState( result, mstate, init );

	reg_id_t        rid;
	abstract_rf_t  *arf;
	pstate_t       *pstate = M_PSTATE;

	// compare all windowed integer registers
	rid.setRtype( RID_INT );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (uint32 cwp = 0; cwp < NWINDOWS; cwp++ ) {
		rid.setVanillaState( cwp, 0 );
		// compare the IN and LOCAL registers
		for (int reg = 31; reg >= 16; reg --) {
			rid.setVanilla( reg );

			arf->check( rid, pstate, result, init );
		}
	}

	if( GSPOT_DEBUG ) {
		DEBUG_OUT( "Check all state: init = %d\n", init ) ;
		DEBUG_OUT( "window_int_over\n" ) ;
	}

	// compare the global registers
	rid.setRtype( RID_INT_GLOBAL );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (uint16 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset++) {
		rid.setVanillaState( 0, gset );
		// ignore register %g0
		for (int reg = 1; reg < 8; reg ++) {
			rid.setVanilla( reg );
			arf->check( rid, pstate, result, init );
		}
	}

	if( GSPOT_DEBUG ) {
		DEBUG_OUT( "global_int_over\n" ) ;
	}

	// compare the fp registers
	rid.setRtype( RID_DOUBLE );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (uint32 reg = 0; reg < MAX_FLOAT_REGS; reg++) {
		rid.setVanilla( (reg*2) );
		arf->check( rid, pstate, result, init );
	}
	hfa_checkerr( "FP registers: end\n" );

	if( GSPOT_DEBUG ) {
		DEBUG_OUT( "fp_reg_over\n" ) ;
	}

	// compare the control registers
	rid.setRtype( RID_CONTROL );
	arf = mstate->regs.getARF( rid.getRtype() );
	hfa_checkerr("control registers: start\n");

	if (GSPOT_DEBUG) out_info("before check control all_state: pc %llx\n",mstate->at.pc);

	for (uint32 reg = 0; reg < CONTROL_NUM_CONTROL_PHYS; reg++) {
		rid.setVanilla( reg );
		arf->check( rid, pstate, result, init );
	}
	hfa_checkerr("control registers: end\n");  

	if(  GSPOT_DEBUG ) {
		DEBUG_OUT( "ctrl_reg_over\n" ) ;
	}

	// patch up condition codes %ccr, %fcc0, %fcc1, %fcc2, %fcc3
	rid.setRtype( RID_CC );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (int reg = REG_CC_CCR; reg <= REG_CC_FCC3; reg++) {
		rid.setVanilla( reg );
		arf->check( rid, pstate, result, init );
	}
	hfa_checkerr("ccr, fcc# registers: end\n");

	if(  GSPOT_DEBUG ) {
		DEBUG_OUT( "patch_reg_over\n" ) ;
	}

#ifdef DEBUG_PSEQ
	out_info("  check all state() ends\n");
#endif
	if( ALEX_ACCURATE_DEBUG) { 
		DEBUG_OUT( "check_all_state_ends\n" ) ;
	}
}

//***************************************************************************
void pseq_t::syncAllState( check_result_t *result, mstate_t *mstate, bool init )
{

	// check the ultimately important (PC, PSTATE) state 
	checkCriticalState( result, mstate, init );

	reg_id_t        rid;
	abstract_rf_t  *arf;
	pstate_t       *pstate = M_PSTATE;

	// compare all windowed integer registers
	rid.setRtype( RID_INT );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (uint32 cwp = 0; cwp < NWINDOWS; cwp++ ) {
		rid.setVanillaState( cwp, 0 );
		// compare the IN and LOCAL registers
		for (int reg = 31; reg >= 16; reg --) {
			rid.setVanilla( reg );

			arf->sync( rid, pstate, result, init );
		}
	}

	if( GSPOT_DEBUG ) {
		DEBUG_OUT( "Check all state: init = %d\n", init ) ;
		DEBUG_OUT( "window_int_over\n" ) ;
	}

	// compare the global registers
	rid.setRtype( RID_INT_GLOBAL );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (uint16 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset++) {
		rid.setVanillaState( 0, gset );
		// ignore register %g0
		for (int reg = 1; reg < 8; reg ++) {
			rid.setVanilla( reg );

			arf->sync( rid, pstate, result, init );
		}
	}

	if( GSPOT_DEBUG ) {
		DEBUG_OUT( "global_int_over\n" ) ;
	}

	// compare the fp registers
	rid.setRtype( RID_DOUBLE );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (uint32 reg = 0; reg < MAX_FLOAT_REGS; reg++) {
		rid.setVanilla( (reg*2) );

		arf->sync( rid, pstate, result, init );
	}

	if( GSPOT_DEBUG ) {
		DEBUG_OUT( "fp_reg_over\n" ) ;
	}

	// compare the control registers
	rid.setRtype( RID_CONTROL );
	arf = mstate->regs.getARF( rid.getRtype() );

	for (uint32 reg = 0; reg < CONTROL_NUM_CONTROL_PHYS; reg++) {
		rid.setVanilla( reg );

		arf->sync( rid, pstate, result, init );
	}

	if(  GSPOT_DEBUG ) {
		DEBUG_OUT( "ctrl_reg_over\n" ) ;
	}

	// patch up condition codes %ccr, %fcc0, %fcc1, %fcc2, %fcc3
	rid.setRtype( RID_CC );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (int reg = REG_CC_CCR; reg <= REG_CC_FCC3; reg++) {
		rid.setVanilla( reg );

		arf->sync( rid, pstate, result, init );
	}

	if(  GSPOT_DEBUG ) {
		DEBUG_OUT( "patch_reg_over\n" ) ;
	}
}

//***************************************************************************
void pseq_t::checkChangedState( check_result_t *result, mstate_t *mstate,
		dynamic_inst_t *d_instr, bool init )
{
#ifdef SW_ROLLBACK
	if (m_op_mode==RECOVERING) return;
#endif

	result->perfect_check = true;
	if (ALEX_ACCURATE_DEBUG) result->verbose=true;
	if (ALEX_ACCURATE_DEBUG) out_info("changed state enter\n");

	if ( d_instr->getTrapType() == Trap_NoTrap ) {
		// check the registers we think should have changed
#ifdef LXL_SNET_RECOVERY
		bool has_mismatch=false;
		bool old_retry_mismatch=result->retry_mismatch;
		result->retry_mismatch=false;
#endif

		for (int i = 0; i < SI_MAX_DEST; i++) {
			reg_id_t &rid = d_instr->getDestReg(i);
#ifdef LXL_SNET_RECOVERY
			reg_mismatch_info_t &mismatch=result->mismatch;
			mismatch.id = i;
#endif
			rid.getARF()->check( rid, M_PSTATE, result, init);

#ifdef LXL_SNET_RECOVERY
			if (result->retry_mismatch) {
				if (rid.getRtype()!=RID_CONTROL) {
					assert(result->do_not_sync==false);
					has_mismatch=true;
					d_instr->insertMismatchInfo(mismatch);
				}
				result->retry_mismatch=false;
			} 
#endif
		}
#ifdef LXL_SNET_RECOVERY
		result->retry_mismatch=old_retry_mismatch||has_mismatch;
#endif
	} else {
		// checking an instruction that takes a trap:
		//    explicitly check some control registers

		// invariant: checking changed state only: never updating
		//          : this messes up arf->check() routine
		ASSERT( result->update_only != true );

		// make some reg ids that point to control registers
		reg_id_t        rid;
		abstract_rf_t  *arf = mstate->regs.getARF( RID_CONTROL );
		// check PSTATE
		rid.setRtype( RID_CONTROL );
		rid.setVanilla( CONTROL_PSTATE );
		arf->check( rid, M_PSTATE, result, init );

		// check CWP
		rid.setVanilla( CONTROL_CWP );
		arf->check( rid, M_PSTATE, result, init );

		// check TL
		rid.setVanilla( CONTROL_TL );
		arf->check( rid, M_PSTATE, result, init );

		// check CC
		arf = mstate->regs.getARF( RID_CC );
		rid.setRtype( RID_CC );
		rid.setVanilla( REG_CC_CCR );
		arf->check( rid, M_PSTATE, result, init );

	}
	if (ALEX_ACCURATE_DEBUG) out_info("changed state exit\n");
}

//***************************************************************************
// Called only from functional trapped instruction
void pseq_t::updateInstructionState( dynamic_inst_t *dinstr, bool init )
{
	check_result_t result;

	result.update_only = true;
	// update the registers this instruction writes
	for (int i = 0; i < SI_MAX_DEST; i++) {
		reg_id_t &rid = dinstr->getDestReg(i);
		rid.getARF()->check( rid, M_PSTATE, &result, init );
	}
	hfa_checkerr("error: after update dynamic instruction state.\n");
}

void pseq_t::log_func_inst_info( dynamic_inst_t *dinst )
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		struct func_inst_info temp;
		for (int i = 0; i < SI_MAX_DEST; i++) {
			reg_id_t &rid = dinst->getDestReg(i);
			if( !rid.isZero()) {
				temp.value[i] = rid.getARF()->readInt64(rid);
				if(SIVA_LOAD_BUFFER_DEBUG)
					DEBUG_OUT(" dest%d = %llx ", i, temp.value[i]);
			}
		}
		if(SIVA_LOAD_BUFFER_DEBUG)
			DEBUG_OUT(" \n");
		func_inst_buffer.push_back(temp);    
		//if(func_inst_buffer.size() >= 1024)
		//DEBUG_OUT(" Func inst buffer is large!!\n");
		if(SIVA_LOAD_BUFFER_DEBUG)
			DEBUG_OUT("%d:logging: func inst log size = %d\n", m_id, func_inst_buffer.size());
	}
}

void pseq_t::restore_func_inst_info( dynamic_inst_t *dinst )
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		check_result_t result;
		result.update_only = true;
#ifdef LXL_SNET_RECOVERY
		result.recovered = false;
#endif
		result.verbose = false;
		if(func_inst_buffer.size() <= 0) {
			DEBUG_OUT("%d: Func buffer empty\n", m_id);
			m_multicore_diagnosis->reportFuncBufferMiss(m_id);
			return;
		}
		struct func_inst_info temp = func_inst_buffer.at(0);
		for (int i = 0; i < SI_MAX_DEST; i++) {
			reg_id_t &rid = dinst->getDestReg(i);
			if( !rid.isZero()) {
				rid.getARF()->updateSimicsReg(rid, M_PSTATE, temp.value[i]);
				rid.getARF()->writeInt64(rid,temp.value[i]);
				//rid.getARF()->check(rid, M_PSTATE, &result, false);
				if(SIVA_LOAD_BUFFER_DEBUG) {
					DEBUG_OUT(" dest%d = %llx ", i, temp.value[i]);
					DEBUG_OUT(" getVanilla=%d, getVanillaState=%d ", rid.getVanilla(), rid.getVanillaState());
					DEBUG_OUT(" getPhysical=%d, isPatched=%d \n", rid.getPhysical(), rid.getARF()->isPatched(rid));
				}
			}
		}
		if(SIVA_LOAD_BUFFER_DEBUG)
			DEBUG_OUT(" \n");
		func_inst_buffer.erase(func_inst_buffer.begin());
		if(SIVA_LOAD_BUFFER_DEBUG)
			DEBUG_OUT("%d:tmr: func inst log size = %d\n",m_id, func_inst_buffer.size());
	}
}

//***************************************************************************
void pseq_t::updateInstructionState( flow_inst_t *flow_inst, bool init )
{
	check_result_t result;

	result.update_only = true;
	// update the registers this instruction writes
	for (int i = 0; i < SI_MAX_DEST; i++) {
		reg_id_t &rid = flow_inst->getDestReg(i);
		rid.getARF()->check( rid, M_PSTATE, &result, init );
	}
	hfa_checkerr("error: after update flow instruction state.\n");
}

//***************************************************************************
void pseq_t::allocateRegBox( mstate_t &ooo )
{
	reg_box_t &rbox = ooo.regs;

	// CM FIX: Most of this memory is leaked, as pointers to it are not
	//         kept outside of the register box (either free them or keep
	//         private pointers to them and free at the destructor).

	/// initialize the fetch_at pointer
	m_fetch_at   = new abstract_pc_t();
	m_inorder_at = &ooo.at;
	rbox.initializeMappings( );
	rbox.addRegisterHandler( RID_NONE, 
			new arf_none_t( NULL, NULL, NULL, fault_stat ) );
	physical_file_t *int_rf = new physical_file_t( CONFIG_IREG_PHYSICAL, this, INT_REG_FILE, fault_stat );
	reg_map_t *int_decode_map = new reg_map_t( int_rf, CONFIG_IREG_LOGICAL );
	reg_map_t *int_retire_map = new reg_map_t( int_rf, CONFIG_IREG_LOGICAL );
	rbox.addRegisterHandler( RID_INT, new arf_int_t( int_rf,
				int_decode_map,
				int_retire_map, 4, fault_stat ) );
	rbox.addRegisterHandler( RID_INT_GLOBAL,
			new arf_int_global_t( int_rf,
				int_decode_map,
				int_retire_map, fault_stat ) );
	physical_file_t *fp_rf = new physical_file_t( CONFIG_FPREG_PHYSICAL, this, fault_stat );
	reg_map_t *fp_decode_map = new reg_map_t( fp_rf, CONFIG_FPREG_LOGICAL );
	reg_map_t *fp_retire_map = new reg_map_t( fp_rf, CONFIG_FPREG_LOGICAL );
	arf_single_t *single_arf = new arf_single_t( fp_rf,
			fp_decode_map,
			fp_retire_map, 16, fault_stat );
	rbox.addRegisterHandler( RID_SINGLE, single_arf );


	arf_double_t *double_arf   = new arf_double_t( fp_rf,
			fp_decode_map,
			fp_retire_map, fault_stat );
	rbox.addRegisterHandler( RID_DOUBLE, double_arf );

	m_cc_rf = new physical_file_t( CONFIG_CCREG_PHYSICAL, this, CC_REG_FILE, fault_stat );
	reg_map_t *cc_decode_map = new reg_map_t( m_cc_rf, CONFIG_CCREG_LOGICAL );
	m_cc_retire_map = new reg_map_t( m_cc_rf, CONFIG_CCREG_LOGICAL );

	arf_cc_t *cc_arf = new arf_cc_t( m_cc_rf, cc_decode_map,
			m_cc_retire_map, 2, fault_stat );
	rbox.addRegisterHandler( RID_CC, cc_arf );

	m_control_rf = (physical_file_t **) malloc( sizeof(physical_file_t *) * 
			CONFIG_NUM_CONTROL_SETS );
	for ( uint32 i = 0; i < CONFIG_NUM_CONTROL_SETS; i++ ) {
		m_control_rf[i] = new physical_file_t( CONTROL_NUM_CONTROL_TYPES, this,fault_stat );
	}
	//printf("***********  %d ***********", CONFIG_NUM_CONTROL_SETS);
	m_control_arf = new arf_control_t( m_control_rf, CONFIG_NUM_CONTROL_SETS, fault_stat);
	rbox.addRegisterHandler( RID_CONTROL, m_control_arf );

	//
	// define some "container" register types
	//
	arf_container_t *container_arf = new arf_container_t(fault_stat);
	container_arf->openRegisterType( CONTAINER_BLOCK, 8 );
	// 64-byte block renames 8 double registers,
	//    each with an offset of 2 from the original register specifier
	for ( int32 i = 0; i < 8; i++ ) {
		container_arf->addRegister( RID_DOUBLE, 2*i, double_arf );
	}
	container_arf->closeRegisterType();

	container_arf->openRegisterType( CONTAINER_YCC, 2 );
	// YCC register type writes both CC registers and control registers
	container_arf->addRegister( RID_CC, REG_CC_CCR, cc_arf );
	container_arf->addRegister( RID_CONTROL, CONTROL_Y,  m_control_arf );
	container_arf->closeRegisterType();

	container_arf->openRegisterType( CONTAINER_QUAD, 4 );
	// quad block renames 4 single-precision registers,
	//    each with an offset of 1 from the original register specifier
	for ( int32 i = 0; i < 4; i++ ) {
		container_arf->addRegister( RID_SINGLE, i, single_arf );
	}
	container_arf->closeRegisterType();
	rbox.addRegisterHandler( RID_CONTAINER, container_arf );  

	// allocate and initialize the register identifiers for "critical" checks
	// The order of these fields MATTERS: must be PSTATE, CWP, TL, CCR
	//          (used in checkCriticalState())
	uint32  critical_count = 0;
	ooo.critical_regs[critical_count].setRtype( RID_CONTROL );
	ooo.critical_regs[critical_count].setVanilla( CONTROL_PSTATE );
	ooo.critical_regs[critical_count].copy( ooo.critical_regs[critical_count],
			ooo.regs );
	critical_count++;
	ooo.critical_regs[critical_count].setRtype( RID_CONTROL );
	ooo.critical_regs[critical_count].setVanilla( CONTROL_CWP );
	ooo.critical_regs[critical_count].copy( ooo.critical_regs[critical_count],
			ooo.regs );
	critical_count++;
	ooo.critical_regs[critical_count].setRtype( RID_CONTROL );
	ooo.critical_regs[critical_count].setVanilla( CONTROL_TL );
	ooo.critical_regs[critical_count].copy( ooo.critical_regs[critical_count],
			ooo.regs );
	critical_count++;
	ooo.critical_regs[critical_count].setRtype( RID_CC );
	ooo.critical_regs[critical_count].setVanilla( REG_CC_CCR );
	ooo.critical_regs[critical_count].copy( ooo.critical_regs[critical_count],
			ooo.regs );
	critical_count++;
	ASSERT( critical_count == PSEQ_CRITICAL_REG_COUNT );
}

//***************************************************************************
void pseq_t::allocateIdealState( void )
{
	/*
	 * ideal register file initialization
	 */

	// initialize the indecies
	m_ideal_retire_count     = 0;
	m_ideal_last_checked     = -1;
	m_ideal_last_freed       = -1;
	m_ideal_last_predictable = -1;

	// build the jump table for the flow instructions
	flow_inst_t::ix_build_jump_table();

	/// build the ideal register file
	allocateFlatRegBox( m_inorder );

	//
	// Initialize the critical register file checking:
	//    copy( source, destination )
	//    e.g. inorder.critical_regs <- ooo.critical_regs
	uint32  count = 0;
	m_inorder.critical_regs[count].copy( m_ooo.critical_regs[count],
			m_inorder.regs );
	count++;
	m_inorder.critical_regs[count].copy( m_ooo.critical_regs[count],
			m_inorder.regs );
	count++;
	m_inorder.critical_regs[count].copy( m_ooo.critical_regs[count],
			m_inorder.regs );
	count++;
	m_inorder.critical_regs[count].copy( m_ooo.critical_regs[count],
			m_inorder.regs );
	count++;
	ASSERT( count == PSEQ_CRITICAL_REG_COUNT );

	//
	// Control flow graph
	//
	// allocate memory dependence tracking structure
	m_mem_deps = new mem_dependence_t();

	// allocate a 'predecessor node' for the control flow graph
	m_cfg_pred = new flow_inst_t( m_fetch_itlbmiss, this, NULL, m_mem_deps );

	// clear all dependence information (set to m_cfg_pred)
	clearFlatRegDeps( m_inorder, m_cfg_pred );
}

/*------------------------------------------------------------------------*/
/* Debugging / Testing                                                    */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pseq_t::printPC( abstract_pc_t *a )
{
	char buffer[128]; // big enough to hold 32 bits + white space

	out_log( "PC :    0x%0llx\n", a->pc );
	out_log( "NPC:    0x%0llx\n", a->npc );
	out_log( "TL :      %d\n", a->tl );
	sprintBits( a->pstate, buffer );
	out_log( "PSTATE:   %s\n", buffer );
	out_log( "CWP:      %d\n", a->cwp );
	out_log( "GSET:     %d\n", a->gset );
}

//**************************************************************************
void pseq_t::print( void )
{
	//out_info("fetch PC:  0x%0llx\n", m_fetch_at->pc);
	//out_info("fetch NPC: 0x%0llx\n", m_fetch_at->npc);
#ifdef DEBUG_PSEQ
	out_info("Primary CTX: %d\n", m_primary_ctx);

	// print fetch/decode per cycles
	out_info("I$ pipeline");
	m_i_cache_line_queue.print();

	out_info("fetch per cycle");
	m_fetch_per_cycle.print();

	out_info("decode per cycle");
	m_decode_per_cycle.print();
#endif

	// print the instruction window and associated pipelines
	// DEBUG_RETIRE
	//print rubycache
	m_ruby_cache->print();
	m_iwin.print();
	out_info("Fetch status: %s\n", fetch_menomic( m_fetch_status ));
	m_iwin.printDetail();

	// print out the outstanding transactions
	if (m_ruby_cache)
		m_ruby_cache->print();

	// print out 'unretired' instructions
	uncheckedPrint();

	// print out recently retired instructions (like a crash log)
	printRetiredInstructions();

	// print out load/store queue
	m_lsq->printLSQ();

	// print out the register mapping
	// m_int_decode_map->print();
}

//***************************************************************************
void pseq_t::printInflight( void )
{
	out_info("*** Inflight instructions\n");
	out_info("Fetch status: %s\n", fetch_menomic( m_fetch_status ));
	out_info("\n");
	m_iwin.printDetail();
	if (m_ruby_cache)
		m_ruby_cache->print();

	// print out recently retired instructions (like a crash log)
	printRetiredInstructions();
}

//***************************************************************************
void pseq_t::printStats( void )
{
	// temporarily stop debug filtering-- you asked for the stats right?
	uint64 filterTime = getDebugTime();
	setDebugTime( 0 );

	if (m_stat_committed > 0) {  
		out_info("*** Opcode stats:\n");
		m_opstat.print( this );
	}

	char   user_mode = '?';
	char   buf[80], buf1[80], buf2[80], buf3[80];
	double total_pct = 0.0;
	uint64 total_ideal_coverage = 0;

	if (m_ideal_retire_count > 0) {
		out_info("*** Ideal opcode stats:\n");
		m_ideal_opstat.print( this );

		out_info("Ideal predictor rating:\n");
		for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
			total_ideal_coverage += m_hist_ideal_coverage[i];
		}
		for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
			if (total_ideal_coverage == 0)
				total_pct = 0.0;
			else
				total_pct = ((double) m_hist_ideal_coverage[i] / (double) total_ideal_coverage)
					*100.0;
			out_info("%-20.20s: %24.24s %6.2lf%%\n",
					fetch_menomic( (pseq_fetch_status_t) i ),
					commafmt(m_hist_ideal_coverage[i], buf, 80),
					total_pct);
		}
		out_info("\n");
	}

	out_info("*** Trap   stats:\n");
	out_info("  [Trap#]  Times-Taken  Simics-Taken    Name\n");
	for (int i = 0; i < TRAP_NUM_TRAP_TYPES; i ++) {
		if ( m_trapstat[i] != 0 || m_simtrapstat[i] != 0 ) {
			out_info("  [%3d] %14lld  %14lld  %s\n", i, m_trapstat[i], m_simtrapstat[i],
					pstate_t::trap_num_menomic( (trap_type_t) i ));
		}
	}

	out_info("*** Internal exception stats:\n");
	out_info("###: seen    name\n");
	for (int i = 0; i < EXCEPT_NUM_EXCEPT_TYPES; i ++) {
		if ( m_exception_stat[i] != 0 ) {
			out_info("  [%3d] %14lld  %s\n", i, m_exception_stat[i],
					pstate_t::async_exception_menomic( (exception_t) i ));
		}
	}

	out_info("*** ASI    stats:\n");
	out_info("  ASI     Reads   Writes  Atomics\n");
	for (uint32 i = 0; i < MAX_NUM_ASI; i ++) {
		if ( m_asi_rd_stat[i] != 0 ||
				m_asi_wr_stat[i] != 0 ||
				m_asi_at_stat[i] != 0 ) {
			out_info("  0x%02x %8lld %8lld %8lld\n", 
					i, m_asi_rd_stat[i], m_asi_wr_stat[i], m_asi_at_stat[i] );
		}
	}

	uint64 total_pred = 0;
	uint64 total_seen = 0;
	uint64 total_right = 0;
	uint64 total_wrong = 0;
	double bpaccuracy;
	out_info("*** Branch   stats: (user, kernel, total)\n");
	out_info("  Type           Preds      Retired        Right       Wrong    %%Right\n");
	for (uint32 i = 0; i < BRANCH_NUM_BRANCH_TYPES; i++) {
		for (uint32 j = 0; j < TOTAL_INSTR_MODE; j++) {

			// get the mode string
			switch (j) {
				case 0:
					user_mode = 'U';
					break;
				case 1:
					user_mode = 'K';
					break;
				case 2:
					user_mode = 'T';
					break;
				default:
					ERROR_OUT("pseq_t::PrintStats(): unknown branch type=%d\n", j);
#ifdef LL_DECODER
					bool curr_priv = fault_stat->getRetPriv() ;
					int trap_level = m_control_arf->getRetireRF()->getInt( CONTROL_TL );
					OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
							m_local_sequence_number,
							pstate_t::trap_num_menomic((trap_type_t)0x10), 
							0x10,0x0,
							curr_priv, 0,
							(uint16)trap_level, m_id);
					HALT_SIMULATION ;
#else // LL_DECODER
					SIM_HALT;
#endif // LL_DECODER 
			}

			if ( m_branch_seen_stat[i][j] != 0) {
				bpaccuracy = ((double) m_branch_right_stat[i][j] / (double) m_branch_seen_stat[i][j]) * 100;
			} else {
				bpaccuracy = 0.0;
			}
			out_info("   %-6.6s %12.12s %12.12s %12.12s %12.12s  %c:%6.2f%%\n",
					pstate_t::branch_type_menomic[i] + 7,
					commafmt( m_branch_pred_stat[i][j], buf, 80 ),
					commafmt( m_branch_seen_stat[i][j], buf1, 80 ),
					commafmt( m_branch_right_stat[i][j], buf2, 80 ),
					commafmt( m_branch_wrong_stat[i][j], buf3, 80 ),
					user_mode, bpaccuracy);
			if (j==2) { // total
				total_pred  += m_branch_pred_stat[i][j];
				total_seen  += m_branch_seen_stat[i][j];
				total_right += m_branch_right_stat[i][j];
				total_wrong += m_branch_wrong_stat[i][j];
			}
		} // j
	} // i
	if (total_seen != 0)
		bpaccuracy = ((double) total_right / (double) total_seen) * 100;
	else
		bpaccuracy = 0.0;
	out_info("   %-6.6s %12.12s %12.12s %12.12s %12.12s  %6.2f%%\n",
			"TOTALB",
			commafmt( total_pred, buf, 80 ),
			commafmt( total_seen, buf1, 80 ),
			commafmt( total_right, buf2, 80 ),
			commafmt( total_wrong, buf3, 80 ),
			bpaccuracy);

	out_info("*** Histogram   stats:\n");
	uint64 total_fetch       = 0;
	uint64 total_decode      = 0;
	uint64 total_schedule    = 0;
	uint64 total_retire      = 0;
	uint64 total_fetch_stall = 0;
	uint64 total_retire_stall = 0;
	for (uint32 i = 0; i < MAX_FETCH + 1; i++) {
		total_fetch  += m_hist_fetch_per_cycle[i];
	}
	for (uint32 i = 0; i < MAX_DECODE + 1; i++) {
		total_decode += m_hist_decode_per_cycle[i];
	}
	for (uint32 i = 0; i < MAX_DISPATCH + 1; i++) {
		total_schedule += m_hist_schedule_per_cycle[i];
	}
	for (uint32 i = 0; i < MAX_RETIRE + 1; i++) {
		total_retire += m_hist_retire_per_cycle[i];
	}
	for (uint32 i = 0; i < MAX_FETCH + 1; i++) {
		if (total_fetch == 0)
			total_pct = 0.0;
		else
			total_pct = ((double) m_hist_fetch_per_cycle[i] / (double) total_fetch)
				*100.0;
		out_info("Fetch  [%d]: %24.24s %6.2lf%%\n",
				i, commafmt(m_hist_fetch_per_cycle[i], buf, 80), total_pct);
	}
	out_info("\n");
	for (uint32 i = 0; i < MAX_DECODE + 1; i++) {
		if (total_decode == 0)
			total_pct = 0.0;
		else
			total_pct = ((double) m_hist_decode_per_cycle[i] / (double) total_decode)
				*100.0;
		out_info("Decode [%d]: %24.24s %6.2lf%%\n",
				i, commafmt(m_hist_decode_per_cycle[i], buf, 80), total_pct);
	}
	out_info("\n");
	for (uint32 i = 0; i < MAX_DISPATCH + 1; i++) {
		if (total_schedule == 0)
			total_pct = 0.0;
		else
			total_pct = ((double) m_hist_schedule_per_cycle[i] / (double) total_schedule) *100.0;
		out_info("Schedule [%d]: %24.24s %6.2lf%%\n",
				i, commafmt(m_hist_schedule_per_cycle[i], buf, 80), total_pct);
	}
	out_info("\n");
	for (uint32 i = 0; i < MAX_RETIRE + 1; i++) {
		if (total_retire == 0)
			total_pct = 0.0;
		else
			total_pct = ((double) m_hist_retire_per_cycle[i] / (double) total_retire)
				*100.0;
		out_info("Retire [%d]: %24.24s %6.2lf%%\n",
				i, commafmt(m_hist_retire_per_cycle[i], buf, 80), total_pct);
	}
	out_info("\n");

	// FIXFIXFIX
#if 0
	for (uint32 i = 0; i < PSEQ_HIST_DECODE; i++) {
		out_info( "Return [%d]: %24.24s\n",
				i, commafmt(m_hist_decode_return[i], buf, 80) );
	}
	out_info("\n");

	for (uint32 i = 0; i < dynamic_inst_t::MAX_INST_STAGE; i++) {
		for (uint32 j = 0; j < IWINDOW_WIN_SIZE; j++) {
			if (m_hist_squash_stage[i][j] != 0) {
				out_info( "%s [%d]: %24.24s\n",
						dynamic_inst_t::printStage( (dynamic_inst_t::stage_t) i ),
						j, commafmt(m_hist_squash_stage[i][j], buf, 80) );
			}
		}
		out_info("\n");
	}
	out_info("\n");
#endif

	out_info("Reasons for fetch stalls:\n");
	for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
		total_fetch_stall += m_hist_fetch_stalls[i];
	}
	for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
		if (total_fetch_stall == 0)
			total_pct = 0.0;
		else
			total_pct = ((double) m_hist_fetch_stalls[i] / (double) total_fetch_stall)
				*100.0;
		out_info("%-20.20s: %24.24s %6.2lf%%\n",
				fetch_menomic( (pseq_fetch_status_t) i ),
				commafmt(m_hist_fetch_stalls[i], buf, 80),
				total_pct);
	}
	out_info("\n");

	out_info("Other fetch statistics:\n");
	total_pct = ((double) m_stat_no_fetch_across_lines / (double) getLocalCycle()) * 100.0;
	out_info("%-34.34s: %20.20s %6.2lf%%\n",
			"Fetch stalled at line boundary",
			commafmt(m_stat_no_fetch_across_lines, buf, 80),
			total_pct);
	total_pct = ((double) m_stat_no_fetch_taken_branch / (double) getLocalCycle()) * 100.0;
	out_info("%-34.34s: %20.20s %6.2lf%%\n",
			"Fetch stalled at taken branch",
			commafmt(m_stat_no_fetch_taken_branch, buf, 80),
			total_pct);
	out_info("\n");

	out_info("Reasons for retire limits:\n");
	for (uint32 i = 0; i < PSEQ_RETIRE_MAX_STATUS; i++) {
		total_retire_stall += m_hist_retire_stalls[i];
	}
	for (uint32 i = 0; i < PSEQ_RETIRE_MAX_STATUS; i++) {
		if (total_retire_stall == 0)
			total_pct = 0.0;
		else
			total_pct = ((double) m_hist_retire_stalls[i] / (double) total_retire_stall)
				*100.0;
		out_info("%-20.20s: %24.24s %6.2lf%%\n",
				retire_menomic( (pseq_retire_status_t) i ),
				commafmt(m_hist_retire_stalls[i], buf, 80),
				total_pct);
	}
	out_info("\n");

	out_info("Execution unit statistics:\n");
	out_info("TYPE (id)         : (# units) [Average in use per cycle]:\n");
	for (uint32 i = 0; i < FU_NUM_FU_TYPES; i++) {
		if (CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[i]] != 0) {
			out_info("%-20.20s: (map:%d) (units:%d) %6.2lf %lld\n",
					pstate_t::fu_type_menomic( (fu_type_t) i ),
					CONFIG_ALU_MAPPING[i],
					CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[i]],
					((double) m_stat_fu_utilization[CONFIG_ALU_MAPPING[i]] /
					 (double) getLocalCycle()),
					m_stat_fu_utilization[CONFIG_ALU_MAPPING[i]] );
		}
	}
	out_info("\n");
	out_info("Execution unit retirement:\n");
	out_info("TYPE (id)         : (retired # of instructions)\n");
	for (uint32 i = 0; i < FU_NUM_FU_TYPES; i++) {
		if ( m_stat_fu_util_retired[i] != 0 ) {
			if (m_stat_committed == 0)
				total_pct = 0.0;
			else
				total_pct = ((double) m_stat_fu_util_retired[i] / (double) m_stat_committed) * 100.0;
			out_info("%-20.20s: %6.2f\t%lld\n",
					pstate_t::fu_type_menomic( (fu_type_t) i ),
					total_pct,
					m_stat_fu_util_retired[i] );
		}
	}
	out_info("\n");

	out_info("\n");
	out_info("*** Fastforward statistics:\n");
	for (uint32 i = 0; i < PSEQ_MAX_FF_LENGTH; i++) {
		if (m_hist_ff_length[i] != 0) {
			out_info("%5d: %24.24s\n",
					i,
					commafmt(m_hist_ff_length[i], buf, 80) );
		}
	}

	out_info("\n");
	out_info("*** Static predictation stats:\n");
	out_info("  When      NotPred       Pred      Taken        Not\n");
	out_info("  Decode %10.10s %10.10s %10.10s %10.10s\n",
			commafmt( m_nonpred_count_stat, buf, 80 ),
			commafmt( m_pred_count_stat, buf1, 80 ),
			commafmt( m_pred_count_taken_stat, buf2, 80 ),
			commafmt( m_pred_count_nottaken_stat, buf3, 80 ) );
	out_info("  Retire %10.10s %10.10s %10.10s %10.10s\n",
			commafmt( m_nonpred_retire_count_stat, buf, 80 ),
			commafmt( m_pred_retire_count_stat, buf1, 80 ),
			commafmt( m_pred_retire_count_taken_stat, buf2, 80 ),
			commafmt( m_pred_retire_count_nottaken_stat, buf3, 80 ));
	out_info("  Regs              %10.10s %10.10s %10.10s\n",
			commafmt( m_pred_reg_count_stat, buf1, 80 ),
			commafmt( m_pred_reg_taken_stat, buf2, 80 ),
			commafmt( m_pred_reg_nottaken_stat, buf3, 80 ));
	out_info("  RegRet            %10.10s %10.10s %10.10s\n",
			commafmt( m_pred_reg_retire_count_stat, buf1, 80 ),
			commafmt( m_pred_reg_retire_taken_stat, buf2, 80 ),
			commafmt( m_pred_reg_retire_nottaken_stat, buf3, 80 ));
	int index = BRANCH_PCOND;
	if ( m_branch_seen_stat[index][2] != 0)
		bpaccuracy = (double) (m_branch_seen_stat[index][2] - m_branch_wrong_static_stat) / (double) m_branch_seen_stat[index][2] * 100;
	else
		bpaccuracy = 0.0;
	out_info( "   STATIC  %12.12s     %6.2lf%%\n",
			commafmt( m_branch_wrong_static_stat, buf, 80 ),
			bpaccuracy);

	// MLP statistics
	out_info( "\n" );
	out_info( "*** # of outstanding misses\n" );
	m_stat_hist_misses->print( this );
	out_info( "*** Interarrival times\n" );
	m_stat_hist_interarrival->print( this );
	out_info( "*** Dependent ops instrs\n" );
	m_stat_hist_dep_ops->print( this );

	out_info( "*** Effective\n" );
	m_stat_hist_effective_ind->print( this );
	out_info( "*** Not Effective\n" );
	m_stat_hist_effective_dep->print( this );
	out_info( "*** inter cluster\n" );
	m_stat_hist_inter_cluster->print( this );

	out_info("*** Pipeline statistics:\n");
	out_info("  %-50.50s %10llu\n", "cycle of decode stalls:", m_reg_stall_count_stat );
	out_info("  %-50.50s %10llu\n", "insts of decode of stall:", m_decode_stall_count_stat );
	out_info("  %-50.50s %10llu\n", "cycle of schedule stalls:", m_iwin_stall_count_stat );
	out_info("  %-50.50s %10llu\n", "insts of schedule of stall:", m_schedule_stall_count_stat );
	out_info("  %-50.50s %10llu\n", "count early store bypasses:", m_stat_early_store_bypass );
	out_info("  %-50.50s %10llu\n", "total number of asi store squashes:", m_stat_count_asistoresquash);
	out_info("  %-50.50s %10llu\n", "count failed retirements:", m_stat_count_badretire );
	out_info("  %-50.50s %10llu\n", "count functional retirements:", m_stat_count_functionalretire );
	out_info("  %-50.50s %10llu\n", "count of I/O loads/stores:", m_stat_count_io_access );
	out_info("  %-50.50s %10llu\n", "count done/retry squashes:", m_stat_count_retiresquash );

	/* register baseline stats */
	out_info("  %-50.50s %10llu\n", "number of instructions read from trace:", m_stat_trace_insn);
	out_info("  %-50.50s %10llu\n", "total number of instructions committed:", m_stat_committed);
	out_info("  %-50.50s %10llu\n", "total number of times squash is called:", m_stat_total_squash);
	out_info("  %-50.50s %10llu\n", "total number of instructions squashing at commit:", m_stat_commit_squash);
	out_info("  %-50.50s %10llu\n", "total number of instructions committing successfully:", m_stat_commit_good);
	out_info("  %-50.50s %10llu\n", "total number of instructions committing unsuccessfully:", m_stat_commit_bad);
	out_info("  %-50.50s %10llu\n", "total number of unimplemented instructions committing:", m_stat_commit_unimplemented);

	out_info("  %-50.50s %10llu\n", "total number of instructions fetched:", m_stat_fetched);
	out_info("  %-50.50s %10llu\n", "total number of mini-itlb misses:", m_stat_mini_itlb_misses);
	out_info("  %-50.50s %10llu\n", "total number of instructions decoded:", m_stat_decoded);
	out_info("  %-50.50s %10llu\n", "total number of instructions executed:", m_stat_total_insts);

	out_info("  %-50.50s %10llu\n", "total number of loads executed:", m_stat_loads_exec);
	out_info("  %-50.50s %10llu\n", "total number of stores executed:", m_stat_stores_exec);
	out_info("  %-50.50s %10llu\n", "total number of atomics executed:", m_stat_atomics_exec);
	out_info("  %-50.50s %10llu\n", "total number of prefetches executed:", m_stat_prefetches_exec);
	out_info("  %-50.50s %10llu\n", "total number of control insts executed:", m_stat_control_exec);

	out_info("  %-50.50s %10llu\n", "total number of loads retired:",
			m_stat_loads_retired);
	out_info("  %-50.50s %10llu\n", "total number of stores retired:",
			m_stat_stores_retired);
	out_info("  %-50.50s %10llu\n", "total number of atomics retired:",
			m_stat_atomics_retired);
	out_info("  %-50.50s %10llu\n", "total number of prefetches retired:",
			m_stat_prefetches_retired);
	out_info("  %-50.50s %10llu\n", "total number of control instrs committed:", m_stat_control_retired);

	out_info("  %-50.50s %10llu\n", "loads with valid data at executed:", m_stat_loads_found);
	out_info("  %-50.50s %10llu\n", "loads with invalid data at execute:", m_stat_loads_notfound);
	out_info("  %-50.50s %10llu\n", "total number of spill traps:", m_stat_spill);
	out_info("  %-50.50s %10llu\n", "total number of fill traps:", m_stat_fill);

	/* cache misses per retired instruction stats */
	out_info("  %-50.50s %10llu\n", "number fetches executed which miss in icache",
			m_stat_num_icache_miss);
	out_info("  %-50.50s %10llu\n", "number misses fetches that hit in mshr",
			m_stat_icache_mshr_hits );

	out_info("  %-50.50s %10llu\n", "number loads executed which miss in dcache",
			m_stat_num_dcache_miss);
	out_info("  %-50.50s %10llu\n", "retiring data cache misses",
			m_stat_retired_dcache_miss );
	out_info("  %-50.50s %10llu\n", "retiring L2 misses",
			m_stat_retired_memory_miss );
	out_info("  %-50.50s %10llu\n", "retiring MSHR hits",
			m_stat_retired_mshr_hits );
	out_info("  %-50.50s %10llu\n", "number of times ruby is not ready",
			m_num_cache_not_ready);

	/* lsq stats */
	out_info("  %-50.50s %10llu\n", "number of loads satisfied by store queue:", m_stat_bypasses);
	out_info("  %-50.50s %10llu\n", "number of stores scheduled before value:", m_stat_num_early_stores);
	out_info("  %-50.50s %10llu\n", "number of loads waiting for early store resolution:", m_stat_num_early_store_bypasses);
	out_info("  %-50.50s %10llu\n", "number of atomics scheduled before value:", m_stat_num_early_atomics);
	out_info("  %-50.50s %10llu\n", "number of stale data speculations", m_stat_stale_predictions);
	out_info("  %-50.50s %10llu\n", "number of successful stale predictions:", m_stat_stale_success);
	if (m_stat_stale_predictions != 0)
		out_info("  %-50.50s %6.2lf%%\n", "percent stale value predictions",
				100.0 * ((float) m_stat_stale_success / (float) m_stat_stale_predictions) );
	for (uint32 i=0; i < (log_base_two(MEMOP_MAX_SIZE*8)+1); i++) {
		out_info("  %-50.50s %4u %10llu\n", "stale prediction size = ", 1 << i,
				m_stat_stale_histogram[i] );
	}

	// print cache statistics only if ruby is not being used
	if (!CONFIG_WITH_RUBY) {
		l2_cache->printStats( this );
		l2_mshr->printStats( this );
		l1_data_cache->printStats( this );
		dl1_mshr->printStats( this );
		l1_inst_cache->printStats( this );
		il1_mshr->printStats( this );
	}

	// print out instruction page statistics
	m_imap->printStats();

	// print how long it took to simulate, and time per cycle
	int64 sec_expired  = 0;
	int64 usec_expired = 0;
	m_simulation_timer.getCumulativeTime( &sec_expired, &usec_expired );

	out_info("*** Runtime statistics:\n");
	out_info("  %-50.50s %10llu\n", "Total number of instructions",
			m_stat_committed );
	out_info("  %-50.50s %10llu\n", "Total number of cycles",
			getLocalCycle() );
	out_info("  %-50.50s %10llu\n", "number of continue calls", m_stat_continue_calls );
	out_info("  %-50.50s %g\n", "Instruction per cycle:",
			(double) m_stat_committed / (double) getLocalCycle() );

	out_info("  %-50.50s %lu sec %lu usec\n", "Total Elapsed Time:",
			sec_expired, usec_expired );

	int64 retire_sec  = 0;
	int64 retire_usec = 0;
	m_retirement_timer.getCumulativeTime( &retire_sec, &retire_usec );
	out_info("  %-50.50s %lld sec %lld usec\n", "Total Retirement Time:",
			retire_sec, retire_usec );

	double dsec = (double) sec_expired + ((double) usec_expired / (double) 1000000);
	out_info("  %-50.50s %g\n", "Approximate cycle per sec:",
			(double) getLocalCycle() / dsec );
	out_info("  %-50.50s %g\n", "Approximate instructions per sec:",
			(double) m_stat_committed / dsec );
	double dretire = (double) retire_sec + ((double) retire_usec / (double) 1000000);
	double overhead = 100.0;
	if ( dsec != 0 ) {
		overhead = (dretire / dsec) * 100.0;
	}
	out_info("  %-50.50s %6.2lf%%\n", "This processor's Simics overhead (retire/elapsed):", overhead );
	if (m_stat_continue_calls != 0) {
		out_info("  %-50.50s %6.2lf\n", "Average number of instructions per continue", ((double) m_stat_committed / (double) m_stat_continue_calls) );
	}

#ifdef RENAME_EXPERIMENT
	for (uint32 i = 0; i < RID_NUM_RID_TYPES; i++ ) {
		uint32 logical;
		switch (i) {
			case RID_INT:
				logical = CONFIG_IREG_LOGICAL;
				out_info("Integer information\n");
				break;

			case RID_SINGLE:
				logical = CONFIG_FPREG_LOGICAL;
				out_info("Floating point info\n");
				break;

			case RID_CC:
				logical = CONFIG_CCREG_LOGICAL;
				out_info("Condition code info\n");
				break;

			default:
				// we don't track rename statistics on all other registers
				continue;
		}

		double sum;
		for (uint32 j = 0; j < logical; j++) {
			sum = 0.0;
			for (uint32 k = 0; k < PSEQ_REG_USE_HIST; k++) {
				sum += (double) m_reg_use[i][j][k];
				out_info("%d ", m_reg_use[i][j][k]);
			}
			out_info("(%d %f)\n", j, sum);
		}
		out_info("\n\n");
	}
#endif
	setDebugTime( filterTime );
}

//**************************************************************************
void pseq_t::validateTrace( void )
{
	if (m_tracefp)
		m_tracefp->validate();
}

//**************************************************************************
uint32 pseq_t::printIpage( bool verbose )
{
	// count the total number of instructions on an instruction page
	out_info("total static instruction count:\n");
	uint32 count = m_imap->print( verbose );
	return (count);
}

//***************************************************************************
const char *pseq_t::fetch_menomic( pseq_fetch_status_t status )
{
	switch ( status ) {
		case PSEQ_FETCH_READY:
			return("Fetch ready");
			break;
		case PSEQ_FETCH_ICACHEMISS:
			return("Fetch i-cache miss");
			break;
		case PSEQ_FETCH_SQUASH:
			return("Fetch squash");
			break;
		case PSEQ_FETCH_ITLBMISS:
			return("Fetch I-TLB miss");
			break;
		case PSEQ_FETCH_WIN_FULL:
			return("Window Full");
			break;
		case PSEQ_FETCH_BARRIER:
			return("Fetch Barrier");
			break;
		default:
			return("Unknown Fetch");
	}
	return("Unknown Fetch");
}

//***************************************************************************
const char *pseq_t::retire_menomic( pseq_retire_status_t status )
{
	switch ( status ) {
		case PSEQ_RETIRE_READY:
			return("Retire ready");
			break;
		case PSEQ_RETIRE_UPDATEALL:
			return("Retire Updating...");
			break;
		case PSEQ_RETIRE_SQUASH:
			return("Retire Squash");
			break;
		case PSEQ_RETIRE_LIMIT:
			return("Retire Limit");
			break;
		default:
			return("Unknown Retire");
	}
	return("Unknown Retire");
}

//***************************************************************************
// void  pseq_t::slidingWindowFill( void )

/*------------------------------------------------------------------------*/
/* Statistics Gathering                                                   */
/*------------------------------------------------------------------------*/

//**************************************************************************
void 
pseq_t::initializeStats(void) {
	m_overall_timer = new stopwatch_t(m_id);
	m_thread_timer = new stopwatch_t(m_id);
	m_thread_histogram = new histogram_t( "ThreadPC", 2048 );
	m_exclude_count = 0;
	m_thread_count = 0;
	m_thread_count_idle = 0;
	for (int i = 0; i < TRAP_NUM_TRAP_TYPES; i ++) {
		m_trapstat[i] = 0;
		m_simtrapstat[i] = 0;
	}
	for (uint32 i = 0; i < MAX_NUM_ASI; i ++) {
		m_asi_rd_stat[i] = 0;
		m_asi_wr_stat[i] = 0;
		m_asi_at_stat[i] = 0;
	}
	m_hist_fetch_per_cycle = (uint64 *) malloc( sizeof(uint64) * (MAX_FETCH+1));
	for (uint32 i = 0; i < MAX_FETCH + 1; i++) {
		m_hist_fetch_per_cycle[i] = 0;
	}
	m_hist_decode_per_cycle = (uint64 *) malloc( sizeof(uint64) * (MAX_DECODE+1) );
	for (uint32 i = 0; i < MAX_DECODE + 1; i++) {
		m_hist_decode_per_cycle[i] = 0;
	}
	m_hist_schedule_per_cycle = (uint64 *) malloc( sizeof(uint64) * (MAX_DISPATCH+1) );
	for (uint32 i = 0; i < MAX_DISPATCH + 1; i++) {
		m_hist_schedule_per_cycle[i] = 0;
	}
	m_hist_retire_per_cycle = (uint64 *) malloc( sizeof(uint64) * (MAX_RETIRE+1) );
	for (uint32 i = 0; i < MAX_RETIRE + 1; i++) {
		m_hist_retire_per_cycle[i] = 0;
	}
	m_hist_fetch_stalls = (uint64 *) malloc( sizeof(uint64) * PSEQ_FETCH_MAX_STATUS );
	m_hist_retire_stalls = (uint64 *) malloc( sizeof(uint64) * PSEQ_RETIRE_MAX_STATUS );
	for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
		m_hist_fetch_stalls[i] = 0;
	}
	for (uint32 i = 0; i < PSEQ_RETIRE_MAX_STATUS; i++) {
		m_hist_retire_stalls[i] = 0;
	}

	m_hist_squash_stage = (uint64 **) malloc( sizeof(uint64 *) *
			(dynamic_inst_t::MAX_INST_STAGE) );
	for (uint32 i = 0; i < dynamic_inst_t::MAX_INST_STAGE; i++) {
		m_hist_squash_stage[i] = (uint64 *) malloc( sizeof(uint64) * (IWINDOW_ROB_SIZE) );
		for (int32 j = 0; j < IWINDOW_ROB_SIZE; j++) {
			m_hist_squash_stage[i][j] = 0;
		}
	}

	m_hist_decode_return = (uint64 *) malloc( sizeof(uint64) * (PSEQ_HIST_DECODE) );
	for (int32 i = 0; i < PSEQ_HIST_DECODE; i++) {
		m_hist_decode_return[i] = 0;
	}

	m_hist_ff_length = (uint64 *) malloc( sizeof(uint64) * PSEQ_MAX_FF_LENGTH );
	for (uint32 i = 0; i < PSEQ_MAX_FF_LENGTH; i++) {
		m_hist_ff_length[i] = 0;
	}

	m_hist_ideal_coverage = (uint64 *) malloc( sizeof(uint64) * PSEQ_FETCH_MAX_STATUS );
	for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
		m_hist_ideal_coverage[i] = 0;
	}

	m_stat_no_fetch_taken_branch = 0;
	m_stat_no_fetch_across_lines = 0;

	m_stat_fu_utilization = (uint64 *) malloc( sizeof(uint64) * 
			FU_NUM_FU_TYPES );
	for (uint32 i = 0; i < FU_NUM_FU_TYPES; i++) {
		m_stat_fu_utilization[i] = 0;
	}
	m_stat_fu_util_retired = (uint64 *) malloc( sizeof(uint64) * 
			FU_NUM_FU_TYPES );
	for (uint32 i = 0; i < FU_NUM_FU_TYPES; i++) {
		m_stat_fu_util_retired[i] = 0;
	}

	for (uint32 i = 0; i < BRANCH_NUM_BRANCH_TYPES; i++) {
		for (uint32 j = 0; j < TOTAL_INSTR_MODE; j++) {
			m_branch_pred_stat[i][j]  = 0;
			m_branch_seen_stat[i][j]  = 0;
			m_branch_right_stat[i][j] = 0;
			m_branch_wrong_stat[i][j] = 0;
		}
		m_branch_except_stat[i] = 0;
	}
	m_stat_early_store_bypass = 0;
	m_branch_wrong_static_stat = 0;
	m_stat_count_badretire = 0;
	m_stat_count_functionalretire = 0;
	m_stat_count_io_access = 0;
	m_stat_count_retiresquash = 0;

	m_stat_trace_insn = 0;
	m_stat_committed = 0;
	m_stat_commit_squash = 0;
	m_stat_count_asistoresquash = 0;
	m_stat_commit_good = 0;
	m_stat_commit_bad = 0;
	m_stat_commit_unimplemented = 0;
	m_stat_count_except = 0;

	m_stat_loads_retired = 0;
	m_stat_stores_retired = 0;
	m_stat_atomics_retired = 0;
	m_stat_prefetches_retired = 0;
	m_stat_control_retired = 0;
	m_stat_fetched = 0;
	m_stat_mini_itlb_misses = 0;
	m_stat_decoded = 0;
	m_stat_total_insts = 0;

	m_stat_loads_exec = 0;
	m_stat_stores_exec = 0;
	m_stat_atomics_exec = 0;
	m_stat_prefetches_exec = 0;
	m_stat_control_exec = 0;
	m_stat_loads_found = 0;
	m_stat_loads_notfound = 0;
	m_stat_total_squash = 0;
	m_stat_spill = 0;
	m_stat_fill = 0;

	/* cache stats */
	m_stat_num_icache_miss = 0;
	m_stat_num_dcache_miss = 0;
	m_stat_icache_mshr_hits = 0;
	m_stat_retired_dcache_miss = 0;
	m_stat_retired_memory_miss = 0;
	m_stat_retired_mshr_hits = 0;
	m_num_cache_not_ready = 0;

	m_stat_miss_count = 0;
	m_stat_last_miss_seq = 0;
	m_stat_last_miss_fetch = 0;
	m_stat_last_miss_issue = 0;
	m_stat_last_miss_retire = 0;

	m_stat_hist_misses = new histogram_t( "Misses", 1024 );
	m_stat_hist_interarrival = new histogram_t( "Bursts", 4096 );

	m_stat_hist_effective_ind  = new histogram_t( "EffInd", 4096 );
	m_stat_hist_effective_dep  = new histogram_t( "EffDep", 4096 );
	m_stat_hist_inter_cluster  = new histogram_t( "IClust", 4096 );
	m_stat_hist_dep_ops        = new histogram_t( "DepOps", 4096 );

	m_stat_miss_effective_ind = 0;
	m_stat_miss_effective_dep = 0;
	m_stat_miss_inter_cluster = 0;

	/* lsq stats */
	m_stat_bypasses = 0;
	m_stat_num_early_stores = 0;
	m_stat_num_early_store_bypasses = 0;
	m_stat_num_early_atomics = 0;
	m_stat_stale_predictions = 0;
	m_stat_stale_success = 0;
	m_stat_stale_histogram = (uint64 *)malloc( sizeof(uint64)*
			(log_base_two(MEMOP_MAX_SIZE*8)+1) );
	for (uint32 i = 0; i < (log_base_two(MEMOP_MAX_SIZE*8) + 1); i++) {  
		m_stat_stale_histogram[i] = 0;
	}

	// predictation statistics
	m_pred_count_stat = 0;
	m_pred_count_taken_stat = 0;
	m_pred_count_nottaken_stat = 0;
	m_nonpred_count_stat = 0;
	m_pred_reg_count_stat = 0;
	m_pred_reg_taken_stat = 0;
	m_pred_reg_nottaken_stat = 0;

	m_pred_retire_count_stat = 0;
	m_pred_retire_count_taken_stat = 0;
	m_pred_retire_count_nottaken_stat = 0;
	m_nonpred_retire_count_stat = 0;
	m_pred_reg_retire_count_stat = 0;
	m_pred_reg_retire_taken_stat = 0;
	m_pred_reg_retire_nottaken_stat = 0;

	m_reg_stall_count_stat = 0;
	m_decode_stall_count_stat = 0;
	m_iwin_stall_count_stat = 0;
	m_schedule_stall_count_stat = 0;

	for (int i = 0; i < EXCEPT_NUM_EXCEPT_TYPES; i ++) {
		m_exception_stat[i] = 0;
	}
	m_stat_continue_calls = 0;
	m_stat_modified_instructions = 0;
	m_inorder_partial_success = 0;

	// start and stop pcs
	start_pc = stop_pc = (my_addr_t)-1 ;
}

bool 
pseq_t::isTrapFatal(uint16 traptype) {
	switch (traptype) {

		case Trap_Watchdog_Reset:
		case Trap_Red_State_Exception:
		case Trap_Instruction_Access_Exception:
		case Trap_Instruction_Access_Error:
		case Trap_Illegal_Instruction:
		case Trap_Division_By_Zero:
		case Trap_Internal_Processor_Error:
		case Trap_Data_Access_Exception:
		case Trap_Data_Access_Error:
		case Trap_Mem_Address_Not_Aligned:
		case Trap_Lddf_Mem_Address_Not_Aligned:
		case Trap_Stdf_Mem_Address_Not_Aligned:
		case Trap_Ldqf_Mem_Address_Not_Aligned:
		case Trap_Stqf_Mem_Address_Not_Aligned:
		case Trap_Async_Data_Error:
			return true;
	}
	return false;
}

#ifdef DETECT_INF_LOOP

//LXL: if wanna control when to start detect, use GET_INF_LOOP_START()
//to get start detect cycle

void pseq_t::isLoop(dynamic_inst_t *cinst) {

	// Dont do inf loop detection till arch state mismatches
	// if( !(fault_stat->getArchMismatch()) ) {
	//     return ;
	// }
#ifdef LXL_SNET_RECOVERY
	if (recovered) {
		return;
	}
#endif

	la_t new_loop_pc = cinst->getVPC(); 

	if( (new_loop_pc>=0x105bfc4) && (new_loop_pc<=0x105e164) )  return;

	uint64 index = ((new_loop_pc >> 2) ^ (new_loop_pc >> 9)) % LOOP_HASH_SIZE;

	if (new_loop_pc != loop_info[index][TAGS_IDX]) {
		loop_info[index][TAGS_IDX] = new_loop_pc;
		loop_info[index][CNT_IDX] = 0;
		loop_info[index][LAST_SEQ_IDX] = cinst->getSequenceNumber();
		loop_info[index][RANGE_IDX] = -1;
		loop_info[index][VALUE_START] = cinst->getSequenceNumber() ;

	} else {
		int range = cinst->getSequenceNumber() - loop_info[index][LAST_SEQ_IDX];

		loop_info[index][LAST_SEQ_IDX] = cinst->getSequenceNumber();

		if (loop_info[index][RANGE_IDX] == -1) {
			if (range < RANGE_THRESHOLD) { // small loop? 
				loop_info[index][RANGE_IDX] = range;
				loop_info[index][CNT_IDX]++;
			} 
		} else {
			bool exact_match;

			exact_match = (range <= loop_info[index][RANGE_IDX]);

			if (exact_match) { //same range loop as last?
				loop_info[index][CNT_IDX]++;

			} else { //diff loop
				loop_info[index][CNT_IDX] = 0;

				loop_info[index][VALUE_START] = cinst->getSequenceNumber() ;
				loop_info[index][VALUE_START+1] = getLocalCycle() ;

				if (range < RANGE_THRESHOLD)
					loop_info[index][RANGE_IDX] = range;
				else
					loop_info[index][RANGE_IDX] = -1;

			}
			if (loop_info[index][CNT_IDX] > INF_LOOP_THRESHOLD) {

				OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
						cinst->getSequenceNumber(),
						"Potential_Inf_Loop",
						0xff,
						cinst->getVPC(),
						cinst->getPrivilegeMode(), 0x0,
						0, 0) ;
				//#define PROFILE_GOLDEN

#ifdef PROFILE_GOLDEN
				DEBUG_OUT("[%d]\t", getLocalCycle() ) ;
				DEBUG_OUT("[%d]\t", loop_info[index][VALUE_START] ) ;
				DEBUG_OUT("Potential_Inf_Loop(%d)\tpc:0x%llx\t",cinst->getPrivilegeMode(), new_loop_pc);
				DEBUG_OUT("range:%d\n",range);
#endif
				loop_info[index][CNT_IDX] = 0; //Used to find out troubling golden run PCs
#ifdef LXL_SNET_RECOVERY
				if(MULTICORE_DIAGNOSIS_ONLY) {
					if(!m_multicore_diagnosis->hasDiagnosisStarted()) 
						symptomDetected(0xff);
				} else {
					symptomDetected(0xff);
				}
#else
#ifndef PROFILE_GOLDEN
				HALT_SIMULATION;
#endif
#endif
			}
		} 
	}
}

#endif //End DETECT_INF_LOOP


bool pseq_t::isInjectionInstr(dynamic_inst_t * d_inst) {
	if(!done_squash_trans && d_inst->getSequenceNumber()==fault_stat->getFaultInjInst()) {
		done_squash_trans = true;
		return true;
	}
	return false;
}

//dest_src = DEST_REG if its for a dest register
//dest_src = SRC_REG if its for a src register
void pseq_t::checkAndInjectTransientFault(dynamic_inst_t *d_inst, int dest_src)
{
	// if (d_inst->getSequenceNumber() > fault_stat->getFaultInjInst()-10 && d_inst->getSequenceNumber() < fault_stat->getFaultInjInst()+10) 
	// 	DEBUG_OUT("At %d PC:0x%llx, looking for=%d\n", d_inst->getSequenceNumber(), d_inst->getVPC(), fault_stat->getFaultInjInst()) ;
	if(d_inst->getSequenceNumber()==fault_stat->getFaultInjInst()) {

		//DEBUG_OUT("%d: ", dest_src);
		//d_inst->printRetireTrace( "Ret: " ) ;

#ifdef ARCH_REGISTER_INJECTION
		// faults in the source registers should be injected right
		// after the decode stage the injected reg value will be used
		// in this instructions execution
		// Obtain physical register for the given arch reg number, and
		// then inject fault in this physical register. 
		int archreg_no = fault_stat->getFaultyReg();
		int srcnum = -1;
		if(!trans_fault_injected) {
			if(dest_src == SRC_REG) {
				srcnum = d_inst->getSourceNum(archreg_no);
				if(srcnum != -1 && done_squash_trans)  {
					DEBUG_OUT("SRC register found\n");
					reg_id_t &rid = d_inst->getSourceReg(srcnum);
					DEBUG_OUT("Injecting Transient fault in register %d(arch), %d(physical)\n", archreg_no, rid.getPhysical());
					injectTransientFault(rid.getPhysical()) ;
					trans_fault_injected = true;
					squash_next_instr = true;
				}
			} else if (dest_src == DEST_REG) {
				srcnum = d_inst->getDestNum(archreg_no);
				if(srcnum != -1)  {
					DEBUG_OUT("DEST register found\n");
					reg_id_t &rid = d_inst->getDestReg(srcnum);
					DEBUG_OUT("Injecting Transient fault in register %d(arch), %d(physical)\n", archreg_no, rid.getPhysical());
					injectTransientFault(rid.getPhysical()) ;
					trans_fault_injected = true;
					squash_next_instr = true;
				}
			}
		} 
#else // ARCH_REGISTER_INJECTION
		injectTransientFault() ;
#endif // ARCH_REGISTER_INJECTION
	} else {
	}
}



#ifdef ARCH_REGISTER_INJECTION
// physical register number is passed, use this instead of the one using
// directly from getFaultyReg(). This is a special case when you want to inject
// faults in arch-level registers
void pseq_t::injectTransientFault(half_t physical_reg_no) 
#else 
void pseq_t::injectTransientFault() 
#endif
{
	if (GET_FAULT_STUCKAT()!=4) {
		return;
	}

	if (fault_stat->getTransInjected()) {
		return;
	}

#ifndef ARCH_TRANS_FAULT
	if (getLocalCycle() != fault_stat->getFaultInjCycle()) {
		return ;
	}
#endif // ARCH_TRANS_FAULT

	// DEBUG_OUT("Injecting transient fault of type %d\n", GET_FAULT_TYPE()) ;
	if (GET_FAULT_TYPE()==REG_FILE_FAULT) {
		arf_int_t* int_arf =(arf_int_t*) m_ooo.regs.getARF(RID_INT);
		physical_file_t* prf = int_arf->getPhysicalRF();
#ifdef ARCH_REGISTER_INJECTION
		prf->injectTransientFault(physical_reg_no);
#else 
		prf->injectTransientFault();
#endif

	} else if(GET_FAULT_TYPE()==FP_REG_FAULT) {
		arf_single_t* fp_arf = (arf_single_t*) m_ooo.regs.getARF(RID_SINGLE) ;
		physical_file_t *prf = fp_arf->getPhysicalRF() ; // Both SINGLE and DOUBLE had same physical register file

		prf->injectTransientFault();

	} else if (GET_FAULT_TYPE()==RAT_FAULT) {
		arf_int_t* int_arf =(arf_int_t*) m_ooo.regs.getARF(RID_INT);
		reg_map_t* decode_map = int_arf->getDecodeMap();
		decode_map->injectTransientFault();

	}  else if (GET_FAULT_TYPE()==INST_WINDOW_FAULT) {
		m_iwin.injectTransientFault();
	}
}


bool pseq_t::inIdleLoop(la_t virtual_address)
{
	// 118943c, 1189438, 1189434, 1179c1c, 1179c18, 1179c30, 1179c2c, 1179c28, 1179c24, 1179c20
#ifdef MULTI_CORE
	if(OPEN_SOL) {
		if ((virtual_address==0x105d800)) {
			return true ;
		}
	} else  
		if(SOL9) {
			if((virtual_address>=0x1049a7c && virtual_address<=0x1049b98) ||
					(virtual_address>=0x104a070 && virtual_address<=0x104a074) ||
					(virtual_address>=0x1048788 && virtual_address<=0x1048798) ||
					(virtual_address>=0x1048730 && virtual_address<=0x104875c) ||
					(virtual_address>=0x1048708 && virtual_address<=0x1048714) ||
					(virtual_address>=0x10486e8 && virtual_address<=0x10486f0) ||
					(virtual_address>=0x1048070 && virtual_address<=0x1048074) ||
					(virtual_address>=0x1039cc0 && virtual_address<=0x1039ce0) ||
					(virtual_address>=0x1039c80 && virtual_address<=0x1039c98)) {
				return true;
			}
		} else {
			return false;
		}
#else // MULTI_CORE

	if(OPEN_SOL) {
		// New PCs after seeing kernel source
		// Explanation of some older PCs in opensol
		// 0x10079c0 - ptl1_panic() which calls vpanic()
		// 0x1023148 - panic_stop_cpus() ends up in vpanic()
		// 0x1008ac0 - sfmmu_kitlb_miss(). Bad context. Causes panic()
		// 0x10090b8 - sfmmu_tsb_miss(). Bad context?. Causes panic()
		// 0x1069374 - Inf loop inside panicsys(), called from panic()

		// True idle loop. The range is checked as there seems to be a goto
		// from libc that enters the middle of the function.
		if( (virtual_address>=0x105bfc4) && (virtual_address<=0x105c118) ) {
			return true ;
		}

		return false ;

		// // Check for idle loop, identified in the # prompt
		// if( (virtual_address==0x105c004) || (virtual_address==0x105d800) ) {
		//     return true ;
		// }

		// // check for PCs executed before application aborts
		// This pc is actually ptl1_panic(), which causes kernel panic
		// if( (virtual_address == 0x10079c0) ) {
		//     return true ;
		// }
		// 
		// // Check if the kernel is dead
		// if( (virtual_address == 0x1023148) || // iTLB, etc is just screwed.
		//     (virtual_address == 0x1008ac0) || // all entires in immu valid and locked. Results undefined error!
		//     (virtual_address == 0x10090b8) || // Kernel is dead at this point. Casa killing us!
		//     (virtual_address == 0x1069374) ) { // This is absolute hell! Just ba and jmpl! OS stuck!
		//     return true ;
		// } 
		// return false ;

	} else if (SOL9) {
		if((virtual_address>=0x1049a7c && virtual_address<=0x1049b7c) ||
				(virtual_address>=0x1039cc0 && virtual_address<=0x1039cdc) ||
				(virtual_address>=0x1049b84 && virtual_address<=0x1049b8c) || 
				(virtual_address>=0x1048748 && virtual_address<=0x1048758) ||
				(virtual_address>=0x1039c80 && virtual_address<=0x1039c94) ||
				virtual_address == 0x1048788 || virtual_address == 0x1048a070 ||
				virtual_address == 0x1048794 || virtual_address == 0x10486ec ||
				virtual_address == 0x1048708 || virtual_address == 0x104810 ||
				virtual_address == 0x1048734) {
			return true ;
		} else {
			return false;
		}
	} else {
		ERROR_OUT("OS unspecified. Neiether SOL9 or OEPN_SOL\n") ;
		assert(0) ;
	}
#endif // MULTI_CORE


}

void pseq_t::addMemRec(uint64 pc, bool isLoad, uint64 address, uint64 data)
{
	mem_rec_t new_rec;

	new_rec.PC = pc;
	new_rec.isLoad = isLoad;
	new_rec.address = address;
	new_rec.data = data;

	m_mem_trace.push_back(new_rec);

}

int pseq_t::getCurrChkptNum() {
	return M_PSTATE->getCurrChkptNum();
}

// FIXME - Deprectaed fcuntion
void pseq_t::startCheckpointing()
{
}

void pseq_t::takeChkpt(dynamic_inst_t *instruction)
{
	long long int old_pc = m_fetch_at->pc ;
#ifdef DEBUG_AMBER_CHKPT
	if(instruction) {
		DEBUG_OUT("Taking chkpt at %d\n", instruction->getSequenceNumber()) ;
	}
#endif // DEBUG_AMBER_CHKPT
	if(instruction) {
		// return ;
		// enum i_opcode op = (enum i_opcode) instruction->getStaticInst()->getOpcode();
		if(RETIRE_TRACE) {
			instruction->printRetireTrace("Chkpt: ") ;
		}
		fullSquash(i_nop) ;
		instruction->Squash() ;
		int next_inst = stepSequenceNumber() ;
	}
	M_PSTATE->checkpointState();
	checkpointTLB();

	if(instruction) {
		m_ruby_cache->newChkpt();
		system_t::inst->newAmberChkpt();
	} else { // This is chkpt at time 0. Notify all to log and buffer
#ifdef REPLAY_PHASE
		m_ruby_cache->notifySNETLogging();
		system_t::inst->initAmber() ;
#endif
		if(DIAGNOSIS_THRESHOLD >= 1) {
			m_ruby_cache->notifySNETLogging();
			system_t::inst->initAmber() ;
		}
	}

	if (ALEX_SNET_DEBUG) {
		DEBUG_OUT("PC during Chkpt: Current = 0x%llx, Chkpt = 0x%llx\n", old_pc, M_PSTATE->getChkptPC()) ;
		DEBUG_OUT("checkpointed state\n");
		DEBUG_OUT("m_fetch_at pc is %llx\n",m_fetch_at->pc);
		DEBUG_OUT("m_inorder_at pc is %llx\n",m_inorder_at->pc);

		ireg_t simreg = M_PSTATE->getControl( CONTROL_PSTATE );
		bool mask = ISADDRESS32(simreg);

		ireg_t simregnew = M_PSTATE->getControl( CONTROL_PC ) ;
		if( mask ) simregnew = AM64( simregnew ) ;
		DEBUG_OUT( "PC: After patching, value = %llx mask:%d\n", simregnew,mask ) ;
		DEBUG_OUT( "SIMICS PC: %llx\n", M_PSTATE->getPC());
	}
}

void pseq_t::printArchState()
{
	M_PSTATE->printArchState() ;
	printTLB() ;
}

void pseq_t::checkpointTLBAfterLogging()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		for (int i=0; i<num_tlb_regs; i++) {
			getMulticoreDiagnosis()->tlb_state[m_id].tlb_reg[i] = SIM_get_attribute(m_mmu_object,cheetah_register_names[i]);
			//DEBUG_OUT("%s:%d\n", cheetah_register_names[i], SIM_get_attribute(m_mmu_object,cheetah_register_names[i]));
		}
	}
}
bool pseq_t::compareState(core_state_t *ps)
{ 
	return M_PSTATE->compareState(ps); 
}

void pseq_t::checkpointTLB()
{
	attr_value_t vlist;
	// LXL: Copy all mmu regs and tlb entries
	for (int i=0; i<num_tlb_regs; i++) {
		tlb_data[getCurrChkptNum()]->reg[i] = SIM_get_attribute(m_mmu_object,cheetah_register_names[i]);
		if(MULTICORE_DIAGNOSIS_ONLY) {
			getMulticoreDiagnosis()->tlb_state[m_id].tlb_reg[i] = SIM_get_attribute(m_mmu_object,cheetah_register_names[i]);
			//DEBUG_OUT("%s:%d\n", cheetah_register_names[i], SIM_get_attribute(mmu,cheetah_register_names[i]));
		}
	}
}

void pseq_t::printTLB()
{
	DEBUG_OUT(" TLB Attributes: \n");

	for (int i=0; i<num_tlb_regs; i++) {
		DEBUG_OUT("%d:", SIM_get_attribute(m_mmu_object,cheetah_register_names[i]));
	}
	DEBUG_OUT("\n");
}

void pseq_t::recoverTLB()
{

	attr_value_t tmp_reg[num_tlb_regs], daccess,tagread;

#ifdef LXL_DIFF_TLB
	for (int i=0; i<num_tlb_regs; i++) {
		tmp_reg[i] = SIM_get_attribute(m_mmu_object,cheetah_register_names[i]);
	}
	for (int i=0; i<num_tlb_regs; i++) {
		if (tmp_reg[i].kind == Sim_Val_Integer) {
			if (tmp_reg[i].u.integer!=tlb_data[getCurrChkptNum()]->reg[i].u.integer) {
				DEBUG_OUT("%s mismatch\t",cheetah_register_names[i]);
				DEBUG_OUT("0x%llx\t",tmp_reg[i].u.integer);
				DEBUG_OUT("0x%llx\n",tlb_data[getCurrChkptNum()]->reg[i].u.integer);
			}
		} else if (tmp_reg[i].kind == Sim_Val_List) {
			DEBUG_OUT("checking %s size %d\n",cheetah_register_names[i],tmp_reg[i].u.list.size);
			for (int j=0; j<tmp_reg[i].u.list.size; j++) {
				if (tmp_reg[i].u.list.vector[j].u.integer!=tlb_data[getCurrChkptNum()]->reg[i].u.list.vector[j].u.integer) {
					DEBUG_OUT("%s mismatch @ ",cheetah_register_names[i]);
					DEBUG_OUT("%d\t",j);
					DEBUG_OUT("0x%llx\t",tmp_reg[i].u.list.vector[j].u.integer);
					DEBUG_OUT("0x%llx\n",tlb_data[getCurrChkptNum()]->reg[i].u.list.vector[j].u.integer);
				}
			}
		} else {
			DEBUG_OUT("weird type reg %s\n",cheetah_register_names[i]);
		}
	}
#endif
	//LXL: Restore all mmu regs/tlb entries
	for (int i=0; i<num_tlb_regs; i++) {
		SIM_set_attribute(m_mmu_object,cheetah_register_names[i], &tlb_data[getCurrChkptNum()]->reg[i]);
	}

}

// Called when simics enters RED state (seen in system/pstate.C::simcontinue().
void pseq_t::simicsDetection()
{
	OUT_SYMPTOM_INFO((uint64) getLocalCycle(),
			GET_RET_INST(),
			pstate_t::trap_num_menomic(Trap_Red_State_Exception),
			Trap_Red_State_Exception,0,
			GET_RET_PRIV(), 0x0, 0, m_id);
	FAULT_SET_NON_RET_TRAP() ;
#ifdef LXL_SNET_RECOVERY
	symptomDetected(Trap_Red_State_Exception) ;
#else // LXL_SNET_RECOVERY
	HALT_SIMULATION ;
#endif // LXL_SNET_RECOVERY
}

void pseq_t::symptomDetected(int traptype)
{
	system_t::inst->setDetected() ;
	//printTLB();    
	//DEBUG_OUT("%d: %d\n", m_id, getLocalCycle());
	DEBUG_OUT("%d: symptom detected 0x%x\n", m_id, traptype);
	fflush(stdout);

	debugio_t::setSymptomDetected();
	if(MULTICORE_DIAGNOSIS_ONLY) {
#ifdef REPLAY_PHASE
		if(m_multicore_diagnosis->isReplayPhase()) {
			DEBUG_OUT("\nSymptom During Replay Phase\n%d: symptom detected 0x%x\n", m_id, traptype);
			HALT_SIMULATION;
		}
#else
		m_multicore_diagnosis->setSymptomCausingCore(m_id, traptype);
#endif
	} else {
		oldSymptomDetected(traptype);
	}
}

void pseq_t::oldSymptomDetected(int traptype)
{
	DEBUG_OUT("%d: symptom detected 0x%x\n", m_id, traptype);
#ifdef CHK_SYMPTOM
	if (recovered) {
		detected_again = true;
		m_diagnosis->symptomAgain(GET_RET_INST(),traptype);
	}
#endif

	m_last_symptom=(trap_type_t)traptype;

	// LXL: FIXME, if you are using NO_CORRUPT switch, uncomment this.
	//     if (LXL_NO_CORRUPT && recovered && (traptype!=0x2)) 
	//         return;

	if (!LXL_NO_CORRUPT && traptype!=0x202) 
		do_not_advance=true;

	if(ROLLBACK_AT_SYMPTOM) {
		m_trigger_recovery=true;
		recovered = false;
	} else {
		HALT_SIMULATION ;
	}
	debugio_t::setSymptomDetected();
}

#ifdef SW_ROLLBACK
void pseq_t::initializeRecoveryInstrMem() 
{

	recovery_instr_mem_size = (REG_GLOBAL_INT-REG_GLOBAL_NORM+1)*8 + 
		NWINDOWS*(31-16+1)
		+MAX_FLOAT_REGS;

	recovery_instr_mem = new unsigned int[recovery_instr_mem_size];
	unsigned int instr_ptr = 0;

	unsigned int fake_load = (0x3<<30) | (0x1b<<19) | (0x8c<<5);

	for ( uint32 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset ++ ) {
		// registers 0...7 are global
		for ( uint32 i = 0; i < 8; i ++ ) {
			unsigned int temp = fake_load | (i<<25) | gset;
			recovery_instr_mem[instr_ptr++]=temp;
		}
	}

	//
	// copy the integer registers using the read_register api.
	//
	for ( int cwp = 0; cwp < NWINDOWS; cwp++ ) {
		// just update the IN and LOCAL registers for ALL windows
		for (uint32 reg = 31; reg >= 16; reg --) {
			unsigned int temp = fake_load | (reg<<25) | cwp;
			recovery_instr_mem[instr_ptr++]=temp;
		}
	}

	fake_load = (0x3<<30) | (0x23<<19) ;
	//
	// copy the floating point registers using the register api
	//
	for ( uint32 reg = 0; reg < MAX_FLOAT_REGS; reg ++ ) {
		unsigned int temp = fake_load | (reg<<25) ;
		//temp = (unsigned int) 0x4<<22; //This is nop // FIXME
		recovery_instr_mem[instr_ptr++]=temp;
	}

	assert(instr_ptr==recovery_instr_mem_size);

	recovery_instr_ptr=0;

}
#endif

void pseq_t::clearAllFaults()
{
	reg_id_t        rid;
	abstract_rf_t  *arf;
	pstate_t       *pstate = M_PSTATE;

	// compare all windowed integer registers
	rid.setRtype( RID_INT );
	arf = m_ooo.regs.getARF( rid.getRtype() );
	arf->clearAllFaults();

	rid.setRtype( RID_DOUBLE );
	arf = m_ooo.regs.getARF( rid.getRtype() );
	arf->clearAllFaults();

	rid.setRtype( RID_CC );
	arf = m_ooo.regs.getARF( rid.getRtype() );
	arf->clearAllFaults();

	rid.setRtype( RID_CONTROL );
	arf = m_ooo.regs.getARF( rid.getRtype() );
	arf->clearAllFaults();

}

ireg_t pseq_t::getCheckpointReg(reg_id_t&rid)
{
	return M_PSTATE->recoverReg(rid);

}

void pseq_t::validateRecovery() 
{
	reg_id_t        rid;
	abstract_rf_t  *arf;
	pstate_t       *pstate = M_PSTATE;
	mstate_t       *mstate = &m_ooo;
	check_result_t result;

	result.verbose=true;


	// compare all windowed integer registers
	rid.setRtype( RID_INT );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (uint32 cwp = 0; cwp < NWINDOWS; cwp++ ) {
		rid.setVanillaState( cwp, 0 );
		// compare the IN and LOCAL registers
		for (int reg = 31; reg >= 16; reg --) {
			result.perfect_check=true;
			rid.setVanilla( reg );
			arf->check( rid, pstate, &result, false );
			if (!result.perfect_check) {
				DEBUG_OUT("mismatch reg ");
				arf->print(rid);
			} else {
				DEBUG_OUT("reg good\n");
			}
		}
	}

	// compare the global registers
	rid.setRtype( RID_INT_GLOBAL );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (uint16 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset++) {
		rid.setVanillaState( 0, gset );
		// ignore register %g0
		for (int reg = 1; reg < 8; reg ++) {
			result.perfect_check=true;
			rid.setVanilla( reg );
			arf->check( rid, pstate, &result, false );
			if (!result.perfect_check) {
				DEBUG_OUT("mismatch reg ");
				arf->print(rid);
			} else {
				DEBUG_OUT("reg good\n");
			}
		}
	}

	rid.setRtype( RID_DOUBLE );
	arf = mstate->regs.getARF( rid.getRtype() );
	for (uint32 reg = 0; reg < MAX_FLOAT_REGS; reg++) {
		result.perfect_check=true;
		rid.setVanilla( (reg*2) );
		arf->check( rid, pstate, &result, false );
		if (!result.perfect_check) {
			DEBUG_OUT("mismatch reg ");
			arf->print(rid);
		} else {
			DEBUG_OUT("reg good\n");
		}

	}

}

void pseq_t::logMemEvent(uint64 addr, int access_size, ireg_t *data)
{
#ifdef SESE_POT
	if(m_start_logging) { 
		mem_event_t me;
		me.address = addr;
		me.access_size = access_size;
		for(int i=0; i<access_size; i++) {
			me.data[i] = data[i];
		}
		mem_event_log.push_back(me);
	}
#endif
}

void pseq_t::printMemEvents()
{
#ifdef SESE_POT
	for(int i=0; i<mem_event_log.size(); i++) {
		DEBUG_OUT("%lld:",mem_event_log[i].address);
		DEBUG_OUT("%d:",mem_event_log[i].access_size);
		for(int j=0; j<mem_event_log[i].access_size; j++) {
			DEBUG_OUT("%lld:",mem_event_log[i].data[j]);
		}
	}
#endif
}

int pseq_t::getChkptInterval()
{
	return M_PSTATE->getChkptInterval() ;
}

void pseq_t::takeProcessorCheckpoint()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {
		//Checkpoint processor state and the TLB at the end of LoggingPhase
		if(getMulticoreDiagnosis()->hasDiagnosisStarted()) {
			M_PSTATE->checkpointStateAfterLogging();
			checkpointTLBAfterLogging();
		}
	}
}

void pseq_t::recoveryModuleExec()
{

	if (getLocalCycle()==0) {
		DEBUG_OUT("Taking chkpt at cycle 0\n") ;
		// printArchState() ;
		takeChkpt(NULL) ;
	}

	if(debugio_t::checkSymptomDetected(m_id)) {
		if(ROLLBACK_AT_SYMPTOM) {
			m_trigger_recovery = true;
		}
		debugio_t::checkedSymptomDetected(m_id);
	}

	if (m_trigger_recovery && recovered) {
		// If hardware hangs
		if (USE_TBFD && 0x2==m_last_symptom) {
			// Extract the stuck instruction. This is specific to
			// TBFD. Normally, when we see hardware hang, we roll back
			dynamic_inst_t *d=m_iwin.getNextInline();
			if (d==NULL) {
				DEBUG_OUT("WEIRD!! null instruction\n");
			} else {
				if (PRINT_INSTR_AFTER_RECOVERY)
					d->printDetail();
			}

			m_diagnosis->treatStuckInstr(d);

			m_trigger_recovery=false;
			m_last_symptom=Trap_NoTrap;

			//HACK ALERT, make the instruction read junk and move on
			dynamic_inst_t::stage_t cur_stage=d->getStage();
			if (cur_stage >=dynamic_inst_t::WAIT_4TH_STAGE && 
					cur_stage<=dynamic_inst_t::WAIT_1ST_STAGE) {
				reg_id_t &rid=d->getSourceReg(3-(cur_stage-dynamic_inst_t::WAIT_4TH_STAGE));
				abstract_rf_t *arf = rid.getARF();

				d->setStuckSrc(3-(cur_stage-dynamic_inst_t::WAIT_4TH_STAGE));

				arf->writeInt64(rid,arf->readInt64(rid));

			} else if (cur_stage==dynamic_inst_t::EARLY_STORE_STAGE || 
					cur_stage==dynamic_inst_t::EARLY_ATOMIC_STAGE) {
				bool already_set=false;
				// Unlike the wait_nth_stage above, we don't know which reg is !ready

				for (int i=0;i<SI_MAX_SOURCE;i++) {
					reg_id_t &rid=d->getSourceReg(i);
					if (!rid.getARF()->isReady(rid)) {
						abstract_rf_t *arf = rid.getARF();
						if (!already_set) {
							already_set=true;
							d->setStuckSrc(i);
						}
						if (rid.getRtype()==RID_CONTAINER) {
							reg_id_t &subid=((arf_container_t *)rid.getARF())->getUnreadySubId(rid);
							subid.getARF()->writeInt64(subid,subid.getARF()->readInt64(subid));
						} else {
							arf->writeInt64(rid,arf->readInt64(rid));
						}
					} 
				}
			} else {
				//DEBUG_OUT("neither, wait stage %s\n",d->printStage(cur_stage));
			}
		}

		if (!ROLLBACK_AT_SYMPTOM) {
			// LXL FIXME: may need to take special care for RED state or something
			if ((m_last_symptom!=0x5) && (m_last_symptom!=0x29)) {
				m_trigger_recovery=false;
			}
		}

#ifdef SW_ROLLBACK
		m_op_mode=NORMAL;
#endif
	}

	// Use the following to test out recovery mechanism
	// int rollback_at = 1050000 ;
	// if (m_local_sequence_number>rollback_at && !recovered) {
	//     m_trigger_recovery=true;
	//     DEBUG_OUT("at cycle %lld, recovery trigger\n",getLocalCycle());
	// }
	// if(m_local_sequence_number > rollback_at+1000000) {
	// 	HALT_SIMULATION ;
	// }

	if (m_trigger_recovery) {
		//if (recovered) return;

		if (USE_TBFD && !recovered && (0x2==m_last_symptom) ) {
			m_diagnosis->setExcludeDatapath();
		}

		if (recovered) {
			uint64 rollback_interval=m_local_sequence_number-last_rollback_time;
			if (rollback_interval==time_from_rollback) {
				steady_state++;
			} else {
				steady_state=0;
				time_from_rollback=rollback_interval;
			}
			last_rollback_time=m_local_sequence_number;
			if (steady_state>100) {
				HALT_SIMULATION;
			}
		}


		DEBUG_OUT("%lld instructions, rollback state\n",m_local_sequence_number);
		rollbackRecovery();
		DEBUG_OUT("done rollback\n");
		m_trigger_recovery=false;


		// If want to make fault disappear after recovery, use below
		//fault_stat->setFaultInjInst(100000000);

		if (USE_TBFD) {
			m_diagnosis->setRecoveryInfo(getLocalCycle(),m_local_sequence_number);
			m_diagnosis->resetInstrTrace();
		}
		if(!m_multicore_diagnosis->isLoggingPhase() && !m_multicore_diagnosis->isTMRPhase())
			recovered=true;

#ifdef SW_ROLLBACK
		m_op_mode=RECOVERING;
#endif

#if HALT_AT_ROLLBACK
		// printArchState() ;
		HALT_SIMULATION;
#endif
	}


#ifdef SW_ROLLBACK
	if (m_op_mode==RECOVERED) {
		check_result_t check_result;

		DEBUG_OUT("finished recovering\n");
		//validateRecovery();
		//check_result.verbose=true;

		checkAllState( &check_result, &m_ooo, true );
		recovered=true;
		*m_fetch_at = *m_inorder_at;

		recoverTLB();
		m_ruby_cache->notifySNETRollback();

		m_trigger_recovery=false;

		//checkRegisterReady();

		FAULT_OUT("cut here\n");
		m_op_mode=NORMAL;

	}
#endif

}

void pseq_t::updateInstrInfo(instruction_information_t&instr_info, dynamic_inst_t*d)
{
	instr_info.VPC=d->getVPC();
	instr_info.opcode = d->getStaticInst()->getOpcode();
	instr_info.seq_num = d->getSequenceNumber();
	instr_info.isMem = ((dynamic_cast<memory_inst_t*>(d))!=NULL);
	instr_info.isTrap = (d->getTrapType()!=Trap_NoTrap);
	instr_info.immediate = d->getStaticInst()->getImmediate();
	instr_info.iword = d->getStaticInst()->getInst();
	instr_info.isStuck = d->isStuck();
	instr_info.stuckSrc = d->getStuckSrc();

	//DEBUG_OUT("insert instr %s\n",decode_opcode(instr_info.opcode));

	// Info of source registers
	for (int i=0;i<SI_MAX_SOURCE;i++) {
		reg_id_t &reg=d->getSourceReg(i);

		instr_info.source[i].rid = reg;
		instr_info.source[i].value = reg.getARF()->readInt64(reg);
		instr_info.source[i].mismatch = false;

		reg_id_t &sreg=d->getStaticInst()->getSourceReg(i);
		instr_info.s_source[i] = sreg;
		uint16 cwp_gset=reg.getVanillaState();
		instr_info.s_source[i].setVanillaState(cwp_gset,cwp_gset);

	}

	// Info of dest registers
	for (int i=0;i<SI_MAX_DEST;i++) {
		reg_id_t &reg=d->getDestReg(i);

		instr_info.dest[i].rid = reg;
		instr_info.dest[i].value = reg.getARF()->readInt64(reg);
		instr_info.dest[i].mismatch = false;

		reg_id_t &sreg=d->getStaticInst()->getDestReg(i);
		instr_info.s_dest[i] = sreg;
		uint16 cwp_gset=reg.getVanillaState();
		instr_info.s_dest[i].setVanillaState(cwp_gset,cwp_gset);

	}

	uarch_info_t &u_info=instr_info.uarch_info;

	u_info.fetch_id = d->getStaticInst()->getFetchId();
	u_info.win_id = d->getWindowIndex();
	u_info.exec_unit = d->getExecUnit();
	u_info.exec_unit_id = d->getExecUnitNum();
	u_info.res_bus = d->getResBus();
	u_info.agu = d->getAGU();

}

bool pseq_t::isAppAbort(la_t pc)
{
	// This app abort is only for mysqld
	if( pc>=0x296ebc && pc<=0x297624) { 
		return true ;
	}
	return false ;
}

bool pseq_t::addLLBEntry(uint64 physical_address, uint64 value)
{
	return llb->addLLBEntry(physical_address, value);
}

//returns if LLB is full
// true - llb is full
// false - llb is not full
bool pseq_t::addLLBEntry(uint64 physical_address, uint64 *value, int num_values)
{
	//DEBUG_OUT("%d:",m_id);
	for(int i=0; i<num_values; i++) {
		//DEBUG_OUT("%d: Logging: Address=%llx Value=%llx \n", m_id, physical_address, value[i]);
		bool success = llb->addLLBEntry(physical_address, value[i]);
		if(success) {
			DEBUG_OUT("%d: Logging Failed \n", m_id);
			return true;
		}
	}
	if(SIVA_LOAD_BUFFER_DEBUG)
		DEBUG_OUT("%d: Logging: Address=%llx Value=%llx, size=%d\n", m_id, physical_address, value[0], llb->size());
	return false;
}

la_t pseq_t::panicPC()
{
	if((SOL9 && OPEN_SOL) || ((!SOL9) && (!OPEN_SOL))) {
		DEBUG_OUT(" specify OS properly\n");
		assert(0);
	}
	if(SOL9)
		return 0x100d9a0;
	if(OPEN_SOL)
		return 0x102300C;
}

#ifdef LLB_ANALYSIS
void pseq_t::find_and_add_in_directory(uint64 * values, int size)
{

	for(int j=0; j<size; j++) {
		bool found = false;
		for(int i=0; i<dummy_directory.size(); i++) {
			if(dummy_directory.at(i) == values[j]) {
				found = true;
				break;
			}
		}
		if(!found)
			dummy_directory.push_back(values[j]);
	}

	if(num_values%40000 >= 0 && num_values%40000 <= 3) 
		print_llb_analysis(); 
}

void pseq_t::print_llb_analysis()
{
	DEBUG_OUT("\n Core: %d\n",m_id);
	DEBUG_OUT(" LLB Analysis\n");
	DEBUG_OUT(" Directory size: %d\n", dummy_directory.size());
	DEBUG_OUT(" Num values: %d\n", num_values);

	float compression = num_values/dummy_directory.size();
	DEBUG_OUT(" Compression: %f\n", compression);

	/*    for(int i=0; i<dummy_directory.size(); i++) {
		  DEBUG_OUT("%llx ", dummy_directory.at(i));
		  if(i%10 == 0)
		  DEBUG_OUT("\n");
		  } 
		  DEBUG_OUT("\n");
	 */
}

#endif //LLB_ANALYSIS


void pseq_t::readMemory(ireg_t log_addr, int size, ireg_t *result_reg)
{
	M_PSTATE->readMemory(log_addr, size, false, result_reg);
}

bool pseq_t::isKernelPanic(la_t pc)
{
	if((SOL9 && OPEN_SOL) || ((!SOL9) && (!OPEN_SOL))) {
		DEBUG_OUT(" specify OS properly\n");
		assert(0);
	}
	if(SOL9)
		return (pc==0x100d9a0) ;
	if(OPEN_SOL) {
		return ( ( (pc>=0x1022fd8) && (pc<=0x1023010)) || // panic_idle()
				( (pc>=0x1007988) && (pc<=0x1007c28)) || // ptl1_panic()
				( (pc>=0x100789c) && (pc<=0x10078a8)) || // sys_tl1_panic()
				( (pc>=0x101375c) && (pc<=0x101378c)) || // kdi_kernpanic()
				( (pc>=0x1022278) && (pc<=0x1022280)) || // bop_panic()
				( (pc>=0x1023014) && (pc<=0x102317c)) || // panic_stopcpus()
				( (pc>=0x10501a4) && (pc<=0x10501d0)) || // prom_panic()
				( (pc>=0x1065ee8) && (pc<=0x1065f50)) || // mutex_panic()
				( (pc>=0x1068d98) && (pc<=0x1069378)) || // panicsys()
				( (pc>=0x106b368) && (pc<=0x106b3cc)) || // rw_panic()
				( (pc>=0x1049bac) && (pc<=0x1049c84)) // vpanic() panic handler
			   ) ;
		// return (pc==0x102300c); // This is inside panic_idle_cpu()
	}
}

ireg_t pseq_t::getSimicsNPC() {
	return M_PSTATE->getControl(CONTROL_NPC) ;
}

#ifdef BUILD_SLICE

// Function to keep track of the dyanmic forward slice
void pseq_t::slice(dynamic_inst_t* inst)
{
	if(!inst) {
		return ;
	}

	if(inst->getPrivilegeMode()) {
		if(DEBUG_SLICE && VERBOSE) {
			DEBUG_OUT("%d is priv inst\n", inst->getSequenceNumber()) ;
		}
		return ;
	}

	dyn_execute_type_t type = inst->getStaticInst()->getType() ;
	bool is_ctrl = (type==DYN_CONTROL) ;
	bool is_mem = (type==DYN_LOAD || type==DYN_STORE || type==DYN_PREFETCH || type==DYN_ATOMIC) ;

	// control_inst_t *is_ctrl = dynamic_cast<control_inst_t*>(inst) ;
	// memory_inst_t *is_mem = dynamic_cast<memory_inst_t*>(inst) ;

	if(is_ctrl || is_mem) {
		if(!fault_list.empty()) {
			if(DEBUG_SLICE) {
				inst->printRetireTrace("Remove bkwd slice of ") ;
			}
			reg_list.clear() ;
			addr_list.clear() ;
			bool marked = markProducers(inst) ;
			// if(marked) {
			// 	deleteMarkedFromSlice() ;
			// }
		}
	} 
	// First, account for dynamically dead instructions
	bool marked = findDeadInstructions(inst) ;
	// if(marked) {
	// 	deleteMarkedFromSlice() ;
	// }

	// Then, add instructions' destination as potential data faults
	// Add only for the first SLICE_FAULTS_TILL instructions
	// if(inst->getSequenceNumber()<=SLICE_FAULTS_TILL) {
		if(!is_ctrl) {
			bool is_st = (type==DYN_STORE) || (type==DYN_ATOMIC) ;
			if(is_st) {
				fault_t *addr_fault = new fault(inst, INVALID_REG, 0, 0) ;
				last_addr_fault[addr_fault->address] = addr_fault->getFaultId() ;
				if(DEBUG_SLICE) {
					addr_fault->print("Adding") ;
				}
				fault_list[addr_fault->getFaultId()] = addr_fault ;
			}

			// Insert a new fault for each <inst,reg> pair
			for( int i=0; i< SI_MAX_DEST; i++ ) {
				reg_id_t &reg = inst->getDestReg(i) ;
				if( !reg.isZero() ) {
					half_t dest = reg.getFlatRegister() ;
					fault_t *dest_fault = new fault(inst, dest, reg.getPhysical(), (int)reg.getRtype()) ;
					dest_fault->setCycle(getLocalCycle()) ;
					if(DEBUG_SLICE) {
						dest_fault->print("Adding") ;
					}
					// fault_list.push_back(dest_fault) ;
					fault_list[dest_fault->getFaultId()] = dest_fault ;
					last_reg_fault[dest] = dest_fault->getFaultId() ;
					if(inst->getSequenceNumber()<=SLICE_FAULTS_TILL) {
						all_reg_faults ++ ;
					}
				}
			}
		} else {
			// For ctrl instructions, faults in dest regs are trivially detected.
			// However, we should account for them too.
			for( int i=0; i< SI_MAX_DEST; i++ ) {
				reg_id_t &reg = inst->getDestReg(i) ;
				if( !reg.isZero() ) {
					if(inst->getSequenceNumber()<=SLICE_FAULTS_TILL) {
						all_reg_faults ++ ;
					}
				}
			}
		}
	// }

	// // Update the last_producer data structure
	// for( int i=0; i< SI_MAX_DEST; i++ ) {
	// 	reg_id_t &reg = inst->getDestReg(i) ;
	// 	if( !reg.isZero() ) {
	// 		half_t dest = reg.getFlatRegister() ;
	// 		last_producer[dest] = inst->getSequenceNumber() ;
	// 	}
	// }

#ifdef BUILD_DDG
	// This instruction now forms a new node in the DDG. It is a child to all its
	// sources which are parents of this node.
	if(!is_ctrl) {
		ddg_node_t *ddg_node = new ddg_node_t(inst->getSequenceNumber(), getLocalCycle(), inst->getVPC() ) ;
		ddg_node_map[inst->getSequenceNumber()] = ddg_node ;
		bool has_source = false ;
		for(int i=0; i<SI_MAX_SOURCE; i++ ) {
			reg_id_t &reg = inst->getSourceReg(i) ;
			if(!reg.isZero()) {
				has_source = true ;
				half_t src = reg.getFlatRegister() ;
				if(ddg_last_producer.find(src)!=ddg_last_producer.end()) {
					// Then, the source is found
					uint64 src_inst = ddg_last_producer[src] ;
					if(ddg_node_map.find(src_inst) != ddg_node_map.end()) {
						ddg_node_t *src_node = ddg_node_map[src_inst] ;
						ddg_node->addParent(src_node) ;
						src_node->addChild(ddg_node) ;
					}
				} else {
					ddg_node->addParent(ddg_root) ;
					ddg_root->addChild(ddg_node) ;
				}
			}
		}
		if(!has_source) {
			ddg_node->addParent(ddg_root) ;
			ddg_root->addChild(ddg_node) ;
		}

		if ((type==DYN_STORE) || (type==DYN_ATOMIC)) {
			// This instruction is the last_store to this address
			memory_inst_t *load = dynamic_cast<memory_inst_t*>(inst) ;
			la_t addr = load->getAddress() ;
			ddg_last_store[addr] = ddg_node ;
		} else if(type==DYN_LOAD) {
			memory_inst_t *load = dynamic_cast<memory_inst_t*>(inst) ;
			la_t addr = load->getAddress() ;
			if(ddg_last_store.find(addr) != ddg_last_store.end()) {
				ddg_node_t *st_node= ddg_last_store[addr] ;
				ddg_node->addParent(st_node) ;
				st_node->addChild(ddg_node) ;
			}
		}
		// This DDG node produces the destination regs of this instruction
		for( int i=0; i< SI_MAX_DEST; i++ ) {
			reg_id_t &reg = inst->getDestReg(i) ;
			if( !reg.isZero() ) {
				half_t dest = reg.getFlatRegister() ;
				ddg_last_producer[dest] = inst->getSequenceNumber() ;
			}
		}
	}
#endif // BUILD_DDG
	// printDataSlice() ;
}

// Mark the producers of this instruction to be removed
bool pseq_t::markProducers(dynamic_inst_t* inst)
{
	slice_inst_t *slice_inst = new slice_inst_t(inst) ;
	bool marked = markProducers(slice_inst) ;
	delete slice_inst ;
	return marked ;
}

bool pseq_t::markProducers(slice_inst* inst)
{
	flush(cout) ;
	bool marked = false ;
	// markProducers may be called for some fault in the middle of the slice if an
	// fault is dynamically dead. In that case, we should start considering producers only
	// from before the given fault. this work also for current as this is always true for current.
	uint64 start_seq = inst->seq ;

	if(DEBUG_SLICE) {
		inst->print("markProducers() of ") ;
		flush(cout) ;
	}

	// Each slice_inst has a src_producer fault data entry. Use that to go back on
	// the instruction's producers.

	for(int i=0; i<SI_MAX_SOURCE; i++) {
		if((inst->type==DYN_STORE||inst->type==DYN_ATOMIC) && i==2 ) {
			// These guys use src[2] as the data value. That is handled separately.
			continue ;
		}
		uint64 src_fault = inst->src_producer_fault[i] ;
		if(src_fault != 0) {
			if(DEBUG_SLICE && VERBOSE) {
				DEBUG_OUT("Checking reg_fault %d\n", src_fault) ;
			}
			flush(cout) ;
			// If this fault hasn't already been deleted, then delete it.
			if(deleted_faults.find(src_fault)==deleted_faults.end()) {
				slice_inst *src_producer_inst = fault_list[src_fault]->inst ;
				marked |= markProducers(src_producer_inst) ;
				// Now, if the source was a load, then the memory value is used
				if(src_producer_inst->type==DYN_LOAD || src_producer_inst->type==DYN_ATOMIC) {
					uint64 addr_fault = src_producer_inst->addr_producer_fault ;
					if(addr_fault!=0) {
						if(DEBUG_SLICE && VERBOSE) {
							DEBUG_OUT("Checking addr_fault %d\n",addr_fault) ;
						}
						flush(cout) ;
						if( deleted_faults.find(addr_fault)==deleted_faults.end()) {
							slice_inst *addr_producer_inst = fault_list[addr_fault]->inst ;
							marked |= markDataProducers(addr_producer_inst) ;
							deleted_faults.insert(addr_fault) ;
							if(DEBUG_SLICE) {
								addr_producer_inst->print("Erasing") ;
								flush(cout) ;
							}
							fault_list.erase(addr_fault) ;
						}
					}
				}
				if(DEBUG_SLICE) {
					src_producer_inst->print("Erasing") ;
					flush(cout) ;
				}
				deleted_faults.insert(src_fault) ;
				fault_list.erase(src_fault) ;
				marked = true ;
			}
		}
	}

	// bool added_regs = addRegsToSlice(inst) ;
	// bool added_reg = false ;
	// bool added_addr = false ;
	// if(added_regs || !addr_list.empty()) {
	// 	for(fault_list_t::reverse_iterator RI=fault_list.rbegin(), RE=fault_list.rend();
	// 			RI!=RE; RI++) {
	// 		fault_t *this_fault = *RI ;
	// 		added_regs = false ;
	// 		added_addr = false ;
	// 		if(this_fault->inst->seq <= start_seq) {
	// 			if(!this_fault->isMarked()) {
	// 				for(reg_list_t::iterator I=reg_list.begin(),E=reg_list.end(); I!=E; I++) {
	// 					half_t reg = *I ;
	// 					if(this_fault->reg_valid && (this_fault->reg == reg)) {
	// 						if(DEBUG_SLICE) {
	// 							this_fault->print("Marking") ;
	// 						}
	// 						// if(this_fault->inst->seq==592916) {
	// 						// 	this_fault->print("Marking") ;
	// 						// }
	// 						// This instruction produces this reg. So mark this inst, remove this reg
	// 						marked = true ;
	// 						this_fault->markToDel() ;
	// 						reg_list.erase(I) ;
	// 						added_regs = addRegsToSlice(this_fault) ;
	// 						if(this_fault->address!=0x0) {
	// 							added_addr = addAddrToSlice(this_fault) ;
	// 						}
	// 						break ;
	// 					}
	// 				}
	// 				for(addr_list_t::iterator II=addr_list.begin(), EE=addr_list.end(); II!=EE; II++) {
	// 					la_t addr = *II ;
	// 					if(this_fault->address == addr &&
	// 							(this_fault->inst->type==DYN_STORE||this_fault->inst->type==DYN_ATOMIC)) {
	// 						if(DEBUG_SLICE) {
	// 							this_fault->print("Marking*") ;
	// 						}
	// 						// if(this_fault->inst->seq==592916) {
	// 						// 	this_fault->print("Marking") ;
	// 						// }
	// 						// This instruction stores at this address. So remove this inst, and address
	// 						marked = true ;
	// 						markToDel(this_fault) ;
	// 						addr_list.erase(II) ;
	// 						added_regs |= addDataRegsToSlice(this_fault) ;
	// 						break ;
	// 					}
	// 				}
	// 			}
	// 		}
	// 		// If we haven't added any regs or addr in this iteration, and if all regs'
	// 		// and addresses' sources have been identified, no need to continue with rest
	// 		// of instr list.
	// 		if(reg_list.empty() && addr_list.empty() && !added_regs && !added_addr) {
	// 			if(DEBUG_SLICE && VERBOSE) {
	// 				DEBUG_OUT("All Regs, addrs' sources found. Done with marking\n") ;
	// 			}
	// 			break ;
	// 		}
	// 	}
	// }

	if(DEBUG_SLICE) {
		DEBUG_OUT("Done markProducers()\n") ;
	}
	flush(cout) ;
	return marked ;
}

bool pseq_t::markDataProducers(slice_inst *inst)
{
	if(DEBUG_SLICE) {
		inst->print("markDataProducers()") ;
	}
	bool marked = false ;
	if(inst->type!=DYN_STORE && inst->type!=DYN_ATOMIC) {
		inst->print("Error: markDataProducers() called for non-store, non-atomic inst ") ;
	}
	uint64 src_data_producer_fault = inst->src_producer_fault[2] ;
	if(DEBUG_SLICE && VERBOSE) {
		DEBUG_OUT("Data producer = %d\n", src_data_producer_fault) ;
	}
	if(src_data_producer_fault!=0 && deleted_faults.find(src_data_producer_fault)==deleted_faults.end()) {
		slice_inst *producer_inst = fault_list[src_data_producer_fault]->inst ;
		marked |= markProducers(producer_inst) ;
		if(producer_inst->type==DYN_LOAD || producer_inst->type==DYN_ATOMIC) {
			uint64 addr_fault = producer_inst->addr_producer_fault ;
			if(addr_fault!=0) {
				if(DEBUG_SLICE && VERBOSE) {
					DEBUG_OUT("Checking addr_fault %d\n",addr_fault) ;
				}
				flush(cout) ;
				if( deleted_faults.find(addr_fault)==deleted_faults.end()) {
					slice_inst *addr_producer_inst = fault_list[addr_fault]->inst ;
					marked |= markDataProducers(addr_producer_inst) ;
					deleted_faults.insert(addr_fault) ;
					if(DEBUG_SLICE) {
						addr_producer_inst->print("Erasing") ;
						flush(cout) ;
					}
					fault_list.erase(addr_fault) ;
				}
			}
		}
		deleted_faults.insert(src_data_producer_fault) ;
		if(DEBUG_SLICE) {
			producer_inst->print("Erasing") ;
			flush(cout) ;
		}
		fault_list.erase(src_data_producer_fault) ;
		marked = true ;
	}
	return marked ;
}

/***********************************************************************
** ///////
** // XXX - Deprecated slice functions. vector<> implementation is just slow
** /////
** 
** // Add ths sources of the given instruction to a list which ctrls which
** // instructions on the backward slice have to be removed
** bool pseq_t::addRegsToSlice(fault_t *fault)
** {
** 	return addRegsToSlice(fault->inst) ;
** }
** 
** 
** // Try and add this inst's regs to the slice. If nothing is added, return false.
** bool pseq_t::addRegsToSlice(slice_inst_t *inst)
** {
** 	bool added_new_regs = false ;
** 	// inst->printRetireTrace("Am I st?") ;
** 	bool is_st = (inst->type==DYN_STORE) ;
** 	int pos = -1 ;
** 	for(reg_list_t::iterator I=inst->src_regs.begin(), E=inst->src_regs.end();
** 			I!=E; I++) {
** 		half_t flat = *I ;
** 		pos++ ;
** 		if(flat!=INVALID_REG) {
** 			// If this source didn't have a producer when we saw it, or its producer was subsequently
** 			// removed, then this source needn't be back propagated
** 			// DEBUG_OUT("Trying to add r%d, prod=%lld, not_deleted=%d\n",
** 			// 		flat,
** 			// 		inst->src_producer[pos],
** 			// 		deleted_instructions.find(inst->src_producer[pos])==deleted_instructions.end()) ;
** 			if(inst->src_producer[pos]!=0) {
** 				bool src_deleted = false ;
** 				if(deleted_instructions.find(inst->src_producer[pos]) != deleted_instructions.end()) {
** 					half_t deleted_reg = deleted_instructions[inst->src_producer[pos]] ;
** 					if(deleted_reg == flat) {
** 						src_deleted = true ;
** 					}
** 				}
** 				if(!src_deleted) {
** 					// For stores, s2 is used as the data. Faults in data are
** 					// not identified. Loads don't have s2.
** 					if(!is_st || pos!=2) {
** 						bool found = false ;
** 						for(reg_list_t::iterator II=reg_list.begin(), EE=reg_list.end();
** 								II!=EE; II++) {
** 							if(*II == flat) {
** 								found = true ;
** 								break ;
** 							}
** 						}
** 						if(!found) {
** 							reg_list.push_back(flat) ;
** 							added_new_regs = true ;
** 						}
** 					}
** 				}
** 			}
** 		}
** 	}
** 	
** 	if(DEBUG_SLICE && VERBOSE) {
** 		DEBUG_OUT("sliceregs = ") ;
** 		for(reg_list_t::iterator I=reg_list.begin(), E=reg_list.end(); I!=E; I++) {
** 			DEBUG_OUT("r%d,", *I) ;
** 		}
** 		DEBUG_OUT("\n") ;
** 	}
** 	return added_new_regs ;
** }
** 
** // Try and add this inst's regs to the slice. If nothing is added, return false.
** bool pseq_t::addDataRegsToSlice(fault_t *fault)
** {
** 	slice_inst_t *inst = fault->inst ;
** 	bool is_st = (fault->inst->type==DYN_STORE) || (fault->inst->type==DYN_ATOMIC) ;
** 	bool added_new_regs = false ;
** 	if(is_st) {
** 		half_t flat = inst->src_regs[2] ;
** 		// If this source didn't have a producer when we saw it, or its producer was subsequently
** 		// removed, then this source needn't be back propagated
** 		if(inst->src_producer[2]!=0) {
** 			bool src_deleted = false ;
** 			if(deleted_instructions.find(inst->src_producer[2]) != deleted_instructions.end()) {
** 				half_t deleted_reg = deleted_instructions[inst->src_producer[2]] ;
** 				if(deleted_reg == flat) {
** 					src_deleted = true ;
** 				}
** 			}
** 			if(!src_deleted) {
** 				bool found = false ;
** 				for(reg_list_t::iterator I=reg_list.begin(), E=reg_list.end();
** 						I!=E; I++) {
** 					if(*I == flat) {
** 						found = true ;
** 						break ;
** 					}
** 				}
** 				if(!found) {
** 					reg_list.push_back(flat) ;
** 					added_new_regs = true ;
** 				}
** 			}
** 		}
** 	}
** 	
** 	if(DEBUG_SLICE && VERBOSE) {
** 		DEBUG_OUT("sliceregs = ") ;
** 		for(reg_list_t::iterator I=reg_list.begin(), E=reg_list.end(); I!=E; I++) {
** 			DEBUG_OUT("r%d,", *I) ;
** 		}
** 		DEBUG_OUT("\n") ;
** 	}
** 	return added_new_regs ;
** }
** 
** 
** // If this instruction is a load instruction, then the instructin that produces
** // the data @ the mem address shoudl also be transitively marked to be deleted.
** bool pseq_t::addAddrToSlice(fault_t *fault)
** {
** 	bool added_address = false ;
** 	dyn_execute_type_t type = fault->inst->type ;
** 	if(type==DYN_LOAD || type==DYN_PREFETCH || type==DYN_ATOMIC) {
** 		addr_list.push_back(fault->address) ;
** 		added_address = true ;
** 	}
** 	if(DEBUG_SLICE && VERBOSE) {
** 		DEBUG_OUT("sliceaddr = ") ;
** 		for(addr_list_t::iterator I=addr_list.begin(), E=addr_list.end(); I!=E; I++) {
** 			DEBUG_OUT("0x%llx,", *I) ;
** 		}
** 		DEBUG_OUT("\n") ;
** 	}
** 	return added_address ;
** }
** 
** // Faults can be detected only within MAX_PROPAGATION_LATENCY instructions
** // So mark them only if curr inst-seq < MAX_PROPAGATION_LATENCY
** void pseq_t::markToDel(fault_t *fault)
** {
** 	if(GET_RET_INST()-fault->inst->seq <= MAX_PROPAGATION_LATENCY) {
** 		fault->markToDel() ;
** 	}
** }
**************************************************************************************/

// Find dynamically dead instructions
bool pseq_t::findDeadInstructions(dynamic_inst_t *inst)
{
	if(DEBUG_SLICE) {
		DEBUG_OUT("findDeadInstructions() with %d\n", inst->getSequenceNumber()) ;
	}
	slice_inst_t *slice_inst = new slice_inst_t(inst) ;
	bool marked = findDeadInstructions(slice_inst) ;
	delete slice_inst ;
	return marked ;
}

bool pseq_t::findDeadInstructions(slice_inst_t *inst)
{
	bool marked = false ;

	// First, all the sources of this instructions are now read
	for(reg_list_t::iterator I=inst->src_regs.begin(), E=inst->src_regs.end();
			I!=E; I++) {
		half_t src = *I ;
		if(src != INVALID_REG) {
			for(reg_list_t::iterator II=unread_regs.begin(); II!=unread_regs.end();II++) {
				half_t reg = *II ;
				if(reg == src) {
					unread_regs.erase(II) ;
					break ;
				}
			}
		}
	}

	// Second, all the destination's previous producers are dead if unread
	reg_list_t dead_regs ;
	for(reg_list_t::iterator I=inst->dest_regs.begin(), E=inst->dest_regs.end();
			I!=E; I++) {
		half_t dest = *I ;
		if(dest!=INVALID_REG) {
			for(reg_list_t::iterator II=unread_regs.begin(); II!=unread_regs.end(); II++) {
				half_t unread_reg = *II ;
				if(dest==unread_reg) {
					// Then, the instruction that produced reg is dynamically dead!
					dead_regs.push_back(dest) ;
					if(DEBUG_SLICE) {
						DEBUG_OUT("r%d is dead\n", dest) ;
					}
					unread_regs.erase(II) ;
					break ;
				}
			}
		}
	}

	// And, this instruction's destinations haven't been read yet. 
	// We track dead regs only till the slice latency. Afterward, we don't care
	if(inst->seq <= SLICE_FAULTS_TILL) {
		for(reg_list_t::iterator I=inst->dest_regs.begin(), E=inst->dest_regs.end();
				I!=E; I++) {
			half_t dest = *I ;
			if(dest!=INVALID_REG) {
				unread_regs.push_back(dest) ;
			}
		}
	}

	// Finally, If there are any dead regs, mark the producers to delete
	for(reg_list_t::iterator I=dead_regs.begin(),E=dead_regs.end(); I!=E; I++) {
		half_t dead_reg = *I ;
		if(DEBUG_SLICE) {
			DEBUG_OUT("r%d is dead\n", dead_reg) ;
		}
		uint64 dead_fault = last_reg_fault[dead_reg] ;
		if(dead_fault!=0 && deleted_faults.find(dead_fault)==deleted_faults.end()) {
			slice_inst *dead_inst = fault_list[dead_fault]->inst ;
			marked |= markProducers(dead_inst) ;
			deleted_faults.insert(dead_fault) ;
			if(DEBUG_SLICE) {
				dead_inst->print("Erasing\n") ;
			}
			fault_list.erase(dead_fault) ;
		}
	}
	// for(fault_list_t::reverse_iterator I=fault_list.rbegin(), E=fault_list.rend();
	// 		I!=E; I++) {
	// 	fault_t *this_fault = *I ;
	// 	if(dead_regs.empty()) {
	// 		break ;
	// 	}
	// 	if(this_fault->reg_valid) {
	// 		half_t fault_reg = this_fault->reg ;
	// 		for(reg_list_t::iterator II=dead_regs.begin(), EE=dead_regs.end();
	// 				II!=EE; II++) {
	// 			half_t dead_reg = *II ;
	// 			if(dead_reg == fault_reg) {
	// 				if(DEBUG_SLICE) {
	// 					this_fault->print("Dead:") ;
	// 				}
	// 				reg_list.clear() ;
	// 				addr_list.clear() ;
	// 				marked |= markProducers(this_fault->inst) ;
	// 				this_fault->markToDel() ;
	// 				marked = true ;
	// 				dead_regs.erase(II) ;
	// 				break ;
	// 			}
	// 		}
	// 	}
	// }

	// Now, deal with memory instructions
	// XXX - Assume that atomic instructions are read-modify-write insts
	dyn_execute_type_t type = inst->type ;
	bool is_load = (type==DYN_LOAD) ;
	bool is_store = (type==DYN_STORE) || (type==DYN_ATOMIC) ;
	
	if(is_load || is_store) {
		la_t addr = inst->address ;
		bool found = false ;
		for(addr_list_t::iterator I=unread_addr.begin(), E=unread_addr.end();
				I!=E; I++) {
			la_t mem_addr = *I ;
			if(addr == mem_addr) {
				if(is_store) {
					// If the mem addr to which the store writes is unread, then the
				// inst that produces it is dead
					if(DEBUG_SLICE) {
						DEBUG_OUT("0x%llx is dead\n", addr) ;
					}
					found = true ;
				}
				// Loads make a previously unread address now read
				unread_addr.erase(I) ;
				break ;
			}
		}
		if(found) {
			if(DEBUG_SLICE) {
				DEBUG_OUT("0x%llx is dead\n", inst->address) ;
			}
			uint64 dead_fault = last_addr_fault[inst->address] ;
			if(dead_fault!=0 && deleted_faults.find(dead_fault)==deleted_faults.end()) {
				slice_inst *dead_inst = fault_list[dead_fault]->inst ;
				marked |= markDataProducers(dead_inst) ;
				deleted_faults.insert(dead_fault) ;
				if(DEBUG_SLICE) {
					dead_inst->print("Erasing\n") ;
				}
				fault_list.erase(dead_fault) ;
			}

			// for(fault_list_t::reverse_iterator I=fault_list.rbegin(); I!=fault_list.rend(); I++) {
			// 	fault_t *this_fault = *I ;
			// 	la_t this_addr = this_fault->address ;
			// 	if( this_addr==addr && ( type==DYN_STORE || type==DYN_ATOMIC)) {
			// 		// Then, this inst and its producers are dead
			// 		if(DEBUG_SLICE) {
			// 			this_fault->print("Dead:") ;
			// 		}
			// 		reg_list.clear() ;
			// 		addr_list.clear() ;
			// 		addr_list.push_back(this_addr) ;
			// 		marked |= markProducers(this_fault->inst) ;
			// 		this_fault->markToDel() ;
			// 		marked = true ;
			// 		break ;
			// 	}
			// }
		}
		if(inst->seq<=SLICE_FAULTS_TILL && is_store) {
			unread_addr.push_back(addr) ;
		}
	}
	return marked ;
}


/*****************************************
** XXX Deprecated
** 
** // Remove all those instructions marked to delete
** void pseq_t::deleteMarkedFromSlice()
** {
** 	if(DEBUG_SLICE) {
** 		DEBUG_OUT("start deleteMarkedFromSlice() size=%d\n", fault_list.size() );
** 	}
** 
** 	// Erasing in-situ. Erasing invalidates the current ptr. So, decrement first!
** 	for(fault_list_t::iterator I=fault_list.begin(); I!=fault_list.end(); I++) {
** 		fault_t *this_fault = *I ;
** 		if(this_fault->isMarked()) {
** 			if(DEBUG_SLICE) {
** 				this_fault->print("Erasing") ;
** 			}
** 			// if(this_fault->inst->seq==592916) {
** 			// 	this_fault->print("Erasing") ;
** 			// }
** 			slice_inst_t *inst = this_fault->inst ;
** 			// deleted_instructions.insert(this_fault->inst->seq) ;
** 			if(this_fault->reg_valid) {
** 				deleted_instructions[this_fault->inst->seq] = this_fault->reg ;
** 			}
** 			I-- ;
** 			delete this_fault ;
** 			fault_list.erase(I+1) ;
** 		}
** 	}
** 
** 	if(DEBUG_SLICE) {
** 		DEBUG_OUT("end deleteMarkedFromSlice() size=%d\n", fault_list.size() );
** 	}
** }
**************************************/

void pseq_t::printDataSlice()
{
	int num_mem = 0 ;
	int num_other = 0 ;
	
	int slice_ops[(int)i_maxcount] ;
	for(int i=0;i<(int)i_maxcount;i++) {
		slice_ops[i] = 0 ;
	}

	DEBUG_OUT("\n") ;
	DEBUG_OUT("#### SLICE of data-only instrs ####\n") ;
	unsigned int slice_size = 0 ;
	for(fault_list_t::iterator I=fault_list.begin(), E=fault_list.end();
			I!=E; I++) {
		fault_t *this_fault = (*I).second ;
		if(this_fault->inst->seq<=SLICE_FAULTS_TILL) {
			this_fault->print("") ;
			// The stores that are on the fault_list are essentially to just help
			// with tracking faults through memory. The real faults that we are after
			// are just register faults in non-mem inst. Count only them.
			if(this_fault->reg_valid) { 
				switch(this_fault->inst->type) {
					case DYN_STORE:
					case DYN_LOAD:
					case DYN_ATOMIC:
						num_mem ++ ;
						break ;
					default:
						num_other ++ ;
				}
				slice_ops[this_fault->inst->opcode] ++ ;
				slice_size++ ;

				// Try and reduce this fault set by using notion of dominance, etc.
				addToReducedFaultSet(this_fault) ;
			}
		}
	}
	DEBUG_OUT("\n") ;

	DEBUG_OUT("#### SLICE stats ####\n") ;
	DEBUG_OUT("Slice Size\t%u\n", slice_size) ;
	DEBUG_OUT("Total Faults Considered\t%u\n", all_reg_faults) ;
	DEBUG_OUT("Memory faults\t%d\n", num_mem) ;
	DEBUG_OUT("Other faults\t%d\n", num_other) ;
	DEBUG_OUT("\n") ;

	DEBUG_OUT("#### SLICE opcodes ####\n") ;
	for(int i=0;i<(int)i_maxcount;i++) {
		if(slice_ops[i]!=0) {
			DEBUG_OUT("%s\t%d\n", decode_opcode((enum i_opcode)i), slice_ops[i]) ;
		}
	}
	DEBUG_OUT("\n") ;

	// DEBUG_OUT("## Unread regs: " );
	// for(reg_list_t::iterator I=unread_regs.begin(),E=unread_regs.end();
	// 		I!=E; I++) {
	// 	DEBUG_OUT("r%d,", *I) ;
	// }
	// DEBUG_OUT("\n## Unread addr: " );
	// for(addr_list_t::iterator I=unread_addr.begin(),E=unread_addr.end();
	// 		I!=E; I++) {
	// 	DEBUG_OUT("0x%llx,", *I) ;
	// }
	// DEBUG_OUT("\n\n") ;

	DEBUG_OUT("#### SLICE faults ####\n") ;
	int num_faults = 0 ;
	for(fault_list_t::iterator I=fault_list.begin(), E=fault_list.end();
			I!=E; I++) {
		fault_t *this_fault = (*I).second ;
		if(this_fault->inst->seq > SLICE_FAULTS_TILL) {
			break ;
		}
		// The stores that are on the fault_list are essentially to just help
		// with tracking faults through memory. The real faults that we are after
		// are just register faults in non-mem inst. Count only them.
		if(this_fault->reg_valid) {
			DEBUG_OUT("%lld\t%lld\t%d\t%d\n",
					this_fault->cycle,
					this_fault->inst->seq,
					this_fault->phys_reg,
					this_fault->reg_type) ;
			num_faults ++ ;
		}
	}
	DEBUG_OUT("Faults to inject\t%d\n", num_faults) ;

	DEBUG_OUT("\n#### Reduced Slice ####\n") ;
	for(fault_set_t::iterator I=reduced_fault_set.begin(), E=reduced_fault_set.end();
			I!=E; I++) {
		fault_t *this_fault = *I ;
		this_fault->print("") ;
	}
	DEBUG_OUT("\n#### Reduced Faults ####\n") ;
	for(fault_set_t::iterator I=reduced_fault_set.begin(), E=reduced_fault_set.end();
			I!=E; I++) {
		fault_t *this_fault = *I ;
		DEBUG_OUT("%lld\t%lld\t%d\t%d\n",
				this_fault->cycle,
				this_fault->inst->seq,
				this_fault->phys_reg,
				this_fault->reg_type) ;
	}
	DEBUG_OUT("Reduced Size\t%d\n", reduced_fault_set.size()) ;

	DEBUG_OUT("\n#### Equivalent Faults ####\n") ;
	for(seq_num_set_t::iterator I=reduced_fault_set_ids.begin(), E=reduced_fault_set_ids.end();
			I!=E; I++) {
		uint64 pc = (*I).first ;
		uint64_list_t eq_inst = (*I).second ;
#ifdef BUILD_DDG
		updateDDGStats(pc, eq_inst) ;
#endif // BUILD_DDG
#ifdef FIND_UNIQUE_PCS
		// int num_instances = 0 ;
		// if(retire_pcs.find(pc)!=retire_pcs.end()) {
		// 	num_instances = retire_pcs[pc] ;
		// }
		// DEBUG_OUT("0x%llx\t%d\t%d\n", pc, num_instances, eq_inst.size()) ;
#endif // FIND_UNIQUE_PCS
		// FIXME - Uncomment if you want to see the equivalent pcs
		// for(uint64_list_t::iterator II=eq_inst.begin(), EE=eq_inst.end();
		// 		II!=EE; II++) {
		// 	uint64 eq_seq = *II ;
		// 	if(eq_seq<=SLICE_FAULTS_TILL) {
		// 		DEBUG_OUT("%lld\t", *II) ;
		// 	}
		// }
		// DEBUG_OUT("\n") ;
	}
	DEBUG_OUT("#### Equivalent Faults End ####\n\n") ;

#ifdef BUILD_DDG
	DEBUG_OUT("DDG Size = %d\n", ddg_node_map.size()) ;
	DEBUG_OUT("#### Size of DDG of data-only faults ####\n") ;
	for(fault_list_t::iterator I=fault_list.begin(), E=fault_list.end();
			I!=E; I++) {
		fault_t *this_fault = (*I).second ;
		if(this_fault->inst->seq > SLICE_FAULTS_TILL) {
			break ;
		}
		if(this_fault->reg_valid) {
			uint64 seq = this_fault->inst->seq ;
			uint64 fanout = 0 ;
			uint64 propagation = 0 ;
			uint64 a_fanout = 0 ;
			uint64 a_propagation = 0 ;
			uint64 a_lifetime = 0 ;
			uint64 last_read_inst = seq ; // Set this as default
			// XXX this prints the stats for all the dynamic instructions
			if(ddg_node_map.find(seq)!=ddg_node_map.end()) {
				ddg_node_t *node = ddg_node_map[seq] ;
				// XXX - Removing these temporarily to keep the memory footprint of the jobs down
				// fanout = node->getNumChildren() ;
				// propagation = node->getDDGSize() ;
				// last_read_inst = node->getLastReadInst() ;
				// Now track the aggregate info
				uint64 pc = this_fault->inst->pc ;
				ddg_a_stats_t *a_stats = ddg_stats_map[pc] ;
				if(a_stats) {
					a_fanout = a_stats->a_fanout ;
					a_propagation = a_stats->a_size ;
					a_lifetime = a_stats->a_lifetime ;
				}
			}
			if(last_read_inst-seq < 0) {
				ERROR_OUT("Inst %d's dest is consumed by an earlier inst! Duh!\n") ;
			}

			DEBUG_OUT("D: r%d @%lld in:[%d 0x%llx]\t%u\t%u\t%u\t%u\t%u\t%u\n",
					this_fault->reg,
					this_fault->cycle,
					this_fault->inst->seq,
					this_fault->inst->pc,
					fanout,
					propagation,
					(last_read_inst-seq),
					a_fanout,
					a_propagation,
					a_lifetime) ;
		}
	}
	DEBUG_OUT("#### Size End ####\n\n") ;
#endif // BUILD_DDG
	fault_list.clear() ;
}

void pseq_t::addToReducedFaultSet(fault_t *fault)
{
	slice_inst *finst = fault->inst ;
	// XXX Optimization 1 - For any given producer-consumer set, only inject fault into producer
	// int pos = -1 ;
	// bool found = false ;
	// for(reg_list_t::iterator I=finst->src_regs.begin(), E=finst->src_regs.end();
	// 		I!=E; I++) {
	// 	pos ++ ;
	// 	half_t src = *I ;
	// 	if(src != INVALID_REG) {
	// 		uint64 producer = finst->src_producer[pos] ;
	// 		seq_num_set_t::iterator II = reduced_fault_set_ids.find(producer) ;
	// 		if(II!=reduced_fault_set_ids.end()) {
	// 			found = true ;
	// 			reduced_fault_set_ids[(*II).first].push_back(finst->seq) ;
	// 			break ;
	// 		}
	// 	}
	// }
	// if(!found) {
	// 	reduced_fault_set.insert(fault) ;
	// 	reduced_fault_set_ids[finst->seq].push_back(finst->seq) ;
	// }
	
	// XXX Optimization 2 - For any given pc, inject only one fault
	la_t pc = finst->pc ;
	if(reduced_fault_set_ids.find(pc)==reduced_fault_set_ids.end()) {
		reduced_fault_set.insert(fault) ;
	}
	reduced_fault_set_ids[pc].push_back(finst->seq) ;
}

#ifdef BUILD_DDG
void pseq_t::printReverseDDG()
{
	for(ddg_node_map_t::iterator I=ddg_node_map.begin(), E=ddg_node_map.end();
			I!=E; I++) {
		ddg_node_t *node = (*I).second ;
		if(node->isChildless()) {
			node->printReverseDDG() ;
		}
	}
}

void pseq_t::printDDG()
{
	DEBUG_OUT("#### DDG ####\n") ;
	ddg_root->printDDG() ;
}

void pseq_t::updateDDGStats(uint64 pc, uint64_list_t &L)
{
	uint64 fanout = 0 ;
	uint64 size = 0 ;
	uint64 lifetime = 0 ;
	uint64_set_t imm_children ;
	uint64_set_t subtree_nodes ;
	for(uint64_list_t::iterator I=L.begin(), E=L.end(); I!=E; I++) {
		uint64 seq = *I ;
		ddg_node_t *N = ddg_node_map[seq] ;
		uint64 oldest_child = 0 ;
		if(N->isChildless()) {
			// Each DDG node will be read by someone. So, childness nodes are simply an
			// artifact of our simulation. Account for this by making them their own
			// child
			imm_children.insert(N->seq) ;
			if(N->seq > oldest_child) {
				oldest_child = N->seq ;
			}
		} else {
			for(ddg_node_list_t::iterator II=N->children.begin(), EE=N->children.end();
					II!=EE; II++) {
				ddg_node_t *C = *II ;
				imm_children.insert(C->seq) ;
				if(C->seq > oldest_child) {
					oldest_child = C->seq ;
				}
				addSubTreeNodes(C, subtree_nodes) ;
			}
		}
		uint64 n_lifetime = oldest_child-seq ;
		if(oldest_child < seq) {
			n_lifetime = 0 ;
		}
		lifetime += n_lifetime ;
	}
	lifetime = lifetime/L.size() ;
	fanout = imm_children.size() ;
	size = subtree_nodes.size() ;
	// DEBUG_OUT("L: [0x%llx]\t%u\n", pc, lifetime) ;
	ddg_a_stats_t *stats = new ddg_a_stats_t(fanout,size,lifetime) ;
	ddg_stats_map[pc] = stats ;
}

void pseq_t::addSubTreeNodes(ddg_node_t *N, uint64_set_t &subtree_nodes)
{
	if(subtree_nodes.find(N->seq)!=subtree_nodes.end()) {
		// Which means that N was already seen, and so were its children
		return ;
	}
	subtree_nodes.insert(N->seq) ;
	for(ddg_node_list_t::iterator I=N->children.begin(), E=N->children.end();
			I!=E;I++) {
		ddg_node_t *C = *I ;
		addSubTreeNodes(C, subtree_nodes) ;
	}
}
#endif // BUILD_DDG

#endif // BUILD_SLICE

#ifdef FIND_UNIQUE_PCS
void pseq_t::addRetirePC(la_t pc)
{
	retire_pcs[pc] ++ ;
}

void pseq_t::printRetirePCs()
{
	// DEBUG_OUT("### Retire PCs ####\n") ;
	// for(retire_pcs_t::iterator I=retire_pcs.begin(), E=retire_pcs.end();
	// 		I!=E; I++) {
	// 	uint64 pc = (*I).first ;
	// 	int count = (*I).second ;
	// 	DEBUG_OUT("0x%llx\t%d\n", pc, count) ;
	// }
	DEBUG_OUT("\n# Unique PCs=%d\n", retire_pcs.size()) ;
}
#endif // FIND_UNIQUE_PCS

/*------------------------------------------------------------------------*/
/* Global Functions                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
static void pseq_mmu_reg_handler( void *pseq_obj, void *ptr,
        uint64 regindex, uint64 newcontext )
{
    pseq_t *pseq = (pseq_t *) pseq_obj;
    uint32  context_as_int = (uint32) newcontext;

    // CTXT_SWITCH
    pseq->contextSwitch( context_as_int );
}
