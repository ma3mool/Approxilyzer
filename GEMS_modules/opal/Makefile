#
# $Id: Makefile 1.125 06/01/31 21:35:22-06:00 lyen@opus.cs.wisc.edu $
#

.PHONY: all module tester clean tags

# GEMS_ROOT points to the base of the CVS tree
GEMS_ROOT := ..
GENERATED_DIR := generated
# GATE LEVEL SIM: edited
# VPATH := common:fetch:sparc:system:module:trace:tester:$(GEMS_ROOT)/common/ioutil:$(GENERATED_DIR)
VPATH := common:fetch:sparc:system:faultSim:module:trace:tester:$(GEMS_ROOT)/common/ioutil:$(GENERATED_DIR)

# DESTINATION = Your Destination Here

include $(GEMS_ROOT)/common/Makefile.common

#
# FLAGS
#
## Sun Native compiler: you must run configure in v9-sol8-32/64 directory with CC and CXX defined to be SUNWspro executables before enabling these options.
#CC = /opt/SUNWspro/bin/CC
#DEPEND_FLAGS := -xM1
#DEBUG_FLAGS += -DUSE_DIRENT -DNO_FSR -DSWAP_ENDIAN
#TESTER_LDFLAGS := -lCstd
#OPT_FLAGS := -g -c -xO0
# OPT_FLAGS := -O3 -pipe -fomit-frame-pointer -finline-functions -DNO_VECTOR_BOUNDS_CHECKS -DMULTIFACET_OPT_WARN -fPIC
#OPT_FLAGS := -xCC  -fast -xarch=v8plusa -xO4 -ftrap=%all,no%inexact -xs -xprefetch=yes

# if compiling to run standalone with ruby, specify a protocol here
PROTOCOL= MOSI_bcast_2level
RUBY_DIR=$(GEMS_ROOT)/ruby/$(PROTOCOL)/$(HOST_TYPE)/tester/

# define is_opal for cross-compiling packages
DEBUG_FLAGS += -DIS_OPAL

# My standard debug package
# DEBUG_FLAGS += -DMODINIT_VERBOSE -DVERIFY_SIMICS -DDEBUG_RETIRE -DDEBUG_LSQ -DDEBUG_FILTER -DDEBUG_TESTER

# Flag to print hte cycle every cycle
DEBUG_FLAGS += -DDEBUG_PRINT_CYCLE=0

# Flag to make sure that simics state is corrupted on fault
DEBUG_FLAGS += -DFAULT_CORRUPT_SIMICS

# Turn off assertion in Opal
DEBUG_FLAGS += -DOPAL_NO_ASSERT

# Flag to log faulty behavior in fault-log file
DEBUG_FLAGS += -DFAULT_LOG

# ALEX debug flag
DEBUG_FLAGS += -DLXL_DEBUG=0 -DALEX_TRACE_PC=0 -DALEX_SANITY_DEBUG=0 -DALEX_DEBUG=0 -DALEX_ACCURATE_DEBUG=0
# Print heart beat every 1m instructions
# DEBUG_FLAGS += -DCHECK_HEART_BEAT
#DEBUG_FLAGS += -DMEASURE_FP
# DEBUG_FLAGS += -DCHECK_TRAP_RATE
DEBUG_FLAGS += -DLXL_FIND_BITFLIP

# Flag to build structures to track fault propagation through memory
#DEBUG_FLAGS += -DTRACK_MEM_SHARING=0

# Flags to indicate which OS we are using
DEBUG_FLAGS += -DOPEN_SOL=1
DEBUG_FLAGS += -DSOL9=0

#this must be on if you are running multiple cores
#DEBUG_FLAGS += -DMULTI_CORE

# Turn on hang detector
DEBUG_FLAGS += -DDETECT_INF_LOOP

# Flag to trun on the high os detector
DEBUG_FLAGS += -DUSE_HIGH_OS

# Flag for the app abort detector. Looks for idle loop
DEBUG_FLAGS += -DUSE_APP_ABORT

# Flags for server workloads
# DEBUG_FLAGS += -DBINARY_32_BIT

# Flags for SPEC workloads
# DEBUG_FLAGS += -DUSE_APP_ABORT -DUSE_HIGH_OS

