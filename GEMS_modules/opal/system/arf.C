
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
 * FileName:  arf.C
 * Synopsis:  Implements all abstract register file classes.
 * Author:    cmauer
 * Version:   $Id: arf.C 1.16 03/07/10 18:07:47-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "checkresult.h"            // need declaration of check_result_t
#include "flow.h"
#include "arf.h"
#include "system.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/
#define ALEX_DEPERR_DEBUG 0
#define ALEX_OS_MIS 0
#define ALEX_ARCH_MIS_DEBUG 0
#define ALEX_REG_TYPE_CHK_FAIL 0
#define LXL_MISMATCH_DEBUG 0
//#define ALEX_PRINT_MISMATCHES
#define MASK_FSR_MISMATCH

#define LXL_NO_NUM_REG_CHK 1

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Abstract Register File                                                 */
/*------------------------------------------------------------------------*/

//***************************************************************************
void    abstract_rf_t::initialize( void )
{
    // initialize this register file: nothing to do!
}

#define LXL_RAT_DEBUG 0

//***************************************************************************
bool    abstract_rf_t::allocateRegister( reg_id_t &rid )
{
    // allocate a new physical register ... set it equal
    half_t logical  = getLogical( rid );
    half_t physical = m_decode_map->regAllocate(logical);
    half_t new_physical ;
    if (physical == PSEQ_INVALID_REG) {
        DEBUG_OUT("allocateRegister: unable to allocate register\n");
        print( rid );
        return (false);
    }
    m_decode_map->clearFault( logical ) ;

    if (arf_type == INT_ARF && 
            getPhysicalRF()->getSequencer()->GET_FAULT_STUCKAT() != TRANSIENT &&
            getFaultStat()->getFaultType() == RAT_FAULT && 
            logical == getFaultStat()->getFaultyReg()) {
        new_physical = getFaultStat()->injectFault(physical);

        if( new_physical != physical ) {
			if (LXL_RAT_DEBUG) DEBUG_OUT("log %d: b %d g %d\n",logical,new_physical,physical);
            m_decode_map->setFault( logical ) ;
            m_decode_map->setOldReg( physical ) ;
            physical = new_physical ;
            // We dont set the phys reg as faulty as the reg may
            // never be written into. When we KNOW that this reg is
            // written (execute of dynamic.C), then the reg is faulty
        } else {
            m_decode_map->setOldReg(PSEQ_INVALID_REG  ) ;
        }
        //m_rf->incRefCnt(physical);
    }

    // write the mapping from logical to physical
    rid.setPhysical( physical );
    m_decode_map->setMapping( logical, physical );
    return (true);
}

//***************************************************************************
bool    abstract_rf_t::freeRegister( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();
    if (physical == PSEQ_INVALID_REG) {
        DEBUG_OUT("freeRegister: error: unable to free INVALID register\n");
        return (false);
    }

    half_t logical = getLogical( rid );
    m_decode_map->regFree( logical, physical );

    // when the register is freed, clear the fault bit 
    // of the fault bit of the physical register taht is no longer
    // used
    rid.setPhysical( PSEQ_INVALID_REG );
    return (true);
}

//***************************************************************************
void    abstract_rf_t::readDecodeMap( reg_id_t &rid )
{
    DEBUG_OUT("readDecodeMap: called for undefined register type: %d\n",
            rid.getRtype() );
    return;
}

//***************************************************************************
void    abstract_rf_t::writeRetireMap( reg_id_t &rid )
{
    half_t logical  = getLogical( rid );
    half_t physical = rid.getPhysical();
    m_retire_map->setMapping( logical, physical );
}

//***************************************************************************
void    abstract_rf_t::writeDecodeMap( reg_id_t &rid )
{
    half_t logical  = getLogical( rid );
    half_t physical = rid.getPhysical();
    m_decode_map->setMapping( logical, physical );
}

//***************************************************************************
bool    abstract_rf_t::isReady( reg_id_t &rid )
{
    // zero %g0, and unmapped registers are always ready
    if (rid.isZero())
        return (true);

    half_t  physical = rid.getPhysical();
    ASSERT( physical != PSEQ_INVALID_REG );
    return (m_rf->isReady( physical ));
}

//***************************************************************************
void    abstract_rf_t::waitResult( reg_id_t &rid, dynamic_inst_t *d_instr )
{
    half_t  physical = rid.getPhysical();  
    ASSERT( physical != PSEQ_INVALID_REG );

    // wait for this physical register to wake up!
    m_rf->waitResult( physical, d_instr );
}

//***************************************************************************
bool    abstract_rf_t::regsAvailable( void )
{
    return (m_decode_map->countRegRemaining() > m_reserves);
}

//***************************************************************************
void    abstract_rf_t::check( reg_id_t &rid, pstate_t *state,
        check_result_t *result, bool init )
{
    // do nothing: default check always succeeds
}

//***************************************************************************
void    abstract_rf_t::sync( reg_id_t &rid, pstate_t *state,
        check_result_t *result, bool init )
{
    // do nothing: default check always succeeds
}

void    abstract_rf_t::updateSimicsReg( reg_id_t &rid, pstate_t *state, uint64 value)
{

}


//***************************************************************************
void    abstract_rf_t::addFreelist( reg_id_t &rid )
{
    bool success = m_decode_map->addFreelist( rid.getPhysical() );
    if (!success) {
        SIM_HALT;    
    }
}

/** print out this register mapping */
//***************************************************************************
void    abstract_rf_t::print( reg_id_t &rid )
{
    const char *rt = reg_id_t::rid_type_menomic( rid.getRtype() );
    DEBUG_OUT( "%.6s (%3.3d : %2.2d) = %3.3d   -->  %3.3d%s\n",
            rt, rid.getVanilla(), rid.getVanillaState(),
            getLogical(rid), rid.getPhysical(), (getFault(rid)?"*":" ") );
}

//***************************************************************************
half_t abstract_rf_t::renameCount( reg_id_t &rid )
{
    half_t logical = getLogical( rid );  
    return ( m_decode_map->countInFlight( logical ) );
}

//***************************************************************************
half_t  abstract_rf_t::getLogical( reg_id_t &rid )
{
    ERROR_OUT("error: abstract_rf_t: getLogical called with abstract register!\n");
    SIM_HALT;
    return 0;
}

//***************************************************************************
my_register_t abstract_rf_t::readRegister( reg_id_t &rid )
{
    // read the register as a 64-bit integer
    my_register_t result;
    result.uint_64 = m_rf->getInt( rid.getPhysical() );
    return result;
}

//***************************************************************************
void abstract_rf_t::writeRegister( reg_id_t &rid, my_register_t value )
{
    ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
    m_rf->setInt( rid.getPhysical(), value.uint_64 );  
}

//***************************************************************************
ireg_t  abstract_rf_t::readInt64( reg_id_t &rid )
{
    return (m_rf->getInt( rid.getPhysical() ));
}

/** read this register as a single precision floating point register */
//***************************************************************************
float32 abstract_rf_t::readFloat32( reg_id_t &rid )
{
    //DEBUG_OUT("error: trying to read float32 out of a non-32 bit register (type =%d)\n", rid.getRtype() );
    print( rid );
    return (-1.0);
}

//***************************************************************************
float64 abstract_rf_t::readFloat64( reg_id_t &rid )
{
    //DEBUG_OUT("error: trying to read float64 out of non-64 bit register (type=%d)\n", rid.getRtype() );
    print( rid );
    return (-1.0);
}

//***************************************************************************
void    abstract_rf_t::writeInt64( reg_id_t &rid, ireg_t value )
{
    m_rf->setInt( rid.getPhysical(), value );
}

//***************************************************************************
void    abstract_rf_t::writeFloat32( reg_id_t &rid, float32 value )
{
    DEBUG_OUT("error: trying to write float32 out of non-64 bit register (type=%d)\n", rid.getRtype() );
}

//***************************************************************************
void    abstract_rf_t::writeFloat64( reg_id_t &rid, float64 value )
{
    DEBUG_OUT("error: trying to write float64 out of non-64 bit register (type=%d)\n", rid.getRtype() );
}

//***************************************************************************
void    abstract_rf_t::initializeControl( reg_id_t &rid )
{
    DEBUG_OUT("error: trying to initializeControl in a non-control register (type=%d)\n", rid.getRtype() );
}

//***************************************************************************
void    abstract_rf_t::finalizeControl( reg_id_t &rid )
{
    DEBUG_OUT("error: trying to finalizeControl in a non-control register (type=%d)\n", rid.getRtype() );
}

//***************************************************************************
flow_inst_t *abstract_rf_t::getDependence( reg_id_t &rid )
{
    // compute a valid logical identifier 
    half_t logical = getLogical( rid );
    return ( m_last_writer[logical] );
}

//***************************************************************************
void         abstract_rf_t::setDependence( reg_id_t &rid,
        flow_inst_t *writer )
{
    half_t logical = getLogical( rid );

    // if already equal, return without making modification
    if (m_last_writer[logical] == writer)
        return;

    writer->incrementRefCount();
    if (m_last_writer[logical] != NULL) {
        m_last_writer[logical]->decrementRefCount();
        if (m_last_writer[logical]->getRefCount() == 0) {
            delete m_last_writer[logical];
        }
    }
    m_last_writer[logical] = writer;
}

//*************************************************************************
bool abstract_rf_t::getFault( reg_id_t &rid )
{
    bool regfile_f = false ;
    bool rat_f = false ;

    if( !rid.isZero() && m_rf ) {
        if( LXL_NO_NUM_REG_CHK || rid.getPhysical() < m_rf->getNumRegisters() ) {
            regfile_f = m_rf->getFault(rid.getPhysical()) ;
        }
    }

    // This is only used near end of dynamic_inst_t::Decode()
    if( (getFaultStat()->getFaultType() == RAT_FAULT) && getDecodeMap() ) {
        rat_f = getDecodeMap()->getFault(getLogical(rid)) ;
    }

    return regfile_f | rat_f ;
}

bool abstract_rf_t::isPatched( reg_id_t &rid )
{
    bool regfile_f = false ;

    if( !rid.isZero() && m_rf ) {
        if( LXL_NO_NUM_REG_CHK || rid.getPhysical() < m_rf->getNumRegisters() ) {
            regfile_f = m_rf->isPatched(rid.getPhysical()) ;
        }
    }

    return regfile_f ;
}

void abstract_rf_t::setPatched( reg_id_t &rid )
{
    if( !rid.isZero() && m_rf ) {
        if( LXL_NO_NUM_REG_CHK || rid.getPhysical() < m_rf->getNumRegisters() ) {
		//DEBUG_OUT(" setPatched: %d\n", rid.getPhysical());
            m_rf->setPatched(rid.getPhysical()) ;
        }
    }
}
void abstract_rf_t::clearPatched( reg_id_t &rid )
{

    if( !rid.isZero() && m_rf ) {
        if( LXL_NO_NUM_REG_CHK || rid.getPhysical() < m_rf->getNumRegisters() ) {
	//DEBUG_OUT(" clearing: %d\n", rid.getPhysical());
            m_rf->clearPatched(rid.getPhysical()) ;
        }
    }

}


void abstract_rf_t::setFault( reg_id_t &rid )
{
    if( !rid.isZero() && m_rf ) {
        m_rf->setFault(rid.getPhysical()) ;
        if( GSPOT_DEBUG ) { 
            DEBUG_OUT("set type %d reg %d faulty\n",rid.getRtype(), rid.getPhysical());
        }
    }
}

void abstract_rf_t::clearFault( reg_id_t &rid )
{
    if( !rid.isZero() && m_rf ) {
        if( PRADEEP_DEBUG_CLEAR ) { 
            DEBUG_OUT( "Clearing %d", rid.getPhysical() ) ;
            DEBUG_OUT( " of %s\n", reg_id_t::rid_type_menomic( rid.getRtype() ) ) ;
        }
        m_rf->clearFault(rid.getPhysical()) ;
    }

}

bool abstract_rf_t::getReady( reg_id_t &rid )
{
    bool regfile_f = false ;

    if( !rid.isZero() && m_rf ) {
        if( LXL_NO_NUM_REG_CHK || rid.getPhysical() < m_rf->getNumRegisters() ) {
            regfile_f = m_rf->getReady(rid.getPhysical()) ;
        }
    }
	return regfile_f ;
}

void abstract_rf_t::setCorrupted( reg_id_t &rid )
{
	if( !rid.isZero() && m_rf ) {
		m_rf->setCorrupted(rid.getPhysical()) ;
	}
}

void abstract_rf_t::unSetCorrupted( reg_id_t &rid )
{
	if( !rid.isZero() && m_rf ) {
			m_rf->unSetCorrupted(rid.getPhysical()) ;
	}
}

bool abstract_rf_t::isCorrupted( reg_id_t &rid )
{
	bool regfile_f = false;
	bool rat_f = false ;
	if( !rid.isZero() && m_rf ) {
		if(rid.getPhysical() < m_rf->getNumRegisters()) {
			regfile_f = m_rf->isCorrupted(rid.getPhysical()) ;
		}
	}

	// there is a special case for RAT that should be handled
	// after adding a corrupted bit for the RAT.
    // if( (getFaultStat()->getFaultType() == RAT_FAULT) && getDecodeMap() ) {
    //     rat_f = getDecodeMap()->isCorrupted(getLogical(rid)) ;
    // }

	return regfile_f | rat_f ;
}

// void abstract_rf_t::setValue( reg_id_t &rid, uint64 value )
// {
// 	if( !rid.isZero() && m_rf ) {
// 		if( PRADEEP_DEBUG_CLEAR ) { 
// 			DEBUG_OUT( "Setting value of %d", rid.getPhysical() ) ;
// 			DEBUG_OUT( " of %s\n", reg_id_t::rid_type_menomic( rid.getRtype() ) ) ;
// 		}
// 
// 		if(arf_type == FP_ARF) {
// 			m_rf->setValue((rid.getPhysical()%256), value) ;
// 			m_rf->setValue((rid.getPhysical()>>8), value) ;
// 		} else 
// 			if(arf_type != UNKNOWN_ARF) {
// 				if(rid.getPhysical() > 256)
// 					DEBUG_OUT(" arf_type=%d physical=%d\n", arf_type, rid.getPhysical());
// 				m_rf->setValue(rid.getPhysical(), value) ;
// 			}
// 	}
// }


bool abstract_rf_t::getReadFault( reg_id_t &rid )
{
	return getFault(rid) ;
    // bool regfile_f = false ;
	// if( !rid.isZero() && m_rf ) {
	// 	if( LXL_NO_NUM_REG_CHK || rid.getPhysical() < m_rf->getNumRegisters() ) {
	// 		regfile_f = m_rf->getReadFault(rid.getPhysical()) ;
	// 	}
	// }
    // return regfile_f;
}

void abstract_rf_t::setReadFault( reg_id_t &rid )
{
    if( !rid.isZero() && m_rf ) {
        m_rf->setReadFault(rid.getPhysical()) ;
    }
}

void abstract_rf_t::clearReadFault( reg_id_t &rid )
{
    if( !rid.isZero() && m_rf ) {
        m_rf->clearReadFault(rid.getPhysical()) ;
    }
}

int abstract_rf_t::getRefCnt(reg_id_t& rid) {
    if (arf_type==INT_ARF) {
        return m_rf->getRefCnt(rid.getPhysical());
    } 
    return 1;
}

void abstract_rf_t::setSyncTarget(reg_id_t &rid) {
    if( ALEX_RAT_DEBUG ) {
        DEBUG_OUT( "Setting sync target " ) ;
        DEBUG_OUT( "p%d -> ", rid.getPhysical() ) ;
        DEBUG_OUT( "r%d\n", getLogical(rid) ) ;
    }
    m_rf->setSyncTarget(rid.getPhysical(), getLogical(rid));
}

bool abstract_rf_t::checkSyncTarget(reg_id_t &rid) {
    if (!m_rf) 
        return true;


    if (LXL_NO_NUM_REG_CHK || rid.getPhysical()<m_rf->getNumRegisters()) {
        half_t this_logical = getLogical(rid) ;
        half_t this_physical = rid.getPhysical() ;
        half_t other_logical = m_rf->getSyncTarget( this_physical ) ;

        if (other_logical == PSEQ_INVALID_REG) {
            return true;
        }

        if (ALEX_RAT_DEBUG) {
            DEBUG_OUT("this logical %d ",this_logical ) ;
            DEBUG_OUT("sync logical %d ",other_logical ) ;
            DEBUG_OUT("sync physical %d \n",this_physical ) ;
        }

        if( this_logical==getFaultStat()->getFaultyReg() ||
            other_logical==getFaultStat()->getFaultyReg() ) {
            return false ;
        } else {
            return true ;
        }

    } else {
        return true;
    }
}

