
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
#ifndef _STATICI_H_
#define _STATICI_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "bitlib.h"
#include "regbox.h"
// #include "statici.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/** define source macros: */
const int     SI_MAX_SOURCE = 4;
const int     SI_MAX_DEST   = 2;

/** define a register use vector */
const byte_t  REG_BYTE    = 1 << 0;
const byte_t  REG_HALF    = 1 << 1;
const byte_t  REG_WORD    = 1 << 2;
const byte_t  REG_DOUBLE  = 1 << 3;
const byte_t  REG_QUAD    = 1 << 4;
const byte_t  REG_BLOCK   = 1 << 6; // 64-byte access
const byte_t  REG_SIGNED  = 1 << 7; // set if this is a signed access
const byte_t  REG_MEMSIZE = REG_BYTE | REG_HALF | REG_WORD | REG_DOUBLE |
                            REG_QUAD | REG_BLOCK;

/** define the flags vector */
const byte_t  SI_BIT0         = 1 << 0;
const byte_t  SI_ISIMMEDIATE  = 1 << 1;
/// overloaded 2nd bit: memops - isasi, controlops - static prediction
const byte_t  SI_ISASI        = 1 << 2;
const byte_t  SI_STATICPRED   = 1 << 2;

const byte_t  SI_UPDATE_CWP   = 1 << 3;
const byte_t  SI_WRITE_CONTROL= 1 << 4;
const byte_t  SI_FETCH_BARRIER= 1 << 5;
const byte_t  SI_COMPLEX_OP   = 1 << 6;
const byte_t  SI_TOFREE       = 1 << 7;

/** defines log-base2(SI_MAXSIZE) */
const uint32  SI_SIZE_BITS    =     7;
/** The maximum size (in bytes) of a static instruction */
const uint32  SI_MAX_SIZE     =   1<<SI_SIZE_BITS;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/** pointer to member function: nextPC - updates PC and NPC for this 
 *  dynamic instruction */
typedef void (dynamic_inst_t::*pmf_nextPC)( abstract_pc_t *a );

/**
 * This object represents a page header for a static instruction.
 * Ideally, you could maintain instruction virtual and physical PC
 * information on the per page basis. However to be completely general
 * (allowing instructions to be mapped to different virtual addresses)
 * prevents this.
 */
typedef struct si_page_header {
  /// The page's physical address
  pa_t      m_physical_addr;
} si_page_header_t;

/**
* This object represents a static instruction.
*
* Instructions ideally would be statically decoded once, and executed
* repeated from a cached copy. "static" instructions should contain
* all information which does not change from one execution to the next:
* instruction type, functional unit information, etc.
*
* @author  cmauer zilles
* @version $Id: statici.h 1.51 06/02/13 18:49:23-06:00 mikem@maya.cs.wisc.edu $
*/
class static_inst_t {

public:
  
  /** @name Constructor(s) / Destructor */
  //@{
  /**
   * Constructor: use default constructor
   */
  static_inst_t( pa_t physical_address, unsigned int inst );
  static_inst_t( pa_t physical_address, unsigned int inst, int32 id, int core_id, fault_stats *fault_stat );

  /**
   * Destructor: use default destructor
   */
  ~static_inst_t( );
  //@}

  /** Given a PC and an instruction, construct a static instruction
   *
   * Side-Effects: massively modified the static_inst member variables.
   *
   * @param physical_address The physical address of this instruction
   * @param inst The instruction itself (32-bit integer)
   */
  void decodeInstructionInfo( pa_t physical_address, unsigned int inst );
  void decodeInstructionInfo( pa_t physical_address, unsigned int inst, int32 fetch_id, int core_id, fault_stats *fault_stat );

  /** @name Accessor / Mutators */
  //@{
  /// get the "raw" instruction
  unsigned int        getInst( void ) const {
    return m_inst;
  }
  /// get the classification of this instruction
  dyn_execute_type_t  getType( void ) const {
    return (dyn_execute_type_t) m_type;
  }
  /// get the decoded opcode
  half_t              getOpcode( void ) const {
    return m_opcode;
  }

	/// get decode fail big
	bool                getDecodeFail( void ) const {
		return decode_fail ;
	}

  /// get the branch type of this instruction (if any)
  branch_type_t       getBranchType( void ) const {
    return ( (branch_type_t) m_branch_type );
  }
  /// if this is a branch (Type == DYN_CONTROL), returns the annul bit
  bool                getAnnul( void ) const {
    return ( (bool) maskBits32( m_inst, 29, 29 ) );
  }

  /// returns how many bits to shift before reading the cc register
  byte_t    getCCShift(void) { return (m_ccshift); };