DEBUG_FLAGS += -DGSPOT_DEBUG=0
DEBUG_FLAGS += -DFAULT_CORRUPT_SIMICS_DEBUG=0
DEBUG_FLAGS += -DPRADEEP_DEBUG_CLEAR=0
DEBUG_FLAGS += -DPRADEEP_DEBUG_INJ=0
DEBUG_FLAGS += -DPRADEEP_DEBUG_TRANSIENT=1
DEBUG_FLAGS += -DPRADEEP_DEBUG_AGEN=0
DEBUG_FLAGS += -DPRADEEP_DEBUG_LSQ=0
DEBUG_FLAGS += -DPRADEEP_LSQ_BADADDR=0
DEBUG_FLAGS += -DALEX_RAT_DEBUG=0
#DEBUG_FLAGS += -DLXL_CTRL_MAP	#Only use when you are tracing, fixed a bug of ORIGINAL GEMS opal tracer/tester
DEBUG_FLAGS += -DSIVA_CORRUPTION_DEBUG=0
DEBUG_FLAGS += -DSIVA_LOAD_BUFFER_DEBUG=0 

# verifying replay
#DEBUG_FLAGS += -DLXL_MEM_REC_VERIFIER

#DEBUG_FLAGS += -DALEX_FAULT_RET_OUT
#DEBUG_FLAGS += -DALEX_FAULT_RET_DEBUG
DEBUG_FLAGS += -DSIVA_DEBUG=0
DEBUG_FLAGS += -DSIVA_CORRUPTION_DEBUG=0
#DEBUG_FLAGS += -DLXL_WAKEDEP_DEBUG
DEBUG_FLAGS += -DNO_FAULT_LOG=1

DEBUG_FLAGS += -DPRADEEP_DEBUG_LL=0

# LXL: define LXL_SNET_RECOVERY if want rollback/replay, set DEBUG flag 1 
# if want debug messages
DEBUG_FLAGS += -DALEX_SNET_DEBUG=0
#DEBUG_FLAGS += -DLXL_SNET_RECOVERY # check ROLLBACK_ON_SYMPTOM in system/pseq.C

# Flags for checking the probabilistic models
DEBUG_FLAGS += -DLXL_COLLECT_PATTERN=0
DEBUG_FLAGS += -DLXL_PROB_FM=0
DEBUG_FLAGS += -DLXL_PROB_DIR_FM=0

# LXL: set USE_TBFD to 1 if want diagnosis, NO_CORRUPT flag 0 means "corrupting" 
# execution, no support for "patching" execution (buggy anyway). CHECK_RESULT 
# means marking mismatches based on instruction results (instead of full arch state).
# Again, setting this flag to 0 is not support, likely buggy.
# CHK_SYMPTOM flag is defined if want to do high level diagnosis, i.e., symptom, rollback, 
# symptom.  
DEBUG_FLAGS += -DUSE_TBFD=0
DEBUG_FLAGS += -DLXL_NO_CORRUPT=0 
DEBUG_FLAGS += -DLXL_CHECK_RESULT_ONLY=1

DEBUG_FLAGS += -DCHK_SYMPTOM

#DEBUG_FLAGS += -DLLB_ANALYSIS # for understanding the compression due to directory 
#DEBUG_FLAGS += -DLLB_IMPLEMENTATION_2 # see system/diagnosis.C for details
DEBUG_FLAGS += -DLLB_IMPLEMENTATION_4 # see system/diagnosis.C for details
DEBUG_FLAGS += -DMULTICORE_DIAGNOSIS_VERBOSE=0
DEBUG_FLAGS += -DCONCISE_VERBOSE=0
DEBUG_FLAGS += -DEXECUTE_STORES
#DEBUG_FLAGS += -DREPLAY_PHASE
# If you turn on MULTICORE_DIAGNOSIS_DEBUG then turn TRACK_MEM_SHARING and TBFD off, turn on LXL_SNET_RECOVERY
DEBUG_FLAGS += -DMULTICORE_DIAGNOSIS_ONLY=0 
#DEBUG_FLAGS += -DSESE_POT

# Execute a total of the specified number of instructions, and not
# 10m after arch-mismatch
# DEBUG_FLAGS += -DFIXED_INSTRUCTIONS

#	flag to print out shorter version of the retiring trace
DEBUG_FLAGS += -DRETIRE_TRACE=0
# MICROARCH_TRACE should be turned on if you want to print microarchitecture level trace (with physical reg numbers)
#DEBUG_FLAGS += -DMICROARCH_TRACE
#earlier called PRADEEP_RETIRE_TRACE
DEBUG_FLAGS += -DFAULTY_RETIRE_TRACE=0

# Turn on for trace of retirnign mem instructions to debug chkpting from amber
# DEBUG_FLAGS += -DDEBUG_AMBER_CHKPT

# Print only the targets of ctrl_instrucitons
# DEBUG_FLAGS += -DCTRL_TARGET

# This flag controls what marked printed in the retire trace.
# If 1, then the trace prints fault prop through mem, else it
# prints info without fault propagation through memory
DEBUG_FLAGS += -DMEM_PROPAGATION=0

