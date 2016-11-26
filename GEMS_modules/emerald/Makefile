#
# $Id: Makefile 1.125 06/01/31 21:35:22-06:00 lyen@opus.cs.wisc.edu $
#

.PHONY: all module tester clean tags

# GEMS_ROOT points to the base of the CVS tree
GEMS_ROOT := ..
GENERATED_DIR := generated
# GATE LEVEL SIM: edited
# VPATH := common:fetch:sparc:system:module:trace:tester:$(GEMS_ROOT)/common/ioutil:$(GENERATED_DIR)
VPATH := common:system:module:$(GEMS_ROOT)/common/ioutil:$(GENERATED_DIR)

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
#OPT_FLAGS := -g -xO3
#OPT_FLAGS := -xCC  -fast -xarch=v8plusa -xO4 -ftrap=%all,no%inexact -xs -xprefetch=yes

# Set if the workloads are server workloads and emerald has to plug into server_cpu0

OPT_FLAGS += -pg -g

# throw error if no phase passed in. if simple.

# Looks at PCs around each use while created def-use chain. Used in categorization step for creating mem equivalence classes. Not needed for error injection.

DEBUG_FLAGS = -DEMERALD					# need for both Phases

ifeq ($(PHASE),1)
  DEBUG_FLAGS += -DPROFILE_APP 			# only need for Phase 1
else
ifeq ($(PHASE),2)
  DEBUG_FLAGS += -DDEPENDENCE_ANALYSIS 	# need for Phase 2
  #DEBUG_FLAGS += -DPC_LIST_AFTER  		# OPTIONAL - takes longer
  #DEBUG_FLAGS += -DPC_LIST_BEFORE		# OPTIONAL - takes longer 
  #DEBUG_FLAGS += -DUSE_HASH  # optimization for PC_LIST Before and After
endif
endif


# Turn on prints for debug
# DEBUG_FLAGS += -DDEBUG_OUTPUT_BUFFER 
# DEBUG_FLAGS += -DDEBUG_INPUT_BUFFER 
# DEBUG_FLAGS += -DDEBUG_DEVICES
# DEBUG_FLAGS += -DDEBUG_DETAIL
# DEBUG_FLAGS += -DDEBUG_MEMORY
# DEBUG_FLAGS += -DCHECK_ALL_MEM_WRITES

# Void writes from device to memory
# DEBUG_FLAGS += -DVOID_DEVICE_MEM_WRITES

# DEBUG_FLAGS += -DIGNORE_CPU_DEVICE_WRITES # XXX - Make sure emerald_dev is not on!

# Print stats of buffered and recorded each chkpt
# DEBUG_FLAGS += -DPRINT_EACH_CHKPT

# if compiling to run standalone with ruby, specify a protocol here
PROTOCOL= MOSI_bcast_2level
RUBY_DIR=$(GEMS_ROOT)/ruby/$(PROTOCOL)/$(HOST_TYPE)/tester/

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

#################################### GATE LEVEL SAF SIM ################################################


# A pointer to the location of the emerald.o object created by simics's makefile
OPAL_MODULE_OBJ = ../simics/$(HOST_TYPE)/obj/modules/emerald/emerald.o

#
# SOURCE
#

SRC_CPP := \
	globals.C \
	init.C \
	opal_interface.C \
	system.C \
	def_use_chain.C \
	commands.C \

SRC_C	 := \

#
# BUILD RULES
#

# The default rule is make the emerald module
default_rule: module

all: tester usd readipage makeipage regtest memscan module

module:
	@echo "Amber: building module"
	${MAKE} BUILD_FLAGS=-DSIMICS module_go
	@echo "Amber: Linking simics module"
ifeq ($(SIMICS_VERSION),2.0.X)
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) emerald
else
ifeq ($(SIMICS_VERSION),2.2.X)
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) emerald
else
ifeq ($(SIMICS_VERSION),3.0)
	cd $(SIMICS_EXEC_ROOT); $(MAKE) emerald
else
	@echo "Unrecognized value for SIMICS_VERSION: see common/Makefile.simics_version"
	exit 1
