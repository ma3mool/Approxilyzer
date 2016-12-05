// faultSimulate.C

#include "faultSimulate.h"

extern structuralModule design;


//****************************** class LUT ************************************//
//LUT::LUT(){}
//LUT::~LUT(){}
//****************************** class LUT ************************************//


//****************************** class faultSimulator ************************************//
faultSimulator::faultSimulator(){

	numGateEval = 0;
	numEvents = 0;

	numInvoke = 0;
	// general purpose counter
	counter = 0;
	numCorrupt = 0;
}

faultSimulator::~faultSimulator(){}

void faultSimulator::clearSim()
{
	events.clear();
	stimuli.clear();
}

void faultSimulator::vecRead(type op1, type op2)
{
	// extractStimuli
	design.numPI = 2*REG_WIDTH + 1; // 1 for cin
	gateData readIn(design.numPI);
	gateData permuted(design.numPI);

	type scratch;
	type mask = (type)1;

	int i;

	// read first operand - least significant bit at [63]
	for(i = 0; i<REG_WIDTH; i++){
		scratch = (mask & op1)>>i;
		if(scratch == (type)1){
			readIn[i] = (int)1;
		}
		else{
			readIn[i] = (int)0;
		}
		mask = mask<<1;
	}

#ifdef DEBUG_FSIM
	printf("First op read as  ");
	for(int u=REG_WIDTH-1; u>-1; u--){
		printf("%d", readIn[u]);
	}
	printf("\n");
#endif
	
	mask = (type)1;

	// read second operand
	for(i = REG_WIDTH; i<2*REG_WIDTH; i++){
		scratch = (mask & op2)>>(i-REG_WIDTH);
		if(scratch == (type)1){
			readIn[i] = (int)1;
		}  
		else{
			readIn[i] = (int)0;
		}
		mask = mask<<1;
	}

#ifdef DEBUG_FSIM
	printf("Second op read as  ");
	for(int u=2*REG_WIDTH-1; u>REG_WIDTH-1; u--){
		printf("%d", readIn[u]);
	}
	printf("\n");
#endif

	readIn.push_back(CIN);

	//TODO - fix spagetti code! 
	// map readIn > A B cin
	permuted[0] = readIn[0];
	permuted[1] = readIn[REG_WIDTH];
	permuted[2] = readIn[2*REG_WIDTH];
	i = 1;
	int index;
	while(i < REG_WIDTH){
		index = 2*i + 1;
		permuted[index] = readIn[i];
		permuted[index + 1] = readIn[i+REG_WIDTH];
		i++;
	}

	stimuli.push_back(permuted);
}

void faultSimulator::initEvents(void){

	int tableSize = (design.maxLevel+1);
	eventTable e( tableSize );

	events.clear();

	for(int i = 0; i < tableSize; i++){
		activeGateList a(design.levelCounter[i]);
		e.push_back(a);
	}

	events = e;
	e.clear();
}

type faultSimulator::faultSimulate(void){

	// TODO if batch mode out of consideration, clean code
	type result;

#ifdef DEBUG_FSIM
	cout << " \n************************ FAULT INJECTED - FSIM ********************************* " << endl;
	cout << "faultyGateId " << design.saf.faultSite << endl;
	cout << "faultyValue " << design.saf.faultType << endl;
	cout << "faultyFanoutIndex " << design.saf.faultyFanoutIndex << endl;
	cout << "**************************************************************************\n " << endl;
#endif
	
	// for each vector read:
//	for(int i = 0; i < stimuli.size(); i++){
		int i = 0;
		// Initialize PI logic values from input vector
		initializePI(i);

#ifdef DEBUG_FSIM01
		cout << "	PIs initialized to the new stimuli" << endl;
#endif
		// detecting events induced at each level
		for(int j = 0; j < (design.maxLevel+1); j++){
		
			traverseLevel(j);
#ifdef DEBUG_FSIM01
			cout << "	traversing level " << j << endl;
#endif
		}

#ifdef DEBUG_FSIM01
		cout << "	control reaches writeResults! " << endl;
#endif	

		// increase the number of calls from the high level simulator
		numInvoke++;

		// write simulation results to output file
		result = writeResults();

//	} // end for: traversing the input stimuli

		// TODO: hashing introduces another source of error!
		// implement a memoization scheme - the table lookup goes here

		return result;
}