void abstract_rf_t::wakeDependents(reg_id_t &rid) {
    if (m_rf) {
        m_rf->wakeDependents(rid.getPhysical());
    }
}

void abstract_rf_t::setRFWakeup(bool value) {
    if (m_rf) {
        m_rf->setRFWakeup(value);
    }
}

void abstract_rf_t::clearAllFaults( )
{
    if( m_rf ) {
		m_rf->clearAllFaults();
	
    } else {
		DEBUG_OUT("m_rf is NULL\n");
		HALT_SIMULATION;
    }
    if (m_decode_map) {
		m_decode_map->clearAllFaults();
    }
    if (m_retire_map) {
		m_retire_map->clearAllFaults();
    }

}

void abstract_rf_t::rollbackReset()
{
	initialize();
}

void abstract_rf_t::checkRegReadiness(reg_id_t&rid)
{
	half_t flatreg;
	if (rid.getRtype()==RID_INT) {
		flatreg = reg_box_t::iregWindowMap( rid.getVanilla(),
												   rid.getVanillaState() );
	} else if (rid.getRtype()==RID_INT_GLOBAL) {
        flatreg = reg_box_t::iregGlobalMap( rid.getVanilla(),
											rid.getVanillaState() );
	} else {
		return;
	}

	half_t physreg = m_retire_map->getMapping( flatreg );
	if  (!m_rf->isReady( physreg )) {
		DEBUG_OUT("f %d p %d not ready!\n",flatreg,physreg);
	}
}

/*------------------------------------------------------------------------*/
/* Int / CC Register File                                                 */
/*------------------------------------------------------------------------*/

//***************************************************************************
void      arf_int_t::initialize( void )
{
    uint32 reg;
    half_t physreg;
    half_t flatreg;

    // allocate windowed integer registers
    for (uint16 cwp = 0; cwp < NWINDOWS; cwp++) {
        for (reg = 31; reg >= 8; reg --) {
            flatreg = reg_box_t::iregWindowMap( reg, cwp );
            physreg = m_decode_map->getMapping( flatreg );
            if (physreg == PSEQ_INVALID_REG) {
                // allocate a new physical register for this windowed register
                physreg = m_decode_map->regAllocate( flatreg );
                m_rf->setInt( physreg, 0 );
                m_decode_map->setMapping( flatreg, physreg );
                m_retire_map->setMapping( flatreg, physreg );

            }
            if (ALEX_RAT_DEBUG) {
                printf("logical %d ",flatreg);
                printf("physical %d \n",physreg);
            }

            m_rf->setSyncTarget(physreg,flatreg);
            // else already allocated a physical register ... nothing to do
        }
    }
}

//***************************************************************************
void      arf_int_t::readDecodeMap( reg_id_t &rid )
{
    half_t logical = reg_box_t::iregWindowMap( rid.getVanilla(),
            rid.getVanillaState() );

    half_t physical =m_decode_map->getMapping( logical ) ;
    half_t new_physical ;

    m_decode_map->clearFault( logical ) ;
    if (arf_type == INT_ARF && 
            getFaultStat()->getFaultStuckat() != TRANSIENT &&
            getFaultStat()->getFaultType() == RAT_FAULT && 
            logical == getFaultStat()->getFaultyReg()) {
        new_physical = getFaultStat()->injectFault(physical);

        if( new_physical != physical ) {
			if (LXL_RAT_DEBUG) DEBUG_OUT("read log %d: b %d g %d\n",logical,new_physical,physical);
            m_decode_map->setFault( getLogical(rid) ) ;
            m_decode_map->setOldReg( physical ) ;
            physical = new_physical ;
            // Corrupt the mapping if this entry was never written
            // before
            m_decode_map->setMapping( logical, physical );
            first_rat_err_on_read = true;
        }
    }
    rid.setPhysical( physical );
}

//***************************************************************************
half_t    arf_int_t::getLogical( reg_id_t &rid )
{
    half_t logical = reg_box_t::iregWindowMap( rid.getVanilla(),
            rid.getVanillaState() );
    return (logical);
}

