/*********************
  Models faults in the decoder
*/

#ifndef _DECODEFAULT_H_
#define _DECODEFAULT_H_

#include "statici.h"
#include "fault.h"

class DecodeFault:public Fault {
	private:
		unsigned int old_inst ;
		unsigned int new_inst ;
		static_inst_t *s_inst ;	// Faulty static inst

	public:
		DecodeFault( static_inst_t *inst, int inst, int stuck_at, int bit ) ;
		~DecodeFault() {}	
		bool isMasked() { return (old_inst == new_inst) ; }
		unsigned int getOldInst() { return old_inst ; }
		unsigned int getNewInst() { return new_inst ; }
		static_inst_t *getFaultyStaticInst() { return s_inst ; }

		void print() ;
} ;

#endif
