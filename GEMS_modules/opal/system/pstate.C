
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
 * FileName:  pstate.C
 * Synopsis:  Implements an in-order core's processors state
 * Author:    cmauer
 * Version:   $Id: pstate.C 1.64 06/02/13 18:49:20-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include "hfa.h"
#include "hfacore.h"
#include "pstate.h"

// The Simics API Wrappers
#include "interface.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define   REGISTER_BASE  128

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/** an array of boolean values that indicate if ASI's are cacheable or not */
char *pstate_t::m_asi_is_cacheable;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

static void pstate_breakpoint_handler( conf_object_t *cpu, void *parameter );

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
pstate_t::pstate_t( int id , multicore_diagnosis_t * m_diag) {
  m_id  = id;
  m_multicore_diagnosis = m_diag;
  m_advance_simics_exception = false;
  m_cpu = SIMICS_proc_no_2_ptr( id );

  // get the interface associated with this processor
  if (CONFIG_IN_SIMICS) {
    m_sparc_intf = (sparc_v9_interface_t*) SIMICS_get_interface( m_cpu, SPARC_V9_INTERFACE );
    
    char mmuname[200];
    pstate_t::getMMUName( m_id, mmuname, 200 );
    // printf("mmu name is %s\n",mmuname);
    m_conf_mmu = SIMICS_get_object( mmuname );
    if (m_conf_mmu == NULL) {
      ERROR_OUT("pstate_t: error: unable to locate object: %s\n", mmuname);
      SIM_HALT;
    }
  }
  hfa_checkerr("pstate: getContext: get mmu object");

  // initialize the core state of the processor to all zeros
  for(int i=0;i<NUM_CHKPTS;i++) {
	  memset( &m_state[i], 0, sizeof(core_state_t) );
  }
  curr_chkpt = -1 ;

  m_translation_cache = new AddressMapping();

  // read control register mappings
  m_control_map = (int32 *) malloc( (CONTROL_NUM_CONTROL_TYPES*2) * sizeof(int32) );
  for (int32 i = 0; i < (CONTROL_NUM_CONTROL_TYPES*2); i++) {
    m_control_map[i] = -1;
  }

  // determine how many of the control registers we actually map
  int32   max_control_reg   = MAX_CTL_REGS;
  m_valid_control_reg = (bool *) malloc( sizeof(bool) * max_control_reg );
  for (int32 i = 0; i < max_control_reg; i++) {
    m_valid_control_reg[i] = false;
  }
  
  if ( CONFIG_IN_SIMICS ) {
    attr_value_t regs = SIMICS_get_all_registers(m_cpu);
    if (regs.kind != Sim_Val_List) SIM_HALT;
    int reg_count = regs.u.list.size;
    SIMICS_free_attribute(regs);

    // verify our count of physical control registers is greater than simics's
    if ((uint64) reg_count > MAX_CTL_REGS) {
      DEBUG_OUT("pstate_t: verify simics warning: Opal has %d control regs (MAX_CTL_REGS).\n", MAX_CTL_REGS);
      DEBUG_OUT("pstate_t: while simics has %d control regs.\n", reg_count);
      SIM_HALT;
    }
    
    // read the simics control register map
    for ( int i = 0; i < CONTROL_NUM_CONTROL_PHYS; i ++ ) {
      if ( pstate_t::control_reg_menomic[i] == NULL ) {
        ERROR_OUT( "pstate_t: error: control register %d has improper menomic.\n", i );
      }
      int simmap = SIMICS_get_register_number( m_cpu,
                                            pstate_t::control_reg_menomic[i]);
      if ( simmap < 0 || simmap > max_control_reg ) {
        ERROR_OUT( "pstate_t: error: control register \"%s\" has improper mapping: %d.\n", pstate_t::control_reg_menomic[i], simmap);
      } else {
		  //		  DEBUG_OUT("reg %s has num %d\n",pstate_t::control_reg_menomic[i],simmap);
        m_valid_control_reg[simmap] = true;
      }
      hfa_checkerr( "pstate_t: constructor: unable to get control register number.\n");
      m_control_map[i] = simmap;
    }

    // compare the control registers that we are defining, versus those
    // that are implemented in simics
    for (int32 i = 0; i < max_control_reg; i++) {
      if (m_valid_control_reg[i] == false) {
        const char *regname = SIMICS_get_register_name(m_cpu, i);
        if (regname != NULL) {
          // our name for this control register
          const char* ctl_reg_menomic = "(null)";
          for(int32 j = 0; j < CONTROL_NUM_CONTROL_PHYS; j++) {
            if(m_control_map[j] == i) {
              ctl_reg_menomic = pstate_t::control_reg_menomic[j];
              break;
            }
          }
          if(strcmp(regname, ctl_reg_menomic) == 0) {
#ifdef VERIFY_SIMICS
            if (m_id == 0) { // only warn once
              DEBUG_OUT("pstate_t: warning: control register #%d == \"%s\" is not modelled.\n", i, regname );
            }
            SIM_HALT;
          } else {
            // if (m_id == 0) { // only warn once
            //   DEBUG_OUT("pstate_t: warning: control register #%d == \"%s\" has simics name \"%s\".\n", i, ctl_reg_menomic, regname );
            // }
          }
#endif
        } else {
          // else filter out non-existant registers
          SIMICS_clear_exception();
        }
      }
    }
    
#ifdef VERIFY_SIMICS
    // validate the simics has the same number of register windows as us
    attr_value_t attr = SIMICS_get_attribute(m_cpu, "num-windows");
    if ( attr.kind != Sim_Val_Integer || attr.u.integer != NWINDOWS ) {
      ERROR_OUT("error: pstate_t: number of register windows differ!");
      if (attr.kind == Sim_Val_Integer)
        ERROR_OUT("Simics num-windows == %lld. N-WINDOWS %d\n",
                  attr.u.integer, NWINDOWS );
      SYSTEM_EXIT;
    }
    hfa_checkerr("pstate: get attribute: num-windows");
  } else { // not in simics
#endif
    for (int32 i = 0; i < max_control_reg; i++) {
      m_valid_control_reg[i] = true;
    }
  }

  // allocate / initialize the ASI mapping
  m_asi_is_cacheable = (char *) malloc( sizeof(char) * MAX_NUM_ASI );
  for ( uint16 i = 0; i < MAX_NUM_ASI; i++ ) {
    m_asi_is_cacheable[i] = 0;
  }
  // indicate that certain ASIs _are_ cacheable
  m_asi_is_cacheable[ASI_PRIMARY] = true;
  m_asi_is_cacheable[ASI_NUCLEUS] = true;
  m_asi_is_cacheable[ASI_SECONDARY] = true;
  m_asi_is_cacheable[ASI_AS_IF_USER_PRIMARY] = true;
  m_asi_is_cacheable[ASI_AS_IF_USER_SECONDARY] = true;
  m_asi_is_cacheable[ASI_NUCLEUS_QUAD_LDD] = true;
  m_asi_is_cacheable[ASI_NUCLEUS_QUAD_LDD_L] = true;
  m_asi_is_cacheable[ASI_PHYS_USE_EC] = true;
  m_asi_is_cacheable[ASI_BLK_AIUP] = true;
  m_asi_is_cacheable[ASI_BLK_AIUS] = true;
  m_asi_is_cacheable[ASI_BLK_AIUPL] = true;
  m_asi_is_cacheable[ASI_BLK_AIUSL] = true;
  m_asi_is_cacheable[ASI_BLK_P] = true;
  m_asi_is_cacheable[ASI_BLK_S] = true;
  m_asi_is_cacheable[ASI_BLK_COMMIT_P] = true;
  m_asi_is_cacheable[ASI_BLK_COMMIT_S] = true;

  m_asi_is_cacheable[ASI_FL8_P] = true;
  m_asi_is_cacheable[ASI_FL8_S] = true;
  m_asi_is_cacheable[ASI_FL16_P] = true;
  m_asi_is_cacheable[ASI_FL16_S] = true;
  m_asi_is_cacheable[ASI_FL8_PL] = true;
  m_asi_is_cacheable[ASI_FL8_SL] = true;
  m_asi_is_cacheable[ASI_FL16_PL] = true;
  m_asi_is_cacheable[ASI_FL16_SL] = true;

  m_chkpt_interval = 100000000 ;
}