# Print a shorter version of the retiring trace, with only flat reg info and values
# Assembly prints the assembly
# Short trace prints only flat regs and values, without fault propagation bit
# SLICE_TRACE gives the trace like a slice, to be used by gen_slice.pl script
DEBUG_FLAGS += -DSHORT_TRACE=0 -DNO_VALUES=0 -DASSEMBLY_TRACE=0 -DSLICE_TRACE=0
# touch system/dynamic.C for this
# DEBUG_FLAGS += -DBUILD_SLICE -DDEBUG_SLICE=0 -DVERBOSE=0 # -DBUILD_DDG # Works only if BUILD_SLICE is already turned on
# DEBUG_FLAGS += -DFIND_UNIQUE_PCS

# If you want to make sure that the selected instruction's destination is injected with a fault
# XXX Works only with ARCH_TRANS_FAULT on. Just the way I implemented it
# DEBUG_FLAGS += -DSET_REG_DEST_REG

# Flags to generate and check data value invariants. Look at common/debugio.C for functions.
# DEBUG_FLAGS += -DGEN_DATA_VALUE_INV -DPRINT_DATA_VALUES=0 -DDEBUG_GEN_DATA_VALUE_INV=0 
# DEBUG_FLAGS += -DCHECK_DATA_VALUE_INV -DDEBUG_CHECK_DATA_VALUE_INV=0
# DEBUG_FLAGS += -DCHECK_DATA_ONLY_VALUES

# Pradeep's Flag for SDC tracking
# DEBUG_FLAGS += -DRETIRE_TRACE=1 -DFIXED_INSTRUCTIONS -DDEBUG_FILTER -DMEM_PROPAGATION=1
# touch system/dynamic.C system/pseq.C system/system.C common/debugio.C and recompile to activate flag
# SHORT_TRACE Print a shorter version of the retiring trace, with only flat reg info and without prop bit
# ASSEMBLY_TRACE prints the assembly
# SLICE_TRACE gives the trace like a slice, to be used by gen_slice.pl script
DEBUG_FLAGS += -DSHORT_TRACE=0 -DNO_VALUES=0 -DASSEMBLY_TRACE=0 -DSLICE_TRACE=0 -DMEM_ONLY_TRACE=0
DEBUG_FLAGS += -DINJECTED_INST=0
DEBUG_FLAGS += -DDEBUG_PREFETCH=0
DEBUG_FLAGS += -DRECORD_FAULTY_INST=0

DEBUG_FLAGS += -DINST_MIX=0

# Flag for checking ranges of loads and stores
# DEBUG_FLAGS += -DCOLLECT_RANGE # touch common/debugio.C
# DEBUG_FLAGS += -DRANGE_CHECK # -DSTACK_ONLY # touch common/debugio.C system/memop.C system/system.C system/pseq.C
# DEBUG_FLAGS += -DFIND_MIN_STACK_ADDR
DEBUG_FLAGS += -DHARD_BOUND_WARMUP=0 -DWARMUP_INST=0

# Don't inject for first so many instructions, and then try to inject fault for 1m after that
DEBUG_FLAGS += -DFAULT_START_INST=0
# DEBUG_FLAGS += -DFAULT_START_INST=10000000

DEBUG_FLAGS += -DDEBUG_HARD_BOUND=0
# DEBUG_FLAGS += -DHARD_BOUND_BASE # - provide bounds ~/pramach2/research/GEMS/simics/home/spec_opensolaris/all_object_table/
# DEBUG_FLAGS += -DHARD_BOUND -DHARD_BOUND_WARMUP=1 -DWARMUP_INST=10000000
# DEBUG_FLAGS += -DDEBUG_HARD_BOUND=1 -DDEBUG_FILTER -DRETIRE_TRACE=1
# touch common/debugio.C system/memop.C system/dynamic.C  system/pseq.C system/system.C system/controlop.C

# FLags for building object tables
# DEBUG_FLAGS += -DBUILD_OBJECT_TABLE

# FLag to turn on round-robin use of the ALU, Register dbus, and agen.
# TODO - Implement this for other structures.
# DEBUG_FLAGS += -DRESOURCE_RR_ALU
# DEBUG_FLAGS += -DRESOURCE_RR_REGBUS
# DEBUG_FLAGS += -DRESOURCE_RR_AGEN
# touch system/dynamic.* system/pseq.* system/scheduler.* for files of this flag

############################################################
## TRANSIENT FAULTS
############################################################