endif
endif
endif
ifdef DESTINATION
	@echo "Amber: Moving module"
	$(MAKE) BUILD_FLAGS=-DSIMICS movemodule DESTINATION=$(DESTINATION)
endif
	echo "Finished."

# NOTE:  In order to build the tester, modify system/simdist12.C so that mm_* stub functions compile
tester:
	@echo "Amber: Building tester.exec"
	$(MAKE) $(BIN_DIR)/tester.exec

# NOTE:  In order to build the tester, modify system/simdist12.C so that mm_* stub functions compile
unitest:
	@echo "Amber: Building unitest.exec"
	@echo "Amber: Building ruby"
	cd $(GEMS_ROOT)/ruby; ${MAKE} SPECIAL_FLAGS=-DLIBRUBYTEST PROTOCOL=$(PROTOCOL) librubytest
	@echo "Amber: Building tester.exec"
	${MAKE} $(GENERATED_DIR)/generated
ifeq ($(PURIFY),)
	@echo "Amber: building simics objects"
ifeq ($(SIMICS_VERSION),2.0.X)
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) emerald
else
ifeq ($(SIMICS_VERSION),2.2.X)
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) emerald
else
ifeq ($(SIMICS_VERSION),3.0)
	cd $(SIMICS_EXEC_ROOT); $(MAKE) emerald
else
	@echo "Unrecognized value for SIMICS_VERSION: see commmon/Makefile.common"
	exit 1
endif
endif
endif
endif
	@echo "Amber: linking ruby and emerald"
	$(MAKE) BUILD_FLAGS=-DUNITESTER $(BIN_DIR)/unitest.exec

usd:		$(BIN_DIR)/usd.exec
	@echo "Amber: built usd"
makeipage:  $(BIN_DIR)/makeipage.exec
	@echo "Amber: built makeipage"
bp:		 $(BIN_DIR)/bp.exec
	@echo "Amber: built bp"
readipage:  $(BIN_DIR)/readipage.exec
	@echo "Amber: built readipage"
readtrace:  $(BIN_DIR)/readtrace.exec
	@echo "Amber: built readtrace"
pipetest: $(BIN_DIR)/pipetest.exec
	@echo "Amber: built pipetest"
regtest:	$(BIN_DIR)/regtest.exec
	@echo "Amber: built regtest"
memscan:	$(BIN_DIR)/memscan.exec
	@echo "Amber: built memscan"
conftest:   $(BIN_DIR)/conftest.exec
	@echo "Amber: built conftest"
rhtest:	 $(BIN_DIR)/rhtest.exec
	@echo "Amber: built rhtest"
ptracetest:   $(BIN_DIR)/ptracetest.exec
	@echo "Amber: built ptrace test module"

module_go: $(OBJ)
	@echo "Made module"

# $(GENERATED_DIR)/generated: $(GENERATED_DIR)/created attrparse.y attrlex.l $(GENERATED_DIR)/default_param.h
#	 @echo "Generating config parser"
#	 $(YACC) $(YFLAGS) -p at -o $(GENERATED_DIR)/attrparse.c ../common/ioutil/attrparse.y
#	 $(LEX) $(LFLAGS) -Pat -o$(GENERATED_DIR)/attrlex.c ../common/ioutil/attrlex.l
#	 touch $(GENERATED_DIR)/generated

# $(GENERATED_DIR)/default_param.h: ../common/ioutil/embedtext.py config/config.defaults
#	 python ../common/ioutil/embedtext.py config/config.defaults $(GENERATED_DIR)/default_param.h global_default_param

movemodule:
ifeq ($(SIMICS_VERSION),2.0.X)
	$(GEMS_ROOT)/scripts/prepare_simics_home.sh $(SIMICS_ROOT)/home/$(DESTINATION) $(HOST_TYPE)
	$(MV) $(SIMICS_ROOT)/$(HOST_TYPE)/lib/emerald.so $(SIMICS_ROOT)/home/$(DESTINATION)/modules/emerald-v9.so
	$(MV) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_emerald_commands.py $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python
