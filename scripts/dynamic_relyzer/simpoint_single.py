import time, sim_commands, string, sys, getpass, os, filecmp, random, struct
from cli import *
from collections import deque


OPT_LEVEL = "fully_optimized"
APP_DETECTORS = 0
ARCH_INJ = 1 # ARCH_INJ injects fault directly in simics, without using opal
MICROARCH_INJECTIONS = 0 # injects faults in microarch units
ISO_OUTPUT="coverage_2" # Output ISO name, used for golden output comparison
BASE_CHECKPOINT = "/home/venktgr2/checkpts/opensolaris_intel_chkpt/optimized_apps/fully_optimized/apps_copied" # Base checkpoint, used to make subsequent chkpts

if OPT_LEVEL == "fully_optimized" :
  WORKLOADS_PATH = os.environ.get('APPROXILYZER') + "/workloads/"
  CHECKPOINT_DIR = os.environ.get('APPROXILYZER') + "/workloads/checkpoints/"
#  MAIN_LOG_DIR = "./fully_optimized_main_logs_new/"
  #MAIN_LOG_DIR = "./abdul_fully_optimized_main_logs_new/"
#  MAIN_LOG_DIR = "./fully_optimized_main_logs_isca_2016/"
#  MAIN_LOG_DIR = "./output_log_input_independence/"
elif OPT_LEVEL == "optimized" :
  MAIN_LOG_DIR = "./optimized_main_logs/"
  CHECKPOINT_DIR = "/home/sadve/shari2/GEMS/simics/home/opensolaris_intel_chkpt/optimized_apps"
elif OPT_LEVEL == "unopt" :
  MAIN_LOG_DIR = "./unopt_main_logs/"
#  CHECKPOINT_DIR = "/home/sadve/shari2/GEMS/simics/home/opensolaris_intel_chkpt/unopt"
  CHECKPOINT_DIR = "/shared/workspace/amahmou2/Relyzer_GEMS/src_GEMS/simics/checkpoints/"
# else:
#   #CHECKPOINT_DIR = "/nfshome/sshari/research/GEMS/simics/home/checkpoints/"
#   CHECKPOINT_DIR = "/home/sadve/shari2/GEMS/simics/home/opensolaris_intel_chkpt"
#   MAIN_LOG_DIR = "./main_logs"

CHKPT_FILE_DIR = "/scratch/amahmou2/chkpt"
COMPARE_CHKPT_DIR = "/mounts/sadve/tmp"
COMPARE_GOLDEN_CHKPT_DIR = "/scratch/amahmou2/compare_golden_checkpoints"
CRAFF_DIR = "/scratch/amahmou2/craff"
LOCAL_OUTPUT_DIR = "/scratch/amahmou2/logs"
CRAFF_BIN = "/home/shari2/GEMS/simics-3.0.31/bin/craff"
LOG_DIR = "./logs"
OUTPUT_DIR = "./logs"
CONTROL_PATTERN_LEN = 5000
#CONTROL_PATTERN_LEN = 500
CONTROL_DEPTH = 5
CONTROL_PATTEN_CAP = 50
if MICROARCH_INJECTIONS == 1:
  SIM_STEPS = 10000000
  INJ_INST = 1000000
else:
  SIM_STEPS = 100
  INJ_INST = 50
print 'STARTING SIMULATION'
print CHECKPOINT_DIR
#print hex(app_txt_info.actual_app_start['blackscholes_simlarge_abdul'])

if os.access(CHECKPOINT_DIR,os.F_OK) == 0:
  print '%s does not exist!'%(CHECKPOINT_DIR)

def get_app_txt_info(app_name):
    app_path = WORKLOADS_PATH + "/apps/" + app_name
    print "app_path is:" + app_path
    sys.path.insert(0,app_path)
    import app_txt_info
    return app_txt_info


def get_main_log_dir(app_name):
    log_file = WORKLOADS_PATH + "/apps/" + app_name + "/analysis_output/"
    return log_file

def run_sim_command(cmd):
# print '### Executing "%s"'%cmd
  try:
    # run() returns None is no error occured
    return run(cmd)
  except:
    run("quit 666")
  return

def run_inside_safe(command, filename):
  conf.con0.input = command
  start_cycle = SIM_cycle_count(conf.cpu0) # for timeout detection
  run('c 100_000_000_000')
  end_cycle = SIM_cycle_count(conf.cpu0) # for timeout detection
  if(end_cycle-start_cycle >= 100000000000):
    FILE = open(filename,"w")
    FILE.write('Detected Timeout\n')
    FILE.write('Later')
    FILE.close()
    print "Timeout"
    run('quit 666')

def run_inside(command):
  conf.con0.input = command
  start_cycle = SIM_cycle_count(conf.cpu0) # for timeout detection
  run('c 100_000_000_000')
  end_cycle = SIM_cycle_count(conf.cpu0) # for timeout detection
  if(end_cycle-start_cycle >= 100000000000):
    print "Timeout"
    run('quit 666')


# profile the application for store-load pattern
def profile(app, type, fn=-1, ed=-1):
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  MAIN_LOG_DIR = get_main_log_dir(app)
  print 'PROFILING %s' % type
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  # run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  run_sim_command('load-module emerald')
  if type == 0 :
    run_sim_command('emerald0.set-log-file %s/%s-%s.output'%(MAIN_LOG_DIR,app,"stack"))
    #run_sim_command('emerald0.set-log-file /dev/null')
    run_sim_command('emerald0.set-inj-log-file %s/%s-%s-inj.output'%(MAIN_LOG_DIR,app,"stack"))
    run_sim_command('emerald0.set-classes-log-file %s/%s-%s-classes.output'%(MAIN_LOG_DIR,app,"stack"))
    run_sim_command('emerald0.set-spprofile-log-file %s/%s-%s-spprofile.output'%(MAIN_LOG_DIR,app,"stack"))
    print 'generated files. analyzing stack'
    run_sim_command('emerald0.analyze-stack')
  else:
    if fn >= 0 :
      if ed >= 0 :
        run_sim_command('emerald0.set-log-file %s/%s-%s-%s-%s.output'%(MAIN_LOG_DIR,app,"heap",fn,ed))
        run_sim_command('emerald0.set-inj-log-file %s/%s-%s-%s-%s-inj.output'%(MAIN_LOG_DIR,app,"heap",fn,ed))
        run_sim_command('emerald0.set-classes-log-file %s/%s-%s-%s-%s-classes.output'%(MAIN_LOG_DIR,app,"heap",fn,ed))
        run_sim_command('emerald0.set-spprofile-log-file %s/%s-%s-%s-%s-spprofile.output'%(MAIN_LOG_DIR,app,"heap",fn,ed))
        run_sim_command('emerald0.analyze-heap')
      else:
        run_sim_command('emerald0.set-log-file %s/%s-%s-%s.output'%(MAIN_LOG_DIR,app,"heap",fn))
        run_sim_command('emerald0.set-inj-log-file %s/%s-%s-%s-inj.output'%(MAIN_LOG_DIR,app,"heap",fn))
        run_sim_command('emerald0.set-classes-log-file %s/%s-%s-%s-classes.output'%(MAIN_LOG_DIR,app,"heap",fn))
        run_sim_command('emerald0.set-spprofile-log-file %s/%s-%s-%s-spprofile.output'%(MAIN_LOG_DIR,app,"heap",fn))
        run_sim_command('emerald0.analyze-heap')

    else:
      run_sim_command('emerald0.set-log-file %s/%s-%s.output'%(MAIN_LOG_DIR,app,"heap"))
      #run_sim_command('emerald0.set-log-file /dev/null')
      run_sim_command('emerald0.set-inj-log-file %s/%s-%s-inj.output'%(MAIN_LOG_DIR,app,"heap"))
      run_sim_command('emerald0.set-classes-log-file %s/%s-%s-classes.output'%(MAIN_LOG_DIR,app,"heap"))
      run_sim_command('emerald0.set-spprofile-log-file %s/%s-%s-spprofile.output'%(MAIN_LOG_DIR,app,"heap"))
      print 'generated files. analyzing heap'
      run_sim_command('emerald0.analyze-heap')


  run_sim_command('emerald0.text-start %s'%(app_txt_info.text_start[app]))
  run_sim_command('emerald0.text-end %s'%(app_txt_info.text_end[app]))
  if app == "mcf_test" and type == 1:
#  if app == "mcf_opt_abdul" and type == 1:
    print 'inside mcf_test'
    gap = (app_txt_info.text_end[app] - app_txt_info.text_start[app])/256
    print hex(gap)

    if fn >= 0 :
      local_points = []
      for local_point in range(app_txt_info.text_start[app], app_txt_info.text_end[app], gap):
        local_points.append(local_point)
      print "len local_points = %d" %(len(local_points))

      text_local_start = local_points[fn]
      text_local_start = ( ( text_local_start / 4 ) * 4 ) + 4
      text_local_end = local_points[fn] + gap

      if ed >= 0 :
        ed_gap = (text_local_end - text_local_start)/8
        print hex(ed_gap)
        ed_local_points = []
        for local_point in range(text_local_start, text_local_end, ed_gap):
          ed_local_points.append(local_point)
        print "len ed_local_points = %d" %(len(ed_local_points))

        text_local_start = ed_local_points[ed]
        text_local_start = ( ( text_local_start / 4 ) * 4 ) + 4
        text_local_end = ed_local_points[ed] + ed_gap

    # print "record start=%x " %(text_local_start)
    # print "record end=%x " %(text_local_end)
    run_sim_command('emerald0.record-text-start %s'%(text_local_start)) # this should follow text-start becuase text-start overwrites record-text-start's value
    run_sim_command('emerald0.record-text-end %s'%(text_local_end))

  run_sim_command('c')
  run_sim_command('emerald0.print-heap-use-counts')
  run("quit 666")


# print branch record information
# this information will be used to fill holed in the CFG generated with the disassembled binary
def br_profile(app):
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  MAIN_LOG_DIR = get_main_log_dir(app)
# def profile(app, type):
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  # run_sim_command('con0.capture-start  br_prof.test.txt')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  run_sim_command('new-branch-recorder br_rec virtual')
  run_sim_command('cpu0.attach-branch-recorder br_rec')
  run_sim_command('c')
  run_sim_command('br_rec.print-branch-arcs start = %s stop = %s'%(app_txt_info.text_start[app], app_txt_info.text_end[app]))
  # run_sim_command('con0.capture-stop')
  run("quit 666")


pc_index = {}
pc_cycles = {}
pc_cycles_t = {}
pc_cycles_nt = {}
pc_index_t = {}
pc_index_nt = {}

prev_pc = 0
prev_cycle = 0

def hap_cycle_count(args, obj, curr_cycle_count):
  pc=SIM_get_program_counter(conf.cpu0)
  #print '%s %s'%(pc, curr_cycle_count)
  if(curr_cycle_count == prev_cycle+2):
    if(prev_pc == pc-8):
      pc_cycles_nt[prev_pc][pc_index_nt[prev_pc]] = curr_cycle_count-2
      pc_index_nt[prev_pc] += 1
      #print "Not Taken"
    else:
      pc_cycles_t[prev_pc][pc_index_t[prev_pc]] = curr_cycle_count-2
      pc_index_t[prev_pc] += 1
      #print "Taken"

hap_id = -1

def dummy_fn(args, obj, curr_cycle_count):
  pass

def branch_taken_check(args, mem, access, brknum, reg, size):
  global prev_cycle
  global prev_pc
  global hap_id
  pc=SIM_get_program_counter(conf.cpu0)
  cycle=SIM_cycle_count(conf.cpu0)
  #print 'b %s %s'%(pc, cycle)
  #print 'adding breakpoint'
  next_cycle = cycle+2
  SIM_hap_delete_callback_id("Core_Cycle_Count", hap_id)
  hap_id = SIM_hap_add_callback_index("Core_Cycle_Count", hap_cycle_count, cycle, next_cycle)
  prev_pc = pc
  prev_cycle = cycle
  check_heart_beat(cycle)

def write_results_to_log_new(app):
  print app
  f_t_filename = 'logs/' + app + '_t.txt'
  f_nt_filename = 'logs/' + app + '_nt.txt'
  f_t = open(f_t_filename,'w')
  f_nt = open(f_nt_filename,'w')

  for i in range(0,pc_list_size[app]):
    local_pc = pc_list[app][i]
    f_t.write('%s: %s :'%(hex(local_pc),pc_index_t[local_pc]))
    for j in range(0,pc_index_t[local_pc]):
      f_t.write(' %s'%pc_cycles_t[local_pc][j])
    f_t.write('\n')

    f_nt.write('%s: %s :'%(hex(local_pc),pc_index_nt[local_pc]))
    for j in range(0,pc_index_nt[local_pc]):
      f_nt.write(' %s'%pc_cycles_nt[local_pc][j])
    f_nt.write('\n')

global_magic = 0
def hap_magic_call_reached(args, cpu, params):
  global global_magic
  print "Magic Break Called"
  global_magic = 1

def hap_test_magic(args, cpu, params):
  global global_magic
  print "Magic Break Called"
  write_results_to_log_new(args)
  global_magic = 1


def test_hap(app, type):
  global global_magic
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  for i in range(0,pc_list_size[app]):
    #pc_index_t[blackscholes_brpc_list.blackscholes_br_pc[i]] = 0
    #pc_index_nt[blackscholes_brpc_list.blackscholes_br_pc[i]] = 0
    #pc_cycles_t[blackscholes_brpc_list.blackscholes_br_pc[i]] = {}
    #pc_cycles_nt[blackscholes_brpc_list.blackscholes_br_pc[i]] = {}
    pc_index_t[pc_list[app][i]] = 0
    pc_index_nt[pc_list[app][i]] = 0
    pc_cycles_t[pc_list[app][i]] = {}
    pc_cycles_nt[pc_list[app][i]] = {}
    SIM_breakpoint(conf.primary_context,
                         Sim_Break_Virtual,
                         Sim_Access_Execute,
                         pc_list[app][i],
                         4,
                         Sim_Breakpoint_Simulation)
  SIM_hap_add_callback("Core_Breakpoint", branch_taken_check, app)
  SIM_hap_add_callback("Core_Magic_Instruction", hap_test_magic, app)
  global hap_id
  cycle=SIM_cycle_count(conf.cpu0)
  hap_id = SIM_hap_add_callback_index("Core_Cycle_Count", dummy_fn, app, cycle+1)
  run_sim_command('c')

def test_simple(app, type):
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  for i in range(0,10000):
    run_sim_command('c 1')


def branch_taken_counter(args, mem, access, brknum, reg, size):
  pc=SIM_get_program_counter(conf.cpu0)
  pc_index[pc] += 1

def hap_magic_prof(args, cpu, params):
  print "magic break called"
  file_dump(args)

def file_dump(app):
  prof_filename = 'logs/' + app + '-prof.txt'
  prof_f = open(prof_filename,'w')
  for i in range(0,pc_list_size[app]):
    local_pc = pc_list[app][i]
    prof_f.write('0x%x:%s'%(local_pc,pc_index[local_pc]))
    prof_f.write('\n')

def instruction_profiling(app,type):
  global global_magic
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  for i in range(0,pc_list_size[app]):
    pc_index[pc_list[app][i]] = 0
    brk_id = SIM_breakpoint(conf.primary_context,
                         Sim_Break_Virtual,
                         Sim_Access_Execute,
                         pc_list[app][i],
                         4,
                         Sim_Breakpoint_Simulation)

  SIM_hap_add_callback("Core_Breakpoint", branch_taken_counter, app)
  SIM_hap_add_callback("Core_Magic_Instruction", hap_magic_prof, app)
  run_sim_command('c')


# Functions to create the sample cycle counts

pc_cycle_list = {}
heart_beat = 0
global_time=0
def check_heart_beat(curr_cycle):
  global heart_beat
  global global_time
  if(curr_cycle > heart_beat + 500000):
    heart_beat = curr_cycle
    temp_time = time.clock()
    time_diff = temp_time - global_time
    global_time = temp_time
    print '%s:%d'%(curr_cycle,time_diff)


def cycle_count_hap(args, mem, access, brknum, reg, size):
  rand = random.random()
  index = brknum-1
  sample_num = rand*cycle_population[args][index]
  curr_cycle = SIM_cycle_count(conf.cpu0)
  if(sample_num <= cycle_sample_size[args][index]):
	  pc_cycle_list[cycle_pc_list[args][index]].append(curr_cycle)
          if(cycle_pc_list[args][index] == int('0x1000057ac', 16)):
  	    print '%s %s'%(sample_num,cycle_sample_size[args][index])
 	    #print '%s %s'%(cycle_pc_list[args][index], int('0x1000057ac', 16))
  check_heart_beat(curr_cycle)


def hap_magic_cycle_count(args, cpu, params):
  print "magic break called"
  app = args
  sample_out_filename = 'logs/' + app + '-samples.txt'
  sample_out_f = open(sample_out_filename,'w')
  for i in range(0,len(cycle_pc_list[app])):
    local_pc = cycle_pc_list[app][i]
    sample_out_f.write('%s: %s :'%(hex(local_pc),len(pc_cycle_list[local_pc])))
    for j in range(0,len(pc_cycle_list[local_pc])):
      sample_out_f.write(' %s'%(pc_cycle_list[local_pc][j]))
    sample_out_f.write('\n')
    # print '%s: %s :'%(hex(local_pc),len(pc_cycle_list[local_pc]))
    # for j in range(0,len(pc_cycle_list[local_pc])):
    #   print ' %s'%pc_cycle_list[local_pc][j]


def cycle_selection(app,type):
  global global_magic
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  heart_beat = SIM_cycle_count(conf.cpu0)
  for i in range(0,len(cycle_pc_list[app])):
    pc_cycle_list[cycle_pc_list[app][i]] = []
    brk_id = SIM_breakpoint(conf.primary_context,
                         Sim_Break_Virtual,
                         Sim_Access_Execute,
                         cycle_pc_list[app][i],
                         4,
                         Sim_Breakpoint_Simulation)
#    print brk_id
#    print cycle_sample_size[app][i]
#    print cycle_population[app][i]

  SIM_hap_add_callback("Core_Breakpoint", cycle_count_hap, app)
  SIM_hap_add_callback("Core_Magic_Instruction", hap_magic_cycle_count, app)
  run_sim_command('c')


# Tests

def hap_cycle_count_test(args, obj, curr_cycle_count):
  global hap_id
#  print '%s %s'%(hap_id, curr_cycle_count)
#  cycle=SIM_cycle_count(conf.cpu0)
  next_cycle = curr_cycle_count+1
  pc=SIM_get_program_counter(conf.cpu0)
  priv_level=SIM_processor_privilege_level(conf.cpu0)
  reg_num=SIM_get_register_number(conf.cpu0,"g0")
  reg_val=SIM_read_register(conf.cpu0,reg_num)
  reg_num=SIM_get_register_number(conf.cpu0,"g1")
  reg_val=SIM_read_register(conf.cpu0,reg_num)
  reg_num=SIM_get_register_number(conf.cpu0,"g2")
  reg_val=SIM_read_register(conf.cpu0,reg_num)
  reg_num=SIM_get_register_number(conf.cpu0,"g3")
  reg_val=SIM_read_register(conf.cpu0,reg_num)
  reg_num=SIM_get_register_number(conf.cpu0,"g4")
  reg_val=SIM_read_register(conf.cpu0,reg_num)
  reg_num=SIM_get_register_number(conf.cpu0,"g5")
  reg_val=SIM_read_register(conf.cpu0,reg_num)
  reg_num=SIM_get_register_number(conf.cpu0,"g6")
  reg_val=SIM_read_register(conf.cpu0,reg_num)
  reg_num=SIM_get_register_number(conf.cpu0,"g7")
  reg_val=SIM_read_register(conf.cpu0,reg_num)
  SIM_hap_delete_callback_id("Core_Cycle_Count", hap_id)
  hap_id = SIM_hap_add_callback_index("Core_Cycle_Count", hap_cycle_count_test, curr_cycle_count, next_cycle)

def test_timing_hap(app,type):
  global global_magic
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  cycle=SIM_cycle_count(conf.cpu0)
  hap_id = SIM_hap_add_callback_index("Core_Cycle_Count", hap_cycle_count_test, app, cycle+1)
  run_sim_command('c 10_000_000')

def test_timing_c1(app):
  global global_magic
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  for i in range(0,10000000):
    pc=SIM_get_program_counter(conf.cpu0)
    SIM_continue(1)

