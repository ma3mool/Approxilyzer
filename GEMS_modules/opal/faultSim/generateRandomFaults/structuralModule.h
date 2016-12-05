// structuralModule.h

#ifndef STRUCTURAL_MODULE_H
#define STRUCTURAL_MODULE_H

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <map>
#include <set>

#define ID 0
#define TYPE 1
#define LEVEL 2
#define NUMI 3
#define NUMO 4
#define OBS 5
#define C0 6
#define C1 7

#define AND 6
#define OR 8
#define BUFFER 11

using namespace std;


//****************************** class definitions ************************************//
class logicValue;
class gate;
class  fault;
class structuralModule;
//****************************** class definitions ************************************//


//****************************** containers *******************************************//
typedef vector<int> gateData;
typedef map<int,gate> circuitTable;
typedef map<int,logicValue> table;
typedef vector<char> valueString;
//****************************** containers *******************************************//


//****************************** class logicValue ************************************//
class logicValue{
	public:
		logicValue();
		~logicValue();
		void setValue(string s);
		void invert();
		bool is0();
		bool is1();
		bool isX();
		bool isZ();

		bool operator!=(logicValue& lv);
//		void operator=(logicValue& lv);
		void print();
		
		bool mostSignificantBit;
		bool leastSignificantBit;

	private:	
};
//****************************** class logicValue ************************************//


//****************************** class gate ************************************//
class gate{
	friend class structuralModule;
	friend class faultSimulator;
	public:
		gate();
		~gate();

		void printTable(table& t);
		void printGate();
		void printLevel();
		
		void operator=(gate& g);
		
		// SEE whether neccessary
		void updateFanOutEntry(int branchId, logicValue& v);
		void updateC0FanInEntry(int branchId, logicValue& v);
		void updateC1FanInEntry(int branchId, logicValue& v);
		void setGateIOToX();

		bool isANDNOR();
		bool isORNAND();
		bool isEquiv();

		bool isRedundant;

		bool isScheduled;
		logicValue value;
		
//		logicValue goodValue;

	// for gate class gateData is a int vector of size 8: 
	//		node identifier|gate type|level|fan-in count|fan-out count|observability|C0|C1
	//		C0 and C1 incdicate 0 and 1 controllability respectively...
	// SEE HITEC/PROOFS user manual Table I for gate type - gate type identifier mapping
		gateData gateDescription;

	// this table is indexed by the unique id of a gate (the node id of its output)
	// each entry corresponds to a list of the fanouts of a gate in order of decreasing 
	// observability (easier to observe)
		table fanOutTable;
		gateData fanoutsInOrder;


	// this table is indexed by the unique id of a gate (the node id of its output)
	// each entry corresponds to a list of the fanins of a gate in order of decreasing 
	// controllability 0 (easier to control)
		table c0fanInTable;
		gateData c0fanInsInOrder;

	
	// this table is indexed by the unique id of a gate (the node id of its output)
	// each entry corresponds to a list of the fanins of a gate in order of decreasing 
	// controllability 1 (easier to control)	
		table c1fanInTable;
		gateData c1fanInsInOrder;


	private:	
};
//****************************** class gate ************************************//


//****************************** class fault ************************************//
class fault{
	public:
		fault();
		~fault();
		// true indicates sa1, false sa0
		bool faultType;
		// index of the gate where the fault appears at a fanout
		int faultSite;
		int faultyFanoutIndex;	
};
//****************************** class fault ************************************//


//****************************** class structuralModule ************************************//
class structuralModule{
	// this class contains circuit information read from the .lev file
	friend class faultSimulator;
	public:
		structuralModule();
		~structuralModule();
		void levRead(string levFileName);
		void extractGate(string line);
		void setLinesToX();
		void countLevels(void);		
		
		
		// debugging routines
		void printGateData();
		void printLevelCounter(void);
		void printCircuit();
		void printTable(gateData& g);
		
		
		void extractModuleName(string circuitFileName);
		void updateValue(int gateId, logicValue v);
		
		void clearDesign();

		// circuit data is kept in the table circuit, which is indexed by gate number 
		// (node identifier at the output of the gate)
		circuitTable circuit;

		gateData PIList;
		gateData POList;
		
		fault saf;
		
		// reference: typedef map<int,logicValue> table
		// the logic value here represents the next state value for the FF under consideration
//		table FF;

		// data structure keeping the number of gates per level
		gateData levelCounter;
		
		// num gates + 1, with PI/POs included
		int numGates;
		
		// maximum level number encountered in the circuit
		int maxLevel;
		
		int numPI;
		string moduleName;

	private:	
};
//****************************** class structuralModule ************************************//

#endif 