//***************************************************************************
void      arf_int_t::check( reg_id_t &rid, pstate_t *state,
        check_result_t *result, bool init )
{
	ireg_t simreg = state->getIntWp( rid.getVanilla(), rid.getVanillaState() );
	if (result->update_only) {
		ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
		m_rf->setInt( rid.getPhysical(), simreg ); //TODO - Fault
	} else {
		bool need_patching = false;
		half_t flatreg = reg_box_t::iregWindowMap( rid.getVanilla(),
				rid.getVanillaState() );
		half_t physreg = m_retire_map->getMapping( flatreg );
		ireg_t myreg   = m_rf->chkGetInt( physreg );

		// get windowed integer registers
		if( LXL_NO_NUM_REG_CHK || physreg < m_rf->getNumRegisters() ) { 
			rid.setPhysical( physreg ) ;
		}

		if (myreg != simreg) {
			if (result->verbose) {
				DEBUG_OUT("%d: arf_int_t:: patch  local cwp:%d reg:%d flat %d phy %d -- 0x%0llx 0x%0llx\n", getFaultStat()->getID(),
						rid.getVanillaState(), rid.getVanilla(), flatreg, rid.getPhysical(),
						myreg, simreg);
				DEBUG_OUT(" isPatched=%d, getFault(rid)=%d \n", isPatched(rid), getFault(rid));
			}
#ifdef FAULT_CORRUPT_SIMICS
#ifdef MULTICORE_DIAGNOSIS_ONLY
		if(isPatched(rid)||  result->patch_dest) {
			need_patching = true;
			clearPatched(rid);
		} else
#endif 
			// Must change simics state from opal instead.
			// Dont mess up OPAL/SIMICS when initializing
			if((!init && getFaultStat()->getDoFaultInjection() && getFault(rid)) ) {	
				// Corrupt simics only if this reg is faulty
				getFaultStat()->setArchCycle() ;

				if (ALEX_OS_MIS && getFaultStat()->getRetPriv()) 
					DEBUG_OUT("Int OS\n");
#if defined( LXL_SNET_RECOVERY) && USE_TBFD
				if (result->recovered) {
					if (LXL_MISMATCH_DEBUG) {
						DEBUG_OUT("mismatch local cwp:%d reg:%d flat %d -- 0x%0llx 0x%0llx\n",
								rid.getVanillaState(), rid.getVanilla(), rid.getPhysical(),
								myreg, simreg);
						DEBUG_OUT("do not sync %d\n",result->do_not_sync);
					}

					if (result->do_not_sync) 
						return;

					if (physreg==PSEQ_INVALID_REG) {
						DEBUG_OUT("something is really wrong here in INT\n");
					}

					reg_mismatch_info_t &mismatch_info=result->mismatch;
					mismatch_info.reg_type=RID_INT;
					mismatch_info.logical_reg=flatreg;
					mismatch_info.physical_reg=physreg;
					mismatch_info.bad_value = myreg;
					mismatch_info.good_value = simreg;

					result->retry_mismatch=true;
#if LXL_NO_CORRUPT
					result->perfect_check = false;
					m_rf->setInt( physreg, simreg ); //TODO - Fault
					if (LXL_MISMATCH_DEBUG) {
						DEBUG_OUT("reg %d ready %d\n",rid.getPhysical(),m_rf->isReady(physreg));
					}

					if (GET_FAULT_TYPE() == RAT_FAULT) {                    
						uint16 target = m_rf->getSyncTarget(physreg);
						if (!((target==PSEQ_INVALID_REG) || 
									target==getLogical(rid))) {
							return;
						}
					}
					if (LXL_MISMATCH_DEBUG) {
						DEBUG_OUT("clear fault for reg %d\n",rid.getPhysical());
					}

					clearFault(rid);
					return;
#else //LXL_NO_CORRUPT
					state->setIntWp( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
					result->perfect_check = true ;    // Not true, but we mimic it

					if (GET_FAULT_TYPE() == RAT_FAULT) {                    
						uint16 target = m_rf->getSyncTarget(physreg);
						if (!((target==PSEQ_INVALID_REG) || 
									target==getLogical(rid))) {
							return;
						}
					}

					clearFault(rid);
					return;
#endif  // LXL_NO_CORRUPT
				}
#endif //defined( LXL_SNET_RECOVERY) && USE_TBFD

				if (ALEX_ARCH_MIS_DEBUG) 
					DEBUG_OUT("Int Mis on %d\n", rid.getPhysical() ); 

				result->perfect_check = true ;    // Not true, but we mimic it

				// LXL: use this as template
				// Prad : This means that the retiring register has fault and is used
#if defined( LXL_SNET_RECOVERY) && USE_TBFD
				if (!result->retry_mismatch && getReadFault(rid))
#else
				if (getReadFault(rid) )
#endif
				{
					if( GSPOT_DEBUG ) {
						DEBUG_OUT( "Before changing, simreg = %llx, myreg = %llx\n", simreg, myreg ) ;
					}
					state->setIntWp( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
					if(getReadFault(rid))
						clearReadFault(rid);
					getFaultStat()->setArchInuseCycle() ;

					if( GSPOT_DEBUG ) {
						ireg_t simregnew = state->getIntWp( rid.getVanilla(), rid.getVanillaState() );
						DEBUG_OUT( "After changing, simreg = %llx\n", simregnew ) ;
					}
				} 

				if (getFaultStat()->getFaultType() == RAT_FAULT) {
					uint16 target = m_rf->getSyncTarget(physreg);
					if ((target==PSEQ_INVALID_REG) || 
							target!=getLogical(rid)) {
						state->setIntWp( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
					}
				}

#ifdef ALEX_PRINT_MISMATCHES
				FAULT_RET_OUT("%d:",GET_RET_INST());
				FAULT_RET_OUT("i%d:",flatreg);
				FAULT_RET_OUT("%llx:",myreg);
				FAULT_RET_OUT("%llx\n",simreg);
#endif

			} else {
				if (getFaultStat()->getFaultType() == RAT_FAULT) {
					uint16 target = m_rf->getSyncTarget(physreg);
					if ((target==PSEQ_INVALID_REG) || 
							target==getLogical(rid)) {
						// correct target, copy from func
						result->perfect_check = false;
						m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
					} else {
#if defined( LXL_SNET_RECOVERY) && USE_TBFD
						if (LXL_MISMATCH_DEBUG && result->recovered) {
							DEBUG_OUT("no sync, target is %d\n",target);
						}
#endif
					}
				} else {
#if ALEX_REG_TYPE_CHK_FAIL
					DEBUG_OUT("check int fail\n");
#endif //ALEX_REG_TYPE_CHK_FAIL

					result->perfect_check = false;
					m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
				}
			}
#else  //FAULT_CORRUPT_SIMICS
			result->perfect_check = false;
			m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
#endif //FAULT_CORRUPT_SIMICS
		} else {
			// myreg == simreg
			// If no mismatch, then the fault is no longer a fault, clear them

#if defined(LXL_SNET_RECOVERY) && USE_TBFD //&& (LXL_NO_CORRUPT)
			if (result->recovered) {
				if (GET_FAULT_TYPE() == RAT_FAULT) {
					uint16 target = m_rf->getSyncTarget(physreg);

					if (target!=PSEQ_INVALID_REG && target!=getLogical(rid))
						return;
				} 
				clearFault(rid);
				clearReadFault(rid);

			}
#endif
		}


#ifdef FAULT_CORRUPT_SIMICS
#if MULTICORE_DIAGNOSIS_ONLY
		if(need_patching) {
			if( GSPOT_DEBUG ) {
				DEBUG_OUT( "Before changing, simreg = %x, myreg = %x\n", simreg, myreg ) ;
			}
			state->setIntWp( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
			getFaultStat()->setArchInuseCycle() ;

			if( GSPOT_DEBUG ) {
				ireg_t simregnew = state->getIntWp( rid.getVanilla(), rid.getVanillaState() );
				DEBUG_OUT( "After changing, simreg = %x\n", simregnew ) ;
			}
		}
#endif 
#endif

	}
}

//***************************************************************************
void arf_int_t::sync( reg_id_t &rid, pstate_t *state,
        check_result_t *result, bool init )
{
	// get windowed integer registers
	ireg_t simreg = state->getIntWp( rid.getVanilla(), rid.getVanillaState() );
	{
		bool need_patching = false;
		half_t flatreg = reg_box_t::iregWindowMap( rid.getVanilla(),
				rid.getVanillaState() );
		half_t physreg = m_retire_map->getMapping( flatreg );
		ireg_t myreg   = m_rf->chkGetInt( physreg );

		if( LXL_NO_NUM_REG_CHK || physreg < m_rf->getNumRegisters() ) { 
			rid.setPhysical( physreg ) ;
		}

		if (myreg != simreg) {
			if (result->verbose) {
				DEBUG_OUT("%d: patch  local cwp:%d reg:%d flat %d phy %d -- 0x%0llx 0x%0llx\n", getFaultStat()->getID(),
						rid.getVanillaState(), rid.getVanilla(), flatreg, rid.getPhysical(),
						myreg, simreg);
				DEBUG_OUT(" isPatched=%d, getFault(rid)=%d \n", isPatched(rid), getFault(rid));
			}
#ifdef FAULT_CORRUPT_SIMICS
			// Must change simics state from opal instead.
			// Dont mess up OPAL/SIMICS when initializing
			if((!init && getFaultStat()->getDoFaultInjection() && (getFault(rid)||getReadFault(rid))) ) {	
				// Corrupt simics only if this reg is faulty
				getFaultStat()->setArchCycle() ;

				if( GSPOT_DEBUG ) {
					DEBUG_OUT( "Before changing, simreg = %x, myreg = %x\n", simreg, myreg ) ;
				}
				state->setIntWp( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
				if(getReadFault(rid)) {
					clearReadFault(rid);
				}
				getFaultStat()->setArchInuseCycle() ;

				if( GSPOT_DEBUG ) {
					ireg_t simregnew = state->getIntWp( rid.getVanilla(), rid.getVanillaState() );
					DEBUG_OUT( "After changing, simreg = %x\n", simregnew ) ;
				}

				if (getFaultStat()->getFaultType() == RAT_FAULT) {
					uint16 target = m_rf->getSyncTarget(physreg);
					if ((target==PSEQ_INVALID_REG) || 
							target!=getLogical(rid)) {
						state->setIntWp( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
					}
				}

			} else {
				if (getFaultStat()->getFaultType() == RAT_FAULT) {
					uint16 target = m_rf->getSyncTarget(physreg);
					if ((target==PSEQ_INVALID_REG) || 
							target==getLogical(rid)) {
						// correct target, copy from func
						result->perfect_check = false;
						m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
					} else {
					}
				} else {
					result->perfect_check = false;
					m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
				}
			}
#else 
			result->perfect_check = false;
			m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
#endif //FAULT_CORRUPT_SIMICS
		} else {
			// myreg == simreg
			// If no mismatch, then the fault is no longer a fault, clear them
		}
	}
}

void arf_int_t::updateSimicsReg(reg_id_t &rid, pstate_t *state, uint64 value)
{
	state->setIntWp( rid.getVanilla(), rid.getVanillaState(), value) ;
}
	

void arf_int_t::rollbackReset()
{
	m_decode_map->Reset();
	m_retire_map->Reset();
	initialize();
}

//***************************************************************************
void      arf_int_global_t::initialize( void )
{
    uint32 reg;
    half_t physreg;
    half_t flatreg;

    // allocate all 4 global registers sets
    for (uint16 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset++) {
        for (reg = 1; reg < 8; reg ++) {
            flatreg = reg_box_t::iregGlobalMap( reg, gset );
            physreg = m_decode_map->getMapping( flatreg );
            if (physreg == PSEQ_INVALID_REG) {
                // allocate a new physical register for this global register
                physreg = m_decode_map->regAllocate( flatreg );
                m_rf->setInt( physreg, 0 );
                m_decode_map->setMapping( flatreg, physreg );
                m_retire_map->setMapping( flatreg, physreg );
            }
            if (ALEX_RAT_DEBUG) {
                printf("logical %d ",flatreg);
                printf("physical %d \n",physreg);
            }
            m_rf->setSyncTarget(physreg,flatreg);
        }
    }

    // special case for global zero register (%g0)
    flatreg = reg_box_t::iregGlobalMap( 0, 0 );
    physreg = m_decode_map->getMapping( flatreg );
    if (physreg == PSEQ_INVALID_REG) {
        physreg = m_decode_map->regAllocate( flatreg );
        m_rf->setInt( physreg, 0 );
        flatreg = reg_box_t::iregGlobalMap( 0, 0 );
        m_decode_map->setMapping( flatreg, physreg );
        m_retire_map->setMapping( flatreg, physreg );


    }

    if (ALEX_RAT_DEBUG) {
        printf("logical %d ",flatreg);
        printf("physical %d ",physreg);
    }
    m_rf->setSyncTarget(physreg,flatreg);

}

//***************************************************************************
void     arf_int_global_t::readDecodeMap( reg_id_t &rid )
{
    half_t logical = reg_box_t::iregGlobalMap( rid.getVanilla(),
            rid.getVanillaState() );
    half_t physical =m_decode_map->getMapping( logical ) ;

    half_t new_physical ;

    m_decode_map->clearFault( logical ) ;
    if (arf_type == INT_ARF && 
            getPhysicalRF()->getSequencer()->GET_FAULT_STUCKAT() != TRANSIENT &&
            getFaultStat()->getFaultType() == RAT_FAULT && 
            logical == getFaultStat()->getFaultyReg()) {
        new_physical = getFaultStat()->injectFault(physical);

        if( new_physical != physical ) {
            m_decode_map->setFault( getLogical(rid) ) ;
            m_decode_map->setOldReg( physical ) ;
            physical = new_physical ;
            // Corrupt the mapping if this entry was never written
            // before
            m_decode_map->setMapping( logical, physical );
            first_rat_err_on_read = true;
        } 
    }

    rid.setPhysical( physical );
}

//***************************************************************************
half_t   arf_int_global_t::getLogical( reg_id_t &rid )
{
    half_t logical = reg_box_t::iregGlobalMap( rid.getVanilla(),
            rid.getVanillaState() );
    return (logical);
}

//***************************************************************************
void     arf_int_global_t::writeInt64( reg_id_t &rid, ireg_t value )
{
    // writes to %g0 do nothing
    if (rid.getVanilla() == 0)
        return;
    m_rf->setInt( rid.getPhysical(), value );
}

//***************************************************************************
void     arf_int_global_t::check( reg_id_t &rid, pstate_t *state,
        check_result_t *result, bool init )
{
    // if the zero register (%g0) ... don't check
    if ( rid.getVanilla() == 0 ) {
        return;
    }

    // get global integer registers
    ireg_t simreg = state->getIntGlobal( rid.getVanilla(), rid.getVanillaState() );
    if (result->update_only) {
        ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
        m_rf->setInt( rid.getPhysical(), simreg ); //TODO - Fault
    } else {
	half_t orig_phys_reg = rid.getPhysical();
	bool need_patching = false;
        half_t flatreg = reg_box_t::iregGlobalMap( rid.getVanilla(),
                rid.getVanillaState() );
        half_t physreg = m_retire_map->getMapping( flatreg );
        ireg_t myreg   = m_rf->chkGetInt( physreg );

        if( LXL_NO_NUM_REG_CHK || rid.getPhysical() < m_rf->getNumRegisters() ) {
            rid.setPhysical( physreg ) ;
        }

	if (myreg != simreg) {
		if (result->verbose) {
			DEBUG_OUT("patch  gset:%d reg:%d flat %d phy %d -- 0x%0llx 0x%0llx\n",
					rid.getVanillaState(), rid.getVanilla(), flatreg, rid.getPhysical(),
					myreg, simreg);
			DEBUG_OUT(" isPatched=%d, getFault=%d\n", isPatched(rid), getFault(rid));
		}
#ifdef FAULT_CORRUPT_SIMICS
#if MULTICORE_DIAGNOSIS_ONLY
		if(isPatched(rid) ||  result->patch_dest) {
			need_patching = true;
			if(isPatched(rid))
				clearPatched(rid);
		} else

#endif
			// Must change simics state from opal instead.
			if(( !init && getFaultStat()->getDoFaultInjection() && getFault(rid))) {    // Dont mess up when initing
				getFaultStat()->setArchCycle() ;
				if (ALEX_OS_MIS && getFaultStat()->getRetPriv()) DEBUG_OUT("Global OS\n");
#if defined( LXL_SNET_RECOVERY) && USE_TBFD
				if (result->recovered) {
					if (LXL_MISMATCH_DEBUG) {
						FAULT_OUT("mismatch  gset:%d reg:%d flat %d -- 0x%0llx 0x%0llx\n",
								rid.getVanillaState(), rid.getVanilla(), physreg,
								myreg, simreg);
					}

					if (result->do_not_sync) 
						return;

					if (physreg==PSEQ_INVALID_REG) {
						DEBUG_OUT("something is really wrong here in INT_GLOBAL\n");
					}


					reg_mismatch_info_t &mismatch_info=result->mismatch;
					mismatch_info.reg_type=RID_INT_GLOBAL;
					mismatch_info.logical_reg=flatreg;
					mismatch_info.physical_reg=physreg;
					mismatch_info.bad_value = myreg;
					mismatch_info.good_value = simreg;

					result->retry_mismatch=true;
#if LXL_NO_CORRUPT

					result->perfect_check = false;
					m_rf->setInt( physreg, simreg ); 

					if (GET_FAULT_TYPE() == RAT_FAULT) {                    
						uint16 target = m_rf->getSyncTarget(physreg);
						if (!((target==PSEQ_INVALID_REG) || 
									target==getLogical(rid))) {
							return;
						}
					}
					clearFault(rid);
					return;
#else
					state->setIntGlobal( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
					result->perfect_check = true ;    // Not true, but we mimic it
					if (GET_FAULT_TYPE() == RAT_FAULT) {
						uint16 target = m_rf->getSyncTarget(physreg);
						if (!((target==PSEQ_INVALID_REG) || 
									target==getLogical(rid))) {
							return;
						}
					}
					clearFault(rid);
					return;
#endif //LXL_NO_CORRUPT
				}
#endif //defined( LXL_SNET_RECOVERY) && USE_TBFD
				if (ALEX_ARCH_MIS_DEBUG) DEBUG_OUT("Global Mis %d\n", rid.getPhysical() ) ;

				result->perfect_check = true ;

#if defined(LXL_SNET_RECOVERY) && USE_TBFD
				if (!result->retry_mismatch && getReadFault(rid)) 
#else
				if (getReadFault(rid)) 
#endif
				{
					if( GSPOT_DEBUG ) DEBUG_OUT( "Before chaning, simreg = %x\n", simreg ) ;

					state->setIntGlobal( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
					getFaultStat()->setArchInuseCycle() ;

					if(getReadFault(rid))
						clearReadFault(rid);

					if( GSPOT_DEBUG ) {
						ireg_t simregnew = state->getIntGlobal( rid.getVanilla(), rid.getVanillaState() );
						DEBUG_OUT( "After changing, simreg = %x\n", simregnew ) ;
					}
				}

				if (getFaultStat()->getFaultType() == RAT_FAULT) {
					uint16 target = m_rf->getSyncTarget(physreg);
					if ((target==PSEQ_INVALID_REG) || 
							target!=getLogical(rid)) {
						state->setIntGlobal( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
					}
				}
#ifdef ALEX_PRINT_MISMATCHES
				FAULT_RET_OUT("%d:",GET_RET_INST());
				FAULT_RET_OUT("g%d:",flatreg);
				FAULT_RET_OUT("%llx:",myreg);
				FAULT_RET_OUT("%llx\n",simreg);
#endif
			} else {
				if (getFaultStat()->getFaultType() == RAT_FAULT) {
					uint16 target = m_rf->getSyncTarget(physreg);
					if ((target==PSEQ_INVALID_REG) || 
							target==getLogical(rid)) {
						result->perfect_check = false;
						m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
					} 

				} else {
#if ALEX_REG_TYPE_CHK_FAIL
					DEBUG_OUT("check global fail\n");
#endif
					result->perfect_check = false;
					m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
				}
			}
#else
		result->perfect_check = false;
		m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
#endif
	} else {
		// myreg == simreg
		// If no mismatch, then the fault is no longer a fault, clear them
		//clearFault(rid);
		//clearReadFault(rid);
#if defined(LXL_SNET_RECOVERY) && USE_TBFD//&& LXL_NO_CORRUPT
		if (result->recovered) {
			if (GET_FAULT_TYPE() == RAT_FAULT) {
				uint16 target = m_rf->getSyncTarget(physreg);

				// If aliasing is known, do NOT clear fault!!
				if (target!=PSEQ_INVALID_REG && target!=getLogical(rid))
					return;
			} 
			clearFault(rid);
			clearReadFault(rid);

		}
#endif
	}

#ifdef FAULT_CORRUPT_SIMICS
#if MULTICORE_DIAGNOSIS_ONLY

	if(need_patching) {
		if( GSPOT_DEBUG ) DEBUG_OUT( "Before chaning, simreg = %x\n", simreg ) ;

		state->setIntGlobal( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
		getFaultStat()->setArchInuseCycle() ;


		if( GSPOT_DEBUG ) {
			ireg_t simregnew = state->getIntGlobal( rid.getVanilla(), rid.getVanillaState() );
			DEBUG_OUT( "After changing, simreg = %x\n", simregnew ) ;
		}
	}
//	rid.setPhysical(orig_phys_reg);
#endif
#endif
    }

}

void arf_int_global_t::sync( reg_id_t &rid, pstate_t *state,
        check_result_t *result, bool init )
{
	// if the zero register (%g0) ... don't check
	if ( rid.getVanilla() == 0 ) {
		return;
	}

	// get global integer registers
	ireg_t simreg = state->getIntGlobal( rid.getVanilla(), rid.getVanillaState() );
	half_t orig_phys_reg = rid.getPhysical();
	bool need_patching = false;
	half_t flatreg = reg_box_t::iregGlobalMap( rid.getVanilla(),
			rid.getVanillaState() );
	half_t physreg = m_retire_map->getMapping( flatreg );
	ireg_t myreg   = m_rf->chkGetInt( physreg );

	if( LXL_NO_NUM_REG_CHK || rid.getPhysical() < m_rf->getNumRegisters() ) {
		rid.setPhysical( physreg ) ;
	}

	if (myreg != simreg) {
		if (result->verbose) {
			DEBUG_OUT("patch  gset:%d reg:%d flat %d phy %d -- 0x%0llx 0x%0llx\n",
					rid.getVanillaState(), rid.getVanilla(), flatreg, rid.getPhysical(),
					myreg, simreg);
		}
#ifdef FAULT_CORRUPT_SIMICS
		// Must change simics state from opal instead.
		if(( !init && getFaultStat()->getDoFaultInjection() && (getFault(rid)||getReadFault(rid)))) {    // Dont mess up when initing
			getFaultStat()->setArchCycle() ;

			if( GSPOT_DEBUG ) DEBUG_OUT( "Before chaning, simreg = %x\n", simreg ) ;

			state->setIntGlobal( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
			getFaultStat()->setArchInuseCycle() ;

			clearReadFault(rid);

			if( GSPOT_DEBUG ) {
				ireg_t simregnew = state->getIntGlobal( rid.getVanilla(), rid.getVanillaState() );
				DEBUG_OUT( "After changing, simreg = %x\n", simregnew ) ;
			}

			if (getFaultStat()->getFaultType() == RAT_FAULT) {
				uint16 target = m_rf->getSyncTarget(physreg);
				if ((target==PSEQ_INVALID_REG) || 
						target!=getLogical(rid)) {
					state->setIntGlobal( rid.getVanilla(), rid.getVanillaState(), myreg ) ;
				}
			}
		} else {
			if (getFaultStat()->getFaultType() == RAT_FAULT) {
				uint16 target = m_rf->getSyncTarget(physreg);
				if ((target==PSEQ_INVALID_REG) || 
						target==getLogical(rid)) {
					result->perfect_check = false;
					m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
				} 

			} else {
				result->perfect_check = false;
				m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
			}
		}
#else
		result->perfect_check = false;
		m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
#endif
	} else {
		// myreg == simreg
		// If no mismatch, then the fault is no longer a fault, clear them
		//clearFault(rid);
		//clearReadFault(rid);
	}

}

void arf_int_global_t::updateSimicsReg(reg_id_t &rid, pstate_t *state, uint64 value)
{
	state->setIntGlobal( rid.getVanilla(), rid.getVanillaState(), value) ;
}


//***************************************************************************
void      arf_cc_t::initialize( void )
{
    uint32 reg;
    half_t physreg;

    // initialize the condition code registers
    //       i.e.  %ccr, %fcc0, %fcc1, %fcc2, %fcc3
    for (reg = (uint32) REG_CC_CCR; reg <= (uint32) REG_CC_FCC3; reg ++) {
        physreg = m_decode_map->getMapping( reg );
        if (physreg == PSEQ_INVALID_REG) {
            physreg = m_decode_map->regAllocate( reg );
            m_rf->setInt( physreg, 0 );
            m_decode_map->setMapping( reg, physreg );
            m_retire_map->setMapping( reg, physreg );      
        }
    }
}

//***************************************************************************
void      arf_cc_t::readDecodeMap( reg_id_t &rid )
{
    half_t logical = rid.getVanilla();
    rid.setPhysical( m_decode_map->getMapping( logical ) );
}

//***************************************************************************
half_t    arf_cc_t::getLogical( reg_id_t &rid )
{
    half_t logical = rid.getVanilla();
    return (logical);
}

//***************************************************************************
void      arf_cc_t::check( reg_id_t &rid, pstate_t *state,
        check_result_t *result, bool init )
{
    ireg_t simreg;
    int32  reg    = rid.getVanilla();
    ireg_t fsr;

    if (reg == REG_CC_CCR) {
        simreg  = state->getControl( CONTROL_CCR );
    } else {
        fsr = state->getControl( CONTROL_FSR );    
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
            return;
        }
    }
    if (result->update_only) {
        ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
        m_rf->setInt( rid.getPhysical(), simreg ); //TODO - Fault
    } else {
        half_t physreg = m_retire_map->getMapping( reg );
        ireg_t myreg   = m_rf->chkGetInt( physreg );

        if( LXL_NO_NUM_REG_CHK ||rid.getPhysical() < m_rf->getNumRegisters() ) {
            rid.setPhysical( physreg ) ;
        }

	if (simreg != myreg) {
		bool need_patching = false;
		if (result->verbose)
			DEBUG_OUT("patch  cc:%d: 0x%0llx 0x%0llx\n", reg, myreg, simreg);
#ifdef FAULT_CORRUPT_SIMICS
#if MULTICORE_DIAGNOSIS_ONLY
		if(isPatched(rid)||  result->patch_dest) {
			need_patching = true;
			clearPatched(rid);
		} else
#endif
		if((!init && getFaultStat()->getDoFaultInjection() && getFault(rid)) ) {
			// Dont mess up OPAL/SIMICS when initializing
			getFaultStat()->setArchCycle() ;
			if (ALEX_OS_MIS && getFaultStat()->getRetPriv()) DEBUG_OUT("CC OS\n");
#if defined(LXL_SNET_RECOVERY) && USE_TBFD
			if (result->recovered) {
				if (LXL_MISMATCH_DEBUG) {
					FAULT_OUT("mismatch  cc:%d: 0x%0llx 0x%0llx\n", reg, myreg, simreg);
				}

				if (result->do_not_sync) 
					return;

				reg_mismatch_info_t &mismatch_info=result->mismatch;
				mismatch_info.reg_type=RID_CC;
				mismatch_info.logical_reg=reg;
				mismatch_info.physical_reg=reg;
				mismatch_info.bad_value = myreg;
				mismatch_info.good_value = simreg;

				result->retry_mismatch=true;
				if (reg == REG_CC_CCR) {
#if LXL_NO_CORRUPT
					result->perfect_check = false;
					m_rf->chkSetInt( physreg, simreg ); 
					clearFault(rid);
					return;
#else
					state->setControl( CONTROL_CCR, myreg );
					result->perfect_check = true ;    // Not true, but we mimic it
					clearFault(rid);
					return;
#endif //LXL_NO_CORRUPT
				} else {
					// Corrupt 2 bits in fsr
					int low;
					if (reg == REG_CC_FCC0) {
						low=10;
					} else if (reg == REG_CC_FCC1) {
						low=32;
					} else if (reg == REG_CC_FCC2) {
						low=34;
					} else if (reg == REG_CC_FCC3) {
						low=36;
					} else {
						DEBUG_OUT( "arf_cc: corrupt fails: unknown rid %d\n", reg);
						return;       
					}
					fsr = (fsr & ~makeMask64(low+1,low)) | ((myreg & 0x3)<<low);
#if LXL_NO_CORRUPT
					result->perfect_check = false;
					m_rf->chkSetInt( physreg, simreg ); 
					clearFault(rid);
					return;
#else
					state->setControl( CONTROL_FSR, fsr );// Then set the control registers
					result->perfect_check = true ;    // Not true, but we mimic it
					return;
#endif //LXL_NO_CORRUPT
				}
			}
#endif //TBFD
			if (ALEX_ARCH_MIS_DEBUG) DEBUG_OUT("CC Mis %d\n", rid.getPhysical() ) ;

			result->perfect_check = true ;    // Not true, but we mimic it

#if defined(LXL_SNET_RECOVERY) && USE_TBFD
			if (!result->retry_mismatch && getReadFault(rid)) 
#else
				if (getReadFault(rid)) 
#endif
				{
					// ALEX: Need to be careful because we only want to corrupt
					// the specific bits. 
					if (reg == REG_CC_CCR) {
						state->setControl( CONTROL_CCR, myreg );
					} else {
						// Corrupt 2 bits in fsr
						int low;
						if (reg == REG_CC_FCC0) {
							low=10;
						} else if (reg == REG_CC_FCC1) {
							low=32;
						} else if (reg == REG_CC_FCC2) {
							low=34;
						} else if (reg == REG_CC_FCC3) {
							low=36;
						} else {
							DEBUG_OUT( "arf_cc: corrupt fails: unknown rid %d\n", reg);
							return;       
						}
						fsr = (fsr & ~makeMask64(low+1,low)) | ((myreg & 0x3)<<low);
						state->setControl( CONTROL_FSR, fsr );// Then set the control registers
					}
					getFaultStat()->setArchInuseCycle() ;

					if(getReadFault(rid))
						clearReadFault(rid);
				}
#ifdef ALEX_PRINT_MISMATCHES
			FAULT_RET_OUT("%d:",GET_RET_INST());
			FAULT_RET_OUT("cc%d:",reg);
			FAULT_RET_OUT("%llx:",myreg);
			FAULT_RET_OUT("%llx\n",simreg);
#endif

		} else {
#if ALEX_REG_TYPE_CHK_FAIL
			DEBUG_OUT("check cc fail\n");
#endif
			m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
			result->perfect_check = false;
		}
#else //FAULT_CORRUPT_SIMICS
		m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
		result->perfect_check = false;


#if MULTICORE_DIAGNOSIS_ONLY
		if(need_patching) {
			// ALEX: Need to be careful because we only want to corrupt
			// the specific bits. 
			if (reg == REG_CC_CCR) {
				state->setControl( CONTROL_CCR, myreg );
			} else {
				// Corrupt 2 bits in fsr
				int low;
				if (reg == REG_CC_FCC0) {
					low=10;
				} else if (reg == REG_CC_FCC1) {
					low=32;
				} else if (reg == REG_CC_FCC2) {
					low=34;
				} else if (reg == REG_CC_FCC3) {
					low=36;
				} else {
					DEBUG_OUT( "arf_cc: corrupt fails: unknown rid %d\n", reg);
					return;       
				}
				fsr = (fsr & ~makeMask64(low+1,low)) | ((myreg & 0x3)<<low);
				state->setControl( CONTROL_FSR, fsr );// Then set the control registers
			}
		}
#endif //MULTICORE_DIAGNOSIS_ONLY
#endif	//FAULT_CORRUPT_SIMICS
	} else {
            // myreg == simreg
            // If no mismatch, then the fault is no longer a fault, clear them
            clearFault(rid);
            clearReadFault(rid);
#if defined(LXL_SNET_RECOVERY) && USE_TBFD//&& LXL_NO_CORRUPT
		    if (result->recovered) {
				clearFault(rid);
				clearReadFault(rid);
			}
#endif
        }
    }	
}

//***************************************************************************
void arf_cc_t::sync( reg_id_t &rid, pstate_t *state,
        check_result_t *result, bool init )
{
	ireg_t simreg;
	int32  reg    = rid.getVanilla();
	ireg_t fsr;

	if (reg == REG_CC_CCR) {
		simreg  = state->getControl( CONTROL_CCR );
	} else {
		fsr = state->getControl( CONTROL_FSR );    
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
			return;
		}
	}
	{
		half_t physreg = m_retire_map->getMapping( reg );
		ireg_t myreg   = m_rf->chkGetInt( physreg );

		if( LXL_NO_NUM_REG_CHK ||rid.getPhysical() < m_rf->getNumRegisters() ) {
			rid.setPhysical( physreg ) ;
		}

		if (simreg != myreg) {
			bool need_patching = false;
			if (result->verbose)
				DEBUG_OUT("patch  cc:%d: 0x%0llx 0x%0llx\n", reg, myreg, simreg);
#ifdef FAULT_CORRUPT_SIMICS
			if((!init && getFaultStat()->getDoFaultInjection() && (getFault(rid) || getReadFault(rid)))) {
				// Dont mess up OPAL/SIMICS when initializing
				getFaultStat()->setArchCycle() ;

				result->perfect_check = true ;    // Not true, but we mimic it

				// ALEX: Need to be careful because we only want to corrupt
				// the specific bits. 
				if (reg == REG_CC_CCR) {
					state->setControl( CONTROL_CCR, myreg );
				} else {
					// Corrupt 2 bits in fsr
					int low;
					if (reg == REG_CC_FCC0) {
						low=10;
					} else if (reg == REG_CC_FCC1) {
						low=32;
					} else if (reg == REG_CC_FCC2) {
						low=34;
					} else if (reg == REG_CC_FCC3) {
						low=36;
					} else {
						DEBUG_OUT( "arf_cc: corrupt fails: unknown rid %d\n", reg);
						return;       
					}
					fsr = (fsr & ~makeMask64(low+1,low)) | ((myreg & 0x3)<<low);
					state->setControl( CONTROL_FSR, fsr );// Then set the control registers
				}
				getFaultStat()->setArchInuseCycle() ;

				clearReadFault(rid);

			} else {
				m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
				result->perfect_check = false;
			}
#else //FAULT_CORRUPT_SIMICS
			m_rf->chkSetInt( physreg, simreg ); //TODO - Fault
			result->perfect_check = false;

#endif	//FAULT_CORRUPT_SIMICS
		} else {
			// myreg == simreg
			// If no mismatch, then the fault is no longer a fault, clear them
			clearFault(rid);
			clearReadFault(rid);
		}
	}	
}



void arf_cc_t::updateSimicsReg(reg_id_t & rid, pstate_t *state, uint64 value)
{
	int32  reg    = rid.getVanilla();
    	ireg_t fsr;
	if (reg == REG_CC_CCR) {
		state->setControl( CONTROL_CCR, value);
	} else {
		// Corrupt 2 bits in fsr
		int low;
		if (reg == REG_CC_FCC0) {
			low=10;
		} else if (reg == REG_CC_FCC1) {
			low=32;
		} else if (reg == REG_CC_FCC2) {
			low=34;
		} else if (reg == REG_CC_FCC3) {
			low=36;
		} else {
			DEBUG_OUT( "arf_cc: corrupt fails: unknown rid %d\n", reg);
			return;       
		}
		fsr = (fsr & ~makeMask64(low+1,low)) | ((value & 0x3)<<low);
		state->setControl( CONTROL_FSR, fsr );// Then set the control registers
	}

}

/*------------------------------------------------------------------------*/
/* Single Register File                                                   */
/*------------------------------------------------------------------------*/

//***************************************************************************
void      arf_single_t::initialize( void )
{
    uint32 reg;
    half_t physreg;

    // Floating point register file
    for (reg = 0; reg < MAX_FLOAT_REGS; reg++) {
        physreg = m_decode_map->getMapping( reg*2 );
        if (physreg == PSEQ_INVALID_REG) {
            // map the upper 32 bits
            physreg = m_decode_map->regAllocate( reg*2 );
            m_rf->setInt( physreg, 0 );
            m_decode_map->setMapping( reg*2, physreg );
            m_retire_map->setMapping( reg*2, physreg );
        }

        physreg = m_decode_map->getMapping( reg*2 + 1 );
        if (physreg == PSEQ_INVALID_REG) {
            // map the lower 32 bits
            physreg = m_decode_map->regAllocate( reg*2 + 1 );
            m_rf->setInt( physreg, 0 );
            m_decode_map->setMapping( reg*2 + 1, physreg );
            m_retire_map->setMapping( reg*2 + 1, physreg );
        }
    }
}

void arf_single_t::rollbackReset()
{
	m_decode_map->Reset();
	m_retire_map->Reset();
	initialize();
}

//***************************************************************************
void      arf_single_t::readDecodeMap( reg_id_t &rid )
{
    half_t logical = rid.getVanilla();
    rid.setPhysical( m_decode_map->getMapping( logical ) );
}

//***************************************************************************
half_t    arf_single_t::getLogical( reg_id_t &rid )
{
    half_t logical = rid.getVanilla();
    return (logical);
}

/** read this register as a single precision floating point register */
//***************************************************************************
float32 arf_single_t::readFloat32( reg_id_t &rid )
{
    return (m_rf->getFloat( rid.getPhysical() ));
}

//***************************************************************************
void    arf_single_t::writeFloat32( reg_id_t &rid, float32 value )
{
    m_rf->setFloat( rid.getPhysical(), value );
}

//***************************************************************************
void    arf_single_t::check( reg_id_t &rid, pstate_t *state,
        check_result_t *result, bool init )
{
    half_t reg = rid.getVanilla();
    // get the double value for the register (rounded to the nearest 2)
    freg_t fsimreg = state->getDouble( (reg / 2)*2 );
    ireg_t simreg32;
#ifdef FAULT_CORRUPT_SIMICS
    ireg_t simreglow, simreghigh ;
   bool need_patching = false;
#endif
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

#ifdef FAULT_CORRUPT_SIMICS
    simreghigh = int_ptr[1] ;
    simreglow = int_ptr[0] ;
#endif

    if (result->update_only) {
        ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
        m_rf->setInt( rid.getPhysical(), simreg32 ); //TODO - Fault
    } else {
        half_t physreg = m_retire_map->getMapping( reg );
        ireg_t myreg   = m_rf->chkGetInt( physreg );// & 0xffffffff;

        if( LXL_NO_NUM_REG_CHK ||rid.getPhysical() < m_rf->getNumRegisters() ) {
            rid.setPhysical( physreg ) ;
        }

		if(physreg == 48317) {
			DEBUG_OUT("arf_single_t reg 48317\n") ;
		}

        if( GSPOT_DEBUG ) DEBUG_OUT( "In single::check, myreg = %x, ", myreg ) ;
        if( GSPOT_DEBUG ) DEBUG_OUT( "simreg32 = %x\n", simreg32 ) ;
	if (simreg32 != myreg) {
		if (result->verbose) {
			const char *str;
			if (reg % 2 == 1)
				str = "(hi)";
			else
				str = "(lo)";
			DEBUG_OUT("patch  float reg:%d %s -- 0x%0x 0x%0llx 0x%llx\n",
					reg, str,
					physreg, myreg, simreg32);
			DEBUG_OUT("isPatched: %d\n", isPatched(rid));
		}
#ifdef FAULT_CORRUPT_SIMICS
#if MULTICORE_DIAGNOSIS_ONLY
		if(isPatched(rid)||  result->patch_dest) {
			need_patching = true;
			result->perfect_check = true ;    // Not true, but we mimic it
		} else
#endif

			// Must change simics state from opal instead.
			if((!init && getFaultStat()->getDoFaultInjection()&& getFault(rid)) ) {    // Dont mess up OPAL/SIMICS when initializing
				getFaultStat()->setArchCycle() ;
				if (ALEX_OS_MIS && getFaultStat()->getRetPriv())
					 DEBUG_OUT("Single OS\n");
#if defined(LXL_SNET_RECOVERY) && USE_TBFD
				if (result->recovered) {
					if (LXL_MISMATCH_DEBUG) {
						DEBUG_OUT("mismatch  float reg:%d -- 0x%0x 0x%0llx 0x%llx\n",
								reg,
								physreg, myreg, simreg32);
					}
					if (result->do_not_sync) 
						return;

					// ENDIAN_MATTERS
					if (ENDIAN_MATCHES) {
						if ( reg % 2 == 1 ) {
							simreghigh = myreg ; 
						} else {
							simreglow = myreg ;
						}
					} else {
						if ( reg % 2 == 1 ) {
							simreglow = myreg ;
						} else {
							simreghigh = myreg ;    
						}
					}
					freg_t simreg = (((freg_t)simreghigh)<<32) | simreglow ;

					reg_mismatch_info_t &mismatch_info=result->mismatch;
					mismatch_info.reg_type=RID_SINGLE;
					mismatch_info.logical_reg=reg;
					mismatch_info.physical_reg=physreg;
					mismatch_info.bad_value = myreg;
					mismatch_info.good_value = simreg32;

					result->retry_mismatch=true;
#if LXL_NO_CORRUPT
					result->perfect_check = false;
					m_rf->chkSetInt( physreg, simreg32 ); 
					clearFault(rid);
					return;
#else
					state->setDouble( (reg/2)*2, simreg ) ;
					result->perfect_check = true ;    // Not true, but we mimic it
					clearFault(rid);
					return;
#endif

				}
#endif //TBFD
				if (ALEX_ARCH_MIS_DEBUG) DEBUG_OUT("Single Mis %d\n", rid.getPhysical() ) ;

				result->perfect_check = true ;    // Not true, but we mimic it


#if defined(LXL_SNET_RECOVERY) && USE_TBFD
				if (!result->retry_mismatch && getReadFault(rid)) {
#else
				if (getReadFault(rid)) {
#endif

					// ENDIAN_MATTERS
					if (ENDIAN_MATCHES) {
						if ( reg % 2 == 1 ) {
							simreghigh = myreg ; 
						} else {
							simreglow = myreg ;
						}
					} else {
						if ( reg % 2 == 1 ) {
							simreglow = myreg ;
						} else {
							simreghigh = myreg ;    
						}
					}
					freg_t simreg = (((freg_t)simreghigh)<<32) | simreglow ;
					// char str1[100];
					// float d1_val ;
					// sprintf(str1, "%x",fsimreg);
					// sscanf(str1, "%x",(uint32*) &d1_val);

					if( GSPOT_DEBUG ) 
						DEBUG_OUT( "Before corrupting, old value = %llx \n", fsimreg ) ;
					if( GSPOT_DEBUG ) DEBUG_OUT( "Value to corrupt with = %x\n", simreg ) ;

					state->setDouble( (reg/2)*2, simreg ) ;

					freg_t simregnew = state->getDouble( (reg/2)*2 );
					if( GSPOT_DEBUG ) {
						DEBUG_OUT( "After corrupting, new value = %llx\n", simregnew ) ;
					}
					getFaultStat()->setArchInuseCycle() ;
					clearReadFault(rid);

					// char str2[100];
					// float d2_val = -1;
					// sprintf(str2, "%x",simregnew);
					// sscanf(str2, "%x",(uint32*) &d2_val);
					// if(getFaultStat()->getFaultType() == AGEN_FAULT) {
					// 	if(fabs((float)(d1_val-d2_val)) < 0.00001) { // no error - practically the same value
					// 		cout << " No Val mis match:single: " ;
					// 		cout << d1_val << " " << d2_val << endl;
					// 		// clearFault(rid);
					// 		// clearReadFault(rid);
					// 	} else {
					// 		cout << " Unmasked : ";
					// 		cout << d1_val << " " << d2_val << endl;
					// 	}
					// 	ofstream hFile;
					// 	hFile.open("/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/perturbations.txt", fstream::app);
					// 	if(hFile.good()) 
					// 		hFile << d1_val << " " << d2_val ;
					// 	hFile.close();
					// }
				}
#ifdef ALEX_PRINT_MISMATCHES
				FAULT_RET_OUT("%d:",GET_RET_INST());
				FAULT_RET_OUT("f%d:",reg);
				FAULT_RET_OUT("%llx:",myreg);
				FAULT_RET_OUT("%llx\n",simreg32);
#endif

			} else {
#if ALEX_REG_TYPE_CHK_FAIL
				DEBUG_OUT("check single fail\n");
#endif

				result->perfect_check = false;
				m_rf->chkSetInt( physreg, simreg32 ); 
			}
#else //FAULT_CORRUPT_SIMICS
			result->perfect_check = false;
			m_rf->chkSetInt( physreg, simreg32 );
#endif//FAULT_CORRUPT_SIMICS

#ifdef FAULT_CORRUPT_SIMICS

#if MULTICORE_DIAGNOSIS_ONLY
		if(need_patching) {

			// ENDIAN_MATTERS
			if (ENDIAN_MATCHES) {
				if ( reg % 2 == 1 ) {
					simreghigh = myreg ; 
				} else {
					simreglow = myreg ;
				}
			} else {
				if ( reg % 2 == 1 ) {
					simreglow = myreg ;
				} else {
					simreghigh = myreg ;    
				}
			}
			freg_t simreg = (((freg_t)simreghigh)<<32) | simreglow ;

			if( GSPOT_DEBUG ) 
				DEBUG_OUT( "Before corrupting, old value = %llx \n", fsimreg ) ;
			if( GSPOT_DEBUG ) 
				DEBUG_OUT( "Value to corrupt with = %x\n", simreg ) ;

			state->setDouble( (reg/2)*2, simreg ) ;

		}
#endif //MULTICORE_DIAGNOSIS_ONLY
#endif// FAULT_CORRUPT_SIMICS
		} else {
			// myreg == simreg
			// If no mismatch, then the fault is no longer a fault, clear them
			//clearFault(rid);
			//clearReadFault(rid);
#if defined(LXL_SNET_RECOVERY) && USE_TBFD//&& LXL_NO_CORRUPT
			if (result->recovered) {
				clearFault(rid);
				clearReadFault(rid);
			}
#endif
		}
	}
}

void arf_single_t::updateSimicsReg(reg_id_t &rid, pstate_t *state, uint64 value)
{
	half_t reg = rid.getVanilla();
	ireg_t simreglow, simreghigh ;
	// ENDIAN_MATTERS
	if (ENDIAN_MATCHES) {
		if ( reg % 2 == 1 ) {
			simreghigh = value; 
		} else {
			simreglow = value;
		}
	} else {
		if ( reg % 2 == 1 ) {
			simreglow = value;
		} else {
			simreghigh = value;    
		}
	}
	freg_t simreg = (((freg_t)simreghigh)<<32) | simreglow ;

	if( GSPOT_DEBUG ) 
	if( GSPOT_DEBUG ) 
		DEBUG_OUT( "Value to corrupt with = %x\n", simreg ) ;

	state->setDouble( (reg/2)*2, simreg ) ;
}

/*------------------------------------------------------------------------*/
/* Double Register File                                                   */
/*------------------------------------------------------------------------*/

//***************************************************************************
void    arf_double_t::initialize( void )
{
    // initialization is handled by single registers
}

//***************************************************************************
bool    arf_double_t::allocateRegister( reg_id_t &rid )
{
    half_t logical = getLogical( rid );

    //   allocate 2 physical registers and concatenate the fields
    half_t reg1 = m_decode_map->regAllocate(logical);
    half_t reg2 = m_decode_map->regAllocate(logical + 1);
    if (reg1 == PSEQ_INVALID_REG || reg2 == PSEQ_INVALID_REG) {
        DEBUG_OUT("error allocating register\n");
        print( rid );
        return (false);
    }

    // check that regs will fit in a 8 bit quantity (see constructor)
    ASSERT( reg1 < 256 );
    ASSERT( reg2 < 256 );
    rid.setPhysical( (reg1 << 8) | reg2 );

    // write the mapping from logical to physical
    m_decode_map->setMapping( logical, reg1 );
    m_decode_map->setMapping( logical + 1, reg2 );
    return (true);
}

//***************************************************************************
bool    arf_double_t::freeRegister( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();
    if (physical == PSEQ_INVALID_REG) {
        DEBUG_OUT("freeRegister: error: (double) unable to free INVALID register\n");
        return (false);
    }
    //   free 2 physical registers by unconcatenating the fields
    half_t logical = getLogical( rid );
    half_t reg1    = (physical >> 8) & 0xff;
    half_t reg2    =  physical & 0xff;
    m_decode_map->regFree( logical, reg1 );
    m_decode_map->regFree( logical + 1, reg2 );
    rid.setPhysical( PSEQ_INVALID_REG );
    return (true);
}

//***************************************************************************
void    arf_double_t::readDecodeMap( reg_id_t &rid )
{
    half_t logical = rid.getVanilla();
    half_t reg     = m_decode_map->getMapping( logical );    
    if ( reg == PSEQ_INVALID_REG ) {
        ERROR_OUT( "error: readDecode: (double) invalid mapping for reg:%d map:%d\n",
                logical, reg );
		DEBUG_OUT("logical is %d\n",logical);
    }
    ASSERT( reg < 256 );
    half_t physical = reg << 8;

    reg = m_decode_map->getMapping( logical + 1 );
    if ( reg == PSEQ_INVALID_REG ) {
        ERROR_OUT( "error: readDecode: (double) invalid mapping for reg:%d map:%d\n",
                (logical + 1), reg );
    }
    ASSERT( reg < 256 );

    physical = physical | reg;
    rid.setPhysical( physical );
}

//***************************************************************************
void    arf_double_t::writeRetireMap( reg_id_t &rid )
{
    half_t logical = getLogical( rid );
    half_t physical= rid.getPhysical();
    half_t reg1    = (physical >> 8) & 0xff;
    m_retire_map->setMapping( logical, reg1 );
    half_t reg2    =  physical & 0xff;
    m_retire_map->setMapping( logical + 1, reg2 );
}

//***************************************************************************
void    arf_double_t::writeDecodeMap( reg_id_t &rid )
{
    half_t logical = getLogical( rid );
    half_t physical= rid.getPhysical();
    half_t reg1    = (physical >> 8) & 0xff;
    m_decode_map->setMapping( logical, reg1 );
    half_t reg2    = physical & 0xff;
    m_decode_map->setMapping( logical + 1, reg2 );
}

//***************************************************************************
bool    arf_double_t::isReady( reg_id_t &rid )
{
    // zero %g0, and unmapped registers are always ready
    if (rid.isZero()) {
        return (true);
    }

    half_t  physical = rid.getPhysical();
    ASSERT( physical != PSEQ_INVALID_REG );
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 = physical & 0xff;
    return ( m_rf->isReady( reg1 ) &&
            m_rf->isReady( reg2 ) );
}

//***************************************************************************
void    arf_double_t::waitResult( reg_id_t &rid, dynamic_inst_t *d_instr )
{
    half_t  physical = rid.getPhysical();  
    ASSERT( physical != PSEQ_INVALID_REG );

    // case RID_DOUBLE:
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 = physical & 0xff;

    if ( !m_rf->isReady( reg1 ) ) {
        // only can wait on one result at a time --
        //    check first register -- if not ready wait on it
        //    (we will check again at wakeup (dynamic_inst_t::Schedule)
        m_rf->waitResult( reg1, d_instr );
    } else if ( !m_rf->isReady( reg2 ) ) {
        //    check second register -- if not ready wait on it
        m_rf->waitResult( reg2, d_instr );
    }
}

//***************************************************************************
half_t    arf_double_t::getLogical( reg_id_t &rid )
{
    half_t logical = rid.getVanilla();
    // and logical + 1
    return (logical);
}

//***************************************************************************
my_register_t arf_double_t::readRegister( reg_id_t &rid )
{
    // read the register as a 64-bit integer
    my_register_t result;
    result.uint_64 = readInt64( rid );
    return result;
}

//***************************************************************************
void arf_double_t::writeRegister( reg_id_t &rid, my_register_t value )
{
    ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
    writeInt64( rid, value.uint_64 );
}

//***************************************************************************
ireg_t  arf_double_t::readInt64( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();  
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    // read from 2 physical registers (both 32 bits)
    ireg_t  value = ( (m_rf->getInt( reg1 ) << 32) |
            (m_rf->getInt( reg2 ) & 0xffffffff) );
    return (value);
}

//***************************************************************************
void    arf_double_t::writeInt64( reg_id_t &rid, ireg_t value )
{
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;
    uint32 *ival = (uint32 *) &value;

    // ENDIAN_MATTERS
    if (ENDIAN_MATCHES) {
        // reg1 is high order bits
        m_rf->setInt( reg1, ival[0] );
        // reg2 is low  order bits
        m_rf->setInt( reg2, ival[1] );
    } else {
        // reg1 is high order bits
        m_rf->setInt( reg1, ival[1] );
        // reg2 is low  order bits
        m_rf->setInt( reg2, ival[0] );
    }
}

//***************************************************************************
float64 arf_double_t::readFloat64( reg_id_t &rid )
{
    half_t  physical = rid.getPhysical();  
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    // read from 2 physical registers (both 32 bits)
    ireg_t  value = ( (m_rf->getInt( reg1 ) << 32) |
            (m_rf->getInt( reg2 ) & 0xffffffff) );

    /*
       DEBUG_OUT("rf64 0x%0llx 0x%0llx = 0x%0llx\n",
       m_rf->getInt( reg1 ),
       m_rf->getInt( reg2 ),
       value);
     */

    // change a 64 bit integer into a float, with out converting it ...
    float64 fval  = *((float64 *) &value);
    return (fval);
}

/** write this register as a double precision floating point register */
//***************************************************************************
void    arf_double_t::writeFloat64( reg_id_t &rid, float64 value )
{
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;
    uint32 *ival = (uint32 *) &value;

    // ENDIAN_MATTERS
    if (ENDIAN_MATCHES) {
        // reg1 is high order bits
        m_rf->setInt( reg1, ival[0] );
        // reg2 is low  order bits
        m_rf->setInt( reg2, ival[1] );
    } else {
        // reg1 is high order bits
        m_rf->setInt( reg1, ival[1] );
        // reg2 is low  order bits
        m_rf->setInt( reg2, ival[0] );
    }
}

//***************************************************************************
void    arf_double_t::check( reg_id_t &rid, pstate_t *state,
		check_result_t *result, bool init )
{
	// get the double value from the floating point rf
	freg_t fsimreg = state->getDouble( rid.getVanilla() );
	uint32 *int_ptr = (uint32 *) &fsimreg;
	ireg_t simupper32 = 0;
	ireg_t simlower32 = 0;
#ifdef FAULT_CORRUPT_SIMICS
	bool mismatch = false ;
	bool read_mismatch = false;
		  bool need_patching = false;
#endif

	// ENDIAN_MATTERS
	if (ENDIAN_MATCHES) {
		// check target upper 32 bits
		simupper32 = int_ptr[0];
		// check target lower 32 bits
		simlower32 = int_ptr[1];
	} else {
		// check target upper 32 bits
		simupper32 = int_ptr[1];
		// check target lower 32 bits
		simlower32 = int_ptr[0];
	}

	// get the physical registers
	half_t reg1;
	half_t reg2;
	ireg_t myreg1,myreg2;

	if (result->update_only) {
		half_t phys_regs = rid.getPhysical();
		reg1 = (phys_regs >> 8) & 0xff;
		reg2 =  phys_regs & 0xff;
	} else {
		reg1 = m_retire_map->getMapping( rid.getVanilla() );
		reg2 = m_retire_map->getMapping( rid.getVanilla() + 1 );
	}

	if(reg1==48317 || reg2==48317) {
		DEBUG_OUT("arf_double_t reg 48317\n") ;
	}

	if (result->update_only) {
		ASSERT( reg1 != PSEQ_INVALID_REG );
		ASSERT( reg2 != PSEQ_INVALID_REG );
		m_rf->setInt( reg1, simupper32 ); //TODO - Fault
		m_rf->setInt( reg2, simlower32 ); //TODO - Fault
	} else {
		myreg1   = m_rf->chkGetInt( reg1 );

		if( LXL_NO_NUM_REG_CHK ||rid.getPhysical() < m_rf->getNumRegisters() ) {
			rid.setPhysical( reg1 ) ;
		}

		  if( GSPOT_DEBUG )
			  DEBUG_OUT( "In double::check, myreg1 = %x, simupper32 = %x\n", myreg1, simupper32 ) ;
		  if (myreg1 != simupper32) {
			  if (result->verbose) {
				  //if( GSPOT_DEBUG ) 
				  DEBUG_OUT("patch  float reg:%d (hi) -- 0x%0x 0x%llx 0x%llx\n",
						  rid.getVanilla(), reg1, myreg1, simupper32);
				DEBUG_OUT("isPatched: %d\n", isPatched(rid));
			  }
#ifdef FAULT_CORRUPT_SIMICS

#if MULTICORE_DIAGNOSIS_ONLY
				  if(isPatched(rid)||  result->patch_dest) {
					  need_patching = true;
					  clearPatched(rid);
					  //result->perfect_check = true ;    // Not true, but we mimic it
				  } else
#endif
					  // Must change simics state from opal instead.
					  if(( !init && getFaultStat()->getDoFaultInjection() && getFault(rid))) {    // Dont mess up when initing
						  result->perfect_check = true ;
						  mismatch = true ;

						  if (getReadFault(rid)) {
							  read_mismatch=true;
							  clearReadFault(rid);
							  getFaultStat()->setArchInuseCycle() ;
						  }

#ifdef ALEX_PRINT_MISMATCHES
						  FAULT_RET_OUT("%d:",GET_RET_INST());
						  FAULT_RET_OUT("f%d:",reg1);
						  FAULT_RET_OUT("%llx:",myreg1);
						  FAULT_RET_OUT("%llx\n",simupper32);
#endif

					  } else {
						  result->perfect_check = false;
#if ALEX_REG_TYPE_CHK_FAIL
						  DEBUG_OUT("r %u ",reg1);
						  DEBUG_OUT("myupper %u ",myreg1);
						  DEBUG_OUT("simupper %u\n",simupper32);
#endif
						  m_rf->chkSetInt( reg1, simupper32 ); //TODO - Fault
					  }
#else //FAULT_CORRUPT_SIMICS
			result->perfect_check = false;
			m_rf->chkSetInt( reg1, simupper32 ); //TODO - Fault
#endif
		} 

		myreg2  = m_rf->chkGetInt( reg2 );

		if( LXL_NO_NUM_REG_CHK ||rid.getPhysical() < m_rf->getNumRegisters() ) {
			rid.setPhysical( reg2 ) ;
		}

			  if( GSPOT_DEBUG ) DEBUG_OUT( "In double::check, myreg2 = %x, simlower32 = %x\n", myreg2, simlower32 ) ;
			  if (myreg2 != simlower32) {
				  if (result->verbose) {
					  DEBUG_OUT("patch  float reg:%d (lo) -- 0x%0x 0x%llx 0x%llx\n",
							  rid.getVanilla(), reg2, myreg2, simlower32);
					  DEBUG_OUT("isPatched: %d\n", isPatched(rid));
				  }
#ifdef FAULT_CORRUPT_SIMICS
#if MULTICORE_DIAGNOSIS_ONLY
				if(isPatched(rid) ||  result->patch_dest) {
					need_patching = true;
					clearPatched(rid);
					//result->perfect_check = true ;    // Not true, but we mimic it
				} else	
#endif
				  // Must change simics state from opal instead.
				  if(( !init && getFaultStat()->getDoFaultInjection() && getFault(rid)) ) {    // Dont mess up when initing
					  result->perfect_check = true ;
					  mismatch = true ;

					  if (getReadFault(rid)) {    
						  read_mismatch = true ;
						clearReadFault(rid);
						  getFaultStat()->setArchInuseCycle() ;
					  }

#ifdef ALEX_PRINT_MISMATCHES
				FAULT_RET_OUT("%d:",GET_RET_INST());
				FAULT_RET_OUT("f%d:",reg2);
				FAULT_RET_OUT("%llx:",myreg2);
				FAULT_RET_OUT("%llx\n",simlower32);
#endif

				//                simlower32 = myreg ;
				  } else {
#if ALEX_REG_TYPE_CHK_FAIL
				DEBUG_OUT("r %u ",reg2);
				DEBUG_OUT("mylower %u ",myreg2);
				DEBUG_OUT("simlower %u\n",simlower32);
#endif
				result->perfect_check = false;
				m_rf->chkSetInt( reg2, simlower32 ); //TODO - Fault
			}
#else
			result->perfect_check = false;
			m_rf->chkSetInt( reg2, simlower32 ); //TODO - Fault
#endif
		} 
	}

#ifdef FAULT_CORRUPT_SIMICS

		  if( mismatch ) {    // If so, we need to set the double thingy..
			  getFaultStat()->setArchCycle() ;
			  if (ALEX_OS_MIS && getFaultStat()->getRetPriv()) DEBUG_OUT("Double OS\n");
			  if (ALEX_ARCH_MIS_DEBUG) DEBUG_OUT("Double Mis\n");

#if defined(LXL_SNET_RECOVERY) && USE_TBFD
			  if (result->recovered) {
				  if (LXL_MISMATCH_DEBUG) {
					  union {
						  float d;
						  uint32 i;
					  } blah;
					  freg_t bad_float = (myreg1<<32) | myreg2;

					  FAULT_OUT( "In double::check, myreg1 = %llx, myreg2 = %llx simupper32 = %llx\n", myreg1,myreg2, fsimreg ) ;
					  blah.i=myreg2;
					  FAULT_OUT("myreg is %.6e\n",blah.d);
					  blah.i=fsimreg&0xffffffff;
					  FAULT_OUT("simreg is %.6e\n",blah.d);
					  blah.i=myreg1;
					  FAULT_OUT("myreg is %.6e\n",blah.d);
					  blah.i=fsimreg>>32&0xffffffff;
					  FAULT_OUT("simreg is %.6e\n",blah.d);
				  }
				  if (result->do_not_sync) 
					  return;

				  freg_t simreg; 
				  uint32 *int_ptr = (uint32 *) &simreg;

				  // ENDIAN_MATTERS
				  if (ENDIAN_MATCHES) {
					  // check target upper 32 bits
					  int_ptr[0] = myreg1;           //simupper32;
					  // check target lower 32 bits
					  int_ptr[1] = myreg2;           //simlower32;
				  } else {
					  // check target upper 32 bits
					  int_ptr[1] = myreg1;           //simupper32;
					  // check target lower 32 bits
					  int_ptr[0] = myreg2;           //simlower32;
				  }

				  reg_mismatch_info_t &mismatch_info=result->mismatch;
				  mismatch_info.reg_type=RID_DOUBLE;
				  mismatch_info.logical_reg=rid.getVanilla();

				  mismatch_info.physical_reg=reg1|(reg2<<8);
				  mismatch_info.bad_value = simreg;
				  mismatch_info.good_value = fsimreg;
  // 				  mismatch_info.physical_reg1=reg2;
  // 				  mismatch_info.bad_value1 = myreg2;
  // 				  mismatch_info.good_value1 = simlower32;

				  result->retry_mismatch=true;
#if LXL_NO_CORRUPT
				  result->perfect_check = false;
				  m_rf->chkSetInt( reg1, simupper32 ); 
				  m_rf->chkSetInt( reg2, simlower32 ); 
				  rid.setPhysical(reg1|(reg2<<8));
				  clearFault(rid);
				  return;
#else
				  state->setDouble( rid.getVanilla(), simreg ) ;
				  result->perfect_check = true ;    // Not true, but we mimic it
				  clearFault(rid);
				  return;
#endif
			  }
#endif //TBFD
	} else {
#if defined(LXL_SNET_RECOVERY) && USE_TBFD//&& LXL_NO_CORRUPT
		if (result->recovered) {
			clearFault(rid);
			clearReadFault(rid);
		}
#endif //TBFD
	}

#if defined(LXL_SNET_RECOVERY) && USE_TBFD
		if( !result->retry_mismatch && read_mismatch )     // If so, we need to set the double thingy..
#else
		if( read_mismatch )     // If so, we need to set the double thingy..
#endif //TBFD
		{
			freg_t simreg; 
			uint32 *int_ptr = (uint32 *) &simreg;

			// ENDIAN_MATTERS
			if (ENDIAN_MATCHES) {
				// check target upper 32 bits
				int_ptr[0] = myreg1;           //simupper32;
				// check target lower 32 bits
				int_ptr[1] = myreg2;           //simlower32;
			} else {
				// check target upper 32 bits
				int_ptr[1] = myreg1;           //simupper32;
				// check target lower 32 bits
				int_ptr[0] = myreg2;           //simlower32;
			}

			// char str1[100];
			// double d1_val ;
			// sprintf(str1, "%llx",fsimreg);
			// sscanf(str1, "%llx",(uint64*) &d1_val);
			// // float d1_val ;
			// // sprintf(str1, "%x",fsimreg);
			// // sscanf(str1, "%x",(uint32*) &d1_val);
			if( GSPOT_DEBUG )  {
				DEBUG_OUT( "Before changing, simreg = %llx\n", fsimreg ) ;
				// DEBUG_OUT( "After corrupting, new value = %lf\n", d1_val) ;
				DEBUG_OUT( "After corrupting, new value = %lf\n", fsimreg) ;

			}
			if( GSPOT_DEBUG ) 
				DEBUG_OUT( "Value to corrupt with = %x\n", simreg ) ;
			state->setDouble( rid.getVanilla(), simreg ) ;

			freg_t simregnew = state->getDouble( rid.getVanilla() ) ;
			// char str2[100];
			// double d2_val = -1;
			// sprintf(str2, "%llx",simregnew);
			// sscanf(str2, "%llx",(uint64*) &d2_val);
			// // float d2_val = -1;
			// // sprintf(str2, "%x",simregnew);
			// // sscanf(str2, "%x",(uint32*) &d2_val);
			if( GSPOT_DEBUG ) {
				DEBUG_OUT( "After corrupting, new value = %x\n", simregnew ) ;
				//DEBUG_OUT( "After corrupting, new value = %lf\n", d2_val) ;
				DEBUG_OUT( "After corrupting, new value = %lf\n", simregnew ) ;
			}
			if(getFaultStat()->getFaultType() == AGEN_FAULT) {

				// uint32 new_val32 = simregnew>>32;
				// uint32 old_val32 = fsimreg>>32;
				// float* n_val = (float*)&new_val32;
				// float* o_val = (float*)&old_val32;

	 			// FILE * hFile;
	 			// hFile = fopen("/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/perturbations.txt", "a");
	 			// if(hFile!= NULL)  {
	 	 		// 	fprintf(hFile,"%llx %f:%llx %f\n", simregnew, *n_val, fsimreg, *o_val);
	 			// }
	 			// fclose(hFile);
			 	// cout << "Compare: " << *n_val << " " << *o_val << endl;

			// 	if(fabs((double)(d1_val-d2_val)) < 0.00001) { // no error - practically the same value
			// 		cout << " No Val mis match:double: " ;
			// 		cout << d1_val << " " << d2_val << endl;
			// 		// clearFault(rid);
			// 		// clearReadFault(rid);
			// 	} else {
			// 		cout << " Unmasked " << endl;
			// 	}
			// 	ofstream hFile;
			// 	hFile.open("/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/perturbations.txt", fstream::app);
			// 	if(hFile.good()) 
			// 		hFile << d1_val << " " << d2_val;
			// 	hFile.close();

			}
		}

#if MULTICORE_DIAGNOSIS_ONLY 
			
		  if(need_patching) {
			  freg_t temp_simreg; 
			  uint32 *temp_int_ptr = (uint32 *) &temp_simreg;

			  // ENDIAN_MATTERS
			  if (ENDIAN_MATCHES) {
				  // check target upper 32 bits
				  temp_int_ptr[0] = myreg1;           //simupper32;
				  // check target lower 32 bits
				  temp_int_ptr[1] = myreg2;           //simlower32;
			  } else {
				  // check target upper 32 bits
				  temp_int_ptr[1] = myreg1;           //simupper32;
				  // check target lower 32 bits
				  temp_int_ptr[0] = myreg2;           //simlower32;
			  }

			  if( GSPOT_DEBUG )
				  DEBUG_OUT( "Before changing, simreg = %llx\n", fsimreg ) ;
			  if( GSPOT_DEBUG ) 
				  DEBUG_OUT( "Value to corrupt with = %llx\n", temp_simreg ) ;
			  state->setDouble( rid.getVanilla(), temp_simreg ) ;
		  }
		  //rid.setPhysical(reg2|(reg1<<8));
#endif


#endif //FAULT_CORRUPT_SIMICS
}


//***************************************************************************
void arf_double_t::sync( reg_id_t &rid, pstate_t *state,
			   check_result_t *result, bool init )
{
	// get the double value from the floating point rf
	freg_t fsimreg = state->getDouble( rid.getVanilla() );
	uint32 *int_ptr = (uint32 *) &fsimreg;
	ireg_t simupper32 = 0;
	ireg_t simlower32 = 0;
#ifdef FAULT_CORRUPT_SIMICS
	bool mismatch = false ;
	bool read_mismatch = false;
	bool need_patching = false;
#endif

	// ENDIAN_MATTERS
	if (ENDIAN_MATCHES) {
		// check target upper 32 bits
		simupper32 = int_ptr[0];
		// check target lower 32 bits
		simlower32 = int_ptr[1];
	} else {
		// check target upper 32 bits
		simupper32 = int_ptr[1];
		// check target lower 32 bits
		simlower32 = int_ptr[0];
	}

	// get the physical registers
	half_t reg1;
	half_t reg2;
	ireg_t myreg1,myreg2;

	reg1 = m_retire_map->getMapping( rid.getVanilla() );
	reg2 = m_retire_map->getMapping( rid.getVanilla() + 1 );

	myreg1   = m_rf->chkGetInt( reg1 );

	if( LXL_NO_NUM_REG_CHK ||rid.getPhysical() < m_rf->getNumRegisters() ) {
		rid.setPhysical( reg1 ) ;
	}

	if (myreg1 != simupper32) {
		if (result->verbose) {
			DEBUG_OUT("patch  float reg:%d (hi) -- 0x%0x 0x%llx 0x%llx\n",
					rid.getVanilla(), reg1, myreg1, simupper32);
		}
#ifdef FAULT_CORRUPT_SIMICS
		// Must change simics state from opal instead.
		if( !init && getFaultStat()->getDoFaultInjection() && (getFault(rid) || getReadFault(rid))) {    // Dont mess up when initing
			result->perfect_check = true ;
			mismatch = true ;
			read_mismatch=true;
			clearReadFault(rid);
			getFaultStat()->setArchInuseCycle() ;
		} else {
			result->perfect_check = false;
			m_rf->chkSetInt( reg1, simupper32 ); //TODO - Fault
		}
#else 
		result->perfect_check = false;
		m_rf->chkSetInt( reg1, simupper32 ); //TODO - Fault
#endif //FAULT_CORRUPT_SIMICS
	} 

	myreg2  = m_rf->chkGetInt( reg2 );

	if( LXL_NO_NUM_REG_CHK ||rid.getPhysical() < m_rf->getNumRegisters() ) {
		rid.setPhysical( reg2 ) ;
	}

	if (myreg2 != simlower32) {
		if (result->verbose) {
			DEBUG_OUT("patch  float reg:%d (lo) -- 0x%0x 0x%llx 0x%llx\n",
					rid.getVanilla(), reg2, myreg2, simlower32);
		}
#ifdef FAULT_CORRUPT_SIMICS

		// Must change simics state from opal instead.
		if(( !init && getFaultStat()->getDoFaultInjection() && (getFault(rid) || getReadFault(rid)))) {    // Dont mess up when initing
			result->perfect_check = true ;
			mismatch = true ;
			read_mismatch = true ;
			clearReadFault(rid);
			getFaultStat()->setArchInuseCycle() ;
		} else {
			result->perfect_check = false;
			m_rf->chkSetInt( reg2, simlower32 ); //TODO - Fault
		}
#else
		result->perfect_check = false;
		m_rf->chkSetInt( reg2, simlower32 ); //TODO - Fault
#endif //FAULT_CORRUPT_SIMICS
	} 

#ifdef FAULT_CORRUPT_SIMICS

	if( mismatch ) {    // If so, we need to set the double thingy..
		getFaultStat()->setArchCycle() ;
	} 

	if( read_mismatch )     // If so, we need to set the double thingy..
	{
		freg_t simreg; 
		uint32 *int_ptr = (uint32 *) &simreg;

		// ENDIAN_MATTERS
		if (ENDIAN_MATCHES) {
			// check target upper 32 bits
			int_ptr[0] = myreg1;           //simupper32;
			// check target lower 32 bits
			int_ptr[1] = myreg2;           //simlower32;
		} else {
			// check target upper 32 bits
			int_ptr[1] = myreg1;           //simupper32;
			// check target lower 32 bits
			int_ptr[0] = myreg2;           //simlower32;
		}

		if( GSPOT_DEBUG ) DEBUG_OUT( "Before changing, simreg = %x\n", fsimreg ) ;
		if( GSPOT_DEBUG ) DEBUG_OUT( "Value to corrupt with = %x\n", simreg ) ;
		state->setDouble( rid.getVanilla(), simreg ) ;

		if( GSPOT_DEBUG ) {
			freg_t simregnew = state->getDouble( rid.getVanilla() ) ;
			DEBUG_OUT( "After corrupting, new value = %x\n", simregnew ) ;
		}
	}

#endif //FAULT_CORRUPT_SIMICS
}



void    arf_double_t::updateSimicsReg( reg_id_t &rid, pstate_t *state, uint64 value)
{
	  freg_t simreg; 
	  uint32 *int_ptr = (uint32 *) &simreg;

	  // ENDIAN_MATTERS
	  if (ENDIAN_MATCHES) {
		  // check target upper 32 bits
		  int_ptr[0] = value>>32;           //simupper32;
		  // check target lower 32 bits
		  int_ptr[1] = value;           //simlower32;
	  } else {
		  // check target upper 32 bits
		  int_ptr[1] = value>>32;           //simupper32;
		  // check target lower 32 bits
		  int_ptr[0] = value;           //simlower32;
	  }

	  if( GSPOT_DEBUG ) 
		  DEBUG_OUT( "Value to corrupt with = %llx\n", simreg ) ;
	  state->setDouble( rid.getVanilla(), simreg ) ;
}


//***************************************************************************
void    arf_double_t::addFreelist( reg_id_t &rid )
{
    half_t phys = rid.getPhysical();
    half_t reg1 = (phys >> 8) & 0xff;
    half_t reg2 =  phys & 0xff;

    bool success = m_decode_map->addFreelist( reg1 );
    if (!success) {
        SIM_HALT;    
    }
    success = m_decode_map->addFreelist( reg2 );
    if (!success) {
        SIM_HALT;    
    }
}

//***************************************************************************
void    arf_double_t::print( reg_id_t &rid )
{
    const char *rt = reg_id_t::rid_type_menomic( rid.getRtype() );
    half_t phys = rid.getPhysical();
    half_t reg1 = (phys >> 8) & 0xff;
    half_t reg2 =  phys & 0xff;

	//DEBUG_OUT("I am here now\n");
    DEBUG_OUT( "%.6s (%3.3d : %2.2d) = %3.3d   -->  %3.3d %3.3d\n",
            rt, rid.getVanilla(), rid.getVanillaState(),
            getLogical(rid), reg1, reg2 );
	//DEBUG_OUT("outta here\n");

}

//*************************************************************************
bool arf_double_t::getFault( reg_id_t &rid )
{
    bool regfile_f = false ;
    bool rat_f = false ;
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
        if( LXL_NO_NUM_REG_CHK ||reg1 < m_rf->getNumRegisters() ) {
            regfile_f = m_rf->getFault(reg1) ;
        }
        if( LXL_NO_NUM_REG_CHK ||reg2 < m_rf->getNumRegisters() ) {
            regfile_f |= m_rf->getFault(reg2) ;
        }
    }
    if( getDecodeMap() ) {
        rat_f = getDecodeMap()->getFault(getLogical(rid)) ;
        rat_f |= getDecodeMap()->getFault(getLogical(rid)+1) ;
    }
    return regfile_f | rat_f ;
}

bool arf_double_t::isPatched( reg_id_t &rid )
{
    bool regfile_f = false ;
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
        if( LXL_NO_NUM_REG_CHK ||reg1 < m_rf->getNumRegisters() ) {
            regfile_f = m_rf->isPatched(reg1) ;
        }
        if( LXL_NO_NUM_REG_CHK ||reg2 < m_rf->getNumRegisters() ) {
            regfile_f |= m_rf->isPatched(reg2) ;
        }
    }
    return regfile_f;
}

void arf_double_t::setPatched( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();
	//DEBUG_OUT("Set: Physical: %d\n", rid.getPhysical());
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
        if( LXL_NO_NUM_REG_CHK ||reg1 < m_rf->getNumRegisters() ) {
		//DEBUG_OUT(" setPatched: %d\n", reg1);
            m_rf->setPatched(reg1) ;
        }
        if( LXL_NO_NUM_REG_CHK ||reg2 < m_rf->getNumRegisters() ) {
		//DEBUG_OUT(" setPatched: %d\n", reg2);
            m_rf->setPatched(reg2) ;
        }
    }
}

void arf_double_t::clearPatched( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();
	//DEBUG_OUT("Clear: Physical: %d\n", rid.getPhysical());
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
            m_rf->clearPatched(reg1) ;
            m_rf->clearPatched(reg2) ;
    }
}


void arf_double_t::setFault( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
        m_rf->setFault(reg1) ;
        m_rf->setFault(reg2) ;
    }
}

void arf_double_t::clearFault( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
        if( PRADEEP_DEBUG_CLEAR ) {
            DEBUG_OUT( "Clearing %d, %d", reg1, reg2 ) ;
            DEBUG_OUT( " of %s\n", reg_id_t::rid_type_menomic( rid.getRtype() ) ) ;
        }
        m_rf->clearFault(reg1) ;
        m_rf->clearFault(reg2) ;
    }
}

bool arf_double_t::getReady( reg_id_t &rid )
{
    bool regfile_f = false ;
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
        if( LXL_NO_NUM_REG_CHK ||reg1 < m_rf->getNumRegisters() ) {
            regfile_f = m_rf->getReady(reg1) ;
        }
        if( LXL_NO_NUM_REG_CHK ||reg2 < m_rf->getNumRegisters() ) {
            regfile_f |= m_rf->getReady(reg2) ;
        }
    }
    return regfile_f ;
}

bool arf_double_t::getReadFault( reg_id_t &rid )
{
	return getFault(rid) ;

    // bool regfile_f = false ;
    // half_t physical = rid.getPhysical();
    // half_t reg1 = (physical >> 8) & 0xff;
    // half_t reg2 =  physical & 0xff;

	// if( !rid.isZero() && m_rf ) {
	// 	if( LXL_NO_NUM_REG_CHK ||reg1 < m_rf->getNumRegisters() ) {
	// 		regfile_f = m_rf->getReadFault(reg1) ;
	// 	}
	// 	if( LXL_NO_NUM_REG_CHK ||reg2 < m_rf->getNumRegisters() ) {
	// 		regfile_f |= m_rf->getReadFault(reg2) ;
	// 	}
	// }
    // return regfile_f;
}

void arf_double_t::setReadFault( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
        m_rf->setReadFault(reg1) ;
        m_rf->setReadFault(reg2) ;
    }
}

void arf_double_t::clearReadFault( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
        m_rf->clearReadFault(reg1) ;
        m_rf->clearReadFault(reg2) ;
    }
}

void arf_double_t::setCorrupted( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
        m_rf->setCorrupted(reg1) ;
        m_rf->setCorrupted(reg2) ;
    }
}

