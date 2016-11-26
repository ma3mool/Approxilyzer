#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include "globals.h"

using namespace std ;

class opal_interface_t {
public:
	// Link the appropriate functions with opal
	opal_interface_t() ;
	static void installInterface( emerald_api_t *opal_interface ) ;

	static void initialize() ;

private:
	int num ;

} ;

#endif // __INTERFACE_H__