type faultSimulator::writeResults(){

	char response[REG_WIDTH+1];
	
	logicValue value;
	type returnValue;

	char *ptr;
	int it;

#ifdef DEBUG_FSIM01
	cout << "In writeResults ..." << endl;
#endif

	// for strtoull to operate correctly, the least significant bit should correspond to position 64!
	int index = REG_WIDTH - 1;
	
#ifdef DEBUG_FSIM01
	cout << " Number of POs is " << design.POList.size() << endl;
#endif
	
	// Outputs are specified in the order read from .lev file
	for(it = 0; it < design.POList.size()-2; it++){
		
		value = design.circuit[design.POList[it]].value;

#ifdef DEBUG_FSIM01
		cout << "	In writeResults: for loop, iteration " << it << endl;
		cout << "	The value of index at this point is " << index << endl;
#endif
	
		// TODO how to handle X and Z values
		if(value.is0()){
			response[index--] = '0';
		}
		else{
			response[index--] = '1';
		}
	}		
	
	value = design.circuit[design.POList[it+1]].value;
	if(value.is0()){
		response[index--] = '0';
	}
	else{
		response[index--] = '1';
	}		

/* TODO cout skipped here - maybe included for more sophisticated add instructions 
 	value = design.circuit[design.POList[it]].value;
	if(value.is0()){
		response.push_back('0');
	}
	else{
		response.push_back('1');
	}
*/
	returnValue = strtoull(response, &ptr, 2);

	#ifdef DEBUG_FSIM
		cout << "RESULT(B): ";
		for(int debug = 0; debug < REG_WIDTH; debug++)
			cout << response[debug];
		cout << endl;
		printf("Result(D): %llu\n", returnValue);
//		printf("Result(H): %X\n", returnValue);
	#endif	
	
	return returnValue;
}

// *********void initializePI(int index)*************************************
//	Purpose: Initialize the PI logic values and enqueue each PI in the event 
//			list of level 0, if a value change is detected 
//	Parameters: The index of the input vector in the stimuli table
//	Returns: -
// **************************************************************************
void faultSimulator::initializePI(int index)
{	
	logicValue nextValue;
	gateData inpVec = stimuli[index];
	activeGateList a;
	a.clear();

	for(int i = 0; i < design.numPI; i++){
		nextValue = encode(inpVec[i]);		
		// enqueue an event if change detected
		if( toBeEvaluated(design.PIList[i], nextValue) ){
			a.push_back(design.PIList[i]);
			updateSched(design.PIList[i]);
			numEvents++;
			design.updateValue(design.PIList[i],nextValue);
		}
	}

	if(!a.empty()){
		// this assignment should be ok, since all event lists are empty when
		// the next input stimuli is read
		events[0] = a;
	}
}


// *********bool toBeEvaluated(int gateID, int nextValue)********************
//	Purpose: Detects whether an event needs to be created upon application
//			of new input stimuli
//	Parameters: The node ID of the corresponding gate output, and the value
//			to be assigned according to the new stimuli
//	Returns: Whether the gate fanouts needs to be evaluated or not
// **************************************************************************
bool faultSimulator::toBeEvaluated(int gateID, logicValue nextValue){

		logicValue prevValue = design.circuit[gateID].value;
		
		if(prevValue != nextValue){
			return true;
		}

		return false;
}

// Updating the scheduled flag
void faultSimulator::updateSched(int gateID){

	if(design.circuit[gateID].isScheduled)
		design.circuit[gateID].isScheduled = false;
	else
		design.circuit[gateID].isScheduled = true;
}


// *********void traverseLevel(int level)****************************************
//	Purpose: Evaluate events at level level
//	Parameters: The level identifier
//	Returns: -
// ******************************************************************************
void faultSimulator::traverseLevel(int level){
	
	int activeGateID;
	logicValue activeGateValue;

	while(!events[level].empty()){
	
		activeGateID = events[level].front();
		// un-scheduling the event
		updateSched(activeGateID);
		events[level].pop_front();	
	
		if(level != 0){
			activeGateValue = evaluate(activeGateID);

			if( toBeEvaluated(activeGateID, activeGateValue) ){

				design.updateValue(activeGateID, activeGateValue);
				updateFanout(activeGateID);
			}
		}
		else{
	
			updateFanout(activeGateID);
		}

	}// end while: traversing event list at level j
}


