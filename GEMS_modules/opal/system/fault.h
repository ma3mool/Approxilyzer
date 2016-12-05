/*****
  Class for defining faults
  The fault right now resides inside the static instruction.
  Some vague include problems when trying to put it into the
  dynamic instruction.. bound to pain the butt later!
 */

#ifndef _FAULT_H_
#define _FAULT_H_

#include "statici.h"

class Fault {
	protected:
		unsigned int seq_num ;	// Sequence number of the inst with fault
		int stuck_at ;
		int bit ;		// Bit stuck at. Assuming big Endian format
		pseq_t *m_pseq ;	// Main processor

		tick_t inject_cycle ;	// Cycle at which fault was injected
		tick_t arch_cycle ;	// Cycle that this fault was architecturally visible

		FILE *f_log ;	

	public:
		Fault( int stuck_at, int bit ):stuck_at(stuck_at), bit(bit) {}
		~Fault() {}

		void setSeqNum( unsigned int num )	{ seq_num = num ; }
		void setPseq( pseq_t *pseq ) 		{ m_pseq = pseq ; }
		void setInjectedCycle( tick_t cycle )	{ inject_cycle = cycle ; }
		void setArchCycle( tick_t cycle )	{ arch_cycle = cycle ; }

		unsigned int getSeqNum() { return seq_num ; }
		int getFaultyBit() { return bit ; }
		int getStuckAt() { return stuck_at ; }

		void out_fault_log( const char *fmt, ... ) ;

		virtual bool isMasked() = 0 ;
		virtual void print() = 0 ;
		virtual unsigned int getNewInst() = 0 ;
} ;
#endif
