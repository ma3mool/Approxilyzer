############################################################### 									
#	 		      				      #                                                                     
###############################################################

DEBUG_FLAGS_SIM += -DDEBUG_SIM00
#DEBUG_FLAGS_SIM += -DDEBUG_SIM01
DEBUG_FLAGS_FSIM += -DDEBUG_FSIM
#DEBUG_FLAGS_FSIM += -DDEBUG_FSIM01

sim: sim.o structuralModule.o faultSimulate.o 
#	g++ -o3 $(DEBUG_FLAGS_SIM) $(DEBUG_FLAGS_FSIM) sim.o structuralModule.o faultSimulate.o -o sim
	g++ -o3 sim.o structuralModule.o faultSimulate.o -o sim
sim.o: sim.c structuralModule.c structuralModule.h faultSimulate.c faultSimulate.h 
	g++ -c $(DEBUG_FLAGS_SIM) sim.c
structuralModule.o: structuralModule.c structuralModule.h
	g++ -c structuralModule.c
faultSimulate.o: faultSimulate.c faultSimulate.h structuralModule.c structuralModule.h
	g++ -c $(DEBUG_FLAGS_FSIM) faultSimulate.c
clean:
	rm *.o