check_dict = {}
init_dict = {}
invariants = []
invariant_counters = [0,0]
def hap_invariants_check(args, cpu, access, brknum, reg, size):
  import invariant_detectors
  if(args == "blackscholes_simlarge" or args == "blackscholes_input_run_00000"
          or args == "blackscholes_input_run_00001"
          or args == "blackscholes_input_run_00002"
          or args == "blackscholes_input_run_00003"
          or args == "blackscholes_input_run_00004"
          ):
  #if(args == "blackscholes_opt_abdul"):
    check_dict = invariant_detectors.blackscholes_check;
    init_dict = invariant_detectors.blackscholes_initialize;
  if(args == "swaptions_simsmall"):
  #if(args == "swaptions_opt_abdul"):
    check_dict = invariant_detectors.swaptions_check;
    init_dict = invariant_detectors.swaptions_initialize;
  if(args == "lu_reduced"):
    check_dict = invariant_detectors.lu_check;
    init_dict = invariant_detectors.lu_initialize;
  if(args == "fft_small"):
  #if(args == "fft_opt_abdul"):
    check_dict = invariant_detectors.fft_check;
    init_dict = invariant_detectors.fft_initialize;
  print "AND I THOUGHT THIS WAS USELESS!"
  # get pc and check if it lies in check_dict or init_dict
  pc = SIM_get_program_counter(conf.cpu0)

  global invariants
  if pc in init_dict:
    del invariants[:]
    # compute the invariant
    for t in init_dict[pc]:
      if len(t) == 3:
        invariant_counters[0] += 1
        reg1 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, t[0]))
        reg2 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, t[2]))
        invariants.append(reg1)
        invariants.append(reg2)
        print "0x%x, reg1=%d, reg2=%d" %(pc,reg1,reg2)
  if pc in check_dict:
    if len(invariants) != 0:
      # check the invariant
      for t in check_dict[pc]:
        if len(t) == 3:
          invariant_counters[1] += 1
          reg1 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, t[0]))
          reg2 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, t[2]))
          print "0x%x, reg1=%d, reg2=%d" %(pc,reg1,reg2)
          denom = invariants.pop(0) - reg1
          num = invariants.pop(0) - reg2
          factor = denom/num
          print "factor=%d, given=%d" %(factor,t[1])
          # test if the value is same
          if factor == t[1]:
            print "invariants matched at 0x%x" %pc
          else:
            print "invariants DIDN'T match at 0x%x" %pc

def add_invariant_checks(app):
  import invariant_detectors
  if(app == "blackscholes_simlarge" or app == "blackscholes_input_run_00000"
          or app == "blackscholes_input_run_00001"
          or app == "blackscholes_input_run_00002"
          or app == "blackscholes_input_run_00003"
          or app == "blackscholes_input_run_00004"
          ):
  #if(args == "blackscholes_opt_abdul"):
    check_dict = invariant_detectors.blackscholes_check;
    init_dict = invariant_detectors.blackscholes_initialize;
  if(app == "swaptions_simsmall"):
  #if(app == "swaptions_opt_abdul"):
    check_dict = invariant_detectors.swaptions_check;
    init_dict = invariant_detectors.swaptions_initialize;
  if(app == "lu_reduced"):
    check_dict = invariant_detectors.lu_check;
    init_dict = invariant_detectors.lu_initialize;
  if(app == "fft_small"):
  #if(args == "fft_opt_abdul"):
    check_dict = invariant_detectors.fft_check;
    init_dict = invariant_detectors.fft_initialize;
  for pc in check_dict:
    print pc
    SIM_breakpoint(conf.primary_context,
                  Sim_Break_Virtual,
                  Sim_Access_Execute,
                  pc,
                  4,
                  Sim_Breakpoint_Simulation)
  for pc in init_dict:
    print pc
    SIM_breakpoint(conf.primary_context,
                  Sim_Break_Virtual,
                  Sim_Access_Execute,
                  pc,
                  4,
                  Sim_Breakpoint_Simulation)
  print "AND I THOUGHT THIS WAS USELESS2!"
  SIM_hap_add_callback("Core_Breakpoint", hap_invariants_check, app)
  print "AND I THOUGHT THIS WAS USELESS3!"


def test_invariant_checks(app):
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  add_invariant_checks(app)
  run_sim_command('c 1_000_000_000')
  print "invariant_counters = ",
  print invariant_counters

def run_new_all(app, phase, seqnum, pc, core=-1,  type=-1, bit=-1, stuck=-1, faultreg=64, srcdest=0, pilot=0, injcycle=0):
  #print 'app=%s, phase=%s, seqnum=%s, pc=%s, core=%s,  type=%s, bit=%s, stuck=%s, faultreg=%s, srcdest=%s, injcycle=%s'%(app, phase, seqnum, pc, core,  type, bit, stuck, faultreg, srcdest, injcycle)
#  start_points = [ 355315684499]

  # These points are for the optimized apps (with -O2 option)
  # blackscholes
  # if(app == "blackscholes_simlarge"):
  #   start_points = [ 310623771176, 310723771176, 310823771176, 310923771176, 311023771176, 311123771176, 311223771176, 311323771176 ]
  # if(app == "blackscholes_simmedium"):
  #   start_points = [ 247580521494, 247680521494, 247780521494, 247880521494, 247980521494, 248080521494, 248180521494, 248280521494, 248380521494, 248480521494 ]
  # if(app == "swaptions_simsmall"):
  #   start_points = [ 225858731234, 225958731234, 226058731234, 226158731234, 226258731234, 226358731234, 226458731234, 226558731234, 226658731234, 226758731234 ]
  # if(app == "lu_reduced"):
  #   start_points = [ 214099802406,  214199802406,  214299802406,  214399802406, 214499802406,  214599802406, 214699802406, 214799802406, 214899802406, 214999802406, 215099802406 ]
  # # fft
  # if(app == "fft_small"):
  #   start_points = [ 217289762830,  217389762830,  217489762830,  217589762830, 217689762830,  217789762830,  217889762830,  217989762830,  218089762830, 218189762830,  218289762830,  218389762830,  218489762830,  218589762830, 218689762830,  218789762830,  218889762830,  218989762830,  219089762830, 219189762830,  219289762830, 219389762830, 219489762830, 219589762830, 219689762830, 219789762830, 219889762830, 219989762830, 220089762830, 220189762830, 220289762830, 220389762830, 220489762830, 220589762830, 220689762830, 220789762830, 220889762830, 220989762830, 221089762830, 221189762830, 221289762830, 221389762830, 221489762830, 221589762830, 221689762830, 221789762830, 221889762830, 221989762830, 222089762830, 222189762830, 222289762830, 222389762830, 222489762830, 222589762830, 222689762830, 222789762830 ]
  # if(app == "libquantum_test"):
  #   start_points = [ 175836406789, 175936406789, 176036406789, 176136406789, 176236406789, 176336406789, 176436406789, 176536406789, 176636406789, 176736406789, 176836406789, 176936406789, 177036406789, 177136406789, 177236406789, 177336406789, 177436406789, 177536406789, 177636406789, 177736406789, 177836406789, 177936406789, 178036406789, 178136406789, 178236406789, 178336406789, 178436406789, 178536406789, 178636406789, 178736406789, 178836406789, 178936406789, 179036406789, 179136406789, 79236406789, 179336406789, 179436406789, 179536406789, 179636406789, 179736406789, 179836406789, 179936406789, 180036406789, 180136406789, 180236406789, 180336406789, 180436406789, 180536406789, 180636406789, 180736406789, 180836406789, 180936406789, 181036406789, 181136406789, 181236406789, 181336406789, 181436406789, 181536406789, 181636406789, 181736406789, 181836406789, 181936406789, 182036406789, 182136406789, 182236406789, 182336406789, 182436406789, 182536406789, 182636406789, 182736406789, 182836406789, 182936406789, 183036406789, 183136406789, 183236406789, 183336406789, 183436406789, 183536406789, 183636406789, 183736406789, 183836406789, 183936406789, 184036406789, 184136406789, 184236406789, 184336406789, 184436406789, 184536406789, 184636406789, 184736406789, 184836406789, 184936406789, 185036406789, 185136406789 ]
  # if(app == "omnetpp_test"):
  #   start_points = [ 250989924297,  251089924297,  251189924297,  251289924297, 251389924297,  251489924297,  251589924297,  251689924297,  251789924297, 251889924297,  251989924297,  252089924297, 252189924297, 252289924297, 252389924297, 252489924297, 252589924297, 252689924297, 252789924297, 252889924297, 252989924297, 253089924297, 253189924297 ]

  # These points are for the fully optimized apps (with -fast option)
  # blackscholes
  #if(app == "blackscholes_opt_abdul"):
  #  start_points = [355315684499, 355415684499, 355515684499, 355615684499, 355715684499, 355815684499, 355915684499, 356015684499 ]
  if(app == "blackscholes_simlarge"):
    start_points = [ 393819720646, 393919720646, 394019720646, 394119720646, 394219720646, 394319720646, 394419720646, 394519720646 ]
  #if(app == "fft_opt_abdul"):
  #    start_points = [  218024393104, 218124393104, 218224393104, 218324393104, 218424393104, 218524393104, 218624393104, 218724393104, 218824393104, 218924393104, 219024393104, 219124393104, 219224393104, 219324393104, 219424393104, 219524393104, 219624393104, 219724393104, 219824393104, 219924393104, 220024393104, 220124393104, 220224393104, 220324393104, 220424393104, 220524393104, 220624393104, 220724393104, 220824393104, 220924393104, 221024393104, 221124393104, 221224393104, 221324393104, 221424393104, 221524393104, 221624393104, 221724393104, 221824393104, 221924393104, 222024393104, 222124393104, 222224393104, 222324393104, 222424393104, 222524393104, 222624393104, 222724393104, 222824393104, 222924393104, 223024393104, 223124393104, 223224393104, 223324393104, 223424393104, 223524393104
 #             ]
  if(app == "fft_small"):
    start_points = [ 382041434832, 382141434832, 382241434832, 382341434832, 382441434832, 382541434832, 382641434832, 382741434832, 382841434832, 382941434832, 383041434832, 383141434832, 383241434832, 383341434832, 383441434832, 383541434832, 383641434832, 383741434832, 383841434832, 383941434832, 384041434832, 384141434832, 384241434832, 384341434832, 384441434832, 384541434832, 384641434832, 384741434832, 384841434832, 384941434832, 385041434832, 385141434832, 385241434832, 385341434832, 385441434832, 385541434832, 385641434832, 385741434832, 385841434832, 385941434832, 386041434832, 386141434832 ]
#  if(app == "fluidanimate_opt_abdul"):
#    start_points = [ 228145975993,228245975993,228345975993,228445975993,228545975993,228645975993,228745975993,228845975993,228945975993,229045975993,229145975993,229245975993,229345975993,229445975993,229545975993,229645975993,229745975993,229845975993,229945975993,230045975993,230145975993,230245975993,230345975993,230445975993,230545975993,230645975993,230745975993,230845975993,230945975993,231045975993,231145975993,231245975993,231345975993 ]
  if(app == "fluidanimate_simsmall"):
          start_points = [ 416643363798, 416743363798, 416843363798, 416943363798, 417043363798, 417143363798, 417243363798, 417343363798, 417443363798, 417543363798, 417643363798, 417743363798, 417843363798, 417943363798, 418043363798 ]
    #start_points = [ 384831564105, 384931564105, 385031564105, 385131564105, 385231564105, 385331564105, 385431564105, 385531564105 ]
  if(app == "streamcluster_simsmall"):
    start_points = [381661633658, 381761633658, 381861633658, 381961633658, 382061633658, 382161633658, 382261633658, 382361633658, 382461633658, 382561633658, 382661633658, 382761633658, 382861633658, 382961633658 ]
  if(app == "gcc_test"):
    start_points = [ 385574521728, 385674521728, 385774521728, 385874521728, 385974521728, 386074521728, 386174521728, 386274521728, 386374521728, 386474521728, 386574521728, 386674521728, 386774521728, 386874521728, 386974521728, 387074521728, 387174521728, 387274521728, 387374521728, 387474521728, 387574521728, 387674521728, 387774521728, 387874521728, 387974521728, 388074521728, 388174521728, 388274521728, 388374521728, 388474521728, 388574521728, 388674521728, 388774521728, 388874521728, 388974521728, 389074521728, 389174521728, 389274521728, 389374521728, 389474521728, 389574521728, 389674521728, 389774521728, 389874521728, 389974521728, 390074521728, 390174521728, 390274521728, 390374521728, 390474521728 ]
  if(app == "libquantum_test"):
    start_points = [ 381949862107, 382049862107 ]
#  if(app == "lu_opt_abdul"):
#    start_points = [ 257234280900, 257334280900,  257434280900,  257534280900,  257634280900,  257734280900,  257834280900,  257934280900,  258034280900,  258134280900 ]

  if(app == "lu_reduced"):
    start_points = [ 382101382816, 382201382816, 382301382816, 382401382816, 382501382816, 382601382816, 382701382816, 382801382816, 382901382816, 383001382816, 382101382816, 382201382816, 382301382816, 382401382816, 382501382816, 382601382816, 382701382816, 382801382816, 382901382816, 383001382816 ]
  if(app == "mcf_test"):
    start_points = [ 384422383208, 384522383208, 384622383208, 384722383208, 384822383208, 384922383208, 385022383208, 385122383208, 385222383208, 385322383208, 385422383208, 385522383208, 385622383208, 385722383208, 385822383208, 385922383208, 386022383208, 386122383208, 386222383208, 386322383208, 386422383208, 386522383208, 386622383208, 386722383208, 386822383208, 386922383208, 387022383208, 387122383208, 387222383208, 387322383208, 387422383208, 387522383208, 387622383208, 387722383208, 387822383208, 387922383208, 388022383208, 388122383208, 388222383208, 388322383208, 388422383208 ]
  if(app == "ocean_small"):
    start_points = [ 381778251864, 381878251864, 381978251864 ]
  if(app == "omnetpp_test"):
    start_points = [ 382210076576, 382310076576, 382410076576, 382510076576, 382610076576, 382710076576, 382810076576, 382910076576, 383010076576, 383110076576, 383210076576, 383310076576, 383410076576, 383510076576, 383610076576, 383710076576, 383810076576, 383910076576, 384010076576, 384110076576, 384210076576, 384310076576, 384410076576 ]

#  if(app == "swaptions_opt_abdul"): #swaptions simsmall, my version
#    start_points = [ 330627258853,  330727258853,  330827258853,  330927258853,  331027258853,  331127258853,  331227258853,  331327258853,  331427258853 ]

  if(app == "swaptions_simsmall"):
    start_points = [ 383791951276, 383891951276, 383991951276, 384091951276, 384191951276, 384291951276, 384391951276, 384491951276, 384591951276 ]
  if(app == "swaptions_simsmall_large"):
    start_points = [ 387684297307, 387784297307, 387884297307, 387984297307, 388084297307, 388184297307,
		     388284297307, 388384297307, 388484297307, 388584297307, 388684297307, 388784297307,
		     388884297307, 388984297307, 389084297307, 389184297307, 389284297307, 389384297307,
		     389484297307, 389584297307, 389684297307, 389784297307, 389884297307, 389984297307,
		     390084297307, 390184297307, 390284297307, 390384297307, 390484297307, 390584297307,
		     390684297307, 390784297307, 390884297307, 390984297307, 391084297307, 391184297307,
		     391284297307, 391384297307, 391484297307, 391584297307, 391684297307, 391784297307,
		     391884297307, 391984297307, 392084297307, 392184297307, 392284297307, 392384297307,
		     392484297307, 392584297307, 392684297307, 392784297307, 392884297307, 392984297307,
		     393084297307, 393184297307, 393284297307, 393384297307, 393484297307, 393584297307,
		     393684297307, 393784297307, 393884297307, 393984297307, 394084297307, 394184297307,
		     394284297307, 394384297307, 394484297307, 394584297307, 394684297307, 394784297307,
		     394884297307, 394984297307, 395084297307, 395184297307, 395284297307, 395384297307,
		     395484297307, 395584297307, 395684297307, 395784297307, 395884297307, 395984297307,
		     396084297307, 396184297307, 396284297307, 396384297307, 396484297307, 396584297307,
		     396684297307, 396784297307, 396884297307, 396984297307, 397084297307, 397184297307,
		     397284297307, 397384297307, 397484297307, 397584297307, 397684297307, 397784297307,
		     397884297307, 397984297307, 398084297307, 398184297307, 398284297307, 398384297307,
		     398484297307, 398584297307, 398684297307, 398784297307, 398884297307, 398984297307,
		     399084297307, 399184297307 ]
  if(app == "water_small"):
    start_points = [ 381375592368, 381475592368, 381575592368, 381675592368, 381775592368]

  if(app == "blackscholes_input_run_00000"):
      start_points = [380537629530]
  if(app == "blackscholes_input_run_00001"):
      start_points = [380537629530]
  if(app == "blackscholes_input_run_00002"):
      start_points = [380537629530]
  if(app == "blackscholes_input_run_00003"):
      start_points = [380537629530]
  if(app == "blackscholes_input_run_00004"):
      start_points = [380537629530]


  intermediate_checkpoints = WORKLOADS_PATH + "/apps/" + app + "/intermediate_checkpoints.txt"
  start_points = open(intermediate_checkpoints).readlines(); #FIX HERE! Does it need the first number??



  if APP_DETECTORS == 1:  # start from the begining of the application
    start_points = [123253189924297]

  checkpoint_gap = 100000000
  initial_skip = 1
  print 'injection cycyle = %s' % injcycle
  start_point = 0
  if injcycle < start_points[0]:
    run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
    print 'starting simulation from the begining'
    initial_skip = 1
  else:
    found_start_point = 0
    for start_point in start_points:
      if injcycle < start_point:
        found_start_point = 1
        break
    if found_start_point == 1:
      start_point = start_point - checkpoint_gap
    if injcycle - start_point < 1000:
      start_point = start_point - checkpoint_gap
    print 'starting simulation at %s' % start_point
    initial_skip = 0
    run_sim_command('read-configuration %s/intermediate/%s_%s' % (CHECKPOINT_DIR,app,start_point))

  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')

  cpu_p = [ SIM_proc_no_2_ptr(i) for i in range(1) ]
  begin = dict([(p, p.steps) for p in cpu_p])
  if APP_DETECTORS == 1:
    # add_detector_locations(app)
    import app_detectors
    # init app detectors
    app_detectors.create_detectors(app)
    app_detectors.add_detector_locations(app)
#    filename = "/dev/null"
    SIM_hap_add_callback("Core_Breakpoint", app_detectors.core_break_pc, [filename, begin, app])

  if initial_skip == 1:
    run_sim_command('c 10')

  SIM_set_attribute(conf.sim, "cpu_switch_time", 1000)
  curr_cycle = SIM_cycle_count(conf.cpu0)
  print "current cycle is: %s" % curr_cycle
  skip_cycles = injcycle - INJ_INST - curr_cycle
  print 'Skip %s instructions'%(skip_cycles)
  run_sim_command('c %d' %(skip_cycles))
  run_name = '%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s' % (app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)

  if ARCH_INJ == 1:
 #   filename = "/dev/null"
    filename = '%s/%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s.completion_output' % (LOCAL_OUTPUT_DIR, app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)
    run_sim_command('c %d' %(INJ_INST))
    inject_trans_fault(pc, faultreg, bit, srcdest, type)
    if APP_DETECTORS == 1:
      app_detectors.panic_break_num = SIM_breakpoint(conf.primary_context,
                                         Sim_Break_Virtual,
                                         Sim_Access_Execute,
                                         0x1049bac,
                                         216,
                                         Sim_Breakpoint_Simulation)
    else:
      SIM_breakpoint(conf.primary_context,
                                       Sim_Break_Virtual,
                                       Sim_Access_Execute,
                                       0x1049bac,
                                       216,
                                       Sim_Breakpoint_Simulation)
      SIM_hap_add_callback("Core_Breakpoint", core_problem_pc, [filename, begin])

    SIM_hap_add_callback("Core_Exception", core_exception, [ filename, begin ])

    run_sim_command('c %d' %(INJ_INST))
    print "Fault not detected\n"
    checkpoint_file = '%s/%s.chkpt' % (CHKPT_FILE_DIR, run_name)
    run_sim_command('write-configuration -z "%s"' % (checkpoint_file))

    if APP_DETECTORS == 1:
      import pickle
      pf = '%s/%s.p' %(CHKPT_FILE_DIR, run_name)
      of = open(pf, "wb")
      pickle.dump(app_detectors.detectors, of)
      pickle.dump(app_detectors.accum_in, of)
      pickle.dump(app_detectors.accum_out, of)
      of.close()
      print "written pickle file"

  else:
    #print "curr_cycle=%d, injcycle=%d, skip_cycles=%d"%(curr_cycle, injcycle, skip_cycles)
    run_sim_command('load-module ruby')
    run_sim_command('load-module opal')
    run_sim_command('ruby0.init')
    run_sim_command('opal0.init')
    #run_sim_command('opal0.sim-start "%s/%s.trace_log"' % (LOG_DIR, run_name))
    run_sim_command('opal0.sim-start "/dev/null"')
    #run_sim_command('opal0.fault-log "%s/%s.fault_log"' % (LOG_DIR, run_name))
    run_sim_command('opal0.fault-log "/dev/null"')
    run_sim_command('opal0.faulty-core %s'%(core))
    run_sim_command('opal0.fault-type %s' %(type))
    run_sim_command('opal0.fault-bit %s' %(bit))
    run_sim_command('opal0.fault-stuck-at %s' %(stuck))
    run_sim_command('opal0.faulty-reg-no %s' %(faultreg))
    run_sim_command('opal0.faulty-reg-srcdest %s' %(srcdest))
    run_sim_command('opal0.fault-inj-inst %s' %(INJ_INST))
    print "execute opal for next %s instructions" %(SIM_STEPS)
    run_sim_command('opal0.sim-step %s'%(SIM_STEPS))
    #run_sim_command('opal0.stats')
    run_sim_command('opal0.fault-stats')
    if (SIM_get_attribute(conf.opal0,"is-detected") == 0):
          print "Fault not detected\n"
          # validation_simics_files/blackscholes_simmedium_merged.pc0x100001bf8.c0.r18.b0.simics
          checkpoint_file = '%s/%s.chkpt' % (CHKPT_FILE_DIR, run_name)
          run_sim_command('write-configuration -z "%s"' % (checkpoint_file))
          if APP_DETECTORS == 1:
            import pickle
            pf = '%s/%s.p' %(CHKPT_FILE_DIR, run_name)
            of = open(pf, "wb")
            pickle.dump(app_detectors.detectors, of)
            pickle.dump(app_detectors.accum_in, of)
            pickle.dump(app_detectors.accum_out, of)
            of.close()
            print "written pickle file"
          if start_point != 0:
            if os.path.isfile('%s/%s_%s.merged.sd25B_2_0_image' % (CHECKPOINT_DIR, app, start_point)) == 0:
              if os.path.isfile('%s.sd25B_2_0_image' % (checkpoint_file)) == 0:
                pass
              else:
                os.system('cp %s.sd25B_2_0_image %s.merged.sd25B_2_0_image ' % (checkpoint_file, checkpoint_file))
            else:
              if os.path.isfile('%s.sd25B_2_0_image' % (checkpoint_file)) == 0:
                os.system('cp %s/%s_%s.merged.sd25B_2_0_image %s.merged.sd25B_2_0_image ' % (CHECKPOINT_DIR, app, start_point, checkpoint_file))
              else:
                os.system('%s %s/%s_%s.merged.sd25B_2_0_image %s.sd25B_2_0_image --dir-entries=1024 --compression=none -o %s.merged.sd25B_2_0_image ' % (CRAFF_BIN, CHECKPOINT_DIR, app, start_point, checkpoint_file, checkpoint_file))
    else :
          print "Fault detected!\n"