void arf_double_t::unSetCorrupted( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
        if( PRADEEP_DEBUG_CLEAR ) {
            DEBUG_OUT( "Clearing %d, %d", reg1, reg2 ) ;
            DEBUG_OUT( " of %s\n", reg_id_t::rid_type_menomic( rid.getRtype() ) ) ;
        }
        m_rf->unSetCorrupted(reg1) ;
        m_rf->unSetCorrupted(reg2) ;
    }
}

bool arf_double_t::isCorrupted( reg_id_t &rid )
{
    bool regfile_f = false ;
    bool rat_f = false ;
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    if( !rid.isZero() && m_rf ) {
        if( LXL_NO_NUM_REG_CHK ||reg1 < m_rf->getNumRegisters() ) {
            regfile_f = m_rf->isCorrupted(reg1) ;
        }
        if( LXL_NO_NUM_REG_CHK ||reg2 < m_rf->getNumRegisters() ) {
            regfile_f |= m_rf->isCorrupted(reg2) ;
        }
    }
	// RAT special case
    // if( getDecodeMap() ) {
    //     rat_f = getDecodeMap()->isCorrupted(getLogical(rid)) ;
    //     rat_f |= getDecodeMap()->isCorrupted(getLogical(rid)+1) ;
    // }
    return regfile_f | rat_f ;
}

