/*****
  Function definitions for Fault class
 *****/

#include "hfa.h"
#include "hfacore.h"

#include "lsq.h"
#include "exec.h"
#include "scheduler.h"
#include "fault.h"
#include "opcode.h"

void Fault::out_fault_log( const char *fmt, ... ) {
	va_list args ;

	f_log = fopen( "Fault.log", "a" ) ;
	printf( "..........................PRINTING..................\n" ) ;
	va_start(args, fmt) ;
	int error_rt = vfprintf( f_log, fmt, args ) ;
	va_end(args) ;
	fclose( f_log ) ;

	ASSERT( error_rt >= 0 ) ;
}