// *********int evaluate(int gateID)*****************************************
//	Purpose: Evaluate the logic value at the output of the gate specified
//			 by the input parameter gateID
//	Parameters: The output node idendifier of the gate the output of which is
//				to be evaluated
//	Returns: The output value of the gate
// **************************************************************************
logicValue faultSimulator::evaluate(int gateID){

	switch(design.circuit[gateID].gateDescription[TYPE]){
		case 1: // input
			return evalINPUT(gateID);
		case 2: // output
			return evalBUF(gateID);
		case 3: // xor	
			return evalXOR(gateID);
		case 4: // xnor
			return evalXNOR(gateID);
		case 6: // and
			return evalAND(gateID);
		case 7: // nand
			return evalNAND(gateID);
		case 8: // or
			return evalOR(gateID);
		case 9: // nor
			return evalNOR(gateID);
		case 10: // not
			return evalNOT(gateID);
		case 11: // buf
			return evalBUF(gateID);
		case 12: // tie1
			return logic1();
		case 13: // tie0
			return logic0();
		case 14: //tieX
			return logicX();
		case 15: //tieZ
			return logicZ();
		default: throw "faultSimulator::evaluate> Gate type not supported...";
	}
}

// *********void updateFanout(int gateID)****************************************

//	Parameters: The output node idendifier of the gate the fanout of which is
//				to be evaluated
//	Returns: -
// ******************************************************************************
void faultSimulator::updateFanout(int gateID){

	gateData::iterator it;
	int fanOutGate;

	// for every gate in the fanout list
	for(it = design.circuit[gateID].fanoutsInOrder.begin(); it != design.circuit[gateID].fanoutsInOrder.end(); it++){		
		fanOutGate = *it;
		if(!design.circuit[fanOutGate].isScheduled){
				schedule(fanOutGate);
		}
	}
}


// *********int encode(int value)*****************************************
//	Purpose: Encode the value read from the stimuli file according to
//			the internal representation of three valued logic (0, 1, X)
//	Parameters: The value (bit) read from the vec file
//	Returns: The encoding
// ************************************************************************
logicValue faultSimulator::encode(int value){

	logicValue v;
	
	if(value == 0){
		v.setValue("0");
	}
	else if(value == 1){
		v.setValue("1");		
	}
	else if(value == logic_X){
		v.setValue("X");
	}
	else if(value == logic_Z){
		v.setValue("Z");
	}
	else throw "faultSimulator::encode> Undefined logic value encountered!";
	
	return v;
}


int faultSimulator::decode(logicValue value){
	
	if(value.is0()){
		return 0;
	}
	else if(value.is1()){
		return 1;
	}
	else if(value.isX()){
		return logic_X;
	}
	else if(value.isZ()){
		return logic_Z;
	}
	else throw "faultSimulator::decode> Undefined logic value encountered!";
}


// *********void schedule(int gateID)*******************************************
//	Purpose: Schedule an event concerning the gate specified by gateID
//	Parameters: The output node idendifier of the gate which is to be scheduled
//	Returns: -
// *****************************************************************************
void faultSimulator::schedule(int gateID){

	int level = design.circuit[gateID].gateDescription[LEVEL];

	updateSched(gateID);

	events[level].push_back(gateID);

	// collecting statistical data
	numEvents++;
}


/********************gate evaluation routines**************************/
logicValue faultSimulator::evalXOR(int gateID){

	int inp0Id = design.circuit[gateID].c0fanInsInOrder[0];
	int inp1Id = design.circuit[gateID].c0fanInsInOrder[1];

	logicValue inp0 = design.circuit[gateID].c0fanInTable[inp0Id];
	logicValue inp1 = design.circuit[gateID].c0fanInTable[inp1Id];

	int numInp = design.circuit[gateID].gateDescription[NUMI];
	if(numInp != 2) throw "faultSimulator::evalXOR: Invalid number of inputs detected...";

	logicValue result;

	// V0 corresponds to mostSignificantBit, V1 to leastSignificantBit	
	result.mostSignificantBit = (inp0.mostSignificantBit&inp1.mostSignificantBit)|(inp0.leastSignificantBit&inp1.leastSignificantBit);
	result.leastSignificantBit = (inp0.mostSignificantBit&inp1.leastSignificantBit)|(inp1.mostSignificantBit&inp0.leastSignificantBit);	
	
	return result;
}


