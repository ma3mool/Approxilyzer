
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
#ifndef _ARF_H_
#define _ARF_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include"diagnosis.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define UNKNOWN_ARF   0
#define INT_ARF       1
#define CONTROL_ARF   2
#define CC_ARF        3
#define SINGLE_ARF    4
#define DOUBLE_ARF    5

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Abstract register file super-class - implements a register interface for
* all logical and physical registers, based on which register type is
*
* @see     reg_box_t
* @author  cmauer
* @version $Id: arf.h 1.14 03/09/04 18:23:54-00:00 xu@cottons.cs.wisc.edu $
*/
class abstract_rf_t {
public:
  /**
   * Constructor: creates object
   * NOTE: all three pointers to this class can be NULL.
   */
  abstract_rf_t( physical_file_t *rf, reg_map_t *decode_map, 
                 reg_map_t *retire_map, uint32 reserves, fault_stats* fault_stat ) {
    m_rf = rf;
    m_decode_map = decode_map;
    m_retire_map = retire_map;
    m_reserves = reserves;
    initialize();
    arf_type=UNKNOWN_ARF;
    first_rat_err_on_read = false;
    m_fs = fault_stat;
  }

  // LXL: overloaded constructor for IDing type of arf
  abstract_rf_t( physical_file_t *rf, reg_map_t *decode_map, 
                 reg_map_t *retire_map, uint32 reserves,  
				 int in_arf_type, fault_stats *fault_stat) {
    m_rf = rf;
    m_decode_map = decode_map;
    m_retire_map = retire_map;
    m_reserves = reserves;
    initialize();
    arf_type=in_arf_type;
    first_rat_err_on_read = false;
    m_fs = fault_stat;
  }

  virtual ~abstract_rf_t( void ) {
    // not responsible for allocating, or freeing the rfs or maps
  };

  /**
   * @name Register Interfaces
   */
  //@{
  /** allocate a new physical register.
   *  @sideeffect  Modifies the rid field: m_physical.
   *  @return      Boolean: true if allocation succeeds.
   */
  virtual bool    allocateRegister( reg_id_t &rid );

  /** frees the physical register specified in the m_physical field */
  virtual bool    freeRegister( reg_id_t &rid );

  /** sets the map from virtual register (vanilla) to physical register
   *  @sideeffect  Modifies the register mapping. */
  virtual void    writeRetireMap( reg_id_t &rid );

  /** sets the map from virtual register (vanilla) to physical register
   *  @sideeffect  Modifies the register mapping. */
  virtual void    writeDecodeMap( reg_id_t &rid );
  
  /** reads the current map for the vanilla register, 
   *  updating the physical register.
   *  @sideeffect  Modifies the rid field: m_physical. */
  virtual void    readDecodeMap( reg_id_t &rid );

  /** wait until this register is written, then wakeup. */
  virtual void    waitResult( reg_id_t &rid, dynamic_inst_t *d_instr );

  /** returns true if registers are still available */
  virtual bool    regsAvailable( void );

  /** tests the physical register to see if it is ready.
   *  @return bool  true if ready, false if not. */
  virtual bool    isReady( reg_id_t &rid );
  
  /** check this register against simics's result */
  virtual void    check( reg_id_t &rid, pstate_t *state,
                         check_result_t *result, bool init );
  virtual void    sync( reg_id_t &rid, pstate_t *state,
                         check_result_t *result, bool init );

  virtual void   updateSimicsReg(reg_id_t &rid, pstate_t *state, uint64 value);
  /**  Given an array of physical registers representing the free registers
   *   in a machine, unset the bit(s) representing any registers that are
   *   used by this reg_id.
   *   Used to validate that there are no register file leaks.
   */
  virtual void    addFreelist( reg_id_t &rid );

  /** print out this register's mapping */
  virtual void    print( reg_id_t &rid );

  /** returns the number of times this logical register has been renamed */
  virtual half_t  renameCount( reg_id_t &rid );

  /** computes the logical register number associated with this register
   *  @returns The logical register number of this register identifier. */
  virtual half_t  getLogical( reg_id_t &rid );
  //@}

