/*
  Copyright 1998-2007 Virtutech AB
  
  The contents herein are Source Code which are a subset of Licensed
  Software pursuant to the terms of the Virtutech Simics Software
  License Agreement (the "Agreement"), and are being distributed under
  the Agreement.  You should have received a copy of the Agreement with
  this Licensed Software; if not, please contact Virtutech for a copy
  of the Agreement prior to using this Licensed Software.
  
  By using this Source Code, you agree to be bound by all of the terms
  of the Agreement, and use of this Source Code is subject to the terms
  the Agreement.
  
  This Source Code and any derivatives thereof are provided on an "as
  is" basis.  Virtutech makes no warranties with respect to the Source
  Code or any derivatives thereof and disclaims all implied warranties,
  including, without limitation, warranties of merchantability and
  fitness for a particular purpose and non-infringement.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <simics/api.h>
#include <simics/alloc.h>
#include <simics/utils.h>

#include "emerald_api.h"
#include "config.h"
#include "init.h"
#include "commands.h"

typedef uint64 ireg_t;

typedef struct {
        /* log_object_t must be the first thing in the device struct */
        log_object_t log;
        int value;
} emerald_t;


cycles_t drain_cyc ;

/*
 * The new_instance() function is registered with the SIM_register_class
 * call (see init_local() below), and is used as a constructor for every
 * instance of the empty-device class.
 */
conf_object_t * new_instance(parse_object_t *parse_obj)
{
        emerald_t *empty = MM_ZALLOC(1, emerald_t);
        SIM_log_constructor(&empty->log, parse_obj);

        /* USER-TODO: Add initialization code for new instances here */

        return &empty->log.obj;
}

cycles_t operation(conf_object_t *obj, conf_object_t *space,
                      map_list_t *map, generic_transaction_t *g)
{
	int retval = emerald_operate(obj, space, map, g) ;
	return retval ;
}

set_error_t
set_value_attribute(void *arg, conf_object_t *obj,
                    attr_value_t *val, attr_value_t *idx)
{
        emerald_t *empty = (emerald_t *)obj;
        empty->value = val->u.integer;
        return Sim_Set_Ok;
}

attr_value_t
get_value_attribute(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        emerald_t *empty = (emerald_t *)obj;
        return SIM_make_attr_integer(empty->value);
}

void device_access_callback(void* desc, conf_object_t *trigger_obj, generic_transaction_t *memop)
{
}

void sim_exit_callback(void* desc, conf_object_t *trigger_obj)
{
	printStats() ;
}

void hap_sync_inst(void *desc, conf_object_t *obj, integer_t type)
{
	// printf("#### SYNC INST ####\n") ;
	// Type 0x1 is a loadload membar. The rest have stores 
	if(type != 0x1) {
		//drainOutputBuffer() ;
	}
}

void
periodicCall(void* desc, conf_object_t *trigger_obj, integer_t cyc_count)
{
}

#define EMERALD_COMMAND( COMMAND )                                        \
    SIM_register_attribute( emerald_class, COMMAND,                    \
                                  NULL, (void *) COMMAND,                \
                                  emerald_set_attr, (void *) COMMAND,        \
                                  Sim_Attr_Session,             \
                                  "See documentation with associated emerald command." )

set_error_t 
emerald_set_attr( void *id, conf_object_t *obj, 
                attr_value_t *val, attr_value_t *idx )
{
    const char *attr_fn = (const char *) id;
    // obj is the hfa0 object

    if (!strcmp(attr_fn, "set-log-file")) {
	if (val->kind == Sim_Val_String) {
	    if(!openLog(val->u.string)) {
               	fprintf(stderr, "error opening %s\n",val->u.string);
            }
        } else {
            return Sim_Set_Need_String;
        }
    } else if (!strcmp(attr_fn, "set-inj-log-file")) {
	if (val->kind == Sim_Val_String) {
	    if(!openInjLog(val->u.string)) {
               	fprintf(stderr, "error opening %s\n",val->u.string);
            }
        } else {
            return Sim_Set_Need_String;
        }
    } else if (!strcmp(attr_fn, "set-classes-log-file")) {
	if (val->kind == Sim_Val_String) {
	    if(!openClassesLog(val->u.string)) {
               	fprintf(stderr, "error opening %s\n",val->u.string);
            }
        } else {
            return Sim_Set_Need_String;
        }
    } else if (!strcmp(attr_fn, "set-spprofile-log-file")) {
	if (val->kind == Sim_Val_String) {
	    if(!openSPProfileLog(val->u.string)) {
               	fprintf(stderr, "error opening %s\n",val->u.string);
            }
        } else {
            return Sim_Set_Need_String;
        }

    } else if (!strcmp(attr_fn, "print-cycles")) {
		// Deprecated
    } else if (!strcmp(attr_fn, "print-heap-use-counts")) {
		printHeapUseCounts();
    } else if (!strcmp(attr_fn, "analyze-stack")) {
		analyzeStack();
    } else if (!strcmp(attr_fn, "analyze-heap")) {
		analyzeHeap();
    } else if (!strcmp(attr_fn, "text-start")) {
      if (val->kind == Sim_Val_Integer)
		textStart(val->u.integer);
    } else if (!strcmp(attr_fn, "text-end")) {
      if (val->kind == Sim_Val_Integer)
		textEnd(val->u.integer);
    } else if (!strcmp(attr_fn, "record-text-start")) {
      if (val->kind == Sim_Val_Integer)
		recordTextStart(val->u.integer);
    } else if (!strcmp(attr_fn, "record-text-end")) {
      if (val->kind == Sim_Val_Integer)
		recordTextEnd(val->u.integer);
    } else {
        fprintf(stderr, "error: unknown command: %s\n", attr_fn );
		return Sim_Set_Attribute_Not_Found ;
    }
    return Sim_Set_Ok;
}


