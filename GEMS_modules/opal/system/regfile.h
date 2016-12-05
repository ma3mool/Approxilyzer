
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
#ifndef _REGFILE_H_
#define _REGFILE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "wait.h"
#include "hfa.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define UNKNOWN_REG_FILE   0
#define INT_REG_FILE       1
#define CONTROL_REG_FILE   2
#define CC_REG_FILE        3
#define FP_REG_FILE        4


/*------------------------------------------------------------------------*/
/* Class declarations                                                     */
/*------------------------------------------------------------------------*/

/**
 * Implements a 64-bit physical register, able to be read as an
 * integer, float, or double.
 */
union my_register_u {
    uint64   uint_64;
    uint32   uint_32;
    int64    int_64;
    int32    int_32;
    double   float_64;
    float    float_32;
};

/// typedef of a register union type
typedef union my_register_u my_register_t;

union register_cc_u {
    unsigned char   ccode;
};

typedef union register_cc_u register_cc_t;

/*****************************************************************/
/********************** Physical Register ************************/
/*****************************************************************/

/** 
 *  A physical register holds one of the inflight values associated
 *  with an architectural register. 
 *
 *  The free bit indicates whether the register has been allocated, 
 *  and the ready bit tracks whether the producer has produced yet.
 *
 * @author  zilles
 * @version $Id: regfile.h 1.24 06/02/13 18:49:21-06:00 mikem@maya.cs.wisc.edu $
 */

class physical_reg_t {

    public:
        /** anonymous union: accesss this register as a FP or INT register */
        union {
            ireg_t   as_int;
            float32  as_float;
        };

        /** pointer to a list of instructions waiting on this value */
        wait_list_t wait_list;

        /** indicates if this register is ready to be consumed yet */
        bool        isReady;

        /** indicates if this register is free or not */
        bool        isFree;
};

/*****************************************************************/
/******************** Physical Register File *********************/
/*****************************************************************/

/**
 * The physical register file in the microprocessor.
 *
 * @author  zilles
 * @version $Id: regfile.h 1.24 06/02/13 18:49:21-06:00 mikem@maya.cs.wisc.edu $
 */
class physical_file_t {

    public:

        /**
         * @name Constructor(s) / Destructor
         */
        //@{
        /** Constructor: creates an empty physical register file */
        physical_file_t( uint16 num_physical, pseq_t *seq, fault_stats *fault_stat );

        /** Constructor: creates an empty physical register file of type
          rf_type*/
        physical_file_t( uint16 num_physical, pseq_t *seq, int rf_type, fault_stats *fault_stat);

        /** Destructor: frees the object */
        ~physical_file_t();
        //@}

        /**
         * @name Accessors / Mutators
         */
        //@{
        /** tests if a register has its results "ready" for the next dependent
         *  operation.
         *  @param  i    The physical register number to test.
         *               Limit: i must be less than DEV_NULL_DEST
         *  implicit     The PSEQ_ZERO_REG is %g0 (alpha r31) == always 0 && ready
         *  @return bool True if the operation is ready or not.
         */
        bool isReady(uint16 reg_no) const;
        //{
        //  FAULT_OUT("[0]\t[%d]\Invalid_Reg_Num_Error(0)\n", GET_RET_INST() ) ;
        //  FAULT_SET_NON_RET_TRAP() ;
        //  HALT_SIMULATION ;
        //
        //    ASSERT(reg_no < m_num_physical);
        //    return ( m_reg[reg_no].isReady );
        //}

        /** Unsets the ready bit for a register --
         *  implicit     No operation must be waiting on this register.
         *  @param  i    The physical register number to test.
         *               Limit: i must be less than DEV_NULL_DEST
         */
        void setUnready(uint16 reg_no) {
            if (reg_no >= m_num_physical) {
                return;
            }

            ASSERT(reg_no < m_num_physical);
            ASSERT( m_reg[reg_no].wait_list.Empty() );
            m_reg[reg_no].isReady = false;
            m_reg[reg_no].isFree  = true;
        }

        /** Gives the size of the physical register file.
         *  @return uint16 the size of the physical register file */
        uint16 getNumRegisters( void ) const {
            return (m_num_physical);
        }