  /**
   * @name Read and write register interfaces
   */
  //@{
  /** read this register -- generic interface */
  virtual my_register_t readRegister( reg_id_t &rid );

  /** write this register -- generic interface */
  virtual void          writeRegister( reg_id_t &rid, my_register_t value );

  /** read this register as a integer register */
  virtual ireg_t  readInt64( reg_id_t &rid );
  
  /** read this register as a single precision floating point register */
  virtual float32 readFloat32( reg_id_t &rid );
  
  /** read this register as a double precision floating point register */
  virtual float64 readFloat64( reg_id_t &rid );
  
  /** write this register as a integer register */
  virtual void    writeInt64( reg_id_t &rid, ireg_t value );

  /** write this register as a single precision floating point register */
  virtual void    writeFloat32( reg_id_t &rid, float32 value );
  
  /** write this register as a double precision floating point register */
  virtual void    writeFloat64( reg_id_t &rid, float64 value );

  /** copy the previous control register values into this block */
  virtual void    initializeControl( reg_id_t &rid );

  /** finalize the control register writes */
  virtual void    finalizeControl( reg_id_t &rid );
  //@}

  /**
   * @name Flow instruction interfaces
   */
  //@{
  /** Get the last flow instruction(s) if any to write this register.
   *  There can be more than one, for instance, a double register could be
   *  written by 2 separate floating point instructions.
   */
  virtual flow_inst_t *getDependence( reg_id_t &rid );

  /** This register is written by this flow instruction, set the last
   *  dependence to this flow instruction.
   */
  virtual void         setDependence( reg_id_t &rid, flow_inst_t *writer );
  //@}

  /** returns the decode map associated with this abstract register file
   *  This is used for validating registers in the regbox. */
  reg_map_t           *getDecodeMap( void ) {
    return (m_decode_map);
  }

  reg_map_t           *getRetireMap( void ) {
    return (m_retire_map);
  }

  virtual void setFault( reg_id_t &rid ) ;
  virtual void clearFault( reg_id_t &rid ) ;
  virtual bool getFault( reg_id_t &rid ) ;
  virtual bool isPatched( reg_id_t &rid ) ;
  virtual void setPatched( reg_id_t &rid ) ;
  virtual void clearPatched( reg_id_t &rid ) ;
  virtual void setCorrupted(reg_id_t &rid);
  virtual void unSetCorrupted(reg_id_t &rid);
  virtual bool isCorrupted(reg_id_t &rid);
  virtual bool getReady( reg_id_t &rid ) ;

  virtual void setReadFault( reg_id_t &rid ) ;
  virtual void clearReadFault( reg_id_t &rid ) ;
  virtual bool getReadFault( reg_id_t &rid ) ;

  //virtual void setValue( reg_id_t &rid, uint64 value );

  int  getARFType() { return arf_type; };

  physical_file_t* getPhysicalRF() { return m_rf; }

  fault_stats* getFaultStat() { return m_fs; }

  int getRefCnt(reg_id_t& rid) ;

  bool getFirstErr() {return first_rat_err_on_read; }

  void setSyncTarget(reg_id_t &rid);
  bool checkSyncTarget(reg_id_t &rid);

  virtual void wakeDependents(reg_id_t &rid); 
  virtual void  setRFWakeup(bool value);

  virtual void clearAllFaults();

  virtual void rollbackReset();

  void setDiagnosis(diagnosis_t* diag) 
  {
      //m_diagnosis=diag;
  }

  void checkRegReadiness(reg_id_t &rid);

protected:
  /** Initializes this register file mapping */
  virtual void         initialize( void );

  /// physical register file associated with this register box
  physical_file_t   *m_rf;

  fault_stats *m_fs;

  /// The number of registers this arf must have in reserve before issuing
  ///      instructions to decode
  uint32             m_reserves;

  /// register mapping (at decode time) associated with this register box
  reg_map_t         *m_decode_map;

  /// register mapping (at retire time) associated with this register box
  reg_map_t         *m_retire_map;
  
  /// A mapping that gives a pointer to the last flow instruction (in program order) to write this register
  flow_inst_t      **m_last_writer;

  // LXL: ARF type
  int arf_type;

