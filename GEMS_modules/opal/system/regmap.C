
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
 * FileName:  regmap.C
 * Synopsis:  Logical register file
 * Author:    zilles
 * Version:   $Id: regmap.C 1.19 06/02/13 18:49:21-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "regmap.h"

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/// This function initializes all of the physical registers to zero.
//**************************************************************************
reg_map_t::reg_map_t(physical_file_t *p, int num_logical) {
  
  m_physical = p;
  m_num_physical = p->getNumRegisters();
  m_num_logical  = num_logical;
  ASSERT( m_num_physical >= m_num_logical );

  m_logical_reg = (uint16 *) malloc( sizeof(uint16) * m_num_logical );
  m_logical_rename_count = (uint16 *) malloc( sizeof(uint16) * m_num_logical );
  for (uint32 i = 0 ; i < m_num_logical ; i ++) {
    m_logical_reg[i] = PSEQ_INVALID_REG;
    m_logical_rename_count[i] = 0;
  }
  
  /* put all registers on the freelist */
  m_freelist = (uint16 *) malloc( sizeof(uint16) * m_num_physical );
  for (uint32 i = 0 ; i < m_num_physical ; i ++) {
    m_freelist[i] = i;
  }
#if defined(LXL_FREE_LIST) || defined(CHECK_REGFILE)
  m_bits = new bitfield_t( m_num_physical );
  for (uint32 i = 0 ; i < m_num_physical ; i ++) {
    m_bits->setBit(i, true);
  }
#endif
  m_freelist_index = m_num_physical - 1;
  
  m_validate_map = (bool *) malloc(sizeof(bool) * m_num_physical);

  has_fault = (bool*) malloc( sizeof(bool) * m_num_logical ) ;
  for( int i=0;i<m_num_logical;i++ ) {
	  has_fault[i] = false;
  };
  old_phys = PSEQ_INVALID_REG ;

}

/// This function initializes all of the physical registers to zero.
//**************************************************************************
reg_map_t::~reg_map_t() {
#if defined(LXL_FREE_LIST) || defined(CHECK_REGFILE)
  if (m_bits)
    delete m_bits;
#endif
  if (m_freelist)
    free( m_freelist );
  if (m_logical_reg)
    free( m_logical_reg );
  if (m_logical_rename_count)
    free( m_logical_rename_count );
  if (m_validate_map)
    free( m_validate_map );
}

//**************************************************************************
void
reg_map_t::setMapping(uint16 logical_reg, uint16 phys_reg)
{
  ASSERT(logical_reg < m_num_logical);
  ASSERT(phys_reg != PSEQ_INVALID_REG);
  m_logical_reg[logical_reg] = phys_reg;
  if (ALEX_RAT_DEBUG) {
      DEBUG_OUT("logical %d ", logical_reg);
      DEBUG_OUT("physical %d \n", phys_reg);
  }
}

/// allocate a new logical register (returns index)
//***************************************************************************
uint16
reg_map_t::regAllocate(uint16 logical_reg)
{
  uint32 physical_reg = m_freelist[m_freelist_index];

  ASSERT(m_physical->isReady(physical_reg) == false);
  ASSERT(m_physical->isWaitEmpty(physical_reg));
#ifdef CHECK_REGFILE
  ASSERT(m_bits->getBit(physical_reg) == true);
#endif

#ifdef LXL_FREE_LIST
  while (!m_bits->getBit(physical_reg)) {
	  m_freelist_index--;
	  physical_reg = m_freelist[m_freelist_index];
  }
#endif

  m_logical_rename_count[logical_reg]++;
  m_physical->setFree(physical_reg, false);

#if defined(LXL_FREE_LIST) || defined(CHECK_REGFILE)
  m_bits->setBit(physical_reg, false);
#endif
  m_freelist_index --;
  return physical_reg;
}

