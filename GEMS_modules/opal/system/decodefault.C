/*****
  Functions for faults in the decoder
 */

#include "hfa.h"
#include "hfacore.h"

#include "lsq.h"
#include "exec.h"
#include "scheduler.h"
#include "decodefault.h"
#include "opcode.h"

DecodeFault::DecodeFault( static_inst_t *s_inst, int inst, int stuck_at, int bit):Fault(stuck_at, bit)
{
	if( s_inst ) {
		// Copy the old instruction object into the old_inst 
		old_inst = inst ;

		this->s_inst = s_inst ;
		if( stuck_at ) {	// then, a stuck@1 fault
			new_inst = old_inst | (stuck_at << bit ) ;
		} else {	// then, a stuck@0 fault
			new_inst = old_inst & (~(1<<bit)) ;
		} 
		//out_fault_log( "%x\t", old_inst ) ;
		//out_fault_log( "%x\t", new_inst ) ;
		//out_fault_log( "%d\n", isMasked() ) ;
	}
}

void DecodeFault::print()
{
	enum i_opcode op = (enum i_opcode) s_inst->getOpcode() ;
	out_fault_log( "%d\t", seq_num ) ;
	out_fault_log( "%x\t", old_inst ) ;
	out_fault_log( "%x\t", s_inst->getInst() ) ;
	out_fault_log( "%s\t", decode_opcode(op) ) ;
	out_fault_log( "%d\t", inject_cycle ) ;
	out_fault_log( "%d\t", arch_cycle ) ;
	out_fault_log( "%d\n", arch_cycle-inject_cycle ) ;
}
