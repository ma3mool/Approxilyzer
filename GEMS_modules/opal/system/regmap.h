
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
#ifndef _REG_MAP_H_
#define _REG_MAP_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "bitfield.h"
#include "regfile.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/
//#define LXL_FREE_LIST

/*------------------------------------------------------------------------*/
/* Class declarations                                                     */
/*------------------------------------------------------------------------*/

/**
* The logical register file in the microprocessor.
*
* @author  zilles
* @version $Id: regmap.h 1.15 06/02/13 18:49:21-06:00 mikem@maya.cs.wisc.edu $
*/
class reg_map_t {

public:

  /**
   * @name Constructor(s) / Destructor
   */
  //@{
  /** Constructor: creates an empty logical register file */
  reg_map_t(physical_file_t *p, int num_logical);
  /** Destructor: frees the object */
  virtual ~reg_map_t();
  //@}

  /// Convert a logical register to a physical register
  inline uint16 getMapping(uint16 logical_reg) const
  { 
/* 	  if ( logical_reg > m_num_logical) { */
/* 		  fprintf(stderr, "getmapping: invalid logical register %d > %d\n", */
/* 				  logical_reg, m_num_logical); */
/* 		  return PSEQ_INVALID_REG; */
/* 	  } */
#if 1 //def LL_DECODER
	  if (logical_reg >= m_num_logical) {
		  logical_reg = logical_reg % m_num_logical;
	  }
#endif
	  //assert(logical_reg<m_num_logical);

	  return m_logical_reg[logical_reg];
  }

  /// explicitly set a logical->physical mapping
  void   setMapping(uint16 logical_reg, uint16 phys_reg);

  /**
   *  @name Allocation of Registers
   */
  //@{
  /// allocate a new physical register (returns index)
  uint16 regAllocate(uint16 logical_reg);
  
  /// frees a physical register (param index The index to be freed)
  void   regFree(uint16 logical_reg, uint16 physical_reg);

  /// returns the number of logical registers in flight
  uint16 countInFlight(uint16 logical_reg);

  /// returns the number of physical registers remaining for allocation
  uint16 countRegRemaining(void);

  /// returns a count of the number of logical registers in this mapping
  uint32 getNumLogical( void ) {
    return m_num_logical;
  }
  //@}

  /// verify that all free'd registers are actually free
  bool  *buildFreelist( void ) const;
  
  /// add a register to the freelist
  bool   addFreelist( uint16 physical_reg );

  /// verify that no registers are "leaked"
  void   leakCheck( uint32 id, const char *map_name ) const;

  /** compare if two register mappings are equal
   *  @return bool true if match, false if not.
   */
  bool   equals(reg_map_t *other);

  /** print out the register mapping */
  void   print( void ) const;

  void setFault( uint16 reg_no ) {
	  if( reg_no < m_num_logical ) 
	  	has_fault[reg_no] = true ;
  }
  void clearFault( uint16 reg_no ) {
	  if( reg_no < m_num_logical ) 
	  	has_fault[reg_no] = false ;
  }
  bool getFault( uint16 reg_no ) {
	  if(reg_no<m_num_logical) 
		  return has_fault[reg_no] ;  
	  else
		  return false;
  }
  
  void setOldReg( half_t p ) { old_phys = p ; }
  half_t getOldReg() { return old_phys ; }         

  void injectTransientFault() {
      uint16 old_map = getMapping(m_physical->getFaultStat()->getFaultyReg());
      uint16 new_map = m_physical->getFaultStat()->injectFault(old_map);

      if( new_map != old_map ) {
          setFault( m_physical->getFaultStat()->getFaultyReg()) ;
		  // TODO - Should implement a corrupted bit for RAT_FAULT
          // setCorrupted( m_physical->getFaultStat()->getFaultyReg()) ;
          setOldReg( old_map ) ;
          setMapping(m_physical->getFaultStat()->getFaultyReg(), new_map);

		  // LXL: injectFault should have taken care of it
          // m_physical->getFaultStat()->markInjTransFault();
      }
  }
  void clearAllFaults( ) {
      for( int i=0;i<m_num_logical; i++ ) {
		  has_fault[i] = false;
	  }
  }

  void Reset( );


protected:
  /** pointer to the physical register file */
  physical_file_t *m_physical;

  /** number of logical registers in the mapping */
  uint32          m_num_logical;
  /** number of phyiscal registers in the register file */
  uint32          m_num_physical;

  /** index from logical register to physical register
   *  length: # of logical registers */
  uint16         *m_logical_reg;

  /** number of times a given logical register has been renamed.
   *  length: # of logical registers */
  uint16         *m_logical_rename_count;

#if defined(LXL_FREE_LIST) || defined(CHECK_REGFILE)
  /** a bit vector of allocated physical registers:
   *  bits is true if the register is free!
   *  length: # of physical registers */
  bitfield_t     *m_bits;
#endif

  /** an array of indicies of currently unallocated physical registers
   *  length: # of physical registers */
  uint16         *m_freelist;

  /** index in the freelist array to the next free register */
  uint16          m_freelist_index;

  /** A bitmap of free registers, used to validate there are no register leaks. */
  bool           *m_validate_map;
  /** bit to track if the injection was successful */
  bool           *has_fault ;
  /** Old register value **/
  half_t         old_phys ;

};

#endif /* _REG_MAP_H_ */
