/*
 * |-----------------------------------------------------------------------|
 * |                                                                       |
 * |   Copyright Cadence Design Systems, Inc. 1985, 1988.                  |
 * |     All Rights Reserved.       Licensed Software.                     |
 * |                                                                       |
 * |                                                                       |
 * | THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF CADENCE DESIGN SYSTEMS |
 * | The copyright notice above does not evidence any actual or intended   |
 * | publication of such source code.                                      |
 * |                                                                       |
 * |-----------------------------------------------------------------------|
 */

/*
 * |-------------------------------------------------------------|
 * |                                                             |
 * | PROPRIETARY INFORMATION, PROPERTY OF CADENCE DESIGN SYSTEMS |
 * |                                                             |
 * |-------------------------------------------------------------|
 */

#include <stdarg.h>
#include "vpi_user.h"
#include "vpi_user_cds.h"


/* ----------------------------------------------------------------
The following is an example of what should be included in this file:

extern void setup_my_callbacks(); <-- Add a declaration for your routine.

void (*vlog_startup_routines[])() = 
{
    $*** add user entries here ***$

  setup_my_callbacks, <-- Add your routine to the table. 

  0 $*** final entry must be 0 ***$

};
------------------------------------------------------------------ */

//extern void readStimuliRegister();
//extern void writeLatchRegister();

// the elements in this array are pointers to functions that the simulator executes when it starts up
// prior to the elaboration and linking
void (*vlog_startup_routines[VPI_MAXARRAY])() =
{
// readStimuliRegister,
// writeLatchRegister,
 0 /*** final entry must be 0 ***/
};