  /// returns a pointer the start of all the registers.
  reg_id_t *getSourceRegBasePtr( void ) {
    return (&m_source_reg[0]);
  }
  /// returns a source register
  reg_id_t &getSourceReg( int sourcenum ) {
    ASSERT( sourcenum < SI_MAX_SOURCE );
    return (m_source_reg[sourcenum]);
  }
  /// returns a destinatoin register
  reg_id_t &getDestReg( int destnum ) {
    ASSERT( destnum < SI_MAX_DEST );
    return (m_dest_reg[destnum]);
  }
  
  /// returns the basic functional unit type which this instruction requires
  fu_type_t getFuType() const { return (fu_type_t) (m_futype); }
  /// returns the target PC for branches or memory instruction offset
  my_addr_t getOffset() const { return (m_imm); }
  /// increment the static instruction reference counter (on allocation)
  void      incrementRefCount( void ) {
    m_ref_count++;
  }
  /// decrement the static instruction reference counter (on free)
  void      decrementRefCount( void ) {
    m_ref_count--;
    ASSERT(m_ref_count >= 0);
  }
  /// get the reference count on this instruction
  uint16    getRefCount( void ) {
    return (m_ref_count);
  }
  /// get a pointer to the stats structure for this static instruction
  static_stat_t *getStats( void );
  /** print a disassembly of this static instruction to a buffer.
   *  Disassembly works but is still primitive. Feel free to improve it.
   *  1) static instructions don't have access to VPC info, so you
   *     may want do something using dynamic instructions to get that.
   *  2) saves/restores are branches (DYN_CONTROL) as well as ALU ops
   *  3) ASI information is not available
   */
  int       printDisassemble( char *str );

  /** @name Flags */
  //@{
  /// set a static instruction flag
  void      setFlag( const byte_t flag, bool value ) {
    if (value)
      m_flags |= flag;
    else
      m_flags &= ~flag;
  }
  /// get a static instruction flag
  bool      getFlag( const byte_t flag ) const { 
    return ((m_flags & flag) != 0); }
  //@}

  /// returns the (sign extended) immediate value
  ireg_t    getImmediate() const { return m_imm; }
  /// returns flags indicating register use --
  byte_t    getAccessSize() const { return (m_access_size & REG_MEMSIZE); }
  /// returns a flag indicating if this is a signed or unsigned load
  bool      getAccessSigned() const {
    return ((m_access_size & REG_SIGNED) != 0);
  }
#ifdef LXL_FIND_BITFLIP
  byte_t getFlags() const {return m_flags;}
  byte_t getRawAccessSize() const {return m_access_size;}

#endif
  //@}

  /** @name Next Program Counter, Dynamic Execute Function */
  //@{
  /// nextPC: computes the next PC, nPC for a given instruction.
  pmf_nextPC          nextPC;  

  /** normal (non-branching) execute semantics */
  void     actualPC_execute( abstract_pc_t *a );
  /** given if a branch is taken, produce the PC, nPC result. */
  void     actualPC_branch( bool isTaken, abstract_pc_t *a );
  /** given an unconditional branch, produce the PC, nPC result */
  void     actualPC_uncond_branch( bool isTaken, abstract_pc_t *a );
  /** given the effective addr of a jmp, produce the PC, nPC result */
  void     actualPC_indirect( my_addr_t effective_addr, abstract_pc_t *a );
  /** given the effective addr of a trap, produce the PC, nPC result */
  void     actualPC_trap( bool isTaken, my_addr_t  effective_addr,
                          abstract_pc_t *a );
  /// return from trap (done, retry) instructions
  void     actualPC_trap_return( abstract_pc_t *a,
                                 my_addr_t tpc, my_addr_t tnpc,
                                 ireg_t pstate, ireg_t tl, ireg_t cwp );
  /** cwp control transfer */
  void     actualPC_cwp( abstract_pc_t *a );
  //@}

  /** @name overload new operator for static array allocation */
  //@{
  /** calls the constructor on a pre-allocated piece of memory
   **/
  void *operator new( size_t size ) {
    return ( :: operator new(size) );
  }
  void *operator new( size_t size, static_inst_t *preallocated_p ) {
    return ( preallocated_p );
  }
  //@}

  /// prints out this static instruction
  void      printDetail(out_intf_t *io);

  unsigned int correct_inst;

  void setFault() { has_fault = true ; }
  void clearFault() { has_fault = false ; }
  bool getFault() { return has_fault ; }