        /** Get an integer value.
         *  The register must be ready to be read when this function is called.
         *  @param reg_no The register to read. Must be < REG_NUM_ACTUAL.
         *  @return uint64 The value in the register.
         */
        uint64 getInt(uint16 reg_no) const {
            if (reg_no >= m_num_physical) {
                return 0;
            }

            ASSERT (reg_no < m_num_physical);

            uint64 value ;

	//LXL: no check for ready, see what happens
#if 0            
            if (!isReady(reg_no)) {
                dependenceError( reg_no );
                return 0;
            }
#endif

            value = m_reg[reg_no].as_int ;

            if( reg_file_type == INT_REG_FILE ) {
                if( GSPOT_DEBUG ) {
                    DEBUG_OUT( "Reading Int reg file. Reg no = %x\t", reg_no ) ;
                    DEBUG_OUT( "(Faulty reg = %x)\n", m_fs->getFaultyReg() ) ;
                    DEBUG_OUT( "FAULT_TYPE = %u\n", m_fs->getFaultType() ) ;
                }

                if( ((m_fs->getFaultType()==REG_FILE_FAULT) || (m_fs->getFaultType()==FP_REG_FAULT)) && 
                    m_fs->getFaultStuckat() != TRANSIENT &&
                    (reg_no==m_fs->getFaultyReg()) ) {

                    uint64 new_value = m_fs->injectFault(value);

					if( new_value != value ) {
						has_fault[reg_no] = true ;
						corrupted[reg_no] = true ;
						// This should happen one time, the first time
						// only. Corrupt the register in the reg file.
						m_reg[reg_no].as_int = new_value;
					}
                    value = new_value ;
                }
            }
            return value ;
        }
                
        /** Set an integer value.
         *  @param reg_no The register to read. Must be < REG_NUM_ACTUAL.
         *  @param value The value to write in the register 
         */
        void   setInt(uint16 reg_no, uint64 value) {
		//printf(" regno = %d, m_num_physical = %d\n", reg_no, m_num_physical);
            if (reg_no >= m_num_physical) {
                return;
            }

            ASSERT (reg_no < m_num_physical);

            if (reg_file_type==INT_REG_FILE) {
                if( GSPOT_DEBUG ) {
                    DEBUG_OUT( "Writing Int reg file. Reg no = %d\t", reg_no ) ;
                    DEBUG_OUT( "(value = %llx)\n", value ) ;
                }
                if( ((m_fs->getFaultType()==REG_FILE_FAULT) || (m_fs->getFaultType()==FP_REG_FAULT)) && 
                     (m_fs->getFaultStuckat() != TRANSIENT) &&
                     (reg_no == m_fs->getFaultyReg())) {

                    uint64 new_value = m_fs->injectFault(value);

                    if( new_value != value ) {
                        setFault(reg_no) ;
                        value = new_value ;
                    }
                }
            }

            m_reg[reg_no].as_int = value;

            setReady(reg_no);
        }

        /* chkGetInt getInt for check command
         */
        uint64 chkGetInt(uint16 reg_no) const {
/*             if (reg_no >= m_num_physical) { */
/*                 return 0; */
/*             } */

            ASSERT (reg_no < m_num_physical);

            uint64 value ;
            
            value = m_reg[reg_no].as_int ;
            return value ;
        }
                
        /** chkSetInt setInt for check command
         */
	void  chkSetInt(uint16 reg_no, uint64 value) {
		if (reg_no >= m_num_physical) {
			return;
		}

		ASSERT (reg_no < m_num_physical);

		if (reg_file_type==INT_REG_FILE) {
			if( GSPOT_DEBUG ) {
				DEBUG_OUT( "Writing Int reg file. Reg no = %d\t", reg_no ) ;
				DEBUG_OUT( "(value = %llx)\n", value ) ;
			}
			if( ((m_fs->getFaultType()==REG_FILE_FAULT) || (m_fs->getFaultType()==FP_REG_FAULT)) && 
				 (m_fs->getFaultStuckat() != TRANSIENT) &&
				 (reg_no == m_fs->getFaultyReg()) ) {

				uint64 new_value = m_fs->injectFault(value);

				if( new_value != value ) {
					setFault(reg_no) ;
					value = new_value ;
				}
			}
		}

		m_reg[reg_no].as_int = value;
	}

        /** Get an single floating point value.
         *  The register must be ready to be read when this function is called.
         *  @param  reg_no  The register to read. Must be < REG_NUM_ACTUAL.
         *  @return float32 The value in the register.
         */
        float32 getFloat(uint16 reg_no) const {
            if (reg_no >= m_num_physical) {
                return 0.0;
            }

            ASSERT (reg_no < m_num_physical);
#if 0
            if (!isReady(reg_no)) {
                dependenceError( reg_no );
                return 0.0;
            }
#endif
            return m_reg[reg_no].as_float;
        }

        /** Set an single floating point value.
         *  @param reg_no The register to read. Must be < REG_NUM_ACTUAL.
         *  @param value The value to set in the register.
         */
        void    setFloat(uint16 reg_no, float32 value) {
            if (reg_no >= m_num_physical) {
                return;
            }

            ASSERT (reg_no < m_num_physical);
            m_reg[reg_no].as_float = value;
            setReady(reg_no);
        }
        //@}

        /** @name Instructions waiting for registers */
        //@{  
        /// wait on this register to be written (i.e. to become ready)
        void    waitResult( uint16 reg_no, dynamic_inst_t *d_instr );

