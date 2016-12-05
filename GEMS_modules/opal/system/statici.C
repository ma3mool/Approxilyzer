
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
 * FileName:  static.C
 * Synopsis:  static instruction (decode instruction) implementation
 * Author:    cmauer
 * Version:   $Id: statici.C 1.67 06/02/13 18:49:23-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "pstate.h"
#include "abstractpc.h"
#include "opcode.h"
#include "sstat.h"
#include "statici.h"
#include "ipage.h"
#include "dynamic.h"
#include "system.h"

#ifdef LL_DECODER
//structuralModule design;
#endif //LL_DECODER
faultSimulator fSim;

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/** static zero register indicates read/write to %go == no dependence */
const int STATIC_ZERO_REG = 0;

#define   I_RS1   m_source_reg[0]
#define   I_RS2   m_source_reg[1]
#define   I_RS3   m_source_reg[2]
#define   I_RS4   m_source_reg[3]

#define   I_RD1   m_dest_reg[0]
#define   I_RD2   m_dest_reg[1]

/// define an integer register
#define SI_INT_RID( vanilla, I_REG )      \
    if ( vanilla < 8 )                  \
I_REG.setRtype( RID_INT_GLOBAL ); \
else                                \
I_REG.setRtype( RID_INT );        \
I_REG.setVanilla( vanilla );

/// define a single precision floating point register
#define SI_SINGLE_RID( vanilla, I_REG )                   \
    I_REG.setVanilla( SDQ_TWIST(RID_SINGLE, vanilla) ); \
I_REG.setRtype( RID_SINGLE );

/// define a double precision floating point register
#define SI_DOUBLE_RID( vanilla, I_REG )   \
    I_REG.setVanilla( SDQ_TWIST(RID_DOUBLE, vanilla) ); \
I_REG.setRtype( RID_DOUBLE );

#define SI_WRITE_FSR                      \
    SI_WRITE_CONTROL( CONTROL_FSR );

/// define a quad   precision floating point register
#define SI_QUAD_RID( vanilla, I_REG )     \
    I_REG.setVanilla( SDQ_TWIST(RID_QUAD, vanilla) ); \
I_REG.setRtype( RID_QUAD );

/// define a condition code register
#define SI_CC_RID( vanilla, I_REG )      \
    I_REG.setRtype( RID_CC );          \
I_REG.setVanilla( vanilla );

/// defines writing a control register
#define SI_WRITE_CONTROL( vanilla )        \
    I_RS4.setRtype( RID_CONTROL );       \
I_RS4.setVanilla( vanilla );         \
I_RD2.setRtype( RID_CONTROL );       \
I_RD2.setVanilla( vanilla );         \
setFlag(SI_WRITE_CONTROL, true );    \
COMPLEX_OPERATION;

/// defines reading a control register
#define SI_READ_CONTROL( vanilla )        \
    I_RS4.setRtype( RID_CONTROL );      \
I_RS4.setVanilla( vanilla );        \
COMPLEX_OPERATION;

/// defines ASI access by memory instruction
#define SI_READASI                                \
    SI_READ_CONTROL( CONTROL_ASI );               \
setFlag(SI_ISASI, true);

#define FETCH_BARRIER                     \
    setFlag(SI_FETCH_BARRIER, true);

/// reads the integer condition codes
#define SI_READ_ICC                       \
    SI_CC_RID( REG_CC_CCR, I_RS3 );

/// writes the integer condition codes
#define SI_WRITE_ICC                      \
    SI_CC_RID( REG_CC_CCR, I_RD2 );

/// reads the Y register
#define SI_READ_Y                        \
    SI_READ_CONTROL( CONTROL_Y );

/// writes the Y register
#define SI_WRITE_Y                        \
    SI_WRITE_CONTROL( CONTROL_Y );

/// twists the 5-bit single, double, quad identifier around to be 'normal'
#define SDQ_TWIST( sdq, reg )             \
    ( sdq == RID_SINGLE ? reg :            \
      ( ((reg & 0x1) << 5) | (reg & ~0x1) ) )

/// two register alu operation, with a conditional third based on the "i" bit
#define REG_TWO_ALU( FUTYPE )             \
    m_futype = FUTYPE;                  \
SI_INT_RID( rd, I_RD1 );            \
SI_INT_RID( rs1, I_RS1 );           \
if (i == 0) {                       \
    setFlag(SI_ISIMMEDIATE, false );          \
    SI_INT_RID( rs2, I_RS2 );         \
} else {                            \
    setFlag(SI_ISIMMEDIATE, true );           \
    m_imm = sign_ext64( maskBits32( inst, 12, 0 ), 12 ); \
}

/// two register shift operation, with a conditional third based on the "i" bit
#define REG_TWO_SHIFT                 \
    x = maskBits32( inst, 12, 12 );     \
m_futype = FU_INTALU;               \
SI_INT_RID( rd, I_RD1 );            \
SI_INT_RID( rs1, I_RS1 );           \
if (i == 0) {                       \
    SI_INT_RID( rs2, I_RS2 );         \
    setFlag(SI_ISIMMEDIATE, false );          \
} else if (x == 1) {                \
    m_imm = maskBits32( inst, 5, 0 ); \
    setFlag(SI_ISIMMEDIATE, true );           \
} else if (x == 0) {                \
    m_imm = maskBits32( inst, 4, 0 ); \
    setFlag(SI_ISIMMEDIATE, true );           \
}

/// destination register, and one source register, dependant on the i bit
#define REG_ONE_MOVE                      \
    m_futype = FU_NONE;                 \
SI_INT_RID( rd, I_RD1 );            \
SI_INT_RID( rd, I_RS1 );            \
if (i == 0) {                       \
    setFlag(SI_ISIMMEDIATE, false );          \
    SI_INT_RID( rs2, I_RS2 );         \
} else {                            \
    setFlag(SI_ISIMMEDIATE, true );           \
    m_imm = sign_ext64( maskBits32( inst, 10, 0 ), 10 ); \
}

/// destination register, source1 for sure, source2 dependant on the i bit
#define REG_TWO_I( IMM_X ) \
    SI_INT_RID( rd, I_RD1 );            \
SI_INT_RID( rs1, I_RS1 );           \
if (i == 0) {                       \
    setFlag(SI_ISIMMEDIATE, false );          \
    SI_INT_RID( rs2, I_RS2 );         \
} else {                            \
    setFlag(SI_ISIMMEDIATE, true );           \
    m_imm = sign_ext64( maskBits32( inst, IMM_X, 0 ), IMM_X ); \
}

/// read instruction: including a
///   destination register, source1 for sure, source2 dependant on the i bit
///   also: depends on the pstate control pstate register- allows for
///         loads to stall if mmu is being modified (stxa instructions)
#define  READ_TWO_I                       \
    m_futype = FU_RDPORT;               \
SI_INT_RID( rd, I_RD1 );            \
SI_INT_RID( rs1, I_RS1 );           \
if (i == 0) {                       \
    setFlag(SI_ISIMMEDIATE, false );          \
    SI_INT_RID( rs2, I_RS2 );         \
} else {                            \
    setFlag(SI_ISIMMEDIATE, true );           \
    m_imm = sign_ext64( maskBits32( inst, 12, 0 ), 12 ); \
}

/// write instruction: including a
///   source1 for sure, source2 dependant on the i bit
///   The register to be written is the third source
#define  WRITE_TWO_I                      \
    m_futype = FU_WRPORT;               \
/* destination is treated as src3*/ \
SI_INT_RID( rd, I_RS3 );            \
SI_INT_RID( rs1, I_RS1 );           \
if (i == 0) {                       \
    setFlag(SI_ISIMMEDIATE, false );          \
    SI_INT_RID( rs2, I_RS2 );         \
} else {                            \
    setFlag(SI_ISIMMEDIATE, true );           \
    m_imm = sign_ext64( maskBits32( inst, 12, 0 ), 12 ); \
}

/// read instruction: including a
///   destination register, source1 for sure, source2 dependant on the i bit
#define  READ_TWO_F( sdq )                \
    m_futype = FU_RDPORT;               \
I_RD1.setRtype( sdq );              \
I_RD1.setVanilla( SDQ_TWIST(sdq, rd) );             \
SI_INT_RID( rs1, I_RS1 );           \
if (i == 0) {                       \
    setFlag(SI_ISIMMEDIATE, false );          \
    SI_INT_RID( rs2, I_RS2 );         \
} else {                            \
    setFlag(SI_ISIMMEDIATE, true );           \
    m_imm = sign_ext64( maskBits32( inst, 12, 0 ), 12 ); \
}

/// write instruction: including a
///   source1 for sure, source2 dependant on the i bit
///   The register to be written is the third source
#define  WRITE_TWO_F( sdq )               \
    m_futype = FU_WRPORT;               \
/* destination is treated as src3*/ \
I_RS3.setRtype( sdq );              \
I_RS3.setVanilla( SDQ_TWIST(sdq, rd) );  \
SI_INT_RID( rs1, I_RS1 );           \
if (i == 0) {                       \
    setFlag(SI_ISIMMEDIATE, false );          \
    SI_INT_RID( rs2, I_RS2 );         \
} else {                            \
    setFlag(SI_ISIMMEDIATE, true );           \
    m_imm = sign_ext64( maskBits32( inst, 12, 0 ), 12 ); \
}

#define  COMPLEX_OPERATION                      \
    setFlag(SI_COMPLEX_OP, true );

#define  INT_SIGNED \
    m_access_size |= REG_SIGNED;

#define  INT_BYTE \
    m_access_size = REG_BYTE;

#define  INT_HALF \
    m_access_size = REG_HALF;

#define  INT_WORD \
    m_access_size = REG_WORD;

#define  INT_EXTENDED \
    m_access_size = REG_DOUBLE;

#define  INT_QUAD \
    m_access_size = REG_QUAD;

#define  FLOAT_SINGLE \
    m_access_size = REG_WORD;

#define  FLOAT_DOUBLE \
    m_access_size = REG_DOUBLE;

#define  FLOAT_QUAD   \
    m_access_size = REG_QUAD;

#define  FLOAT_BLOCK  \
    m_access_size = REG_BLOCK;

#define  FP_MONO_OP( sdq )                       \
    SI_ ## sdq ## _RID(rd, I_RD1);                 \
SI_ ## sdq ## _RID(rs2, I_RS2);

#define  FP_MOVE_OP( sdq )                       \
    SI_ ## sdq ## _RID(rd, I_RD1);                 \
SI_ ## sdq ## _RID(rd, I_RS1);                 \
SI_ ## sdq ## _RID(rs2, I_RS2);

#define  FP_TRI_OP( sdq )                        \
    SI_ ## sdq ## _RID(rd, I_RD1);                 \
SI_ ## sdq ## _RID(rs1, I_RS1);                \
SI_ ## sdq ## _RID(rs2, I_RS2);

#define  FP_CMP( sdq )                           \
    SI_ ## sdq ## _RID(rs1, I_RS1);                \
SI_ ## sdq ## _RID(rs2, I_RS2);

#define  SI_RESET( I_R )         \
    I_R.reset();

#define FAIL()                   \
    decodeFails( __LINE__, inst ); \
goto decode_finished;

#define LL_FAIL()                   \
	ERROR_OUT("LL_DECODER failed - "); \
	ll_decode_fail = true ; \
    decodeFails( __LINE__, inst );

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/** Construct a static instruction (calls decodeInstruction)
 **  I couldn't think of a good reason not to have all of the initialization
 **  not encapsulated in the constructor. */
//**************************************************************************
static_inst_t::static_inst_t( pa_t physical_address, unsigned int inst )

{
    has_fault = false ;
    changed_dest = false;
    decode_fail = false ;
    ll_decode_fail = false ;
    assert_fail = false ;
	m_exit_code = 0;
    decodeInstructionInfo( physical_address, inst );
	setNextPC() ;
    m_ref_count = 0;
    m_stats = NULL;
	original = NULL;
}

// Overloaded construtor to help with fault injection
static_inst_t::static_inst_t( pa_t physical_address, unsigned int inst, int32 id, int core_id, fault_stats *fault_stat )

{
    has_fault = false ;
    changed_dest = false;
    decode_fail = false ;
    ll_decode_fail = false ;
    assert_fail = false ;
	m_exit_code = 0;
    decodeInstructionInfo( physical_address, inst, id, core_id, fault_stat );
	m_ref_count = 0;
    m_stats = NULL;
	original = NULL;

#ifdef DECODER_CHECKER

#ifdef VAR_TRACE
	// for(int i=0;i<32;i++) {
	// 	TRACE_OUT("inst[%d]\n%d\n1\n",i,maskBits32(inst,i,i)) ;
	// } 
    TRACE_OUT("inst_1\n%d\n", maskBits32( inst, 30, 31 )) ;
    TRACE_OUT("inst_2\n%d\n", maskBits32( inst, 22, 24 )) ;
    TRACE_OUT("inst_3\n%d\n", maskBits32( inst, 19, 24 )) ;
    TRACE_OUT("inst_4\n%d\n", maskBits32( inst, 25, 29 )) ;
    // TRACE_OUT("inst_5\n%d\n", maskBits32( inst, 14, 18 )) ;
    // TRACE_OUT("inst_6\n%d\n", maskBits32( inst, 0, 4 )) ;
    // TRACE_OUT("inst_7\n%d\n", maskBits32( inst, 9, 13 )) ;
    // TRACE_OUT("inst_8\n%d\n", maskBits32( inst, 5, 8 )) ;
    // TRACE_OUT("inst_9\n%d\n", maskBits32( inst, 25, 28 )) ;
    // TRACE_OUT("inst_10\n%d\n", maskBits32( inst, 14, 17 )) ;
    // TRACE_OUT("inst_11\n%d\n", maskBits32( inst, 10, 12 )) ;
    // TRACE_OUT("inst_12\n%d\n", maskBits32( inst, 13, 13 )) ;
    // TRACE_OUT("inst_13\n%d\n", maskBits32( inst, 12, 5 )) ;
	TRACE_OUT("m_type\n%d\n",m_type ) ;
	TRACE_OUT("m_futype\n%d\n",m_futype ) ;
	TRACE_OUT("m_opcode\n%d\n",m_opcode ) ;
	TRACE_OUT("m_branch_type\n%d\n",m_branch_type ) ;
	TRACE_OUT("m_imm\n%d\n",m_imm ) ;
	TRACE_OUT("m_ccshift\n%d\n",m_ccshift ) ;
	// TRACE_OUT("m_access_size\n%d\n2\n",m_access_size ) ;
	// for(int i=0;i<8;i++) {
	// 	TRACE_OUT("m_flags[%d]\n%d\n",i,maskBits32(m_flags,i,i)) ;
	// }
	TRACE_OUT("\n") ;
# else // VAR_TRACE
	// // Assertion 1: (inst_3!=0) => (m_opcode!=0)    0.7511, 1.2389, 1103798
	// if(maskBits32(inst,19,24)!=0 && m_opcode==0) {
	// Assertion 2 : inst_2=0 => m_futype!=0 : 0.4045, 1.0881, 91661
	if(maskBits32( inst, 22, 24)==0 && m_futype==0) {
		assert_fail = true ;
	}

#endif // VAR_TRACE

#endif //DECODER_CHECKER

	// DEBUG_OUT("\ninst          %8x\n",inst ) ;
	// DEBUG_OUT("rd            %x\t%x\t%x\n",I_RD1.getVanilla(),I_RD1.getRtype(),(I_RD1.getRtype()!=RID_NONE)) ;
	// DEBUG_OUT("rd2           %x\t%x\t%x\n",I_RD2.getVanilla(),I_RD2.getRtype(),(I_RD2.getRtype()!=RID_NONE)) ;
	// DEBUG_OUT("rs1           %x\t%x\t%x\n",I_RS1.getVanilla(),I_RS1.getRtype(),(I_RS1.getRtype()!=RID_NONE)) ;
	// DEBUG_OUT("rs2           %x\t%x\t%x\n",I_RS2.getVanilla(),I_RS2.getRtype(),(I_RS2.getRtype()!=RID_NONE)) ;
	// DEBUG_OUT("rs3           %x\t%x\t%x\n",I_RS3.getVanilla(),I_RS3.getRtype(),(I_RS3.getRtype()!=RID_NONE)) ;
	// DEBUG_OUT("rs4           %x\t%x\t%x\n",I_RS4.getVanilla(),I_RS4.getRtype(),(I_RS4.getRtype()!=RID_NONE)) ;
	// DEBUG_OUT("m_type        %x\n",m_type ) ;
	// DEBUG_OUT("m_futype      %x\n",m_futype ) ;
	// DEBUG_OUT("m_opcode      %x\n",m_opcode ) ;
	// DEBUG_OUT("m_branch_type %x\n",m_branch_type ) ;
	// DEBUG_OUT("m_imm         %x\n",m_imm ) ;
	// DEBUG_OUT("m_ccshift     %x\n",m_ccshift ) ;
	// DEBUG_OUT("m_access_size %x\n",m_access_size ) ;
	// DEBUG_OUT("m_flags       %x\n",m_flags ) ;
	// DEBUG_OUT("fail          %x\n",decode_fail) ;
}
//**************************************************************************
static_inst_t::~static_inst_t( )
{
    if (m_stats != NULL) {
        delete m_stats;
        m_stats = NULL;
    }
	if (original != NULL) {
		delete original;
		original = NULL;
	}
}

/** Based on the instruction type, set the nextPC function pointer.
 ** This allows the next PC to be set by a function call (lower overhead
 ** more reuable than a switch statement.
 */