  // LXL: record first read
  bool first_rat_err_on_read;

  //  diagnosis_t *m_diagnosis;
};

/**
* Integer register file class: implements abstract interface for all
* integer registers. See abstract_rf_t for function descriptions.
*
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h 1.14 03/09/04 18:23:54-00:00 xu@cottons.cs.wisc.edu $
*/

class arf_int_t : public abstract_rf_t {
public:
  /// constructor
  arf_int_t( physical_file_t *rf, reg_map_t *decode_map, 
             reg_map_t *retire_map, uint32 reserves, fault_stats *fault_stat ) :
	  abstract_rf_t( rf, decode_map, retire_map, reserves, INT_ARF, fault_stat) {
	  initialize();
  }
  /// destructor
  ~arf_int_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  void    readDecodeMap( reg_id_t &rid );
  half_t  getLogical( reg_id_t &rid );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  void    sync( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  virtual void   updateSimicsReg(reg_id_t &rid, pstate_t *state, uint64 value);
  void rollbackReset();
  //@}
protected:
  void    initialize( void );
};

/**
* Integer register file class: implements abstract interface for all
* integer registers. See abstract_rf_t for function descriptions.
*
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h 1.14 03/09/04 18:23:54-00:00 xu@cottons.cs.wisc.edu $
*/

class arf_int_global_t : public abstract_rf_t {
public:
  /// constructor
  arf_int_global_t( physical_file_t *rf, reg_map_t *decode_map, 
                    reg_map_t *retire_map, fault_stats *fault_stat ) :
	  abstract_rf_t( rf, decode_map, retire_map, 0, INT_ARF, fault_stat ) {
    initialize();
  }

  /// destructor
  ~arf_int_global_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  void    readDecodeMap( reg_id_t &rid );
  half_t  getLogical( reg_id_t &rid );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  void    sync( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  void   updateSimicsReg(reg_id_t &rid, pstate_t *state, uint64 value);
  //@}

  /**
   * @name Read and write register interfaces
   */
  //@{
  void    writeInt64( reg_id_t &rid, ireg_t value );
  //@}
protected:
  void    initialize( void );
};

/**
* Floating point register file class: implements abstract interface for 
* single precision floating point registers.
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h 1.14 03/09/04 18:23:54-00:00 xu@cottons.cs.wisc.edu $
*/

class arf_single_t : public abstract_rf_t {
public:
  /// constructor
  arf_single_t( physical_file_t *rf, reg_map_t *decode_map, 
                reg_map_t *retire_map, uint32 reserves, fault_stats* fault_stat ) :
    abstract_rf_t( rf, decode_map, retire_map, reserves, SINGLE_ARF, fault_stat ) {
    initialize();
  }
  /// destructor
  ~arf_single_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  void    readDecodeMap( reg_id_t &rid );
  half_t  getLogical( reg_id_t &rid );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  void   updateSimicsReg(reg_id_t &rid, pstate_t *state, uint64 value);
  void rollbackReset();
  //@}

  /**
   * @name Read and write register interfaces
   */
  //@{
  float32 readFloat32( reg_id_t &rid );
  void    writeFloat32( reg_id_t &rid, float32 value );

  //@}
protected:
  void    initialize( void );
};

/**
* Floating point register file class: implements abstract interface for 
* double precision floating point registers.
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h 1.14 03/09/04 18:23:54-00:00 xu@cottons.cs.wisc.edu $
*/

class arf_double_t : public abstract_rf_t {
public:
  /// constructor
  arf_double_t( physical_file_t *rf, reg_map_t *decode_map, 
                reg_map_t *retire_map, fault_stats* fault_stat ) :
    abstract_rf_t( rf, decode_map, retire_map, 0, DOUBLE_ARF, fault_stat ) {
    initialize();
  }
  /// destructor
  ~arf_double_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  bool    allocateRegister( reg_id_t &rid );
  bool    freeRegister( reg_id_t &rid );
  void    readDecodeMap( reg_id_t &rid );
  void    writeRetireMap( reg_id_t &rid );
  void    writeDecodeMap( reg_id_t &rid );