# If this flag is set, bit to inject transient fault is not randomly selected
# Fault is injected into bit specified by GET_FAULT_BIT
DEBUG_FLAGS += -DFIXED_TRANSIENT=1 
# Flag to stop simulation if the transient fault didn't result in arch state
# mismatch TRANSIENT_MASK_THRESHOLD instructions after activation
DEBUG_FLAGS += -DTRANSIENT_MASK=1
DEBUG_FLAGS += -DTRANSIENT_MASK_THRESHOLD=100000

DEBUG_FLAGS += -DARCH_TRANS_FAULT -DFIXED_INSTRUCTIONS # Instruction-based transient fault. 
DEBUG_FLAGS += -DARCH_REGISTER_INJECTION # This flag is used to inject a fault in the arch register 
# it uses REG_FAULT_TYPE with register number from 0-32 not 0-64
# for integer registers it directly injects in physical reg file
# for fp registers TBD



# Flag for detecting invariant violation
# DEBUG_FLAGS += -DDETECT_INV_VIOLATION

#	My standard "functional correctness" debugging package
#DEBUG_FLAGS  += -DDEBUG_FUNCTIONALITY

#	defines for startup debugging
#DEBUG_FLAGS += -DMODINIT_VERBOSE
#	some run-time checks to validate simics envr is what we expect
DEBUG_FLAGS += -DVERIFY_SIMICS
#	PIPELINE_VIS: define for pipeline vis stuff
#DEBUG_FLAGS += -DPIPELINE_VIS

#	DEBUG_RETIRE for more verbose retirement
# DEBUG_FLAGS += -DDEBUG_RETIRE
#	defines debugging of the LSQ
# DEBUG_FLAGS += -DDEBUG_LSQ
#	Enables enables a debug filtering mechanism based on cycle, per proc
# DEBUG_FLAGS += -DDEBUG_FILTER
#	defines a lot of verbosity in pseq
#DEBUG_FLAGS += -DDEBUG_PSEQ
#	defines debugging of the ruby interface
#DEBUG_FLAGS += -DDEBUG_RUBY
#	defines a lot of verbosity in the cache
#DEBUG_FLAGS += -DDEBUG_CACHE
#	DEBUG_TESTER: prints out differences between simics steps
#	              may define for tester, not for other modules
#DEBUG_FLAGS += -DDEBUG_TESTER
#   DEBUG_CC to debug condition codes
#DEBUG_FLAGS += -DDEBUG_CC

#	Define to debug ideal (inorder) processor modes
#DEBUG_FLAGS += -DDEBUG_IDEAL

#	FAKE_RUBY allows opal to imitate running w/ruby, stand-alone or in simics.
#DEBUG_FLAGS += -DFAKE_RUBY

#	defines for "zero tolerance" retirement checking: if a single register
#	doesn't match -- reload the entire state && squash the pipeline
DEBUG_FLAGS += -DRETIRE_ZERO_TOLERANCE

#	define to decode static instructions on a per-dynamic instruction basis
#	for higher performance, leave this commented out
DEBUG_FLAGS += -DREDECODE_EACH

#	define to cache instruction TLB translations on a per processor basis
#	for higher performance, uncomment this define
#DEBUG_FLAGS += -DUSE_MINI_ITLB

#	define to create statistics structures that are expensive in runtime&space
#DEBUG_FLAGS += -DEXPENSIVE_STATS

#	define to track the number of renames per logical register
#DEBUG_FLAGS += -DRENAME_EXPERIMENT

#	define run-time register file leak checking. Computationally expensive!
#DEBUG_FLAGS += -DCHECK_REGFILE

#	EXPENSIVE_ASSERTIONS mainly cover verifying the LSQ, scheduler lists, etc
#DEBUG_FLAGS += -DEXPENSIVE_ASSERTIONS

#	defines for static instruction window profiling
#DEBUG_FLAGS += -DSTATICI_PROFILE

#   MEMTRACE_DEBUG for debugging memory trace statements
#DEBUG_FLAGS += -DMEMTRACE_DEBUG

#	defines locations in the code where HOST endianess matters for the
#	correctness of this program.
#ENDIAN_MATTERS

#	USE_FSR     defines for using the FSR (linux vs solaris)
#	USE_DIRENT  defines for different directory scanning (linux vs solaris)
#	SWAP_ENDIAN defines for endianness (explicit currently)
#	INSTRUCTION_OVERWRITE tags selected debugging of i-cache changes

ifeq ($(SIMICS_VERSION),2.0.X)
  INCLUDES  = -I$(SIMICS_ROOT)/$(HOST_TYPE)/obj/include/
  INCLUDES += -I$(SIMICS_ROOT)/$(HOST_TYPE)/obj/include/simics
  INCLUDES += -I$(SIMICS_ROOT)/$(HOST_TYPE)/obj/include/simics-api-1
  INCLUDES += -I$(SIMICS_ROOT)/$(HOST_TYPE)/config
  INCLUDES += -Ibenchmark/tester