void arf_double_t::wakeDependents(reg_id_t &rid) {
    half_t physical = rid.getPhysical();
    half_t reg1 = (physical >> 8) & 0xff;
    half_t reg2 =  physical & 0xff;

    m_rf->wakeDependents(reg1);
    m_rf->wakeDependents(reg2);
    
}


/*------------------------------------------------------------------------*/
/* Container Register File                                                */
/*------------------------------------------------------------------------*/

//***************************************************************************
arf_container_t::~arf_container_t( void )
{
    for (int32 i = 0; i < CONTAINER_NUM_CONTAINER_TYPES; i++) {
        // if dispatch map != NULL, then this register type has been initialized
        if (m_dispatch_map[i] != NULL) {
            for (uint32 k = 0; k < m_size; k++) {
                if (m_rename_map[i][k] != NULL)
                    delete [] m_rename_map[i][k];
            }
            delete [] m_dispatch_map[i];
        }
        if (m_rename_map[i] != NULL)
            free( m_rename_map[i] );
    }
}

//***************************************************************************
void    arf_container_t::initialize( void )
{
    // container class is manually initialized as it contains references
    //      across multiple register files.
}

//***************************************************************************
bool    arf_container_t::allocateRegister( reg_id_t &rid )
{
    // DEBUG_OUT("container: alloc reg\n");
    // assign a new register handler for this register instance
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();
    uint32 index          = m_rename_index[rtype];
    m_rename_index[rtype] = (m_rename_index[rtype] + 1) % m_size;
    uint32 basevanilla    = rid.getVanilla();

    // copy the registers in dispatch into the next free register array
    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
        reg_id_t &subid    = m_rename_map[rtype][index][i];
        reg_id_t &dispatch = m_dispatch_map[rtype][i];
        abstract_rf_t *arf = dispatch.getARF();

        subid.setRtype( dispatch.getRtype() );
        subid.setVanilla( basevanilla + dispatch.getVanilla() );
        subid.setARF( arf );
        bool success = arf->allocateRegister( subid );
        if (!success) {
            // DEBUG_OUT("container: alloc fails=%d\n", i);
            return (false);
        }
    }

    /* for (uint32 i=0; i < m_dispatch_size[rtype]; i++) {
       m_rename_map[rtype][index][i].print( rid );
       }
     */
    rid.setPhysical( index );
    // DEBUG_OUT("container: end alloc=%d\n", index);
    return (true);
}