def run_sese_pot(app, phase, seqnum, pc, core=-1,  type=-1, bit=-1, stuck=-1, faultreg=64, srcdest=0, pilot=0, injcycle=0):
  #print 'app=%s, phase=%s, seqnum=%s, pc=%s, core=%s,  type=%s, bit=%s, stuck=%s, faultreg=%s, srcdest=%s, injcycle=%s'%(app, phase, seqnum, pc, core,  type, bit, stuck, faultreg, srcdest, injcycle)

  # blackscholes
  if(app == "blackscholes_simmedium"):
    start_points = [ 247580521494, 247680521494, 247780521494, 247880521494, 247980521494, 248080521494, 248180521494, 248280521494, 248380521494, 248480521494 ]
  if(app == "swaptions_simsmall"):
    start_points = [ 225858731234, 225958731234, 226058731234, 226158731234, 226258731234, 226358731234, 226458731234, 226558731234, 226658731234, 226758731234 ]
  if(app == "lu_reduced"):
    start_points = [ 214099802406,  214199802406,  214299802406,  214399802406, 214499802406,  214599802406, 214699802406, 214799802406, 214899802406, 214999802406, 215099802406 ]
  # fft
  if(app == "fft_small"):
    start_points = [ 217289762830,  217389762830,  217489762830,  217589762830, 217689762830,  217789762830,  217889762830,  217989762830,  218089762830, 218189762830,  218289762830,  218389762830,  218489762830,  218589762830, 218689762830,  218789762830,  218889762830,  218989762830,  219089762830, 219189762830,  219289762830, 219389762830, 219489762830, 219589762830, 219689762830, 219789762830, 219889762830, 219989762830, 220089762830, 220189762830, 220289762830, 220389762830, 220489762830, 220589762830, 220689762830, 220789762830, 220889762830, 220989762830, 221089762830, 221189762830, 221289762830, 221389762830, 221489762830, 221589762830, 221689762830, 221789762830, 221889762830, 221989762830, 222089762830, 222189762830, 222289762830, 222389762830, 222489762830, 222589762830, 222689762830, 222789762830 ]
  if(app == "libquantum_test"):
    start_points = [ 175836406789, 175936406789, 176036406789, 176136406789, 176236406789, 176336406789, 176436406789, 176536406789, 176636406789, 176736406789, 176836406789, 176936406789, 177036406789, 177136406789, 177236406789, 177336406789, 177436406789, 177536406789, 177636406789, 177736406789, 177836406789, 177936406789, 178036406789, 178136406789, 178236406789, 178336406789, 178436406789, 178536406789, 178636406789, 178736406789, 178836406789, 178936406789, 179036406789, 179136406789, 79236406789, 179336406789, 179436406789, 179536406789, 179636406789, 179736406789, 179836406789, 179936406789, 180036406789, 180136406789, 180236406789, 180336406789, 180436406789, 180536406789, 180636406789, 180736406789, 180836406789, 180936406789, 181036406789, 181136406789, 181236406789, 181336406789, 181436406789, 181536406789, 181636406789, 181736406789, 181836406789, 181936406789, 182036406789, 182136406789, 182236406789, 182336406789, 182436406789, 182536406789, 182636406789, 182736406789, 182836406789, 182936406789, 183036406789, 183136406789, 183236406789, 183336406789, 183436406789, 183536406789, 183636406789, 183736406789, 183836406789, 183936406789, 184036406789, 184136406789, 184236406789, 184336406789, 184436406789, 184536406789, 184636406789, 184736406789, 184836406789, 184936406789, 185036406789, 185136406789 ]
  if(app == "omnetpp_test"):
    start_points = [ 250989924297,  251089924297,  251189924297,  251289924297, 251389924297,  251489924297,  251589924297,  251689924297,  251789924297, 251889924297,  251989924297,  252089924297, 252189924297, 252289924297, 252389924297, 252489924297, 252589924297, 252689924297, 252789924297, 252889924297, 252989924297, 253089924297, 253189924297 ]
  if(app == "fft_opt_abdul"):
      print "NEED TO MAKE SOME ADJUSTMENTS!!!"
  checkpoint_gap = 100000000
  initial_skip = 1
  print 'injection cycyle = %s' % injcycle
  start_point = 0
  if injcycle < start_points[0]:
    run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
    print 'starting simulation from the begining'
    initial_skip = 1
  else:
    for start_point in start_points:
      if injcycle < start_point:
        break
    start_point = start_point - checkpoint_gap
    if injcycle - start_point < 1000:
      start_point = start_point - checkpoint_gap
    print 'starting simulation at %s' % start_point
    initial_skip = 0
    run_sim_command('read-configuration %s/intermediate/%s_%s' % (CHECKPOINT_DIR,app,start_point))

  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  SIM_set_attribute(conf.sim, "cpu_switch_time", 1)
  if initial_skip == 1:
    run_sim_command('c 10')
  curr_cycle = SIM_cycle_count(conf.cpu0)
  skip_cycles = injcycle - INJ_INST - curr_cycle
  print 'Skip %s instructions'%(skip_cycles)
  run_sim_command('c %d' %(skip_cycles))
  #print "curr_cycle=%d, injcycle=%d, skip_cycles=%d"%(curr_cycle, injcycle, skip_cycles)
  run_sim_command('load-module ruby')
  run_sim_command('load-module opal')
  run_sim_command('ruby0.init')
  run_sim_command('opal0.init')
  run_name = '%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s' % (app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)
  run_sim_command('opal0.sim-start "%s/%s.trace_log"' % (LOG_DIR, run_name))
  run_sim_command('opal0.fault-log "%s/%s.fault_log"' % (LOG_DIR, run_name))
  run_sim_command('opal0.faulty-core %s'%(core))
  run_sim_command('opal0.fault-type %s' %(type))
  run_sim_command('opal0.fault-bit %s' %(bit))
  run_sim_command('opal0.fault-stuck-at %s' %(stuck))
  run_sim_command('opal0.faulty-reg-no %s' %(faultreg))
  run_sim_command('opal0.faulty-reg-srcdest %s' %(srcdest))
  run_sim_command('opal0.fault-inj-inst %s' %(INJ_INST+1))
  run_sim_command('opal0.start-logging %s' %(start_logging))
  run_sim_command('opal0.compare-point %s' %(compare_point))
  run_sim_command('opal0.sim-step %s'%(SIM_STEPS))
  # run_sim_command('opal0.stats')
  run_sim_command('opal0.fault-stats')
  if (SIM_get_attribute(conf.opal0,"is-detected") == 0):
        print "Fault not detected\n"
	# validation_simics_files/blackscholes_simmedium_merged.pc0x100001bf8.c0.r18.b0.simics
	checkpoint_file = '%s/%s.chkpt' % (CHKPT_FILE_DIR, run_name)
        run_sim_command('write-configuration -z "%s"' % (checkpoint_file))
	if start_point != 0:
	  if os.path.isfile('%s/%s_%s.merged.sd25B_2_0_image' % (CHECKPOINT_DIR, app, start_point)) == 0:
	    if os.path.isfile('%s.sd25B_2_0_image' % (checkpoint_file)) == 0:
	      pass
	    else:
	      os.system('cp %s.sd25B_2_0_image %s.merged.sd25B_2_0_image ' % (checkpoint_file, checkpoint_file))
	  else:
	    if os.path.isfile('%s.sd25B_2_0_image' % (checkpoint_file)) == 0:
              os.system('cp %s/%s_%s.merged.sd25B_2_0_image %s.merged.sd25B_2_0_image ' % (CHECKPOINT_DIR, app, start_point, checkpoint_file))
	    else:
	      os.system('%s %s/%s_%s.merged.sd25B_2_0_image %s.sd25B_2_0_image --dir-entries=1024 --compression=none -o %s.merged.sd25B_2_0_image ' % (CRAFF_BIN, CHECKPOINT_DIR, app, start_point, checkpoint_file, checkpoint_file))
  else :
        print "Fault detected!\n"


priv_range = ""

def hap_mode_change_info(args, cpu, old_mode, new_mode):
  global priv_range
  MAIN_LOG_DIR = get_main_log_dir(args)
  curr_cycle = SIM_cycle_count(cpu)
  if(new_mode == 0): # entering user mode
    priv_range += '-%d'%(curr_cycle)
    print priv_range
    filename = MAIN_LOG_DIR + args + '_modes.txt'
    pf = open(filename,'a')
    pf.write(priv_range)
    pf.write('\n')
    pf.close()
  else:
    priv_range = '%d'%(curr_cycle)

def hap_mode_magic(args, cpu, params):
  print 'magic called'

def mode_change_info(app):
  global priv_range
  MAIN_LOG_DIR = get_main_log_dir(app)
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')


  # reset the file content
  filename = MAIN_LOG_DIR + app + '_modes.txt'
  pf = open(filename,'w')
  pf.write('\n')
  pf.close()

  curr_mode = SIM_processor_privilege_level(conf.cpu0)
  if(curr_mode == 1):
    curr_cycle = SIM_cycle_count(conf.cpu0)
    priv_range = '%d' %(curr_cycle)
  SIM_hap_add_callback("Core_Mode_Change", hap_mode_change_info, app)
  SIM_hap_add_callback("Core_Magic_Instruction", hap_mode_magic, app)
  run_sim_command('c')


exception_count = {}
def hap_exception_info(args, cpu, exp_num):
  exp_str = str(hex(exp_num))
  if exp_str in exception_count:
    exception_count[exp_str] += 1
  else:
    exception_count[exp_str] = 1

def exception_info (app):
  global priv_range
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  SIM_hap_add_callback("Core_Exception", hap_exception_info, app)
  SIM_hap_add_callback("Core_Magic_Instruction", hap_mode_magic, app)
  run_sim_command('c')
  for e in exception_count:
    print e + ":" + str(exception_count[e])

exception_code = [0x02, 0x05, 0x0a,
                  0x08, 0x10, 0x28,
		  0x29, 0x30, 0x32,
		  0x34, 0x35, 0x36,
		  0x38, 0x39, 0x40]

def compare_disk_image(start_checkpoint, curr_checkpoint, golden_checkpoint):
  is_equal = 0
  start_image = '%s.merged.sd25B_2_0_image' % start_checkpoint
  curr_image = '%s.sd25B_2_0_image' % curr_checkpoint
  golden_image = '%s.sd25B_2_0_image' % golden_checkpoint
  if os.path.isfile(golden_image) == 1:
    if os.path.isfile(start_image) == 1:
      #print "found start_image file"
      if os.path.isfile(curr_image) == 1:
        #print "found curr_image file"
        os.system('%s %s %s --dir-entries=1024 --compression=none -o %s.merged.sd25B_2_0_image ' % (CRAFF_BIN, start_image, curr_image, curr_checkpoint))
        cmp_image = '%s.merged.sd25B_2_0_image' % (curr_checkpoint)
      else:
        #print "not found curr_image file"
        cmp_image = start_image
    else:
      #print "not found start_image file"
      if os.path.isfile(curr_image) == 1:
        #print "found curr_image file"
        cmp_image = curr_image
      else:
        print "not found curr_image file: Not possible"
	is_equal = 0

    if(filecmp.cmp(cmp_image, golden_image, False)):
      #print "is_equal = 1"
      is_equal = 1
  else:
    is_equal = 1

  #print 'returning %s' %is_equal
  return is_equal

def compare_mmu_obj(filename1, filename2):
  FILE1 = open(filename1,"r")
  FILE2 = open(filename2,"r")

  for line in FILE1:
    if "OBJECT chmmu0 TYPE cheetah-plus-mmu" in line:
      break
  for line in FILE2:
    if "OBJECT chmmu0 TYPE cheetah-plus-mmu" in line:
      break

  # both FILE1 and FILE2 are at the beginning of cpu0 object
  is_equal = 1
  for line1 in FILE1:
    if "OBJECT" in line1: # stop comparing if next object starts
      break
    for line2 in FILE2:
      break
    #print '%s%s'%(line1,line2)

    if line1 != line2:
      is_equal = 0
      break

  if(is_equal == 1):
    print "chmmu0 objects are equal"
  else:
    print "chmmu0 objects are not equal"

  FILE1.close()
  FILE2.close()
  return is_equal

def compare_cpu_obj(filename1, filename2):
  FILE1 = open(filename1,"r")
  FILE2 = open(filename2,"r")

  for line in FILE1:
    if "OBJECT cpu0 TYPE ultrasparc-iii-plus" in line:
      break
  for line in FILE2:
    if "OBJECT cpu0 TYPE ultrasparc-iii-plus" in line:
      break

  # both FILE1 and FILE2 are at the beginning of cpu0 object
  is_equal = 1
  skip = 0
  for line1 in FILE1:
    if "OBJECT" in line1: # stop comparing if next object starts
      break
    for line2 in FILE2:
      break
    #print '%s%s'%(line1,line2)
    # skip comparison from line:time_queue to line:step_rate
    if "time_queue" in line1:
      skip = 1
    if "step_rate" in line1:
      skip = 0

    if skip == 0 and line1 != line2:
      is_equal = 0
      break

  if(is_equal == 1):
    print "cpu0 objects are equal"
  else:
    print "cpu0 objects are not equal"

  FILE1.close()
  FILE2.close()
  return is_equal

magic_reached = 0
""" Exit on exception """
def core_exception(args, cpu, exp_num):
  if exp_num in exception_code:
    count = cpu.steps - args[1][cpu]
    print args[0]
    FILE = open(args[0],"w")
    FILE.write('Detected Exception\n')
    line = str(hex(exp_num)) + ":" + str(cpu) + ":" + str(count) + " PC: " + str(hex(cpu.control_registers['pc']))
    FILE.write(line)
    FILE.write('\n')
    FILE.close()
    print "Fault detected : core_exception"
    print line
    run('quit 666')


""" stop at magic call (app ended..) """
def core_magic(arg, cpu, param):
  global magic_reached
  magic_reached = 1
  pass
  #count = cpu.steps - arg[1][cpu]
  #line = str(cpu) + ":" + str(count)
  #FILE = open(arg[0],"w")
  #FILE.write(line)
  #FILE.write('\n')
  #FILE.close()

""" Exit on exception """
def core_problem_pc(args, cpu, access, brknum, reg, size):
  if(brknum==1):
    count = conf.cpu0.steps - args[1][conf.cpu0]
    FILE = open(args[0],"w")
    FILE.write('Detected Panic\n')
    line = "Panic:" + str(conf.cpu0) + ":" + str(count) + " PC: " + str(hex(conf.cpu0.control_registers['pc']))
    FILE.write(line)
    FILE.write('\n')
    FILE.close()
    print line
    print "Panic"
    print "Fault detected : Panic"
    run('quit 666')
  elif(brknum==2):
    count = conf.cpu0.steps - args[1][conf.cpu0]
    FILE = open(args[0],"w")
    FILE.write('Detected Idle Loop\n')
    line = "Idle:" + str(conf.cpu0) + ":" + str(count) + " PC: " + str(hex(conf.cpu0.control_registers['pc']))
    FILE.write(line)
    FILE.write('\n')
    FILE.close()
    print "Idle_Loop"
    print "Fault detected : Idle_Loop"
    run('quit 666')

def check_app_error(capture_log, filename, str):
  # check in the captured log whether there are some error messages
  infile = open('%s'%capture_log,"r")
  text = infile.read()
  infile.close()

  search = "No differences encounter"
  index = text.count(search)
  if(index==2):
    FILE = open(filename,"w")
    FILE.write('Masked\n')
    FILE.write(str)
    FILE.close()
    os.remove('%s'%(capture_log))
    print "Masked"
    print "Here"
    run('quit 666')

  search = "Segmentation Fault"
  index = text.find(search)
  if(index != -1):
    FILE = open(filename,"w")
    FILE.write('Detected Segmentation Fault \n')
    FILE.write(str)
    FILE.close()
    os.remove('%s'%(capture_log))
    print "exception: Detected Segmentation Fault "
 #   print "Fault detected : Seg Fault"
    run('quit 666')

  search = "Bus Error"
  index = text.find(search)
  if(index != -1):
    FILE = open(filename,"w")
    FILE.write('Detected Bus Error\n')
    FILE.write(str)
    FILE.close()
    os.remove('%s'%(capture_log))
    print "exception: Detected Bus Error"
 #  print "Fault detected : Bus Error"
    run('quit 666')

  search = "ERROR"
  index = text.find(search)
  if(index != -1):
    FILE = open(filename,"w")
    FILE.write('Detected ERROR\n')
    FILE.write(str)
    FILE.close()
    os.remove('%s'%(capture_log))
    print "exception: Detected ERROR"
 #  print "Fault detected : ERROR"
    run('quit 666')


def exit(stop, start, cpu):
  lines = [ '\nDetected', 'Exit: probably high os' ]
  map(lambda line: OUT.write(line+'\n'), lines)
  OUT.write('\nDetected High OS\n')
  line = str("0x202") + ":" + str(cpu) + " at " + str(stop-start)
  OUT.write(line)
  OUT.write('\n')
  OUT.close()
  run('quit 666')
  print 'probably high os'


def run_complete_new(app, phase, seqnum, pc, core=-1,  type=-1, bit=-1, stuck=-1, faultreg=64, srcdest=0, pilot=0, injcycle=0):
  core = 0
  global magic_reached

  print "Running benchmark to completion\n"
  run_name = '%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s' % (app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)
  start_checkpoint_name = '%s/%s.chkpt' %(CHKPT_FILE_DIR, run_name)
  run_sim_command('read-configuration %s' % (start_checkpoint_name))
  filename = '%s/%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s.completion_output' % (LOCAL_OUTPUT_DIR, app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)
  OUT = open(filename, 'w')
  cpu_p = [ SIM_proc_no_2_ptr(i) for i in range(1) ]
  begin = dict([(p, p.steps) for p in cpu_p])
  mark = dict([(p, p.steps) for p in cpu_p])

  if APP_DETECTORS == 1:
    # add_detector_locations(app)
    import app_detectors
    # init app detectors
    app_detectors.create_detectors(app)
    app_detectors.add_detector_locations(app)
    pf = '%s/%s.p' %(CHKPT_FILE_DIR, run_name)
    import pickle
    of = open(pf, "rb")
    app_detectors.detectors = pickle.load(of)
    app_detectors.accum_in = pickle.load(of)
    app_detectors.accum_out = pickle.load(of)

    cpu_p = [ SIM_proc_no_2_ptr(i) for i in range(1) ]
    begin = dict([(p, p.steps) for p in cpu_p])
    SIM_hap_add_callback("Core_Breakpoint", app_detectors.core_break_pc, [filename, begin, app])
    app_detectors.panic_break_num = SIM_breakpoint(conf.primary_context,
                                         Sim_Break_Virtual,
                                         Sim_Access_Execute,
                                         0x1049bac,
                                         216,
                                         Sim_Breakpoint_Simulation)
  else:
    SIM_breakpoint(conf.primary_context,
                                       Sim_Break_Virtual,
                                       Sim_Access_Execute,
                                       0x1049bac,
                                       216,
                                       Sim_Breakpoint_Simulation)
    SIM_hap_add_callback("Core_Breakpoint", core_problem_pc, [filename, begin])

  SIM_hap_add_callback("Core_Exception", core_exception, [filename, begin])
  SIM_hap_add_callback("Core_Magic_Instruction", core_magic, [filename, begin])

  #cycle=SIM_cycle_count(conf.cpu0)

  run_sim_command('magic-break-enable')
  run_sim_command('cpu-switch-time 1000')
  start_cycle = SIM_cycle_count(conf.cpu0) # for timeout detection
  capture_log = '%s/%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s.capture_log' % (LOCAL_OUTPUT_DIR, app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)
  run_sim_command('con0.capture-start %s' % (capture_log))

  checkpoint1 = -1
  checkpoint2 = -1
  checkpoint3 = -1

  run_only_once = 0

  while(magic_reached == 0):
    run_sim_command('c 10_000_000_000') # 10 billion
    # breakpoint reached
    curr_cycle = SIM_cycle_count(conf.cpu0)