//**************************************************************************
void   static_inst_t::setNextPC( void )
{
    bool annulBit;
    int  cond = -1;


    nextPC = NULL;
    switch ( m_type ) {

        case DYN_EXECUTE:
            nextPC = &dynamic_inst_t::nextPC_execute;
            break;

        case DYN_CONTROL:
            // next program counter depends on type of the branch ...
            switch ( m_branch_type ) {

                case BRANCH_UNCOND:
                    cond = maskBits32( m_inst, 25, 28 );
                    annulBit = (bool) maskBits32( m_inst, 29, 29 );
                    if (cond == 8) {               // cond == binary 1000 => branch always
                        // most logical uncond branch -- branch always
                        if ( annulBit ) {
                            // unconditional, taken, annulled
                            nextPC = &dynamic_inst_t::nextPC_taken_a;
                        } else {
                            // unconditional, taken, not annulled
                            nextPC = &dynamic_inst_t::nextPC_taken;
                        }
                    } else if (cond == 0) {        // cond == binary 0000 => branch never
                        // branch never?
                        if ( annulBit ) {
                            // unconditional, not taken, annulled
                            nextPC = &dynamic_inst_t::nextPC_untaken_a;
                        } else {
                            // unconditional, not taken, not annulled (NOP)
                            nextPC = &dynamic_inst_t::nextPC_execute;
                        }
                    } else {
                        ERROR_OUT("error: static_inst: setNextPC: unconditional branch with unknown cond code = 0x%0x\n", cond);
#ifdef LL_DECODER
						ERROR_OUT("LL_DECODER failed - ");	
						ll_decode_fail = true ;				
						decodeFails( __LINE__, m_inst );
#endif
                    }
                    break;
                case BRANCH_COND:
                    // set the static prediction bit for this branch
                    // CM FIX: check that this predicts forward branches taken, backwards not
                    if ( m_imm >= 0 ) {
                        setFlag( SI_STATICPRED, 0 );
                    } else {
                        setFlag( SI_STATICPRED, 1 );
                    }
                    nextPC = &dynamic_inst_t::nextPC_predict_branch;
                    break;
                case BRANCH_PCOND:
                    // set the static prediction bit for this branch
                    setFlag( SI_STATICPRED, maskBits32( m_inst, 19, 19 ) );
                    nextPC = &dynamic_inst_t::nextPC_predicated_branch;
                    break;

                case BRANCH_CALL:
                    // delayed call to PC relative address
                    nextPC = &dynamic_inst_t::nextPC_call;
                    break;
                case BRANCH_RETURN:
                    // indirect return, predicted using return addr stack (RAS)
                    nextPC = &dynamic_inst_t::nextPC_return;
                    break;
                case BRANCH_INDIRECT:
                    // indirect call
                    nextPC = &dynamic_inst_t::nextPC_indirect;
                    break;
                case BRANCH_CWP:
                    // current window pointer update
                    nextPC = &dynamic_inst_t::nextPC_cwp;
                    break;
                case BRANCH_TRAP_RETURN:
                    nextPC = &dynamic_inst_t::nextPC_trap_return;
                    break;
                case BRANCH_TRAP:
                    nextPC = &dynamic_inst_t::nextPC_trap;
                    break;
                case BRANCH_PRIV:
                    nextPC = &dynamic_inst_t::nextPC_priv;
                    break;
                default:
                    DEBUG_OUT("unknown branch type : %d\n", m_branch_type);
            }
            break;

        case DYN_LOAD:
        case DYN_STORE:
        case DYN_PREFETCH:
        case DYN_ATOMIC:
            // now that we're running production, we always want a nextPC function
        case DYN_NONE:
            nextPC = &dynamic_inst_t::nextPC_execute;
            break;

        default:
            DEBUG_OUT("error: instruction: %s == 0x%0x\n", 
                    decode_opcode(m_opcode), m_inst);
            DEBUG_OUT("error: unimplemented instruction type %d\n", (int) m_type);
    }

    /** Special Case: MOP --
     *     Do not keep fetching "in-order" ... cause an error sooner!
     *  CM FIX: should stall fetch until and mop squashed.
     */
    if (m_opcode == i_mop)
        nextPC = &dynamic_inst_t::nextPC_nop;

    ASSERT(nextPC);

}

//**************************************************************************
void static_inst_t::decodeFails( int line, uint32 inst )
{
	// FIXME - disabling prints for now, just for brevity
    DEBUG_OUT("### statici::decodeFails. line:%d. inst=0x%0x\n", line, inst);
    printBits( inst );
    initialize();
    m_type   = DYN_EXECUTE;
    m_opcode = i_mop;
    decode_fail = true ;
    for (int i=0; i < SI_MAX_SOURCE; i++) {
        m_source_reg[i].reset();
    }
    for (int i=0; i < SI_MAX_DEST; i++) {
        m_dest_reg[i].reset();
    }
}

//**************************************************************************
void static_inst_t::initialize( void )
{
    // set initial invalid opcode
    this->m_opcode = i_ill;
    this->m_type = DYN_NONE;
    this->m_branch_type = BRANCH_NONE;
    this->m_futype = FU_NONE;
    this->m_access_size = 0;
    this->m_ccshift = 0;
    this->m_flags = 0;
    this->m_imm   = (ireg_t) -1;
}

//**************************************************************************
static_stat_t *static_inst_t::getStats( void )
{
    if (m_stats == NULL) {
        m_stats = new static_stat_t();
    }
    return (m_stats);
}