logicValue faultSimulator::evalNOT(int gateID)
{	
	int numInp = design.circuit[gateID].gateDescription[NUMI];
	if(numInp != 1) throw "faultSimulator::evalNOT: Invalid number of inputs detected...";

	int fanInId = design.circuit[gateID].c0fanInsInOrder[0];
	logicValue inputNode = design.circuit[gateID].c0fanInTable[fanInId];

	logicValue result;
	
	result.mostSignificantBit = inputNode.leastSignificantBit;
	result.leastSignificantBit = inputNode.mostSignificantBit;
	
	return result;
}


logicValue faultSimulator::evalAND(int gateID)
{	
	int numInp = design.circuit[gateID].gateDescription[NUMI];

	logicValue result;
	result.mostSignificantBit = false;
	result.leastSignificantBit = true;
	
	table::iterator it;	
	for(it = design.circuit[gateID].c0fanInTable.begin(); it != design.circuit[gateID].c0fanInTable.end(); it++){
		result.mostSignificantBit = result.mostSignificantBit|(*it).second.mostSignificantBit;
		result.leastSignificantBit = result.leastSignificantBit&(*it).second.leastSignificantBit;
	}

	return result;
}


logicValue faultSimulator::evalOR(int gateID)
{	
	int numInp = design.circuit[gateID].gateDescription[NUMI];
	
	logicValue result;
	result.mostSignificantBit = true;
	result.leastSignificantBit = false;

	table::iterator it;	
	for(it = design.circuit[gateID].c0fanInTable.begin(); it != design.circuit[gateID].c0fanInTable.end(); it++){
		result.mostSignificantBit = result.mostSignificantBit&(*it).second.mostSignificantBit;
		result.leastSignificantBit = result.leastSignificantBit|(*it).second.leastSignificantBit;
	}
			
	return result;	
}


logicValue faultSimulator::evalBUF(int gateID)
{	
	int numInp = design.circuit[gateID].gateDescription[NUMI];
	if(numInp != 1) throw "implier::evalBUF: Invalid number of inputs detected...";

	int fanInId = design.circuit[gateID].c0fanInsInOrder[0];
	logicValue inputNode = design.circuit[gateID].c0fanInTable[fanInId];

	logicValue result;

	result.mostSignificantBit = inputNode.mostSignificantBit;
	result.leastSignificantBit = inputNode.leastSignificantBit;

	return result;
}

logicValue faultSimulator::evalNAND(int gateID)
{
	logicValue result;
	logicValue complementedResult = evalAND(gateID);

	result.mostSignificantBit = complementedResult.leastSignificantBit;
	result.leastSignificantBit = complementedResult.mostSignificantBit;

	return result;
}

logicValue faultSimulator::evalNOR(int gateID)
{
	logicValue result;
	logicValue complementedResult = evalOR(gateID);

	result.mostSignificantBit = complementedResult.leastSignificantBit;
	result.leastSignificantBit = complementedResult.mostSignificantBit;
	
	return result;
}


logicValue faultSimulator::evalXNOR(int gateID){

	logicValue result;
	logicValue complementedResult = evalXOR(gateID);

	result.mostSignificantBit = complementedResult.leastSignificantBit;
	result.leastSignificantBit = complementedResult.mostSignificantBit;

	return result;
}

logicValue faultSimulator::evalINPUT(int gateID)
{
	int numInp = design.circuit[gateID].gateDescription[NUMI];
	if(numInp != 0) throw "faultSimulator::evalINPUT: Invalid number of inputs detected...";

	return design.circuit[gateID].value;
}

logicValue faultSimulator::logic1()
{
	logicValue l;
	l.mostSignificantBit = false;
	l.leastSignificantBit = true;
	return l;
}

