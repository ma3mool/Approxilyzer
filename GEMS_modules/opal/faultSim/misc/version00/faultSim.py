import time, sim_commands, string, sys, getpass, os
from cli import *

LOG_FILE_DIR = "./logs/"
#SIM_STEPS = "11000000"
SIM_STEPS = "11000"
 
def get_rand_simics_fd(app,phase):
  print "Get Simics Random forwarding checkpoint\n"
  run_sim_command('read-configuration ../sim_phases/%s_%s' %(app,phase) )
  run_sim_command('c 4431287')
  run_sim_command('run-python-file check_privilege.py')
  run_sim_command('write-configuration ../sim_phases/%s_%s-1' %(app,phase) )
  run_sim_command('c 2410963')
  run_sim_command('run-python-file check_privilege.py')
  run_sim_command('write-configuration ../sim_phases/%s_%s-2' %(app,phase) )

def get_rand_super(app):
  print "Get Simics Random forwarding checkpoint\n"
  run_sim_command('read-configuration ../sim_phases/%s' %(app) )
  run_sim_command('c 4431287')
  run_sim_command('run-python-file check_privilege.py')
  run_sim_command('write-configuration ../sim_phases/%s_p1-1' %(app) )
  run_sim_command('c 2410963')
  run_sim_command('run-python-file check_privilege.py')
  run_sim_command('write-configuration ../sim_phases/%s_p1-2' %(app) )

def get_rand_simics_chkpt(app,phase):
  print "Get Random Simics checkpoint\n"
  run_sim_command('read-configuration ../sarek_chkpt/%s_%s' %(app,phase) )
  run_sim_command('c 1000000')
  run_sim_command('run-python-file check_privilege.py')
  run_sim_command('write-configuration ../sarek_phases/%s_%s' %(app,phase) )

def get_rand_cache_chkpt(app,phase):
  print "Get Random Cache Checkpoint\n"
  run_sim_command('read-configuration ../sarek_chkpt/%s_%s' %(app,phase))
  run_sim_command("instruction-fetch-mode instruction-fetch-trace")
  run_sim_command("istc-disable")
  run_sim_command("dstc-disable")
  run_sim_command('load-module ruby')
  run_sim_command('ruby0.init')
  run_sim_command('c 1000000')
  run_sim_command('run-python-file check_privilege.py')
  run_sim_command('ruby0.save-caches ../sarek_phases/%s_%s.caches.gz' %(app,phase))

def run_all(app, phase, type=-1, bit=-1, stuck=-1, faultreg=64, injinst=0, seqnum=0):
  print "Running simulation for 10M instructions\n"
  run_sim_command('read-configuration ../sarek_phases/%s_%s' % (app,phase))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('load-module ruby')
  run_sim_command('load-module opal')
  run_sim_command('ruby0.setparam g_NUM_PROCESSORS 1')
  run_sim_command('ruby0.init')
  run_sim_command('opal0.init')
  run_sim_command('ruby0.load-caches ../sarek_phases/%s_%s.caches.gz' % (app,phase) )
  run_sim_command('opal0.sim-start "/dev/null"')
  run_sim_command('opal0.fault-log "%s/%s_%s.t%s.i%s.s%s.fault_log"' % (LOG_FILE_DIR, app, phase, type, seqnum, stuck))
  run_sim_command('opal0.fault-type %s' %(type))
  run_sim_command('opal0.fault-bit %s' %(bit))
  run_sim_command('opal0.fault-stuck-at %s' %(stuck))
  run_sim_command('opal0.faulty-reg-no %s' %(faultreg))
  run_sim_command('opal0.fault-inj-inst %s' %(injinst))
  run_sim_command('opal0.sim-step %s'%(SIM_STEPS))
  run_sim_command('opal0.stats')
  run_sim_command('opal0.fault-stats')

def run_golden(app,phase):
  print "Golden run of 10M instructions\n"
  run_sim_command('read-configuration ../sarek_phases/%s_%s' % (app,phase))
  # all instr fetches visible:
  run_sim_command("instruction-fetch-mode instruction-fetch-trace")
  # disable simulation translation caches (STCs) of SIMICS:
  run_sim_command("istc-disable")
  run_sim_command("dstc-disable")
  run_sim_command('load-module ruby')
  run_sim_command('load-module opal')
  run_sim_command('ruby0.setparam g_NUM_PROCESSORS 1')
  run_sim_command('ruby0.init')
  run_sim_command('opal0.init')
  run_sim_command('ruby0.load-caches ../sarek_phases/%s_%s.caches.gz' % (app,phase) )
  run_sim_command('opal0.sim-start "/dev/null"')
  run_sim_command('opal0.fault-log "%s/%s_%s.golden.fault_log"' % (LOG_FILE_DIR,app,phase) )
  run_sim_command('opal0.sim-step %s'%(SIM_STEPS))
  run_sim_command('opal0.stats')
  run_sim_command('opal0.fault-stats')

def run_sim_command(cmd):
  print '### Executing "%s"'%cmd
  try:
    # run() returns None is no error occured
    return run(cmd)
  except:
    run("quit 666")
  return


  # NOTES:
  # a module is a file containing python definitions and statements - the file name is module name.py
  # within this file the name is available as the value of the global variable __name__
  # use import to import the defs 
  # the imported module names are placed in the importing module's global symbol table
  # to import all names that a module defines: from module import * (all names except those beginning with an 
  # underscore)

  # when a module m is imported, the interpreter searches for a file name called m.py in the current directory
  # then in  the dirs specified by PYTHONPATH, then the default path for python
  # modules are searched in the list of directories given by the variable sys.path
  # the script should not have the same name as the std module - it is on the search path
  # if a file m.pyc exists for a file m.py, this is assumed to contain a byte compiled version of the module m

  # the modification time for m.py when m.pyc was created is recorded, should a mismatch found, 
  # the .pyc file gets ignored
  # whenever the .py file is successfully compiled, a .pyc file is tried to be automatically generated
  # the speed-up in using .pyc files stems from faster loading, the program itself does not run faster

  # std modules -> sys
  # 		->os ...