        /// is the wait list empty
        bool    isWaitEmpty( uint16 reg_no );

        /// set the physical register free bit to "value"
        void    setFree(uint16 reg_no, bool value) {
            if (reg_no >= m_num_physical) {
                return;
            }

            ASSERT(reg_no < m_num_physical);
            m_reg[reg_no].isFree  = value;
        }

        /** sets a watch point on a physical register. used for debugging. */
        void   setWatchpoint( uint16 reg_no ) {
            m_watch = reg_no;
        }

        /// print out a particular physical register
        void    print( uint16 reg_no );
        //@}

        void setFault( uint16 reg_no ) { 
            if( reg_no < m_num_physical ) {
                has_fault[reg_no] = true ;
		corrupted[reg_no] = true ;
            }
        }
        void clearFault( uint16 reg_no ) {
            if( PRADEEP_DEBUG_CLEAR ) {
                DEBUG_OUT( "Faulty = %d, Total = %d\n", reg_no, m_num_physical ) ;
            }
            if( reg_no < m_num_physical ) {
                if( PRADEEP_DEBUG_CLEAR ) {
                    DEBUG_OUT( "Clearing\n" ) ;
                }
                has_fault[reg_no] = false ;
            }
        }
	bool getFault( uint16 reg_no ) {
		return has_fault[reg_no] ; 
	}
	void clearPatched( uint16 reg_no) {
		patched[reg_no] = false;
	}
	bool getReady( uint16 reg_no ) {
		return m_reg[reg_no].isReady ;
	}
	bool isPatched( uint16 reg_no ) {
		return patched[reg_no] ; 
	}
	void setPatched( uint16 reg_no ) {
		patched[reg_no] = true ; 
	}
	void setValue(uint16 reg_no, uint64 value) {
		if( reg_no < m_num_physical ) {
			m_reg[reg_no].as_int = value;
		}
	}
	bool isCorrupted(uint16 reg_no) {
		if( reg_no < m_num_physical ){
			if(SIVA_CORRUPTION_DEBUG) {
				if(corrupted[reg_no]) {
					DEBUG_OUT("isCorrupted: corrupted bit is true!!!!");	
				}
			}
			// if(reg_no==149) {
			// 	DEBUG_OUT("corupted = %d at 0x%llx\n", corrupted[reg_no], &corrupted[reg_no]) ;
			// }
			return (corrupted[reg_no]);
		}
		else {
			return false;
		}
	}
	void setCorrupted(uint16 reg_no) {
		if( reg_no < m_num_physical ) {
			if(SIVA_CORRUPTION_DEBUG) {
				DEBUG_OUT("setcorrupted: setting corrupted bit to true!!!!");	
			}
			corrupted[reg_no] = true;
			// if(reg_no==149) {
			// 	DEBUG_OUT("corupted = %d at 0x%llx\n", corrupted[reg_no], &corrupted[reg_no]) ;
			// }
		}
	}
	void unSetCorrupted(uint16 reg_no) {
		if( reg_no < m_num_physical ) {
			corrupted[reg_no] = false;
		}
	}

	inline void setReadFault( uint16 reg_no ) {
		if( reg_no < m_num_physical ) {
			read_fault[reg_no] = true ;
		}
	}

	inline void clearReadFault( uint16 reg_no ) {
		if( reg_no < m_num_physical ) {
			read_fault[reg_no] = false ;
		}
	}

	inline bool getReadFault( uint16 reg_no ) { return read_fault[reg_no] ; }

	void printFaultBits() {
		for( int i=0; i< m_num_physical; i++ ) {
			printf( "%x -> %d\n", i, has_fault[i] ) ;
		}
	}