else
ifeq ($(SIMICS_VERSION),2.2.X)
  INCLUDES  = -I$(SIMICS_ROOT)/$(HOST_TYPE)/obj/include/
  INCLUDES += -I$(SIMICS_ROOT)/$(HOST_TYPE)/obj/include/simics
  INCLUDES += -I$(SIMICS_ROOT)/$(HOST_TYPE)/obj/include/simics-api-1
  INCLUDES += -I$(SIMICS_ROOT)/$(HOST_TYPE)/config
  INCLUDES += -Ibenchmark/tester
else
ifeq ($(SIMICS_VERSION),3.0)
  INCLUDES  = -I$(SIMICS_INCLUDE_ROOT)
  INCLUDES += -I$(SIMICS_INCLUDE_ROOT)/simics
  INCLUDES += -I$(SIMICS_INCLUDE_ROOT)/simics/core
  INCLUDES += -I$(SIMICS_INCLUDE_ROOT)/simics/arch
  INCLUDES += -I$(SIMICS_INCLUDE_ROOT)/simics/util
else
  # Very bad - this should halt compilation with an error
  INCLUDE += UNKNOWN_SIMICS_VERSION
endif
endif
endif

# Flags for some checkers
# DEBUG_FLAGS += -DRENAME_CHECKER -DDEBUG_RENAME_CHECKER -DVAR_TRACE 
# DEBUG_FLAGS += -DFP_CHECKER # -DVAR_TRACE # -DDEBUG_FP_CHECKER 
# DEBUG_FLAGS += -DDECODER_CHECKER # -DVAR_TRACE # -DDEBUG_DECODER_CHECKER 
# DEBUG_FLAGS += -DFETCH_CHECKER # -DVAR_TRACE # -DDEBUG_FETCH_CHECKER 

############################## GATE LEVEL SAF SIM ##################

# Don't perform any low level simulations. On by default
# DEBUG_FLAGS += -DHIGH_LEVEL_ONLY

# Flag to turn on low-level printing
# DEBUG_FLAGS += -DPRINT_LL_INFO

# Flag to turn on the Low-level simulation for Decoder
# DEBUG_FLAGS += -DLL_DECODER
DEBUG_FLAGS += -DDEBUG_LL_DECODER=0

# Flag to turn on the Low-level simulation for int_alu
#DEBUG_FLAGS += -DLL_INT_ALU
# DEBUG_FLAGS += -DLL_INT_ALU_COMPARE

#DEBUG_FLAGS += -DLL_AGEN
DEBUG_FLAGS += -DDEBUG_LL_AGEN=0

# Separate fault log type for low level simulator. Always on when doing low level simulations
#DEBUG_FLAGS += -DSAF_FAULT_LOG

# Only collect activation statistics, but don't corrupt high level simulation
# DEBUG_FLAGS += -DACT_STATS

# Compare results with low-level, but do fault injections in the high-level only!
# DEBUG_FLAGS += -DCOMPARE_TO_LL

# Profile the golden runs?
# DEBUG_FLAGS += -DPROFILE

# DEBUG_FLAGS += -DOMIT_NCVLOG # Do low-level simulations with in-house fault simulator
# DEBUG_FLAGS += -DDEBUG_FSIM # Debug flag for custom fault simulator
#
# Compare fault injection results of low-level and high-level simulations
#DEBUG_FLAGS += -DCOMPARE
#
# In summary:
# [SET = comment out]
# low level custom simulation: saf_fault_log and omit_ncvlog 
# high level sim involving just alu instructions: comment out high_level_only and compare_to_ll
# low level simulations (verilog) excluding fault injection (profiling): 
#	 high_level_only, and profile
# basic fault injection simulations with ncvlog -> saf_fault_log 
#################################### GATE LEVEL SAF SIM ################################################

# CM extra: must define TARGET_SPARC_V9 to use sparc_api.h
SPECIAL_FLAGS += -DTARGET_SPARC_V9
# CM extra: must define TARGET_ULTRA to successfully use sparc_api.h include...
SPECIAL_FLAGS += -DTARGET_ULTRA -DTARGET_VA_BITS=64 -DTARGET_PA_BITS=64
SPECIAL_FLAGS += -DYY_NO_UNPUT

# A pointer to the location of the opal.o object created by simics's makefile
OPAL_MODULE_OBJ = ../simics/$(HOST_TYPE)/obj/modules/opal/opal.o

#
# SOURCE
#

