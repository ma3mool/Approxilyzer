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

//******************* HIGH LEVEL SIM INTERFACE *********************************//
// TODO: OPTIMIZE
//#include "debugio.h" // log routines for file I/O
#include "hfa.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/stat.h>	
#include <fcntl.h>

#define LUT_LIMIT 500
//******************* HIGH LEVEL SIM INTERFACE *********************************//

#define logic_X 2
#define logic_Z 3
#define REG_WIDTH 65
#define CIN 0
#define NAME_SIZE 1001

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
typedef set<type> operandKey;
typedef map<operandKey,type> fSimLUT;
//****************************** containers *******************************************//

struct pattern_t {
	uint64 mask;
	uint64 dir;
	pattern_t(): mask(0ULL),dir(0ULL) {}
} ;

typedef struct {
	// uint64 pattern;
	pattern_t pattern;
	double prob;
} pattern_prob_t;

typedef vector< pattern_prob_t > pattern_prob_list_t;

typedef struct {
	int num_bit_flips;
	double prob;
	pattern_prob_list_t pattern_prob;
} bit_flip_prob_t;

struct compare_pattern
{
	bool operator()(const pattern_t &s1, const pattern_t &s2)  {
		if (s1.mask!= s2.mask) 
			return (s1.mask < s2.mask);
		return (s1.dir < s2.dir);
	}
};

typedef	map<pattern_t,uint64,compare_pattern> pattern_map_t;
typedef vector< bit_flip_prob_t > pattern_profile_t;

//****************************** class faultSimulator ************************************//
class faultSimulator{
	public:
		faultSimulator();
		~faultSimulator();
		void extractStimuli(string line);
		void vecRead(type op1, type op2);
		void initEvents(void);
		type faultSimulate(type op1, type op2);
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
		void insertToLUT(type op1, type op2, type &inj_result);
	
		//************************************** fault injection routines **********************************//
		void pickFaultyBehavior(int faultyGateId, bool faultyValue, int faultyFanoutId);
		void pickFaultyBehavior();
		void injectRandomFault();
		void insertOutputBuffer(int gateId, int faultyGateId);
		int generateConstantOutputGate(int type, int faultyGateId);
		void numBitFlips(type op1, type op2);
		void numBitFlipsRetire(type op1, type op2);
		bool checkLUT(type op1, type op2, type &result);
		void lluToBinStr(char *s, type val, int width);	
		//*********************************** fault injection routines *************************************//
		
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

		int getFlipCounters(int idx) {
			switch (idx) {
			case 1:
				return multiBit1;
			case 2:
				return multiBit2;
			case 3:
				return multiBit3;
			case 4:
				return multiBit4;
			case 5:
				return multiBit5;
			default:
				return multiBit5B;
			}
		}

		int getFlipCountersRetire(int idx) {
			switch (idx) {
			case 1:
				return multiBit1Retire;
			case 2:
				return multiBit2Retire;
			case 3:
				return multiBit3Retire;
			case 4:
				return multiBit4Retire;
			case 5:
				return multiBit5Retire;
			default:
				return multiBit5BRetire;
			}
		}

		uint64 getAnalysis(int idx) {
			switch (idx) {
			case 1:
				return bit_invariant;
			case 2:
				return min;
			case 3:
				return max;
			case 4:
				return (numCorrupt==0)?0:avg/numCorrupt;
			}
		}

		uint64 getRetireAnalysis(int idx) {
			switch (idx) {
			case 1:
				return ret_bit_invariant;
			case 2:
				return ret_min;
			case 3:
				return ret_max;
			case 4:
				return (numCorruptRetire==0)?0:ret_avg/numCorruptRetire;
			}
		}
	
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
		// keeps the number of outputs of the circuit that changed
		int multiBitCorruption;

		// general purpose counter
		int counter;
		// keeps the number of references made to LUT
		int LUTReference;
		// regarding u-arch. corruption analysis
		int indexMeter[REG_WIDTH-1];

		// RETIRE
		int numInvokeRetire;
		int numCorruptRetire;
		int numMaskRetire;

		int multiBitCorruptionRetire;

		int logic_zero;
		int logic_one;
		int logic_zero_retire;
		int logic_one_retire;

		int inst_logic_count;
		int inst_arith_count;

		int counterRetire; // currently not used
		int indexMeterRetire[REG_WIDTH-1];

		int tmp_op2[REG_WIDTH-1];

		//--------------------------- STATS -------------------------------------//

		//------------------------ FAULT INJECTION RELATED ---------------------//
		//--------------------------------- memoization table
		fSimLUT fSimMem;
		//--------------------------------- interface to low level simulator
		int fdWrite;
		int fdRead;
		char writeFIFO[NAME_SIZE];
		char readFIFO[NAME_SIZE];
//		string writeFIFO;
//		string readFIFO;
		int nread;
		int nwrite;
		//------------------------ FAULT INJECTION RELATED ---------------------//
		uint64 useBadALU;

		pattern_map_t &getPatternList() {
			return pattern_list;
		}

		void processFaultModelFile(char* app_name, int act_rate);

		pattern_t genPattern();
		double getActRate() { return m_act_rate;}

	private:	

		void printProfile();
		int multiBit1;
		int multiBit2;
		int multiBit3;
		int multiBit4;
		int multiBit5;
		int multiBit5B;

		int multiBit1Retire;
		int multiBit2Retire;
		int multiBit3Retire;
		int multiBit4Retire;
		int multiBit5Retire;
		int multiBit5BRetire;

		uint64 bit_invariant;
	    int64 min, max, avg;

		uint64 ret_bit_invariant;
	    int64 ret_min, ret_max, ret_avg;

		double m_act_rate;

		pattern_map_t pattern_list;

		pattern_profile_t pattern_profile;
};
//****************************** class faultSimulator ************************************//

#endif 