#   check if it reached a comparison point
#   if yes, then compare checkpoints and take appropriate action
    if(curr_cycle == checkpoint1 or curr_cycle == checkpoint2 or curr_cycle == checkpoint3):
      if run_only_once == 0:
        run_only_once = 1
        checkpoint_name ='%s/%s_%s'%(COMPARE_CHKPT_DIR, run_name, curr_cycle)
        curr_mem_image ='%s.mem_image'%(checkpoint_name)
        golden_checkpoint_name ='%s/%s_%s'%(COMPARE_GOLDEN_CHKPT_DIR, app, curr_cycle)
        golden_mem_image ='%s.mem_image'%(golden_checkpoint_name)
        #print 'golden_image name : %s'%(golden_mem_image)

        run_sim_command('write-configuration %s' % (checkpoint_name))
        #SIM_write_configuration_to_file(checkpoint_name)
        # compare checkpoint with golden one
        if compare_cpu_obj(checkpoint_name, golden_checkpoint_name) == 1:
          print "Cpu0 matched"
          if compare_mmu_obj(checkpoint_name, golden_checkpoint_name) == 1:
            print "chmmu0 matched"
            if compare_disk_image(start_checkpoint_name, checkpoint_name, golden_checkpoint_name) == 1:
              print "disk images matched matched"
              run_sim_command('memory0_image.save %s'%(curr_mem_image))
              if(filecmp.cmp(curr_mem_image, golden_mem_image, False)):
                print "Mem images matched"
                # write to file
                FILE = open(filename,"w")
                FILE.write('Masked at %s\n'%curr_cycle)
                FILE.close()
                print "Masked"
                # before moving on to next checkpoints
	        try:
                  os.remove(curr_mem_image)
                  os.remove(checkpoint_name)
                  os.remove('%s.raw'%checkpoint_name)
                  os.remove('%s.memory0_image'%checkpoint_name)
                  os.remove('%s.sd25B_2_0_image'%checkpoint_name)
                  os.remove('%s.merged.sd25B_2_0_image'%checkpoint_name)
	        except Exception:
	          pass
                run('quit 666')
              os.remove(curr_mem_image)
        else:
          pass

        # before moving on to next checkpoints
	try:
          os.remove(checkpoint_name)
          os.remove('%s.raw'%checkpoint_name)
          os.remove('%s.memory0_image'%checkpoint_name)
          os.remove('%s.sd25B_2_0_image'%checkpoint_name)
          os.remove('%s.merged.sd25B_2_0_image'%checkpoint_name)
	except Exception:
	  pass

#   if not, then you reached the magic break or something is wrong and stop checking
    else:
      #print "This should be a magic break"
      print ""
      if(magic_reached == 1):
        #print "Yes, Magic reached! "
        print ""
      else:
        end_cycle = SIM_cycle_count(conf.cpu0) # for timeout detection
        if(end_cycle-start_cycle >= 20000000000):
          FILE = open(filename,"w")
          FILE.write('Detected Timeout\n')
          FILE.close()
          print "Timeout"
          run('quit 666')

  # magic break reached
  #if(magic_reached == 1):
  #  print "Sanity check passed"
  #else:
  #  print "Error in python script: sanity check failed"

  run_sim_command('con0.capture-stop')

  end_cycle = SIM_cycle_count(conf.cpu0) # for timeout detection
  if(end_cycle-start_cycle >= 20000000000):
    print "End=%s, Start=%s" %(end_cycle, start_cycle)
    FILE = open(filename,"w")
    FILE.write('Detected Timeout\n')
    FILE.close()
    print "1:Timeout"
    run('quit 666')

  if APP_DETECTORS == 1:
    app_detectors.stop_app_detectors()

  check_app_error(capture_log, filename, "")

  # unmount/mount to completely flush file system caches (sync just doesn't work)
  #run_inside('umount /export/home; magic_brk; \n')
  #run_inside('mount /export/home; magic_brk; \n')
  # save the end state
  fault = '%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s' % (app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)
  craff_filename = '%s/%s.diff.craff' % (CRAFF_DIR, fault)
  run_sim_command('con0.capture-start %s' % (capture_log))

# REMEMBER TO MAKE THIS SCALABLE GOLDEN
  #run('new-file-cdrom /home/sadve/shari2/outputs/fully_optimized_output.iso')
#  run('new-file-cdrom /shared/workspace/approx_comp/workloads/iso/fully_optimized_output.iso')
#  run('new-file-cdrom /shared/workspace/approx_comp/workloads/iso/apps_output.iso')
  run('new-file-cdrom /shared/workspace/approx_comp/workloads/iso/%s'%(ISO_OUTPUT))
#  run('new-file-cdrom /shared/workspace/approx_comp/workloads/iso/blackscholes_input_outcomes.iso')
  run_inside_safe(' umount /mnt/cdrom ;magic_brk\n', filename)
  run('cd25B_2_6.eject ')
#  run('cd25B_2_6.insert blackscholes_input_outcomes')
#  run('cd25B_2_6.insert fully_optimized_output')
#  run('cd25B_2_6.insert apps_output')
  run('cd25B_2_6.insert %s'%(ISO_OUTPUT))
  run_inside_safe(' mount -F hsfs /dev/dsk/c0t6d0s0 /mnt/cdrom ;magic_brk\n', filename)
  run_inside_safe(' diff -c /mnt/cdrom/%s.output output.txt > test_out.txt; grep "No differences encounter" test_out.txt; magic_brk\n' %(app), filename)
  check_app_error(capture_log, filename, "")

  run_inside_safe(' devfsadm ; mkdir /mnt/new_disk ; magic_brk\n', filename)
  run_inside_safe(' mount /dev/dsk/c0t1d0s2 /mnt/new_disk ; magic_brk\n', filename)
  run_inside_safe(' cp output.txt /mnt/new_disk/%s.output ; magic_brk\n' %(fault), filename)
  if(app == "libquantum_test" or app == "omnetpp_test" ):
    run_inside_safe(' cp output.err /mnt/new_disk/%s.output.err ; magic_brk\n' %(fault), filename)
  run_inside_safe(' umount /mnt/new_disk ; magic_brk\n', filename)
  # if(app == "swaptions_simsmall_merged"):
  #   pass
  # else:
  #   run('c')
  run_sim_command('con0.capture-stop')
  check_app_error(capture_log, filename, "Later")
  os.remove('%s'%(capture_log))
  print "saving diff file\n"
  run('sd1.save-diff-file %s' %(craff_filename))

def sol9_get_output(app, phase, seqnum, pc, core=-1,  type=-1, bit=-1, stuck=-1, faultreg=64, srcdest=0, pilot=0, injcycle=0):
#def sol9_get_output(app, phase, type, bit, stuck, execUnit, injinst, seqnum, prefix=DIR_PREFIX):
  core = 0
  print "Reading output files from inside the simulator\n"
  fault = '%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s' % (app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)

#  print "got here 1\n"

# REMEMBER TO MAKE THIS SCALABLE GOLDEN
#  run('read-configuration /home/pramach2/research/GEMS/simics/home/opensolaris_chkpt/sol9_100mb_disk_with_magic')
  run('read-configuration /home/venktgr2/checkpts/spec_opensolaris/sol9_with_magic_break')
  run('magic-break-enable')
  run('sd1.add-diff-file %s/%s.diff.craff' %(CRAFF_DIR,fault))
  run_inside('mount /host ; ls /host ; mkdir /mnt/new_disk ; mount /dev/dsk/c0t1d0s2 /mnt/new_disk ; magic_brk \n')
  #run_inside('cp /mnt/new_disk/%s.output /host/home/sadve/shari2/outputs/ ; magic_brk \n' %(fault))
  #REMEMBER TO CHANGE PATH TO MAKE SCALABLE
  run_inside('cp /mnt/new_disk/%s.output /host/scratch/amahmou2/outputs/ ; magic_brk \n' %(fault))
  if(app == "libquantum_test" or app == "omnetpp_test" ):
    run_inside('cp /mnt/new_disk/%s.output.err /host/scratch/amahmou2/outputs/ ; magic_brk \n' %(fault))
#  print "got here 2\n"

def print_checkpoint_cycle_nums(check_list, app):
  filename = WORKLOADS_PATH + "/apps/" + app + "/intermediate_checkpoints.txt"
  file_path = open(filename, 'w')

  if len(check_list) == 1:
      file_path.write("%s\n" % check_list[0])
  else:
      for x in check_list[1:]:
          file_path.write("%s\n" % x)

  file_path.close;

def create_intermediate_checkpoints(app):
  checkpoint_list =  []
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  curr_cycle = SIM_cycle_count(conf.cpu0)

  # print cycle count to file
  print curr_cycle
  checkpoint_list.append(curr_cycle)
  while True:
    run_sim_command('c 100_000_000')
    new_curr_cycle = SIM_cycle_count(conf.cpu0)
  # print cycle count to file
    if(new_curr_cycle - curr_cycle == 100000000):  # magic break not reached
      print new_curr_cycle
      checkpoint_list.append(new_curr_cycle)
      run_sim_command('write-configuration -z %s/intermediate/%s_%s' % (CHECKPOINT_DIR, app, new_curr_cycle))
    else:
      break
    curr_cycle = new_curr_cycle
  print_checkpoint_cycle_nums(checkpoint_list, app)

def create_compare_points(app):
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('cpu-switch-time 1')
  run_sim_command('magic-break-enable')
  curr_cycle = SIM_cycle_count(conf.cpu0)
  while True:
    run_sim_command('c 400_000_001')
    new_curr_cycle = SIM_cycle_count(conf.cpu0)
    if(new_curr_cycle - curr_cycle == 400000001):  # magic break not reached
      run_sim_command('write-configuration %s/%s_%s' % (CHECKPOINT_DIR, app, new_curr_cycle))
      run_sim_command('memory0_image.save %s/%s_%s.mem_image'%(CHECKPOINT_DIR, app, new_curr_cycle))
    else:
      break
    curr_cycle = new_curr_cycle



location_counter = 0
def pc_reached(args, mem, access, brknum, reg, size):
  global location_counter
  location_counter = location_counter + 1
  if(location_counter % 10000 == 0):
    pc=SIM_get_program_counter(conf.cpu0)
    cycle=SIM_cycle_count(conf.cpu0)
    print 'b %s %s'%(pc, cycle)

def find_test_site(app):
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('cpu-switch-time 1')
  run_sim_command('magic-break-enable')
  SIM_breakpoint(conf.primary_context,
                       Sim_Break_Virtual,
                       Sim_Access_Execute,
                       0x100006330,
                       4,
                       Sim_Breakpoint_Simulation)
  SIM_hap_add_callback("Core_Breakpoint", pc_reached, app)
  run_sim_command('cba 165031164407');
  run_sim_command('c');
  pc=SIM_get_program_counter(conf.cpu0)
  cycle=SIM_cycle_count(conf.cpu0)
  print 'b %s %s'%(pc, cycle)
  global location_counter
  print '%s'%(location_counter)

def get_combined_ccr(pc):
  return -1 # do not check anything at movCC or movRCC instructions

  split_instruction = []
  opcode = ""
  try:
    instruction_info = SIM_disassemble(conf.cpu0,pc,1)
    instruction = instruction_info[1]
    split_instruction = instruction.split()
    opcode = split_instruction[0]
  except:
    # print "exception at pc=%s" %(pc)
    # print "Error", sys.exc_info()[0]
    pass
  if(opcode.find("mov") >= 0):
    if(opcode == "move" or opcode == "movleu" or opcode == "movle" or opcode == "movl" or opcode == "movne" or opcode == "movg"):
      ccr = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,'ccr')) # read integer condition codes
      # CCR - 8 bits - [%xcc 4 bits,%icc 4 bits]
      fsr = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,'fsr')) # read FP state register
      fcc0 = (fsr>>10) & 0x3 # fcc0-fsr[11:10] -
      fcc1 = (fsr>>32) & 0x3 # fcc1-fsr[33:32] -
      fcc2 = (fsr>>34) & 0x3 # fcc2-fsr[35:34] -
      fcc3 = (fsr>>36) & 0x3 # fcc3-fsr[37-36] -
      fccn = fcc0 | (fcc1<<2) | (fcc2<<4) | (fcc3<<6)
      combined_ccr = (fccn<<8) | ccr
      return combined_ccr
    elif (opcode == "movr" or opcode == "movrlez" or opcode == "movrlz" or opcode == "movrnz" or opcode == "movrgz" or opcode == "movrgez"):
      op1 = split_instruction[1].lstrip('%').rstrip(',')
      op1_val = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,op1))
      z = op1_val == 0
      n = int(op1_val) > 0
      #print "%s: %d, int(op1_val)=%d z=%d n=%d" %(op1,op1_val,int(op1_val),z,n)
      return_val = z | (n<<1)
      return return_val
    else:
      return -1
  else:
    return -1

def test_disassemble(app):
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')

  for i in range(0,1000000):
    pc=SIM_get_program_counter(conf.cpu0)
    combined_ccr = get_combined_ccr(pc)
    if(combined_ccr != -1):
      #print " %d" %(combined_ccr)
      pass
    SIM_continue(1)

  run_sim_command('q')

instr_info_map = {}
def record_instr_info(pc):
  global instr_info_map
  if pc not in instr_info_map:
    reg_names = ["%i0", "%i1", "%i2", "%i3", "%i4", "%i5", "%i6", "%i7",
	       "%l0", "%l1", "%l2", "%l3", "%l4", "%l5", "%l6", "%l7",
	       "%o0", "%o1", "%o2", "%o3", "%o4", "%o5", "%o6", "%o7",
	       "%g1", "%g2", "%g3", "%g4", "%g5", "%g6", "%g7", "%sp", "%fp"]

    split_instruction = []
    opcode = ""
    try:
      instruction_info = SIM_disassemble(conf.cpu0,pc,1)
      instruction = instruction_info[1]
      split_instruction = instruction.split()
      opcode = split_instruction[0]
      if opcode == "save":
        opcode_i = 1
      elif opcode == "restore":
        opcode_i = 2
      else:
        opcode_i = 0
      if opcode.startswith("st"):
        instr_info_map[pc] = [opcode_i, 0, []]
	return

      dest = ""
      sources = []
      sources_i = []

      # get registers
      found_regs = []
      for op in split_instruction:
        for reg in reg_names:
          pos = op.find(reg)
          if pos != -1:
            #print op[pos:pos+3]
            found_regs.append(op[pos:pos+3])

        len_found_regs = len(found_regs)
        if len_found_regs > 0:
          no_dest = 0
          import re
          expressions = [re.compile(r"b"), re.compile(r"cmp"), re.compile(r"fcmp"), re.compile(r"prefet"), re.compile(r"st")]
          for e in expressions:
            if e.match(opcode):
              no_dest = 1
          if not no_dest:
            dest = found_regs.pop() # last element is dest, and it is being removed from found regs so that remaining regs can become sources
          for reg in found_regs:
            sources.append(reg)
            if get_flat_reg_num(reg,0) != 0:
              sources_i.append(int(get_flat_reg_num(reg,0)))

      instr_info_map[pc] = [opcode_i, int(get_flat_reg_num(dest, 0)), sources_i]

    except:
      # print "exception at pc=%s" %(pc)
      print "Error", sys.exc_info()[0]
      pass


is_control_flow = {}
def record_branch_info(pc):
  if pc not in is_control_flow:
    split_instruction = []
    opcode = ""
    try:
      instruction_info = SIM_disassemble(conf.cpu0,pc,1)
      instruction = instruction_info[1]
      split_instruction = instruction.split()
      opcode = split_instruction[0]
    except:
      # print "exception at pc=%s" %(pc)
      # print "Error", sys.exc_info()[0]
      pass
    #find returns the index where b is found
    if (opcode.find("b") == 0 or opcode.find("fb") == 0 or opcode.find("fb") == 0 or opcode.find("jmp") == 0 or opcode.find("return") == 0 or opcode.find("call") == 0 or opcode.find("retl") == 0):
      # its one of : integer branch instruction, fp branch instruction, jump instruction, return instruction, call instruction
      is_control_flow[pc] = 1
    elif opcode.find("st") == 0 or opcode.find("call") == 0 or opcode.find("nop") == 0: # if its a store instruction then mark it as 2
      is_control_flow[pc] = 2
    else:
      is_control_flow[pc] = 0

main_dict = {}
found  = 0
pc_patterns = {}
pc_pattern_cycles = {}
pc_pattern_sample_cycles = {}
pc_pattern_pop_cycles = {}
pc_control_depth = {}

def get_reg_name(reg_num, cwp):
  if 0 <= reg_num <= 7:
    return "%g" + str(reg_num)
  if 8 <= reg_num <= 15:
    return "%i" + str(reg_num)
  if 16 <= reg_num <= 23:
    return "%l" + str(reg_num)
  if 24 <= reg_num <= 31:
    return "%o" + str(reg_num)
  return ""


def get_flat_reg_num(reg_name, cwp):
  if reg_name == '%sp':
    reg_name = '%o6'
  elif reg_name == '%fp':
    reg_name = '%i6'

  if reg_name.startswith('%g'):
    return int(reg_name[2])
  elif reg_name.startswith('%i'):
    return cwp*16 + int(reg_name[2]) + 8
  elif reg_name.startswith('%l'):
    return cwp*16 + int(reg_name[2]) + 16
  elif reg_name.startswith('%o'):
    return cwp*16 + int(reg_name[2]) + 24
  return 0

def get_flat_reg_num_int(reg_num, cwp):
  if reg_num <= 7:
    return int(reg_num);
  else:  #  i, l, o
    return cwp*16 + int(reg_num)


def check_sum(pc):
  if pc != 0x100003554:
    return
  sum = 0
  for c in pc_pattern_pop_cycles[pc]:
    sum += c
  print "PC: 0x100003554, sum=%d, len=%d" %(sum, len(pc_pattern_pop_cycles[pc]))