SRC_CPP := \
	initvar.C \
	symtrace.C \
	chain.C \
	sstat.C \
	ptrace.C \
	pseq.C \
	system.C \
	dx.C \
	ix.C \
	exec.C \
	opcode.C \
	arf.C \
	actor.C \
	agree.C \
	bitlib.C \
	bitfield.C \
	bitdist.C \
	branchfile.C \
	cache.C \
	confio.C \
	controlop.C \
	decode.C \
	debugio.C \
	dependence.C \
	dtlb.C \
	dynamic.C \
	fatpredict.C \
	fileio.C \
	flow.C \
	flatarf.C \
	gshare.C \
	igshare.C \
	hfa.C \
	histogram.C \
	iwindow.C \
	ipage.C \
	ipagemap.C \
	indirect.C \
	listalloc.C \
	lockstat.C \
	lsq.C \
	mlpredict.C \
	memop.C \
	memstat.C \
	memtrace.C \
	mshr.C \
	pipepool.C \
	pipestate.C \
	pstate.C \
	ras.C \
	regbox.C \
	regfile.C \
	regmap.C \
	rubycache.C \
	scheduler.C \
	statici.C \
	sysstat.C \
	stopwatch.C \
	simdist12.C \
	threadstat.C \
	tlstack.C \
	tracefile.C \
	transaction.C \
	utimer.C \
	wait.C \
	yags.C \
	interface.C\
	structuralModule.C\
	faultSimulate.C \
	diagnosis.C  \
	ddg.C  \
	crc16.C 

SRC_C	 := \
	ccops.c \
	attrparse.c \
	attrlex.c

#
# BUILD RULES
#

# The default rule is make the opal module
default_rule: module

all: tester usd readipage makeipage regtest memscan module

module: 
	@echo "Opal: building module"
	${MAKE} $(GENERATED_DIR)/generated
	${MAKE} BUILD_FLAGS=-DSIMICS module_go
	@echo "Opal: Linking simics module"
ifeq ($(SIMICS_VERSION),2.0.X)
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) opal
else
ifeq ($(SIMICS_VERSION),2.2.X)
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) opal
else
ifeq ($(SIMICS_VERSION),3.0)
	cd $(SIMICS_EXEC_ROOT); $(MAKE) opal
else
	@echo "Unrecognized value for SIMICS_VERSION: see common/Makefile.simics_version"
	exit 1
endif
endif
endif
ifdef DESTINATION
	@echo "Opal: Moving module"
	$(MAKE) BUILD_FLAGS=-DSIMICS movemodule DESTINATION=$(DESTINATION)
endif
	echo "Finished."

# NOTE:  In order to build the tester, modify system/simdist12.C so that mm_* stub functions compile
tester:
	@echo "Opal: Building tester.exec"
	$(MAKE) $(BIN_DIR)/tester.exec

# NOTE:  In order to build the tester, modify system/simdist12.C so that mm_* stub functions compile
unitest:
	@echo "Opal: Building unitest.exec"
	@echo "Opal: Building ruby"
	cd $(GEMS_ROOT)/ruby; ${MAKE} SPECIAL_FLAGS=-DLIBRUBYTEST PROTOCOL=$(PROTOCOL) librubytest
	@echo "Opal: Building tester.exec"
	${MAKE} $(GENERATED_DIR)/generated
ifeq ($(PURIFY),)
	@echo "Opal: building simics objects"
ifeq ($(SIMICS_VERSION),2.0.X)
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) opal
else
ifeq ($(SIMICS_VERSION),2.2.X)
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) opal
else
ifeq ($(SIMICS_VERSION),3.0)
	cd $(SIMICS_EXEC_ROOT); $(MAKE) opal
else
	@echo "Unrecognized value for SIMICS_VERSION: see commmon/Makefile.common"
	exit 1
endif
endif
endif
endif
	@echo "Opal: linking ruby and opal"
	$(MAKE) BUILD_FLAGS=-DUNITESTER $(BIN_DIR)/unitest.exec

usd:	    $(BIN_DIR)/usd.exec
	@echo "Opal: built usd"
makeipage:  $(BIN_DIR)/makeipage.exec
	@echo "Opal: built makeipage"
bp:	     $(BIN_DIR)/bp.exec
	@echo "Opal: built bp"
readipage:  $(BIN_DIR)/readipage.exec
	@echo "Opal: built readipage"
readtrace:  $(BIN_DIR)/readtrace.exec
	@echo "Opal: built readtrace"
pipetest: $(BIN_DIR)/pipetest.exec
	@echo "Opal: built pipetest"
regtest:	$(BIN_DIR)/regtest.exec
	@echo "Opal: built regtest"
memscan:	$(BIN_DIR)/memscan.exec
	@echo "Opal: built memscan"