  bool    isReady( reg_id_t &rid );
  void    waitResult( reg_id_t &rid, dynamic_inst_t *d_instr );
  half_t  getLogical( reg_id_t &rid );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  void    sync( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  void   updateSimicsReg(reg_id_t &rid, pstate_t *state, uint64 value);
  void    addFreelist( reg_id_t &rid );
  void    print( reg_id_t &rid );
  //@}
  /**
   * @name Read and write register interfaces
   */
  //@{
  my_register_t readRegister( reg_id_t &rid );
  void          writeRegister( reg_id_t &rid, my_register_t value );

  ireg_t  readInt64( reg_id_t &rid );
  void    writeInt64( reg_id_t &rid, ireg_t value );
  float64 readFloat64( reg_id_t &rid );
  void    writeFloat64( reg_id_t &rid, float64 value );
  //@}
  void setFault( reg_id_t &rid ) ;
  void clearFault( reg_id_t &rid ) ;
  bool getFault( reg_id_t &rid ) ;
  bool isPatched( reg_id_t &rid ) ;
  void setPatched( reg_id_t &rid ) ;
  void clearPatched( reg_id_t &rid ) ;

  bool getReady( reg_id_t &rid ) ;

  void setReadFault( reg_id_t &rid ) ;
  void clearReadFault( reg_id_t &rid ) ;
  bool getReadFault( reg_id_t &rid ) ;
  void wakeDependents(reg_id_t &rid); 

  void setCorrupted( reg_id_t &rid ) ;
  void unSetCorrupted(reg_id_t &rid);
  bool isCorrupted( reg_id_t &rid ) ;
protected:
  void    initialize( void );
};

/**
* A container class -- when you have multiple registers to rename
* like for 64-byte blocks (8 fp registers) or really ugly instructions
* which modify more than N registers, where N is the maximum number of
* destinations (SI_MAX_DEST), you need to use this container class.
* It allows you to construct a class dynamically which contains an
* arbitrary number of registers.
*
* Ideally you'd like to be able to have integer windowed registers, as
* well as integer global registers in the container class. However,
* as the container register can't store both the "cwp" and "gset" registers
* at the same time, this is currently infeasible. Thankfully, there
* are options as far as this is concerned (e.g. there are 3 other
* source registers)
*
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h 1.14 03/09/04 18:23:54-00:00 xu@cottons.cs.wisc.edu $
*/

class arf_container_t : public abstract_rf_t {
public:
  /// constructor
  arf_container_t( fault_stats *fault_stat/*void*/ ) :
    abstract_rf_t( NULL, NULL, NULL, 0, fault_stat ) {
    m_cur_type     = CONTAINER_NUM_CONTAINER_TYPES;
    m_cur_element  = -1;
    m_max_elements = -1;

    /* for safty sake, make it big enough */
    m_size = IWINDOW_ROB_SIZE * 4;
    for (int32 i = 0; i < CONTAINER_NUM_CONTAINER_TYPES; i++) {
      m_dispatch_size[i]= 0;
      m_dispatch_map[i] = NULL;
      m_rename_map[i]   = (reg_id_t **) malloc( sizeof(reg_id_t *) * m_size );
      m_rename_index[i] = 0;
    }
    initialize();
  }
  
  /// destructor
  ~arf_container_t( void );
  
  /**
   * @name Register Interfaces
   */
  //@{
  bool    allocateRegister( reg_id_t &rid );
  bool    freeRegister( reg_id_t &rid );
  void    readDecodeMap( reg_id_t &rid );
  void    writeRetireMap( reg_id_t &rid );
  void    writeDecodeMap( reg_id_t &rid );

  bool    isReady( reg_id_t &rid );
  void    waitResult( reg_id_t &rid, dynamic_inst_t *d_instr );
  half_t  getLogical( reg_id_t &rid );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  void    addFreelist( reg_id_t &rid );
  void    print( reg_id_t &rid );
  reg_id_t& getUnreadySubId(reg_id_t &rid);
  //@}

