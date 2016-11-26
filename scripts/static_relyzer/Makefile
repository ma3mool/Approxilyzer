SRC_DIR = src
OBJ_DIR = obj
PERL_DIR = scripts/perl
PERL = perl -w
APPS = apps
#APPS = apps/optimized/fully_optimized/
#APPS = apps/unopt/

all: perl-front-end

front-end: perl-front-end cpp-front-end

#hello: profile_hello hello-perl-front-end cpp-front-end profile_hello 
#prime: profile_prime prime-perl-front-end cpp-front-end profile_prime 

###############################################################
#CPP Front end
###############################################################


cpp-front-end: $(OBJ_DIR)/main.o $(OBJ_DIR)/instruction.o $(OBJ_DIR)/unit_fault.o $(OBJ_DIR)/instructions.o
	g++ -O2 $(OBJ_DIR)/main.o $(OBJ_DIR)/instruction.o $(OBJ_DIR)/instructions.o $(OBJ_DIR)/unit_fault.o -o relyzer-front-end
	
$(OBJ_DIR)/instructions.o: $(SRC_DIR)/instructions.cpp $(SRC_DIR)/instructions.h $(SRC_DIR)/unit_fault.h $(SRC_DIR)/instruction.h $(SRC_DIR)/common_functions.h
	g++ -O2 -c $(SRC_DIR)/instructions.cpp -o $(OBJ_DIR)/instructions.o

$(OBJ_DIR)/instruction.o: $(SRC_DIR)/instruction.cpp $(SRC_DIR)/instruction.h $(SRC_DIR)/unit_fault.h $(SRC_DIR)/common_functions.h
	g++ -O2 -c $(SRC_DIR)/instruction.cpp -o $(OBJ_DIR)/instruction.o

$(OBJ_DIR)/unit_fault.o: $(SRC_DIR)/unit_fault.cpp $(SRC_DIR)/unit_fault.h
	g++ -O2 -c $(SRC_DIR)/unit_fault.cpp -o $(OBJ_DIR)/unit_fault.o

$(OBJ_DIR)/main.o: $(SRC_DIR)/instructions.h $(SRC_DIR)/unit_fault.h $(SRC_DIR)/main.cpp
	g++ -O2 -c $(SRC_DIR)/main.cpp -o $(OBJ_DIR)/main.o


###############################################################
#profile applications to generate gcov
###############################################################

profile_prime: 
	gcc -O0 -fprofile-arcs -ftest-coverage $(APPS)/prime.c -o profile_prime
	./profile_prime
	gcov $(APPS)/prime.c
	mv prime.c.gcov $(APPS)/
	rm -f profile_prime prime.gc*
	
profile_hello: 
	gcc -O0 -fprofile-arcs -ftest-coverage $(APPS)/hello.c -o profile_hello 
	./profile_hello
	gcov $(APPS)/hello.c
	mv hello.c.gcov $(APPS)/
	rm -f profile_hello hello.gc*
	
generate_asm: 
	# cc -xO0 -m64 -S $(APPS)/prime.c # output: prime.s
	# cc -xO0 -m64 -S $(APPS)/hello.c # output: prime.s


###############################################################
#PERL FRONT END PARSING
###############################################################

#perl-front-end: hello-perl-front-end prime-perl-front-end blackscholes-perl-front-end
#perl-front-end: prime-perl-front-end blackscholes-perl-front-end swaptions-perl-front-end lu-perl-front-end fft-perl-front-end
#perl-front-end: blackscholes-perl-front-end swaptions-perl-front-end lu-perl-front-end fft-perl-front-end omnetpp-perl-front-end gcc-perl-front-end libquantum-perl-front-end
#perl-front-end: blackscholes-perl-front-end swaptions-perl-front-end fft-perl-front-end lu-perl-front-end libquantum-perl-front-end

#perl-front-end: fft-perl-front-end gcc-perl-front-end libquantum-perl-front-end lu-perl-front-end mcf-perl-front-end ocean-perl-front-end streamcluster-perl-front-end swaptions-perl-front-end water-perl-front-end

#perl-front-end: $(APP)-perl-front-end

perl-front-end:
	$(PERL) $(PERL_DIR)/sparc-front-end.pl $(APP) $(APP)

#perl-front-end: blackscholes-perl-front-end 
#perl-front-end: swaptions-perl-front-end 
#perl-front-end: blackscholes-perl-front-end swaptions-perl-front-end fluidanimate-perl-front-end streamcluster-perl-front-end swaptions-perl-front-end fft-perl-front-end lu-perl-front-end ocean-perl-front-end water-perl-front-end gcc-perl-front-end mcf-perl-front-end libquantum-perl-front-end omnetpp-perl-front-end 
#perl-front-end: blackscholes-perl-front-end swaptions-perl-front-end fluidanimate-perl-front-end streamcluster-perl-front-end swaptions-perl-front-end fft-perl-front-end lu-perl-front-end ocean-perl-front-end water-perl-front-end gcc-perl-front-end mcf-perl-front-end libquantum-perl-front-end 


#hello-perl-front-end:
#	#$(PERL) $(PERL_DIR)/parse_gcov.pl hello
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl hello hello c
#
#prime-perl-front-end: 
#	#$(PERL) $(PERL_DIR)/parse_gcov.pl prime # output: prime.clean.s
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl prime prime c # output: prime.clean.s
#
#blackscholes_simlarge-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl blackscholes_simlarge blackscholes_simlarge c
#blackscholes-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl blackscholes blackscholes c
#
#fluidanimate-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl fluidanimate fluidanimate cpp
#streamcluster-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl streamcluster streamcluster cpp
#swaptions_simsmall-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl swaptions_simsmall swaptions_simsmall cpp
#
#fft_small-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl fft_small fft_small C
#lu_reduced-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl lu_reduced lu_reduced C
#ocean-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl ocean ocean C
#water_small-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl water_small water_small C
#
#gcc-perl-front-end:
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl gcc gcc c
#mcf-perl-front-end:
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl mcf mcf c
#libquantum-perl-front-end:
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl libquantum libquantum c
#omnetpp-perl-front-end:
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl omnetpp omnetpp c
#
#blackscholes_input_run_00000-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl blackscholes_input_run_00000 blackscholes_input_run_00000 c
#blackscholes_input_run_00001-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl blackscholes_input_run_00001 blackscholes_input_run_00001 c
#blackscholes_input_run_00002-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl blackscholes_input_run_00002 blackscholes_input_run_00002 c
#blackscholes_input_run_00003-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl blackscholes_input_run_00003 blackscholes_input_run_00003 c
#blackscholes_input_run_00004-perl-front-end: 
#	$(PERL) $(PERL_DIR)/sparc-front-end.pl blackscholes_input_run_00004 blackscholes_input_run_00004 c
#
#



###############################################################
#cleaning
###############################################################

clean:
	rm -f $(APPS)/*/*.clean.s relyzer-front-end $(OBJ_DIR)/*.o 

clean-cpp-front-end:
	rm -f relyzer-front-end $(OBJ_DIR)/*.o