def check_patterns_dynamic(app, control_info_q, instr_info_q, big_pc_q, cycle):
  all_pcs = True # True: local_pc_q will contain all pcs not just control pcs, False: only control pcs are recorded in local_pc_q and for pattern matching
  global pc_patterns
  global pc_pattern_cycles
  global pc_pattern_pop_cycles
  global pc_pattern_sample_cycles
  global pc_control_depth
  #global instr_info_map

  #check_sum(pc)

  pc = big_pc_q[0]
  if control_info_q[0] == 2: # if the top instruction is a store or call instruction then don't collect patterns for it
    return

  cwp = 0
  start_pattern = 0
  flat_reg_list = []

  [opcode, dest_num, sources] = instr_info_q[0]
  if dest_num != 0:
    flat_reg_list.append(get_flat_reg_num_int(dest_num, cwp))
  for reg_num in sources:
    flat_reg_list.append(get_flat_reg_num_int(reg_num, cwp))


  if len(flat_reg_list) == 0:
    start_pattern = 1

  # get the pc_q that is of interest
  # We are only interested in the first part of pc_q that ends at 5th control instruction
  start_list_index = 0

  index = 0
  for [temp_opcode, temp_dest, temp_sources] in instr_info_q:
    if index == 0:
      index += 1
      continue
    if start_pattern != 1: # if this pc has a dest or source reg
      #[temp_opcode, temp_dest, temp_sources] = instr_info_q[index]

      # print "%x: %x" %(pc, big_pc_q[index])
      # run_sim_command('quit 666')
      for reg in temp_sources:
        reg_num = get_flat_reg_num_int(reg, cwp)
        if reg_num in flat_reg_list:
          start_pattern = 1 # found a use, start forming patter from here
          start_list_index = index
          #print "%x info found at %x, at %d, %s" %(pc, big_pc_q[start_list_index], start_list_index, get_reg_name(reg_num, 0))
          break

      if temp_opcode == 1: #save
        cwp += 1
      elif temp_opcode == 2: # "restore":
        cwp -= 1
    else:
      break
    index += 1
    if index >= 750: # no use found so far, so stop looking
      break

  #print "%x info found at %x, at %d" %(pc, big_pc_q[start_list_index], start_list_index)

  local_pc_q = []
  control_instrn_count = 0
  for list_index in range(start_list_index, len(control_info_q)):
    flag = control_info_q[list_index]

    if all_pcs:
      local_pc_q.append(big_pc_q[list_index])

    if flag == 1:
      control_instrn_count += 1
      if not all_pcs:
        local_pc_q.append(big_pc_q[list_index])
      if control_instrn_count >= pc_control_depth[pc]:
        break

  # print "top = %x" %pc,
  # print "local_pc_q = [",
  # for pc_temp in local_pc_q:
  #   print "%x" %pc_temp,
  # print "]"

  #cycle = big_pc_cycle_q[0]
  #print len(local_pc_q)
  #t3 = time.clock()

  if pc in pc_patterns: # pc is a key in pc_patterns
    pattern_found = 0
    index = 0
    for item in pc_patterns[pc] :
      n = len(item)
      if n == len(local_pc_q):
        if item == local_pc_q:
          pattern_found = 1
          # print "pattern_found: [",
          # for pc_temp in local_pc_q:
 	  #        print "%x" %pc_temp,
          # print "]"
          pc_pattern_pop_cycles[pc][index] = pc_pattern_pop_cycles[pc][index] + 1
          if pc_pattern_cycles[pc][index] == 0 :
            pc_pattern_cycles[pc][index] = cycle
            pc_pattern_sample_cycles[pc][index].append(cycle)
          else:
            if len(pc_pattern_sample_cycles[pc][index]) < 750:
              pc_pattern_sample_cycles[pc][index].append(cycle)
              replace_index = int(random.random()*len(pc_pattern_sample_cycles[pc][index]))
              pc_pattern_cycles[pc][index] = pc_pattern_sample_cycles[pc][index][replace_index]
            else:
              if random.random() < 0.0005 :
                replace_index = int(random.random()*749)
                pc_pattern_sample_cycles[pc][index][replace_index] = cycle
                replace_index = int(random.random()*749)
                pc_pattern_cycles[pc][index] = pc_pattern_sample_cycles[pc][index][replace_index]
          break
      index = index + 1

    if pattern_found == 0:

      if len(pc_patterns[pc]) >= CONTROL_PATTEN_CAP:
        #temp_t1 = time.clock()

        #print "Cap reached for pc=%x, depth=%d, adjusting..." %(pc, pc_control_depth[pc])
	while len(pc_patterns[pc]) >= CONTROL_PATTEN_CAP:
	  # if pc_control_depth[pc] > 30:
          #   pc_control_depth[pc] -= 10
	  # elif pc_control_depth[pc] > 5:
	  #   pc_control_depth[pc] -= 1
	  # else:
	  #   break
	  if pc_control_depth[pc] > 5:
	    pc_control_depth[pc] -= 1
	  else:
	    break
	  removal_list = []
          for i in range(len(pc_patterns[pc])) : # for every pattern in the list
	    control_count = pc_control_depth[pc]
	    delete_from_here = 0
            for j in range(len(pc_patterns[pc][i])) : # for every pc in a pattern
	      if is_control_flow.get(pc_patterns[pc][i][j], 0):
	        control_count -= 1
	        delete_from_here = j
	        if control_count <= 0:
	          break

	    if delete_from_here != 0:
	      del pc_patterns[pc][i][delete_from_here:]

            for j in range(i): # for every pattern in the list until now
	      if len(pc_patterns[pc][j]) != len(pc_patterns[pc][i]):
	        continue
	      if pc_patterns[pc][j] == pc_patterns[pc][i]:
	        removal_list.append(i)
	        # merge cycles and samples from i into j
                pc_pattern_pop_cycles[pc][j] = pc_pattern_pop_cycles[pc][j] + pc_pattern_pop_cycles[pc][i]

                if len(pc_pattern_sample_cycles[pc][j]) + len(pc_pattern_sample_cycles[pc][i]) < 750:
                  pc_pattern_sample_cycles[pc][j].extend(pc_pattern_sample_cycles[pc][i])
                else:
                  replace_i_indices = set()
	          for l in range(int(750*pc_pattern_pop_cycles[pc][i]/pc_pattern_pop_cycles[pc][j])):
                    replace_i_index = int(random.random()*(len(pc_pattern_sample_cycles[pc][i])-1))
                    if replace_i_index not in replace_i_indices:
                      replace_j_index = int(random.random()*(len(pc_pattern_sample_cycles[pc][j])-1))
                      pc_pattern_sample_cycles[pc][j][replace_j_index] = pc_pattern_sample_cycles[pc][i][replace_i_index]
                      replace_i_indices.add(replace_i_index)


                replace_index = int(random.random()*(len(pc_pattern_sample_cycles[pc][j])-1))
                pc_pattern_cycles[pc][j] = pc_pattern_sample_cycles[pc][j][replace_index]
	        break

          if len(removal_list) != 0:
	    removal_list.sort(reverse=True)
	    #print removal_list
	    for i in removal_list:
	      pc_patterns[pc].pop(i)
	      pc_pattern_sample_cycles[pc].pop(i)
	      pc_pattern_pop_cycles[pc].pop(i)
	      pc_pattern_cycles[pc].pop(i)

        # temp_t2 = time.clock()
        # time_diff = temp_t2 - temp_t1
        # print "%f "%(time_diff)

	delete_from_here = 0
        control_count = pc_control_depth[pc]
        for j in range(len(local_pc_q)) :
	  if is_control_flow.get(local_pc_q[j], 0):
	    control_count -= 1
	    delete_from_here = j
	    if control_count <= 0:
	      break
        if delete_from_here != 0:
          del local_pc_q[delete_from_here:]

      pc_patterns[pc].append(list(local_pc_q))
      pc_pattern_sample_cycles[pc].append([cycle])
      pc_pattern_cycles[pc].append(cycle)
      pc_pattern_pop_cycles[pc].append(1)

  else:
    print "pc=%x not found!! " %(pc)
    pass



def check_patterns(app, control_info_q, big_pc_q, cycle):
  all_pcs = True # True: local_pc_q will contain all pcs not just control pcs, False: only control pcs are recorded in local_pc_q and for pattern matching
  #t1 = time.clock()
  global pc_patterns
  global pc_pattern_cycles
  global pc_pattern_pop_cycles
  global pc_pattern_sample_cycles

  local_pc_q = []
  # get the pc_q that is of interest
  # We are only interested in the first part of pc_q that ends at 5th control instruction
  control_instrn_count = 0
  list_index = 0
  for flag in control_info_q:
    list_index += 1
    if all_pcs:
      local_pc_q.append(big_pc_q[list_index-1])

    if flag == 1:
      control_instrn_count += 1
      if not all_pcs:
        local_pc_q.append(big_pc_q[list_index-1])
      if control_instrn_count >= CONTROL_DEPTH:
        break

  pc = big_pc_q[0]
  # print "top = %x" %pc,
  # print "local_pc_q = [",
  # for pc_temp in local_pc_q:
  #   print "%x" %pc_temp,
  # print "]"

  #cycle = big_pc_cycle_q[0]
  #print len(local_pc_q)
  #t3 = time.clock()

  if pc in pc_patterns: # pc is a key in pc_patterns
    pattern_found = 0
    index = 0
    for item in pc_patterns[pc] :
      n = len(item)
      if n == len(local_pc_q):
        if item == local_pc_q:
          pattern_found = 1
          # print "pattern_found: [",
          # for pc_temp in local_pc_q:
 	  #        print "%x" %pc_temp,
          # print "]"
          pc_pattern_pop_cycles[pc][index] = pc_pattern_pop_cycles[pc][index] + 1
          if pc_pattern_cycles[pc][index] == 0 :
            pc_pattern_cycles[pc][index] = cycle
            pc_pattern_sample_cycles[pc][index].append(cycle)
          else:
            if len(pc_pattern_sample_cycles[pc][index]) < 750:
              pc_pattern_sample_cycles[pc][index].append(cycle)
	      replace_index = int(random.random()*len(pc_pattern_sample_cycles[pc][index]))
              pc_pattern_cycles[pc][index] = pc_pattern_sample_cycles[pc][index][replace_index]
            else:
	      if random.random() < 0.0005 :
                replace_index = int(random.random()*749)
                pc_pattern_sample_cycles[pc][index][replace_index] = cycle
	        replace_index = int(random.random()*749)
                pc_pattern_cycles[pc][index] = pc_pattern_sample_cycles[pc][index][replace_index]
          break
      index = index + 1
    if pattern_found == 0:
      pc_patterns[pc].append(list(local_pc_q))
      pc_pattern_sample_cycles[pc].append([cycle])
      pc_pattern_cycles[pc].append(cycle)
      pc_pattern_pop_cycles[pc].append(1)
  else:
    print "pc=%x not found!! " %(pc)
    pass

  # t2 = time.clock()
  # diff=t2-t1
  # if diff > 1 :
  #   print t2-t1
  #   print t3-t1
  #   print "pc=%x len(pc_pattern[pc])=%d" %(pc, len(pc_patterns[pc]))

def print_pc_pattern_cycles(app, lc, fn):
  MAIN_LOG_DIR = get_main_log_dir(app)
  filename = MAIN_LOG_DIR + app
  sample_filename = MAIN_LOG_DIR + app
  if lc == 1:
    filename = filename + '_live_pc_patterns_' + str(CONTROL_DEPTH) + '_' + str(CONTROL_PATTERN_LEN) + '.txt'
    sample_filename = sample_filename + '_sample_live_pc_patterns_' + str(CONTROL_DEPTH) + '_' + str(CONTROL_PATTERN_LEN) + '.txt'
  else:
    if fn >= 0:
      filename = filename + '_%d_control_pc_patterns_' %(fn) + str(CONTROL_DEPTH) + '_' + str(CONTROL_PATTERN_LEN) + '.txt'
      sample_filename = sample_filename + '_%d_sample_control_pc_patterns_' %(fn) + str(CONTROL_DEPTH) + '_' + str(CONTROL_PATTERN_LEN) + '.txt'
    else:
      filename = filename + '_control_pc_patterns_' + str(CONTROL_DEPTH) + '_' + str(CONTROL_PATTERN_LEN) + '.txt'
      sample_filename = sample_filename + '_sample_control_pc_patterns_' + str(CONTROL_DEPTH) + '_' + str(CONTROL_PATTERN_LEN) + '.txt'
  pf = open(filename,'w')
  psf = open(sample_filename,'w')

  key_list = pc_pattern_cycles.keys()
  key_list.sort()
  num_patterns = 0
  for k in key_list:
    pc_string = str(hex(k)) + ":"
    pf.write(pc_string)
    for i in range(0,len(pc_pattern_cycles[k])):
    #for cycle in pc_pattern_cycles[k]:
      cycle = pc_pattern_cycles[k][i]
      if cycle != 0:
        num_patterns = num_patterns + 1
        cycle_line = " " + str(cycle)
        pf.write(cycle_line)

        # write samples to the log
        psf.write(pc_string)
        cycle_str = str(cycle) + ":"
        psf.write(cycle_str)
	pop_str = str(pc_pattern_pop_cycles[k][i]) + ":"
        psf.write(pop_str)
        samples = pc_pattern_sample_cycles[k][i]
        for c in samples:
	  sample_str = " " + str(c)
          psf.write(sample_str)
        psf.write('\n')

    pf.write('\n')
  pf.close()
  psf.close()
  print "num patterns = %s" %num_patterns

def new_app_pattern_profile(app, lc, fn, depth):
# fn - file number - fn=-1, nothing; fn>=0, use fn
# lc - live or control; 1 - live, 0 - control
# control depth
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  global global_magic
  global pc_patterns
  global pc_control_depth
  global pc_pattern_cycles
  global pc_pattern_sample_cycles
  global pc_pattern_pop_cycles
  global CONTROL_DEPTH
  CONTROL_DEPTH = depth
  #CONTROL_DEPTH = 50

  text_local_start = app_txt_info.text_start[app]
  text_local_end = app_txt_info.text_end[app]
  print "start=%x, end=%x" %(text_local_start, text_local_end)

  if app == "gcc_test" or app == "mcf_test" or app == "libquantum_test" :
  #if app == "gcc_opt_abdul" or app == "mcf_opt_abdul" or app == "libquantum_opt_abdul" :
    gap = (app_txt_info.text_end[app] - app_txt_info.text_start[app])/8
    print hex(gap)

    if fn >= 0 :
      local_points = []
      for local_point in range(app_txt_info.text_start[app], app_txt_info.text_end[app], gap):
        local_points.append(local_point)
      print len(local_points)

      text_local_start = local_points[fn]
      text_local_start = ( ( text_local_start / 4 ) * 4 ) + 4
      text_local_end = local_points[fn] + gap

    print "start=%x " %(text_local_start)
    print "end=%x " %(text_local_end)

  # init pc_pattern_cycles
  for pc in range(text_local_start, text_local_end, 4):
    pc_patterns[pc] = [ [] ]
    pc_pattern_sample_cycles[pc] = [[]]

    pc_pattern_cycles[pc] = []
    pc_pattern_cycles[pc].append(0)
    pc_pattern_pop_cycles[pc] = []
    pc_pattern_pop_cycles[pc].append(0)
    pc_control_depth[pc] = CONTROL_DEPTH

  #run_sim_command('read-configuration %s/intermediate/%s_384641434832' % (CHECKPOINT_DIR,app))
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')

  temp_t1 = time.clock()
  run_sim_command('b %d' %app_txt_info.actual_app_start[app])
  run_sim_command('c')
  temp_t2 = time.clock()
  time_diff = temp_t2 - temp_t1
  print 'time to skip:%d'%(time_diff)
  print "Starting now"

  profile_now = 1
  #***********************
  # run_sim_command('cba 384681096000')
  # print "skipping: current=%s, target=384681095000" %(SIM_cycle_count(conf.cpu0))
  # run_sim_command('c')
  # print "starting categorization"

  run_sim_command('magic-break-enable')

  SIM_hap_add_callback("Core_Magic_Instruction", hap_magic_call_reached, app)

  control_info_q = []
  instr_info_q = []
  pc_q = []
  pc_cycle_q = []

  priv_pc = 0

  beat_counter = 0
  while True:
    pc=SIM_get_program_counter(conf.cpu0)
    if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
      if priv_pc != pc:
        record_branch_info(pc)
        record_instr_info(pc)
        pc_q.append(pc)
        control_info_q.append( is_control_flow.get(pc, 0) )  # 1 - control flow, 0 - not control flow, 2 - store
        instr_info_q.append( instr_info_map.get(pc, [0, 0, []]) )  # 1 - control flow, 0 - not control flow,
        pc_cycle_q.append(SIM_cycle_count(conf.cpu0))
        if len(pc_q) == CONTROL_PATTERN_LEN:
          break;
        # combined_ccr = get_combined_ccr(pc)
        # if(combined_ccr != -1):
        #   pc_q.append(combined_ccr)
        #   pc_cycle_q.append(0)
        # if len(pc_q) == CONTROL_PATTERN_LEN:
        #   break
      priv_pc = pc

    if global_magic == 0:
      SIM_continue(1)
    else:
      break

  check_patterns_dynamic(app, control_info_q, instr_info_q, pc_q, pc_cycle_q[0])
  print "Done with buffer init"

  while global_magic == 0 and profile_now == 1:
  #for i in range(0,2000000): # 1 million
  #for i in range(0,10000): # 1000 instruction
    pc=SIM_get_program_counter(conf.cpu0)
    if pc == app_txt_info.actual_app_end[app]:
      profile_now = 0

    if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
      record_instr_info(pc)
      if pc_q[CONTROL_PATTERN_LEN-1] != pc:
        # if pc == 0x100003d74:
        #   print "found pc: %x" %pc
        record_branch_info(pc)
        pc_q.append(pc)
        control_info_q.append(is_control_flow.get(pc,0))
        instr_info_q.append(instr_info_map.get(pc,[0, 0, []]))
        pc_cycle_q.append(SIM_cycle_count(conf.cpu0))
        pc_q.pop(0)
        control_info_q.pop(0)
        instr_info_q.pop(0)
        pc_cycle_q.pop(0)
        check_patterns_dynamic(app, control_info_q, instr_info_q, pc_q, pc_cycle_q[0])
        # combined_ccr = get_combined_ccr(pc)
        # if(combined_ccr != -1):
        #   pc_q.append(combined_ccr)
        #   pc_cycle_q.append(0)
        #   pc_q.pop(0)
        #   pc_cycle_q.pop(0)
        #   check_patterns(app, control_info_q, pc_q, pc_cycle_q[0])

    SIM_continue(1)
    check_heart_beat(SIM_cycle_count(conf.cpu0))

  # for last CONTROL_PATTERN_LEN pcs
  end_range = CONTROL_PATTERN_LEN - 1
  for i in range(0,end_range):
    pc_q.append(0)
    control_info_q.append(0)
    instr_info_q.append([0,0,[]])
    pc_cycle_q.append(0)
    pc_q.pop(0)
    control_info_q.pop(0)
    instr_info_q.pop(0)
    pc_cycle_q.pop(0)
    check_patterns_dynamic(app, control_info_q, instr_info_q,  pc_q, pc_cycle_q[0])

  print_pc_pattern_cycles(app, lc, fn)
  run("quit 666")


def new_app_pattern_profile_chunk(app, lc, fn, depth, chunk_id):
# chunk is 0, 1, 2, 3, ...
# fn - file number - fn=-1, nothing; fn>=0, use fn
# lc - live or control; 1 - live, 0 - control
# control depth
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  temp_t1 = time.clock()
  global global_magic
  global pc_patterns
  global pc_pattern_cycles
  global pc_pattern_sample_cycles
  global pc_pattern_pop_cycles
  global pc_control_depth
  global CONTROL_DEPTH
  CONTROL_DEPTH = depth
  chunk_size = 10000000

  text_local_start = app_txt_info.text_start[app]
  text_local_end = app_txt_info.text_end[app]
  print "start=%x, end=%x" %(text_local_start, text_local_end)

  if app == "gcc_test" or app == "mcf_test" or app == "libquantum_test" :
    gap = (app_txt_info.text_end[app] - app_txt_info.text_start[app])/8
    print hex(gap)

    if fn >= 0 :
      local_points = []
      for local_point in range(app_txt_info.text_start[app], app_txt_info.text_end[app], gap):
        local_points.append(local_point)
      print len(local_points)

      text_local_start = local_points[fn]
      text_local_start = ( ( text_local_start / 4 ) * 4 ) + 4
      text_local_end = local_points[fn] + gap

    print "start=%x " %(text_local_start)
    print "end=%x " %(text_local_end)

  # init pc_pattern_cycles
  for pc in range(text_local_start, text_local_end, 4):
    pc_patterns[pc] = [ [] ]
    pc_pattern_sample_cycles[pc] = [[]]

    pc_pattern_cycles[pc] = []
    pc_pattern_cycles[pc].append(0)
    pc_pattern_pop_cycles[pc] = []
    pc_pattern_pop_cycles[pc].append(0)
    pc_control_depth[pc] = CONTROL_DEPTH

  #run_sim_command('read-configuration %s/intermediate/%s_384641434832' % (CHECKPOINT_DIR,app))
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')

  run_sim_command('b %d' %app_txt_info.actual_app_start[app])
  run_sim_command('c')

  start_cycle = SIM_cycle_count(conf.cpu0)
  run_sim_command('b %d' %app_txt_info.actual_app_end[app])
  if chunk_id != 1:
    run_sim_command('c %s' %((chunk_id-1)*chunk_size))
    curr_cycle = SIM_cycle_count(conf.cpu0)
    if curr_cycle-start_cycle < (chunk_id-1)*chunk_size:
      run('quit 666') # magic break or end pc has reached


  print "Starting now"

  profile_now = 1
  #***********************
  # run_sim_command('cba 384681096000')
  # print "skipping: current=%s, target=384681095000" %(SIM_cycle_count(conf.cpu0))
  # run_sim_command('c')
  # print "starting categorization"


  SIM_hap_add_callback("Core_Magic_Instruction", hap_magic_call_reached, app)

  start_cycle = SIM_cycle_count(conf.cpu0)

  control_info_q = []
  instr_info_q = []
  pc_q = []
  pc_cycle_q = []

  priv_pc = 0

  beat_counter = 0
  while True:
    pc=SIM_get_program_counter(conf.cpu0)
    if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
      record_instr_info(pc)
      if priv_pc != pc:
        record_branch_info(pc)
        pc_q.append(pc)
        control_info_q.append( is_control_flow.get(pc, 0) )  # 1 - control flow, 0 - not control flow,
        instr_info_q.append( instr_info_map.get(pc, [0, 0, []]) )  # 1 - control flow, 0 - not control flow,
        pc_cycle_q.append(SIM_cycle_count(conf.cpu0))
        if len(pc_q) == CONTROL_PATTERN_LEN:
          break;
        # combined_ccr = get_combined_ccr(pc)
        # if(combined_ccr != -1):
        #   pc_q.append(combined_ccr)
        #   pc_cycle_q.append(0)
        # if len(pc_q) == CONTROL_PATTERN_LEN:
        #   break
      priv_pc = pc

    if global_magic == 0:
      SIM_continue(1)
    else:
      break

  check_patterns_dynamic(app, control_info_q, instr_info_q, pc_q, pc_cycle_q[0])
  print "Done with buffer init"

  end_counter = CONTROL_PATTERN_LEN
  start_counter = 0
  while global_magic == 0 and profile_now == 1:
  #for i in range(0,100000): # 1 million
  #for i in range(0,20000): # 1000 instruction
    pc=SIM_get_program_counter(conf.cpu0)
    if pc == app_txt_info.actual_app_end[app]:
      start_counter = 1
    if start_counter:
      end_counter -= 1
      if end_counter == 0:
        profile_now = 0

    if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
      record_instr_info(pc)
      if pc_q[CONTROL_PATTERN_LEN-1] != pc:
        # if pc == 0x100003d74:
        #   print "found pc: %x" %pc
        record_branch_info(pc)
        pc_q.append(pc)
        control_info_q.append(is_control_flow.get(pc,0))
        instr_info_q.append( instr_info_map.get(pc, [0, 0, []]) )  # 1 - control flow, 0 - not control flow,
        pc_cycle_q.append(SIM_cycle_count(conf.cpu0))
        pc_q.pop(0)
        control_info_q.pop(0)
        instr_info_q.pop(0)
        pc_cycle_q.pop(0)

        check_patterns_dynamic(app, control_info_q, instr_info_q, pc_q, pc_cycle_q[0])

        # combined_ccr = get_combined_ccr(pc)
        # if(combined_ccr != -1):
        #   pc_q.append(combined_ccr)
        #   pc_cycle_q.append(0)
        #   pc_q.pop(0)
        #   pc_cycle_q.pop(0)
        #   check_patterns(app, control_info_q, pc_q, pc_cycle_q[0])

    SIM_continue(1)
    curr_cycle = SIM_cycle_count(conf.cpu0)
    check_heart_beat(curr_cycle)
    if curr_cycle - start_cycle >= chunk_size+CONTROL_PATTERN_LEN-1:
      profile_now = 0

  # # for last CONTROL_PATTERN_LEN pcs
  # end_range = CONTROL_PATTERN_LEN - 1
  # for i in range(0,end_range):
  #   pc_q.append(0)
  #   control_info_q.append(0)
  #   pc_cycle_q.append(0)
  #   pc_q.pop(0)
  #   control_info_q.pop(0)
  #   pc_cycle_q.pop(0)
  #   check_patterns(app, control_info_q, pc_q, pc_cycle_q[0])

  #print_pc_pattern_cycles(app, lc, fn)
  print "writing prickle files"
  import pickle
  pickle.dump(pc_patterns, open("pickle_dir/pc_patterns_%s_%s_%s" %(app, chunk_id, CONTROL_DEPTH), "w"))
  pickle.dump(pc_pattern_cycles, open("pickle_dir/pc_pattern_cycles_%s_%s_%s" %(app, chunk_id, CONTROL_DEPTH), "w"))
  pickle.dump(pc_pattern_sample_cycles, open("pickle_dir/pc_pattern_sample_cycles_%s_%s_%s" %(app, chunk_id, CONTROL_DEPTH), "w"))
  pickle.dump(pc_pattern_pop_cycles, open("pickle_dir/pc_pattern_pop_cycles_%s_%s_%s" %(app, chunk_id, CONTROL_DEPTH), "w"))
  pickle.dump(pc_control_depth, open("pickle_dir/pc_control_depth_%s_%s_%s" %(app, chunk_id, CONTROL_DEPTH), "w"))
  pickle.dump(is_control_flow, open("pickle_dir/pc_is_control_flow_%s_%s_%s" %(app, chunk_id, CONTROL_DEPTH), "w"))

  temp_t2 = time.clock()
  time_diff = temp_t2 - temp_t1
  print 'time: %f'%(time_diff)

  run("quit 666")