  /** defines a register type to have a certain number of elements. */
  void    openRegisterType( rid_container_t rtype, int32 numElements );
  /** Adds a register to the container type. The offset is used to modify
   *  the physical register given in the rid structure ("vanilla").
   */
  void    addRegister( rid_type_t regtype, int32 offset, abstract_rf_t *arf );
  /** closes the definition of a container register type */
  void    closeRegisterType( void );
  /**
   * @name Read and write register interfaces
   */
  //@{
  ireg_t  readInt64( reg_id_t &rid );
  void    writeInt64( reg_id_t &rid, ireg_t value );
  void    initializeControl( reg_id_t &rid );
  void    finalizeControl( reg_id_t &rid );
  //@}

protected:
  void    initialize( void );

  // current type of element being defined
  rid_container_t  m_cur_type;
  // current element being defined by an open register
  int32            m_cur_element;
  // maximum number of elements which can be defined
  int32            m_max_elements;
  // size of rename map (max num. of registers in flight)
  uint32           m_size;

  // size of element in the dispatch array
  uint32       m_dispatch_size[CONTAINER_NUM_CONTAINER_TYPES];
  // array of arf handlers for a particular container
  reg_id_t    *m_dispatch_map[CONTAINER_NUM_CONTAINER_TYPES];
  // per instruction window array for decode, retire mappings of inflight instructions
  reg_id_t   **m_rename_map[CONTAINER_NUM_CONTAINER_TYPES];
  // array of indicies into the dispatch map
  int32        m_rename_index[CONTAINER_NUM_CONTAINER_TYPES];
};

/**
* Condition code register file class: implements abstract interface for 
* condition code registers.
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h 1.14 03/09/04 18:23:54-00:00 xu@cottons.cs.wisc.edu $
*/
class arf_cc_t : public abstract_rf_t {
public:
  /// constructor
  arf_cc_t( physical_file_t *rf, reg_map_t *decode_map, 
            reg_map_t *retire_map, uint32 reserves, fault_stats* fault_stat ) :
    abstract_rf_t( rf, decode_map, retire_map, reserves, fault_stat ) {
    initialize();
  }
  /// destructor
  ~arf_cc_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  void    readDecodeMap( reg_id_t &rid );
  half_t  getLogical( reg_id_t &rid );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  void    sync( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  void   updateSimicsReg(reg_id_t &rid, pstate_t *state, uint64 value);
  //@}
protected:
  void    initialize( void );
};

/**
* Control register file class. Each control rid represents an entire
* set of control registers (%pc through %cleanwin). Having a source
* control register file means you can read from all registers in the set.
* Having a "destination" control register file means you can write 
* ANY register in the set.
*
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h 1.14 03/09/04 18:23:54-00:00 xu@cottons.cs.wisc.edu $
*/

class arf_control_t : public abstract_rf_t {
public:
  /// constructor
  arf_control_t( physical_file_t **rf_array, uint32 count, fault_stats *fault_stat ) :
    abstract_rf_t( NULL, NULL, NULL, 0, fault_stat ) {
    m_rf_count = count;
    if ( m_rf_count == 0 ) {
      DEBUG_OUT( "Error: ARF Control: NO control sets. Expect a crash.\n" );
    }
    m_rf_array = rf_array;
    m_rf_array[0]->setInt( CONTROL_ISREADY, 1 );
    m_retire_rf = 0;
    m_decode_rf = 0;
    initialize();
  }
  /// destructor
  ~arf_control_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  bool    allocateRegister( reg_id_t &rid );
  bool    freeRegister( reg_id_t &rid );
  void    readDecodeMap( reg_id_t &rid );
  void    writeRetireMap( reg_id_t &rid );
  void    writeDecodeMap( reg_id_t &rid );

  bool    isReady( reg_id_t &rid );
  void    waitResult( reg_id_t &rid, dynamic_inst_t *d_instr );
  half_t  renameCount( reg_id_t &rid );
  half_t  getLogical( reg_id_t &rid );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  void    sync( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init );
  bool    regsAvailable( void );
  void    addFreelist( reg_id_t &rid );

