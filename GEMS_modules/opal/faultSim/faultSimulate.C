// faultSimulate.C

#include "faultSimulate.h"

#define MAX_NUM_PATTERNS 1000

structuralModule design;

//****************************** class faultSimulator ************************************//
faultSimulator::faultSimulator(){

	numGateEval = 0;
	numEvents = 0;

	numInvoke = 0;
	counter = 0;
	numCorrupt = 0;
	LUTReference = 0;

	multiBitCorruption = 0;

	multiBit1 = 0;
	multiBit2 = 0;
	multiBit3 = 0;
	multiBit4 = 0;
	multiBit5 = 0;
	multiBit5B = 0;

	// RETIRE
	numInvokeRetire = 0;
	numCorruptRetire = 0;
	numMaskRetire = 0;
	multiBitCorruptionRetire = 0;
	multiBit1Retire = 0;
	multiBit2Retire = 0;
	multiBit3Retire = 0;
	multiBit4Retire = 0;
	multiBit5Retire = 0;
	multiBit5BRetire = 0;
	counterRetire = 0;

	// initialize LUT
	fSimMem.clear();

	int i;
	for(i=0; i<REG_WIDTH; i++){
		indexMeter[i]=0;
		// RETIRE
		indexMeterRetire[i]=0;
		tmp_op2[i]=0;
	}

	logic_zero = 0;
	logic_one = 0;
	logic_zero_retire = 0;
	logic_one_retire = 0;

	inst_logic_count = 0;
	inst_arith_count = 0;

	bit_invariant=0ULL;
	min = max = avg = 0ULL;

	ret_bit_invariant=0ULL;
	ret_min = ret_max = ret_avg = 0ULL;

	useBadALU=0ULL;

	pattern_list.clear();
	pattern_profile.clear();
	m_act_rate=0.0;
	// FAULT_STAT_OUT("----------------------------------- FSIM CONSTR DONE\n");
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

#ifdef DEBUG_FSIM01
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

#ifdef DEBUG_FSIM01
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

#define BIT_FLIPS_ONLY 1

// DETECT WHETHER THE INJECTION RESULTS IN MULTI-BIT FLIPS
// KEEP TRACK OF THE NUMBER OF CORRUPTIONS PER EACH BIT OF THE OUTPUT LATCH
void faultSimulator::numBitFlips(type op1, type op2){

	// op1 corresponds to the golden result, op2 to the infected

	type result, mask, scratch, direction; 
	int count=0;
	int i;

	int tmp_one = 0;
	int tmp_zero = 0;

	//	int tmp_op2[64];
#if !(BIT_FLIPS_ONLY)
	for(i=0; i<64; i++){
		tmp_op2[i]=0;
	}

	type process_op2 = op2;

	mask = (type)1;
	for(i=0; i<64; i++){	
		scratch =  (mask & process_op2)>>i;
		if(scratch == (type)1){
			tmp_op2[i]=1;
		}
		else{
			tmp_op2[i]=0;
		}
		mask = mask<<1;
	}
#endif

	result = op1^op2;

#if LXL_COLLECT_PATTERN
	direction = op2&result;

	pattern_t pat;
	pat.mask = result;
	pat.dir = direction;


	pattern_list[pat]++;

#define DEBUG_PAT 0

	if (DEBUG_PAT && pattern_list[pat]==1) {
		DEBUG_OUT("pattern %llx %llx before increment %lld\n",pat.mask,pat.dir,
				  pattern_list[pat]);
		DEBUG_OUT("plist size %d\n",pattern_list.size());

		for (pattern_map_t::iterator I=getPatternList().begin(), E=getPatternList().end();
			 I!=E; ++I) {
			const pattern_t &this_pattern = I->first;
			uint64 value = I->second;

			DEBUG_OUT("P:%llx\t%llx\t%lld\n", this_pattern.mask, this_pattern.dir, value);
		}

	}

	if (pattern_list.size() > MAX_NUM_PATTERNS) {
		pattern_map_t::iterator min = getPatternList().begin();
		for (pattern_map_t::iterator I=getPatternList().begin(), E=getPatternList().end();
			 I!=E; ++I) {

			const pattern_t &this_pattern = I->first;
			uint64 value = I->second;

			if (value < min->second) {
				min = I;
			}
		}
		pattern_list.erase(min);

		if (DEBUG_PAT)
			DEBUG_OUT("erase pattern %llx %llx before increment %lld\n",min->first.mask,
					  min->first.dir, min->second);

	}
#endif // LXL_COLLECT_PATTERN
 	

	bit_invariant = bit_invariant | result;

	int64 diff = op1 - op2;
	diff = (diff<0)? -diff : diff;
	if (max==0) {
		min = max = avg = diff;
	} else {
		if (diff<min) min =diff;
		if (diff>max) max =diff;
		avg+=diff;
	}
	

	mask = (type)1;
	for(i=0; i<64; i++){	
		scratch = (mask & result)>>i;
		if(scratch == (type)1){
			count++;

#if !(BIT_FLIPS_ONLY)
			indexMeter[i]++;

			if(tmp_op2[i]==0){
				//				tmp_count_zero++;
				tmp_zero++;
			}
			if(tmp_op2[i]==1){
				//				tmp_count_one++;
				tmp_one++;
			}
#endif
		}

		// THIS PART IS TO BE COMMENTED OUT WHEN BINNING IMPLEMENTED
		// OTHERWISE USEFUL TO INCREASE SIMULATION PERFORMANCE
		//		if(count>1)
		//			break;			
		mask = mask<<1;
	}
	if(count > 1){
		multiBitCorruption++;
#ifdef DEBUG_FSIM	
		cout << "MULTI-BIT FLIPS ENCOUNTERED..." << endl;
#endif		
	} 

#if !(BIT_FLIPS_ONLY)
	if(count == 1){

		logic_zero += tmp_zero;
		logic_one += tmp_one;

		// if(tmp_zero > 1){
		// 	FAULT_STAT_OUT("--------------------------- tmp_zero>1");	
		// }
		// if(tmp_one > 1){
		// 	FAULT_STAT_OUT("--------------------------- tmp_one>1");	
		// 	//			HALT_SIMULATION();
		// }
	}
#endif
	//	if(count == 1) {
	//		logic_zero = tmp_count_zero;
	//		logic_one = tmp_count_one;
	//	}

	// if(count == 0){
	// 	FAULT_STAT_OUT("--------------------------- ERROR: %llu vs %llu\n", op1, op2);	
	// 	//		HALT_SIMULATION();
	// }

	switch(count){
		case 1:
			multiBit1++;
			break;
		case 2:
			multiBit2++;
			break;

		case 3:
			multiBit3++;
			break;
		case 4:
			multiBit4++;
			break;	
		case 5:
			multiBit5++;
			break;
		default:
			//			if(count!=0){
			multiBit5B++;
			//			}
			break;
	}

	// This function gets only called when there is a mismatch btw the results of the fault injection experiment
	// and the golden run
	numCorrupt++;

	// FAULT_STAT_OUT("--------------------------- FAULT-SIM: numBitFlips done \n");
}

// DETECT WHETHER THE INJECTION RESULTS IN MULTI-BIT FLIPS
// KEEP TRACK OF THE NUMBER OF CORRUPTIONS PER EACH BIT OF THE OUTPUT LATCH
void faultSimulator::numBitFlipsRetire(type op1, type op2){

	type result, mask, scratch; 
	int count=0;
	int i;

#if !(BIT_FLIPS_ONLY)

	//	int tmp_op2[64];
	type process_op2 = op2;

	for(i=0; i<64; i++){
		//		FAULT_STAT_OUT("--------------------------- initializing tmp_op2[i]\n");
		tmp_op2[i]=0;
		//		FAULT_STAT_OUT("--------------------------- ERROR: %d\n",tmp_op2[i]);			
	}

	int tmp_one = 0;
	int tmp_zero = 0;

	mask = (type)1;
	for(i=0; i<64; i++){	
		scratch =  (mask & process_op2)>>i;
		if(scratch == (type)1){
			tmp_op2[i]=1;
		}
		else{
			tmp_op2[i]=0;
		}

		mask = mask<<1;
	}
#endif

	result = op1^op2;

	ret_bit_invariant = ret_bit_invariant | result;

	int64 diff = op1 - op2;
	diff = (diff<0)? -diff : diff;
	if (ret_max==0) {
		ret_min = ret_max = ret_avg = diff;
	} else {
		if (diff<ret_min) ret_min =diff;
		if (diff>ret_max) ret_max =diff;
		ret_avg+=diff;
	}

	mask = (type)1;
	for(i=0; i<64; i++){	
		scratch = (mask & result)>>i;
		if(scratch == (type)1){
			count++;

#if !(BIT_FLIPS_ONLY)
			indexMeterRetire[i]++;

			if(tmp_op2[i]==0){
				//				tmp_count_zero++;
				tmp_zero++;
			}
			if(tmp_op2[i]==1){
				//				tmp_count_one++;
				tmp_one++;
			}
#endif
		}

		// THIS PART IS TO BE COMMENTED OUT WHEN BINNING IMPLEMENTED
		// OTHERWISE USEFUL TO INCREASE SIMULATION PERFORMANCE
		//		if(count>1)
		//			break;

		mask = mask<<1;
	}
	if(count > 1){
		multiBitCorruptionRetire++;

#ifdef DEBUG_FSIM	
		cout << "MULTI-BIT FLIPS ENCOUNTERED..." << endl;
#endif		
	}

#if !(BIT_FLIPS_ONLY)
	if(count == 1){

		logic_zero_retire += tmp_zero;
		logic_one_retire += tmp_one;

		// if(tmp_zero > 1){
		// 	FAULT_STAT_OUT("--------------------------- tmp_zero>1");	
		// }
		// if(tmp_one > 1){
		// 	FAULT_STAT_OUT("--------------------------- tmp_one>1");	
		// 	//			HALT_SIMULATION();
		// }
	}
#endif
	// if(count == 0){
	// 	FAULT_STAT_OUT("--------------------------- ERROR (retire): %llu vs %llu\n", op1, op2);	
	// 	//		HALT_SIMULATION();
	// }

	switch(count){
		case 1:
			multiBit1Retire++;
			break;
		case 2:
			multiBit2Retire++;
			break;

		case 3:
			multiBit3Retire++;
			break;
		case 4:
			multiBit4Retire++;
			break;	
		case 5:
			multiBit5Retire++;
			break;
		default:
			//			if(count!=0){
			multiBit5BRetire++;
			//			}
			break;
	}

	// This function gets only called when there is a mismatch btw the results of the fault injection experiment
	// and the golden run
	//	if(count!=0){
	numCorruptRetire++;
	//	}

	// BUG: If this function is called, there is a mismatch btw the results of both simulators
	// So, the count!=0 checks above should be redundant (which is not the case)
}

// The operators are to be send explicitly, and then to be encapsulated within  the function
// Different versions of the same function with different number of input arguments can be overwirtten
bool faultSimulator::checkLUT(type op1, type op2, type &result){

	operandKey testKey;
	fSimLUT::iterator mit;

	testKey.insert(op1);
	testKey.insert(op2);

	// check the mem table for the result
	if(!fSimMem.empty()){
		mit = fSimMem.find(testKey);
		if(mit!=fSimMem.end()){
			LUTReference++;
			result = (*mit).second;
			return true;
		}
	}
	return false;
}

void faultSimulator::lluToBinStr(char *s, type val, int width){
	type scratch;
	type mask;
	int i;

	mask = (type)1;	
	for(i = 0; i<width-1; i++){
		scratch = (mask & val)>>i;
		if(scratch == (type)1){
			//op1Str[i] = (int)1;
			s[-i+width-2] = '1';
		}
		else{
			//op1Str[i] = (int)0;
			s[-i+width-2] = '0';
		}
		mask = mask<<1;
	}
	s[width-1] = '\0';
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

// This function can be overridden with versions of various number of arguments
void faultSimulator::insertToLUT(type op1, type op2, type &inj_result){

	// place the new result into the table
	pair<operandKey, type> newEntry;
	fSimLUT::iterator mit;

	operandKey testKey;

	testKey.insert(op1);
	testKey.insert(op2);

	newEntry.first = testKey;
	newEntry.second = inj_result;

	if( fSimMem.size() == LUT_LIMIT ){
		// FIXME place the entry into the beginning - since stl maps are kept sorted in mem,
		// this does not necessarily correspond to the element that was first placed!
		mit =  fSimMem.begin();
		fSimMem.erase(mit);
	}	
	fSimMem.insert(newEntry);
}

type faultSimulator::faultSimulate(type op1, type op2){

	operandKey testKey;
	fSimLUT::iterator mit;

#ifdef DEBUG_FSIM
	FAULT_STAT_OUT("--------------------------- %d elements in the LUT.\n", fSimMem.size());
	//	cout << " --------------------------- " << fSimMem.size() << " elements in the LUT." << endl;
#endif

	testKey.insert(op1);
	testKey.insert(op2);

	// check the mem table for the result
	if(!fSimMem.empty()){
		mit = fSimMem.find(testKey);
		if(mit!=fSimMem.end()){	
			LUTReference++;
#ifdef DEBUG_FSIM
			FAULT_STAT_OUT("--------------------------- When evaluating %llu + % llu, result read from LUT\n", op1, op2);
			FAULT_STAT_OUT("---------------------------------------------- LUT referenced %d times so far\n", LUTReference);
			//	cout << " When evaluating " << op1 << " + " << op2 << ":" << endl;
			//	cout << " --------------------------- RESULT READ FROM LUT." << endl;
#endif
			return (*mit).second;
		}
	}

	// Read input stimulus
	vecRead(op1,op2);
	// initializing (memory allocation) the event table
	initEvents();

#ifdef DEBUG_FSIM01
	cout << " \n************************ FAULT INJECTED - FSIM ********************************* " << endl;
	cout << "faultyGateId " << design.saf.faultSite << endl;
	cout << "faultyValue " << design.saf.faultType << endl;
	cout << "faultyFanoutIndex " << design.saf.faultyFanoutIndex << endl;
	cout << "**************************************************************************\n " << endl;
#endif

	type result;

	// TODO if batch mode out of consideration, clean code

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
	// a clever replacement mechanism to be implemented!

	// place the new result into the table
	/*		pair<operandKey, type> newEntry;
			newEntry.first = testKey;
			newEntry.second = result;

			if( fSimMem.size() == LUT_LIMIT ){
	// place the entry into the beginning - since stl maps are kept sorted in mem,
	// this does not necessarily correspond to the element that was first placed! - FIX
	mit = fSimMem.begin();
	fSimMem.erase(mit);
	}
	fSimMem.insert(newEntry);
	 */

	insertToLUT(op1, op2, result);

#ifdef DEBUG_FSIM01
	fSimLUT::iterator lit;
	operandKey::iterator oit;
	for(lit = fSimMem.begin(); lit != fSimMem.end(); lit++){
		cout << "FOR ";
		for(oit = (*lit).first.begin(); oit != (*lit).first.end(); oit++){
			cout << (*oit) << " ";
		}
		cout << " THE RESULT IS "<< (*lit).second << endl;
	}
#endif
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

#ifdef DEBUG_FSIM01
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
	// check if the randomly generated fanout id is out of bounds:
	if( faultyFanoutId > design.circuit[faultyGateId].fanOutTable.size() ){
		design.saf.faultyFanoutIndex = 	design.circuit[faultyGateId].fanOutTable.size();
	}
	else{
		design.saf.faultyFanoutIndex = 	faultyFanoutId;
	}

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
	//		pickFaultyBehavior();			
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
	//	FAULT_STAT_OUT("\n----------------------------- FAULT INJECTED ----------------------------\n");
	//	FAULT_STAT_OUT("------------------- faultyGateId %d\n", faultyGateId);
	//	FAULT_STAT_OUT("------------------- faultyValue %d\n", faultyValue);
	//	FAULT_STAT_OUT("------------------- faultyFanoutIndex %d\n",  design.saf.faultyFanoutIndex);
	//	FAULT_STAT_OUT("\n--------------------------------------------------------------------------\n");

	//	cout << " \n************************ FAULT INJECTED ********************************* " << endl;
	//	cout << "faultyGateId " << faultyGateId << endl;
	//	cout << "faultyValue " << faultyValue << endl;
	//	cout << "faultyFanoutIndex " << design.saf.faultyFanoutIndex << endl;
	//	cout << "**************************************************************************\n " << endl;
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
	else{
		// insert a 2-input OR gate
		gateId = generateConstantOutputGate(OR, faultyGateId);

		g.gateDescription[ID] = ++gateId;
		g.gateDescription[TYPE] = OR;
	}

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

#define PROFILE_DEBUG 0

void faultSimulator::processFaultModelFile(char* app_name, int act_rate)
{

	char fm_filename[256];
	char token[256], *token_ptr;
	char line[256],*line_ptr;
	FILE *fault_model_fp;
	bool all_done=false, done_pattern;

	m_act_rate = (double) act_rate / 10.0;
	m_act_rate = (((double) rand() / RAND_MAX) * 0.10) + m_act_rate;
	
	line_ptr=&line[0];

	strcpy(fm_filename,app_name);

	if (PROFILE_DEBUG) DEBUG_OUT("opening profile %s\n",fm_filename);
	if (PROFILE_DEBUG) DEBUG_OUT("actual act rate %f\n",m_act_rate);

	fault_model_fp = fopen( fm_filename, "r" ) ;

	if (fault_model_fp) {

		done_pattern=false;
		while (!all_done && !feof(fault_model_fp)) {
			int bit_flip;

			if (!done_pattern) {
				fgets(line,256,fault_model_fp);
				token_ptr = strtok(line_ptr,":");
			}

			if (!strcmp(token_ptr,"P")) {
				double prob;

				token_ptr = strtok(NULL,"\t");
				bit_flip = atoi(token_ptr);
				token_ptr = strtok(NULL,"\t");
				prob = atof(token_ptr);
				if (PROFILE_DEBUG) DEBUG_OUT("read bf %d prob %f\n",bit_flip,prob);
				done_pattern=false;

				bit_flip_prob_t bit_flip_prob;

				bit_flip_prob.num_bit_flips = bit_flip;
				bit_flip_prob.prob = prob;

				pattern_profile.push_back(bit_flip_prob);
				int index = pattern_profile.size()-1;
				double running_prob = 0.0;

				pattern_prob_list_t& pattern_prob = pattern_profile[index].pattern_prob;
				
				while (!feof(fault_model_fp) && !done_pattern) {
					pattern_t pattern;
					double p_prob;

					fgets(line,256,fault_model_fp);
					line_ptr=&line[0];
					token_ptr = NULL;
					token_ptr = strtok(line_ptr,":\t");
					if (!token_ptr) {
						done_pattern=true;
						all_done = true;
					} else {
						if (strcmp(token_ptr,"P")) {
							sscanf(token_ptr,"%llx",&(pattern.mask));
							token_ptr = strtok(NULL,":\t");
							if (!token_ptr) {
								all_done=true;
								done_pattern=true;
								break;
							}
#if LXL_PROB_DIR_FM
							sscanf(token_ptr,"%llx",&(pattern.dir));
							token_ptr = strtok(NULL,":\t");
							if (!token_ptr) {
								all_done=true;
								done_pattern=true;
								break;
							}
#endif
							p_prob = atof(token_ptr);
							if (PROFILE_DEBUG) {
								DEBUG_OUT("pattern %llx dir %llx prob %f\n",pattern.mask, pattern.dir,p_prob);
							}
							running_prob += p_prob;

							pattern_prob_t p_prob_entry;

							p_prob_entry.pattern = pattern;
							p_prob_entry.prob = running_prob;
							pattern_prob.push_back(p_prob_entry);
						} else {
							done_pattern=true;
						}
					}
				}
			} else {
				exit(1);
			}
		}
	} else {
		DEBUG_OUT("Null profile file pointer\n");
		exit(1);
	}

 	//printProfile();

// 	for (int i=0;i<10;i++)
// 		genPattern();

// 	exit(1);
}

void faultSimulator::printProfile()
{

	int size = pattern_profile.size();

	for (int i =0; i<size;i++) {
		DEBUG_OUT("BF %d\t%f\n",pattern_profile[i].num_bit_flips,pattern_profile[i].prob);

		int p_size = pattern_profile[i].pattern_prob.size();
		for (int j=0;j<p_size;j++) {
			DEBUG_OUT("P:%llx\t%f\n",pattern_profile[i].pattern_prob[j].pattern,
					  pattern_profile[i].pattern_prob[j].prob);
		}
	}

}

#define DEBUG_GEN_PATTERN 0
pattern_t faultSimulator::genPattern()
{

	int bf_sel_int = rand();
	double bf_sel = (double) bf_sel_int / RAND_MAX;

	if (DEBUG_GEN_PATTERN) {
		DEBUG_OUT("rand num %f\n",bf_sel);
		//assert(0);
	}

	double running_bf_prob = 0.0;
	int target_bf=0;
	int target_index=0;
	// uint64 target_pattern=0ULL;
	pattern_t target_pattern ;
	int total_bf = pattern_profile.size();

// 	if (bf_sel < (m_act_rate)) {
// 		return target_pattern;
// 	}
	
	bf_sel_int = rand();
	bf_sel = (double) bf_sel_int / RAND_MAX;

	for (int i=0;i<total_bf;i++) {
		running_bf_prob+=pattern_profile[i].prob;
		if ((bf_sel<=running_bf_prob) || (i==total_bf-1)) {
			target_bf = pattern_profile[i].num_bit_flips;
			target_index=i;
			break;
		}
	}
	if (!target_bf) return target_pattern;
	if (DEBUG_GEN_PATTERN) DEBUG_OUT("I choose %d bit-flips\n",target_bf);


	bf_sel_int = rand();
	bf_sel = (double) bf_sel_int / RAND_MAX;
	
	if (DEBUG_GEN_PATTERN) DEBUG_OUT("rand num %f\n",bf_sel);

	pattern_prob_list_t &pattern_prob = pattern_profile[target_index].pattern_prob;
	int num_pattern = pattern_prob.size();

	for (int i=0;i<num_pattern;i++) {
		//running_bf_prob+=pattern_prob[i].prob;
		if ((bf_sel<=pattern_prob[i].prob) || (i==num_pattern-1)) {
			target_pattern = pattern_prob[i].pattern;
			break;
		}
	}
	if (DEBUG_GEN_PATTERN) {
		DEBUG_OUT("I choose pattern %llx - %llx\n",target_pattern.mask, target_pattern.dir);
	}

	return target_pattern;
}

//****************************** class faultSimulator ************************************//