else
ifeq ($(SIMICS_VERSION),2.2.X)
	$(GEMS_ROOT)/scripts/prepare_simics_home.sh $(SIMICS_ROOT)/home/$(DESTINATION) $(HOST_TYPE)
	$(MV) $(SIMICS_ROOT)/$(HOST_TYPE)/lib/emerald.so $(SIMICS_ROOT)/home/$(DESTINATION)/modules/emerald-v9.so
	$(MV) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_emerald_commands.py $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python
else
ifeq ($(SIMICS_VERSION),3.0)
	$(GEMS_ROOT)/scripts/prepare_simics_home.sh $(SIMICS_EXEC_ROOT)/home/$(DESTINATION) $(HOST_TYPE)
	$(MV) $(SIMICS_EXEC_ROOT)/$(HOST_TYPE)/lib/emerald.so $(SIMICS_EXEC_ROOT)/home/$(DESTINATION)/modules/emerald-v9.so
	$(MV) -f $(SIMICS_EXEC_ROOT)/$(HOST_TYPE)/lib/python/mod_emerald_commands.py $(SIMICS_EXEC_ROOT)/home/$(DESTINATION)/modules/python
else
	@echo "Unrecognized value for SIMICS_VERSION: see commmon/Makefile.simics_version"
	exit 1
endif
endif
endif

removemodule:
ifeq ($(SIMICS_VERSION),2.0.X)
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/emerald-v9.so
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python/mod_emerald_commands.py
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python/mod_emerald_commands.pyc
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/emerald-v9.so
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_emerald_commands.py
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_emerald_commands.pyc
else
ifeq ($(SIMICS_VERSION),2.2.X)
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/emerald-v9.so
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python/mod_emerald_commands.py
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python/mod_emerald_commands.pyc
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/emerald-v9.so
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_emerald_commands.py
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_emerald_commands.pyc
else
	@echo "removemodule target not supported for this SIMICS_VERSION"
	exit 1
endif
endif

# -- Generate Tester
$(BIN_DIR)/tester.exec: $(OBJ) $(GENERATED_DIR)/generated $(BIN_DIR)/created $(OBJ_DIR)/simmain.o
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/simmain.o $(OBJ) $(TESTER_LDFLAGS)
	@echo "Made tester"

# -- Generate Ruby-Amber Tester
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
	@echo "Making Amber module"
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/conftest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate a ptrace file reader
$(BIN_DIR)/ptracetest.exec: $(OBJ_DIR) $(ALLOBJ) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/ptracetest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	@echo "Making Amber module"
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/ptracetest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate run-ahead tester
$(BIN_DIR)/rhtest.exec: $(OBJ) $(GENERATED_DIR)/generated tester/rhtest.C $(OBJ_DIR)/rhtest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/rhtest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Make a tar file of all the appropriate source files
tar:
	@echo "Building tar file"
	tar czvf emerald.tgz Makefile common/*[Ch] simics/Makefile simics/*.[Chi] simics/*.py system/*.[Ccsh] simics/*.def system/*.def trace/*.[Ch] tester/*.[Ch] spitfire-mmu/*.[ch] where

doc:
# -d specifies subdirectory to write files into
# -g specifies to not generate GIFS
# -p specifies to document private member variables
	doc++ -g -p -d docs doc++.items

clean: 
ifeq ($(SIMICS_VERSION),2.0.X)
	/bin/rm -rf $(HOST_TYPE) $(GENERATED_DIR) ../simics/$(HOST_TYPE)/obj/extensions/emerald
	$(RM) -rf $(SIMICS_ROOT)/home/$(DESTINATION)/modules/modcap.buf
else
ifeq ($(SIMICS_VERSION),2.2.X)
	/bin/rm -rf $(HOST_TYPE) $(GENERATED_DIR) ../simics/$(HOST_TYPE)/obj/extensions/emerald
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
	/bin/rm -rf $(SIMICS_ROOT)/$(HOST_TYPE)/obj/modules/emerald
else
ifeq ($(SIMICS_VERSION),2.2.X)
	/bin/rm -rf $(SIMICS_ROOT)/$(HOST_TYPE)/obj/modules/emerald
else
ifeq ($(SIMICS_VERSION),3.0)
	$(RM) -rf $(SIMICS_EXEC_ROOT)/$(HOST_TYPE)/lib/emerald*
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

