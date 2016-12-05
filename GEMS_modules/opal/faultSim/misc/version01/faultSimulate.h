// faultSimulate.h

#ifndef FAULTSIMULATE_H
#define FAULTSIMULATE_H

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <deque>

#include <sys/types.h>

#include "structuralModule.h"

#define logic_X 2
#define logic_Z 3
#define REG_WIDTH 64
#define CIN 0

using namespace std;

typedef u_int64_t type;

//****************************** class definitions ************************************//
class faultSimulator;
//****************************** class definitions ************************************//

//****************************** containers *******************************************//
typedef vector<gateData> stimuliList;
// event queues per level
typedef deque<int> activeGateList;
typedef vector<activeGateList> eventTable;

// TODO optimize: memoize results 
typedef pair<type,type> operandKey;
typedef map<operandKey,type> table;
//****************************** containers *******************************************//


//****************************** class LUT ************************************//
//class LUT{
//	public:
//		LUT();
//		~LUT();
//		table data;
//	private:
//}
//****************************** class LUT ************************************//


//****************************** class faultSimulator ************************************//
class faultSimulator{
	public:
		faultSimulator();
		~faultSimulator();
		void extractStimuli(string line);
		void vecRead(type op1, type op2);
		void initEvents(void);
		type faultSimulate(void);
		void clearSim();
		void initializePI(int index);	
		logicValue encode(int value);		
		int decode(logicValue value);
		bool toBeEvaluated(int gateID, logicValue nextValue);
		void updateFanout(int gateID);
		void updateSched(int gateID);
		void traverseLevel(int level);
		logicValue evaluate(int gateID);
		void schedule(int gateID);
		type writeResults();
	
		//**************** fault injection routines **********************//
		void pickFaultyBehavior(int faultyGateId, bool faultyValue, int faultyFanoutId);
		void pickFaultyBehavior();
		void injectRandomFault(void);
		void insertOutputBuffer(int gateId, int faultyGateId);
		int generateConstantOutputGate(int type, int faultyGateId);
		//**************** fault injection routines **********************//
		
		//**************** gate evaluation routines **********************//
		logicValue evalXOR(int gateID);
		logicValue evalNOT(int gateID);
		logicValue evalAND(int gateID);
		logicValue evalOR(int gateID);
		logicValue evalBUF(int gateID);
		logicValue evalINPUT(int gateID);
		logicValue evalNAND(int gateID);
		logicValue evalNOR(int gateID);
		logicValue evalXNOR(int gateID);
		logicValue logicZ();
		logicValue logicX();
		logicValue logic0();
		logicValue logic1();
		//**************** gate evaluation routines **********************//
	
		// stimuli table
		stimuliList stimuli;
		// event queue
		eventTable events;
		// simulation statistics
		int numGateEval;
		int numEvents;	

		//--------------------------- STATS -------------------------------------//
		// keeps the number of calls from the high level simulator
		int numInvoke;
		// keeps the number of corruptions
		int numCorrupt;
		// general purpose counter
		int counter;
		//--------------------------- STATS -------------------------------------//

		// memoization table
//		LUT memory;

	private:	
};
//****************************** class faultSimulator ************************************//

#endif 

