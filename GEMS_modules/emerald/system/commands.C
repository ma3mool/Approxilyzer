#include "commands.h"
#include "globals.h"

extern "C" {
#include <simics/api.h>
#include <simics/alloc.h>
#include <simics/utils.h>
} ;


extern "C"
void emerald_operate(conf_object_t *obj, conf_object_t *space, map_list_t *map, generic_transaction_t *g)
{
	System->operate(obj, space, map, g) ;
}


extern "C"
void initialize()
{
	System->initialize() ;
}

extern "C"
void printStats()
{
	System->printStats() ;
}

extern "C"
void printHeapUseCounts()
{
	System->printHeapUseCounts() ;
}

extern "C"
void analyzeHeap()
{
	System->setAddressSpace(0) ;
}

extern "C"
void analyzeStack()
{
	System->setAddressSpace(1) ;
}

extern "C"
void textStart(uint64 start)
{
	System->textStart(start) ;
}

extern "C"
void textEnd(uint64 end)
{
	System->textEnd(end) ;
}

extern "C"
void recordTextStart(uint64 start)
{
	System->recordTextStart(start) ;
}

extern "C"
void recordTextEnd(uint64 end)
{
	System->recordTextEnd(end) ;
}

extern "C"
int openLog(char *name)
{
	m_outfp = fopen(name, "w") ;
	if(m_outfp != NULL) {
		return 1;
	} else {
		return 0 ;
	}
}

extern "C"
int openInjLog(char *name)
{
	m_injfp = fopen(name, "w") ;
	if(m_injfp != NULL) {
		return 1;
	} else {
		return 0 ;
	}
}

extern "C"
int openClassesLog(char *name)
{
	m_classfp = fopen(name, "w") ;
	if(m_classfp != NULL) {
		return 1;
	} else {
		return 0 ;
	}
}

extern "C"
int openSPProfileLog(char *name)
{
	m_spprofilefp = fopen(name, "w") ;
	if(m_spprofilefp != NULL) {
		return 1;
	} else {
		return 0 ;
	}
}

