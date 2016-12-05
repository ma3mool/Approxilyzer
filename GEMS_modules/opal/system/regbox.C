
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
 * FileName:  regbox.C
 * Synopsis:  abstract interface to register map, register file
 *            to allow dynamic instructions more general decode, squash, 
 *            schedule logic.
 * Author:    cmauer
 * Version:   $Id: regbox.C 1.40 06/02/13 18:49:21-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "iwindow.h"
#include "regfile.h"
#include "regmap.h"
#include "pstate.h"         // need translation for control registers
#include "regbox.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// boolean indicating if the register box has been initialized yet
bool         reg_box_t::m_is_init = false;
/// mapping from [cwp][arch_reg] -> single register number
byte_t     **reg_box_t::m_window_map;
/// mapping from [gset][arch_reg] -> single register number
byte_t     **reg_box_t::m_global_map;
/// mapping from [priv][vanilla_reg] -> single register number
byte_t     **reg_box_t::m_control_map;
/// size of the control registers (in bits)
byte_t      *reg_box_t::m_control_size;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//***************************************************************************
reg_id_t::reg_id_t( void )
{
  // default constructor
  reset();
}

//***************************************************************************
reg_id_t::~reg_id_t( void )
{
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/


//***************************************************************************
const char *reg_id_t::rid_type_menomic( rid_type_t rtype )
{
  const char *rt = NULL;

  switch (rtype) {
  case RID_NONE:                // No register
    rt = "None";
    break;

  case RID_INT:                 // integer identifier (windowed)
    rt = "Int";
    break;

  case RID_INT_GLOBAL:          // integer identifier (global)
    rt = "Global";
    break;

  case RID_CC:                  // condition code identifier
    rt = "cc";
    break;

  case RID_CONTROL:             // control register
    rt = "control";
    break;

  case RID_SINGLE:              // floating point: single precision
    rt = "single";
    break;

  case RID_DOUBLE:              // floating point: double precision
    rt = "double";
    break;

  case RID_QUAD:                // floating point: quad   precision
    rt = "quad";
    break;

  case RID_CONTAINER:           // container class
    rt = "container";
    break;

  default:
    ERROR_OUT("error: reg_id_t: unidentified rid type: %d\n", rtype);
    SIM_HALT;
  }

  ASSERT( rt != NULL );
  return (rt);
}

//***************************************************************************
half_t  reg_id_t::getFlatRegister( void )
{
  int         flatreg;
  
  switch (m_rtype) {
  case RID_NONE:                // No register
    flatreg = 0;
    break;

  case RID_INT:                 // integer identifier (windowed)
    flatreg = reg_box_t::iregWindowMap( m_vanilla, m_vstate );
    break;

  case RID_INT_GLOBAL:          // integer identifier (global)
    flatreg = reg_box_t::iregGlobalMap( m_vanilla, m_vstate );
    break;

  case RID_CC:                  // condition code identifier
    flatreg = m_vanilla;
    break;

  case RID_CONTROL:             // control register
    flatreg = m_vanilla;
    break;

  case RID_SINGLE:              // floating point: single precision
    flatreg = m_vanilla;
    break;

  case RID_DOUBLE:              // floating point: double precision
    flatreg = m_vanilla;
    // and vanilla + 1
    break;

  case RID_QUAD:                // floating point: quad   precision
    flatreg = m_vanilla;
    // and vanilla + 1, + 2 and + 3
    break;

  case RID_CONTAINER:           // container class
    flatreg = m_vanilla;
    // CM FIX: rid container doesn't really have a single flat register
    // and logical + 1, + 2, + 3
    // and logical + 4, + 5, + 6, + 7
    break;

  default:
	// FIXME - THis is just a fix to make some sims run.
	// gzip has this problem even in the golden run!
	flatreg = m_vanilla ;
	ERROR_OUT("Invalid Reg type = %d\n", m_rtype) ;
    SIM_HALT;
  }
  return (flatreg);
}

/** print out this register mapping */
//***************************************************************************
void    reg_id_t::print( void )
{
  const char *rt      = rid_type_menomic( (rid_type_t) m_rtype );
  int         flatreg = getFlatRegister();
  if (m_rtype == RID_DOUBLE) {
    half_t reg1 = (m_physical >> 8) & 0xff;
    half_t reg2 =  m_physical & 0xff;
    DEBUG_OUT( "%.6s (%3.3d : %2.2d) = %3.3d   -->  %3.3d %3.3d\n",
               rt, m_vanilla, m_vstate, flatreg, reg1, reg2 );
  } else {
    DEBUG_OUT( "%.6s (%3.3d : %2.2d) = %3.3d   -->  %3.3d\n",
               rt, m_vanilla, m_vstate, flatreg, m_physical );
  }
}

/** print the disassembled form of this instruction */
//***************************************************************************
int reg_id_t::printDisassemble( char *str )
{
  char ch;
  int  count = 0;
  switch (m_rtype) {
  case RID_INT:                 // integer identifier (windowed)
    if (m_vanilla <= 7)
      SIM_HALT;
    else if (m_vanilla <= 15)
      ch = 'o';
    else if (m_vanilla <= 23)
      ch = 'l';
    else if (m_vanilla <= 31)
      ch = 'i';
    else
      SIM_HALT;
    count = sprintf( str, "%%%c%d", ch, m_vanilla % 8);
    break;

  case RID_INT_GLOBAL:          // integer identifier (global)
    count = sprintf( str, "%%g%d", m_vanilla );
    break;

  case RID_CC:                  // condition code identifier
    switch (m_vanilla) {
    case REG_CC_CCR:
      // if (m_ccshift == 0)
      count = sprintf( str, "%%i/xcc" );
      break;
    case REG_CC_FCC0:
    case REG_CC_FCC1:
    case REG_CC_FCC2:
    case REG_CC_FCC3:
      count = sprintf( str, "%%fcc%d", (m_vanilla - REG_CC_FCC0) );
      break;

    default:
      SIM_HALT;
    }
    break;

  case RID_CONTROL:             // control register
    count = sprintf( str, "%%%s", 
                     pstate_t::control_reg_menomic[m_vanilla] );
    break;

  case RID_SINGLE:              // floating point: single precision
  case RID_DOUBLE:              // floating point: double precision
  case RID_QUAD:                // floating point: quad   precision
    count = sprintf( str, "%%f%d", m_vanilla );
    break;

  case RID_CONTAINER:           // container class
    // you could expand the container class to 'decode' properly
    // however, it may make output less readable, currently, just print type
    count = sprintf( str, "%%spec* [%d]", m_vstate );
    break;

  default:
    SIM_HALT;
  }
  return count;
}

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
reg_box_t::reg_box_t( )
{
  // initialize abstract register types
  for (int i = 0; i < (int) RID_NUM_RID_TYPES; i++) {
    m_arf[i] = NULL;
  }
}

//**************************************************************************
reg_box_t::~reg_box_t( )
{
  // CM FIX: a small amount of memory is lost freeing this object.
}

//***************************************************************************
void reg_box_t::initializeMappings( void )
{
  uint32 reg;

  if (m_is_init == true) {
    return;
  }

  m_window_map = (byte_t **) malloc( sizeof(byte_t *) * NWINDOWS );
  for ( uint32 cwp = 0; cwp < NWINDOWS; cwp++ ) {
    // integer registers are 8 to 31
    m_window_map[cwp] = (byte_t *) malloc( sizeof(byte_t) * 32 );

    // initialize the last 24 registers
    for (reg = 31; reg >= 8; reg --) {
      m_window_map[cwp][reg] = initIregWindowMap( reg, cwp );
    }
    // initialize the first 8 entries
    for (reg = 0; reg < 8; reg++) {
      m_window_map[cwp][reg] = (byte_t) -1;
    }
  }

  m_global_map = (byte_t **) malloc( sizeof(byte_t *) *
                                     (REG_GLOBAL_INT + 1) );
  for (uint16 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset++) {
    m_global_map[gset] = (byte_t *) malloc( sizeof(byte_t) * 8 );
    for (reg = 0; reg < 8; reg ++) {
      m_global_map[gset][reg] = initIregGlobalMap( reg, gset );
    }
  }

  m_control_map = (byte_t **) malloc( sizeof(byte_t *) * 2 );
  for ( uint32 i = 0; i < 2; i++ ) {

    // integer registers are 0 to 7
    m_control_map[i] = (byte_t *) malloc( sizeof(byte_t) * CONTROL_NUM_CONTROL_TYPES);
    for ( reg = 0; reg < CONTROL_NUM_CONTROL_TYPES; reg++ ) {
      m_control_map[i][reg] = initStateregMap( reg, i );
    }
  }

  m_control_size = (byte_t *) malloc( sizeof(byte_t) * CONTROL_NUM_CONTROL_TYPES );
  for ( reg = 0; reg < CONTROL_NUM_CONTROL_TYPES; reg++ ) {
    m_control_size[reg] = initControlSizeMap( (control_reg_t) reg );
  }

  m_is_init = true;
}

//***************************************************************************
void    reg_box_t::addRegisterHandler( rid_type_t       rtype,
                                       abstract_rf_t   *arf )
{
  if ( m_arf[rtype] != NULL ) {
    ERROR_OUT( "regbox_t: addRegisterHandler: type %d already added!\n",
               (int) rtype );
    ASSERT( m_arf[rtype] == NULL );
  }
  m_arf[rtype] = arf;
}  

//**************************************************************************
bool      reg_box_t::registersAvailable( void )
{
  bool avail = true;

  avail = ( m_arf[RID_INT]->regsAvailable() &&
            m_arf[RID_SINGLE]->regsAvailable() &&
            m_arf[RID_CC]->regsAvailable() &&
            m_arf[RID_CONTROL]->regsAvailable() );

  return avail;
}

//**************************************************************************
void
reg_box_t::validateMapping( uint32 id, iwindow_t &iwin )
{
  reg_map_t  *int_map   = m_arf[RID_INT]->getDecodeMap();
  reg_map_t  *float_map = m_arf[RID_SINGLE]->getDecodeMap();
  reg_map_t  *cc_map    = m_arf[RID_CC]->getDecodeMap();
  
  int_map->buildFreelist();
  float_map->buildFreelist();
  cc_map->buildFreelist();
  
  int32  index = iwin.getLastRetired();

  // step past the sentinel "null" value in the window
  dynamic_inst_t *d = iwin.peekWindow( index );
  ASSERT(d == NULL);

  d = iwin.peekWindow( index );
  while (d != NULL) {
    for (int i = 0; i < SI_MAX_DEST; i ++) {
      ASSERT( d != NULL );
      reg_id_t &tofree       = d->getToFreeReg(i);
      half_t    physical_reg = tofree.getPhysical();
      if ((!tofree.isZero()) && 
          physical_reg != PSEQ_INVALID_REG) {
        tofree.getARF()->addFreelist( tofree );
      }
    }
    d = iwin.peekWindow( index );
  }
  
  int_map->leakCheck( id, "int map" );
  float_map->leakCheck( id, "fp map" );
  cc_map->leakCheck( id, "cc map" );
}

//#define PRINT_ARCH_MAPPING


/* mapping from windowed / global to flat register space for renaming
 *  | g0 | g1 | g2 | g3 | oli W0 | oli W1 | li W2 | ... | W7 l |
 *  g0 = normal globals
 *  g1 = alt globals
 *  g2 = interrupt globals
 *  g3 = mmu globals
 *  oli = output/local/input registers (8 registers each)
 *  note: window 7's registers wrap around to W0's
 */
//**************************************************************************
half_t 
reg_box_t::initIregWindowMap( byte_t arch_reg, uint16 cwp )
{
#if 1
  if (arch_reg < 8 || arch_reg > 31) {
    DEBUG_OUT("ireg map: invalid local: reg == %d\n", arch_reg);
  }
  if ( cwp > NWINDOWS - 1) {
    DEBUG_OUT("ireg map: invalid cwp: cwp == %d\n", cwp);
  }
#endif
  short flatReg = PSEQ_FLATREG_NWIN - cwp*16 - 32 + arch_reg;
  // wrap around 0th register window to maximum window
  if (flatReg < 0) {
    flatReg = PSEQ_FLATREG_NWIN + flatReg;
  }
#ifdef PRINT_ARCH_MAPPING
    DEBUG_OUT("cwp\t%d\tarch_reg\t%d\tflat\t%d\n",cwp, arch_reg,flatReg);
#endif
  return (flatReg);
}

//**************************************************************************
half_t 
reg_box_t::initIregGlobalMap( byte_t arch_reg, uint16 gset )
{
#if 1
  if ( gset > 3 ) {
    DEBUG_OUT("global map: invalid set: gset == %d\n", gset);
  }
  if ( arch_reg > 7 ) {
    DEBUG_OUT("global map: invalid global: reg == %d\n", arch_reg);
  }
#endif
  // writes to %g0 are always to this sentinel value
  if (arch_reg == 0) {
    return (PSEQ_FLATREG_NWIN);
  }
  // The flat register space extends up to FLATREG_NWIN, after that
  // its all the globals
#ifdef PRINT_ARCH_MAPPING
  short flatReg = PSEQ_FLATREG_NWIN + gset*8 + arch_reg;
  DEBUG_OUT("gset\t%d\tarch_reg\t%d\tflat\t%d\n",gset, arch_reg,flatReg);
#endif
  return (PSEQ_FLATREG_NWIN + gset*8 + arch_reg);  
}

//**************************************************************************
int16
reg_box_t::initStateregMap( byte_t reg_no, bool isPriv )
{
  control_reg_t  regid = CONTROL_ASI;

  if (isPriv) {

    switch (reg_no) {
    case 0:      // TPC
      regid = CONTROL_TPC1;
      break;
    case 1:      // TNPC
      regid = CONTROL_TNPC1;
      break;
    case 2:      // TSTATE
      regid = CONTROL_TSTATE1;
      break;
    case 3:      // TT
      regid = CONTROL_TT1;
      break;
    case 4:      // TICK
      // CM WRPR: decided to 'unimplement' the pesky TICK register
      // regid = CONTROL_TICK;
      regid = CONTROL_UNIMPL;
      break;
    case 5:      // TBA
      regid = CONTROL_TBA;
      break;
    case 6:      // PSTATE
      regid = CONTROL_PSTATE;
      break;
    case 7:      // TL
      regid = CONTROL_TL;
      break;
    case 8:      // PIL
      regid = CONTROL_PIL;
      break;
    case 9:      // CWP
      regid = CONTROL_CWP;
      break;
    case 10:     // CANSAVE
      regid = CONTROL_CANSAVE;
      break;
    case 11:     // CANRESTORE
      regid = CONTROL_CANRESTORE;
      break;
    case 12:     // CLEANWIN
      regid = CONTROL_CLEANWIN;
      break;
    case 13:     // OTHERWIN
      regid = CONTROL_OTHERWIN;
      break;
    case 14:     // WSTATE
      regid = CONTROL_WSTATE;
      break;
    default:
      // DEBUG_OUT("ERROR: prrd to register %d\n", reg_no);
      regid = CONTROL_UNIMPL;
    }

  } else {
    
    switch (reg_no) {
    case 0:      // Y
      regid = CONTROL_Y;
      break;
    case 2:      // CCR
      regid = CONTROL_CCR;
      break;
    case 3:      // ASI
      regid = CONTROL_ASI;
      break;
    case 4:      // TICK register
      // CM WRPR: decided to 'unimplement' the pesky TICK register
      // regid = CONTROL_TICK;
      regid = CONTROL_UNIMPL;
      break;
    case 5:      // PC
      regid = CONTROL_PC;
      break;
    case 6:      // FPRS
      // CM RD: fprs would need to be modified on all fp register writes--
      //        its rarely used in our integer workloads, and I don't want
      //        to rename it everywhere.
      // regid = CONTROL_FPRS;
      regid = CONTROL_UNIMPL;
      break;
    case 19:     // GSR
      regid = CONTROL_GSR;
      break;
#if 0
    case 20:     // SET_SOFTINT
      regid = CONTROL_SET_SOFTINT;
      break;
    case 21:     // CLEAR_SOFTINT
      regid = CONTROL_CLEAR_SOFTINT;
      break;
    case 22:     // SOFTINT_REG
      regid = CONTROL_SOFTINT;
      break;
#endif
    case 23:     // TICK_CMPR_REG
      regid = CONTROL_TICK_CMPR;
      break;
      
    default:
      // DEBUG_OUT("ERROR: rd to register %d\n", reg_no);
      regid = CONTROL_UNIMPL;
    }
  }
  return (regid);
}

//**************************************************************************
int16
reg_box_t::initControlSizeMap( control_reg_t reg )
{
  int16  controlsize = 64;

  switch (reg) {
  case CONTROL_Y:
    controlsize = 32;
    break;
  case CONTROL_CCR:
    controlsize = 8;
    break;
  case CONTROL_FPRS:
    controlsize = 3;
    break;
  case CONTROL_ASI:
    controlsize = 8;
    break;
  case CONTROL_GSR:
    controlsize = 64;
    break;
  case CONTROL_PSTATE:
    controlsize = 11;
    break;
  case CONTROL_TL:
    controlsize = 3;
    break;
  case CONTROL_PIL:
    controlsize = 4;
    break;
  case CONTROL_TSTATE1:
  case CONTROL_TSTATE2:
  case CONTROL_TSTATE3:
  case CONTROL_TSTATE4:
  case CONTROL_TSTATE5:
    controlsize = 40;
    break;
  case CONTROL_TT1:
  case CONTROL_TT2:
  case CONTROL_TT3:
  case CONTROL_TT4:
  case CONTROL_TT5:
    controlsize = 9;
    break;
  case CONTROL_VER:
    controlsize = 64;
    break;
  case CONTROL_CWP:
    controlsize = 5;
    break;
  case CONTROL_CANSAVE:
    controlsize = 5;
    break;
  case CONTROL_CANRESTORE:
    controlsize = 5;
    break;
  case CONTROL_OTHERWIN:
    controlsize = 5;
    break;
  case CONTROL_WSTATE:
    controlsize = 6;
    break;
  case CONTROL_CLEANWIN:
    controlsize = 5;
    break;
  default:
    controlsize = 64;
  }
  return controlsize;
}

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/


/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************

