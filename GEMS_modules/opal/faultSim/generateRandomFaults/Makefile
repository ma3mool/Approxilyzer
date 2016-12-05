############################################################### 									
#	 		      				      #                                                                     
###############################################################

generateRandomFault: generateRandomFault.o structuralModule.o 
	g++ -o3 generateRandomFault.o structuralModule.o -o generateRandomFault
generateRandomFault.o: generateRandomFault.cpp structuralModule.cpp structuralModule.h  
	g++ -c  generateRandomFault.cpp
structuralModule.o: structuralModule.cpp structuralModule.h
	g++ -c structuralModule.cpp
clean:
	rm *.o