conftest:   $(BIN_DIR)/conftest.exec
	@echo "Opal: built conftest"
rhtest:	 $(BIN_DIR)/rhtest.exec
	@echo "Opal: built rhtest"
ptracetest:   $(BIN_DIR)/ptracetest.exec
	@echo "Opal: built ptrace test module"

module_go: $(GENERATED_DIR)/generated $(OBJ)
	@echo "Made module"

$(GENERATED_DIR)/generated: $(GENERATED_DIR)/created attrparse.y attrlex.l $(GENERATED_DIR)/default_param.h
	@echo "Generating config parser"
	$(YACC) $(YFLAGS) -p at -o $(GENERATED_DIR)/attrparse.c ../common/ioutil/attrparse.y
	$(LEX) $(LFLAGS) -Pat -o$(GENERATED_DIR)/attrlex.c ../common/ioutil/attrlex.l
	touch $(GENERATED_DIR)/generated

$(GENERATED_DIR)/default_param.h: ../common/ioutil/embedtext.py config/config.defaults
	python ../common/ioutil/embedtext.py config/config.defaults $(GENERATED_DIR)/default_param.h global_default_param

movemodule:
ifeq ($(SIMICS_VERSION),2.0.X)
	$(GEMS_ROOT)/scripts/prepare_simics_home.sh $(SIMICS_ROOT)/home/$(DESTINATION) $(HOST_TYPE)
	$(MV) $(SIMICS_ROOT)/$(HOST_TYPE)/lib/opal.so $(SIMICS_ROOT)/home/$(DESTINATION)/modules/opal-v9.so
	$(MV) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_opal_commands.py $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python
else
ifeq ($(SIMICS_VERSION),2.2.X)
	$(GEMS_ROOT)/scripts/prepare_simics_home.sh $(SIMICS_ROOT)/home/$(DESTINATION) $(HOST_TYPE)
	$(MV) $(SIMICS_ROOT)/$(HOST_TYPE)/lib/opal.so $(SIMICS_ROOT)/home/$(DESTINATION)/modules/opal-v9.so
	$(MV) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_opal_commands.py $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python
else
ifeq ($(SIMICS_VERSION),3.0)
	$(GEMS_ROOT)/scripts/prepare_simics_home.sh $(SIMICS_EXEC_ROOT)/home/$(DESTINATION) $(HOST_TYPE)
	$(MV) $(SIMICS_EXEC_ROOT)/$(HOST_TYPE)/lib/opal.so $(SIMICS_EXEC_ROOT)/home/$(DESTINATION)/modules/opal-v9.so
	$(MV) -f $(SIMICS_EXEC_ROOT)/$(HOST_TYPE)/lib/python/mod_opal_commands.py $(SIMICS_EXEC_ROOT)/home/$(DESTINATION)/modules/python
else
	@echo "Unrecognized value for SIMICS_VERSION: see commmon/Makefile.simics_version"
	exit 1
endif
endif
endif

removemodule:
ifeq ($(SIMICS_VERSION),2.0.X)
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/opal-v9.so
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python/mod_opal_commands.py
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python/mod_opal_commands.pyc
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/opal-v9.so
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_opal_commands.py
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_opal_commands.pyc
else
ifeq ($(SIMICS_VERSION),2.2.X)
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/opal-v9.so
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python/mod_opal_commands.py
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python/mod_opal_commands.pyc
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/opal-v9.so
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_opal_commands.py
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_opal_commands.pyc
else
	@echo "removemodule target not supported for this SIMICS_VERSION"
	exit 1
endif
endif

# -- Generate Tester
$(BIN_DIR)/tester.exec: $(OBJ) $(GENERATED_DIR)/generated $(BIN_DIR)/created $(OBJ_DIR)/simmain.o
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/simmain.o $(OBJ) $(TESTER_LDFLAGS)
	@echo "Made tester"

# -- Generate Ruby-Opal Tester
$(BIN_DIR)/unitest.exec: $(OBJ) $(GENERATED_DIR)/generated $(BIN_DIR)/created $(OBJ_DIR)/simmain.o $(RUBY_DIR)/bin/librubytest.a
	$(PURIFY) $(CC) -o $@ $(CFLAGS) $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/simmain.o $(OBJ) -L$(RUBY_DIR)/bin/ -lrubytest $(TESTER_LDFLAGS)
	@echo "Made unitester"