//***************************************************************************
bool    arf_container_t::freeRegister( reg_id_t &rid )
{
    // DEBUG_OUT("container: free reg\n");
    uint32 index = rid.getPhysical();
    if (index == PSEQ_INVALID_REG) {
        DEBUG_OUT("freeRegister: error: (container) unable to free INVALID register\n");
        return (false);
    }
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();

    // free the registers in the rename map
    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
        reg_id_t &subid = m_rename_map[rtype][index][i];
        subid.getARF()->freeRegister( subid );
    }

    // CM You might think its a good idea to free the physical register here.
    //    However, its not!
    // rid.setPhysical( PSEQ_INVALID_REG );
    // DEBUG_OUT("container: end free=%d\n", index);
    return (true);
}

//***************************************************************************
void    arf_container_t::readDecodeMap( reg_id_t &rid )
{
    // DEBUG_OUT("container: read decode\n");
    // assign a new register handler for this register instance
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();
    uint32 index          = m_rename_index[rtype];
    m_rename_index[rtype] = (m_rename_index[rtype] + 1) % m_size;
    uint32 basevanilla    = rid.getVanilla();

    // copy the registers in dispatch into the next free register array
    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
        reg_id_t &subid    = m_rename_map[rtype][index][i];
        reg_id_t &dispatch = m_dispatch_map[rtype][i];
        abstract_rf_t *arf = dispatch.getARF();

        subid.setRtype( dispatch.getRtype() );
        subid.setVanilla( basevanilla + dispatch.getVanilla() );
        subid.setARF( arf );
        arf->readDecodeMap( subid );
    }

    rid.setPhysical( index );
    // DEBUG_OUT("container: end alloc=%d\n", index);
}

