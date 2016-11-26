
##  Copyright 2000-2007 Virtutech AB
##  
##  The contents herein are Source Code which are a subset of Licensed
##  Software pursuant to the terms of the Virtutech Simics Software
##  License Agreement (the "Agreement"), and are being distributed under
##  the Agreement.  You should have received a copy of the Agreement with
##  this Licensed Software; if not, please contact Virtutech for a copy
##  of the Agreement prior to using this Licensed Software.
##  
##  By using this Source Code, you agree to be bound by all of the terms
##  of the Agreement, and use of this Source Code is subject to the terms
##  the Agreement.
##  
##  This Source Code and any derivatives thereof are provided on an "as
##  is" basis.  Virtutech makes no warranties with respect to the Source
##  Code or any derivatives thereof and disclaims all implied warranties,
##  including, without limitation, warranties of merchantability and
##  fitness for a particular purpose and non-infringement.

from cli import *
import sim_commands


def emerald_set_log(obj, logfile):
    SIM_set_attribute(obj, "set-log-file", logfile)

new_command( "set-log-file", emerald_set_log,
                 args = [arg(str_t, "logfile", "?", "")],
                 alias = "",
                 type = "emerald commands",
                 short = "set log file name.",
                 namespace = "emerald",
                 doc = "set log file name." )

def emerald_set_inj_log(obj, logfile):
    SIM_set_attribute(obj, "set-inj-log-file", logfile)

new_command( "set-inj-log-file", emerald_set_inj_log,
                 args = [arg(str_t, "logfile", "?", "")],
                 alias = "",
                 type = "emerald commands",
                 short = "set inj log file name.",
                 namespace = "emerald",
                 doc = "set inj log file name." )

def emerald_set_classes_log(obj, logfile):
    SIM_set_attribute(obj, "set-classes-log-file", logfile)

new_command( "set-classes-log-file", emerald_set_classes_log,
                 args = [arg(str_t, "logfile", "?", "")],
                 alias = "",
                 type = "emerald commands",
                 short = "set classes log file name.",
                 namespace = "emerald",
                 doc = "set classes log file name." )

def emerald_set_spprofile_log(obj, logfile):
    SIM_set_attribute(obj, "set-spprofile-log-file", logfile)

new_command( "set-spprofile-log-file", emerald_set_spprofile_log,
                 args = [arg(str_t, "logfile", "?", "")],
                 alias = "",
                 type = "emerald commands",
                 short = "set spprofile log file name.",
                 namespace = "emerald",
                 doc = "set spprofile log file name." )

def emerald_print_heap_use_counts(obj):
    SIM_set_attribute(obj, "print-heap-use-counts", 1)

new_command( "print-heap-use-counts", emerald_print_heap_use_counts,
                 args = [],
                 alias = "",
                 type = "emerald commands",
                 short = "print heap use counts.",
                 namespace = "emerald",
                 doc = "Print the number of uses each def of heap address has." )

def emerald_analyze_stack(obj):
    SIM_set_attribute(obj, "analyze-stack", 1)

new_command( "analyze-stack", emerald_analyze_stack,
                 args = [],
                 alias = "",
                 type = "emerald commands",
                 short = "Analyze stack .",
                 namespace = "emerald",
                 doc = "" )

def emerald_analyze_heap(obj):
    SIM_set_attribute(obj, "analyze-heap", 1)

new_command( "analyze-heap", emerald_analyze_heap,
                 args = [],
                 alias = "",
                 type = "emerald commands",
                 short = "Analyze heap .",
                 namespace = "emerald",
                 doc = "" )

def emerald_text_start(obj, start):
    SIM_set_attribute(obj, "text-start", start)

new_command( "text-start", emerald_text_start,
                 args = [arg(uint64_t, "text start address")],
                 alias = "",
                 type = "emerald commands",
                 short = "Start Text Address.",
                 namespace = "emerald",
                 doc = "Start of text section" )

def emerald_text_end(obj, end):
    SIM_set_attribute(obj, "text-end", end)

new_command( "text-end", emerald_text_end,
                 args = [arg(uint64_t, "text end address")],
                 alias = "",
                 type = "emerald commands",
                 short = "Start Text Address.",
                 namespace = "emerald",
                 doc = "End of text section" )

def emerald_text_start(obj, start):
    SIM_set_attribute(obj, "record-text-start", start)

new_command( "record-text-start", emerald_text_start,
                 args = [arg(uint64_t, "text start address")],
                 alias = "",
                 type = "emerald commands",
                 short = "Start Text Address.",
                 namespace = "emerald",
                 doc = "This PC is where you want to start recording the store-load patterns. pc is for the store instructions." )

def emerald_text_end(obj, end):
    SIM_set_attribute(obj, "record-text-end", end)

new_command( "record-text-end", emerald_text_end,
                 args = [arg(uint64_t, "text end address")],
                 alias = "",
                 type = "emerald commands",
                 short = "Start Text Address.",
                 namespace = "emerald",
                 doc = "This PC is where you want to end recording the store-load patterns. pc is for the store instructions." )


def emerald_print_cycles(obj):
    SIM_set_attribute(obj, "print-cycles", 1)

new_command( "print-cycles", emerald_print_cycles,
                 args = [],
                 alias = "",
                 type = "emerald commands",
                 short = "Print total cycles",
                 namespace = "emerald",
                 doc = "Print the total number of cycles elapsed")

# increment command
def increment_value_cmd(obj):
    try:
        obj.value += 1
        print "The new counter value is:", obj.value
    except Exception, msg:
        print "Error incrementing counter:", msg

new_command("increment", increment_value_cmd, [],
            alias = "",
            type  = "emerald commands",
            short = "increment value",
	    namespace = "emerald",
            doc = """
Increments the value by adding 1 to it.
""")

#
# ------------------------ info -----------------------
#

def get_info(obj):
    # USER-TODO: Return something useful here
    return []

sim_commands.new_info_command('emerald', get_info)

#
# ------------------------ status -----------------------
#

def get_status(obj):
    # USER-TODO: Return something useful here
    return [("Internals",
             [("Attribute 'value'", SIM_get_attribute(obj, "value"))])]


sim_commands.new_status_command('emerald', get_status)
