#include "opal_interface.h"

opal_interface_t::opal_interface_t()
{
	num = 0 ;
}

void opal_interface_t::installInterface( emerald_api_t *opal_interface )
{
	printf("Registering opal_interface functions\n") ;
	//opal_interface->newChkpt = &opal_interface_t::newChkpt ;
	//opal_interface->initialize = &opal_interface_t::initialize ;
	//opal_interface->drainBuffer = &opal_interface_t::drainAllBuffers ;
	//opal_interface->recover = &opal_interface_t::rollbackRecovery ;
}

void opal_interface_t::initialize()
{
	System->initialize() ;
}