  void setFault( reg_id_t &rid ) ;
  inline void clearFault( reg_id_t &rid ) 
  {
	  if( /*!rid.isZero() &&*/ (rid.getVanilla() < CONTROL_NUM_CONTROL_TYPES)) {
        if( PRADEEP_DEBUG_CLEAR ) {
            DEBUG_OUT( "Clearing %d", rid.getPhysical() ) ;
            DEBUG_OUT( " of %s\n", reg_id_t::rid_type_menomic( rid.getRtype() ) ) ;
        }
        m_rf_array[m_retire_rf]->clearFault(rid.getVanilla()) ;
	  } else {
		  //assert(0);
	  }
  }
  inline void unSetCorrupted( reg_id_t &rid ) 
  {
	  if( /*!rid.isZero() &&*/ (rid.getVanilla() < CONTROL_NUM_CONTROL_TYPES)) {
        if( PRADEEP_DEBUG_CLEAR ) {
            DEBUG_OUT( "Clearing %d", rid.getPhysical() ) ;
            DEBUG_OUT( " of %s\n", reg_id_t::rid_type_menomic( rid.getRtype() ) ) ;
        }
        m_rf_array[m_retire_rf]->unSetCorrupted(rid.getVanilla()) ;
	  } else {
		  //assert(0);
	  }
  }
  bool getFault( reg_id_t &rid ) ;
  bool isPatched( reg_id_t &rid ) ;
  void setPatched( reg_id_t &rid ) ;
  void clearPatched( reg_id_t &rid ) ;
  bool getReady( reg_id_t &rid ) ;
  void setCorrupted( reg_id_t &rid ) ;
  bool isCorrupted( reg_id_t &rid ) ;
  void clearAllFaults();

  //@}
  /**
   * @name Read and write register interfaces
   */
  //@{
  ireg_t  readInt64( reg_id_t &rid );
  void    writeInt64( reg_id_t &rid, ireg_t value );
  void    initializeControl( reg_id_t &rid );
  void    finalizeControl( reg_id_t &rid );
  //@}

  /// returns the retirement register file
  physical_file_t *getRetireRF( void ) {
      return (m_rf_array[m_retire_rf]);
  }
  /// returns the decode register file
  physical_file_t *getDecodeRF( void ) {
    return (m_rf_array[m_decode_rf]);
  }

  void rollbackReset() {
	  m_rf_array[0]->setInt( CONTROL_ISREADY, 1 );
	  m_retire_rf = 0;
	  m_decode_rf = 0;
	  initialize();
  }

protected:
  void    initialize( void );

  /// number of control registers files which exist in the system
  uint32             m_rf_count;
  /// Oldest set of registers which is still alive
  uint32             m_retire_rf;
  /// Most recent set of registers allocated
  uint32             m_decode_rf;

  /// control physical register files
  physical_file_t  **m_rf_array;
};

/**
* Empty register file class. Does not implement any actions.
*
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h 1.14 03/09/04 18:23:54-00:00 xu@cottons.cs.wisc.edu $
*/
class arf_none_t : public abstract_rf_t {
public:
  /// constructor
  arf_none_t( physical_file_t *rf, reg_map_t *decode_map, 
             reg_map_t *retire_map, fault_stats *fault_stat ) :
    abstract_rf_t( rf, decode_map, retire_map, 0, fault_stat ) {
  }
  /// destructor
  ~arf_none_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  bool    allocateRegister( reg_id_t &rid ) { return (true); }
  bool    freeRegister( reg_id_t &rid ) { return (true); }
  void    writeRetireMap( reg_id_t &rid ) {}
  void    writeDecodeMap( reg_id_t &rid ) {}
  void    readDecodeMap( reg_id_t &rid ) {}
  bool    isReady( reg_id_t &rid ) { return (true); }
  void    waitResult( reg_id_t &rid, dynamic_inst_t *d_instr ) {}
  half_t  renameCount( reg_id_t &rid ) { return (0); }
  half_t  getLogical( reg_id_t &rid ) { return (0); }
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init ) {}
  void    synce( reg_id_t &rid, pstate_t *state, check_result_t *result, bool init ) {}

  /**
   * @name Read and write register interfaces
   */
  //@{
  ireg_t  readInt64( reg_id_t &rid ) { return (0); }
  void    writeInt64( reg_id_t &rid, ireg_t value ) {}
  //@}
protected:
  void    initialize( void ) { }
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _ARF_H_ */
