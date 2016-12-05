
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
#ifndef _REG_BOX_H_
#define _REG_BOX_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define WARN_OUT_OF_RANGE 0
/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
*  Abstract interface to all register maps, register files
*            to allow dynamic instructions more general decode, squash, 
*            schedule logic.
*
* [Documentation]
* @see     physical_file_t, reg_map_t
* @author  cmauer
* @version $Id: regbox.h 1.29 06/02/13 18:49:21-06:00 mikem@maya.cs.wisc.edu $
*/
class reg_box_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   */
  reg_box_t();

  /**
   * Destructor: frees object.
   */
  ~reg_box_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /** add a new group of registers to the register box */
  void    addRegisterHandler( rid_type_t       rtype,
                              abstract_rf_t   *arf );

  /** returns true if registers are available in the machine */
  bool    registersAvailable( void );

  /** validate all registers in the machine */
  void    validateMapping( uint32 id, iwindow_t &iwin );

  /** returns true if registers are available in the machine */
  inline abstract_rf_t *getARF( rid_type_t reg_type ) {
	  if (reg_type>=RID_NUM_RID_TYPES) 
		  return NULL;
    return (m_arf[reg_type]);
  }
  //@}

  /** @name Static Methods */
  //@{

  /** initializes register mappings. must be called before any other register
   *  activities are initiated.
   */
  static void  initializeMappings( void );

  /** converts an (integer, window pointer) pair into a single register number.
   *  note: arch_reg must be >= 8 (i.e. not a global) */
  static half_t  iregWindowMap( byte_t arch_reg, uint16 cwp ) {
#if WARN_OUT_OF_RANGE
    if (arch_reg < 8 || arch_reg > 31) {
      DEBUG_OUT("ireg map: invalid local: reg == %d\n", arch_reg);
    }
    if ( cwp > NWINDOWS - 1) {
      DEBUG_OUT("ireg map: invalid cwp: cwp == %d\n", cwp);
    }
#endif
    return ( (half_t) m_window_map[cwp][arch_reg] );
  }
  
  /** converts an (integer, global set) pair into a single register number.
   *  note: arch_reg must be < 8  (i.e. a global) */
  static half_t  iregGlobalMap( byte_t arch_reg, uint16 gset ) {
#if WARN_OUT_OF_RANGE
    if ( gset > 3 ) {
      DEBUG_OUT("global map: invalid set: gset == %d\n", gset);
    }
    if ( arch_reg > 7 ) {
      DEBUG_OUT("global map: invalid global: reg == %d\n", arch_reg);
    }
#endif
    return ( (half_t) m_global_map[gset][arch_reg] );
  }
  
  /** convert a (vanilla register, isPriv) pair into a single control reg */
  static int16   stateregMap( byte_t reg_no, bool isPriv ) {
#if WARN_OUT_OF_RANGE
    if (reg_no > CONTROL_NUM_CONTROL_TYPES) {
      DEBUG_OUT("state reg map: invalid reg: reg == %d\n", reg_no);
    }
#endif
    return ( (int16) m_control_map[isPriv][reg_no] );
  }

  /** returns the size of the control register */
  static int16   controlRegSize( control_reg_t reg ) {
#if WARN_OUT_OF_RANGE
    if ( reg > CONTROL_NUM_CONTROL_TYPES) {
      DEBUG_OUT("controlRegSize: invalid reg: reg == %d\n", reg);
    }
#endif
    return ( (int16) m_control_size[reg] );
  }
  //@}

private:
  /// register file dispatch: different handler for each rid_type
  abstract_rf_t *m_arf[RID_NUM_RID_TYPES];

  /** converts an (integer, window pointer) pair into a single register number.
   *  note: arch_reg must be >= 8 (i.e. not a global) */
  static half_t  initIregWindowMap( byte_t arch_reg, uint16 cwp );

  /** converts an (integer, global set) pair into a single register number.
   *  note: arch_reg must be < 8  (i.e. a global) */
  static half_t  initIregGlobalMap( byte_t arch_reg, uint16 gset );

  /** convert a (vanilla register, isPriv) pair into a single control reg */
  static int16   initStateregMap( byte_t reg_no, bool isPriv );
  /** initalizes the size of control registers */
  static int16   initControlSizeMap( control_reg_t reg );

  /// boolean indicating if the register box has been initialized yet
  static bool         m_is_init;
  /// mapping from [cwp][arch_reg] -> single register number
  static byte_t     **m_window_map;
  /// mapping from [gset][arch_reg] -> single register number
  static byte_t     **m_global_map;
  /// mapping from [priv][vanilla_reg] -> single register number
  static byte_t     **m_control_map;
  /// size of the control registers (in bits)
  static byte_t      *m_control_size;
};