logicValue faultSimulator::logic0()
{
	logicValue l;
	l.mostSignificantBit = true;
	l.leastSignificantBit = false;
	return l;
}

logicValue faultSimulator::logicX()
{
	logicValue l;
	l.mostSignificantBit = false;
	l.leastSignificantBit = false;
	return l;
}

logicValue faultSimulator::logicZ()
{
	logicValue l;
	l.mostSignificantBit = true;
	l.leastSignificantBit = true;
	return l;
}
/********************gate evaluation routines**************************/


void faultSimulator::pickFaultyBehavior()
{
	srand(time(NULL));
	int faultyGateId = random()%(design.numGates-1);
	int faultyValue = random()%2;

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
}


void faultSimulator::pickFaultyBehavior(int faultyGateId, bool faultyValue, int faultyFanoutId)
{
	design.saf.faultyFanoutIndex = 	faultyFanoutId;
	// true indicates sa1, false sa0
	design.saf.faultType = faultyValue;
	// index of the gate where the fault appears at a fanout
	design.saf.faultSite = faultyGateId;
}


void faultSimulator::injectRandomFault()
{
	int faultyGateId;
	bool faultyValue;

//	if(numInvoke==0)
		pickFaultyBehavior();			
//	else
//		pickFaultyBehavior(design.saf.faultSite, design.saf.faultType, design.saf.faultyFanoutIndex);

	faultyGateId = design.saf.faultSite;
	faultyValue =  design.saf.faultType;

#ifdef DEBUG_FSIM01
	printf("Number of outputs of the faulty gate: %d\n", design.circuit[faultyGateId].gateDescription[NUMO]);
#endif	
	// if the fanout value is different than 0, the objective is regarded as the input saf
	// excitation of the gate (specified by fanOutIndex) this gate feeds in
	//
	// gates are characterized as faulty according to faults at their fanouts  

#ifdef DEBUG_FSIM
	cout << " \n************************ FAULT INJECTED ********************************* " << endl;
	cout << "faultyGateId " << faultyGateId << endl;
	cout << "faultyValue " << faultyValue << endl;
	cout << "faultyFanoutIndex " << design.saf.faultyFanoutIndex << endl;
	cout << "**************************************************************************\n " << endl;
#endif	
	
	// following PROOFS method: Fault injection by gate insertion
	gate g;
	int gateId;
	if(!faultyValue){
		// insert a 2-input AND gate
		gateId = generateConstantOutputGate(AND, faultyGateId);
				
		g.gateDescription[ID] = ++gateId;
		g.gateDescription[TYPE] = AND;	
	}
	else if(faultyValue){
		// insert a 2-input OR gate
		gateId = generateConstantOutputGate(OR, faultyGateId);
		
		g.gateDescription[ID] = ++gateId;
		g.gateDescription[TYPE] = OR;
	}
	else throw "faultSimulator::injectRandomFault> Unrecognized logic value! ";

	// 1. update fanin lists and tables of the inserted AND/OR gate
	g.c1fanInsInOrder.push_back(faultyGateId);
	g.c1fanInsInOrder.push_back(gateId-1);
	g.c0fanInsInOrder.push_back(gateId-1);
	g.c0fanInsInOrder.push_back(faultyGateId);

	// add the faulty gate
	pair<int,logicValue> p;
	p.first = faultyGateId;
	p.second = logicX();
	g.c0fanInTable.insert(p);
	g.c1fanInTable.insert(p);
	// add the const output gate
	p.first = gateId - 1;
	p.second = design.circuit[p.first].value;
	g.c0fanInTable.insert(p);
	g.c1fanInTable.insert(p);


	// 2. fill in remaining tables in the gate description list
	g.gateDescription[LEVEL] = design.circuit[faultyGateId].gateDescription[LEVEL] + 1;
	g.gateDescription[NUMI] = 2;

	// TODO fix if neccessary!
//	g.gateDescription[OBS] = design.circuit[faultyGateId].gateDescription[OBS];
//	g.gateDescription[C0] = design.circuit[faultyGateId].gateDescription[C0];
//	g.gateDescription[C1] = design.circuit[faultyGateId].gateDescription[C1];
	
	// if injecting into a PO 
	if(design.circuit[faultyGateId].gateDescription[NUMO]==0){		
		g.fanOutTable = design.circuit[faultyGateId].fanOutTable;
		g.fanoutsInOrder = design.circuit[faultyGateId].fanoutsInOrder;
		g.gateDescription[NUMO] = design.circuit[faultyGateId].gateDescription[NUMO];
	}// end injection into a PO
	else{	
		// fault injected into stem
		if(design.saf.faultyFanoutIndex == 0){
			g.fanOutTable = design.circuit[faultyGateId].fanOutTable;
			g.fanoutsInOrder = design.circuit[faultyGateId].fanoutsInOrder;
			g.gateDescription[NUMO] = design.circuit[faultyGateId].gateDescription[NUMO];
		}
		else{ // fault injected into fanout branch
			int faultyFanOutId = design.circuit[faultyGateId].fanoutsInOrder[design.saf.faultyFanoutIndex-1];
			pair<int,logicValue> p;
			p.first = faultyFanOutId;
			p.second = logicX();
			g.fanOutTable.insert(p);
			g.fanoutsInOrder.push_back(faultyFanOutId);
			g.gateDescription[NUMO] = 1; 
		}
	} 
		
	g.isRedundant = true;
	g.isScheduled = false;

	// filling in the circuit table
	pair<int,gate> pC;
	pC.first = gateId;
	pC.second = g;
	design.circuit.insert(pC);	

	// upating the fanin and fanout lists
	p.first = gateId; // this is the inserted and/or gate 
	p.second = logicX();
	// 1. insert the new gate into the fanout list of the faulty gate
	// 2. insert the new gate into the fanin lists of the faulty gate's fanouts	
	if(design.saf.faultyFanoutIndex == 0){
		// injecting a fault at  the stem...
		
		// the fault site corresponds to a PO
		if(design.circuit[faultyGateId].gateDescription[NUMO]==0){
		
			design.circuit[faultyGateId].fanOutTable.clear();
			design.circuit[faultyGateId].fanOutTable.insert(p);
			design.circuit[faultyGateId].fanoutsInOrder.clear();
			design.circuit[faultyGateId].fanoutsInOrder.push_back(gateId);

			design.circuit[faultyGateId].gateDescription[NUMO] = 1;
			design.circuit[faultyGateId].gateDescription[TYPE] = BUFFER;

//			gateData::iterator git;
//			git = find(design.POList.begin(), design.POList.end(), faultyGateId);
//			design.POList.erase(git);

			// insert an output buffer at the output of the faulty gate
			insertOutputBuffer(gateId, faultyGateId);
		}
		// the fault site is not a PO, the injection is at the stem, but the faulty gate can be a fanout point or not
		// if it is a fanout point, the fanin tables of all its fanouts should be updated!
		else{
			gateData::iterator rePlace;
			gateData::iterator it;
			for(it = design.circuit[faultyGateId].fanoutsInOrder.begin(); it != design.circuit[faultyGateId].fanoutsInOrder.end(); it++){
				design.circuit[(*it)].c0fanInTable.erase(faultyGateId);
				design.circuit[(*it)].c0fanInTable.insert(p);
				rePlace = find(design.circuit[(*it)].c0fanInsInOrder.begin(), design.circuit[(*it)].c0fanInsInOrder.end(), faultyGateId);
				*rePlace = gateId;

				design.circuit[(*it)].c1fanInTable.erase(faultyGateId);
				design.circuit[(*it)].c1fanInTable.insert(p);
				rePlace = find(design.circuit[(*it)].c1fanInsInOrder.begin(), design.circuit[(*it)].c1fanInsInOrder.end(), faultyGateId);
				*rePlace = gateId;
			}
						
			design.circuit[faultyGateId].fanoutsInOrder[0] = gateId;
			design.circuit[faultyGateId].fanOutTable.clear();
			design.circuit[faultyGateId].fanOutTable.insert(p);
			design.circuit[faultyGateId].gateDescription[NUMO] = 1;
		}
	}
	else{ // we are injecting at a branch, not the stem!
		// extract the index of the faulty fanout 
		int faultyFanOutId = design.circuit[faultyGateId].fanoutsInOrder[design.saf.faultyFanoutIndex-1];
		gateData::iterator rePlace;

		design.circuit[faultyGateId].fanoutsInOrder[design.saf.faultyFanoutIndex-1] = gateId;
		design.circuit[faultyGateId].fanOutTable.erase(faultyFanOutId);
		design.circuit[faultyGateId].fanOutTable.insert(p);	

		design.circuit[faultyFanOutId].c0fanInTable.erase(faultyGateId);
		design.circuit[faultyFanOutId].c0fanInTable.insert(p);
		rePlace = find(design.circuit[faultyFanOutId].c0fanInsInOrder.begin(), design.circuit[faultyFanOutId].c0fanInsInOrder.end(), faultyGateId);
		*rePlace = gateId;

		design.circuit[faultyFanOutId].c1fanInTable.erase(faultyGateId);
		design.circuit[faultyFanOutId].c1fanInTable.insert(p);
		rePlace = find(design.circuit[faultyFanOutId].c1fanInsInOrder.begin(), design.circuit[faultyFanOutId].c1fanInsInOrder.end(), faultyGateId);
		*rePlace = gateId;
	}

	design.numGates += 2;
}

