#include "globals.h"
#include "emerald_api.h"

// Global system pointer
system_t *System = NULL ;

// simulation environment
conf_object_t *cpu_obj = NULL ;
cycles_t start_cyc = 0x0 ;
cycles_t end_cyc = 0x0 ;

bool is_chkpting = false ;

// File handle for log
FILE *m_outfp = NULL ;
FILE *m_injfp = NULL ;
FILE *m_classfp = NULL ;
FILE *m_spprofilefp = NULL ;

void printTransaction(FILE *file, char *msg, generic_transaction_t *g)
{
	if(!file) {
		file = stdout ;
	}
	fprintf(file, "%s: [%llu] {%d} %s [%s] at 0x%llx of %d bytes\n",
			msg,
			(SIM_cycle_count(g->ini_ptr) - start_cyc),
			g->id,
			SIM_mem_op_is_write(g)?"St":"Ld",
			g->ini_ptr->name,
			g->physical_address,
			g->size) ;
}

void printTransaction(FILE *file, char *msg, conf_object_t *obj, conf_object_t *space, map_list_t *map, generic_transaction_t *g)
{
	if(!file) {
		file = stdout ;
	}
	fprintf(file, "%s: [%llu] {%d} %s [%s]->[%s] at 0x%llx of %d bytes\n",
			msg,
			(SIM_cycle_count(g->ini_ptr) - start_cyc),
			g->id,
			SIM_mem_op_is_write(g)?"St":"Ld",
			g->ini_ptr->name,
			map->object->name,
			g->physical_address,
			g->size) ;
}

void SIMICS_read_physical_memory_buffer(physical_address_t addr, char *buffer, int len)
{
  assert( buffer != NULL );

  int buffer_pos = 0;
  physical_address_t start = addr;
  do {
    int size = (len < 8)? len:8;
    integer_t result = SIM_read_phys_memory( cpu_obj, start, size );
    int isexcept = SIM_get_pending_exception();
    if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
      sim_exception_t except_code = SIM_clear_exception();
      fprintf(stderr, "emerald: SIMICS_read_physical_memory_buffer: raised exception %s\n", SIM_last_error() );
	  fprintf(stderr, "size = %d\n", size) ;
    }

    // assume big endian (i.e. SPARC V9 target)
    for(int i = size-1; i >= 0; i--) {
      buffer[buffer_pos++] = (char) ((result>>(i<<3))&0xff);
    }

    len -= size;
    start += size;
  } while(len != 0);
}

/*
 * write data to simics memory from a buffer (assumes the buffer is valid)
 */
void SIMICS_write_physical_memory_buffer(physical_address_t addr,
                                        char* buffer, int len ) {
  assert( buffer != NULL );

  int buffer_pos = 0;
  physical_address_t start = addr;
  do {
    int size = (len < 8)? len:8;
    //integer_t result = SIM_read_phys_memory( cpu_obj, start, size );
    integer_t value = 0;
    // assume big endian (i.e. SPARC V9 target)
    for(int i = size-1; i >= 0; i--) {
      //buffer[buffer_pos++] = (char) ((result>>(i<<3))&0xff);
      integer_t mask = buffer[buffer_pos++];
      value |= ((mask&0xff)<<(i<<3));
    } 

    SIM_write_phys_memory( cpu_obj, start, value, size);
    int isexcept = SIM_get_pending_exception();
    if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
      sim_exception_t except_code = SIM_clear_exception();
      fprintf(stderr, "emerald: SIMICS_write_physical_memory_buffer: raised exception %s\n", SIM_last_error() );
    }

    len -= size;
    start += size;
  } while(len != 0);
}