//**************************************************************************
pstate_t::~pstate_t() {
  if (m_translation_cache != NULL)
    delete m_translation_cache;
  if (m_control_map)
    free( m_control_map );
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pstate_t::checkpointState( core_state_t *a_state )
{
	// copies the current processors state

	// hfa_checkerr("pstate updatestate: begin\n");
	// zero the state
	memset( (void*) a_state, 0, sizeof( core_state_t ) );

	//
	// copy the global  registers using the read_register api.
	//
	// There are 4 sets of globals: normal, alt, mmu, and interrupt
	for ( uint32 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset ++ ) {
		// registers 0...7 are global
		for ( uint32 i = 0; i < 8; i ++ ) {
			a_state->global_regs[8*gset + i] = m_sparc_intf->read_global_register(
					m_cpu, gset, i );
			if(MULTICORE_DIAGNOSIS_ONLY)
				m_multicore_diagnosis->core_state[m_id].global_regs[8*gset + i] = m_sparc_intf->read_global_register(
						m_cpu, gset, i );
		}
	}
	// hfa_checkerr("pstate: end global\n");

	//
	// copy the integer registers using the read_register api.
	//
	for ( int cwp = 0; cwp < NWINDOWS; cwp++ ) {
		// just update the IN and LOCAL registers for ALL windows
		for (uint32 reg = 31; reg >= 16; reg --) {
			a_state->int_regs[pstateWindowMap(cwp, reg)] =
				m_sparc_intf->read_window_register( m_cpu, cwp, reg );
			if(MULTICORE_DIAGNOSIS_ONLY)
				m_multicore_diagnosis->core_state[m_id].int_regs[pstateWindowMap(cwp, reg)] =
					m_sparc_intf->read_window_register( m_cpu, cwp, reg );
		}
	}
	hfa_checkerr("pstate: end iregister\n");

	//
	// copy the floating point registers using the register api
	//
	for ( uint32 reg = 0; reg < MAX_FLOAT_REGS; reg ++ ) {
		// simics registers are numbered 0 .. 62
		a_state->fp_regs[reg] = m_sparc_intf->read_fp_register_x( m_cpu, (reg*2) );
		if(MULTICORE_DIAGNOSIS_ONLY)
			m_multicore_diagnosis->core_state[m_id].fp_regs[reg] = m_sparc_intf->read_fp_register_x( m_cpu, (reg*2) );
#if 0
		// CM had a bug here earlier using the wrong register numbers...
		char str[200];
		sprintf(str, "fp register %d", reg);
		hfa_checkerr(str);
#endif
	}
	hfa_checkerr("all fp registers");

	//
	// copy the control registers using register api
	//
	for (uint32 i = 0; i < MAX_CTL_REGS; i++) {
		if (m_valid_control_reg[i]) {
			// only those valid ones (i.e. Those that Opal really modelled)
			a_state->ctl_regs[i] = SIMICS_read_register( m_cpu, i );
			if(MULTICORE_DIAGNOSIS_ONLY)
				m_multicore_diagnosis->core_state[m_id].ctl_regs[i] = SIMICS_read_register( m_cpu, i );
			//sprintf(str, "control update state: %d.", i );
			// hfa_checkerr(str);
		}
	}
}

void pstate_t::checkpointStateAfterLogging()
{
	if(MULTICORE_DIAGNOSIS_ONLY) {

		//
		// copy the global  registers using the read_register api.
		//
		// There are 4 sets of globals: normal, alt, mmu, and interrupt
		for ( uint32 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset ++ ) {
			// registers 0...7 are global
			for ( uint32 i = 0; i < 8; i ++ ) {
				m_multicore_diagnosis->core_state[m_id].global_regs[8*gset + i] = m_sparc_intf->read_global_register(
						m_cpu, gset, i );
			}
		}

		//
		// copy the integer registers using the read_register api.
		//
		for ( int cwp = 0; cwp < NWINDOWS; cwp++ ) {
			// just update the IN and LOCAL registers for ALL windows
			for (uint32 reg = 31; reg >= 16; reg --) {
				m_multicore_diagnosis->core_state[m_id].int_regs[pstateWindowMap(cwp, reg)] =
					m_sparc_intf->read_window_register( m_cpu, cwp, reg );
			}
		}

		//
		// copy the floating point registers using the register api
		//
		for ( uint32 reg = 0; reg < MAX_FLOAT_REGS; reg ++ ) {
			// simics registers are numbered 0 .. 62
			m_multicore_diagnosis->core_state[m_id].fp_regs[reg] = m_sparc_intf->read_fp_register_x( m_cpu, (reg*2) );
		}

		//
		// copy the control registers using register api
		//
		for (uint32 i = 0; i < MAX_CTL_REGS; i++) {
			if (m_valid_control_reg[i]) {
				// only those valid ones (i.e. Those that Opal really modelled)
				m_multicore_diagnosis->core_state[m_id].ctl_regs[i] = SIMICS_read_register( m_cpu, i );
				//sprintf(str, "control update state: %d.", i );
				// hfa_checkerr(str);
			}
		}
	}
}

void pstate_t::printArchState()
{
	// Prints the entire processor state
	DEBUG_OUT("Arch state start\n") ;
	for ( uint32 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset ++ ) {
		for ( uint32 i = 0; i < 8; i ++ ) {
			DEBUG_OUT("0x%x\n", m_sparc_intf->read_global_register(m_cpu, gset, i )) ;
		}
	}
	for ( int cwp = 0; cwp < NWINDOWS; cwp++ ) {
		for (uint32 reg = 31; reg >= 16; reg --) {
			DEBUG_OUT("0x%x\n",  m_sparc_intf->read_window_register( m_cpu, cwp, reg )) ;
		}
	}
	for ( uint32 reg = 0; reg < MAX_FLOAT_REGS; reg ++ ) {
		DEBUG_OUT("0x%x\n", m_sparc_intf->read_fp_register_x( m_cpu, (reg*2) )) ;
	}
	for (uint32 i = 0; i < MAX_CTL_REGS; i++) {
		if (m_valid_control_reg[i]) {
			DEBUG_OUT("0x%x\n", SIMICS_read_register( m_cpu, i )) ;
		}
	}
	DEBUG_OUT("Arch state start\n") ;
}

bool pstate_t::compareState( core_state_t * ps )
{
  bool  are_same = true;
  
		//
		// copy the global  registers using the read_register api.
		//
		// There are 4 sets of globals: normal, alt, mmu, and interrupt
		for ( uint32 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset ++ ) {
			// registers 0...7 are global
			for ( uint32 i = 0; i < 8; i ++ ) {
				if(ps->global_regs[8*gset + i] != m_sparc_intf->read_global_register(
						m_cpu, gset, i ))
					are_same = false;
			}
		}

		//
		// copy the integer registers using the read_register api.
		//
		for ( int cwp = 0; cwp < NWINDOWS; cwp++ ) {
			// just update the IN and LOCAL registers for ALL windows
			for (uint32 reg = 31; reg >= 16; reg --) {
				if(ps->int_regs[pstateWindowMap(cwp, reg)] !=
					m_sparc_intf->read_window_register( m_cpu, cwp, reg ))
					are_same = false;
			}
		}

		//
		// copy the floating point registers using the register api
		//
		for ( uint32 reg = 0; reg < MAX_FLOAT_REGS; reg ++ ) {
			// simics registers are numbered 0 .. 62
			if(ps->fp_regs[reg] != m_sparc_intf->read_fp_register_x( m_cpu, (reg*2) ))
				are_same = false;
		}

		//
		// copy the control registers using register api
		//
		for (uint32 i = 0; i < MAX_CTL_REGS; i++) {
			if (m_valid_control_reg[i]) {
				// only those valid ones (i.e. Those that Opal really modelled)
				if(ps->ctl_regs[i] = SIMICS_read_register( m_cpu, i ))
					are_same = false;
				//sprintf(str, "control update state: %d.", i );
				// hfa_checkerr(str);
			}
		}



  return (are_same);
}



//**************************************************************************
void pstate_t::recoverState( core_state_t *a_state )
{
  // recovers the current processors state

  //
  // copy the global registers using the read_register api.
  //
  // There are 4 sets of globals: normal, alt, mmu, and interrupt
  //DEBUG_OUT("globals\n");
  for ( uint32 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset ++ ) {
    // registers 0...7 are global
    for ( uint32 i = 0; i < 8; i ++ ) {
      m_sparc_intf->write_global_register( m_cpu, gset, i,
        a_state->global_regs[8*gset + i] );
	//DEBUG_OUT("%d:%llx\n", 8*gset+i, a_state->global_regs[8*gset+i]);
    }
  }
  hfa_checkerr("pstate_t: recoverState: end global\n");

  //
  // copy the integer registers using the read_register api.
  //
  //DEBUG_OUT("integer registers\n");
  for ( int cwp = 0; cwp < NWINDOWS; cwp++ ) {
    // just update the IN and LOCAL registers for ALL windows
    for (uint32 reg = 31; reg >= 16; reg --) {
      m_sparc_intf->write_window_register( m_cpu, cwp, reg,
        a_state->int_regs[pstateWindowMap(cwp, reg)] );
	//DEBUG_OUT("%d:%llx\n", pstateWindowMap(cwp,reg), a_state->int_regs[pstateWindowMap(cwp,reg)]);
    }
  }
  hfa_checkerr("pstate_t: recoverState: end iregister\n");

  //
  // copy the floating point registers using the register api
  //
  for ( uint32 reg = 0; reg < MAX_FLOAT_REGS; reg ++ ) {
    // simics registers are numbered 0 .. 62
    m_sparc_intf->write_fp_register_x( m_cpu, (reg*2), 
                                       a_state->fp_regs[reg] );
  }
  hfa_checkerr("pstate_t: recoverState: end fp registers");

  //
  // copy the control registers using register api
  //
  //DEBUG_OUT("control registers\n");
  for (uint32 i = 0; i < MAX_CTL_REGS; i++) {
    if (m_valid_control_reg[i]) {
		//ERROR_OUT( "warning: write_register " ) ;
      SIMICS_write_register( m_cpu, i, a_state->ctl_regs[i] );
	//DEBUG_OUT("%d:%llx\n", i, a_state->ctl_regs[i]);
      //sprintf(str, "write checkpoint control state: %d.", i );
      // hfa_checkerr(str);
    }
  }

  hfa_checkerr("pstate_t: recoverState: all state updated");
}

//**************************************************************************
void pstate_t::checkpointState( void )
{
  curr_chkpt = (curr_chkpt+1)%NUM_CHKPTS ;
  // DEBUG_OUT("Opal taking chkpt - #%d\n", curr_chkpt) ;
  checkpointState( &m_state[curr_chkpt] );
}

//**************************************************************************
void pstate_t::recoverState( void )
{
  recoverState( &m_state[curr_chkpt] );
}

ireg_t pstate_t::recoverReg( reg_id_t&rid )
{
	if (rid.getRtype()==RID_INT) {
		return m_state[curr_chkpt].int_regs[pstateWindowMap(rid.getVanillaState(), rid.getVanilla())];

	} else 	if (rid.getRtype()==RID_INT_GLOBAL) {
		return m_state[curr_chkpt].global_regs[8*rid.getVanillaState() + rid.getVanilla()];

	} else 	if (rid.getRtype()==RID_DOUBLE) {
		return (ireg_t) m_state[curr_chkpt].fp_regs[rid.getVanillaState()];
	}
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/// read an integer register relative to the global set
//**************************************************************************
ireg_t  pstate_t::intIntGlobal( unsigned int reg, int global_set )
{
  ireg_t value;

  if (reg > 8) {
    ERROR_OUT("pstate_t: error: invalid int global register %d\n", reg);
    return (0);
  }

  if ( reg == 0 ) {
    // global register zero always reads as zero
    value = 0;
  } else {
    //int real_reg = pstateGlobalMap( global_set, reg );
    value = m_state[curr_chkpt].global_regs[ global_set*8 + reg ];
  }
  return (value);
}

/// read an integer register relative to a cwp
//**************************************************************************
ireg_t  pstate_t::intIntWp( unsigned int reg, int cwp )
{
  ireg_t value;

  if (reg < 8 || reg > 32) {
    ERROR_OUT("pstate_t: error: invalid int windowed register %d\n", reg);
    return (0);
  }
  
  /* offset into the register window */
  int real_reg = pstateWindowMap( cwp, reg );

  //DEBUG_OUT("reg %d (cwp 0x%0llx) ==  %d\n", reg, cwp, real_reg);
  value = m_state[curr_chkpt].int_regs[real_reg];
  return (value);
}

//**************************************************************************
freg_t pstate_t::intDouble( unsigned int reg )
{
  if (reg > (MAX_FLOAT_REGS)) {
    ERROR_OUT("pstate_t: error: invalid floating point register %d\n", reg);
    return 0;
  }
  return (m_state[curr_chkpt].fp_regs[reg]);
}

//**************************************************************************
ireg_t pstate_t::intControl( unsigned int reg )
{
  if (reg > MAX_CTL_REGS) {
    ERROR_OUT("pstate_t: intcontrol: invalid ctl register %d\n", reg);
    return (0);
  }

  if (!m_valid_control_reg[reg]) {
    ERROR_OUT("pstate_t: intControl: invalid control register: %d\n",
              reg);
    return (0);
  }
  ireg_t local_res = m_state[curr_chkpt].ctl_regs[reg];

  return (local_res);
}

//**************************************************************************
bool pstate_t::readMemory( ireg_t log_addr, int size, bool sign_extend,
                           ireg_t *result_reg )
{
  // SIMICS_logical_to_physical assumes word read ...
#if 0
  if (size > 4)
    ERROR_OUT("warning: read memory only works for size <= 4\n");
#endif
  pa_t phys_pc = SIMICS_logical_to_physical( m_cpu, Sim_DI_Data, log_addr );

  // outside of physical memory -- this is an error code
  if (phys_pc == (pa_t) -1) {
    *result_reg = 0x0;
    return (false);
  }

  // strip off less than word read bits && tack on the end
  int  lower_bits = log_addr & 0x3;
  phys_pc        += lower_bits;
  ireg_t reg      = SIMICS_read_phys_memory( m_cpu, phys_pc, size );
  /*
  ireg_t reg_word  = SIMICS_read_phys_memory( phys_pc, 4 );
  DEBUG_OUT( "reading memory v:0x%0llx p:0x%0llx -> 0x%0llx 0x%0llx %d\n",
          log_addr, phys_pc, reg_word, reg, size );
  */

  if (sign_extend) {
    //DEBUG_OUT(" extending %d -- %d\n", size, (size * 8) - 1);
    reg = sign_ext64( reg, ((size * 8) - 1) );
  }
  
  *result_reg = reg;
  return (true);
}

//**************************************************************************
bool pstate_t::readPhysicalMemory( pa_t phys_addr, int size,
                                   ireg_t *result_reg )
{
  // use phys_mem0.map to determine which physical addresses are in I/O space
  if ( memory_inst_t::isIOAccess( phys_addr ) ) {
    return (false);
  }

  if ( (phys_addr & (size - 1)) != 0 ) {
    ERROR_OUT("error: readPhysicalMemory(): memory access not aligned: 0x%0llx size=%d\n", phys_addr, size );
    return (false);
  }

  // if size is greater than 8, split into chunks of 8
  while ( size > 0 ) {
    ireg_t reg = SIMICS_read_phys_memory( m_cpu, phys_addr, MIN(size, 8) );
    int isexcept = SIMICS_get_pending_exception();
    if ( isexcept != 0 ) {
      SIMICS_clear_exception();

      // CM: There never should be exceptions on read physical memory.
      //     ANY that occur are bad signs & should be investigated.
//       DEBUG_OUT( "warning: pstate_t::readPhysicalMemory: Exception: %s (address: 0x%0llx size: %d) %d\n",
//                  SIMICS_last_error(), phys_addr, size, isexcept );
      return (false);
    }
    *result_reg++ = reg;
    size -= 8;
    phys_addr += 8;

  }

  return (true);
}

//**************************************************************************
bool  pstate_t::writeMemory( ireg_t phys_addr, int size, ireg_t *value )
{
  // use phys_mem0.map to determine which physical addresses are in I/O space
  if ( memory_inst_t::isIOAccess( phys_addr ) ) {
    return (false);
  }

  if ( (phys_addr & (size - 1)) != 0 ) {
    ERROR_OUT("error: writePhysicalMemory():" ) ;
	ERROR_OUT("memory access not aligned: 0x%0llx size=%d\n", phys_addr, size );
    return (false);
  }
  
  // if size is greater than 8, split into chunks of 8
  while ( size > 0 ) {
//      ERROR_OUT( "warning: write_phys_memory " ) ;
    SIM_write_phys_memory( m_cpu, phys_addr, value[0], MIN(size, 8) );
    int isexcept = SIMICS_get_pending_exception();
    if ( isexcept != 0 ) {
      SIMICS_clear_exception();

      // CM: There never should be exceptions on read physical memory.
      //     ANY that occur are bad signs & should be investigated.
      DEBUG_OUT( "warning: pstate_t::writePhysicalMemory: " ) ;
	  DEBUG_OUT( "Exception: %s (address: 0x%0llx size: %d) %d\n",
                 SIMICS_last_error(), phys_addr, size, isexcept );
      return (false);
    }
	value++ ;
    size -= 8;
    phys_addr += 8;

  }
  return (true);
}

//**************************************************************************
bool pstate_t::translateInstruction( ireg_t log_addr, int size,
                                     pa_t &phys_addr )
{
  // SIMICS_logical_to_physical assumes word read ...
  phys_addr = SIMICS_logical_to_physical( m_cpu, Sim_DI_Instruction, log_addr );
  /// physical pc == -1 indicates an error
  if (phys_addr == (pa_t) -1) {
    return (false);
  }

  // strip off less than word read bits && tack on the end
  int  lower_bits = log_addr & 0x3;
  phys_addr      += lower_bits;
  return (true);
}

//**************************************************************************
bool pstate_t::translate( ireg_t log_addr, int size, pa_t &phys_addr )
{
  // SIMICS_logical_to_physical assumes word read ...
  phys_addr = SIMICS_logical_to_physical( m_cpu, Sim_DI_Data, log_addr );
  /// physical pc == -1 indicates an error
  if (phys_addr == (pa_t) -1) {
    return (false);
  }

  // strip off less than word read bits && tack on the end
  int  lower_bits = log_addr & 0x3;
  phys_addr      += lower_bits;
  return (true);
}

//**************************************************************************
la_t pstate_t::dereference( la_t ptr, uint32 size )
{
  pa_t  physical = (pa_t) -1;
  bool  success = true;

  physical = SIMICS_logical_to_physical( m_cpu, Sim_DI_Instruction, ptr );
  success = (physical != (pa_t) -1);
  if (!success) {
    // clear translation exception
    SIMICS_clear_exception();
    physical = SIMICS_logical_to_physical( m_cpu, Sim_DI_Data, ptr );
    success = (physical != (pa_t) -1);
  }
  if (!success) {
    // clear translation exception
    SIMICS_clear_exception();
    if ( m_translation_cache->find( ptr ) == m_translation_cache->end() ) {
      // DEBUG_OUT("dereference: no translation for: 0x%0llx\n", ptr );
      return (la_t) -1;
    } else {
      physical = (*m_translation_cache)[ptr];
    }
  }

  la_t  value = SIMICS_read_phys_memory( m_cpu, physical, size );
  if (m_translation_cache->find( ptr ) == m_translation_cache->end()) {
    (*m_translation_cache)[ptr] = physical;
  }
  return value;
}

/*------------------------------------------------------------------------*/
/* Package Interfaces                                                     */
/*------------------------------------------------------------------------*/

//***************************************************************************
int pstate_t::getContext( void )
{
  attr_value_t   attr_reg = SIMICS_get_attribute( m_conf_mmu, "ctxt_primary" );
  ireg_t         pctx_reg = 0;
  if (attr_reg.kind == Sim_Val_Integer) {
    pctx_reg = attr_reg.u.integer;
  } else {
    ERROR_OUT("error: unable to read attribute on sfmmu: ctxt_primary\n");
  }

  int      context  = (int) pctx_reg;
  return (context);
}

//**************************************************************************
void pstate_t::askSimics( uint32 instr, pa_t paddr, la_t addr )
{
  if (instr != 0) {
    // The proposed methodology would be to write a physical address, then disassemble it there
    DEBUG_OUT("askSimics: current not authorized to disassemble instructions\n");
  }
  if (paddr != 0) {
    tuple_int_string_t *result = SIMICS_disassemble( m_cpu, paddr, /* phy */ 0 );
    if ( result == NULL ) {
      DEBUG_OUT("   pa: 0x%0llx NULL\n", paddr );
    } else {
      DEBUG_OUT("   pa: 0x%0llx %s\n", paddr, result->string );
    }
  }
  if (addr != 0) {
    tuple_int_string_t *result = SIMICS_disassemble( m_cpu, addr, /* log */ 1 );
    if (result == NULL) {
      DEBUG_OUT("   la: 0x%0llx NULL\n", addr );
    } else {
      DEBUG_OUT("   la: 0x%0llx %s\n", addr, result->string );
    }
  }
  // may (likely) cause a translation exception while trying to do this.
  SIMICS_clear_exception();
}

/** See the related documentation in the '.h' file before you get any idea
 *  about calling this function -- you shouldn't have to !
 */
//**************************************************************************
core_state_t  *pstate_t::getProcessorState( void ) {
  return ( &m_state[curr_chkpt] );
}

//***************************************************************************
void pstate_t::copyProcessorState( pstate_t *correct_state )
{
  core_state_t *ps = correct_state->getProcessorState();
  memcpy( &m_state[curr_chkpt], ps, sizeof(core_state_t) );
}

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

//***************************************************************************
void pstate_breakpoint_handler( conf_object_t *cpu, void *parameter )
{
  // int id = ((pstate_t *) parameter)->getID();
  // DEBUG_OUT( "bp: %d\n", id );
  SIMICS_break_simulation( NULL );
}

// new style window mapping: always return zero oriented register
//***************************************************************************
int pstate_t::pstateWindowMap( int cwp, unsigned int reg )
{
  int real_reg;
  if (cwp == 7 && reg < 16) {
    real_reg = REGISTER_BASE + reg;
  } else {
    real_reg = REGISTER_BASE - ((cwp + 1) * 16) + reg;
  }
  return (real_reg);
} 

// copies the name of the mmu for processor number "id" in to mmuname.
//***************************************************************************
void pstate_t::getMMUName( int id, char *mmuname, int len )
{
  conf_object_t *cpu = SIMICS_proc_no_2_ptr( id );

  // use the current cpu to get the attribute 
  attr_value_t mmu_obj = SIMICS_get_attribute( cpu, "mmu" );
  if (mmu_obj.kind != Sim_Val_Object) {
    ERROR_OUT("pstate_t: getMMUName: unable to get mmu name. not an object\n");
    SIM_HALT;
  }
  attr_value_t mmu_name = SIMICS_get_attribute( mmu_obj.u.object, "name" );
  if (mmu_name.kind != Sim_Val_String) {
    ERROR_OUT("pstate_t: getMMUName: unable to get mmu name. not a string\n");
    ERROR_OUT("pstate_t: getMMUName: kind = %d\n", mmu_name.kind );
    SIM_HALT;
  }
  strncpy( mmuname, mmu_name.u.string, len );
}

/*------------------------------------------------------------------------*/
/* Static Member Variables                                                */
/*------------------------------------------------------------------------*/

//**************************************************************************
const char *pstate_t::control_reg_menomic[CONTROL_NUM_CONTROL_TYPES] = {
  /* CONTROL_PC */               "pc",
  /* CONTROL_NPC */              "npc",
  /* CONTROL_Y */                "y",
  /* CONTROL_CCR */              "ccr",
  /* CONTROL_FPRS */             "fprs",
  /* CONTROL_FSR */              "fsr",
  /* CONTROL_ASI */              "asi",
  /* CONTROL_TICK */             "tick",
  /* CONTROL_GSR */              "gsr",
  /* CONTROL_TICK_CMPR */        "tick_cmpr",
  /* CONTROL_DISPATCH_CONTROL */ "dcr",
  /* CONTROL_PSTATE */           "pstate",
  /* CONTROL_TL */               "tl",
  /* CONTROL_PIL */              "pil",
  /* CONTROL_TPC1 */             "tpc1",
  /* CONTROL_TPC2 */             "tpc2",
  /* CONTROL_TPC3 */             "tpc3",
  /* CONTROL_TPC4 */             "tpc4",
  /* CONTROL_TPC5 */             "tpc5",
  /* CONTROL_TNPC1 */            "tnpc1",
  /* CONTROL_TNPC2 */            "tnpc2",
  /* CONTROL_TNPC3 */            "tnpc3",
  /* CONTROL_TNPC4 */            "tnpc4",
  /* CONTROL_TNPC5 */            "tnpc5",
  /* CONTROL_TSTATE1 */          "tstate1",
  /* CONTROL_TSTATE2 */          "tstate2",
  /* CONTROL_TSTATE3 */          "tstate3",
  /* CONTROL_TSTATE4 */          "tstate4",
  /* CONTROL_TSTATE5 */          "tstate5",
  /* CONTROL_TT1 */              "tt1",
  /* CONTROL_TT2 */              "tt2",
  /* CONTROL_TT3 */              "tt3",
  /* CONTROL_TT4 */              "tt4",
  /* CONTROL_TT5 */              "tt5",
  /* CONTROL_TBA */              "tba",
  /* CONTROL_VER */              "ver",
  /* CONTROL_CWP */              "cwp",
  /* CONTROL_CANSAVE */          "cansave",
  /* CONTROL_CANRESTORE */       "canrestore",
  /* CONTROL_OTHERWIN */         "otherwin",
  /* CONTROL_WSTATE */           "wstate",
  /* CONTROL_CLEANWIN */         "cleanwin",
  /* CONTROL_NUM_CONTROL_PHYS*/  "physcnt",
  /* CONTROL_UNIMPL */           "unimpl"
};

//**************************************************************************
const char *pstate_t::branch_type_menomic[BRANCH_NUM_BRANCH_TYPES] = {
  "BRANCH_NONE",               // not a branch
  "BRANCH_UNCOND",             // unconditional branch
  "BRANCH_COND",               // conditional branch
  "BRANCH_PCOND",              // predicated conditional branch
  "BRANCH_CALL",               // call
  "BRANCH_RETURN",             // return from call (jmp addr, %g0)
  "BRANCH_INDIRECT",           // indirect call    (jmpl)
  "BRANCH_CWP",                // current window pointer update
  "BRANCH_TRAP_RETURN",        // return from trap
  "BRANCH_TRAP",               // trap ? indirect jump ??? incorrect?
  "BRANCH_PRIV"                // explicit privilege  level change
};

//**************************************************************************
const char *pstate_t::branch_predictor_type_menomic[BRANCHPRED_NUM_BRANCH_TYPES] = {
  "BRANCHPRED_GSHARE",         // gshare predictor
  "BRANCHPRED_AGREE",          // agree predictor
  "BRANCHPRED_YAGS",           // YAGS predictor
  "BRANCHPRED_IGSHARE",        // infinite (ideal) gshare predictor
  "BRANCHPRED_MLPRED",         // multi-level predictor
  "BRANCHPRED_EXTREME"         // not implemented
};

//**************************************************************************
const char *pstate_t::trap_num_menomic( trap_type_t traptype ) {

  switch (traptype) {

  case (0x00):
    return ("NoTrap");
  case (0x01):
    return ("Power_On_Reset");
  case (0x02):
    return ("Watchdog_Reset");
  case (0x03):
    return ("Externally_Initiated_Reset");
  case (0x04):
    return ("Software_Initiated_Reset");
  case (0x05):
    return ("Red_State_Exception");
  case (0x08):
    return ("Instruction_Access_Exception");
  case (0x09):
    return ("Instruction_Access_Mmu_Miss");
  case (0x0a):
    return ("Instruction_Access_Error");
  case (0x10):
    return ("Illegal_Instruction");
  case (0x11):
    return ("Privileged_Opcode");
  case (0x12):
    return ("Unimplemented_Ldd");
  case (0x13):
    return ("Unimplemented_Std");
  case (0x20):
    return ("Fp_Disabled");
  case (0x21):
    return ("Fp_Exception_Ieee_754");
  case (0x22):
    return ("Fp_Exception_Other");
  case (0x23):
    return ("Tag_Overflow");
  case (0x24):
    return ("Clean_Window");
  case (0x28):
    return ("Division_By_Zero");
  case (0x29):
    return ("Internal_Processor_Error");
  case (0x30):
    return ("Data_Access_Exception");
  case (0x31):
    return ("Data_Access_Mmu_Miss");
  case (0x32):
    return ("Data_Access_Error");
  case (0x33):
    return ("Data_Access_Protection");
  case (0x34):
    return ("Mem_Address_Not_Aligned");
  case (0x35):
    return ("Lddf_Mem_Address_Not_Aligned");
  case (0x36):
    return ("Stdf_Mem_Address_Not_Aligned");
  case (0x37):
    return ("Privileged_Action");
  case (0x38):
    return ("Ldqf_Mem_Address_Not_Aligned");
  case (0x39):
    return ("Stqf_Mem_Address_Not_Aligned");
  case (0x40):
    return ("Async_Data_Error");
  case (0x41):
    return ("Interrupt_Level_1");
  case (0x42):
    return ("Interrupt_Level_2");
  case (0x43):
    return ("Interrupt_Level_3");
  case (0x44):
    return ("Interrupt_Level_4");
  case (0x45):
    return ("Interrupt_Level_5");
  case (0x46):
    return ("Interrupt_Level_6");
  case (0x47):
    return ("Interrupt_Level_7");
  case (0x48):
    return ("Interrupt_Level_8");
  case (0x49):
    return ("Interrupt_Level_9");
  case (0x4a):
    return ("Interrupt_Level_10");
  case (0x4b):
    return ("Interrupt_Level_11");
  case (0x4c):
    return ("Interrupt_Level_12");
  case (0x4d):
    return ("Interrupt_Level_13");
  case (0x4e):
    return ("Interrupt_Level_14");
  case (0x4f):
    return ("Interrupt_Level_15");
  case (0x60):
    return ("Interrupt_Vector");
  case (0x61):
    return ("PA_Watchpoint");
  case (0x62):
    return ("VA_Watchpoint");
  case (0x63):
    return ("Corrected_ECC_Error");
  case (0x64):
    return ("Fast_Instruction_Access_MMU_Miss");
  case (0x68):
    return ("Fast_Data_Access_MMU_Miss");
  case (0x6c):
    return ("Fast_Data_Access_Protection");
  case (0x80):
    return ("Spill_0_Normal");
  case (0x84):
    return ("Spill_1_Normal");
  case (0x88):
    return ("Spill_2_Normal");
  case (0x8c):
    return ("Spill_3_Normal");
  case (0x90):
    return ("Spill_4_Normal");
  case (0x94):
    return ("Spill_5_Normal");
  case (0x98):
    return ("Spill_6_Normal");
  case (0x9c):
    return ("Spill_7_Normal");
  case (0xa0):
    return ("Spill_0_Other");
  case (0xa4):
    return ("Spill_1_Other");
  case (0xa8):
    return ("Spill_2_Other");
  case (0xac):
    return ("Spill_3_Other");
  case (0xb0):
    return ("Spill_4_Other");
  case (0xb4):
    return ("Spill_5_Other");
  case (0xb8):
    return ("Spill_6_Other");
  case (0xbc):
    return ("Spill_7_Other");
  case (0xc0):
    return ("Fill_0_Normal");
  case (0xc4):
    return ("Fill_1_Normal");
  case (0xc8):
    return ("Fill_2_Normal");
  case (0xcc):
    return ("Fill_3_Normal");
  case (0xd0):
    return ("Fill_4_Normal");
  case (0xd4):
    return ("Fill_5_Normal");
  case (0xd8):
    return ("Fill_6_Normal");
  case (0xdc):
    return ("Fill_7_Normal");
  case (0xe0):
    return ("Fill_0_Other");
  case (0xe4):
    return ("Fill_1_Other");
  case (0xe8):
    return ("Fill_2_Other");
  case (0xec):
    return ("Fill_3_Other");
  case (0xf0):
    return ("Fill_4_Other");
  case (0xf4):
    return ("Fill_5_Other");
  case (0xf8):
    return ("Fill_6_Other");
  case (0xfc):
    return ("Fill_7_Other");
  default:
    ;
  }
  return ("Unknown_Trap");
}

//**************************************************************************
const char *pstate_t::async_exception_menomic( exception_t except_type ) {

  switch (except_type) {
  case EXCEPT_NONE:
    return("EXCEPT_NONE");
    break;
  case EXCEPT_MISPREDICT:
    return("EXCEPT_MISPREDICT");
    break;
  case EXCEPT_VALUE_MISPREDICT: 
    return("EXCEPT_VALUE_MISPREDICT");
    break;
  case EXCEPT_MEMORY_REPLAY_TRAP: 
    return("EXCEPT_MEMORY_REPLAY_TRAP");
    break;
  case EXCEPT_NUM_EXCEPT_TYPES:
    return("EXCEPT_NUM_EXCEPT_TYPES");
    break;
  }
  return ("EXCEPT_UNKNOWN");
}

//**************************************************************************
const char *pstate_t::fu_type_menomic( fu_type_t fu_type ) {
  switch (fu_type) {
  case FU_NONE:
    return("FU_NONE");
    break;
  case FU_INTALU:
    return("FU_INTALU");
    break;
  case FU_INTMULT:
    return("FU_INTMULT");
    break;
  case FU_INTDIV:
    return("FU_INTDIV");
    break;
  case FU_BRANCH:
    return("FU_BRANCH");
    break;
  case FU_FLOATADD:
    return("FU_FLOATADD");
    break;
  case FU_FLOATCMP:
    return("FU_FLOATCMP");
    break;
  case FU_FLOATCVT:
    return("FU_FLOATCVT");
    break;
  case FU_FLOATMULT:
    return("FU_FLOATMULT");
    break;
  case FU_FLOATDIV:
    return("FU_FLOATDIV");
    break;
  case FU_FLOATSQRT:
    return("FU_FLOATSQRT");
    break;
  case FU_RDPORT:
    return("FU_RDPORT");
    break;
  case FU_WRPORT:
    return("FU_WRPORT");
    break;
  default:
    return ("UNKNOWN FU_TYPE");
  }
  return ("UNKNOWN FU_TYPE");
}

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

//***************************************************************************
void  pstate_t::simcontinue( uint32 numsteps )
{
  m_advance_simics_exception = false;
  sim_exception_t except_code;
  SIMICS_enable_processor( m_cpu );


  //DEBUG_OUT("m_id is this %d name %s\n",m_id,m_cpu->name);
  //assert(m_id==0);

  // new simics 1.2.5
  SIMICS_time_post_cycle( m_cpu, 1, Sim_Sync_Processor,
                       pstate_breakpoint_handler, (void *) this );
  SIMICS_continue( 0 );
  //DEBUG_OUT( "pstate: stepped processor %d\n", m_id );

  // ignore the message: time breakpoint
  except_code = SIMICS_clear_exception();
  // SimExc_General is raised when a run-time breakpoint is reached
  if (except_code != SimExc_No_Exception &&
      except_code != SimExc_General) {
    if ( (!strncmp(SIMICS_last_error(), "Received control-c", 17)) ||
         (!strncmp(SIMICS_last_error(), "control C", 8)) ) 
    {
      DEBUG_OUT("control-C: ending simulation...\n");
      system_t::inst->breakSimulation();
    }
    DEBUG_OUT( "Exception error message: %s\n", SIMICS_last_error() );
    DEBUG_OUT( "pstate_t::simcontinue: caught exception (#%d).\n", (int) except_code );
    // LXL Simics likely got red state
#ifdef LXL_SNET_RECOVERY
	system_t::inst->detectedSymp() ;
#else // LXL_SNET_RECOVERY
    system_t::inst->breakSimulation();
#endif // LXL_SNET_RECOVERY
  }
  SIMICS_disable_processor( m_cpu );
}


bool pstate_t::getAdvanceSimicsException()
{
	return m_advance_simics_exception;
}

void pstate_t::disableInterrupts(){
  int rn_pstate = SIMICS_get_register_number(m_cpu, "pstate");
  uinteger_t pstate = SIMICS_read_register(m_cpu, rn_pstate);
  uinteger_t new_pstate = (pstate & (~0x2));
  SIMICS_write_register(m_cpu, rn_pstate, new_pstate);
  //pstate = SIMICS_read_register(m_cpu, rn_pstate);
  //assert((pstate & 0x2) == 0);
  int rn_pil = SIMICS_get_register_number(m_cpu, "pil");
  uinteger_t pil = SIMICS_read_register(m_cpu, rn_pil);
  SIMICS_write_register(m_cpu, rn_pil, 15);
  //pil = SIMICS_read_register(m_cpu, rn_pil);
  //assert(pil == 15);
}

// Function to get PIL
uinteger_t pstate_t::getPIL()
{
  int rn_pil = SIMICS_get_register_number(m_cpu, "pil");
  uinteger_t pil = SIMICS_read_register(m_cpu, rn_pil);
  return pil ;
}

bool pstate_t::isIE()
{
  int rn_pstate = SIMICS_get_register_number(m_cpu, "pstate");
  uinteger_t pstate = SIMICS_read_register(m_cpu, rn_pstate);
  uinteger_t ie = pstate & 0x2 ;
  return (ie==0x2) ;
}

/**
 * pseudo-static package interface:
 *   pstate_step_callback: callback function after a single step on the cpu.
 */
//**************************************************************************
void pstate_step_callback( conf_object_t *obj, void *mypstate )
{
  // currently unused
  //   pstate_t *ps = (pstate_t *) mypstate;
}

/** compares two processors states, returns true if the same. */
//**************************************************************************
bool ps_silent_compare( core_state_t * ps_1, core_state_t * ps_2 )
{
  bool  are_same = true;
  
  for ( uint32 i=0; i < MAX_GLOBAL_REGS; i++ )
    {
      if ( ps_1->global_regs[i] != ps_2->global_regs[i] ) {
        are_same = false;
      }
    }

  for ( uint32 i=0; i < MAX_INT_REGS; i++ )
    {
      if ( ps_1->int_regs[i] != ps_2->int_regs[i] ) {
        are_same = false;
      }
    }

  for ( uint32 i=0; i < MAX_FLOAT_REGS; i++ )
    {
      if ( ps_1->fp_regs[i] != ps_2->fp_regs[i] ) {
        are_same = false;
      }
    }

  // compare control registers
  for ( uint32 i=0; i < MAX_CTL_REGS; i++ )
    {
      if ( ps_1->ctl_regs[i] != ps_2->ctl_regs[i] ) {
        are_same = false;
      }
    }

  return (are_same);
}

/** compares two processors states, returns true if the same. */
//**************************************************************************
bool ps_compare( core_state_t * ps_1, core_state_t * ps_2 )
{
  bool  are_same = true;
  
  for ( uint32 i=0; i < MAX_GLOBAL_REGS; i++ )
    {
      if ( ps_1->global_regs[i] != ps_2->global_regs[i] ) {
        are_same = false;
 DEBUG_OUT( "greg %d: 0x%0llx 0x%0llx\n", i, 
                      ps_1->global_regs[i], ps_2->global_regs[i] );
      }
    }

  for ( uint32 i=0; i < MAX_INT_REGS; i++ )
    {
      if ( ps_1->int_regs[i] != ps_2->int_regs[i] ) {
        are_same = false;
 DEBUG_OUT( "ireg %d: 0x%0llx 0x%0llx\n", i, 
                      ps_1->int_regs[i], ps_2->int_regs[i] );
      }
    }

  for ( uint32 i=0; i < MAX_FLOAT_REGS; i++ )
    {
      if ( ps_1->fp_regs[i] != ps_2->fp_regs[i] ) {
        are_same = false;
 DEBUG_OUT( "fpreg %d: [0] 0x%0x 0x%0x [1] 0x%0x 0x%0x\n", i,
                ((uint32 *) &ps_1->fp_regs[i])[0],
                ((uint32 *) &ps_2->fp_regs[i])[0],
                ((uint32 *) &ps_1->fp_regs[i])[1],
                ((uint32 *) &ps_2->fp_regs[i])[1] );
      }
    }

  // compare control registers
  for ( uint32 i=0; i < MAX_CTL_REGS; i++ )
    {
      if ( ps_1->ctl_regs[i] != ps_2->ctl_regs[i] ) {
        are_same = false;
        DEBUG_OUT( "cntl %d: 0x%0llx 0x%0llx\n", i,
                   ps_1->ctl_regs[i], ps_2->ctl_regs[i] );
      }
    }

  return (are_same);
}

/** Prints out the entire core processor state structure. */
//**************************************************************************
void  ps_print( core_state_t * ps )
{
  for ( uint32 i=0; i < MAX_GLOBAL_REGS; i++ ) {
    DEBUG_OUT( "greg %d: 0x%016llx\n", i, 
               ps->global_regs[i]);
  }

  for ( uint32 i=0; i < MAX_INT_REGS; i++ ) {
    DEBUG_OUT( "ireg %d: 0x%016llx\n", i, 
               ps->int_regs[i]);
  }

  for ( uint32 i=0; i < MAX_FLOAT_REGS; i++ ) {
    DEBUG_OUT( "fpreg %d: %016llx\n", i,
               ps->fp_regs[i]);
  }

  for ( uint32 i=0; i < MAX_CTL_REGS; i++ ) {
    DEBUG_OUT( "ctl %d: %016llx\n", i,
               ps->ctl_regs[i]);
  }
}
