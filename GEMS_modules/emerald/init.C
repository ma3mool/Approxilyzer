#include "init.h"
#include "globals.h"

void init_variables()
{
	cpu_obj = SIM_get_object(CPU_NAME);
	start_cyc = SIM_cycle_count(cpu_obj);
	System = new system_t() ;
}

void init_opal_interface( emerald_api_t *opal_interface )
{
	opal_interface_t::installInterface(opal_interface) ;
}
