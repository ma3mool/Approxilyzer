SRC_DIR = .
OBJ_DIR = ../obj

all: obj_dir cpp-front-end 

obj_dir:
	mkdir -p ../obj

OPTS = -g 

CC = g++

FLAGS = -DMICRO_ARCH_FAULTS=0 -DCONSTANT_OP_FAULTS=0 -DUNOPT_APPS=0
FLAGS += -DSESE=1	# Read GangES paper (ISCA 14) for more details
					# 0 - do not create SESE regions
					# 1 - create SESE regions, it can be slow for several apps
					#
FLAGS += -DCFG_DEBUG=1	 # prints dot files for CFGs of every function in the program

###############################################################
#CPP Front end
###############################################################


cpp-front-end: $(OBJ_DIR)/main.o $(OBJ_DIR)/instruction.o $(OBJ_DIR)/unit_fault.o $(OBJ_DIR)/program.o $(OBJ_DIR)/bb.o $(OBJ_DIR)/cfg.o
	$(CC) $(OPTS) $(FLAGS) $(OBJ_DIR)/*.o -o relyzer-front-end
	
$(OBJ_DIR)/program.o: $(SRC_DIR)/program.cpp $(SRC_DIR)/program.h $(SRC_DIR)/unit_fault.h $(SRC_DIR)/instruction.h $(SRC_DIR)/common_functions.h $(SRC_DIR)/bb.h $(SRC_DIR)/cfg.h
	$(CC) $(OPTS) $(FLAGS) -c $(SRC_DIR)/program.cpp -o $(OBJ_DIR)/program.o

$(OBJ_DIR)/instruction.o: $(SRC_DIR)/instruction.cpp $(SRC_DIR)/instruction.h $(SRC_DIR)/unit_fault.h $(SRC_DIR)/common_functions.h $(SRC_DIR)/bb.h
	$(CC) $(OPTS) $(FLAGS) -c $(SRC_DIR)/instruction.cpp -o $(OBJ_DIR)/instruction.o

$(OBJ_DIR)/unit_fault.o: $(SRC_DIR)/unit_fault.cpp $(SRC_DIR)/unit_fault.h
	$(CC) $(OPTS) $(FLAGS) -c $(SRC_DIR)/unit_fault.cpp -o $(OBJ_DIR)/unit_fault.o

$(OBJ_DIR)/main.o: $(SRC_DIR)/program.h $(SRC_DIR)/unit_fault.h $(SRC_DIR)/main.cpp
	$(CC) $(OPTS) $(FLAGS) -c $(SRC_DIR)/main.cpp -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/bb.o: $(SRC_DIR)/program.h $(SRC_DIR)/unit_fault.h  $(SRC_DIR)/cfg.h  $(SRC_DIR)/bb.cpp
	$(CC) $(OPTS) $(FLAGS) -c $(SRC_DIR)/bb.cpp -o $(OBJ_DIR)/bb.o

$(OBJ_DIR)/cfg.o: $(SRC_DIR)/program.h $(SRC_DIR)/unit_fault.h $(SRC_DIR)/bb.h  $(SRC_DIR)/cfg.cpp 
	$(CC) $(OPTS) $(FLAGS) -c $(SRC_DIR)/cfg.cpp -o $(OBJ_DIR)/cfg.o



###############################################################
#cleaning
###############################################################


clean:
	rm -f relyzer-front-end $(OBJ_DIR)/*.o