//***************************************************************************
void    arf_container_t::writeRetireMap( reg_id_t &rid )
{
    // DEBUG_OUT("container: write retire\n");
    uint32 index          = rid.getPhysical();
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();

    // write the retire maps
    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
        reg_id_t &subid = m_rename_map[rtype][index][i];
        subid.getARF()->writeRetireMap( subid );
    }
}

//***************************************************************************
void    arf_container_t::writeDecodeMap( reg_id_t &rid )
{
    uint32 index          = rid.getPhysical();
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();

    // write the decode maps
    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
        reg_id_t &subid = m_rename_map[rtype][index][i];
        subid.getARF()->writeDecodeMap( subid );
    }
}

//***************************************************************************
bool    arf_container_t::isReady( reg_id_t &rid )
{
    // DEBUG_OUT("container: is ready\n");
    // zero %g0, and unmapped registers are always ready
    if (rid.isZero())
        return (true);

    uint32 index          = rid.getPhysical();
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();
    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
        reg_id_t &subid = m_rename_map[rtype][index][i];
        if (!subid.getARF()->isReady( subid )) {
            return (false);
        }
    }

    // DEBUG_OUT("container: ready now\n");
    return (true);
}

reg_id_t& arf_container_t::getUnreadySubId(reg_id_t &rid) 
{

    uint32 index          = rid.getPhysical();
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();
    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
        reg_id_t &subid = m_rename_map[rtype][index][i];
        if (!subid.getARF()->isReady( subid )) {
            return (subid);
        }
    }
}

//***************************************************************************
void    arf_container_t::waitResult( reg_id_t &rid, dynamic_inst_t *d_instr )
{
    // DEBUG_OUT("container: wait\n");
    uint32 index          = rid.getPhysical();
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();

    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
        reg_id_t &subid = m_rename_map[rtype][index][i];
        if (!subid.getARF()->isReady( subid )) {
            // only wait on at most one register
            subid.getARF()->waitResult( subid, d_instr );
            return;
        }
    }
}

//***************************************************************************
half_t    arf_container_t::getLogical( reg_id_t &rid )
{
    // warning: getlogical() does not really apply to container class
    half_t logical = rid.getPhysical();
    return (logical);
}

//***************************************************************************
ireg_t    arf_container_t::readInt64( reg_id_t &rid )
{
    // DEBUG_OUT("container: readint\n");
    // index contains which register array in m_rename_map to use
    uint32 index          = rid.getPhysical();
    // Vanilla is used in at retirement to select which register to write to
    uint32 selector       = rid.getSelector();
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();

    /*
       DEBUG_OUT("index=%d\n", index);
       DEBUG_OUT("rtype=%d\n", rtype);
       DEBUG_OUT("selector=%d\n", selector);
       DEBUG_OUT("size=%d\n", m_dispatch_size[rtype] );
     */

    if ( selector >= m_dispatch_size[rtype] ) {
        DEBUG_OUT( "warning: readInt64 selector out of range: %d\n",
                selector );
        return (0);
    }

    reg_id_t &subid = m_rename_map[rtype][index][selector];
    return (subid.getARF()->readInt64( subid ));
}

//***************************************************************************
void      arf_container_t::writeInt64( reg_id_t &rid, ireg_t value )
{
    // DEBUG_OUT("container: writeint\n");
    // index contains which register array in m_rename_map to use
    uint32 index          = rid.getPhysical();
    // Vanilla is used in at retirement to select which register to write to
    uint32 selector       = rid.getSelector();
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();

    if ( selector > m_dispatch_size[rtype] ) {
        ERROR_OUT( "warning: writeInt64 selector out of range: %d\n",
                selector );
        return;
    }

    /*
       DEBUG_OUT("index=%d\n", index);
       DEBUG_OUT("rtype=%d\n", rtype);
       DEBUG_OUT("selector=%d\n", selector);
     */

    reg_id_t &subid = m_rename_map[rtype][index][selector];
    subid.getARF()->writeInt64( subid, value );
}

//***************************************************************************
void    arf_container_t::initializeControl( reg_id_t &rid )
{
    // DEBUG_OUT("container: init control\n");
    // index contains which register array in m_rename_map to use
    uint32 index          = rid.getPhysical();
    // Vanilla is used in at retirement to select which register to write to
    uint32 selector       = rid.getSelector();
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();

    /*
       DEBUG_OUT("index=%d\n", index);
       DEBUG_OUT("rtype=%d\n", rtype);
       DEBUG_OUT("selector=%d\n", selector);
       DEBUG_OUT("size=%d\n", m_dispatch_size[rtype] );
     */

    if ( selector >= m_dispatch_size[rtype] ) {
        DEBUG_OUT( "warning: initializeControl selector out of range: %d\n",
                selector );
        return;
    }

    reg_id_t &subid = m_rename_map[rtype][index][selector];
    subid.getARF()->initializeControl( subid );
}

//**************************************************************************
void    arf_container_t::finalizeControl( reg_id_t &rid )
{
    // DEBUG_OUT("container: finalize control\n");
    // index contains which register array in m_rename_map to use
    uint32 index          = rid.getPhysical();
    // Vanilla is used in at retirement to select which register to write to
    uint32 selector       = rid.getSelector();
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();

    /*
       DEBUG_OUT("index=%d\n", index);
       DEBUG_OUT("rtype=%d\n", rtype);
       DEBUG_OUT("selector=%d\n", selector);
       DEBUG_OUT("size=%d\n", m_dispatch_size[rtype] );
     */

    if ( selector >= m_dispatch_size[rtype] ) {
        DEBUG_OUT( "warning: finalize control selector out of range: %d\n",
                selector );
        return;
    }

    reg_id_t &subid = m_rename_map[rtype][index][selector];
    subid.getARF()->finalizeControl( subid );
    // DEBUG_OUT("container: end finalize control\n");
}

//***************************************************************************
void      arf_container_t::check( reg_id_t &rid, pstate_t *state,
        check_result_t *result, bool init )
{
    // DEBUG_OUT("container: checkerino\n");
    // index contains which register array in m_rename_map to use
    uint32 index          = rid.getPhysical();
    ASSERT( index < m_size );
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();

    // check each register identifier in the array
    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
        //DEBUG_OUT("check: %d %d %d\n", rtype, index, i);
        reg_id_t &subid = m_rename_map[rtype][index][i];
        subid.getARF()->check( subid, state, result, init );
    }
    // DEBUG_OUT("endth\n");
}

//***************************************************************************
void    arf_container_t::addFreelist( reg_id_t &rid )
{
    // index contains which register array in m_rename_map to use
    uint32 index          = rid.getPhysical();
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();

    // check each register identifier in the array
    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
        // DEBUG_OUT("print: %d %d %d\n", rtype, index, i);
        reg_id_t &subid = m_rename_map[rtype][index][i];
        subid.getARF()->addFreelist( subid );
    }
}

//***************************************************************************
void    arf_container_t::print( reg_id_t &rid )
{
    // index contains which register array in m_rename_map to use
    uint32 index          = rid.getPhysical();
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();

    // print each register identifier in the array
    // DEBUG_OUT( "container:%d type:%d\n", index, rtype );
    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
        // DEBUG_OUT("print: %d %d %d\n", rtype, index, i);
        reg_id_t &subid = m_rename_map[rtype][index][i];
        subid.getARF()->print( subid );
    }
}

//***************************************************************************
void  arf_container_t::openRegisterType( rid_container_t rtype,
        int32 numElements )
{
    //   DEBUG_OUT("open reg\n");
    if (m_cur_type != CONTAINER_NUM_CONTAINER_TYPES) {
        ERROR_OUT("container_t: open register called while register already open.\n");
        SIM_HALT;
    }
    m_cur_type             = rtype;
    m_cur_element          = 0;
    m_max_elements         = numElements;
    m_dispatch_size[rtype] = numElements;
    m_dispatch_map[m_cur_type] = new reg_id_t[numElements];

    for ( uint32 i = 0; i < m_size; i++ ) {
        m_rename_map[m_cur_type][i] = new reg_id_t[numElements];
    }
}

//***************************************************************************
void  arf_container_t::addRegister( rid_type_t regtype, int32 offset,
        abstract_rf_t *arf )
{
    // DEBUG_OUT("add reg\n");
    if (m_cur_element == -1) {
        ERROR_OUT("container_t: addRegister called before open register.\n");
        SIM_HALT;
    }
    if (m_cur_element >= m_max_elements) {
        ERROR_OUT("container_t: addRegister called too many times %d (%d).\n",
                m_cur_element, m_max_elements);
        SIM_HALT;
    }
    m_dispatch_map[m_cur_type][m_cur_element].setRtype( regtype );
    m_dispatch_map[m_cur_type][m_cur_element].setVanilla( offset );
    m_dispatch_map[m_cur_type][m_cur_element].setARF( arf );

    m_cur_element = m_cur_element + 1;
}

//***************************************************************************
void  arf_container_t::closeRegisterType( void )
{
    //  DEBUG_OUT("close reg\n");
    if (m_cur_element != m_max_elements) {
        ERROR_OUT("container_t: addRegister called too few times %d (%d).\n",
                m_cur_element, m_max_elements);
        SIM_HALT;
    }

    /*
       for (uint32 i=0; i < m_dispatch_size[m_cur_type]; i++) {
       reg_id_t &rid = m_dispatch_map[m_cur_type][i];
       rid.getARF->print( rid );
       }
     */
    m_cur_type     = CONTAINER_NUM_CONTAINER_TYPES;
    m_cur_element  = -1;
    m_max_elements = -1;
}

/*------------------------------------------------------------------------*/
/* Control Register File                                                  */
/*------------------------------------------------------------------------*/

//***************************************************************************
void    arf_control_t::initialize( void )
{
    uint32 reg;

    // Control Register File:
    //    no mappings to set, but initialize some values.
    for ( uint32 i = 0; i < CONFIG_NUM_CONTROL_SETS; i++ ) {
        for (reg = CONTROL_PC; reg < CONTROL_NUM_CONTROL_TYPES; reg ++) {
            m_rf_array[i]->setInt( reg, 0 );
            m_rf_array[i]->setFree( reg, false );
            if (reg == CONTROL_ISREADY) {
                // control isready indicates if this group is able to be used yet.
                // the set 0 is ok at the beginning of the program, others are not.
                if (i != 0) {
                    m_rf_array[i]->setUnready( reg );
                    m_rf_array[i]->setFree( reg, false );
                }
            }
        }
    }
}