	void injectTransientFault(half_t inj_reg_no = -1) {

		if ( (m_fs->getFaultType()!=REG_FILE_FAULT) && (m_fs->getFaultType()!=FP_REG_FAULT) ) {
			return;
		}

		int reg_no = m_fs->getFaultyReg();
#ifdef ARCH_REGISTER_INJECTION
		reg_no = inj_reg_no;
#endif // ARCH_TRANS_FAULT
		uint64 value = m_reg[reg_no].as_int ;
		if( m_fs->getFaultType()==REG_FILE_FAULT) {
			// DEBUG_OUT("Injecting Trans REG_FILE_FAULT fault in system/regfile.h\n") ;
			uint64 new_value = m_fs->injectFault(value);

			if( new_value != value ) {
				has_fault[reg_no] = true ;
				corrupted[reg_no] = true ;
				m_reg[reg_no].as_int = new_value;

				DEBUG_OUT("Old value = 0x%llx, New value = 0x%llx\n", value, new_value);
				// DEBUG_OUT("Old = 0x%llx, New = 0x%llx, bit = %d\n", value, new_value, m_fs->getFaultBit()) ;
			}
		} else if (m_fs->getFaultType()==FP_REG_FAULT) {
			// DEBUG_OUT("Injecting Trans FP_REG_FAULT fault in system/regfile.h\n") ;
			if(reg_no>m_num_physical) {
				// Doubles are implemented as two 32-bit registers.
				if(m_fs->getFaultBit()>31) {
					reg_no = (reg_no>>8) & 0xff ;
					m_fs->setFaultBit(m_fs->getFaultBit()-31) ;
				} else {
					reg_no = reg_no& 0xff ;
				}
			}
			uint64 value = m_reg[reg_no].as_int ;
			uint64 new_value = m_fs->injectFault(value);

			if( new_value != value ) {
				has_fault[reg_no] = true ;
				corrupted[reg_no] = true ;
				m_reg[reg_no].as_int = new_value;
				DEBUG_OUT("Old value = 0x%llx, New value = 0x%llx\n", value, new_value);
				// DEBUG_OUT("reg_no: %d, total=%d, Old = 0x%llx, New = 0x%llx, bit = %d\n",
				// 		reg_no, m_num_physical, value, new_value, m_fs->getFaultBit()) ;
			}
		}
	}

	void setSyncTarget(uint16 phy_reg, uint16 log_reg) {
		sync_target[phy_reg] = log_reg;
	}

	uint16 getSyncTarget(uint16 phy_reg) {
		return sync_target[phy_reg];
	}

	inline void incRefCnt(uint16 reg_no) { 
		if( reg_no < m_num_physical ) {
			ref_count[reg_no] ++ ;
		}
	}            

	inline void decRefCnt(uint16 reg_no) { 
		if( reg_no < m_num_physical ) {
			ref_count[reg_no] -- ;
		}
	}            

	inline int getRefCnt(uint16 reg_no) { 
		return ref_count[reg_no];
	}

	void wakeDependents (uint16 reg_no) {
			// wakeup all instructions waiting for this register to be written
#ifdef LXL_WAKEDEP_DEBUG
		DEBUG_OUT("wake %d dep\n",reg_no); //FIXME
#endif
		m_reg[reg_no].wait_list.WakeupChain();
	}

	void setRFWakeup(bool value) 
	{
		wake_dep = value;
	}

	pseq_t* getSequencer() { return m_pseq; }

	fault_stats* getFaultStat() { return m_fs; }

	void clearAllFaults() {
		for( int i=0;i<m_num_physical;i++) {
			has_fault[i] = false;
			read_fault[i] = false;
			patched[i] = false;
			corrupted[i] = false;
			ref_count[i] = 0 ;
			sync_target[i] = PSEQ_INVALID_REG ;

			if (reg_file_type==INT_REG_FILE) {
				m_reg[i].wait_list.wl_reset();
				m_reg[i].isReady = false;
				m_reg[i].isFree  = true;
				m_reg[i].as_int  = 0;
			}
		}
	}

	protected:
	/** sets a register has having its results "ready" for the next dependent
	 *  operation. Implicitly called when setInt() is called -- 
	 *  should not be called anywhere else.
	 *
	 *  @param  i    The physical register number to test.
	 *               Limit: i must be less than DEV_NULL_DEST
	 *  implicit     The PSEQ_ZERO_REG is %g0 (alpha r31) == always 0 && ready
	 */
	void setReady(uint16 reg_no) {
		// already have checked this: ASSERT(reg_no < m_num_physical);
		m_reg[reg_no].isReady = true;

#ifdef LXL_WAKEDEP_DEBUG
			DEBUG_OUT("set %d ready\n",reg_no); //FIXME
#endif
			// ALEX's comment: This is a bad thing, it messes up when
			// we do INT ALU injection

			// wakeup all instructions waiting for this register to be written
			if (wake_dep) {
				m_reg[reg_no].wait_list.WakeupChain();
			}
		}

		void dependenceError( uint16 reg_no ) const;

		/** number of physical registers in this file */
		uint16          m_num_physical;

		/** number of actual (allocated) registers -- to account for "virtual regs"*/
		uint16          m_num_actual;

		/** watch point on a physical register in the machine */
		uint16          m_watch;

		/** physical registers in machine */
		physical_reg_t *m_reg;

		/** owning sequencer: used for debugging  */
		pseq_t         *m_pseq;

		fault_stats 	*m_fs;

		bool           *patched ;

		/** bit to track if the injection was successful */
		bool           *has_fault ;

		/** bit to track if the regester is corrupted */
		bool           *corrupted ;

		/** bit to track if the faulty value was read */        
		bool           *read_fault ;

		/** reference count array */
		int         *ref_count ;

		/** sync target */
		uint16     *sync_target;

        /** physical register file type **/
        int reg_file_type;

        bool wake_dep;
};

#endif /* _REGFILE_H_ */