void faultSimulator::insertOutputBuffer(int gateId, int faultyGateId)
{
	int outputBufferId = gateId + 1;	
	gate g;
	g.gateDescription[ID] = outputBufferId;
	g.gateDescription[TYPE] = 2;
	g.gateDescription[LEVEL] = design.circuit[gateId].gateDescription[LEVEL] + 1;
	g.gateDescription[NUMI] = 1;
	g.gateDescription[NUMO] = 0;
	g.value = logicX();

	pair<int,logicValue> fi;
	fi.first = gateId;
	fi.second = logicX();
	g.c0fanInTable.insert(fi);
	g.c0fanInsInOrder.push_back(gateId);
	g.c1fanInTable.insert(fi);
	g.c1fanInsInOrder.push_back(gateId);

	pair<int,logicValue> fo;
	fo.first = outputBufferId;
	fo.second = logicX();
	design.circuit[gateId].fanOutTable.insert(fo);
	design.circuit[gateId].fanoutsInOrder.push_back(outputBufferId);
	design.circuit[gateId].gateDescription[NUMO] = 1;
	
	g.isRedundant = true;
	g.isScheduled = false;
	
	// filling in the circuit table
	pair<int,gate> pC;
	pC.first = g.gateDescription[ID];
	pC.second = g;
	design.circuit.insert(pC);

	// insert into the list of POs
	gateData::iterator rePlace;
	rePlace = find(design.POList.begin(), design.POList.end(), faultyGateId);
	*rePlace = outputBufferId;

	design.numGates++;

	if( design.maxLevel < g.gateDescription[LEVEL] ){
		design.maxLevel = g.gateDescription[LEVEL];
	}
}