/// frees a logical register (param index The index to be freed)
//***************************************************************************
void 
reg_map_t::regFree(uint16 logical_reg, uint16 physical_reg)
{
  ASSERT(physical_reg < m_num_physical);
#if defined(LXL_FREE_LIST) || defined(CHECK_REGFILE)
  ASSERT(m_bits->getBit(physical_reg) == false);
  if (m_bits->getBit(physical_reg))
	  return;
  m_bits->setBit(physical_reg, true);
#endif
  // The register may or may not be ready -- if ready, is at retire,
  // if not ready, freed during squash --

  // wait list must be empty
  ASSERT(m_physical->isWaitEmpty(physical_reg));

  if (m_logical_rename_count[logical_reg] == 0) {
    ERROR_OUT("reg_map: error: zero rename count at free. logical_reg %d\n",
              logical_reg);
    ASSERT(m_logical_rename_count[logical_reg] != 0);
  }
  m_logical_rename_count[logical_reg]--;
  m_freelist_index ++;
  m_freelist[m_freelist_index] = physical_reg;
  m_physical->setUnready(physical_reg);
}

/// returns the number of logical registers in flight
//***************************************************************************
uint16
reg_map_t::countInFlight( uint16 logical_reg )
{
  if ( logical_reg > m_num_logical ) {
    ERROR_OUT("error: countInFlight: illegal parmeter: %d\n", logical_reg);
    SIM_HALT;
  }
  return (m_logical_rename_count[logical_reg]);
}

/// returns the number of physical registers remaining for allocation
//***************************************************************************
uint16
reg_map_t::countRegRemaining(void)
{
  return (m_freelist_index + 1);
}

//**************************************************************************
bool *
reg_map_t::buildFreelist( void ) const
{
  /* build a bitmap of allocated registers, then test all logical
     registers */

  /* initialize all as free */
  for (uint32 i = 0 ; i < m_num_physical; i ++ ) {
    m_validate_map[i] = true;
  }

  /* mark allocated ones as not free */
  for (uint32 i = 0 ; i < m_num_logical ; i ++ ) { 
    if (m_logical_reg[i] != PSEQ_INVALID_REG) {
      m_validate_map[m_logical_reg[i]] = false;
    }
  }

  /* compare a list of the allocated registers from a logical register
     file versus the global logical free list */
  for (int i = m_freelist_index ; i >= 0 ; i --) {
    if (m_validate_map[m_freelist[i]] == true) {
      // set this register to be "free"
      m_validate_map[m_freelist[i]] = false;
    } else {
      // register should never be already "free"
      SIM_HALT;
    }
  }

  return (m_validate_map);
}

//***************************************************************************
void
reg_map_t::leakCheck( uint32 id, const char *map_name ) const
{
  /* check for 'register leaks' */
  for (int i = 0; i < (int) m_num_physical; i++) {
    if (m_validate_map[i] == true) {
      // not on free list or allocated
      ERROR_OUT("[%d] register leak: %s. %d not allocated, and not on free list\n",
                id, map_name, i);
      print();
      SIM_HALT;
    }
  }
}

//**************************************************************************
bool
reg_map_t::addFreelist( uint16 id )
{
  if (m_validate_map[id] == true) {
    m_validate_map[id] = false;
  } else {
    return false;
  }
  return true;
}

//**************************************************************************
bool 
reg_map_t::equals(reg_map_t *other) {

  ASSERT(m_physical == other->m_physical);

  /* index from logical register to physical register */
  for (int i = 0 ; i < (int) m_num_logical; i ++) {
    if (m_logical_reg[i] != other->m_logical_reg[i]) {
      return false;
    }
  }
  return true;
}

//***************************************************************************
void
reg_map_t::print(void) const
{
  DEBUG_OUT("logical reg  -->  physical reg\n");
  /* index from logical register to physical register */
  for (int i = 0 ; i < (int) m_num_logical; i ++) {
    DEBUG_OUT("%3d %3d\n", i, m_logical_reg[i]);
  }

  DEBUG_OUT("freelist\n");
  for (int i = m_freelist_index ; i >= 0 ; i --) {
    DEBUG_OUT("    %3d\n", m_freelist[i] );
  }  
}

void reg_map_t::Reset(void) 
{

	for (uint32 i = 0 ; i < m_num_logical ; i ++) {
		m_logical_reg[i] = PSEQ_INVALID_REG;
		m_logical_rename_count[i] = 0;
	}
  
	/* put all registers on the freelist */
	for (uint32 i = 0 ; i < m_num_physical ; i ++) {
		m_freelist[i] = i;
	}
#if defined(LXL_FREE_LIST) || defined(CHECK_REGFILE)
	for (uint32 i = 0 ; i < m_num_physical ; i ++) {
		m_bits->setBit(i, true);
	}
#endif
	m_freelist_index = m_num_physical - 1;
  
	old_phys = PSEQ_INVALID_REG ;

}