//**************************************************************************
void  static_inst_t::printDetail(out_intf_t *io)
{
    io->out_info("static_inst_t:: 0x%0x\n", (int) this);
    io->out_info("         inst:: 0x%0x\n", m_inst );
    io->out_info("       opcode:: %s (%d)\n", decode_opcode(m_opcode), m_opcode);
    io->out_info("    immediate:: 0x%0llx\n", m_imm);
    io->out_info("    ref count:: %d\n", m_ref_count );
    io->out_info("    access   :: %d\n", m_access_size );
    io->out_info("    cc       :: %d\n", m_ccshift);
    io->out_info("    FU type  :: %s\n", 
            pstate_t::fu_type_menomic( getFuType() ) );
    io->out_info("    branch type:: %s\n", 
            pstate_t::branch_type_menomic[ getBranchType() ]);

    io->out_info("    access1   :: %d\n", m_access_size );
    io->out_info("    access2   :: %d\n", REG_SIGNED );
    io->out_info("    access3   :: %d\n", 
            m_access_size & REG_SIGNED );
    io->out_info("    access4   :: %d\n", 
            (m_access_size & REG_SIGNED) != 0 );
    io->out_info("    access5   :: %d\n", 
            m_access_size & REG_SIGNED != 0 );
    if (getAccessSigned() == true) {
        io->out_info("  SIGNED access\n");
    }
    io->out_info("    flags    ::\n");

#define SI_CONDITIONAL_PRINT( FLAG ) \
    if (getFlag( FLAG ))             \
    io->out_info( "     %s\n", #FLAG );

    SI_CONDITIONAL_PRINT( SI_BIT0         );
    SI_CONDITIONAL_PRINT( SI_ISIMMEDIATE  );
    SI_CONDITIONAL_PRINT( SI_ISASI        );
    SI_CONDITIONAL_PRINT( SI_STATICPRED   );
    SI_CONDITIONAL_PRINT( SI_UPDATE_CWP   );
    SI_CONDITIONAL_PRINT( SI_WRITE_CONTROL);
    SI_CONDITIONAL_PRINT( SI_FETCH_BARRIER);
    SI_CONDITIONAL_PRINT( SI_COMPLEX_OP   );
    SI_CONDITIONAL_PRINT( SI_TOFREE       );
}

//**************************************************************************
int32 static_inst_t::printNormalDis( char *str )
{
    int32 count;

    // print the decoded opcode
    count = sprintf( str, "%s ", decode_opcode( getOpcode() ) );

    // print the first source ... if it is valid
    reg_id_t rid = getSourceReg( 0 );
    if (rid.getRtype() != RID_NONE) {
        count += rid.printDisassemble( &str[count] );
        strcat( &str[count], ", " );
        count += 2;
    }

    // print the second source ... if it is valid
    if (getFlag(SI_ISIMMEDIATE)) {
        count += sprintf( &str[count], "%lld, ", getImmediate() );
    } else {
        rid = getSourceReg( 1 );
        if (rid.getRtype() != RID_NONE) {
            count += rid.printDisassemble( &str[count] );
            strcat( &str[count], ", " );
            count += 2;
        }
    }

    // print the destination
    rid = getDestReg( 0 );
    if (rid.getRtype() != RID_NONE) {
        count += rid.printDisassemble( &str[count] );
    }
    return count;
}

/// print out the disassembled instruction to the string str.
//**************************************************************************
int32 static_inst_t::printControlDis( char *str )
{
    int32 count;

    // print the decoded opcode
    count = sprintf( str, "%s ", decode_opcode( getOpcode() ) );

    // print the first source ... if it is valid
    reg_id_t rid = getSourceReg( 0 );
    if (rid.getRtype() != RID_NONE) {
        count += rid.printDisassemble( &str[count] );
        strcat( &str[count], ", " );
        count += 2;
    }

    // print the offset (if known)
    if ( getOffset() != (my_addr_t) -1 ) {
        count += sprintf( &str[count], "<0x%0llx>", getOffset() );
    }

    return count;
}

//**************************************************************************
int32 static_inst_t::printMemoryDis( char *str )
{
    int32 count;
    reg_id_t rid;

    // print the decoded opcode
    count = sprintf( str, "%s ", decode_opcode( getOpcode() ) );

    // print the store destination (if any)
    rid = getSourceReg( 2 );
    if (rid.getRtype() != RID_NONE) {
        count += rid.printDisassemble( &str[count] );
        count += sprintf( &str[count], ", " );
    }

    // print the address
    rid = getSourceReg( 0 );
    count += sprintf( &str[count], "[" );
    count += rid.printDisassemble( &str[count] );
    count += sprintf( &str[count], " + " );

    if ( getFlag(SI_ISIMMEDIATE) ) {
        count += sprintf( &str[count], "%lld]", getImmediate() );
    } else {
        rid = getSourceReg( 1 );
        count += rid.printDisassemble( &str[count] );
        count += sprintf( &str[count], "]" );
    }

    // print the load destination (if any)
    rid = getDestReg( 0 );
    if (rid.getRtype() != RID_NONE) {
        count += sprintf( &str[count], ", " );
        count += rid.printDisassemble( &str[count] );
    }
    return count;
}

//**************************************************************************
int static_inst_t::printDisassemble( char *str )
{
    switch (m_type) {
        case DYN_NONE:
        case DYN_EXECUTE:
            return (printNormalDis(str));
            break;
        case DYN_CONTROL:
            return (printControlDis(str));
            break;
        case DYN_LOAD:
        case DYN_STORE:
        case DYN_PREFETCH:
        case DYN_ATOMIC:
            return (printMemoryDis(str));
            break;
        default:
            return (printNormalDis(str));
    }
    return (printNormalDis(str));
}

/*------------------------------------------------------------------------*/
/* Actual PC Methods                                                        */
/*------------------------------------------------------------------------*/

//**************************************************************************
void   static_inst_t::actualPC_execute( abstract_pc_t *a )
{
    // advance the program counter (PC = nPC)
    a->pc  = a->npc;
    a->npc = a->npc + sizeof(uint32);
}

/** actualPC: given if a branch is taken, produce the PC, nPC result  */
//**************************************************************************
void   static_inst_t::actualPC_branch( bool isTaken,
        abstract_pc_t *a )
{
    if (isTaken) {
        // (cond, taken, annulled) and (cond, taken, not annulled) have
        // the same behavoir
        la_t targetPC = (int64) a->pc + (int64) getOffset();
        a->pc  = a->npc;
        a->npc = targetPC;
    } else {
        if (getAnnul()) {
            // cond, not taken, annulled
            a->pc  = a->npc + sizeof(uint32);
            a->npc = a->npc + (2*sizeof(uint32));
        } else {
            // cond, not taken, not annulled
            a->pc  = a->npc;
            a->npc = a->npc + sizeof(uint32);
        }
    }
}

/** given an unconditional branch, produce the PC, nPC result */
//**************************************************************************
void   static_inst_t::actualPC_uncond_branch( bool isTaken,
        abstract_pc_t *a )
{
    if (isTaken) {
        la_t targetPC = (int64) a->pc + (int64) getOffset();
        if (getAnnul()) {
            // (uncond, taken, annulled)
            a->pc  = targetPC;
            a->npc = a->pc + sizeof(uint32);
        } else {
            // (uncond, taken, not annulled)
            a->pc  = a->npc;
            a->npc = targetPC;
        }
    } else {
        if (getAnnul()) {
            // uncond, not taken, annulled
            a->pc  = a->npc + sizeof(uint32);
            a->npc = a->npc + (2*sizeof(uint32));
        } else {
            // uncond, not taken, not annulled
            a->pc  = a->npc;
            a->npc = a->npc + sizeof(uint32);
        }
    }
}

/** given the effective addr of a jmp, produce the PC, nPC result */
//**************************************************************************
void   static_inst_t::actualPC_indirect( my_addr_t  effective_addr,
        abstract_pc_t *a )
{
    a->pc  = a->npc;
    a->npc = effective_addr;
}

/** given the effective addr of a trap, produce the PC, nPC result */
//**************************************************************************
void   static_inst_t::actualPC_trap( bool       isTaken,
        my_addr_t  effective_addr,
        abstract_pc_t *a )
{
    if (isTaken) {
        a->pc  = effective_addr;
        a->npc = effective_addr + sizeof(uint32);
    } else {
        a->pc  = a->npc;
        a->npc = a->npc + sizeof(uint32);
    }
}

/// return from trap (done, retry) instructions
//**************************************************************************
void   static_inst_t::actualPC_trap_return( abstract_pc_t *a,
        my_addr_t tpc, my_addr_t tnpc,
        ireg_t pstate, ireg_t tl,
        ireg_t cwp )
{
    if (getOpcode() == i_done) {
        a->pc  = tnpc;
        a->npc = tnpc + sizeof(uint32);
    } else if (getOpcode() == i_retry) {
        a->pc  = tpc;
        a->npc = tnpc;
    } else {
		// LL_DECODER can a fault every bring us here? Maybe some code
		// is needed here
		ERROR_OUT("dx: done/retry: called for unknown opcode\n");
		SIM_HALT;
    }
    a->pstate = pstate;
    a->tl     = tl;
    a->cwp    = cwp;
    a->gset   = pstate_t::getGlobalSet( pstate );
}

//**************************************************************************
void   static_inst_t::actualPC_cwp( abstract_pc_t *a )
{
    // advance the program counter (PC = nPC)
    a->pc  = a->npc;
    a->npc = a->npc + sizeof(uint32);

    // Task: update the cwp for saves and restores
    if (getOpcode() == i_save) {
        a->cwp = (a->cwp + 1) & (NWINDOWS - 1);
    } else if (getOpcode() == i_restore) {
        a->cwp = (a->cwp - 1) & (NWINDOWS - 1);
    } else {
        SIM_HALT;
    }
}

/*------------------------------------------------------------------------*/
/* Instruction Decode                                                     */
/*------------------------------------------------------------------------*/

/** this function takes in a pointer to the instruction in memory and
  an empty structure and decodes the instruction to fill the structure */
//**************************************************************************

// this function is just a wrapper to help with fault injection
    void
static_inst_t::decodeInstructionInfo( pa_t physical_address,
        unsigned int inst, int32 fetch_id, int core_id, fault_stats *fault_stat )
{
    m_fetch_id = fetch_id;
    // Inject fault only in the chosen decoder
	//std::cout<<"core_id before: "<<core_id;

	// LXL: High level injection code. Still work if uncommented because
	// fault-bit should be -1, but why waste the cpu cycles?
#if !defined(LL_DECODER) 
    if( (fault_stat->getFaultType() == DECODER_INPUT_FAULT) &&
		(fetch_id == fault_stat->getFaultyReg() )) {    	//siva
		unsigned int old_inst=inst;
		// FIXME - SHould set FAULT_CURR_INST() here
		inst = fault_stat->injectFault(inst);
		correct_inst = old_inst;

		if( inst != old_inst ) {
			//std::cout<<" core_id inside: "<<core_id;
			setFault() ;
		}
		
		if (ALEX_SANITY_DEBUG) {
			if (inst!=old_inst) {
				DEBUG_OUT( "Inject@PC:%x\t", physical_address ) ;
				DEBUG_OUT( "Old:%x\t", old_inst ) ;
				DEBUG_OUT( "New:%x\n", inst ) ;
			}
		}
	}
#endif
    
    decodeInstructionInfo( physical_address, inst ) ;
#ifdef LL_DECODER
    if( (fault_stat->getFaultType() == DECODER_INPUT_FAULT) &&
			(fault_stat->getRetInst() > fault_stat->getFaultInjInst()) &&
			(fetch_id == fault_stat->getFaultyReg()) ) {

		// Stimuli to the verilog module
		char s_inst[32] ;
		fSim.lluToBinStr(s_inst, inst, 33) ;
		if(fSim.counter ==0) {
			char stimFIFO[NAME_SIZE];
			char responseFIFO[NAME_SIZE];
			string stim = fault_stat->getStimPipe();
			string response = fault_stat->getResponsePipe();

			strcpy(stimFIFO,stim.c_str());
			strcpy(responseFIFO,response.c_str());

			strcpy(fSim.writeFIFO,stimFIFO);
			strcpy(fSim.readFIFO,responseFIFO);

			fSim.fdWrite = open(fSim.writeFIFO, O_WRONLY|O_ASYNC);
			fSim.fdRead = open(fSim.readFIFO, O_RDWR|O_ASYNC);
		}
		fSim.counter++ ;
		fSim.nwrite = write(fSim.fdWrite, &s_inst, 33) ;
		
		// Responses from the verilog module 
		char r_rd[7];
		char r_rd_type[5];
		int r_rd_valid;
		char r_rd2[7];
		char r_rd2_type[5];
		int r_rd2_valid ;
		char r_rs1[7];
		char r_rs1_type[5];
		int r_rs1_valid ;
		char r_rs2[7];
		char r_rs2_type[5];
		int r_rs2_valid;
		char r_rs3[7];
		char r_rs3_type[5];
		int r_rs3_valid;
		char r_rs4[7];
		char r_rs4_type[5];
		int r_rs4_valid;
		char r_m_type[4];
		char r_m_futype[5];
		char r_m_opcode[10];
		char r_m_branch_type[5];
		char r_m_imm[65];
		char r_m_ccshift[9];
		char r_m_access_size[9];
		char r_m_flags[9];
		int r_fail;

		// Read responses from verilog module
		fSim.nread = read(fSim.fdRead, &r_rd, 7) ;
		fSim.nread = read(fSim.fdRead, &r_rd_type, 5) ;
		fSim.nread = read(fSim.fdRead, &r_rd_valid, sizeof(int)) ;

		fSim.nread = read(fSim.fdRead, &r_rd2, 7) ;
		fSim.nread = read(fSim.fdRead, &r_rd2_type, 5) ;
		fSim.nread = read(fSim.fdRead, &r_rd2_valid, sizeof(int)) ;

		fSim.nread = read(fSim.fdRead, &r_rs1, 7) ;
		fSim.nread = read(fSim.fdRead, &r_rs1_type, 5) ;
		fSim.nread = read(fSim.fdRead, &r_rs1_valid, sizeof(int)) ;

		fSim.nread = read(fSim.fdRead, &r_rs2, 7) ;
		fSim.nread = read(fSim.fdRead, &r_rs2_type, 5) ;
		fSim.nread = read(fSim.fdRead, &r_rs2_valid, sizeof(int)) ;

		fSim.nread = read(fSim.fdRead, &r_rs3, 7) ;
		fSim.nread = read(fSim.fdRead, &r_rs3_type, 5) ;
		fSim.nread = read(fSim.fdRead, &r_rs3_valid, sizeof(int)) ;

		fSim.nread = read(fSim.fdRead, &r_rs4, 7) ;
		fSim.nread = read(fSim.fdRead, &r_rs4_type, 5) ;
		fSim.nread = read(fSim.fdRead, &r_rs4_valid, sizeof(int)) ;

		fSim.nread = read(fSim.fdRead, &r_m_type, 4) ;
	  	fSim.nread = read(fSim.fdRead, &r_m_futype, 5) ;
	  	fSim.nread = read(fSim.fdRead, &r_m_opcode, 10) ;
	  	fSim.nread = read(fSim.fdRead, &r_m_branch_type, 5) ;
	  	fSim.nread = read(fSim.fdRead, &r_m_imm, 65) ;
	  	fSim.nread = read(fSim.fdRead, &r_m_ccshift, 9) ;
		fSim.nread = read(fSim.fdRead, &r_m_access_size, 9) ;
		fSim.nread = read(fSim.fdRead, &r_m_flags, 9) ;
		fSim.nread = read(fSim.fdRead, &r_fail, sizeof(int)) ;
		
		// DEBUG_OUT("Valid array = %d, %d, %d, %d, %d, %d\n",
		// 		r_rd_valid,
		// 		r_rd2_valid,
		// 		r_rs1_valid,
		// 		r_rs2_valid,
		// 		r_rs3_valid,
		// 		r_rs3_valid
		// 		) ;
		// DEBUG_OUT("Fail = %d\n", r_fail) ;
		// DEBUG_OUT("OPAL: m_type = %s(%d)\n", r_m_type, sizeof(r_m_type)) ;
		// DEBUG_OUT("OPAL: m_futype = %s(%d)\n", r_m_futype, sizeof(r_m_futype)) ;
		// DEBUG_OUT("OPAL: m_opcode = %s(%d)\n", r_m_opcode, sizeof(r_m_opcode)) ;
		// DEBUG_OUT("OPAL: m_branch_type = %s(%d)\n", r_m_branch_type, sizeof(r_m_branch_type)) ;
		// Now, do the conversion
		char *ptr ; // Needed by strtoull

		uint64 v_rd = (byte_t) strtoull(r_rd, &ptr, 2 ) ;
		uint64 v_rd_type = strtoull(r_rd_type, &ptr, 2 ) ;
		bool v_rd_valid = r_rd_valid ;

		uint64 v_rd2 = (byte_t) strtoull(r_rd2, &ptr, 2 ) ;
		uint64 v_rd2_type = strtoull(r_rd2_type, &ptr, 2 ) ;
		bool v_rd2_valid = r_rd2_valid ;

		uint64 v_rs1 = (byte_t) strtoull(r_rs1, &ptr, 2 ) ;
		uint64 v_rs1_type = strtoull(r_rs1_type, &ptr, 2 ) ;
		bool v_rs1_valid = r_rs1_valid ;

		uint64 v_rs2 = (byte_t) strtoull(r_rs2, &ptr, 2 ) ;
		uint64 v_rs2_type = strtoull(r_rs2_type, &ptr, 2 ) ;
		bool v_rs2_valid = r_rs2_valid ;

		uint64 v_rs3 = (byte_t) strtoull(r_rs3, &ptr, 2 ) ;
		uint64 v_rs3_type = strtoull(r_rs3_type, &ptr, 2 ) ;
		bool v_rs3_valid = r_rs3_valid ;

		uint64 v_rs4 = (byte_t) strtoull(r_rs4, &ptr, 2 ) ;
		uint64 v_rs4_type = strtoull(r_rs4_type, &ptr, 2 ) ;
		bool v_rs4_valid = r_rs4_valid ;

		uint64 v_m_type = strtoull(r_m_type, &ptr, 2 ) ;
	  	uint64 v_m_futype = strtoull(r_m_futype, &ptr, 2 ) ;
	  	uint64 v_m_opcode = strtoull(r_m_opcode, &ptr, 2 ) ;
	  	uint64 v_m_branch_type = strtoull(r_m_branch_type, &ptr, 2 ) ;
	  	uint64 v_m_imm = strtoull(r_m_imm, &ptr, 2 ) ;
	  	uint64 v_m_ccshift = strtoull(r_m_ccshift, &ptr, 2 ) ;
		uint64 v_m_access_size = strtoull(r_m_access_size, &ptr, 2 ) ;
		uint64 v_m_flags = strtoull(r_m_flags, &ptr, 2 ) ;
		bool v_fail = r_fail ;

		// DEBUG_OUT("instruction   %x(%s)\n", inst,decode_opcode(m_opcode));
		// if(DEBUG_LL_DECODER) {
		// 	DEBUG_OUT("\n#### Outputs from LL_DECODER #### \n") ;
		// 	DEBUG_OUT("instruction   %x(%s)\n", inst,decode_opcode(m_opcode));
		// 	DEBUG_OUT("rd            %llx\t%llx\t%x\n", v_rd, v_rd_type, v_rd_valid);
		// 	DEBUG_OUT("rd2           %llx\t%llx\t%x\n", v_rd2, v_rd2_type, v_rd2_valid);
		// 	DEBUG_OUT("rs1           %llx\t%llx\t%x\n", v_rs1, v_rs1_type, v_rs1_valid);
		// 	DEBUG_OUT("rs2           %llx\t%llx\t%x\n", v_rs2, v_rs2_type, v_rs2_valid);
		// 	DEBUG_OUT("rs3           %llx\t%llx\t%x\n", v_rs3, v_rs3_type, v_rs3_valid);
		// 	DEBUG_OUT("rs4           %llx\t%llx\t%x\n", v_rs4, v_rs4_type, v_rs4_valid);
		// 	DEBUG_OUT("m_type        %llx\n", v_m_type); //
		// 	DEBUG_OUT("m_futype      %llx\n", v_m_futype);
		// 	DEBUG_OUT("m_opcode      %llx\n", v_m_opcode);
		// 	DEBUG_OUT("m_branch_type %llx\n", v_m_branch_type);
		// 	DEBUG_OUT("m_imm         %llx\n", v_m_imm);
		// 	DEBUG_OUT("m_ccshift     %llx\n", v_m_ccshift);
		// 	DEBUG_OUT("m_access_size %llx\n", v_m_access_size);
		// 	DEBUG_OUT("m_flags       %llx\n", v_m_flags);
		// 	DEBUG_OUT("fail          %x\n", v_fail);
		// }
		
		if(0&&v_fail) {
			DEBUG_OUT("vfail\n");
			//m_type = account_flips("m_type",true,v_m_type,m_type,fault_stat,&total_num_flips) ;
			LL_FAIL() ;
		} else {
			// copy values into the statici fields if they are different
			// rd
			int total_num_flips = 0;
			
			I_RD1.setVanilla(account_flips("rd",v_rd_valid,v_rd,I_RD1.getVanilla(),fault_stat,&total_num_flips));
			I_RD1.setRtype((rid_type_t)account_flips("rd_type",v_rd_valid,v_rd_type,I_RD1.getRtype(),fault_stat,&total_num_flips));
		
			I_RD2.setVanilla(account_flips("rd2",v_rd2_valid,v_rd2,(uint64)(I_RD2.getVanilla()),fault_stat,&total_num_flips));
			I_RD2.setRtype((rid_type_t)account_flips("rd2_type",v_rd2_valid,v_rd2_type,I_RD2.getRtype(),fault_stat,&total_num_flips));

			I_RS1.setVanilla(account_flips("rs1",v_rs1_valid,v_rs1,I_RS1.getVanilla(),fault_stat,&total_num_flips));
			I_RS1.setRtype((rid_type_t)account_flips("rs1_type",v_rs1_valid,v_rs1_type,I_RS1.getRtype(),fault_stat,&total_num_flips));

			I_RS2.setVanilla(account_flips("rs2",v_rs2_valid,v_rs2,I_RS2.getVanilla(),fault_stat,&total_num_flips));
			I_RS2.setRtype((rid_type_t)account_flips("rs2_type",v_rs2_valid,v_rs2_type,I_RS2.getRtype(),fault_stat,&total_num_flips));

			I_RS3.setVanilla(account_flips("rs3",v_rs3_valid,v_rs3,I_RS3.getVanilla(),fault_stat,&total_num_flips));
			I_RS3.setRtype((rid_type_t)account_flips("rs3_type",v_rs3_valid,v_rs3_type,I_RS3.getRtype(),fault_stat,&total_num_flips));

			I_RS4.setVanilla(account_flips("rs4",v_rs4_valid,v_rs4,I_RS4.getVanilla(),fault_stat,&total_num_flips));
			I_RS4.setRtype((rid_type_t)account_flips("rs4_type",v_rs4_valid,v_rs4_type,I_RS4.getRtype(),fault_stat,&total_num_flips));

			dyn_execute_type_t exec_type = system_t::inst->lookupOpExecTab(v_m_opcode);
			if (v_m_opcode==i_wrpr) {
				int x = v_rd2;
				if (x==CONTROL_PSTATE ||
					x == CONTROL_TL ||
					x == CONTROL_CWP ) {
					exec_type = DYN_CONTROL;
				} else {
					exec_type = DYN_EXECUTE;
				}
				//DEBUG_OUT("x is %d m_type is %d\n",x,v_m_type);

			}

			if ( exec_type != v_m_type) {
				v_m_type = exec_type;
				//DEBUG_OUT("exec_type now %d\n",exec_type);
			}

			m_type = account_flips("m_type",true,v_m_type,m_type,fault_stat,&total_num_flips) ;
			m_futype = account_flips("m_futype",true,v_m_futype,m_futype,fault_stat,&total_num_flips) ;
			m_opcode = account_flips("m_opcode",true,v_m_opcode,m_opcode,fault_stat,&total_num_flips) ;
			m_branch_type = account_flips("m_branch_type",true,v_m_branch_type,m_branch_type,fault_stat,&total_num_flips) ;
			m_imm = account_flips("m_imm",true,v_m_imm,m_imm,fault_stat,&total_num_flips) ;
			m_ccshift = account_flips("m_ccshift",true,v_m_ccshift,m_ccshift,fault_stat,&total_num_flips) ;
			m_access_size = account_flips("m_access_size",true,v_m_access_size,m_access_size,fault_stat,&total_num_flips) ;
			m_flags = account_flips("m_flags",true,v_m_flags,m_flags,fault_stat,&total_num_flips) ;

			account_flips("v_fail",true,v_fail,decode_fail,fault_stat,&total_num_flips) ;

			if (total_num_flips!=0) {
				uint64 fake_pattern=0xffffffffffffffffULL;
				int fake_num_flips = (total_num_flips>64)?64:total_num_flips;
				fake_pattern = fake_pattern<<(64-fake_num_flips);

				fSim.numBitFlips(0,fake_pattern) ;
				fault_stat->incTotalInj() ;
				fault_stat->injectSafFault() ;
				setFault() ;

			} else {

				fault_stat->incTotalMask() ;

			}

			if (!validAccessSize(m_access_size) || !validRtype() || !validTypes() || v_fail) {
// 				DEBUG_OUT("invalid %d %d %d, flips %d\n",validAccessSize(m_access_size),
// 						  validRtype(),validTypes(), total_num_flips);

				LL_FAIL();
			}
		}
	}
	// After propagating values, then set next PC. Even for
	// non-faulty units, this is where NPC is set of LL_DECODER
	// flags is on.
#endif // LL_DECODER
    setNextPC();
}

void 
static_inst_t::setOriginalInstr(static_inst_t*orig) {

	original = orig ;

	int total_num_flips = 0;
			
	account_flips("rd",true,orig->getDestReg(0).getVanilla(),I_RD1.getVanilla(),NULL,&total_num_flips);
	account_flips("rd_type",true,orig->getDestReg(0).getRtype(),I_RD1.getRtype(),NULL,&total_num_flips);
		
	account_flips("rd2",true,orig->getDestReg(1).getVanilla(),(uint64)(I_RD2.getVanilla()),NULL,&total_num_flips);
	account_flips("rd2_type",true,orig->getDestReg(1).getRtype(),I_RD2.getRtype(),NULL,&total_num_flips);

	account_flips("rs1",true,orig->getSourceReg(0).getVanilla(),I_RS1.getVanilla(),NULL,&total_num_flips);
	account_flips("rs1_type",true,orig->getSourceReg(0).getRtype(),I_RS1.getRtype(),NULL,&total_num_flips);

	account_flips("rs2",true,orig->getSourceReg(1).getVanilla(),I_RS2.getVanilla(),NULL,&total_num_flips);
	account_flips("rs2_type",true,orig->getSourceReg(1).getRtype(),I_RS2.getRtype(),NULL,&total_num_flips);

	account_flips("rs3",true,orig->getSourceReg(2).getVanilla(),I_RS3.getVanilla(),NULL,&total_num_flips);
	account_flips("rs3_type",true,orig->getSourceReg(2).getRtype(),I_RS3.getRtype(),NULL,&total_num_flips);

	account_flips("rs4",true,orig->getSourceReg(3).getVanilla(),I_RS4.getVanilla(),NULL,&total_num_flips);
	account_flips("rs4_type",true,orig->getSourceReg(3).getRtype(),I_RS4.getRtype(),NULL,&total_num_flips);

	account_flips("m_type",true,orig->getType(),m_type,NULL,&total_num_flips) ;
	account_flips("m_futype",true,orig->getFuType(),m_futype,NULL,&total_num_flips) ;
	account_flips("m_opcode",true,orig->getOpcode(),m_opcode,NULL,&total_num_flips) ;
	account_flips("m_branch_type",true,orig->getBranchType(),m_branch_type,NULL,&total_num_flips) ;
	account_flips("m_imm",true,orig->getOffset(),m_imm,NULL,&total_num_flips) ;
	account_flips("m_ccshift",true,orig->getCCShift(),m_ccshift,NULL,&total_num_flips) ;
	account_flips("m_access_size",true,orig->getRawAccessSize(),m_access_size,NULL,&total_num_flips) ;
	account_flips("m_flags",true,orig->getFlags(),m_flags,NULL,&total_num_flips) ;

	if (total_num_flips!=0) {
		uint64 fake_pattern=0xffffffffffffffffULL;
		int fake_num_flips = (total_num_flips>64)?64:total_num_flips;
		fake_pattern = fake_pattern<<(64-fake_num_flips);

		fSim.numBitFlips(0,fake_pattern) ;

	} else {


	}
}

// Function to help account flips stats when doing LL_DECODER
#if defined(LL_DECODER) || defined(LXL_FIND_BITFLIP)
ireg_t
static_inst_t::account_flips(string s, bool vlog_valid, uint64 vlog, uint64 opal, fault_stats *fs, int* num_flips) {
	if(!vlog_valid) {
		return opal ;
	} 
	if(vlog!=opal) {
		if(DEBUG_LL_DECODER) {
			DEBUG_OUT("LL_DECODER Mismatch - op:%s, %s: Opal = 0x%llx, ncsim=0x%llx\n",
					decode_opcode(m_opcode), s.c_str(), opal, vlog) ;
		}
		ll_decode_fail = true ;

		// The following will just prompt to duplicate counting
		// 		fSim.numBitFlips(opal,vlog) ;
		// 		fs->incTotalInj() ;
		// 		fs->injectSafFault() ;
		// 		setFault() ;

		uint64 result = vlog^opal;
		int count=0;

		for(int i=0; i<64; i++){	
			count += result&(0x1ULL);
			result = result >> 1;
		}

		*num_flips = *num_flips+count;
		return vlog ;
	} else {
		//fs->incTotalMask() ;
		return opal ;
	}
}

bool
static_inst_t::validAccessSize(byte_t access_size) {

	if (m_type < DYN_LOAD) 
		return true;

	byte_t masked_size = access_size & REG_MEMSIZE;
	int count=0;

	for (int i=0; i<8; i++) {
		count += ((masked_size&(1<<i))==0)?0:1;
	}
	if (count!=1) {
		return false;
	}
	return true;

}

inline bool
static_inst_t::validRtype() {
	return ((I_RS1.getRtype() < RID_NUM_RID_TYPES) &&
			(I_RS2.getRtype() < RID_NUM_RID_TYPES) &&
			(I_RS3.getRtype() < RID_NUM_RID_TYPES) &&
			(I_RS4.getRtype() < RID_NUM_RID_TYPES) &&
			(I_RD1.getRtype() < RID_NUM_RID_TYPES) &&
			(I_RD2.getRtype() < RID_NUM_RID_TYPES));
}

inline bool
static_inst_t::validTypes() {
	bool correct_types = ((m_type <  DYN_NUM_EXECUTE_TYPES) &&
						  (m_futype <    FU_NUM_FU_TYPES) &&
						  (m_branch_type <   BRANCH_NUM_BRANCH_TYPES) &&
						  (m_opcode < i_maxcount) &&
						  ((m_type != DYN_CONTROL) || ((m_type == DYN_CONTROL) && m_branch_type != BRANCH_NONE)));
	return correct_types;

}
#endif //LL_DECODER


// The real function itself
    void
static_inst_t::decodeInstructionInfo( pa_t physical_address,
        unsigned int inst )
{

	// 1000 1101 1011 0000:1000 0110 0011 0111
    int op = maskBits32( inst, 30, 31 );
    int op2 = maskBits32( inst, 22, 24 );
    int op3 = maskBits32( inst, 19, 24 );
    int rd = maskBits32( inst, 25, 29 );
    int rs1 = maskBits32( inst, 14, 18 );
    int rs2 = maskBits32( inst, 0, 4 );
    int opf_hi = maskBits32( inst, 9, 13 );
    int opf_lo = maskBits32( inst, 5, 8 );
    int cond = maskBits32( inst, 25, 28 );
    int mcond = maskBits32( inst, 14, 17 );
    int rcond = maskBits32( inst, 10, 12 );
    int i = maskBits32( inst, 13, 13 );
    int asi = maskBits32( inst, 12, 5 );
    int x, movcc2, sdq;

    this->m_inst = inst;
    // set initial invalid opcode
    initialize();

    /** warning: the following code uses "goto" statement to handle error
     *           case, which jump out of multiple "switch" statement after
     *           encountered a decoding error.
     */
    switch (op) {
        case 0:
            /// Op 0 instructions
            switch (op2) {
                case 0: // _trap
                    // CM imprecise: occasionally, while speculating, we decode 0 instruction
                    m_type   = DYN_EXECUTE;
                    m_futype = FU_NONE;
                    m_opcode = i_nop;
                    // CM KNOWNBUG
#if 0
                    DEBUG_OUT("statici::decode(): trap instruction PC 0x%0llx Instr 0x%0x\n",
                            physical_address, inst);
#endif
                    break;

                case 1: // _bpcc
                    m_type = DYN_CONTROL;
                    m_futype = FU_BRANCH;
                    m_branch_type = BRANCH_PCOND;
                    // disp19
                    m_imm = (4 * sign_ext64( maskBits32( inst, 0, 18 ), 18 ));
                    SI_READ_ICC;
                    m_ccshift = maskBits32( inst, 21, 20 ) << 1;
                    if (m_ccshift != 0 && m_ccshift != 4) {
                        FAIL();
                    }
                    // read a control register to verify prediction
                    SI_READ_CONTROL( CONTROL_ISREADY );

                    switch (cond) {
                        case 0:   // bpn
                            m_branch_type = BRANCH_UNCOND;
                            m_opcode = i_bpn;
                            break;
                        case 1:   // bpe
                            m_opcode = i_bpe;
                            break;
                        case 2:  // bple
                            m_opcode = i_bple;
                            break;
                        case 3:  // bpl
                            m_opcode = i_bpl;
                            break;
                        case 4:  // bpleu
                            m_opcode = i_bpleu;
                            break;
                        case 5:  // bpcs
                            m_opcode = i_bpcs;
                            break;
                        case 6:  // bpneg
                            m_opcode = i_bpneg;
                            break;
                        case 7:  // bpvs
                            m_opcode = i_bpvs;
                            break;
                        case 8:  // bpa
                            m_branch_type = BRANCH_UNCOND;
                            m_opcode = i_bpa;
                            break;
                        case 9:  // bpne
                            m_opcode = i_bpne;
                            break;
                        case 10:  // bpg
                            m_opcode = i_bpg;
                            break;
                        case 11:  // bpge
                            m_opcode = i_bpge;
                            break;
                        case 12:  // bpgu
                            m_opcode = i_bpgu;
                            break;
                        case 13:  // bpcc
                            m_opcode = i_bpcc;
                            break;
                        case 14:  // bppos
                            m_opcode = i_bppos;
                            break;
                        case 15:  // bpvc
                            m_opcode = i_bpvc;
                            break;
                        default:
                            FAIL();
                    }
                    break;

                case 2:  // bicc
                    m_type = DYN_CONTROL;
                    m_futype = FU_BRANCH;
                    m_branch_type = BRANCH_COND;
                    // disp22
                    m_imm = (4 * sign_ext64( maskBits32( inst, 0, 21 ), 21));
                    // bicc always acts on icc (icc == 0, xcc == 4)
                    m_ccshift = 0;
                    SI_READ_ICC;
                    // read a control register to verify prediction
                    SI_READ_CONTROL( CONTROL_ISREADY );

                    switch (cond) {
                        case 0:  // bn
                            m_branch_type = BRANCH_UNCOND;
                            m_opcode = i_bn;
                            break;
                        case 1:  // be
                            m_opcode = i_be;
                            break;
                        case 2:  // ble
                            m_opcode = i_ble;
                            break;
                        case 3:  // bl
                            m_opcode = i_bl;
                            break;
                        case 4:  // bleu
                            m_opcode = i_bleu;
                            break;
                        case 5:  // bcs
                            m_opcode = i_bcs;
                            break;
                        case 6:  // bneg
                            m_opcode = i_bneg;
                            break;
                        case 7:  // bvs
                            m_opcode = i_bvs;
                            break;
                        case 8:  // ba
                            m_branch_type = BRANCH_UNCOND;
                            m_opcode = i_ba;
                            break;
                        case 9:  // bne
                            m_opcode = i_bne;
                            break;
                        case 10:  // bg
                            m_opcode = i_bg;
                            break;
                        case 11:  // bge
                            m_opcode = i_bge;
                            break;
                        case 12:  // bgu
                            m_opcode = i_bgu;
                            break;
                        case 13:  // bcc
                            m_opcode = i_bcc;
                            break;
                        case 14:  // bpos
                            m_opcode = i_bpos;
                            break;
                        case 15:  // bvc
                            m_opcode = i_bvc;
                            break;
                        default:
                            FAIL();
                    }
                    break;    

                case 3:  // bpr
                    m_type = DYN_CONTROL;
                    m_futype = FU_BRANCH;
                    m_branch_type = BRANCH_PCOND;
                    SI_INT_RID( rs1, I_RS1 );
                    x = maskBits32( inst, 21, 20 ) << 14 | maskBits32( inst, 13, 0 );
                    m_imm = (4 * sign_ext64( x, 15 ) );
                    // read a control register to verify prediction
                    SI_READ_CONTROL( CONTROL_ISREADY );

                    switch (cond) {
                        case 0:  // reserved
                            break;
                        case 1:  // brz
                            m_opcode = i_brz;
                            break;
                        case 2:  // brlez
                            m_opcode = i_brlez;
                            break;
                        case 3:  // brlz
                            m_opcode = i_brlz;
                            break;
                        case 4:  // reserved
                            break;
                        case 5:  // brnz
                            m_opcode = i_brnz;
                            break;
                        case 6:  // brgz
                            m_opcode = i_brgz;
                            break;
                        case 7:  // brgez
                            m_opcode = i_brgez;
                            break;
                        default:
                            FAIL();
                    }
                    break;

                case 4:  // sethi
                    m_type   = DYN_EXECUTE;
                    m_futype = FU_NONE;

                    // x is the immediate
                    x = maskBits32( inst, 0, 21 );
                    if ( rd == 0 && x == 0 ) {
                        m_opcode = i_nop;
                    } else if ( rd == 0 && x != 0 ) {
                        // technically a magic breakpoint
                        m_opcode = i_sethi;
						if ((x & 0x3f0000) == 0x40000) {
							// 0x40000 + 100 + INV_ID
							m_exit_code = x & 0xffff;
							/* //subtract later, as 0 exitcode is used as a special case
							   if (m_exit_code > 100) {
							   m_exit_code -= 100;
							   }*/
						}
                    } else {
                        SI_INT_RID( rd, I_RD1 );
                        m_opcode = i_sethi;
                        m_imm    = maskBits32( inst, 21, 0 ) << 10;
                        setFlag(SI_ISIMMEDIATE, true );
                    }
                    break;

                case 5:  // fbpfcc
                    m_type = DYN_CONTROL;
                    m_futype = FU_BRANCH;
                    m_branch_type = BRANCH_PCOND;
                    // disp19 
                    m_imm = (4 * sign_ext64( maskBits32( inst, 0, 18 ), 18 ));
                    // read a control register to verify prediction
                    SI_READ_CONTROL( CONTROL_ISREADY );

                    // set up the third source -- the condition code register
                    movcc2 = maskBits32( inst, 21, 20 );
                    switch (movcc2) {
                        case 0:
                            SI_CC_RID( REG_CC_FCC0, I_RS3 );
                            break;
                        case 1:
                            SI_CC_RID( REG_CC_FCC1, I_RS3 );
                            break;
                        case 2:
                            SI_CC_RID( REG_CC_FCC2, I_RS3 );
                            break;
                        case 3:
                            SI_CC_RID( REG_CC_FCC3, I_RS3 );
                            break;
                        default:
                            FAIL();
                    }

                    switch (cond) {
                        case 0:  // fbpn
                            m_opcode = i_fbpn;
                            break;
                        case 1:  // fbpne
                            m_opcode = i_fbpne;
                            break;
                        case 2:  // fbplg
                            m_opcode = i_fbplg;
                            break;
                        case 3:  // fbpul
                            m_opcode = i_fbpul;
                            break;
                        case 4:  // fbpl
                            m_opcode = i_fbpl;
                            break;
                        case 5:  // fbpug
                            m_opcode = i_fbpug;
                            break;
                        case 6:  // fbpg
                            m_opcode = i_fbpg;
                            break;
                        case 7:  // fbpu
                            m_opcode = i_fbpu;
                            break;
                        case 8:  // fbpa
                            m_opcode = i_fbpa;
                            break;
                        case 9:  // fbpe
                            m_opcode = i_fbpe;
                            break;
                        case 10:  // fbpue
                            m_opcode = i_fbpue;
                            break;
                        case 11:  // fbpge
                            m_opcode = i_fbpge;
                            break;
                        case 12:  // fbpuge
                            m_opcode = i_fbpuge;
                            break;
                        case 13:  // fbple
                            m_opcode = i_fbple;
                            break;
                        case 14:  // fbpule
                            m_opcode = i_fbpule;
                            break;
                        case 15:  // fbpo
                            m_opcode = i_fbpo;
                            break;
                        default:
                            FAIL();
                    }
                    break;

                case 6:  // fbfcc 
                    m_type   = DYN_CONTROL;
                    m_branch_type = BRANCH_COND;
                    // disp22
                    m_imm = (4 * sign_ext64( maskBits32( inst, 0, 21 ), 21));
                    SI_CC_RID( REG_CC_FCC0, I_RS3 );
                    // read a control register to verify prediction
                    SI_READ_CONTROL( CONTROL_ISREADY );

                    switch (cond) {
                        case 0:  // fbn
                            m_branch_type = BRANCH_UNCOND;
                            m_opcode = i_fbn;
                            break;
                        case 1:  // fbne
                            m_opcode = i_fbne;
                            break;
                        case 2:  // fblg
                            m_opcode = i_fblg;
                            break;
                        case 3:  // fbul
                            m_opcode = i_fbul;
                            break;
                        case 4:  // fbl
                            m_opcode = i_fbl;
                            break;
                        case 5:  // fbug
                            m_opcode = i_fbug;
                            break;
                        case 6:  // fbg
                            m_opcode = i_fbg;
                            break;
                        case 7:  // fbu
                            m_opcode = i_fbu;
                            break;
                        case 8:  // fba
                            m_branch_type = BRANCH_UNCOND;
                            m_opcode = i_fba;
                            break;
                        case 9:  // fbe
                            m_opcode = i_fbe;
                            break;
                        case 10:  // fbue
                            m_opcode = i_fbue;
                            break;
                        case 11:  // fbge
                            m_opcode = i_fbge;
                            break;
                        case 12:  // fbuge
                            m_opcode = i_fbuge;
                            break;
                        case 13:  // fble
                            m_opcode = i_fble;
                            break;
                        case 14:  // fbule
                            m_opcode = i_fbule;
                            break;
                        case 15:  // fbo
                            m_opcode = i_fbo;
                            break;
                        default:
                            FAIL();
                    }
                    break;

                case 7: // _
                    FAIL();
                    break;

                default:
                    FAIL();
            }
            break; /// ** break out of Op 0 instructions

        case 1:
            /// Op 1 instructions
            m_type = DYN_CONTROL;
            m_futype = FU_BRANCH;
            m_branch_type = BRANCH_CALL;
            m_opcode = i_call;

            m_imm = (4 * sign_ext64( maskBits32( inst, 0, 29 ), 29));
            // call writes the value of the PC to r[15] (out reg. #7)
            I_RD1.setRtype( RID_INT );
            I_RD1.setVanilla( 15 );
            break; /// ** break out of Op 1 instructions

        case 2:
            /// Op 2 instructions

            // Note: all op 2 instructions are presumed to be "executable"
            //       unless otherwise tagged. (dyn_execute_type_t)
            m_type = DYN_EXECUTE;
            switch (op3) {
                case 0:  // add
                    REG_TWO_ALU( FU_INTALU );
                    m_opcode = i_add;
                    break;
                case 1:  // and
                    REG_TWO_ALU( FU_INTALU );
                    m_opcode = i_and;
                    break;
                case 2:  // or
                    REG_TWO_ALU( FU_INTALU );
                    if ( rs1 == 0 ) {
                        m_opcode = i_mov;
                    } else {
                        m_opcode = i_or;
                    }
                    break;
                case 3:  // xor
                    REG_TWO_ALU( FU_INTALU );
                    m_opcode = i_xor;
                    break;
                case 4:  // sub
                    REG_TWO_ALU( FU_INTALU );
                    m_opcode = i_sub;
                    break;
                case 5:  // andn
                    REG_TWO_ALU( FU_INTALU );
                    m_opcode = i_andn;
                    break;
                case 6:  // orn
                    REG_TWO_ALU( FU_INTALU );
                    m_opcode = i_orn;
                    break;
                case 7:  // xnor
                    REG_TWO_ALU( FU_INTALU );
                    if ( i == 0 && rs2 == 0 ) {
                        m_opcode = i_not;
                    } else {
                        m_opcode = i_xnor;
                    }
                    break;
                case 8:  // addc
                    REG_TWO_ALU( FU_INTALU );
                    SI_READ_ICC;
                    m_opcode = i_addc;
                    break;
                case 9:  // mulx
                    REG_TWO_ALU( FU_INTMULT );
                    m_opcode = i_mulx;
                    break;
                case 10:  // umul
                    REG_TWO_ALU( FU_INTMULT );
                    SI_WRITE_Y;
                    m_opcode = i_umul;
                    break;
                case 11:  // smul
                    REG_TWO_ALU( FU_INTMULT );
                    SI_WRITE_Y;
                    m_opcode = i_smul;
                    break;
                case 12:  // subc
                    REG_TWO_ALU( FU_INTALU );
                    SI_READ_ICC;
                    m_opcode = i_subc;
                    break;
                case 13:  // udivx
                    REG_TWO_ALU( FU_INTDIV );
                    m_opcode = i_udivx;
                    break;
                case 14:  // udiv
                    REG_TWO_ALU( FU_INTDIV );
                    SI_READ_Y;
                    m_opcode = i_udiv;
                    COMPLEX_OPERATION;
                    break;
                case 15:  // sdiv
                    REG_TWO_ALU( FU_INTDIV );
                    SI_READ_Y;
                    m_opcode = i_sdiv;
                    COMPLEX_OPERATION;
                    break;
                case 16:  // addcc
                    REG_TWO_ALU( FU_INTALU );
                    SI_WRITE_ICC;
                    m_opcode = i_addcc;
                    break;
                case 17:  // andcc
                    REG_TWO_ALU( FU_INTALU );
                    SI_WRITE_ICC;
                    m_opcode = i_andcc;
                    break;
                case 18:  // orcc
                    REG_TWO_ALU( FU_INTALU );
                    SI_WRITE_ICC;
                    m_opcode = i_orcc;
                    break;
                case 19:  // xorcc
                    REG_TWO_ALU( FU_INTALU );
                    SI_WRITE_ICC;
                    m_opcode = i_xorcc;
                    break;
                case 20:  // subcc
                    REG_TWO_ALU( FU_INTALU );
                    SI_WRITE_ICC;
                    if ( rd == 0 ) {
                        m_opcode = i_cmp;
                    } else {
                        m_opcode = i_subcc;
                    }
                    break;
                case 21:  // andncc
                    REG_TWO_ALU( FU_INTALU );
                    SI_WRITE_ICC;
                    m_opcode = i_andncc;
                    break;
                case 22:  // orncc
                    REG_TWO_ALU( FU_INTALU );
                    SI_WRITE_ICC;
                    m_opcode = i_orncc;
                    break;
                case 23:  // xnorcc
                    REG_TWO_ALU( FU_INTALU );
                    SI_WRITE_ICC;
                    m_opcode = i_xnorcc;
                    break;
                case 24:  // addccc
                    REG_TWO_ALU( FU_INTALU );
                    SI_READ_ICC;
                    SI_WRITE_ICC;
                    m_opcode = i_addccc;
                    break;
                case 25:  // _
                    FAIL();
                    break;
                case 26:  // umulcc
                    REG_TWO_ALU( FU_INTMULT );
                    // RS4: reads control_y to establish dependency
                    I_RS4.setRtype( RID_CONTROL );
                    I_RS4.setVanilla( CONTROL_Y );
                    I_RD2.setRtype( RID_INT );
                    I_RD2.setVanillaState( CONTAINER_YCC, 0 );
                    I_RD2.setRtype( RID_CONTAINER );
                    I_RD2.setVanilla( 0 );
                    setFlag(SI_WRITE_CONTROL, true );
                    m_opcode = i_umulcc;
                    break;
                case 27:  // smulcc
                    REG_TWO_ALU( FU_INTMULT );
                    // RS4: reads control_y to establish dependency
                    I_RS4.setRtype( RID_CONTROL );
                    I_RS4.setVanilla( CONTROL_Y );
                    I_RD2.setRtype( RID_INT );
                    I_RD2.setVanillaState( CONTAINER_YCC, 0 );
                    I_RD2.setRtype( RID_CONTAINER );
                    I_RD2.setVanilla( 0 );
                    setFlag(SI_WRITE_CONTROL, true );
                    m_opcode = i_smulcc;
                    break;
                case 28:  // subccc
                    REG_TWO_ALU( FU_INTALU );
                    SI_READ_ICC;
                    m_opcode = i_subccc;
                    break;
                case 29:  // _
                    FAIL();
                    break;
                case 30:  // udivcc
                    REG_TWO_ALU( FU_INTDIV );
                    SI_WRITE_ICC;
                    SI_READ_Y;
                    m_opcode = i_udivcc;
                    COMPLEX_OPERATION;
                    break;
                case 31:  // sdivcc
                    REG_TWO_ALU( FU_INTDIV );
                    SI_WRITE_ICC;
                    SI_READ_Y;
                    m_opcode = i_sdivcc;
                    COMPLEX_OPERATION;
                    break;
                case 32:  // taddcc
                    REG_TWO_ALU( FU_INTALU );
                    m_opcode = i_taddcc;
                    break;
                case 33:  // tsubcc
                    REG_TWO_ALU( FU_INTALU );
                    m_opcode = i_tsubcc;
                    break;
                case 34:  // taddcctv
                    REG_TWO_ALU( FU_INTALU );
                    m_opcode = i_taddcctv;
                    break;
                case 35:  // tsubcctv
                    REG_TWO_ALU( FU_INTALU );
                    m_opcode = i_tsubcctv;
                    break;
                case 36:  // mulscc
                    REG_TWO_ALU( FU_INTALU );
                    // RS3: reads %ccr
                    SI_READ_ICC;
                    // RS4: reads %y
                    SI_READ_Y;

                    // RD2: writes %ccr, and %y
                    I_RD2.setRtype( RID_INT );
                    I_RD2.setVanillaState( CONTAINER_YCC, 0 );
                    I_RD2.setRtype( RID_CONTAINER );
                    I_RD2.setVanilla( 0 );
                    setFlag(SI_WRITE_CONTROL, true );
                    // no condition code shift: use ICC in computation
                    m_opcode = i_mulscc;
                    break;
                case 37:  // _sll
                    REG_TWO_SHIFT;
                    if (x) {
                        m_opcode = i_sllx;
                    } else {
                        m_opcode = i_sll;
                    }
                    break;
                case 38:  // _srl
                    REG_TWO_SHIFT;
                    x = maskBits32( inst, 12, 12 );
                    if (x) {
                        m_opcode = i_srlx;
                    } else {
                        m_opcode = i_srl;
                    }
                    break;
                case 39:  // _sra
                    REG_TWO_SHIFT;
                    x = maskBits32( inst, 12, 12 );
                    if (x) {
                        m_opcode = i_srax;
                    } else {
                        m_opcode = i_sra;
                    }
                    break;
                case 40:  // _rd
                    m_futype = FU_NONE;

                    // stbar membar
                    if (rs1 == 15 && rd == 0) {
                        m_type = DYN_EXECUTE;
                        m_opcode = i_stbar;

#if 0
                        /* CM FIX: simics decodes both stbar, and membar as "stbar".
                           when we don't compare as much with simics's decode, we should
                           comment in this section and use both of them for more correctness.
                         */   
                        if (i == 0) {
                            // stbar
                            m_opcode = i_stbar;
                        } else {
                            // membar
                            m_opcode = i_membar;
                        }
#endif
                    } else {
                        // rd %y, %ccr, etc...
                        m_type   = DYN_EXECUTE;
                        SI_INT_RID( rd, I_RD1 );
                        // x is the internal representation of the control register
                        x = reg_box_t::stateregMap( rs1, false );
                        if ( x == CONTROL_CCR ) {
                            SI_READ_ICC;
                            m_opcode = i_rdcc;
                        } else {
                            SI_READ_CONTROL( x );
                            m_opcode = i_rd;
                            COMPLEX_OPERATION;
                        }
                    }
                    break;

                case 41:  // _
                    FAIL();
                    break;
                case 42:  // rdpr
                    m_futype = FU_NONE;
                    SI_INT_RID( rd, I_RD1 );
                    SI_READ_CONTROL( reg_box_t::stateregMap( rs1, true ) );
                    m_opcode = i_rdpr;
                    break;
                case 43:  // flushw
                    m_futype = FU_NONE;
                    if ( rd == 0 && maskBits32( inst, 18, 0 ) == 0 ) {
                        SI_READ_CONTROL( CONTROL_CWP );
                        m_opcode = i_flushw;
                        COMPLEX_OPERATION;
                    } else {
                        FAIL();
                    }
                    break;
                case 44:  // _mov (MOVcc)
                    m_futype = FU_NONE;
                    movcc2 = maskBits32( inst, 18, 18 );
                    REG_ONE_MOVE;
                    if (movcc2) {
                        SI_READ_ICC;
                        m_ccshift = maskBits32( inst, 12, 11 ) << 1;
                        if (m_ccshift != 0 && m_ccshift != 4) {
                            FAIL();
                        }
                        switch (mcond) {
                            case 0:  // movn
                                m_opcode = i_movn;
                                break;
                            case 1:  // move
                                m_opcode = i_move;
                                break;
                            case 2:  // movle
                                m_opcode = i_movle;
                                break;
                            case 3:  // movl
                                m_opcode = i_movl;
                                break;
                            case 4:  // movleu
                                m_opcode = i_movleu;
                                break;
                            case 5:  // movcs
                                m_opcode = i_movcs;
                                break;
                            case 6:  // movneg
                                m_opcode = i_movneg;
                                break;
                            case 7:  // movvs
                                m_opcode = i_movvs;
                                break;
                            case 8:  // mova
                                m_opcode = i_mova;
                                break;
                            case 9:  // movne
                                m_opcode = i_movne;
                                break;
                            case 10:  // movg
                                m_opcode = i_movg;
                                break;
                            case 11:  // movge
                                m_opcode = i_movge;
                                break;
                            case 12:  // movgu
                                m_opcode = i_movgu;
                                break;
                            case 13:  // movcc
                                m_opcode = i_movcc;
                                break;
                            case 14:  // movpos
                                m_opcode = i_movpos;
                                break;
                            case 15:  // movvc
                                m_opcode = i_movvc;
                                break;
                            default:
                                FAIL();
                        }
                    } else {
                        // set up the third source -- the condition code register
                        movcc2 = maskBits32( inst, 12, 11 );
                        switch (movcc2) {
                            case 0:
                                SI_CC_RID( REG_CC_FCC0, I_RS3 );
                                break;
                            case 1:
                                SI_CC_RID( REG_CC_FCC1, I_RS3 );
                                break;
                            case 2:
                                SI_CC_RID( REG_CC_FCC2, I_RS3 );
                                break;
                            case 3:
                                SI_CC_RID( REG_CC_FCC3, I_RS3 );
                                break;
                            default:
                                FAIL();
                        }
                        switch (mcond) {
                            case 0:  // movfn
                                m_opcode = i_movfn;
                                break;
                            case 1:  // movfne
                                m_opcode = i_movfne;
                                break;
                            case 2:  // movflg
                                m_opcode = i_movflg;
                                break;
                            case 3:  // movful
                                m_opcode = i_movful;
                                break;
                            case 4:  // movfl
                                m_opcode = i_movfl;
                                break;
                            case 5:  // movfug
                                m_opcode = i_movfug;
                                break;
                            case 6:  // movfg
                                m_opcode = i_movfg;
                                break;
                            case 7:  // movfu
                                m_opcode = i_movfu;
                                break;
                            case 8:  // movfa
                                m_opcode = i_movfa;
                                break;
                            case 9:  // movfe
                                m_opcode = i_movfe;
                                break;
                            case 10:  // movfue
                                m_opcode = i_movfue;
                                break;
                            case 11:  // movfge
                                m_opcode = i_movfge;
                                break;
                            case 12:  // movfuge
                                m_opcode = i_movfuge;
                                break;
                            case 13:  // movfle
                                m_opcode = i_movfle;
                                break;
                            case 14:  // movfule
                                m_opcode = i_movfule;
                                break;
                            case 15:  // movfo
                                m_opcode = i_movfo;
                                break;
                            default:
                                FAIL();
                        }
                    }
                    break;
                case 45:  // sdivx
                    REG_TWO_ALU( FU_INTDIV );
                    m_opcode = i_sdivx;
                    break;
                case 46:  // _popc
                    switch (rs1) {
                        case 0:
                            m_futype = FU_INTALU;
                            SI_INT_RID( rd, I_RD1 );
                            if (i == 0) {             
                                setFlag(SI_ISIMMEDIATE, false );
                                SI_INT_RID( rs2, I_RS2 );
                            } else {                  
                                setFlag(SI_ISIMMEDIATE, true ); 
                                m_imm = sign_ext64( maskBits32( inst, 12, 0 ), 12 );
                            }
                            m_opcode = i_popc;
                            break;
                        default:
                            FAIL();
                    }
                    break;
                case 47:  // _movrz
                    m_futype = FU_NONE;
                    // RD1 and RS1 are the destination register
                    // RS2 is the source register
                    // RS3 is the test integer register
                    // see FP_MOV_OP / fmovrsz for other examples of this
                    REG_TWO_I( 9 );
                    SI_INT_RID( rd,  I_RS1 );
                    SI_INT_RID( rs1, I_RS3 );
                    switch (rcond) {
                        case 0:  // _
                            FAIL();
                            break;
                        case 1:  // movrz
                            m_opcode = i_movrz;
                            break;
                        case 2:  // movrlez
                            m_opcode = i_movrlez;
                            break;
                        case 3:  // movrlz
                            m_opcode = i_movrlz;
                            break;
                        case 4:  // _
                            FAIL();
                            break;
                        case 5:  // movrnz
                            m_opcode = i_movrnz;
                            break;
                        case 6:  // movrgz
                            m_opcode = i_movrgz;
                            break;
                        case 7:  // movrgez
                            m_opcode = i_movrgez;
                            break;
                        default:
                            FAIL();
                    }
                    break;
                case 48:  // wr
                    m_futype = FU_NONE;
                    switch ( rd ) {
                        case 2:      // CCR
                            SI_WRITE_ICC;
                            m_opcode = i_wrcc;
                            break;

                        default:
                            SI_WRITE_CONTROL( reg_box_t::stateregMap(rd, false) );
                            m_opcode = i_wr;
                    }
                    SI_INT_RID( rs1, I_RS1 );  
                    if (i == 0) {              
                        setFlag(SI_ISIMMEDIATE, false ); 
                        SI_INT_RID( rs2, I_RS2 );
                    } else {                   
                        setFlag(SI_ISIMMEDIATE, true );  
                        m_imm = sign_ext64( maskBits32( inst, 12, 0 ), 12 );
                    }
                    break;
                case 49:  // _saverestored
                    switch ( rd ) {
                        case 0:   
                            m_futype = FU_NONE;
                            m_opcode = i_saved;
                            SI_WRITE_CONTROL( CONTROL_PSTATE );
                            COMPLEX_OPERATION;
                            break;
                        case 1:   
                            m_futype = FU_NONE;
                            m_opcode = i_restored;
                            SI_WRITE_CONTROL( CONTROL_PSTATE );
                            COMPLEX_OPERATION;
                            break;
                        default:
                            FAIL();
                    }
                    break;
                case 50:  // wrpr
                    m_futype = FU_NONE;
                    // x is the internal representation of the control register
                    x = reg_box_t::stateregMap(rd, true);
                    if (x == CONTROL_PSTATE ||
                            x == CONTROL_TL ||
                            x == CONTROL_CWP ) {
                        m_type = DYN_CONTROL;
                        m_futype = FU_BRANCH;
                        m_branch_type = BRANCH_PRIV;
                    }
                    SI_INT_RID( rs1, I_RS1 );  

                    // wrpr depends on current tl in certain instances
                    SI_WRITE_CONTROL( x );
                    if (i == 0) {              
                        setFlag(SI_ISIMMEDIATE, false ); 
                        SI_INT_RID( rs2, I_RS2 );
                    } else {                   
                        setFlag(SI_ISIMMEDIATE, true );  
                        m_imm = sign_ext64( maskBits32( inst, 12, 0 ), 12 );
                    }
                    m_opcode = i_wrpr;
					// DEBUG_OUT("i_wrpr: Inst = 0x%x\t", inst) ;
					// DEBUG_OUT("rd = 0x%x, x = 0x%x\n", rd, x) ;
                    break;
                case 51:  // _
                    FAIL();
                    break;
                case 52:  // _fop1
                    m_futype = FU_NONE;
                    switch (opf_hi) {

                        case 0: // _fmov
                            switch (opf_lo) {
                                case 0:  // _
                                    FAIL();
                                    break;
                                case 1:  // fmovs
                                    FP_MONO_OP( SINGLE );
                                    m_opcode = i_fmovs;
                                    break;
                                case 2:  // fmovd
                                    FP_MONO_OP( DOUBLE );
                                    m_opcode = i_fmovd;
                                    break;
                                case 3:  // fmovq
                                    //NOQD FP_MONO_OP( QUAD );
                                    m_opcode = i_fmovq;
                                    break;
                                case 4:  // _
                                    FAIL();
                                    break;
                                case 5:  // fnegs
                                    m_futype = FU_FLOATADD;
                                    FP_MONO_OP( SINGLE );
                                    m_opcode = i_fnegs;
                                    break;
                                case 6:  // fnegd
                                    m_futype = FU_FLOATADD;
                                    FP_MONO_OP( DOUBLE );
                                    m_opcode = i_fnegd;
                                    break;
                                case 7:  // fnegq
                                    m_futype = FU_FLOATADD;
                                    //NOQD          FP_MONO_OP( QUAD );
                                    m_opcode = i_fnegq;
                                    break;
                                case 8:  // _
                                    FAIL();
                                    break;
                                case 9:  // fabss
                                    m_futype = FU_FLOATADD;
                                    FP_MONO_OP( SINGLE );
                                    m_opcode = i_fabss;
                                    break;
                                case 10:  // fabsd
                                    m_futype = FU_FLOATADD;
                                    FP_MONO_OP( DOUBLE );
                                    m_opcode = i_fabsd;
                                    break;
                                case 11:  // fabsq
                                    m_futype = FU_FLOATADD;
                                    //NOQD          FP_MONO_OP( QUAD );
                                    m_opcode = i_fabsq;
                                    break;
                                default:
                                    FAIL();
                            }
                            break;

                        case 2: // _sqrt
                            switch (opf_lo) {
                                case 9:
                                    m_futype = FU_FLOATSQRT;
                                    FP_MONO_OP( SINGLE );
                                    m_opcode = i_fsqrts;
                                    break;
                                case 10:
                                    m_futype = FU_FLOATSQRT;
                                    FP_MONO_OP( DOUBLE );
                                    m_opcode = i_fsqrtd;
                                    break;
                                case 11:
                                    m_futype = FU_FLOATSQRT;
                                    //NOQD          FP_MONO_OP( QUAD );
                                    m_opcode = i_fsqrtq;
                                    break;
                                default:
                                    FAIL();
                            }
                            break;

                        case 4: // _fadd || _fmul
                            switch (opf_lo) {
                                case 1:
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
                                    m_opcode = i_fadds;
                                    break;
                                case 2:
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
                                    m_opcode = i_faddd;
                                    break;
                                case 3:
                                    m_futype = FU_FLOATADD;
                                    //NOQD          FP_TRI_OP( QUAD );
                                    m_opcode = i_faddq;
                                    break;
                                case 5:
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
                                    m_opcode = i_fsubs;
                                    break;
                                case 6:
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
                                    m_opcode = i_fsubd;
                                    break;
                                case 7:
                                    m_futype = FU_FLOATADD;
                                    //NOQD          FP_TRI_OP( QUAD );
                                    m_opcode = i_fsubq;
                                    break;

                                case 9:
                                    m_futype = FU_FLOATMULT;
                                    FP_TRI_OP( SINGLE );
                                    m_opcode = i_fmuls;
                                    break;
                                case 10:
                                    m_futype = FU_FLOATMULT;
                                    FP_TRI_OP( DOUBLE );
                                    m_opcode = i_fmuld;
                                    break;
                                case 11:
                                    m_futype = FU_FLOATMULT;
                                    //NOQD          FP_TRI_OP( QUAD );
                                    m_opcode = i_fmulq;
                                    break;
                                case 13:
                                    m_futype = FU_FLOATDIV;
                                    FP_TRI_OP( SINGLE );
                                    m_opcode = i_fdivs;
                                    break;
                                case 14:
                                    m_futype = FU_FLOATDIV;
                                    FP_TRI_OP( DOUBLE );
                                    m_opcode = i_fdivd;
                                    break;
                                case 15:
                                    m_futype = FU_FLOATDIV;
                                    //NOQD          FP_TRI_OP( QUAD );
                                    m_opcode = i_fdivq;
                                    break;

                                default:
                                    FAIL();
                            }
                            break;

                        case 6: // _fmul
                            switch (opf_lo) {
                                case 9:
                                    m_futype = FU_FLOATMULT;
                                    SI_DOUBLE_RID(rd, I_RD1);
                                    SI_SINGLE_RID(rs1, I_RS1);
                                    SI_SINGLE_RID(rs2, I_RS2);
                                    m_opcode = i_fsmuld;
                                    break;
                                case 14:
                                    m_futype = FU_FLOATMULT;
                                    //NOQD          SI_QUAD_RID(rd, I_RD1);
                                    SI_DOUBLE_RID(rs1, I_RS1);
                                    SI_DOUBLE_RID(rs2, I_RS2);
                                    m_opcode = i_fdmulq;
                                    break;
                                default:
                                    FAIL();
                            }
                            break;

                        case 8: // _fytoz
                            m_futype = FU_FLOATCVT;
                            switch (opf_lo) {
                                case 1:  // fstox
                                    //source should be SINGLE, dest should be DOUBLE (64bit integer double) written using writeInt64():
                                    SI_DOUBLE_RID(rd, I_RD1);
                                    SI_SINGLE_RID(rs2, I_RS2);   
                                    m_opcode = i_fstox;
                                    break;
                                case 2:  // fdtox
                                    SI_DOUBLE_RID(rd, I_RD1);
                                    SI_DOUBLE_RID(rs2, I_RS2);
                                    m_opcode = i_fdtox;
                                    break;
                                case 3:  // fqtox
                                    //NOQD          SI_QUAD_RID(rd, I_RD1);
                                    SI_DOUBLE_RID(rs2, I_RS2);
                                    m_opcode = i_fqtox;
                                    break;
                                case 4:  // fxtos
                                    SI_SINGLE_RID(rd, I_RD1);
                                    SI_DOUBLE_RID(rs2, I_RS2);
                                    m_opcode = i_fxtos;
                                    break;
                                case 8:  // fxtod
                                    SI_DOUBLE_RID(rd, I_RD1);
                                    SI_DOUBLE_RID(rs2, I_RS2);
                                    m_opcode = i_fxtod;
                                    break;
                                case 12:  // fxtoq
                                    //NOQD          SI_QUAD_RID(rd, I_RD1);
                                    SI_DOUBLE_RID(rs2, I_RS2);
                                    m_opcode = i_fxtoq;
                                    break;
                                default:
                                    FAIL();
                            }
                            break;

                        case 12: // _fytoz
                            m_futype = FU_FLOATCVT;
                            switch (opf_lo) {
                                case 9:  // fstod
                                    SI_DOUBLE_RID(rd, I_RD1);
                                    SI_SINGLE_RID(rs2, I_RS2);
                                    m_opcode = i_fstod;
                                    break;
                                case 13:  // fstoq
                                    //NOQD          SI_QUAD_RID(rd, I_RD1);
                                    SI_SINGLE_RID(rs2, I_RS2);
                                    m_opcode = i_fstoq;
                                    break;
                                case 6:  // fdtos
                                    SI_SINGLE_RID(rd, I_RD1);
                                    SI_DOUBLE_RID(rs2, I_RS2);
                                    m_opcode = i_fdtos;
                                    break;
                                case 14:  // fdtoq
                                    //NOQD          SI_QUAD_RID(rd, I_RD1);
                                    SI_DOUBLE_RID(rs2, I_RS2);
                                    m_opcode = i_fdtoq;
                                    break;
                                case 7:  // fqtos
                                    SI_SINGLE_RID(rd, I_RD1);
                                    //NOQD          SI_QUAD_RID(rs2, I_RS2);
                                    m_opcode = i_fqtos;
                                    break;
                                case 11:  // fqtod
                                    SI_DOUBLE_RID(rd, I_RD1);
                                    //NOQD          SI_QUAD_RID(rs2, I_RS2);
                                    m_opcode = i_fqtod;
                                    break;

                                case 4:  // fitos
                                    SI_SINGLE_RID(rd, I_RD1);
                                    SI_SINGLE_RID(rs2, I_RS2);
                                    m_opcode = i_fitos;
                                    break;
                                case 8:  // fitod
                                    SI_DOUBLE_RID(rd, I_RD1);
                                    SI_SINGLE_RID(rs2, I_RS2);
                                    m_opcode = i_fitod;
                                    break;
                                case 12:  // fitoq
                                    //NOQD          SI_QUAD_RID(rd, I_RD1);
                                    SI_SINGLE_RID(rs2, I_RS2);
                                    m_opcode = i_fitoq;
                                    break;
                                default:
                                    FAIL();
                            }
                            break;

                        case 13: // _fytoz

                            switch (opf_lo) {
                                case 1:  // fstoi
                                    SI_SINGLE_RID(rd, I_RD1);
                                    SI_SINGLE_RID(rs2, I_RS2);
                                    m_opcode = i_fstoi;
                                    break;
                                case 2:  // fdtoi
                                    SI_SINGLE_RID(rd, I_RD1);
                                    SI_DOUBLE_RID(rs2, I_RS2);
                                    m_opcode = i_fdtoi;
                                    break;
                                case 3:  // fqtoi
                                    SI_SINGLE_RID(rd, I_RD1);
                                    //NOQD          SI_QUAD_RID(rs2, I_RS2);
                                    m_opcode = i_fqtoi;
                                    break;
                                default:
                                    FAIL();
                            }
                            break;

                        default:
                            FAIL();
                    }
                    break; // end case 52 _fop1

                case 53:  // _fop2
                    m_futype = FU_NONE;
                    x = maskBits32( inst, 29, 27 );
                    movcc2 = maskBits32( inst, 13, 13 );
                    sdq = maskBits32( inst, 5, 7 );

                    // _fcmp
                    if ( x == 0 && opf_hi == 5 ) {
                        m_futype = FU_FLOATCMP;

                        // set up the third source -- the condition code register
                        movcc2 = maskBits32( inst, 26, 25 );
                        switch (movcc2) {
                            case 0:
                                SI_CC_RID( REG_CC_FCC0, I_RD1 );
                                break;
                            case 1:
                                SI_CC_RID( REG_CC_FCC1, I_RD1 );
                                break;
                            case 2:
                                SI_CC_RID( REG_CC_FCC2, I_RD1 );
                                break;
                            case 3:
                                SI_CC_RID( REG_CC_FCC3, I_RD1 );
                                break;
                            default:
                                FAIL();
                        }

                        switch (opf_lo) {
                            case 1:
                                FP_CMP( SINGLE );
                                m_opcode = i_fcmps;
                                break;
                            case 2:
                                FP_CMP( DOUBLE );
                                m_opcode = i_fcmpd;
                                break;
                            case 3:
                                //NOQD          FP_CMP( QUAD );
                                m_opcode = i_fcmpq;
                                break;
                            case 5:
                                FP_CMP( SINGLE );
                                m_opcode = i_fcmpes;
                                break;
                            case 6:
                                FP_CMP( DOUBLE );
                                m_opcode = i_fcmped;
                                break;
                            case 7:
                                //NOQD          FP_CMP( QUAD );
                                m_opcode = i_fcmpeq;
                                break;
                            default:
                                FAIL();
                        }
                        break;

                    } else if (movcc2) { // integer op
                        switch (sdq) {

                            case 1: // _single
                                FP_MOVE_OP( SINGLE );
                                SI_READ_ICC;

                                switch (mcond) {
                                    case 0:  // fmovsn
                                        m_opcode = i_fmovsn;
                                        break;
                                    case 1:  // fmovse
                                        m_opcode = i_fmovse;
                                        break;
                                    case 2:  // fmovsle
                                        m_opcode = i_fmovsle;
                                        break;
                                    case 3:  // fmovsl
                                        m_opcode = i_fmovsl;
                                        break;
                                    case 4:  // fmovsleu
                                        m_opcode = i_fmovsleu;
                                        break;
                                    case 5:  // fmovscs
                                        m_opcode = i_fmovscs;
                                        break;
                                    case 6:  // fmovsneg
                                        m_opcode = i_fmovsneg;
                                        break;
                                    case 7:  // fmovsvs
                                        m_opcode = i_fmovsvs;
                                        break;
                                    case 8:  // fmovsa
                                        m_opcode = i_fmovsa;
                                        break;
                                    case 9:  // fmovsne
                                        m_opcode = i_fmovsne;
                                        break;
                                    case 10:  // fmovsg
                                        m_opcode = i_fmovsg;
                                        break;
                                    case 11:  // fmovsge
                                        m_opcode = i_fmovsge;
                                        break;
                                    case 12:  // fmovsgu
                                        m_opcode = i_fmovsgu;
                                        break;
                                    case 13:  // fmovscc
                                        m_opcode = i_fmovscc;
                                        break;
                                    case 14:  // fmovspos
                                        m_opcode = i_fmovspos;
                                        break;
                                    case 15:  // fmovsvc
                                        m_opcode = i_fmovsvc;
                                        break;
                                    default:
                                        FAIL();
                                }
                                break;

                            case 2: // _double
                                FP_MOVE_OP( DOUBLE );
                                SI_READ_ICC;

                                switch (mcond) {
                                    case 0:  // fmovdn
                                        m_opcode = i_fmovdn;
                                        break;
                                    case 1:  // fmovde
                                        m_opcode = i_fmovde;
                                        break;
                                    case 2:  // fmovdle
                                        m_opcode = i_fmovdle;
                                        break;
                                    case 3:  // fmovdl
                                        m_opcode = i_fmovdl;
                                        break;
                                    case 4:  // fmovdleu
                                        m_opcode = i_fmovdleu;
                                        break;
                                    case 5:  // fmovdcs
                                        m_opcode = i_fmovdcs;
                                        break;
                                    case 6:  // fmovdneg
                                        m_opcode = i_fmovdneg;
                                        break;
                                    case 7:  // fmovdvs
                                        m_opcode = i_fmovdvs;
                                        break;
                                    case 8:  // fmovda
                                        m_opcode = i_fmovda;
                                        break;
                                    case 9:  // fmovdne
                                        m_opcode = i_fmovdne;
                                        break;
                                    case 10:  // fmovdg
                                        m_opcode = i_fmovdg;
                                        break;
                                    case 11:  // fmovdge
                                        m_opcode = i_fmovdge;
                                        break;
                                    case 12:  // fmovdgu
                                        m_opcode = i_fmovdgu;
                                        break;
                                    case 13:  // fmovdcc
                                        m_opcode = i_fmovdcc;
                                        break;
                                    case 14:  // fmovdpos
                                        m_opcode = i_fmovdpos;
                                        break;
                                    case 15:  // fmovdvc
                                        m_opcode = i_fmovdvc;
                                        break;
                                    default:
                                        FAIL();
                                }
                                break;

                            case 3: // _quad
                                //NOQD          FP_MOVE_OP( QUAD );
                                SI_READ_ICC;

                                switch (mcond) {
                                    case 0:  // fmovqn
                                        m_opcode = i_fmovqn;
                                        break;
                                    case 1:  // fmovqe
                                        m_opcode = i_fmovqe;
                                        break;
                                    case 2:  // fmovqle
                                        m_opcode = i_fmovqle;
                                        break;
                                    case 3:  // fmovql
                                        m_opcode = i_fmovql;
                                        break;
                                    case 4:  // fmovqleu
                                        m_opcode = i_fmovqleu;
                                        break;
                                    case 5:  // fmovqcs
                                        m_opcode = i_fmovqcs;
                                        break;
                                    case 6:  // fmovqneg
                                        m_opcode = i_fmovqneg;
                                        break;
                                    case 7:  // fmovqvs
                                        m_opcode = i_fmovqvs;
                                        break;
                                    case 8:  // fmovqa
                                        m_opcode = i_fmovqa;
                                        break;
                                    case 9:  // fmovqne
                                        m_opcode = i_fmovqne;
                                        break;
                                    case 10:  // fmovqg
                                        m_opcode = i_fmovqg;
                                        break;
                                    case 11:  // fmovqge
                                        m_opcode = i_fmovqge;
                                        break;
                                    case 12:  // fmovqgu
                                        m_opcode = i_fmovqgu;
                                        break;
                                    case 13:  // fmovqcc
                                        m_opcode = i_fmovqcc;
                                        break;
                                    case 14:  // fmovqpos
                                        m_opcode = i_fmovqpos;
                                        break;
                                    case 15:  // fmovqvc
                                        m_opcode = i_fmovqvc;
                                        break;
                                    default:
                                        FAIL();
                                }
                                break;

                            default:          // end single / double / quad
                                FAIL();
                        }
                        break;

                    } else {      // floating point move

                        // set up the third source -- the condition code register
                        movcc2 = maskBits32( inst, 12, 11 );
                        switch (movcc2) {
                            case 0:
                                SI_CC_RID( REG_CC_FCC0, I_RS3 );
                                break;
                            case 1:
                                SI_CC_RID( REG_CC_FCC1, I_RS3 );
                                break;
                            case 2:
                                SI_CC_RID( REG_CC_FCC2, I_RS3 );
                                break;
                            case 3:
                                SI_CC_RID( REG_CC_FCC3, I_RS3 );
                                break;
                            default:
                                FAIL();
                        }

                        switch (sdq) {

                            case 1: // _single
                                FP_MOVE_OP( SINGLE );

                                switch (mcond) {
                                    case 0:  // fmovfsn  -- a more correct decode may be "fmovsn %fcc"
                                        m_opcode = i_fmovfsn;
                                        break;
                                    case 1:  // fmovfsne
                                        m_opcode = i_fmovfsne;
                                        break;
                                    case 2:  // fmovfslg
                                        m_opcode = i_fmovfslg;
                                        break;
                                    case 3:  // fmovfsul
                                        m_opcode = i_fmovfsul;
                                        break;
                                    case 4:  // fmovfsl
                                        m_opcode = i_fmovfsl;
                                        break;
                                    case 5:  // fmovfsug
                                        m_opcode = i_fmovfsug;
                                        break;
                                    case 6:  // fmovfsg
                                        m_opcode = i_fmovfsg;
                                        break;
                                    case 7:  // fmovfsu
                                        m_opcode = i_fmovfsu;
                                        break;
                                    case 8:  // fmovfsa
                                        m_opcode = i_fmovfsa;
                                        break;
                                    case 9:  // fmovfse
                                        m_opcode = i_fmovfse;
                                        break;
                                    case 10:  // fmovfsue
                                        m_opcode = i_fmovfsue;
                                        break;
                                    case 11:  // fmovfsge
                                        m_opcode = i_fmovfsge;
                                        break;
                                    case 12:  // fmovfsuge
                                        m_opcode = i_fmovfsuge;
                                        break;
                                    case 13:  // fmovfsle
                                        m_opcode = i_fmovfsle;
                                        break;
                                    case 14:  // fmovfsule
                                        m_opcode = i_fmovfsule;
                                        break;
                                    case 15:  // fmovfso
                                        m_opcode = i_fmovfso;
                                        break;
                                    default:
                                        FAIL();
                                }
                                break;

                            case 2: // _double
                                FP_MOVE_OP( DOUBLE );

                                switch (mcond) {
                                    case 0:  // fmovfdn
                                        m_opcode = i_fmovfdn;
                                        break;
                                    case 1:  // fmovfdne
                                        m_opcode = i_fmovfdne;
                                        break;
                                    case 2:  // fmovfdlg
                                        m_opcode = i_fmovfdlg;
                                        break;
                                    case 3:  // fmovfdul
                                        m_opcode = i_fmovfdul;
                                        break;
                                    case 4:  // fmovfdl
                                        m_opcode = i_fmovfdl;
                                        break;
                                    case 5:  // fmovfdug
                                        m_opcode = i_fmovfdug;
                                        break;
                                    case 6:  // fmovfdg
                                        m_opcode = i_fmovfdg;
                                        break;
                                    case 7:  // fmovfdu
                                        m_opcode = i_fmovfdu;
                                        break;
                                    case 8:  // fmovfda
                                        m_opcode = i_fmovfda;
                                        break;
                                    case 9:  // fmovfde
                                        m_opcode = i_fmovfde;
                                        break;
                                    case 10:  // fmovfdue
                                        m_opcode = i_fmovfdue;
                                        break;
                                    case 11:  // fmovfdge
                                        m_opcode = i_fmovfdge;
                                        break;
                                    case 12:  // fmovfduge
                                        m_opcode = i_fmovfduge;
                                        break;
                                    case 13:  // fmovfdle
                                        m_opcode = i_fmovfdle;
                                        break;
                                    case 14:  // fmovfdule
                                        m_opcode = i_fmovfdule;
                                        break;
                                    case 15:  // fmovfdo
                                        m_opcode = i_fmovfdo;
                                        break;
                                    default:
                                        FAIL();
                                }
                                break;

                            case 3: // _quad
                                //NOQD          FP_MOVE_OP( QUAD );

                                switch (mcond) {
                                    case 0:  // fmovfqn
                                        m_opcode = i_fmovfqn;
                                        break;
                                    case 1:  // fmovfqne
                                        m_opcode = i_fmovfqne;
                                        break;
                                    case 2:  // fmovfqlg
                                        m_opcode = i_fmovfqlg;
                                        break;
                                    case 3:  // fmovfqul
                                        m_opcode = i_fmovfqul;
                                        break;
                                    case 4:  // fmovfql
                                        m_opcode = i_fmovfql;
                                        break;
                                    case 5:  // fmovfqug
                                        m_opcode = i_fmovfqug;
                                        break;
                                    case 6:  // fmovfqg
                                        m_opcode = i_fmovfqg;
                                        break;
                                    case 7:  // fmovfqu
                                        m_opcode = i_fmovfqu;
                                        break;
                                    case 8:  // fmovfqa
                                        m_opcode = i_fmovfqa;
                                        break;
                                    case 9:  // fmovfqe
                                        m_opcode = i_fmovfqe;
                                        break;
                                    case 10:  // fmovfque
                                        m_opcode = i_fmovfque;
                                        break;
                                    case 11:  // fmovfqge
                                        m_opcode = i_fmovfqge;
                                        break;
                                    case 12:  // fmovfquge
                                        m_opcode = i_fmovfquge;
                                        break;
                                    case 13:  // fmovfqle
                                        m_opcode = i_fmovfqle;
                                        break;
                                    case 14:  // fmovfqule
                                        m_opcode = i_fmovfqule;
                                        break;
                                    case 15:  // fmovfqo
                                        m_opcode = i_fmovfqo;
                                        break;
                                    default:
                                        FAIL();
                                }
                                break;

                            case 5: // _single based on icc ixx (FMOVr)
                                // RD1 and RS1 are the destination register
                                // RS2 is the source register
                                // RS3 is the test integer register
                                FP_MOVE_OP( SINGLE );
                                SI_INT_RID( rs1, I_RS3 );
                                COMPLEX_OPERATION;

                                switch (rcond) {
                                    case 0:  // _
                                        FAIL();
                                        break;
                                    case 1:  // fmovrsz
                                        m_opcode = i_fmovrsz;
                                        break;
                                    case 2:  // fmovrslez
                                        m_opcode = i_fmovrslez;
                                        break;
                                    case 3:  // fmovrslz
                                        m_opcode = i_fmovrslz;
                                        break;
                                    case 4:  // _
                                        FAIL();
                                        break;
                                    case 5:  // fmovrsnz
                                        m_opcode = i_fmovrsnz;
                                        break;
                                    case 6:  // fmovrsgz
                                        m_opcode = i_fmovrsgz;
                                        break;
                                    case 7:  // fmovrsgez
                                        m_opcode = i_fmovrsgez;
                                        break;
                                    default:
                                        FAIL();
                                }
                                break;

                            case 6: // _double based on icc ixx (FMOVr)
                                // RD1 and RS1 are the destination register
                                // RS2 is the source register
                                // RS3 is the test integer register
                                FP_MOVE_OP( DOUBLE );
                                SI_INT_RID( rs1, I_RS3 );
                                COMPLEX_OPERATION;

                                switch (rcond) {
                                    case 0:  // _
                                        FAIL();
                                        break;
                                    case 1:  // fmovrdz
                                        m_opcode = i_fmovrdz;
                                        break;
                                    case 2:  // fmovrdlez
                                        m_opcode = i_fmovrdlez;
                                        break;
                                    case 3:  // fmovrdlz
                                        m_opcode = i_fmovrdlz;
                                        break;
                                    case 4:  // _
                                        FAIL();
                                        break;
                                    case 5:  // fmovrdnz
                                        m_opcode = i_fmovrdnz;
                                        break;
                                    case 6:  // fmovrdgz
                                        m_opcode = i_fmovrdgz;
                                        break;
                                    case 7:  // fmovrdgez
                                        m_opcode = i_fmovrdgez;
                                        break;
                                    default:
                                        FAIL();
                                }
                                break;

                            case 7: // _quad based on icc ixx (FMOVr)
                                // RD1 and RS1 are the destination register
                                // RS2 is the source register
                                // RS3 is the test integer register
                                //NOQD          FP_MOVE_OP( QUAD );
                                SI_INT_RID( rs1, I_RS3 );
                                COMPLEX_OPERATION;

                                switch (rcond) {
                                    case 0:  // _
                                        FAIL();
                                        break;
                                    case 1:  // fmovrqz
                                        m_opcode = i_fmovrqz;
                                        break;
                                    case 2:  // fmovrqlez
                                        m_opcode = i_fmovrqlez;
                                        break;
                                    case 3:  // fmovrqlz
                                        m_opcode = i_fmovrqlz;
                                        break;
                                    case 4:  // _
                                        FAIL();
                                        break;
                                    case 5:  // fmovrqnz
                                        m_opcode = i_fmovrqnz;
                                        break;
                                    case 6:  // fmovrqgz
                                        m_opcode = i_fmovrqgz;
                                        break;
                                    case 7:  // fmovrqgez
                                        m_opcode = i_fmovrqgez;
                                        break;
                                    default:
                                        FAIL();
                                }
                                break;

                            default:          // end single / double / quad
                                FAIL();
                        }
                    }
                    break;

                case 54:  // impdep1 
					// DEBUG_OUT("Implementation dependent inst\n") ;
					// DEBUG_OUT("hi = %d, lo = %d\n", opf_hi, opf_lo) ;

                    m_futype = FU_NONE;

                    /// VIS instructions
                    switch (opf_hi) {
						case 0: // edge handlign instrs
							switch (opf_lo) {
								case 0: //	edge8
									m_opcode = i_edge8 ;
									break ;
								case 1: //	edge8n
									m_opcode = i_edge8n ;
									break ;
								case 2: //	edge8l
									m_opcode = i_edge8l ;
									break ;
								case 3: //	edge8ln
									m_opcode = i_edge8ln ;
									break ;
								case 4: //	edge16
									m_opcode = i_edge16 ;
									break ;
								case 5: //	edge16n
									m_opcode = i_edge16n ;
									break ;
								case 6: //	edge16l
									m_opcode = i_edge16l ;
									break ;
								case 7: //	edge16ln
									m_opcode = i_edge16ln ;
									break ;
								case 8: //	edge32
									m_opcode = i_edge32 ;
									break ;
								case 9: //	edge32n
									m_opcode = i_edge32n ;
									break ;
								case 10: //	edge32l
									m_opcode = i_edge32l ;
									break ;
								case 11: //	edge32ln
									m_opcode = i_edge32ln ;
									break ;

								default:
									FAIL() ;
							}
							break ;

                        case 1:
                            switch (opf_lo) {
								case 0: // array8
									m_opcode = i_array8 ;
									break ;
								case 2: // array16
									m_opcode = i_array16 ;
									break ;
								case 4: // array32
									m_opcode = i_array32 ;
									break ;
                                case 8: // alignaddr
                                    m_opcode = i_alignaddr;
                                    SI_INT_RID( rd, I_RD1 );
                                    SI_INT_RID( rs1, I_RS1 );
                                    SI_INT_RID( rs2, I_RS2 );
                                    SI_WRITE_CONTROL( CONTROL_GSR );
                                    break;

                                case 9: // bmask
                                    m_opcode = i_bmask;
                                    m_futype = FU_INTALU;
                                    SI_INT_RID( rd,  I_RD1 );
                                    SI_INT_RID( rs1, I_RS1 );
                                    SI_INT_RID( rs2, I_RS2 );
                                    SI_WRITE_CONTROL( CONTROL_GSR );
                                    break;

                                case 10: // alignaddrl
                                    m_opcode = i_alignaddrl;
                                    SI_INT_RID( rd, I_RD1 );
                                    SI_INT_RID( rs1, I_RS1 );
                                    SI_INT_RID( rs2, I_RS2 );
                                    SI_WRITE_CONTROL( CONTROL_GSR );
                                    break;

                                default:
                                    FAIL();
                            }
                            break;

                        case 2:
                            m_futype = FU_FLOATCMP;
                            switch (opf_lo) {
                                case 0:  // fcmple16
                                    m_opcode = i_fcmple16;
                                    break;
                                case 2:  // fcmpne16
                                    m_opcode = i_fcmpne16;
                                    break;
                                case 4:  // fcmple32
                                    m_opcode = i_fcmple32;
                                    break;
                                case 6:  // fcmpne32
                                    m_opcode = i_fcmpne32;
                                    break;
                                case 8:  // fcmpgt16
                                    m_opcode = i_fcmpgt16;
                                    break;
                                case 10: // fcmpeq16
                                    m_opcode = i_fcmpeq16;
                                    break;
                                case 12: // fcmpgt32
                                    m_opcode = i_fcmpgt32;
                                    break;
                                case 14: // fcmpeq32
                                    m_opcode = i_fcmpeq32;
                                    break;
                                default:
                                    FAIL();
                            }
                            break;

						case 3: //	paritioned multiply instructions
							switch (opf_lo) {
								case 1: // fmul8x16
									m_opcode = i_fmul8x16; break ;
								case 3: // fmul8x16au
									m_opcode = i_fmul8x16au; break ;
								case 5: // fmul8x16al
									m_opcode = i_fmul8x16al; break ;
								case 6: // fmul8sux16
									m_opcode = i_fmul8sux16; break ;
								case 7: // fmul8ulx16
									m_opcode = i_fmul8ulx16; break ;
								case 8: // fmuld8sux16
									m_opcode = i_fmuld8sux16; break ;
								case 9: // fmuld8ulx16
									m_opcode = i_fmuld8ulx16; break ;
								case 10: // fpack32
									m_opcode = i_fpack32; break ;
								case 11: // fpack16
									m_opcode = i_fpack16; break ;
								case 13: // fpackfix
									m_opcode = i_fpackfix; break ;
								case 14: // pdist
									m_opcode = i_pdist ; break ;
								default:
									FAIL() ;
							}
							break ;

                        case 4:
                            switch (opf_lo) {
                                case 8: // faligndata
                                    m_opcode = i_faligndata;
                                    SI_DOUBLE_RID( rd, I_RD1 );
                                    SI_DOUBLE_RID( rs1, I_RS1 );
                                    SI_DOUBLE_RID( rs2, I_RS2 );
                                    SI_READ_CONTROL( CONTROL_GSR );
                                    break;

								case 11: // i_fpmerge
								   m_opcode = i_fpmerge; break ;	

                                case 12: // bshuffle
                                    m_opcode = i_bshuffle;
                                    m_futype = FU_INTALU;
                                    SI_DOUBLE_RID( rd,  I_RD1 );
                                    SI_DOUBLE_RID( rs1, I_RS1 );
                                    SI_DOUBLE_RID( rs2, I_RS2 );
                                    SI_READ_CONTROL( CONTROL_GSR );
                                    break;
								
								case 13: // i_fexpand
								   m_opcode = i_fexpand; break ;	

                                default:
                                    FAIL();
                            }
                            break;

						case 5: // partitioned add/subtract
							switch (opf_lo) {
								case 0: // fpadd16
									m_opcode = i_fpadd16 ;
									break ;
								case 1: // fpadd16s
									m_opcode = i_fpadd16s ;
									break ;
								case 2: // fpadd32
									m_opcode = i_fpadd32 ;
									break ;
								case 3: // fpadd32s
									m_opcode = i_fpadd32s ;
									break ;
								case 4: // fpsub16
									m_opcode = i_fpsub16 ;
									break ;
								case 5: // fpsub16s
									m_opcode = i_fpsub16s ;
									break ;
								case 6: // fpsub32
									m_opcode = i_fpsub32 ;
									break ;
								case 7: // fpsub32s
									m_opcode = i_fpsub32s ;
									break ;

								default:
									FAIL() ;
							}
							break ;

                        case 6:
                            switch (opf_lo) {
                                case 0: // fzero
                                    m_opcode = i_fzero;
                                    SI_DOUBLE_RID( rd, I_RD1 );
                                    break;
                                case 1: // fzeros
                                    m_opcode = i_fzeros;
                                    SI_SINGLE_RID( rd, I_RD1 );
                                    break;
								case 2: // fnor
									m_opcode = i_fnor ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
									break ;
								case 3: // fnors
									m_opcode = i_fnors ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
									break ;
								case 4: // fandnot2
									m_opcode = i_fandnot2 ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
									break ;
								case 5: // fandnot2s
									m_opcode = i_fandnot2s ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
									break ;
								case 6: // fnot2
									m_opcode = i_fnot2;
									SI_DOUBLE_RID( rd, I_RD1 ) ;
									SI_DOUBLE_RID( rs2, I_RS2 ) ;
									break ;
								case 7: // fnot2s
									m_opcode = i_fnot2s;
									SI_SINGLE_RID( rd, I_RD1 ) ;
									SI_SINGLE_RID( rs2, I_RS2 ) ;
									break ;
								case 8: // fandnot1
									m_opcode = i_fandnot1 ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
									break ;
								case 9: // fandnot1s
									m_opcode = i_fandnot1s ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
									break ;
								case 10: // fnot1
									m_opcode = i_fnot1;
									SI_DOUBLE_RID( rd, I_RD1 ) ;
									SI_DOUBLE_RID( rs1, I_RS1 ) ;
									break ;
								case 11: // fnot1s
									m_opcode = i_fnot1s;
									SI_SINGLE_RID( rd, I_RD1 ) ;
									SI_SINGLE_RID( rs1, I_RS1 ) ;
									break ;
								case 12: // fxor
									m_opcode = i_fxor ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
									break ;
								case 13: // fxors
									m_opcode = i_fxors ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
									break ;
								case 14: // fnand
									m_opcode = i_fnand ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
									break ;
								case 15: // fnands
									m_opcode = i_fnands ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
									break ;

                                default:
                                    FAIL();
                            }
                            break;

                        case 7:
                            switch (opf_lo) {
								case 0: // fand
									m_opcode = i_fand ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
									break ;
								case 1: // fands
									m_opcode = i_fands ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
									break ;
								case 2: // fnxor
									m_opcode = i_fnxor ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
									break ;
								case 3: // fnxors
									m_opcode = i_fnxors ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
									break ;
                                case 4: // fsrc1
                                    m_opcode = i_fsrc1;
                                    SI_DOUBLE_RID( rd, I_RD1 );
                                    SI_DOUBLE_RID( rs1, I_RS1 );
                                    break;
                                case 5: // fsrc1s
                                    m_opcode = i_fsrc1s;
                                    SI_SINGLE_RID( rd, I_RD1 );
                                    SI_SINGLE_RID( rs1, I_RS1 );
                                    break;
								case 6: // fornot2
									m_opcode = i_fornot2 ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
									break ;
								case 7: // fornot2s
									m_opcode = i_fornot2s ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
									break ;
                                case 8: // fsrc2
                                    m_opcode = i_fsrc2;
                                    SI_DOUBLE_RID( rd, I_RD1 );
                                    SI_DOUBLE_RID( rs2, I_RS2 );
                                    break;
                                case 9: // fsrc2s
                                    m_opcode = i_fsrc2s;
                                    SI_SINGLE_RID( rd, I_RD1 );
                                    SI_SINGLE_RID( rs2, I_RS2 );
                                    break;
								case 10: // fornot1
									m_opcode = i_fornot1 ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
									break ;
								case 11: // fornot1s
									m_opcode = i_fornot1s ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
									break ;
								case 12: // for
									m_opcode = i_for ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( DOUBLE );
									break ;
								case 13: // fors
									m_opcode = i_fors ;
                                    m_futype = FU_FLOATADD;
                                    FP_TRI_OP( SINGLE );
									break ;
								case 14: // fone
									m_opcode = i_fone ;
                                    SI_DOUBLE_RID( rd, I_RD1 );
                                    break;
								case 15: // fone
									m_opcode = i_fones ;
                                    SI_SINGLE_RID( rd, I_RD1 );
                                    break;

                                default:
                                    FAIL();
                            }
                            break;
						case 8: 
							switch (opf_lo) {
								case 0: // shutdown
									m_opcode = i_shutdown ; break ;
								case 1: // siam
									m_opcode = i_siam ; break ;

								default:
									FAIL() ;
							}
							break ;

                        default:
                            FAIL();
                    } // end opf_hi
                    break;

                case 55:  // impdep2
                    m_opcode = i_impdep2;
                    break;

                case 56:  // jmpl
                    m_type   = DYN_CONTROL;
                    m_futype = FU_NONE;
                    m_branch_type = BRANCH_INDIRECT;
                    REG_TWO_I( 12 );
                    // read a pstate register for address mask bits
                    SI_READ_CONTROL( CONTROL_PSTATE );
                    if ( (rd == 0) &&
                            (i == 1 && (rs1 == 15 || rs1 == 31) &&
                             8 == maskBits32( inst, 0, 12 ))
                       ) {
                        // returns use register %i7 == 31|%o7 == 15, and link dest to %g0 == 0
                        m_branch_type = BRANCH_RETURN;
                        m_opcode = i_jmpl;
                    } else if ( rd == 15 ) {
                        // jmpl call use link dest == 15 (%07)
                        m_branch_type = BRANCH_CALL;
                        m_opcode = i_jmpl;
                    } else {
                        m_opcode = i_jmpl;
                    }
                    COMPLEX_OPERATION;
                    break;

                case 57:  // _retrn
                    switch (rd) {
                        case 0:
                            m_type = DYN_CONTROL;
                            m_futype = FU_BRANCH;
                            m_branch_type = BRANCH_RETURN;
                            REG_TWO_I( 12 );
                            SI_RESET( I_RD1 );
                            // read a control register to verify prediction
                            SI_READ_CONTROL( CONTROL_ISREADY );
                            m_opcode = i_retrn;
                            break;
                        default:
                            FAIL();
                    }
                    break;
                case 58:  // _tcc
                    m_type = DYN_CONTROL;
                    m_futype = FU_BRANCH;
                    m_branch_type = BRANCH_TRAP;

                    SI_INT_RID( rs1, I_RS1 );
                    if (i == 0) {
                        setFlag(SI_ISIMMEDIATE, false );
                        SI_INT_RID( rs2, I_RS2 );
                    } else {
                        setFlag(SI_ISIMMEDIATE, true );
                        m_imm = maskBits32( inst, 6, 0 );
                    }
                    SI_READ_ICC;
                    m_ccshift = maskBits32( inst, 12, 11 ) << 1;
                    if (m_ccshift != 0 && m_ccshift != 4) {
                        FAIL();
                    }

                    switch (cond) {
                        case 0:  // tn
                            m_opcode = i_tn;
                            break;
                        case 1:  // te
                            m_opcode = i_te;
                            break;
                        case 2:  // tle
                            m_opcode = i_tle;
                            break;
                        case 3:  // tl
                            m_opcode = i_tl;
                            break;
                        case 4:  // tleu
                            m_opcode = i_tleu;
                            break;
                        case 5:  // tcs
                            m_opcode = i_tcs;
                            break;
                        case 6:  // tneg
                            m_opcode = i_tneg;
                            break;
                        case 7:  // tvs
                            m_opcode = i_tvs;
                            break;
                        case 8:  // ta
                            m_opcode = i_ta;
                            break;
                        case 9:  // tne
                            m_opcode = i_tne;
                            break;
                        case 10:  // tg
                            m_opcode = i_tg;
                            break;
                        case 11:  // tge
                            m_opcode = i_tge;
                            break;
                        case 12:  // tgu
                            m_opcode = i_tgu;
                            break;
                        case 13:  // tcc
                            m_opcode = i_tcc;
                            break;
                        case 14:  // tpos
                            m_opcode = i_tpos;
                            break;
                        case 15:  // tvc
                            m_opcode = i_tvc;
                            break;
                        default:
                            FAIL();
                    }
                    break;

                case 59:  // _flush
                    switch (rd) {
                        case 0:
                            m_futype = FU_NONE;
                            SI_INT_RID( rs1, I_RS1 );  
                            if (i == 0) {              
                                setFlag(SI_ISIMMEDIATE, false ); 
                                SI_INT_RID( rs2, I_RS2 );
                            } else {                   
                                setFlag(SI_ISIMMEDIATE, true );  
                                m_imm = sign_ext64( maskBits32( inst, 12, 0 ), 12 );
                            }
                            m_opcode = i_flush;
                            break;
                        default:
                            FAIL();
                    }
                    break;
                case 60:  // save
                    m_type = DYN_CONTROL;
                    m_futype = FU_BRANCH;
                    REG_TWO_ALU( FU_INTALU );
                    m_branch_type = BRANCH_CWP;
                    m_opcode = i_save;
                    SI_WRITE_CONTROL( CONTROL_PSTATE );
                    COMPLEX_OPERATION;
                    setFlag(SI_UPDATE_CWP, true );
                    break;
                case 61:  // restore
                    m_type = DYN_CONTROL;
                    m_futype = FU_BRANCH;
                    REG_TWO_ALU( FU_INTALU );
                    m_branch_type = BRANCH_CWP;
                    m_opcode = i_restore;
                    SI_WRITE_CONTROL( CONTROL_PSTATE );
                    COMPLEX_OPERATION;
                    setFlag(SI_UPDATE_CWP, true );
                    break;
                case 62:  // _doneretry
                    m_futype = FU_NONE;
                    switch ( rd ) {
                        case 0:
                            m_type = DYN_CONTROL;
                            m_futype = FU_BRANCH;
                            // can't use SI_WRITE_ICC with SI_WRITE_CONTROL, manually configure
                            SI_CC_RID( REG_CC_CCR, I_RD1 );
                            SI_WRITE_CONTROL( CONTROL_PSTATE );
                            m_branch_type = BRANCH_TRAP_RETURN;
                            m_opcode = i_done;
                            break;
                        case 1:
                            m_type = DYN_CONTROL;
                            m_futype = FU_BRANCH;
                            // can't use SI_WRITE_ICC with SI_WRITE_CONTROL, manually configure
                            SI_CC_RID( REG_CC_CCR, I_RD1 );
                            SI_WRITE_CONTROL( CONTROL_PSTATE );
                            m_branch_type = BRANCH_TRAP_RETURN;
                            m_opcode = i_retry;
                            break;
                        default:
                            FAIL();
                    }
                    break;
                case 63:  // _
                    FAIL();
                    break;
                default:
                    FAIL();
            }
            break; /// ** break out of Op 2 instructions

        case 3:
            /// Op 3 instructions

            switch (op3) {
                case 0:  // lduw
                    m_type = DYN_LOAD;
                    m_opcode = i_lduw;
                    READ_TWO_I;
                    INT_WORD;
                    break;
                case 1:  // ldub
                    m_type = DYN_LOAD;
                    m_opcode = i_ldub;
                    READ_TWO_I;
                    INT_BYTE;
                    break;
                case 2:  // lduh
                    m_type = DYN_LOAD;
                    m_opcode = i_lduh;
                    READ_TWO_I;
                    INT_HALF;
                    break;
                case 3:  // ldd
					if (rd&0x1) {
						FAIL();
					} else {
						m_type = DYN_LOAD;
						m_opcode = i_ldd;
						READ_TWO_I;
						// allocate the next contiguous register
						SI_INT_RID( (rd + 1), I_RD2 );
						INT_EXTENDED;
						COMPLEX_OPERATION;
					}
                    break;
                case 4:  // stw
                    m_type = DYN_STORE;
                    m_opcode = i_stw;
                    WRITE_TWO_I;
                    INT_WORD;
                    break;
                case 5:  // stb
                    m_type = DYN_STORE;
                    m_opcode = i_stb;
                    WRITE_TWO_I;
                    INT_BYTE;
                    break;
                case 6:  // sth
                    m_type = DYN_STORE;
                    m_opcode = i_sth;
                    WRITE_TWO_I;
                    INT_HALF;
                    break;
                case 7:  // std
                    m_type = DYN_STORE;
                    m_opcode = i_std;
                    WRITE_TWO_I;
                    // CM imprecise: std completes before both registers are ready...
                    INT_EXTENDED;
                    break;
                case 8:  // ldsw
                    m_type = DYN_LOAD;
                    m_opcode = i_ldsw;
                    READ_TWO_I;
                    INT_WORD;
                    INT_SIGNED;
                    break;
                case 9:  // ldsb
                    m_type = DYN_LOAD;
                    m_opcode = i_ldsb;
                    READ_TWO_I;
                    INT_BYTE;
                    INT_SIGNED;
                    break;
                case 10:  // ldsh
                    m_type = DYN_LOAD;
                    m_opcode = i_ldsh;
                    READ_TWO_I;
                    INT_HALF;
                    INT_SIGNED;
                    break;
                case 11:  // ldx
                    m_type = DYN_LOAD;
                    m_opcode = i_ldx;
                    READ_TWO_I;
                    INT_EXTENDED;
                    break;
                case 12:  // _
                    FAIL();
                    break;
                case 13:  // ldstub
                    m_type = DYN_ATOMIC;
                    m_opcode = i_ldstub;
                    WRITE_TWO_I;
                    SI_INT_RID( rd, I_RD1 );
                    INT_BYTE;
                    COMPLEX_OPERATION;
                    break;
                case 14:  // stx
                    m_type = DYN_STORE;
                    m_opcode = i_stx;
                    WRITE_TWO_I;
                    INT_EXTENDED;
                    break;
                case 15:  // swap
                    m_type = DYN_ATOMIC;
                    m_opcode = i_swap;
                    WRITE_TWO_I;
                    SI_INT_RID( rd, I_RD1 );
                    INT_WORD;
                    COMPLEX_OPERATION;
                    break;

                case 16:  // lduwa
                    m_type = DYN_LOAD;
                    m_opcode = i_lduwa;
                    READ_TWO_I;
                    INT_WORD;
                    SI_READASI;
                    break;
                case 17:  // lduba
                    m_type = DYN_LOAD;
                    m_opcode = i_lduba;
                    READ_TWO_I;
                    INT_BYTE;
                    SI_READASI;
                    break;
                case 18:  // lduha
                    m_type = DYN_LOAD;
                    m_opcode = i_lduha;
                    READ_TWO_I;
                    INT_HALF;
                    SI_READASI;
                    break;
                case 19:  // ldda
					if (rd&0x1) {
						FAIL();
					} else {
						m_type = DYN_LOAD;
						READ_TWO_I;
						// allocate the next contiguous register
						SI_INT_RID( (rd + 1), I_RD2 );
						SI_READASI;
						COMPLEX_OPERATION;

						// test for atomic quad load
						if (i == 0 && ( asi == 0x24 || asi == 0x2c )) {
							// atomic quad load
							INT_QUAD;
							m_opcode = i_ldqa;
						} else {
							// load double word
							INT_EXTENDED;
							m_opcode = i_ldda;
						}
					}
                    break;
                case 20:  // stwa
                    m_type = DYN_STORE;
                    m_opcode = i_stwa;
                    WRITE_TWO_I;
                    INT_WORD;
                    SI_READASI;
                    FETCH_BARRIER;
                    break;
                case 21:  // stba
                    m_type = DYN_STORE;
                    m_opcode = i_stba;
                    WRITE_TWO_I;
                    INT_BYTE;
                    SI_READASI;
                    FETCH_BARRIER;
                    break;
                case 22:  // stha
                    m_type = DYN_STORE;
                    m_opcode = i_stha;
                    WRITE_TWO_I;
                    INT_HALF;
                    SI_READASI;
                    FETCH_BARRIER;
                    break;
                case 23:  // stda
                    m_type = DYN_STORE;
                    m_opcode = i_stda;
                    WRITE_TWO_I;
                    // CM imprecise: stda completes before both registers are ready...
                    INT_EXTENDED;
                    SI_READASI;
                    FETCH_BARRIER;
                    break;
                case 24:  // ldswa
                    m_type = DYN_LOAD;
                    m_opcode = i_ldswa;
                    READ_TWO_I;
                    INT_WORD;
                    INT_SIGNED;
                    SI_READASI;
                    break;
                case 25:  // ldsba
                    m_type = DYN_LOAD;
                    m_opcode = i_ldsba;
                    READ_TWO_I;
                    INT_BYTE;
                    INT_SIGNED;
                    SI_READASI;
                    break;
                case 26:  // ldsha
                    m_type = DYN_LOAD;
                    m_opcode = i_ldsha;
                    READ_TWO_I;
                    INT_HALF;
                    INT_SIGNED;
                    SI_READASI;
                    break;
                case 27:  // ldxa
                    m_type = DYN_LOAD;
                    m_opcode = i_ldxa;
                    READ_TWO_I;
                    INT_EXTENDED;
                    SI_READASI;
                    break;
                case 28:  // _
                    FAIL();
                    break;
                case 29:  // ldstuba
                    m_type = DYN_ATOMIC;
                    m_opcode = i_ldstuba;
                    WRITE_TWO_I;
                    SI_INT_RID( rd, I_RD1 );
                    INT_BYTE;
                    SI_READASI;
                    COMPLEX_OPERATION;
                    break;
                case 30:  // stxa
                    m_type = DYN_STORE;
                    m_opcode = i_stxa;
                    WRITE_TWO_I;
                    INT_EXTENDED;
                    SI_READASI;
                    FETCH_BARRIER;
                    // ASI stores to 0x54 or 0x5c modify the contents of the mmu
                    //   the CONTROL_CWP is a random register (not changed) in the function
                    //   this creates a dependence between this and other regs
                    if (i == 0 || ( (i == 1) && (m_imm == 0x54 || m_imm == 0x5c) )) {
                        SI_WRITE_CONTROL( CONTROL_PSTATE );
                    }
#if 0
                    // writes to address 0x8 of ASI_DMMU_TAG = 0x58 
                    // modify the primary context register of the mmu
                    if (inst == 0xc4f04b00 || inst == 0xe0f44b00) {
                        DEBUG_OUT("asi write control instruction 0x%0x\n", getInst());
                    }
#endif
                    break;
                case 31:  // swapa
                    m_type = DYN_ATOMIC;
                    m_opcode = i_swapa;
                    WRITE_TWO_I;
                    SI_INT_RID( rd, I_RD1 );
                    INT_WORD;
                    SI_READASI;
                    FETCH_BARRIER;
                    COMPLEX_OPERATION;
                    break;

                case 32:  // ldf
                    m_type = DYN_LOAD;
                    m_opcode = i_ldf;
                    READ_TWO_F( RID_SINGLE );
                    FLOAT_SINGLE;
                    break;

                case 33:  // _ldfsr
                    m_type = DYN_LOAD;
                    if (rd == 0) {
                        m_opcode = i_ldfsr;
                        READ_TWO_I;
                        INT_WORD;
                        // This loads directly to a control register, so RD1 is not needed
                        SI_RESET( I_RD1 );
                        SI_WRITE_CONTROL( CONTROL_FSR );
                    } else if (rd == 1) {
                        m_opcode = i_ldxfsr;
                        READ_TWO_I;
                        INT_EXTENDED;
                        // make the destination a control register
                        SI_RESET( I_RD1 );
                        SI_WRITE_CONTROL( CONTROL_FSR );
                    } else {
                        FAIL();
                    }
                    break;
                case 34:  // ldqf
                    m_type = DYN_LOAD;
                    m_opcode = i_ldqf;
                    //NOQD       READ_TWO_F( RID_QUAD );
                    FLOAT_QUAD;
                    break;
                case 35:  // lddf
                    m_type = DYN_LOAD;
                    m_opcode = i_lddf;
                    READ_TWO_F( RID_DOUBLE );
                    FLOAT_DOUBLE;
                    break;
                case 36:  // stf
                    m_type = DYN_STORE;
                    m_opcode = i_stf;
                    WRITE_TWO_F( RID_SINGLE );
                    FLOAT_SINGLE;
                    break;
                case 37:  // _stfsr
                    if (rd == 0) {
                        m_type = DYN_STORE;
                        m_opcode = i_stfsr;
                        WRITE_TWO_F( RID_SINGLE );
                        FLOAT_SINGLE;
                    } else if (rd == 1) {
                        m_type = DYN_STORE;
                        m_opcode = i_stxfsr;
                        WRITE_TWO_F( RID_DOUBLE );
                        FLOAT_DOUBLE;
                    } else {
                        FAIL();
                    }
                    break;
                case 38:  // stqf
                    m_type = DYN_STORE;
                    m_opcode = i_stqf;
                    // CM imprecise: modeling stq as a st double
                    WRITE_TWO_F( RID_DOUBLE );
                    //NOQD       WRITE_TWO_F( RID_QUAD );
                    FLOAT_QUAD;
                    break;
                case 39:  // stdf
                    m_type = DYN_STORE;
                    m_opcode = i_stdf;
                    WRITE_TWO_F( RID_DOUBLE );
                    FLOAT_DOUBLE;
                    break;
                case 40:  // _
                    FAIL();
                    break;
                case 41:  // _
                    FAIL();
                    break;
                case 42:  // _
                    FAIL();
                    break;
                case 43:  // _
                    FAIL();
                    break;
                case 44:  // _
                    FAIL();
                    break;
                case 45:  // prefetch
                    m_type = DYN_PREFETCH;
                    m_futype = FU_RDPORT;
                    m_opcode = i_prefetch;
                    WRITE_TWO_I;
                    // uninitialize IRS3
                    SI_RESET( I_RS3 );
                    // CM imprecise -- need to use rd to determine prefetch type (exclusive?)
                    //                 and the implementation dependent size
                    INT_WORD;
                    break;
                case 46:  // _
                    FAIL();
                    break;
                case 47:  // _
                    FAIL();
                    break;

                    // ldf for asi space
                case 48:  // ldfa
                    m_type = DYN_LOAD;
                    m_opcode = i_ldfa;
                    READ_TWO_F( RID_SINGLE );
                    FLOAT_SINGLE;
                    SI_READASI;
                    break;
                case 49:  // _
                    FAIL();
                    break;
                case 50:  // ldqfa
                    m_type = DYN_LOAD;
                    m_opcode = i_ldqfa;
                    //NOQD       READ_TWO_F( RID_QUAD );
                    FLOAT_QUAD;
                    SI_READASI;
                    break;
                case 51:  // lddfa
                    m_type = DYN_LOAD;
                    m_opcode = i_lddfa;
                    READ_TWO_F( RID_DOUBLE );
                    SI_READASI;

                    // lddfa: this instruction either loads 8 bytes (lddfa) or with
                    // special ASIs loads 64 bytes. The %asi register could
                    // be part of abstract_pc_t, and we could determine at decode
                    // which type it is. But we don't do that.

                    // Instead, lddfa conservatively renames 8 double registers
                    // (I_RD1), and (because it may copy previous values
                    // if it only loads 8 bytes) it also reads from the same
                    // 8 registers (I_RS3).
                    I_RD1.setRtype( RID_INT );
                    I_RD1.setVanillaState( CONTAINER_BLOCK, 0 );
                    I_RD1.setRtype( RID_CONTAINER );
                    I_RD1.setVanilla( SDQ_TWIST(RID_DOUBLE, rd) );

                    I_RS3.setRtype( RID_INT );
                    I_RS3.setVanillaState( CONTAINER_BLOCK, CONTAINER_BLOCK );
                    // set the vanilla register to contain the 'base' register
                    I_RS3.setRtype( RID_CONTAINER );
                    I_RS3.setVanilla( SDQ_TWIST(RID_DOUBLE, rd) );

                    //In memop.C we change the dynamic size for the 8-byte lddfa
                    //instructions. For now, set it to be a 64-byte load.
                    FLOAT_BLOCK;
                    COMPLEX_OPERATION;
                    break;
                case 52:  // stfa
                    m_type = DYN_STORE;
                    m_opcode = i_stfa;
                    WRITE_TWO_F( RID_SINGLE );
                    FLOAT_SINGLE;
                    SI_READASI;
                    FETCH_BARRIER;
                    break;
                case 53:  // _
                    FAIL();
                    break;
                case 54:  // stqfa
                    m_type = DYN_STORE;
                    m_opcode = i_stqfa;
                    // CM imprecise: modeling stq as a st double
                    WRITE_TWO_F( RID_DOUBLE );
                    //NOQD       WRITE_TWO_F( RID_QUAD );
                    FLOAT_QUAD;
                    SI_READASI;
                    FETCH_BARRIER;
                    break;
                case 55:  // stdfa
                    m_type = DYN_STORE;
                    m_opcode = i_stdfa;
                    WRITE_TWO_F( RID_DOUBLE );
                    SI_READASI;
                    FETCH_BARRIER;
                    // set the vanilla state modifier to contain which container type this is
                    I_RS3.setRtype( RID_INT );
                    I_RS3.setVanillaState( CONTAINER_BLOCK, CONTAINER_BLOCK );
                    // set the vanilla register to contain the 'base' register
                    I_RS3.setRtype( RID_CONTAINER );
                    I_RS3.setVanilla( SDQ_TWIST(RID_DOUBLE, rd) );
                    // We decode stdfa conservatively (it waits for 8 double registers
                    // even if it only stores 2)
                    FLOAT_BLOCK;
                    break;

                case 56:  // _
                    FAIL();
                    break;
                case 57:  // _
                    FAIL();
                    break;
                case 58:  // _
                    FAIL();
                    break;
                case 59:  // _
                    FAIL();
                    break;
                case 60:  // casa
                    m_type = DYN_ATOMIC;
                    m_futype = FU_WRPORT;
                    m_opcode = i_casa;
                    SI_INT_RID( rd, I_RD1 );
                    SI_INT_RID( rs1, I_RS1 );
                    SI_INT_RID( rs2, I_RS2 );
                    SI_INT_RID( rd, I_RS3 );
                    INT_WORD;
                    SI_READASI;
                    FETCH_BARRIER;
                    COMPLEX_OPERATION;
                    //  isImmediate is used for address computation:
                    //     set to true, and set immediate value to zero
                    setFlag(SI_ISIMMEDIATE, true );
                    m_imm = 0;
                    break;
                case 61:  // prefetcha
                    m_type = DYN_PREFETCH;
                    m_futype = FU_RDPORT;
                    m_opcode = i_prefetcha;
                    WRITE_TWO_I;
                    // uninitialize IRS3
                    SI_RESET( I_RS3 );
                    // CM imprecise -- need to use rd to determine prefetch type (exclusive?)
                    //                 and the implementation dependent size
                    INT_WORD;
                    SI_READASI;
                    break;
                case 62:  // casxa
                    m_type = DYN_ATOMIC;
                    m_futype = FU_WRPORT;
                    m_opcode = i_casxa;
                    SI_INT_RID( rd, I_RD1 );
                    SI_INT_RID( rs1, I_RS1 );
                    SI_INT_RID( rs2, I_RS2 );
                    SI_INT_RID( rd, I_RS3 );
                    INT_EXTENDED;
                    SI_READASI;
                    FETCH_BARRIER;
                    COMPLEX_OPERATION;
                    //  isImmediate is used for address computation:
                    //     set to true, and set immediate value to zero
                    setFlag(SI_ISIMMEDIATE, true );
                    m_imm = 0;
                    break;

                case 63:
                    m_type   = DYN_EXECUTE;
                    m_futype = FU_NONE;
                    if ( maskBits32( inst, 0, 0 ) == 0 ) {
                        m_opcode = i_mop;
                    }
                    break;

                default:
                    FAIL();
            }
            break; /// ** break out of Op 3 instructions

        default: /// not Op 0 1 2 or 3 (impossible)
            FAIL();
    }

decode_finished:
    if ( m_type == DYN_NONE ) {
        DEBUG_OUT("ERROR: unidentifiable instruction 0x%0x\n", inst );
    }
	// This is #undefed because we first want to pass values from LL
	// to HL before setting next PC.
// #ifndef LL_DECODER
    // setNextPC();
// #endif // LL_DECODER
}