int faultSimulator::generateConstantOutputGate(int type, int faultyGateId)
{
	// following PROOFS method: Fault injection by gate insertion
	int gateId = design.numGates - 1;	
	gate g;
	g.gateDescription[ID] = gateId;
	// pseudo PI
	g.gateDescription[TYPE] = 1;
	g.gateDescription[LEVEL] = 0;
	g.gateDescription[NUMI] = 0;
	g.gateDescription[NUMO] = 1;
	
	if(type == AND)
		g.value = logic0();
	else if(type == OR)
		g.value = logic1();
	else throw "faultSimulator::generateConstantOutputGate> Unrecognized gate type!"; 
		
	// TODO - fix if needed!
//	g.gateDescription[OBS] = design.circuit[faultyGateId].gateDescription[OBS];	
//	g.gateDescription[C0] = 0;
//	g.gateDescription[C1] = 0;
	
	pair<int,logicValue> fo;
	fo.first = gateId+1;
	fo.second = logicX();
	g.fanOutTable.insert(fo);
		
	g.fanoutsInOrder.push_back(gateId+1);
		
	g.isRedundant = true;
	g.isScheduled = false;
	
	// filling in the circuit table
	pair<int,gate> pC;
	pC.first = g.gateDescription[ID];
	pC.second = g;
	design.circuit.insert(pC);

	// insert into the list of PIs
	design.PIList.push_back(pC.first);
	
	return gateId;
}
//****************************** class faultSimulator ************************************//