def app_pattern_profile(app, lc, fn):
# fn - file number - fn=-1, nothing; fn>=0, use fn
# lc - live or control; 1 - live, 0 - control
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  global global_magic
  global main_dict

  if lc == 1:
    if(app == "blackscholes_simmedium"):
      import blackscholes_patterns
      print "just imported"
      main_dict = blackscholes_patterns.blackscholes_patterns
    elif(app == "fft_small"):
      print "NEED MORE ADJUSTMENTS!!!!"
      import fft_patterns
      print "just imported"
      main_dict = fft_patterns.fft_patterns
    elif(app == "swaptions_simsmall"):
      import swaptions_patterns
      print "just imported"
      main_dict = swaptions_patterns.swaptions_patterns
    elif(app == "lu_reduced"):
      import lu_patterns
      print "just imported"
      main_dict = lu_patterns.lu_patterns
    elif(app == "gcc_test"):
      import gcc_patterns
      print "just imported"
      main_dict = gcc_patterns.gcc_patterns
    elif(app == "libquantum_test"):
      import libquantum_patterns
      print "just imported"
      main_dict = libquantum_patterns.libquantum_patterns
    elif(app == "omnetpp_test"):
      import omnetpp_patterns
      print "just imported"
      main_dict = omnetpp_patterns.omnetpp_patterns
  else:
    if(app == "blackscholes_simmedium"):
      import blackscholes_control_patterns
      print "just imported"
      main_dict = blackscholes_control_patterns.blackscholes_control_patterns
    elif(app == "fft_small"):
      import fft_control_patterns
      print "just imported"
      main_dict = fft_control_patterns.fft_control_patterns
    elif(app == "swaptions_simsmall"):
      import swaptions_control_patterns
      print "just imported"
      main_dict = swaptions_control_patterns.swaptions_control_patterns
    elif(app == "lu_reduced"):
      import lu_control_patterns
      print "just imported"
      main_dict = lu_control_patterns.lu_control_patterns
    elif(app == "gcc_test"):
      import gcc_control_patterns
      print "just imported"
      main_dict = gcc_control_patterns.gcc_control_patterns
    elif(app == "libquantum_test"):
      import libquantum_control_patterns
      print "just imported"
      main_dict = libquantum_control_patterns.libquantum_control_patterns
    elif(app == "omnetpp_test"):
      if fn == -1:
        import omnetpp_control_patterns
        print "just imported"
        main_dict = omnetpp_control_patterns.omnetpp_control_patterns
      elif fn == 0:
        import omnetpp_control_patterns_0
        print "just imported"
        main_dict = omnetpp_control_patterns_0.omnetpp_control_patterns_0
      elif fn == 1:
        import omnetpp_control_patterns_1
        print "just imported"
        main_dict = omnetpp_control_patterns_1.omnetpp_control_patterns_1
      elif fn == 2:
        import omnetpp_control_patterns_2
        print "just imported"
        main_dict = omnetpp_control_patterns_2.omnetpp_control_patterns_2
      elif fn == 3:
        import omnetpp_control_patterns_3
        print "just imported"
        main_dict = omnetpp_control_patterns_3.omnetpp_control_patterns_3
      elif fn == 4:
        import omnetpp_control_patterns_4
        print "just imported"
        main_dict = omnetpp_control_patterns_4.omnetpp_control_patterns_4
      elif fn == 5:
        import omnetpp_control_patterns_5
        print "just imported"
        main_dict = omnetpp_control_patterns_5.omnetpp_control_patterns_5


  print "copied main_dict"

  # init pc_pattern_cycles
  for pc in range(app_txt_info.text_start[app], app_txt_info.text_end[app], 4):
    if pc in main_dict: # pc is a key in main_dict
      pc_pattern_cycles[pc] = []
      pc_pattern_pop_cycles[pc] = []
      pc_pattern_sample_cycles[pc] = [[]]
      for l in main_dict[pc]:
        pc_pattern_cycles[pc].append(0)
        pc_pattern_pop_cycles[pc].append(0)
        pc_pattern_sample_cycles[pc].append([])

  print "init done"

  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('c 8_000_000')
  run_sim_command('cpu-switch-time 1')
  run_sim_command('magic-break-enable')

  SIM_hap_add_callback("Core_Magic_Instruction", hap_magic_call_reached, app)

 #pc_q = deque([])
  pc_q = []
  pc_cycle_q = []

  while True:
    pc=SIM_get_program_counter(conf.cpu0)
    if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
      pc_q.append(pc)
      pc_cycle_q.append(SIM_cycle_count(conf.cpu0))
      if len(pc_q) == 1000:
        break;
    if global_magic == 0:
      SIM_continue(1)
    else:
      break

  while global_magic == 0:
  #for i in range(0,1000000): # 1 million
    pc=SIM_get_program_counter(conf.cpu0)
    if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
      pc_q.append(pc)
      pc_cycle_q.append(SIM_cycle_count(conf.cpu0))
      pc_q.pop(0)
      pc_cycle_q.pop(0)
      check_patterns(pc_q, pc_cycle_q[0])
    SIM_continue(1)
    check_heart_beat(SIM_cycle_count(conf.cpu0))

  print_pc_pattern_cycles(app, lc, fn)
  run("quit 666")

def test_dummy(app):
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('print-time')
  run_sim_command('cpu0.pregs')
  run_sim_command('c 1_000_000')
  run_sim_command('print-time')

def app_int_only_profile(app):
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  MAIN_LOG_DIR = get_main_log_dir(app)
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('print-time')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  global global_magic
  SIM_hap_add_callback("Core_Magic_Instruction", hap_magic_call_reached, app)
  run_sim_command('b %d' %app_txt_info.actual_app_start[app])
  run_sim_command('c')
  #run_sim_command('cpu-switch-time 1')

  print SIM_get_program_counter(conf.cpu0)
  import re
  ignore_instr_expressions = [re.compile(r"prefet"), re.compile(r"save"), re.compile(r"restore"), re.compile(r"call")]
  reg_names = ["%i0", "%i1", "%i2", "%i3", "%i4", "%i5", "%i6", "%i7",
	       "%l0", "%l1", "%l2", "%l3", "%l4", "%l5", "%l6", "%l7",
	       "%o0", "%o1", "%o2", "%o3", "%o4", "%o5", "%o6", "%o7",
	       "%g1", "%g2", "%g3", "%g4", "%g5", "%g6", "%g7", "%sp", "%fp"]


  pc_counters = {} # dictionary pc:counter
  profile_now = 1

  while True:
  #for i in range(0,100000000): # 10 million
    pc=SIM_get_program_counter(conf.cpu0)
    if pc == app_txt_info.actual_app_start[app]:
      profile_now = 1
    if pc == app_txt_info.actual_app_end[app]:
      profile_now = 0
    if profile_now == 1:
      if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
        if pc in pc_counters: # pc is the key in pc_counters dictionary
          pc_counters[pc] += 1
        else:
          #print 'new %x' %(pc)
          ignore_instr = 1
          try:
            instruction_info = SIM_disassemble(conf.cpu0,pc,1)
            instruction = instruction_info[1]
            split_instruction = instruction.split()
            opcode = split_instruction[0]
            # check if this instruction uses an integer register
            for op in split_instruction:
              for reg in reg_names:
                pos = op.find(reg)
                if pos != -1:
                  ignore_instr = 0
                  break
              if ignore_instr == 0: # found one integer register, no need to check other operands in this instruction
                break

            # check if this instruction should be ignored all together
            for e in ignore_instr_expressions:
              if e.match(opcode):
                ignore_instr = 1
          except:
            pass

          if ignore_instr == 0:
            pc_counters[pc] = 1

    if global_magic == 0:
      SIM_continue(1)
    else:
      break

  # print pc_list
  total_instructions = 0
  print app
  filename = MAIN_LOG_DIR + app + '_int_only_profile.txt'
  pf = open(filename,'w')
  key_list = pc_counters.keys()
  key_list.sort()
  for k in key_list:
    line = str(hex(k)) + ":" + str(pc_counters[k])
    total_instructions += pc_counters[k]
    pf.write(line)
    pf.write('\n')
  pf.close()
  print 'num emelents in the dict: %s' %len(pc_counters)
  print 'total_instructions = %d' %total_instructions
  run("quit 666")


def app_profile(app):
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  MAIN_LOG_DIR = get_main_log_dir(app)
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('print-time')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  global global_magic
  SIM_hap_add_callback("Core_Magic_Instruction", hap_magic_call_reached, app)
  run_sim_command('b %d' %app_txt_info.actual_app_start[app])
  run_sim_command('c')
  #run_sim_command('cpu-switch-time 1')

  print SIM_get_program_counter(conf.cpu0)

  pc_counters = {} # dictionary pc:counter
  profile_now = 1

  while True:
  #for i in range(0,100000000): # 10 million
    pc=SIM_get_program_counter(conf.cpu0)
    if pc == app_txt_info.actual_app_start[app]:
      profile_now = 1
    if pc == app_txt_info.actual_app_end[app]:
      profile_now = 0
    if profile_now == 1:
      if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
        if pc in pc_counters: # pc is the key in pc_counters dictionary
          pc_counters[pc] += 1
        else:
          #print 'new %x' %(pc)
          pc_counters[pc] = 1

    if global_magic == 0:
      SIM_continue(1)
    else:
      break

  # print pc_list
  print app
  filename = MAIN_LOG_DIR + app + '_profile.txt'
  pf = open(filename,'w')
  key_list = pc_counters.keys()
  key_list.sort()
  for k in key_list:
    line = str(hex(k)) + ":" + str(pc_counters[k])
    pf.write(line)
    pf.write('\n')
  pf.close()
  print 'num emelents in the dict: %s' %len(pc_counters)
  run("quit 666")


def new_app_profile(app):
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  MAIN_LOG_DIR = get_main_log_dir(app)
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('cpu-switch-time 1')
  run_sim_command('magic-break-enable')
  global global_magic
  SIM_hap_add_callback("Core_Magic_Instruction", hap_magic_call_reached, app)

  pc_counters = {} # dictionary pc:counter
  #while True:
  for i in range(0,100000000): # 10 million
    pc=SIM_get_program_counter(conf.cpu0)
    if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
      if pc in pc_counters: # pc is the key in pc_counters dictionary
        pc_counters[pc] += 1
      else:
	#print '%x' %(pc)
        pc_counters[pc] = 1

    if global_magic == 0:
      SIM_continue(1)
    else:
      break

  # print pc_list
  print app
  filename = MAIN_LOG_DIR + app + '_new_profile.txt'
  pf = open(filename,'w')
  key_list = pc_counters.keys()
  key_list.sort()
  for k in key_list:
    line = str(hex(k)) + ":" + str(pc_counters[k])
    pf.write(line)
    pf.write('\n')
  pf.close()
  print 'num emelents in the dict: %s' %len(pc_counters)
  run("quit 666")


pst_pc_cycle = {}
def pst_record(args, mem, access, brknum, reg, size):
  pc=SIM_get_program_counter(conf.cpu0)
  if pc in pst_pc_cycle:
    if len(pst_pc_cycle[pc]) < 10:
      cycle=SIM_cycle_count(conf.cpu0)
      pst_pc_cycle[pc].append(cycle)
  else:
    pst_pc_cycle[pc] = []
    cycle=SIM_cycle_count(conf.cpu0)
    pst_pc_cycle[pc].append(cycle)

def pst_points(app):
  MAIN_LOG_DIR = get_main_log_dir(app)
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('cpu-switch-time 1')
  run_sim_command('magic-break-enable')
  global global_magic
  SIM_hap_add_callback("Core_Magic_Instruction", hap_magic_call_reached, app)

  filename = MAIN_LOG_DIR + app + '_pst_pcs.txt'
  f = open(filename,'r')
  for line in f:
    SIM_breakpoint(conf.primary_context,
                         Sim_Break_Virtual,
                         Sim_Access_Execute,
                         int(line,16),
                         4,
                         Sim_Breakpoint_Simulation)
  SIM_hap_add_callback("Core_Breakpoint", pst_record, app)
  f.close()

  run_sim_command('c')

  global pst_pc_cycle
  filename = MAIN_LOG_DIR + app + '_pst_pcs_cycles.txt'
  pf = open(filename,'w')
  key_list = pst_pc_cycle.keys()
  key_list.sort()
  for k in key_list:
    #pf.write('0x%x:'%(k))
    line = str(hex(k)) + ":"
    pf.write(line)
    for cycle in pst_pc_cycle[k]:
      line = " " + str(cycle)
      pf.write(line)
    pf.write('\n')
  pf.close()
  run("quit 666")

detector_counter = 0
def detection_point_reached(args, mem, access, brknum, reg, size):
  global detector_counter
  detector_counter += 1

def count_detectors(app):
  global detector_counter
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  #local_pc_list = [0x1000018b0, 0x1000015ec, 0x100001704] # blackscholes
  #local_pc_list = [0x1000043f4, 0x100004408, 0x10000445c] # LU
  local_pc_list = [0x1000035cc, 0x1000043c4, 0x100004860, 0x100004750, 0x100004954, 0x100004860] # FFT
  for pc in local_pc_list:
    SIM_breakpoint(conf.primary_context,
                         Sim_Break_Virtual,
                         Sim_Access_Execute,
                         pc,
                         4,
                         Sim_Breakpoint_Simulation)
  SIM_hap_add_callback("Core_Breakpoint", detection_point_reached, app)
  SIM_hap_add_callback("Core_Magic_Instruction", hap_mode_magic, app)
  run_sim_command('c')
  print "Number of detectors to be places: ",
  print detector_counter


injected_trans_fault = 0
panic_brk_num = -1
detector1 = []
detector2 = []
detector3 = []
detector4 = []
detector5 = []
detector6 = []
detector7 = []
num_extra_computations = 0
def core_break_pc(args, cpu, access, brknum, reg, size):
  global num_extra_computations
  global panic_brk_num
  if(brknum==panic_brk_num):
    count = conf.cpu0.steps - args[1][conf.cpu0]
    FILE = open(args[0],"w")
    FILE.write('Detected Panic\n')
    line = "Panic:" + str(conf.cpu0) + ":" + str(count) + " PC: " + str(hex(conf.cpu0.control_registers['pc']))
    FILE.write(line)
    FILE.write('\n')
    FILE.close()
    curr_pc = SIM_get_program_counter(conf.cpu0)
    print "0x%x" %curr_pc
    print "Panic here"
    run('quit 666')
    pass
  else:
#   check for detectors
    detected = 0
    if (args[2] == "blackscholes_simlarge" or args[2] == "blackscholes_input_run_00000"
            or args[2] == "blackscholes_input_run_00001"
            or args[2] == "blackscholes_input_run_00002"
            or args[2] == "blackscholes_input_run_00003"
            or args[2] == "blackscholes_input_run_00004"

            ):
    #if args[2] == "blackscholes_opt_abdul":
      curr_pc = SIM_get_program_counter(conf.cpu0)
      if curr_pc  == 0x100001818:
        del detector1[:]
        detector1.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"i2")))
        detector1.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"i3")))
        #detector1.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"i0")))
        detector1.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l5")))
        detector1.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l7")))
	num_extra_computations += 1
      if curr_pc == 0x1000018a0:
        if len(detector1) != 0:
	  num_extra_computations += 1
          i2 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i2"))
          i3 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i3"))
          #print "0x%x, i2=%d, i3=%d" %(curr_pc,i2,i3)
          denom = detector1.pop(0) - i2
          num = detector1.pop(0) - i3
          factor = num/denom
          print "factor=%d, given=4" %(factor)
          if factor != 4:
	    detected = 1
            print "invariant DIDN'T match at 0x%x" %curr_pc
          #i0 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i0"))
          l5 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l5"))
          l7 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l7"))
          #old_i0 = detector1.pop(0)
          old_l5 = detector1.pop(0)
          old_l7 = detector1.pop(0)
          if l5 != old_l5 or l7 != old_l7:
            print "Detection: regs dont have cont value at 0x%x" %curr_pc
	    detected = 1
      if curr_pc == 0x1000014cc:
        del detector2[:]
        detector2.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"o6")))
	num_extra_computations += 1
	# if SIM_cycle_count(conf.cpu0) >= 311129470287 and SIM_cycle_count(conf.cpu0) <= 311129475287:
  	#   print "+"
      if curr_pc == 0x1000015fc:
	# if SIM_cycle_count(conf.cpu0) >= 311129470287 and SIM_cycle_count(conf.cpu0) <= 311129475287:
  	#   print "-"
        if len(detector2) != 0:
	  num_extra_computations += 1
          i1 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i1"))
          sp = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o6"))
          i2 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i2"))
          l7 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l7"))
          old_sp = detector2.pop(0)
          if sp != old_sp or i1 - l7 != 0x478 or (i2 != 0 and i2 != 1):
            print "Detection: 0x%x" %curr_pc
	    detected = 1
	else:
	  num_extra_computations += 1
          i1 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i1"))
          i2 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i2"))
          if i1 != 0x100001478 or (i2 != 0 and i2 != 1):
            print "Detection: 0x%x" %curr_pc
	    detected = 1
      if curr_pc == 0x100001644:
        del detector3[:]
        detector3.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"o6")))
	num_extra_computations += 1
      if curr_pc == 0x100001708:
        if len(detector3) != 0:
	  num_extra_computations += 1
          sp = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o6"))
          old_sp = detector3.pop(0)
          if sp != old_sp :
            print "Detection: 0x%x" %curr_pc
	    detected = 1

    if args[2] == "lu_reduced":
      curr_pc = SIM_get_program_counter(conf.cpu0)
      if curr_pc  == 0x100004428:
        del detector1[:]
        detector1.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"o0")))
        detector1.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"o1")))
	num_extra_computations += 1
      if curr_pc == 0x10000445c:
        if len(detector1) != 0:
	  num_extra_computations += 1
          o0 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o0"))
          o1 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o1"))
          o5 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o5"))
          num1 = o0 - detector1.pop(0)
          num2 = o1 - detector1.pop(0)
          denom = o5
	  if denom != 0:
            factor1 = num1/denom
            factor2 = num2/denom
            if factor1 != 8 or factor2 != 8:
	      detected = 1
	      print "factor1=%d, given=8, factor2=%d, given=8" %(factor1, factor2 )
              print "invariant DIDN'T match at 0x%x" %curr_pc
      if curr_pc  == 0x1000043b8:
        del detector2[:]
        detector2.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l2")))
        detector2.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"i2"))) # l7
        detector2.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l6")))
        detector2.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l5")))
	num_extra_computations += 1
      if curr_pc == 0x1000043f4:
        if len(detector2) != 0:
	  num_extra_computations += 1
          l2 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l2"))
          l7 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l7"))
          l6 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l6"))
          l5 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l5"))
          l0 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l0"))
          num1 = l2 - detector2.pop(0)
          num2 = l7 - detector2.pop(0)
          denom = l0
	  if denom != 0:
            factor1 = num1/denom
            factor2 = num2/denom
            given_factor2 = detector2.pop(0)
	    given_factor1 = detector2.pop(0)
            if factor2 != given_factor2 or factor1 != given_factor1:
	      detected = 1
              print "invariant DIDN'T match at 0x%x" %curr_pc
	      print "factor1=%d, given=%d, factor2=%d, given=%d" %(factor1, factor2, given_factor1, given_factor2)
	      print "num1=%d, num2=%d, denom=%d" %(num1, num2, denom)
	      print "l2=%d, l7=%d, l6=%d, l5=%d, l0=%d" %(l2, l7, l6, l5, l0)
      if curr_pc  == 0x1000043b0:
        del detector3[:]
        detector3.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"i1")))
        detector3.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"i0")))
        detector3.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l3")))
	num_extra_computations += 1
      if curr_pc == 0x100004408:
        if len(detector3) != 0:
	  num_extra_computations += 1
          i1 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i1"))
          i0 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i0"))
          l3 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l3"))
          l4 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l4"))
          num1 = i1 - detector3.pop(0)
          num2 = i0 - detector3.pop(0)
          denom = l4
	  if denom != 0:
            factor1 = num1/denom
            factor2 = num2/denom
	    given_factor2 = detector3.pop(0)
            if factor1 != 8 or factor2 != given_factor2:
	      detected = 1
              print "invariant DIDN'T match at 0x%x" %curr_pc
	      print "factor1=%d, given=8, factor2=%d, given=%d" %(factor1, factor2, given_factor2)
	      print "num1=%d, num2=%d, denom=%d" %(num1, num2, denom)
	      print "i1=%d, i0=%d, l3=%d, l4=%d" %(i1, i0, l3, l4)

    if args[2] == "fft_small":
      print "HAHA THOUGHT THIS WAS USELESS!"
      curr_pc = SIM_get_program_counter(conf.cpu0)

      # 1. 0x1000035cc - g3 +1, g4 +16, g5 +16, o0 +16 - 0x10000357c
      if curr_pc  == 0x10000357c:
        del detector1[:]
        detector1.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"g3")))
        detector1.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"g4")))
        detector1.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"g5")))
        detector1.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"o0")))
	num_extra_computations += 1
      if curr_pc == 0x1000035cc:
        if len(detector1) != 0:
	  num_extra_computations += 1
          g3 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "g3"))
          g4 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "g4"))
          g5 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "g5"))
          o0 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o0"))
          denom = g3 - detector1.pop(0)
          num1 = g4 - detector1.pop(0)
          num2 = g5 - detector1.pop(0)
          num3 = o0 - detector1.pop(0)
	  if denom != 0:
            factor1 = num1/denom
            factor2 = num2/denom
            factor3 = num3/denom
            if factor1 != 16 or factor2 != 16 or factor3 != 16:
	      detected = 1
	      print "factor1=%d, given=16, factor2=%d, given=16, factor3=%d, given=16" %(factor1, factor2, factor3)
              print "invariant DIDN'T match at 0x%x" %curr_pc
      # 2. 0x1000043c4 - i4 +16, l4 +1, l5 +16 - 0x100004378
      if curr_pc  == 0x100004378:
        del detector2[:]
        detector2.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"i4")))
        detector2.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l5")))
        detector2.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l4")))
	num_extra_computations += 1
      if curr_pc == 0x1000043c4:
        if len(detector2) != 0:
	  num_extra_computations += 1
          i4 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i4"))
          l5 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l5"))
          l4 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l4"))
          num1 = i4 - detector2.pop(0)
          num2 = l5 - detector2.pop(0)
          denom = l4 - detector2.pop(0)
	  if denom != 0:
            factor1 = num1/denom
            factor2 = num2/denom
            if factor2 != 16 or factor1 != 16:
	      detected = 1
              print "invariant DIDN'T match at 0x%x" %curr_pc
	      print "factor1=%d, given=16, factor2=%d, given=16" %(factor1, factor2)
      # 3. 0x100004860 - l7 +1, i5 +16 - 0x100004810
      if curr_pc  == 0x100004810:
        del detector3[:]
        detector3.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"i5")))
        detector3.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l7")))
	num_extra_computations += 1
      if curr_pc == 0x100004408:
        if len(detector3) != 0:
	  num_extra_computations += 1
          i5 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i5"))
          l7 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l7"))
          num1 = i5 - detector3.pop(0)
          denom = l7 - detector3.pop(0)
	  if denom != 0:
            factor1 = num1/denom
            if factor1 != 16:
	      detected = 1
              print "invariant DIDN'T match at 0x%x" %curr_pc
	      print "factor1=%d, given=16" %(factor1)
      # 4. 0x100004750 - i2 +1, i1 +16, i5 +g4 - 0x100004728
      if curr_pc  == 0x100004728:
        del detector4[:]
        detector4.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"i1")))
        detector4.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"o7")))  #i5
        #detector4.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"g4")))
        detector4.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"i2")))
	num_extra_computations += 1
      if curr_pc == 0x100004750:
        if len(detector4) != 0:
	  num_extra_computations += 1
          i1 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i1"))
          i5 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i5"))
          g4 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "g4"))
          i2 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i2"))
          num1 = i1 - detector4.pop(0)
          num2 = i5 - detector4.pop(0)
	  given_factor2 = g4
          denom = i2 - detector4.pop(0)
	  if denom != 0:
            factor1 = num1/denom
            factor2 = num2/denom
            if factor1 != 16 or factor2 != given_factor2:
	      detected = 1
              print "invariant DIDN'T match at 0x%x" %curr_pc
	      print "factor1=%d, given=16, factor2=%d, given=%d" %(factor1, factor2, given_factor2)
      # 5. 0x100004954 - l3 +1, l0 +16, l1 +16, o7 +16 - 0x1000048e4
      if curr_pc  == 0x1000048e4:
        del detector5[:]
        detector5.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l0")))
        detector5.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l1")))
        detector5.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"o7")))
        detector5.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"l3")))
	num_extra_computations += 1
      if curr_pc == 0x100004954:
        if len(detector5) != 0:
	  num_extra_computations += 1
          l0 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l0"))
          l1 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l1"))
          o7 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o7"))
          l3 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "l3"))
          num1 = l0 - detector5.pop(0)
          num2 = l1 - detector5.pop(0)
          num3 = o7 - detector5.pop(0)
          denom = l3 - detector5.pop(0)
	  if denom != 0:
            factor1 = num1/denom
            factor2 = num2/denom
            factor3 = num3/denom
            if factor1 != 16 or factor2 != 16 or factor3 != 16:
	      detected = 1
              print "invariant DIDN'T match at 0x%x" %curr_pc
	      print "factor1=%d, given=16, factor2=%d, given=16, factor3=%d, given=16" %(factor1, factor2, factor3)
	      print "l0=%d, l1=%d, o7=%d, l3=%d" %(l0, l1, o7, l3)
      # 6. 0x1000039dc - o3 == 00
      if curr_pc == 0x1000039dc:
	num_extra_computations += 1
        o3 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o3"))
        o0 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o0"))
        if o3 != o0:
	  detected = 1
          print "invariant DIDN'T match at 0x%x" %curr_pc
	  print "o3=%d, o0=%d" %(o3, o0)

      # 7. 0x100004968 - o2 +1, o3 +o5, o4 +o5 - 0x1000048c0
      if curr_pc  == 0x1000048c0:
        del detector7[:]
        detector7.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"o3")))
        detector7.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"o4")))
        detector7.append(SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0,"o2")))
	num_extra_computations += 1
      if curr_pc == 0x100004968:
        if len(detector7) != 0:
	  num_extra_computations += 1
          o3 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o3"))
          o4 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o4"))
          o2 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o2"))
          o5 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "o5"))
          num1 = o3 - detector7.pop(0)
          num2 = o4 - detector7.pop(0)
          denom = o2 - detector7.pop(0)
	  if denom != 0:
            factor1 = num1/denom
            factor2 = num2/denom
            if factor1 != o5 or factor2 != o5:
	      detected = 1
              print "invariant DIDN'T match at 0x%x" %curr_pc
	      print "factor1=%d, given=%d, factor2=%d" %(factor1, o5, factor2)

      # 8. 0x100004978 - g4 +1, i1
      if curr_pc == 0x100004978:
	num_extra_computations += 1
        g4 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "g4"))
        i1 = SIM_read_register(conf.cpu0,SIM_get_register_number(conf.cpu0, "i1"))
        if g4 != i1:
	  detected = 1
          print "invariant DIDN'T match at 0x%x" %curr_pc
	  print "g4=%d, i1=%d" %(g4, i1)


    #print "injected_trans_fault=%d, detected=%d" %(injected_trans_fault,detected)
    global injected_trans_fault
    if injected_trans_fault == 1:
      if detected == 1:
        count = conf.cpu0.steps - args[1][conf.cpu0]
        FILE = open(args[0],"w")
        FILE.write('Detected App level \n')
        line = "App-level-invariant:" + str(conf.cpu0) + ":" + str(count) + " PC: " + str(hex(conf.cpu0.control_registers['pc']))
        FILE.write(line)
        FILE.write('\n')
        FILE.close()
        curr_pc = SIM_get_program_counter(conf.cpu0)
        print "0x%x" %curr_pc
        print "App level Detector"
        run('quit 666')

def add_detector_locations(app):
  print "INSIDE ADD_DETECTOR_LOCATIONS"
  detector_pc_list = []
  if app == "lu_reduced":
    detector_pc_list = [0x100004428, 0x10000445c, 0x1000043b0, 0x1000043f4, 0x1000043b4, 0x100004408]
  if app == "blackscholes_simlarge":
    detector_pc_list = [0x1000018a0, 0x100001818, 0x1000015fc, 0x1000014cc,  0x100001708, 0x100001644]
  if app == "fft_small":
    print "WHY DID I MISS THIS????"
    detector_pc_list = [0x1000035cc, 0x10000357c, 0x1000043c4, 0x100004378,
    0x100004860, 0x100004810, 0x100004750, 0x100004728, 0x100004954,
    0x1000048e4, 0x1000039dc, 0x100004968, 0x1000048c0,
    0x100004978]

  for detector_location in detector_pc_list:
    SIM_breakpoint(conf.primary_context,
                  Sim_Break_Virtual,
                  Sim_Access_Execute,
                  detector_location,
                  4,
                  Sim_Breakpoint_Simulation)


def get_injected_val(golden_val, bit):
  mask = 1<<bit
  #print "mask=%x" %mask
  faulty_val = golden_val
  print "golden reg val=%d (%x)" %(golden_val, golden_val)
  if golden_val & mask == 0:
    faulty_val = golden_val | mask
  else:
    faulty_val = golden_val & (~mask)
  print "faulty val=%d (%x)" %(faulty_val, faulty_val)
  return faulty_val

def inject_trans_fault(pc, faultreg, bit, srcdest, faulttype):
  int_pc = int(pc,16)
  curr_pc = SIM_get_program_counter(conf.cpu0)
  if curr_pc == int_pc:
    print "injecting at pc=%x" %(curr_pc)
    print "faultreg = %d" %(faultreg)
    print "faulttype = %d" %(faulttype)
    faulty_val = 0
    golden_val = 0
    cp = conf.cpu0

# read a fp
    #valueHERE = cp.iface.sparc_v9.read_fp_register_i(cp, faultreg)



    if srcdest == 0:
      # inject now
      print "Injecting Transient fault"
      print "injecting in source"

      if faulttype == 4:
          golden_val = SIM_read_register(conf.cpu0,faultreg)
      elif faulttype == 12:
          golden_val = cp.iface.sparc_v9.read_fp_register_i(cp, faultreg)
      elif faulttype == 16:
          golden_val = cp.iface.sparc_v9.read_fp_register_x(cp, faultreg)

      faulty_val = get_injected_val(golden_val,bit)
      # To test whether the register exists at this point
      # instruction_info = SIM_disassemble(conf.cpu0,curr_pc,1)
      # print instruction_info
      # instruction = instruction_info[1]
      # split_instruction = instruction.split()
      # opcode = split_instruction[0]
    else:
      # inject in next instruction
      SIM_continue(1)
      print "Injecting Transient fault"
      print "injecting in destination register"
#      golden_val = SIM_read_register(conf.cpu0,faultreg)


      if faulttype == 4:
          golden_val = SIM_read_register(conf.cpu0,faultreg)
      elif faulttype == 12:
          golden_val = cp.iface.sparc_v9.read_fp_register_i(cp, faultreg)
      elif faulttype == 16:
          golden_val = cp.iface.sparc_v9.read_fp_register_x(cp, faultreg)

      faulty_val = get_injected_val(golden_val,bit)
      pl = SIM_processor_privilege_level(conf.cpu0)
      if pl == 1:
        print "please check this injection"


    if faulttype == 4: # gpio
        print "integer injection"
        SIM_write_register(conf.cpu0,faultreg,faulty_val)
    elif faulttype == 12: # floating point
        print "floating point injection"
        cp.iface.sparc_v9.write_fp_register_s(cp, faultreg, faulty_val)
    elif faulttype == 16: # double floating point
        print "double floating point injection"
        cp.iface.sparc_v9.write_fp_register_d(cp, faultreg, faulty_val)

    global injected_trans_fault
    injected_trans_fault = 1
    written_val = -1

    if faulttype == 4:
        written_val = SIM_read_register(conf.cpu0,faultreg)
    elif faulttype == 12:
        written_val = cp.iface.sparc_v9.read_fp_register_s(cp, faultreg)
    elif faulttype == 16:
        written_val = cp.iface.sparc_v9.read_fp_register_d(cp, faultreg)

    if faulttype == 4:
        print "written val=%d (%x)" %(written_val, written_val)
    elif faulttype == 12:
        print "written val=%f" %(written_val)
    elif faulttype == 16:
        print "written val=%f" %(written_val)

#    written_val = SIM_read_register(conf.cpu0,faultreg)

def measure_detector_overhead(app):
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  print 'starting simulation from the begining'

  # add_detector_locations(app)
  import app_detectors
  # init app detectors
  app_detectors.create_detectors(app)
  app_detectors.add_detector_locations(app)

  filename = '%s/%s.completion_output' % (LOCAL_OUTPUT_DIR, app)
  cpu_p = [ SIM_proc_no_2_ptr(i) for i in range(1) ]
  begin = dict([(p, p.steps) for p in cpu_p])
  SIM_hap_add_callback("Core_Breakpoint", app_detectors.core_break_pc, [filename, begin, app])

  run_sim_command('c 10')
  run_sim_command('cpu-switch-time 1000')
  run_sim_command('magic-break-enable')

  print filename

  app_detectors.panic_break_num = SIM_breakpoint(conf.primary_context,
                                         Sim_Break_Virtual,
                                         Sim_Access_Execute,
                                         0x1049bac,
                                         216,
                                         Sim_Breakpoint_Simulation)
  SIM_hap_add_callback("Core_Exception", core_exception, [ filename, begin ])
  SIM_hap_add_callback("Core_Magic_Instruction", core_magic, [filename, begin])

  SIM_continue(0)

  #while(magic_reached == 0):
  #  print "in while loop"
  #  threshold_cycle = 8000000000 #  billion
  #  run_sim_command('c %d' %threshold_cycle)
  #  curr_cycle = SIM_cycle_count(conf.cpu0)

  if(magic_reached == 1):
    #print "Yes, Magic reached! "
    print ""
    extra_instructions = app_detectors.get_num_extra_computation(app)
    print "Num extra computations = %d (collectors=%d, checkers=%d)" %(extra_instructions[0], extra_instructions[1], extra_instructions[2])

  print "Reached magic call"


def run_arch_all(app, phase, seqnum, pc, core=-1,  type=-1, bit=-1, stuck=-1, faultreg=64, srcdest=0, pilot=0, injcycle=0):
  #print 'app=%s, phase=%s, seqnum=%s, pc=%s, core=%s,  type=%s, bit=%s, stuck=%s, faultreg=%s, srcdest=%s, injcycle=%s'%(app, phase, seqnum, pc, core,  type, bit, stuck, faultreg, srcdest, injcycle)

  start_points = [ 4310623771176 ]

  checkpoint_gap = 100000000
  initial_skip = 1
  print 'injection cycyle = %s' % injcycle
  start_point = 0
  if injcycle < start_points[0]:
    run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
    print 'starting simulation from the begining'
    initial_skip = 1
  else:
    found_start_point = 0
    for start_point in start_points:
      if injcycle < start_point:
        found_start_point = 1
        break
    if found_start_point == 1:
      start_point = start_point - checkpoint_gap
    if injcycle - start_point < 1000:
      start_point = start_point - checkpoint_gap
    print 'starting simulation at %s' % start_point
    initial_skip = 0
    run_sim_command('read-configuration %s/intermediate/%s_%s' % (CHECKPOINT_DIR,app,start_point))

  #run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  #run_sim_command('istc-disable')
  #run_sim_command('dstc-disable')

  # add_detector_locations(app)
  import app_detectors
  # init app detectors
  app_detectors.create_detectors(app)
  app_detectors.add_detector_locations(app)

  filename = '%s/%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s.completion_output' % (LOCAL_OUTPUT_DIR, app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)
  cpu_p = [ SIM_proc_no_2_ptr(i) for i in range(1) ]
  begin = dict([(p, p.steps) for p in cpu_p])
  #SIM_hap_add_callback("Core_Breakpoint", core_break_pc, [filename, begin, app])
  SIM_hap_add_callback("Core_Breakpoint", app_detectors.core_break_pc, [filename, begin, app])

  if initial_skip == 1:
    run_sim_command('c 10')
  run_sim_command('cpu-switch-time 1000')

  #curr_cycle = SIM_cycle_count(conf.cpu0)
  #skip_cycles = injcycle - curr_cycle
  #print 'Skip %s instructions'%(skip_cycles)
  #run_sim_command('c %d' %(skip_cycles))
  run_sim_command('cba %d' %(injcycle))
  SIM_continue(0)
  curr_cycle = SIM_cycle_count(conf.cpu0)
  while curr_cycle != injcycle:
    SIM_continue(0)
    curr_cycle = SIM_cycle_count(conf.cpu0)

  #SIM_set_attribute(conf.sim, "cpu_switch_time", 1)
  run_name = '%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s' % (app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)
  print run_name
  inject_trans_fault(pc, faultreg, bit, srcdest, type)

  print "Running benchmark to completion\n"
  print filename
  begin = dict([(p, p.steps) for p in cpu_p])

  app_detectors.panic_break_num = SIM_breakpoint(conf.primary_context,
                                         Sim_Break_Virtual,
                                         Sim_Access_Execute,
                                         0x1049bac,
                                         216,
                                         Sim_Breakpoint_Simulation)
  SIM_hap_add_callback("Core_Exception", core_exception, [ filename, begin ])
  SIM_hap_add_callback("Core_Magic_Instruction", core_magic, [filename, begin])

  run_sim_command('magic-break-enable')
  run_sim_command('cpu-switch-time 1000')
  start_cycle = SIM_cycle_count(conf.cpu0) # for timeout detection
  capture_log = '%s/%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s.capture_log' % (LOCAL_OUTPUT_DIR, app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)
  run_sim_command('con0.capture-start %s' % (capture_log))

  while(magic_reached == 0):
    threshold_cycle = 8000000000 #  billion
    run_sim_command('c %d' %threshold_cycle)

    curr_cycle = SIM_cycle_count(conf.cpu0)
#   you reached the magic break or something is wrong
#    print "This should be a magic break"
    if(magic_reached == 1):
      #print "Yes, Magic reached! "
      print ""
      SIM_delete_breakpoint(app_detectors.panic_break_num)
      #SIM_delete_breakpoint(panic_brk_num)
      extra_instructions = app_detectors.get_num_extra_computation(app)
      print "Num extra computations = %d (collectors=%d, checkers=%d)" %(extra_instructions[0], extra_instructions[1], extra_instructions[2])
    else:
      end_cycle = SIM_cycle_count(conf.cpu0) # for timeout detection
      if(end_cycle-start_cycle >= threshold_cycle):
        FILE = open(filename,"w")
        FILE.write('Detected Timeout\n')
        FILE.close()
        print "Timeout"
        run('quit 666')

  curr_cycle = SIM_cycle_count(conf.cpu0)
  diff_cycle = curr_cycle - injcycle
  print "cycles since injection= %d" %(diff_cycle)

  run_sim_command('con0.capture-stop')

  check_app_error(capture_log, filename, "")

  # unmount/mount to completely flush file system caches (sync just doesn't work)
  #run_inside('umount /export/home; magic_brk; \n')
  #run_inside('mount /export/home; magic_brk; \n')
  # save the end state
  fault = '%s.pc%s.p%s.c%s.t%s.b%s.s%s.r%s.d%s.%s' % (app, pc, pilot, core, type, bit, stuck, faultreg, srcdest, seqnum)
  craff_filename = '%s/%s.diff.craff' % (CRAFF_DIR, fault)
  run_sim_command('con0.capture-start %s' % (capture_log))
  run_inside_safe(' devfsadm ; mkdir /mnt/new_disk ; magic_brk\n', filename)
  run_inside_safe(' mount /dev/dsk/c0t1d0s2 /mnt/new_disk ; magic_brk\n', filename)
  run_inside_safe(' cp output.txt /mnt/new_disk/%s.output ; magic_brk\n' %(fault), filename)
  if(app == "libquantum_test" or app == "omnetpp_test" ):
    run_inside_safe(' cp output.err /mnt/new_disk/%s.output.err ; magic_brk\n' %(fault), filename)
  run_inside_safe(' umount /mnt/new_disk ; magic_brk\n', filename)
  run_sim_command('con0.capture-stop')
  check_app_error(capture_log, filename, "Later")
  os.remove('%s'%(capture_log))
  run('sd1.save-diff-file %s' %(craff_filename))
  print "Reached here"