//***************************************************************************
bool    arf_control_t::allocateRegister( reg_id_t &rid )
{
    // allocate a whole set of control registers
    uint32  nextset = (m_decode_rf + 1) % m_rf_count;
    if (nextset == m_retire_rf) {
        ERROR_OUT("error: arf_control: too few control register sets to continue\n");
        ERROR_OUT("     : you may need to increase the CONFIG_NUM_CONTROL_SETS variable\n");
        return false;
    }

    if ( m_rf_array[nextset]->isReady( CONTROL_ISREADY ) ) {
        ERROR_OUT("error: arf_control: allocating set which is already active.\n");
        return false;
    }

    rid.setPhysical( nextset );
    physical_file_t *rf = m_rf_array[nextset];
    rf->setUnready( CONTROL_ISREADY );
    rf->setFree( CONTROL_ISREADY, false );
    m_decode_rf = nextset;
    return true;
}

//***************************************************************************
bool    arf_control_t::freeRegister( reg_id_t &rid )
{
    half_t  physical = rid.getPhysical();

    /* DEBUG_OUT("arf_control: freeRegister: physical=%d decode=%d retire=%d\n",
       physical, m_decode_rf, m_retire_rf );
     */
    ASSERT( physical < m_rf_count );
    physical_file_t *rf = m_rf_array[physical];
    rf->setUnready( CONTROL_ISREADY );
    rf->setFree( CONTROL_ISREADY, false );
    return (true);
}

//***************************************************************************
void    arf_control_t::readDecodeMap( reg_id_t &rid )
{
    rid.setPhysical( m_decode_rf );
}

//***************************************************************************
void    arf_control_t::writeRetireMap( reg_id_t &rid )
{
    half_t physical= rid.getPhysical();
    half_t next    = (m_retire_rf + 1) % m_rf_count;
    if ( physical != next ) {
        DEBUG_OUT( "warning: arf_control: retire map does not advance sequentially %d != %d\n", next, physical );
    }
    m_retire_rf = physical;
}

//***************************************************************************
void    arf_control_t::writeDecodeMap( reg_id_t &rid )
{
    half_t physical= rid.getPhysical();
    half_t prev    = (m_decode_rf - 1) % m_rf_count;
    if ( physical != prev ) {
        DEBUG_OUT( "warning: arf_control: decode map does not decrease sequentially %d != %d\n", prev, physical );
    }
    m_decode_rf = physical;
}

//***************************************************************************
bool    arf_control_t::isReady( reg_id_t &rid )
{
    half_t physical = rid.getPhysical();
    ASSERT( physical < m_rf_count );
    /*  DEBUG_OUT( "arf_control: isready: phys=%d %d\n", physical,
        m_rf_array[physical]->isReady( CONTROL_ISREADY ) ); */
    return ( m_rf_array[physical]->isReady( CONTROL_ISREADY ) );
}

//***************************************************************************
void    arf_control_t::waitResult( reg_id_t &rid, dynamic_inst_t *d_instr )
{
    half_t physical = rid.getPhysical();
    ASSERT( physical < m_rf_count );
    m_rf_array[physical]->waitResult( CONTROL_ISREADY, d_instr );
}

//***************************************************************************
half_t  arf_control_t::renameCount( reg_id_t &rid )
{
    // FIX: rename count not implemented for control registers.
    return 0;
}


//***************************************************************************
half_t    arf_control_t::getLogical( reg_id_t &rid )
{
    half_t logical = rid.getVanilla();
    return (logical);
}

//***************************************************************************
ireg_t  arf_control_t::readInt64( reg_id_t &rid )
{
    half_t  rset = rid.getPhysical();
    half_t  reg  = rid.getVanilla();
    return (m_rf_array[rset]->getInt( reg ));
}

//***************************************************************************
void    arf_control_t::writeInt64( reg_id_t &rid, ireg_t value )
{
    half_t  rset = rid.getPhysical();
    half_t  reg  = rid.getVanilla();

    return (m_rf_array[rset]->setInt( reg, value ));
}

//***************************************************************************
void    arf_control_t::initializeControl( reg_id_t &rid )
{
    half_t  rset = rid.getPhysical();
    // copy all of the control registers from source to dest
    half_t  prev = (rset - 1) % m_rf_count;
    if ( rid.getPhysical() == PSEQ_INVALID_REG ) {
        DEBUG_OUT("error: arf_control: initialize control fails- invalid register.\n");
        print( rid );
        return;
    }
    if ( !m_rf_array[prev]->isReady( CONTROL_ISREADY ) ) {
        DEBUG_OUT("error: arf_control: initialize control fails- prev is uninitialized.\n");
        print( rid );
        return;
    }
    physical_file_t *source = m_rf_array[prev];
    physical_file_t *dest   = m_rf_array[rset];
    for (uint32 i = 0; i < (uint32) CONTROL_NUM_CONTROL_PHYS; i++) {
        dest->setInt( i, source->getInt(i) );
    }
}

//**************************************************************************
void    arf_control_t::finalizeControl( reg_id_t &rid )
{
    // now flag the register set as 'ready'
    physical_file_t *dest   = m_rf_array[rid.getPhysical()];
    dest->setInt( CONTROL_ISREADY, 1 );  
}

//***************************************************************************
void    arf_control_t::check( reg_id_t &rid, pstate_t *state,
		check_result_t *result, bool init )
{
	// invalid control register: unable to check
	if ( rid.getVanilla() == CONTROL_UNIMPL )
		return;

	half_t  reg    = rid.getVanilla();
	ireg_t  simreg = state->getControl( reg );
#ifdef FAULT_CORRUPT_SIMICS
	bool    do_not_corrupt = true;
#endif

	if (result->update_only) {
		ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
		m_rf_array[rid.getPhysical()]->setInt( reg, simreg ); //TODO - Fault
		finalizeControl( rid );
	} else {
		ireg_t  myreg  = m_rf_array[m_retire_rf]->chkGetInt( reg );

		// Control registers have same physical num as logical num
		rid.setPhysical( reg ) ;

		// The condition code bits (FCC0-3) are renamed, so mask them off
		//   for the comparison and update if any
		if ( simreg != myreg ) {
			// certain registers are not maintained, or are register renamed
			if ( !(reg == CONTROL_PC ||
						reg == CONTROL_NPC ||
						reg == CONTROL_CCR ||
						reg == CONTROL_TICK ||
						reg == CONTROL_TICK_CMPR) ) {
#if ALEX_REG_TYPE_CHK_FAIL
				DEBUG_OUT("check control fail\n");
				DEBUG_OUT("r %u ",reg);
				DEBUG_OUT("mycon 0x%x ",myreg);
				DEBUG_OUT("simcon 0x%x\n",simreg);
#endif
				result->perfect_check = false;
#ifdef  FAULT_CORRUPT_SIMICS
				do_not_corrupt = false;
#endif

#ifdef MASK_FSR_MISMATCH
				if (reg==CONTROL_FSR) {
					if (((simreg>>23)&0x1f)==0) { //should not cause any trap
						result->perfect_check = true;
						do_not_corrupt = true;
					}
				}
#endif
			}
			if (result->verbose /*&& !do_not_corrupt*/) {
				DEBUG_OUT( "patch %%control %s -- 0x%0llx 0x%0llx\n",
						pstate_t::control_reg_menomic[reg],
						myreg, simreg );
			}

#ifdef FAULT_CORRUPT_SIMICS

			// Must change simics state from opal instead.
			if(!init && !do_not_corrupt && getFaultStat()->getDoFaultInjection()
					&& getFault(rid)) {   
				// Dont mess up OPAL/SIMICS when initializing
				getFaultStat()->setArchCycle() ;


#if defined(LXL_SNET_RECOVERY) && USE_TBFD
				if (result->recovered) {
					if (LXL_MISMATCH_DEBUG) {
						FAULT_OUT( "mismatch %%control %s -- 0x%0llx 0x%0llx\n",
								pstate_t::control_reg_menomic[reg],
								myreg, simreg );
					}

					reg_mismatch_info_t mismatch_info;
					mismatch_info.reg_type=RID_CONTROL;
					mismatch_info.logical_reg=reg;
					mismatch_info.physical_reg=reg;
					mismatch_info.bad_value = myreg;
					mismatch_info.good_value = simreg;

					result->retry_mismatch=true;
#if LXL_NO_CORRUPT
					result->perfect_check = false;
					m_rf_array[m_retire_rf]->chkSetInt( reg, simreg ); 
					clearFault(rid);
					return;
#else
					state->setControl( reg, myreg );
					result->perfect_check = true ;    // Not true, but we mimic it
					clearFault(rid);
					return;
#endif
				}
#endif
				if (ALEX_ARCH_MIS_DEBUG) DEBUG_OUT("Control Mis %d\n", rid.getPhysical() ) ;

				result->perfect_check = true ;    // Not true, but we mimic it

#ifdef ALEX_CONTROL_ARF_DEBUG // Sorry, don't want to change the Makefile
				DEBUG_OUT("corrupt %d ",reg);
				DEBUG_OUT("0x%x to ",simreg);
				DEBUG_OUT("0x%x\n",myreg);
#endif

#ifdef ALEX_PRINT_MISMATCHES
				FAULT_RET_OUT("%d:",GET_RET_INST());
				FAULT_RET_OUT("c%d:",reg);
				FAULT_RET_OUT("%llx:",myreg);
				FAULT_RET_OUT("%llx\n",simreg);
#endif
				//DEBUG_OUT("setting control to %llx\n",myreg);
				state->setControl( reg, myreg );// Then set the control registers

			} else {
				m_rf_array[m_retire_rf]->chkSetInt( reg, simreg ); //TODO - Fault
			}
#else 
			m_rf_array[m_retire_rf]->chkSetInt( reg, simreg ); //TODO - Fault
#endif
		}
		// This clearFault is special because there WILL not be
		// aliasing caused by ROB/RAT fault for control registers.
		clearFault(rid);
	}

}


//***************************************************************************
void arf_control_t::sync( reg_id_t &rid, pstate_t *state,
				check_result_t *result, bool init )
{
	// invalid control register: unable to check
	if ( rid.getVanilla() == CONTROL_UNIMPL )
		return;

	half_t  reg    = rid.getVanilla();
	ireg_t  simreg = state->getControl( reg );
#ifdef FAULT_CORRUPT_SIMICS
	bool    do_not_corrupt = true;
#endif
	ireg_t  myreg  = m_rf_array[m_retire_rf]->chkGetInt( reg );

	// Control registers have same physical num as logical num
	rid.setPhysical( reg ) ;

	// The condition code bits (FCC0-3) are renamed, so mask them off
	//   for the comparison and update if any
	if ( simreg != myreg ) {
		// certain registers are not maintained, or are register renamed
		if ( !(reg == CONTROL_PC ||
					reg == CONTROL_NPC ||
					reg == CONTROL_CCR ||
					reg == CONTROL_TICK ||
					reg == CONTROL_TICK_CMPR) ) {
			result->perfect_check = false;
#ifdef  FAULT_CORRUPT_SIMICS
			do_not_corrupt = false;
#endif

#ifdef MASK_FSR_MISMATCH
			if (reg==CONTROL_FSR) {
				if (((simreg>>23)&0x1f)==0) { //should not cause any trap
					result->perfect_check = true;
					do_not_corrupt = true;
				}
			}
#endif

		}
		if (result->verbose /*&& !do_not_corrupt*/) {
			DEBUG_OUT( "patch %%control %s -- 0x%0llx 0x%0llx\n",
					pstate_t::control_reg_menomic[reg],
					myreg, simreg );
		}

#ifdef FAULT_CORRUPT_SIMICS

		// Must change simics state from opal instead.
		if((!init && !do_not_corrupt && getFaultStat()->getDoFaultInjection()
					&& (getFault(rid) || getReadFault(rid)))) {
			// Dont mess up OPAL/SIMICS when initializing
			getFaultStat()->setArchCycle() ;

			result->perfect_check = true ;    // Not true, but we mimic it

			state->setControl( reg, myreg );// Then set the control registers
			clearReadFault(rid);

		} else {
			m_rf_array[m_retire_rf]->chkSetInt( reg, simreg ); //TODO - Fault
		}
#else 
		m_rf_array[m_retire_rf]->chkSetInt( reg, simreg ); //TODO - Fault
#endif
	}
	// This clearFault is special because there WILL not be
	// aliasing caused by ROB/RAT fault for control registers.
	clearFault(rid);
}
//***************************************************************************
bool    arf_control_t::regsAvailable( void )
{
    uint32  nextset = (m_decode_rf + 1) % m_rf_count;
    // at least one control register remaining
    return (nextset != m_retire_rf);
}

//***************************************************************************
void    arf_control_t::addFreelist( reg_id_t &rid )
{
    // control register files are unable to be leaked -- they are not
    // handled using the register map.
}

bool arf_control_t::getFault( reg_id_t &rid )
{
    bool regfile_f = false ;

    if( !rid.isZero() && (rid.getVanilla() < CONTROL_NUM_CONTROL_TYPES)) {
        regfile_f =  m_rf_array[m_retire_rf]->getFault(rid.getVanilla()) ;
    }

    return regfile_f ;
}

bool arf_control_t::isPatched( reg_id_t &rid )
{
    bool regfile_f = false ;

    if( !rid.isZero() && (rid.getVanilla() < CONTROL_NUM_CONTROL_TYPES)) {
        regfile_f =  m_rf_array[m_retire_rf]->isPatched(rid.getVanilla()) ;
    }

    return regfile_f ;
}

void arf_control_t::setPatched( reg_id_t &rid )
{
    if( !rid.isZero() && (rid.getVanilla() < CONTROL_NUM_CONTROL_TYPES)) {
	//	DEBUG_OUT(" setPatched: %d\n", rid.getPhysical());
        m_rf_array[m_retire_rf]->setPatched(rid.getPhysical()) ;
    }
}

void arf_control_t::clearPatched( reg_id_t &rid )
{
    if( !rid.isZero() && (rid.getVanilla() < CONTROL_NUM_CONTROL_TYPES)) {
	
	//DEBUG_OUT(" clearing: %d\n", rid.getPhysical());
        m_rf_array[m_retire_rf]->clearPatched(rid.getPhysical()) ;
    }
}


void arf_control_t::setFault( reg_id_t &rid )
{
    if( !rid.isZero() && (rid.getVanilla() < CONTROL_NUM_CONTROL_TYPES) &&
            getFaultStat()->getArchMismatch()) {	//siva removed debugio_t::
        m_rf_array[m_retire_rf]->setFault(rid.getVanilla()) ;
    }
}

bool arf_control_t::getReady( reg_id_t &rid )
{
    bool regfile_f = false ;

    if( !rid.isZero() && (rid.getVanilla() < CONTROL_NUM_CONTROL_TYPES)) {
        regfile_f =  m_rf_array[m_retire_rf]->getReady(rid.getVanilla()) ;
    }

    return regfile_f ;
}

void arf_control_t::setCorrupted( reg_id_t &rid )
{
    if( !rid.isZero() && (rid.getVanilla() < CONTROL_NUM_CONTROL_TYPES) &&
            getFaultStat()->getArchMismatch()) {	//siva removed debugio_t::
        m_rf_array[m_retire_rf]->setCorrupted(rid.getVanilla()) ;
    }
}

// void arf_control_t::clearFault( reg_id_t &rid )
// {
//     if( !rid.isZero() && (rid.getVanilla() < CONTROL_NUM_CONTROL_TYPES)) {
//         if( PRADEEP_DEBUG_CLEAR ) {
//             DEBUG_OUT( "Clearing %d", rid.getPhysical() ) ;
//             DEBUG_OUT( " of %s\n", reg_id_t::rid_type_menomic( rid.getRtype() ) ) ;
//         }
//         m_rf_array[m_retire_rf]->clearFault(rid.getVanilla()) ;
//     }
// }

bool arf_control_t::isCorrupted( reg_id_t &rid )
{
    bool regfile_f = false ;

    if( !rid.isZero() && (rid.getVanilla() < CONTROL_NUM_CONTROL_TYPES)) {
        regfile_f =  m_rf_array[m_retire_rf]->isCorrupted(rid.getVanilla()) ;
    }

    return regfile_f ;
}

void arf_control_t::clearAllFaults()
{
    for ( uint32 i = 0; i < CONFIG_NUM_CONTROL_SETS; i++ ) {
		m_rf_array[i]->clearAllFaults() ;
    }
}
/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

