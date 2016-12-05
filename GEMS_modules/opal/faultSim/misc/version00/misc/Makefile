############################################################### 									
#	 		      				      #                                                                     
###############################################################
sim: sim.o module.o faultSimulate.o 
	g++ -o3 sim.o module.o faultSimulate.o -o sim
sim.o: sim.c module.c module.h faultSimulate.c faultSimulate.h 
	g++ -c sim.c
module.o: module.c module.h
	g++ -c module.c
faultSimulate.o: faultSimulate.c faultSimulate.h module.c module.h
	g++ -c faultSimulate.c

clean:
	rm *.o

#NOTES
#do not link:	g++ -c
#include g++ optimization parameters