# find length of application
def find_length(app):
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  MAIN_LOG_DIR = get_main_log_dir(app)
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  run_sim_command('c 10')
  count_app = 0
  num_cycles = 0
  actual_num_cycles = 0
  global global_magic
  SIM_hap_add_callback("Core_Magic_Instruction", hap_magic_call_reached, app)
  while True:
    pc = SIM_get_program_counter(conf.cpu0)
    if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
      num_cycles = num_cycles + 1
      actual_num_cycles = actual_num_cycles + count_app
      if pc == app_txt_info.actual_app_start[app]:
        count_app = 1
      if pc == app_txt_info.actual_app_end[app]:
        count_app = 0
    if global_magic == 0:
      SIM_continue(1)
    else:
      break
  print "%s\t%d\t%d" %(app, actual_num_cycles, num_cycles)
  filename = MAIN_LOG_DIR + app
  filename = filename + '_app_length.txt'
  pf = open(filename,'w')
  out_string = app + '\t' + str(actual_num_cycles) + '\t' + str(num_cycles) + '\n'
  pf.write(out_string)
  pf.close()
  run_sim_command('q')


def find_all_length(app):
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  MAIN_LOG_DIR = get_main_log_dir(app)
  run_sim_command('read-configuration %s/%s'%(CHECKPOINT_DIR,app))
  run_sim_command('c 10')
  start_cycle_count = SIM_cycle_count(conf.cpu0)
  run_sim_command('magic-break-enable')
  run_sim_command('b %d' %(app_txt_info.actual_app_start[app]))
  run_sim_command('b %d' %(app_txt_info.actual_app_end[app]))
  run_sim_command('c')
  actual_start_cycle_count = SIM_cycle_count(conf.cpu0)
  run_sim_command('c')
  actual_end_cycle_count = SIM_cycle_count(conf.cpu0)
  run_sim_command('c')
  end_cycle_count = SIM_cycle_count(conf.cpu0)

  num_cycles = end_cycle_count - start_cycle_count
  actual_num_cycles = actual_end_cycle_count - actual_start_cycle_count

  print "%s\t%d\t%d" %(app, actual_num_cycles, num_cycles)
  filename = MAIN_LOG_DIR + app
  filename = filename + '_length.txt'
  pf = open(filename,'w')
  out_string = app + '\t' + str(actual_num_cycles) + '\t' + str(num_cycles) + '\n'
  pf.write(out_string)
  pf.close()

  run_sim_command('q')

def count_static_instructions(app):
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  MAIN_LOG_DIR = get_main_log_dir(app)
  print '%s/%s' % (CHECKPOINT_DIR,app)
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  global global_magic
  SIM_hap_add_callback("Core_Magic_Instruction", hap_magic_call_reached, app)
  run_sim_command('b %d' %app_txt_info.actual_app_start[app])
  run_sim_command('c')

  pc_counters = {} # dictionary pc:counter

  while True:
  #for i in range(0,100000000): # 10 million
    pc=SIM_get_program_counter(conf.cpu0)
    if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
      if pc not in pc_counters: # pc is the key in pc_counters dictionary
        pc_counters[pc] = 1

    if global_magic == 0:
      SIM_continue(1)
    else:
      break

  # print pc_list
  print app
  filename = MAIN_LOG_DIR + app + '_static_instrn_count.txt'
  pf = open(filename,'w')
  key_list = pc_counters.keys()
  key_list.sort()
  for k in key_list:
    line = str(hex(k))
    pf.write(line)
    pf.write('\n')
  pf.close()
  print 'num emelents in the dict: %s' %len(pc_counters)
  run("quit 666")

malloc_start = 0
free_start = 0
temp_size = 0
pointer_size_dict = {}
num_edits = 0

def hap_breakpoint_reached(args, mem, access, brknum, reg, size):
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  global malloc_start
  global free_start
  global temp_size
  global num_edits
  if brknum == malloc_start:
    temp_size = SIM_read_register(conf.cpu0, SIM_get_register_number(conf.cpu0,"o0"))
    #print "Malloc start reached. Size requested = %d" %temp_size
    SIM_breakpoint(conf.primary_context,
                         Sim_Break_Virtual,
                         Sim_Access_Execute,
                         app_txt_info.text_start[args],
                         app_txt_info.text_end[args]-app_txt_info.text_start[args],
                         Sim_Breakpoint_Simulation)
  elif brknum == free_start:
    pointer = SIM_read_register(conf.cpu0, SIM_get_register_number(conf.cpu0,"o0"))
    #print "Free start reached. pointer to be deleted = %d" %pointer
    num_edits += 1
    if pointer in pointer_size_dict:
      #del pointer_size_dict[pointer]
      pass
    else:
      print "Error: pointer (%d) not allocated!!" %pointer
  else:
    SIM_delete_breakpoint(brknum)
    pointer = SIM_read_register(conf.cpu0, SIM_get_register_number(conf.cpu0,"o0"))
    num_edits += 1
    if pointer in pointer_size_dict:
      print "Error: pointer (%d) already allocated!!" %pointer
    else:
      pointer_size_dict[pointer] = temp_size
    #print "Reached end of malloc. pointer = %d" %pointer

def hap_exp_inout_values(args, mem, access, brknum, reg, size):
  f0 = OLD_read_fp_register_i(conf.cpu0,0)
  if f0 != 0:
    f0_str = str(hex(f0))[2:-1]
    f0 = struct.unpack(">f", f0_str.decode('hex'))[0]

  f1 = OLD_read_fp_register_i(conf.cpu0,1)
  if f1 != 0:
    f1_str = str(hex(f1))[2:-1]
    f1 = struct.unpack(">f", f1_str.decode('hex'))[0]

  print "input: %f, output=%f" %(f1, f0)
#  print "input: %f" %(f1)

def test_exp_values(app):
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('cpu-switch-time 1000')
  run_sim_command('magic-break-enable')
#  break_pcs = [0x10000357c, 0x10000358c, 0x1000035a4, 0x1000035c0, 0x100003614, 0x1000038cc, 0x1000038cc]
#  break_pcs = [0x100003520]
  break_pcs = [0x100001508, 0x10000159c, 0x10000171c, 0x1000017b4, 0x100001858,
  0x1000018f0, 0x100001984, 0x100001dfc, 0x100001e94, 0x100001f38, 0x100001fd4,
  0x100002068, 0x100002208, 0x10000229c, 0x100002340, 0x1000023dc, 0x100002470,
  0x100002bac, 0x100002c40, 0x100002ce4, 0x100002d80, 0x100002e14, 0x100002f5c,
  0x100002ff4, 0x100003098, 0x100003134, 0x1000031c8 ]

  for pc in break_pcs:
    SIM_breakpoint(conf.primary_context,
                         Sim_Break_Virtual,
                         Sim_Access_Execute,
                         pc,
                         4,
                         Sim_Breakpoint_Simulation)

  SIM_hap_add_callback("Core_Breakpoint", hap_exp_inout_values, app)
  run_sim_command('c')


def check_address(args, obj, curr_cycle_count):
# Old value = 0x10015b038, New value = 0x10014b038
  found = False
  for k in pointer_size_dict :
    v = pointer_size_dict[k]
    #if k <= 0x100176200 <= k+v :
    if k <= 0x10014b038 <= k+v :
      found = True
      break
  if found :
    print "Faulty Address found in allocated addresses";
  else:
    print "Faulty Address NOT found in allocated addresses";

  found = False
  for k in pointer_size_dict :
    v = pointer_size_dict[k]
    #if k <= 0x100166200 <= k+v :
    if k <= 0x10015b038 <= k+v :
      found = True
      break
  if found :
    print "Address found in allocated addresses";
  else:
    print "Address NOT found in allocated addresses";

def print_malloc_map():
  for k in pointer_size_dict :
    v = pointer_size_dict[k]
    print "start=%x, end=%x" %(k, k+v)

def find_address_map(app):
  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('istc-enable')
  run_sim_command('dstc-enable')
  run_sim_command('c 10')
  run_sim_command('magic-break-enable')
  global malloc_start
  global free_start
  # for malloc start
  malloc_start = SIM_breakpoint(conf.primary_context,
                         Sim_Break_Virtual,
                         Sim_Access_Execute,
                         0x100104ba0, #0x10011d6e0,
                         4,
                         Sim_Breakpoint_Simulation)
  # for free start
  free_start = SIM_breakpoint(conf.primary_context,
                         Sim_Break_Virtual,
                         Sim_Access_Execute,
                         0x100104c00, #0x10011d700,
                         4,
                         Sim_Breakpoint_Simulation)
  SIM_hap_add_callback("Core_Breakpoint", hap_breakpoint_reached, app)
  #SIM_hap_add_callback_index("Core_Cycle_Count", check_address, app, 384034732268)
  run_sim_command('c')
  global num_edits
  print "num_edits = %d" %num_edits
  print_malloc_map()
  run("quit 666")

def get_reg_number(reg):
  #print reg[1:3]
  if reg == "%sp":
    reg = "%o6"
  elif reg == "%fp":
    reg = "%i6"
  return SIM_get_register_number(conf.cpu0,reg[1:3])

def gen_targeted_random_injection_points(app):
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('cpu-switch-time 1000')
  run_sim_command('magic-break-enable')

  dummy_app = app
  if app == "swaptions_simsmall_large":
    dummy_app = "swaptions_simsmall"

  pc_start = -1
  pc_end = -1
  reg_names = []
  filename = "detectors_details.txt"
  rf = open(filename,'r')
  for line in rf:
    sl = line.rstrip().split(":")
    if len(sl) > 2:
      if sl[0] == dummy_app:
        if sl[2] == "T9" or sl[2] == "T10" or sl[2] == "T11" or sl[2] == "T7":
          pc_start = int(sl[3], 16)
          pc_end = int(sl[4], 16)
          # print sl
          if sl[5] == "all":
            reg_names = ["%i0", "%i1", "%i2", "%i3", "%i4", "%i5", "%i6", "%i7",
	                 "%l0", "%l1", "%l2", "%l3", "%l4", "%l5", "%l6", "%l7",
                         "%o0", "%o1", "%o2", "%o3", "%o4", "%o5", "%o6", "%o7",
                 	 "%g1", "%g2", "%g3", "%g4", "%g5", "%g6", "%g7", "%sp", "%fp"]
          else:
            for word in sl[5].split("-"):
              word = "%"+word
              reg_names.append(word)

  # print "%s\t%s\t" %(pc_start, pc_end)
  print reg_names

  run_sim_command('b %d' %(app_txt_info.actual_app_start[dummy_app]))
  run_sim_command('c')

  # acutual app execution starts now - after init
  import re
  expressions = [re.compile(r"b"), re.compile(r"cmp"), re.compile(r"fcmp"), re.compile(r"prefet"), re.compile(r"st")]

  print "created random points "

  # for pc in range(pc_start, pc_end, 4):
  #   run_sim_command('b %d' %(pc))
  # run_sim_command('c')

  jobs_list = []
  start_cycle = SIM_cycle_count(conf.cpu0)
  seq_num = 0
  skip_count = 2
  while True:
    pc = SIM_get_program_counter(conf.cpu0)
    if pc_start <= pc <= pc_end:
      # print "%s <= %s <= %s" %(pc_start, pc, pc_end)
      try:
        instruction_info = SIM_disassemble(conf.cpu0, pc, 1)
        instruction = instruction_info[1]
        split_instruction = instruction.split()
        # print split_instruction
        found_regs = []
        for op in split_instruction:
          for reg in reg_names:
            pos = op.find(reg)
            if pos != -1:
              #print op[pos:pos+3]
              found_regs.append(op[pos:pos+3])

        len_found_regs = len(found_regs)
        if len_found_regs > 0:
          reg_index = random.randint(0,len_found_regs-1)
          #print "%d %d" %(len_found_regs, reg_index)
          dest = 0
          if split_instruction[len(split_instruction)-1].find(found_regs[reg_index]) != -1:
            opcode = split_instruction[0]
            no_dest = 0
            for e in expressions:
              if e.match(opcode):
                no_dest = 1
            if no_dest == 0:
              dest = 1
            job_str = "%s-0x%x-%d-%d-%d-%d-4" %(app, pc, SIM_cycle_count(conf.cpu0), get_reg_number(found_regs[reg_index]), random.randint(0,63), dest)
            jobs_list.append(job_str)
            #print "%s.0x%x.%d.%d.%d.%d.%d.4" %(app, pc, SIM_cycle_count(conf.cpu0), seq_num, get_reg_number(found_regs[reg_index]), random.randint(0,63), dest)
            seq_num += 1
            skip_count += 1
      except:
        print "exception"
        pass
    if seq_num%100 == 1:
      skip_cyc = int(actual_app_length[app]/10000)
      SIM_continue(skip_cyc)
      skip_count = 2
    else:
      SIM_continue(1)
    end_cycle = SIM_cycle_count(conf.cpu0)
    if end_cycle > start_cycle+actual_app_length[app]:
      break
  print seq_num

  seq_num = 0
  num_jobs = 10000
  if app == "fft_small":
    print "MISSED THIS TOO???"
    num_jobs = 10000
  if len(jobs_list) > num_jobs:
    for point in range(num_jobs):
      seq_num += 1
      factor = int(len(jobs_list)/num_jobs)
      sl = jobs_list[(point*factor)].split("-")
      job_name = "%s.%s.%s.%d.%s.%s.%s.4" %(sl[0], sl[1], sl[2], seq_num, sl[3], sl[4], sl[5])
      print job_name
  else:
    for job in jobs_list:
      seq_num += 1
      sl = job.split("-")
      job_name = "%s.%s.%s.%d.%s.%s.%s.4" %(sl[0], sl[1], sl[2], seq_num, sl[3], sl[4], sl[5])
      print job_name

def gen_random_injection_points(app):
  app_txt_info = get_app_txt_info(app)
  app_txt_info.OPT_LEVEL = OPT_LEVEL
  app_txt_info.init_txt_info()


  run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR,app))
  run_sim_command('instruction-fetch-mode instruction-fetch-trace')
  run_sim_command('istc-disable')
  run_sim_command('dstc-disable')
  run_sim_command('c 10')
  run_sim_command('cpu-switch-time 1000')
  run_sim_command('magic-break-enable')
  run_sim_command('b %d' %(app_txt_info.actual_app_start[app]))
  run_sim_command('c')

  # acutual app execution starts now - after init
  print "creating random points"
  curr_cycle_count = SIM_cycle_count(conf.cpu0)
  print "curr=%s" %(curr_cycle_count)
  random.seed(111)
  random_points = [ random.randint(curr_cycle_count, curr_cycle_count+actual_app_length[app]) for i in range(10000)]
  random_points.sort()
  print len(random_points)

  import re
  expressions = [re.compile(r"b"), re.compile(r"cmp"), re.compile(r"fcmp"), re.compile(r"prefet"), re.compile(r"st")]
  reg_names = ["%i0", "%i1", "%i2", "%i3", "%i4", "%i5", "%i6", "%i7",
	       "%l0", "%l1", "%l2", "%l3", "%l4", "%l5", "%l6", "%l7",
	       "%o0", "%o1", "%o2", "%o3", "%o4", "%o5", "%o6", "%o7",
	       "%g1", "%g2", "%g3", "%g4", "%g5", "%g6", "%g7", "%sp", "%fp"]

  print "created random points "

  seq_num = 0
  for point in random_points:
    if point > SIM_cycle_count(conf.cpu0):
      run_sim_command('cba %d' %point)
      run_sim_command('c')
      curr_cycle = SIM_cycle_count(conf.cpu0)
      #print "curr=%s point=%s" %(curr_cycle, point)
      # point reached - check its properties
      break_now = 0
      thresh = 0
      while True:
        pc = SIM_get_program_counter(conf.cpu0)
        if app_txt_info.text_start[app] <= pc <= app_txt_info.text_end[app]:
          # print "%s <= %s <= %s" %(app_txt_info.text_start[app], pc, app_txt_info.text_end[app])
          thresh = 0
          try:
            instruction_info = SIM_disassemble(conf.cpu0,pc,1)
            instruction = instruction_info[1]
            split_instruction = instruction.split()
            # print split_instruction
            found_regs = []
            for op in split_instruction:
              for reg in reg_names:
                pos = op.find(reg)
                if pos != -1:
                  #print op[pos:pos+3]
                  found_regs.append(op[pos:pos+3])

            len_found_regs = len(found_regs)
            if len_found_regs > 0:
              reg_index = random.randint(0,len_found_regs-1)
              #print "%d %d" %(len_found_regs, reg_index)
              dest = 0
              if split_instruction[len(split_instruction)-1].find(found_regs[reg_index]) != -1:
                opcode = split_instruction[0]
                no_dest = 0
                for e in expressions:
                  if e.match(opcode):
                    no_dest = 1
                if no_dest == 0:
                  dest = 1
              print "%s.0x%x.%d.%d.%d.%d.%d.4" %(app, pc, SIM_cycle_count(conf.cpu0), seq_num, get_reg_number(found_regs[reg_index]), random.randint(0,63), dest)
              seq_num += 1
              break_now = 1
          except:
            print "exception"
            pass
        if break_now:
          break
        else:
          rand_num  = random.randint(1, 10)
          #print "continuing %s cycles" %(rand_num)
          SIM_continue(rand_num)
          thresh += 1
          if thresh > 100:
            break
          curr_cycle = SIM_cycle_count(conf.cpu0)
          if curr_cycle > curr_cycle_count+actual_app_length[app]:
            break

def make_iso_checkpoint(iso_name):
    run_sim_command('read-configuration %s'%(BASE_CHECKPOINT))
    run_sim_command('instruction-fetch-mode instruction-fetch-trace')
    run_sim_command('istc-disable')
    run_sim_command('dstc-disable')
    run_sim_command('c 10')
    run_sim_command('magic-break-enable')
    cdrom_file = "%s/iso/%s.iso" % (WORKLOADS_PATH,iso_name)
    run('new-file-cdrom "%s"'%(cdrom_file))
    run_inside(' umount /mnt/cdrom ;magic_brk\n')
    run_sim_command('cd25B_2_6.eject ')
    run_sim_command('cd25B_2_6.insert %s'%(iso_name))
    #run_sim_command('cd0.eject ') #khalique
    #run_sim_command('cd0.insert %s'%(iso_name)) #khalique
    run_inside(' mount -F hsfs /dev/dsk/c0t6d0s0 /mnt/cdrom ;magic_brk\n')
    run_inside(' mkdir /export/home/apps/%s ;magic_brk\n'%(iso_name))
    run_inside(' cp -R /mnt/cdrom/* /export/home/apps/%s ;magic_brk\n'%(iso_name)) # Radha's checkpoint
    #run_inside(' cp -R /mnt/cdrom/%s/* /export/home/apps/%s ;magic_brk\n'%(iso_name, iso_name)) # khalique checkpoint
    run_inside(' cd /export/home/apps/%s ;magic_brk\n'%(iso_name))
    checkpoint_file = '%s/%s' % (CHECKPOINT_DIR, iso_name)
    run_sim_command('write-configuration -z "%s"' % (checkpoint_file))
    print "Checkpoint resides in %s" % (CHECKPOINT_DIR)

def make_run_checkpoint(iso_name, app_name, run_name):
    run_sim_command('read-configuration %s/%s' % (CHECKPOINT_DIR, iso_name))
    run_sim_command('instruction-fetch-mode instruction-fetch-trace')
    run_sim_command('istc-disable')
    run_sim_command('dstc-disable')
    run_sim_command('c 10')
    run_sim_command('magic-break-enable')
    run_inside('cd %s; magic_brk; \n'%(app_name))
    run_inside('magic_brk; ./%s.sh; magic_brk; \n'%(run_name))
    checkpoint_file = '%s/%s_%s' % (CHECKPOINT_DIR, app_name, run_name)
    run_sim_command('write-configuration -z "%s"' % (checkpoint_file))
    print "Checkpoint resides in %s" % (CHECKPOINT_DIR)

def read_fp_reg_i_cmd(cp, num):
    return cp.iface.sparc_v9.read_fp_register_s(cp,num)

