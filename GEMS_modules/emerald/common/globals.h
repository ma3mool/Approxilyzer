#ifndef __GLOBALS_H__
#define __GLOBALS_H__

extern "C" {
#include <simics/api.h>
#include <simics/alloc.h>
#include <simics/utils.h>
} ;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "system.h"
#include "emerald_api.h"
#include "opal_interface.h"

class system_t ;

// #include "chkpt.h"

using namespace std ;


// Useful typedefs
typedef uint64 physical_address_t ;

#ifdef __cplusplus
extern "C" {
#endif

extern void init_variables() ;
extern void init_opal_interface(emerald_api_t *opal_interface) ;

#ifdef __cplusplus
}
#endif // __cplusplus

// Global handle for the System
extern system_t *System ;

// File handle of log file
extern FILE *m_outfp;
extern FILE *m_injfp;
extern FILE *m_classfp;
extern FILE *m_spprofilefp;

// Global variables about simulation environment
extern conf_object_t *cpu_obj;
extern cycles_t start_cyc;
extern cycles_t end_cyc ;
extern bool is_chkpting ;

// Functions
extern void printTransaction(FILE *file, char *msg, generic_transaction_t *g) ;
extern void printTransaction(FILE *file, char *msg, conf_object_t *obj, conf_object_t *space, map_list_t *map, generic_transaction_t *g) ;
extern void SIMICS_read_physical_memory_buffer(physical_address_t addr, char *buffer, int length) ;
extern void SIMICS_write_physical_memory_buffer(physical_address_t addr, char* buffer, int len ) ;
#endif // __GLOBALS_H__