/* init_local() is called once when the device module is loaded into Simics */
void init_local(void)
{
	printf("About to load module emerald\n");
    class_data_t emerald_funcs;
    conf_class_t *emerald_class;
    timing_model_interface_t *timing_iface;
    conf_object_t *emerald_obj ;
    attr_value_t   val;

    /* Register the empty device class. */
    bzero(&emerald_funcs, sizeof(class_data_t));
    emerald_funcs.new_instance = new_instance;
    emerald_funcs.delete_instance = NULL ;
    emerald_class = SIM_register_class("emerald", &emerald_funcs);


    timing_iface = MM_ZALLOC(1, timing_model_interface_t);
    timing_iface->operate = operation;
    SIM_register_interface(emerald_class, OBSERVE_MODEL, timing_iface);
    //SIM_register_interface(emerald_class, "timing-model", timing_iface);
    // SIM_register_interface(emerald_class, "snoop-model", timing_iface);

    /* USER-TODO: Add any attributes for the device here */
    //SIM_register_typed_attribute(
    //        emerald_class, "value",
    //        get_value_attribute, NULL,
    //        set_value_attribute, NULL,
    //        Sim_Attr_Optional,
    //        "i", NULL,
    //        "Value containing a valid valuable valuation.");

    EMERALD_COMMAND( "set-log-file" );
    EMERALD_COMMAND( "set-inj-log-file" );
    EMERALD_COMMAND( "set-classes-log-file" );
    EMERALD_COMMAND( "set-spprofile-log-file" );
    EMERALD_COMMAND( "print-cycles" ) ;
    EMERALD_COMMAND( "print-heap-use-counts" ) ;
    EMERALD_COMMAND( "analyze-heap" ) ;
    EMERALD_COMMAND( "analyze-stack" ) ;
    EMERALD_COMMAND( "text-start" ) ;
    EMERALD_COMMAND( "text-end" ) ;
    EMERALD_COMMAND( "record-text-start" ) ;
    EMERALD_COMMAND( "record-text-end" ) ;

	init_variables() ;

	emerald_obj = SIM_new_object(emerald_class, "emerald0") ;
	val.kind = Sim_Val_Object ;
	val.u.object = emerald_obj ;
	conf_object_t *phys_mem = SIM_get_object(PHYS_MEM_NAME) ;
    set_error_t install_error = SIM_set_attribute(phys_mem, MODEL_STRING, &val);
    if(install_error != Sim_Set_Ok) {
        printf("Some error in installing the emerald_mem name space object\n") ;
	}

	// SIM_hap_add_callback("Core_Device_Access_Memop", (obj_hap_func_t) device_access_callback, NULL);
	// SIM_hap_add_callback("Core_External_Interrupt", (obj_hap_func_t) interrupt_callback, NULL);
    if(install_error != Sim_Set_Ok) {
        printf("Some error in installing the emerald0 name space object\n") ;
    }

#ifdef CONFIG_IN_OPAL
	// Opal interface object
	emerald_api_t *opal_interface ;
    opal_interface = MM_ZALLOC(1, emerald_api_t) ;
    SIM_register_interface(emerald_class, "emerald-api", opal_interface) ;
	init_opal_interface( opal_interface ) ;
	printf("Done Initing emerald-api interface\n") ;
#endif // CONFIG_IN_OPAL

    SIM_hap_add_callback("Core_At_Exit", (obj_hap_func_t) sim_exit_callback, NULL);
    // SIM_hap_add_callback("Core_Sync_Instruction", (obj_hap_func_t) hap_sync_inst, NULL);
    printf("loaded module emerald\n") ;
}
