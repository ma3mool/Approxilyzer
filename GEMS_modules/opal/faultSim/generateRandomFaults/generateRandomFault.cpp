// generateRandomFault.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include "structuralModule.h"

#define NUM_FAULTS 250

structuralModule design;

int main(void)
{
	int faultyGateId;
	int faultyValue;

	ofstream randomData;	
	string outFileName = "randomData.txt";

	string levFileName = "RAC.lev";

	srand(time(NULL));

	try{
		randomData.open(outFileName.c_str(),ios::app);
		if(!randomData) throw "generateRandomFault> Cannot open data file...";

		// read design into mem:
		design.levRead(levFileName);

		if(randomData.is_open()){
			randomData << "FAULT SITE\tFAULTY FANOUT\tSAF VALUE" << endl;

			for(int i = 0; i < NUM_FAULTS; i++){

				faultyGateId = random()%(design.numGates-1);
				faultyValue = random()%2;
				if(design.circuit[faultyGateId].fanOutTable.size() != 0){
					design.saf.faultyFanoutIndex = random()%(design.circuit[faultyGateId].fanOutTable.size()+1);
					// which fanout branch of the gate is faulty? 0 is not excluded here!
				}
				else{
					design.saf.faultyFanoutIndex = 0;
					// this gate has no fanout, so its output net is characterized as fault site
				}

				// true indicates sa1, false sa0
				design.saf.faultType = (faultyValue == 1);
				// index of the gate where the fault appears at a fanout
				design.saf.faultSite = faultyGateId;

				randomData << design.saf.faultSite << "\t\t" << design.saf.faultyFanoutIndex << "\t\t" << design.saf.faultType << endl;
			}			
		}	
		randomData << endl;
		randomData.close();
	}
	catch (string msg) {
		cout << "ERROR :" << msg << endl;
		exit(1);
	}

	return 0;
}