/**
*  Abstract register identifier: contains a register bank (integer, floating
*    point, control code, etc, along with the register number.
*
* This allows  instructions more general decode, squash, schedule logic.
*
* @see     dynamic_inst_t, physical_file_t, reg_map_t
* @author  cmauer
* @version $Id: regbox.h 1.29 06/02/13 18:49:21-06:00 mikem@maya.cs.wisc.edu $
*/
class reg_id_t {

public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   */
  reg_id_t();

  /**
   * Destructor: frees object.
   */
  ~reg_id_t();
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  inline bool isZero( void ) {
    return ((m_rtype == RID_NONE) || 
            (m_rtype == RID_INT_GLOBAL && 
             m_vanilla == 0));
  }
  
  /// explicit copy constructor
  inline void copy( reg_id_t &other, reg_box_t &rbox ) {
    m_physical = other.m_physical;
    m_rtype    = other.m_rtype;
    m_vanilla  = other.m_vanilla;
    m_vstate   = other.m_vstate;
    m_arf      = rbox.getARF((rid_type_t) other.m_rtype);
  }

  /// access the rtype member
  inline rid_type_t getRtype( void ) {
    // if getRtype can return > 16 choices (constant 0xf), change setRtype too
    return ((rid_type_t) (m_rtype & 0xf) );
  }
  /// access the static (vanilla) register number
  inline byte_t     getVanilla( void ) {
    return ((byte_t) m_vanilla);
  }
  /// access the state associated with a "vanilla" register
  inline byte_t     getVanillaState( void ) {
    return ((byte_t) m_vstate);
  }

  /// access the abstract register file assoc with this register
  inline abstract_rf_t *getARF( void ) {
    return (m_arf);
  }

  /// return the rtype as a string
  static const char*rid_type_menomic( rid_type_t rtype );

  /// access the physical register number
  half_t            getFlatRegister( void );

  /** print out this register */
  void              print( void );

  /** print the disassembled form of this register */
  int               printDisassemble( char *str );

  /// access the physical register number
  inline half_t     getPhysical( void ) {
    return (m_physical);
  }

  /// set the rtype member
  inline void       setRtype( rid_type_t rtype ) {
    // if rtype > 16, then getRtype must change constant 0xf
    ASSERT((uint16) rtype < 16);
    m_rtype = (unsigned int) rtype;
  }

  /// set the static (vanilla) register number
  /// This function must also have no side-effects.
  inline void       setVanilla( byte_t vanilla ) {
    ASSERT(vanilla < 64);
    m_vanilla = vanilla;
  }

  /** set the state associated with a "vanilla" register. This function
   *  must have no side-effects as it is called multiple times for saves
   *  and restores.
   */
  inline void       setVanillaState( uint16 cwp, uint16 gset ) {
    ASSERT(cwp < NWINDOWS);
    ASSERT(gset <= REG_GLOBAL_INT);
    if (m_rtype == RID_INT) {
      m_vstate = cwp;
    } else if (m_rtype == RID_INT_GLOBAL) {
      if(m_vanilla == 8)
	DEBUG_OUT(" setting m_vstate = %d\n", gset);
      m_vstate = gset;
    }
  }
  
  /// set the physical register number
  inline void       setPhysical( half_t physical ) {
    m_physical = physical;
  }

  /// set the abstract register file assoc with this register:
  ///    note: you really shouldn't call this function 'copy' handles every
  //           case except for container classes
  inline void       setARF( abstract_rf_t *arf ) {
    m_arf = arf;
  }
  
  /** reset the dependency of this register */
  inline void       reset( void ) {
    m_physical    = PSEQ_INVALID_REG;
    m_rtype       = RID_NONE;
    m_vanilla     = 0;
    m_vstate      = 0;
    m_arf         = NULL;
  }

  /** setSelector: in the RID_CONTAINER register type, there is more than
   *               source/destination register possible. This function selects
   *               among the possible registers.
   */
  inline void       setSelector( byte_t selected ) {
    ASSERT( selected < 16 );
    m_rtype = (m_rtype & 0xf) | (selected << 4);
  }

  /** getSelector: in the RID_CONTAINER register type, there is more than
   *               source/destination register possible. This function returns
   *               the selected registers.
   */
  inline byte_t     getSelector( void ) {
    return ( (m_rtype >> 4) & 0xf );
  }
  //@}

  char* getVanPtr() { return &m_vanilla;}

  
protected:

  /// physical register number
  half_t         m_physical;

  /// register type identifier: lower 4 bits   complex selector: upper 4 bits
  /// for more about selectors, see arf_container_t, flat_container_t
  char           m_rtype;

  /// static "vanilla" register number
  char           m_vanilla;

  /// state associated with (in flight) vanilla identifier (cwp, gset)
  char           m_vstate;

  /// pointer to the abstract register file associated with this register type
  abstract_rf_t *m_arf;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _REG_BOX_H_ */