  //LXL: This is to compare whether the decoder fault has caused the
  //dest reg to be mutated. If so, then the faulty instr needs to get
  //info from the original dest reg
  bool compareDestRegs(static_inst_t *A) {
      for (int i=0; i<SI_MAX_DEST; i++) {
	      reg_id_t &dest=A->getDestReg(i);
	      if ((dest.getRtype() != m_dest_reg[i].getRtype()) ||
	          (dest.getVanilla() != m_dest_reg[i].getVanilla()) ||
	          (dest.getVanillaState() != m_dest_reg[i].getVanillaState()))
	          return false;

      }
      return true;
  }

  void getOrigDest(static_inst_t *A) {
      for (int i=0; i<SI_MAX_DEST; i++) {
	      reg_id_t &dest=A->getDestReg(i);
	      orig_dest_reg[i].setRtype(dest.getRtype());
	      orig_dest_reg[i].setVanilla(dest.getVanilla());
	      orig_dest_reg[i].setVanillaState(dest.getVanillaState(),dest.getVanillaState());
      }
  }

  void setChangedDest(bool value) { changed_dest = value; }
  bool getChangedDest() { return changed_dest; }

  reg_id_t &getOrigDestReg(int destnum) { 
      ASSERT( destnum < SI_MAX_DEST );
      return (orig_dest_reg[destnum]);
  }

  int getFetchId() {return m_fetch_id;}

  void setExitCode(int value) { m_exit_code = value; }
  int  getExitCode() { return m_exit_code; }

  void setOriginalInstr(static_inst_t*orig);// { original=orig;}
  static_inst_t* getOriginalInstr() { return original;}

  bool getLLFail() { return ll_decode_fail ; }
  bool getAssertFail() { return assert_fail; }

private:

  /// set the nextPC function based on the opcode, and instruction
  void      setNextPC( void );
  /** called when an instruction can't be properly decoded.
   *  Resets dependencies, prints a warning, and continues.
   */
  void      decodeFails( int line, uint32 inst );
  /// set initial invalid opcode
  void      initialize( void );

  /// prints this instruction to the character string str (returns len)
  int32     printNormalDis( char *str );
  /// prints this ctl instruction to the character string str (returns len)
  int32     printControlDis( char *str );
  /// prints this mem instruction to the character string str (returns len)
  int32     printMemoryDis( char *str );

  /// decoded opcode
  half_t    m_opcode;

	/// Did decoding fail?
	bool      decode_fail ;
	/// Did decoding fail because of LL
	bool 	  ll_decode_fail ; 
	/// Did the assertion catch the fault?
	bool      assert_fail ;

  /// The undecoded "raw" instruction to execute
  uint32    m_inst;

  /// register identifiers for the sources
  reg_id_t  m_source_reg[SI_MAX_SOURCE];
  /// register identifiers for the destination registers
  reg_id_t  m_dest_reg[SI_MAX_DEST];

  /** 64-bit sign-extended value
   *     * ALU ops use this (if SI_ISIMMEDIATE) as the second input operand
   *     * Control ops use this as a target PC for branches
   */
  ireg_t    m_imm;

  /// A pointer to statistics tracking data structure
  static_stat_t *m_stats;
  
  /// The number of in-flight instances of this instruction
  int16     m_ref_count;

  /// The register use, defined by the bit vectors REG_ parameters
  byte_t    m_access_size;
  /** The condition code: cc1 cc0 which selects icc or xcc.
   *  ccshift == 0 for icc, ccshift == 4 for xcc
   */
  byte_t    m_ccshift;
  
  /// boolean flags
  //@{
  /// a bit vector of 8 flags: SI_ISIMMEDIATE, SI_UPDATE_CWP, ...
  byte_t    m_flags;
  //@}

  /// Type of instruction (for statistics counting)
  byte_t    m_type;
  /// Type of control instructions (for fetch / execution)
  byte_t    m_branch_type;
  /// functional unit for executing
  byte_t    m_futype;
  /// Is this inst corrupted
  bool      has_fault ;
  // Has the dest reg been corrupted?
  bool      changed_dest;
  reg_id_t  orig_dest_reg[SI_MAX_DEST];

  int m_fetch_id;
  int       m_exit_code;

  static_inst_t* original;
  
  // Function to help LL_DECODER
#if defined( LL_DECODER) || defined(LXL_FIND_BITFLIP)
  ireg_t account_flips(string s, bool vlog_valid, uint64 vlog, uint64 opal, fault_stats *fs, int* num_flips) ;
  bool validAccessSize(byte_t access_size);
  bool validRtype();
  bool validTypes();
#endif //LL_DECODER

};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif /* _STATICI_H_ */