# -- Generate the Ipage map constructor
$(BIN_DIR)/makeipage.exec: $(OBJ) $(GENERATED_DIR)/generated tester/makeipage.C $(OBJ_DIR)/makeipage.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/makeipage.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate the Ipage map tester
$(BIN_DIR)/readipage.exec: $(OBJ) $(GENERATED_DIR)/generated tester/readipage.C $(OBJ_DIR)/readipage.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/readipage.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate the trace reader
$(BIN_DIR)/readtrace.exec: $(OBJ) $(GENERATED_DIR)/generated tester/readtrace.C $(OBJ_DIR)/readtrace.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/readtrace.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate the UltraSparc Decoder
$(BIN_DIR)/usd.exec: $(OBJ) $(GENERATED_DIR)/generated tester/usd.C $(OBJ_DIR)/usd.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/usd.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate the branch predictor executable
$(BIN_DIR)/bp.exec: $(OBJ) $(GENERATED_DIR)/generated tester/bp.C $(OBJ_DIR)/bp.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/bp.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate the pipeline tester
$(BIN_DIR)/pipetest.exec: $(OBJ) $(GENERATED_DIR)/generated tester/pipetest.C $(OBJ_DIR)/pipetest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/pipetest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate a register file tester
$(BIN_DIR)/regtest.exec: $(OBJ) $(GENERATED_DIR)/generated tester/regtest.C $(OBJ_DIR)/regtest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/regtest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate a memory scan tester
$(BIN_DIR)/memscan.exec: $(OBJ) $(GENERATED_DIR)/generated tester/memscan.C $(OBJ_DIR)/memscan.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/memscan.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate a configuration file tester
$(BIN_DIR)/conftest.exec: $(OBJ_DIR) $(ALLOBJ) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/conftest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	@echo "Making Opal module"
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/conftest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate a ptrace file reader
$(BIN_DIR)/ptracetest.exec: $(OBJ_DIR) $(ALLOBJ) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/ptracetest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	@echo "Making Opal module"
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/ptracetest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate run-ahead tester
$(BIN_DIR)/rhtest.exec: $(OBJ) $(GENERATED_DIR)/generated tester/rhtest.C $(OBJ_DIR)/rhtest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/rhtest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Make a tar file of all the appropriate source files
tar:
	@echo "Building tar file"
	tar czvf opal.tgz Makefile common/*[Ch] simics/Makefile simics/*.[Chi] simics/*.py system/*.[Ccsh] simics/*.def system/*.def trace/*.[Ch] tester/*.[Ch] spitfire-mmu/*.[ch] where

doc:
# -d specifies subdirectory to write files into
# -g specifies to not generate GIFS
# -p specifies to document private member variables
	doc++ -g -p -d docs doc++.items

clean: 
ifeq ($(SIMICS_VERSION),2.0.X)
	/bin/rm -rf $(HOST_TYPE) $(GENERATED_DIR) ../simics/$(HOST_TYPE)/obj/extensions/opal
	$(RM) -rf $(SIMICS_ROOT)/home/$(DESTINATION)/modules/modcap.buf
else
ifeq ($(SIMICS_VERSION),2.2.X)
	/bin/rm -rf $(HOST_TYPE) $(GENERATED_DIR) ../simics/$(HOST_TYPE)/obj/extensions/opal
	$(RM) -rf $(SIMICS_ROOT)/home/$(DESTINATION)/modules/modcap.buf
else
ifeq ($(SIMICS_VERSION),3.0)
	$(RM) -rf $(HOST_TYPE) $(GENERATED_DIR)
else
	@echo "Unrecognized value for SIMICS_VERSION: see commmon/Makefile.simics_version"
	exit 1
endif
endif
endif

modclean:
ifeq ($(SIMICS_VERSION),2.0.X)
	/bin/rm -rf $(SIMICS_ROOT)/$(HOST_TYPE)/obj/modules/opal
else
ifeq ($(SIMICS_VERSION),2.2.X)
	/bin/rm -rf $(SIMICS_ROOT)/$(HOST_TYPE)/obj/modules/opal
else
ifeq ($(SIMICS_VERSION),3.0)
	$(RM) -rf $(SIMICS_EXEC_ROOT)/$(HOST_TYPE)/lib/opal*
else
	@echo "Unrecognized value for SIMICS_VERSION: see commmon/Makefile.simics_version"
	exit 1
endif
endif
endif

# rebuild for PIPELINE_VIS without doing a make clean
vis:
	/bin/rm $(OBJ_DIR)/controlop.o $(OBJ_DIR)/dynamic.o $(OBJ_DIR)/memop.o $(OBJ_DIR)/pseq.o $(OBJ_DIR)/debugio.o $(OBJ_DIR)/lsq.o

etags: 
	etags common/*.[Ch] system/*.[Ch] trace/*.[Ch] tester/*.[Ch]

include $(GEMS_ROOT)/common/Makefile.dep

