// structuralModule.C

#include "structuralModule.h"


//****************************** class logicValue ************************************//
logicValue::logicValue()
{
	setValue("X");
}

logicValue::~logicValue(){}

void logicValue::print()
{
	cout << " VALUE(MSB|LSB) " << mostSignificantBit << leastSignificantBit << endl;
}

bool logicValue::operator!=(logicValue& lv)
{
	if( this->mostSignificantBit != lv.mostSignificantBit ){
		return true;
	}
	if( this->leastSignificantBit != lv.leastSignificantBit ){
		return true;
	}
	return false;
}

/*void logicValue::operator=(logicValue& lv)
{
	this->mostSignificantBit = lv.mostSignificantBit;
	this->leastSignificantBit = lv.leastSignificantBit;
}*/

bool logicValue::is0()
{
	if( (mostSignificantBit)&&!(leastSignificantBit) ){
		return true;
	}
	return false;
}

bool logicValue::is1()
{
	if( !(mostSignificantBit)&&(leastSignificantBit) ){
		return true;
	}
	return false;
}

bool logicValue::isX()
{
	if( !(mostSignificantBit)&&!(leastSignificantBit) ){
		return true;
	}
	return false;
}

bool logicValue::isZ()
{
	if( (mostSignificantBit)&&(leastSignificantBit) ){
		return true;
	}
	return false;
}


void logicValue::setValue(string s)
{
	// PROOFS convention: 
	// X	00
	// 0	10
	// 1	01
	// Z	11

	if(s == "0"){
		mostSignificantBit = true;
		leastSignificantBit = false;
	}
	else if(s == "1"){
		mostSignificantBit = false;
		leastSignificantBit = true;
	}
	else if(s == "X"){
		mostSignificantBit = false;
		leastSignificantBit = false;
	}
	else if(s == "Z"){
		mostSignificantBit = true;
		leastSignificantBit = true;
	}
	else throw "Invalid Logic Value!";

}

void logicValue::invert()
{
	// PROOFS convention: 
	// X	00
	// 0	10
	// 1	01
	// Z	11

	if( (mostSignificantBit)&&(!leastSignificantBit) ){
		mostSignificantBit = false;
		leastSignificantBit = true;
	}
	else if( (!mostSignificantBit)&&(leastSignificantBit) ){
		mostSignificantBit = true;
		leastSignificantBit = false;
	}
	// Z and X values are preserved
}
//****************************** class logicValue ************************************//


//****************************** class fault ************************************//
fault::fault(){}
fault::~fault(){}
//****************************** class fault ************************************//


//****************************** class gate ************************************//
gate::gate()
{		
	fanoutsInOrder.clear();
	fanOutTable.clear();
	c0fanInsInOrder.clear();
	c0fanInTable.clear();
	c1fanInsInOrder.clear();
	c1fanInTable.clear();
	isScheduled = false;
	
	gateData g(8);
	gateDescription = g;
}

gate::~gate(){}

void gate::operator=(gate &g)
{
	this->isRedundant = g.isRedundant;
	this->isScheduled = g.isScheduled;
	this->value = g.value;		
//	this->goodValue = g.goodValue;
	this->gateDescription = g.gateDescription;
	this->fanOutTable = g.fanOutTable;
	this->fanoutsInOrder = g.fanoutsInOrder;
	this->c0fanInTable = g.c0fanInTable;
	this->c0fanInsInOrder = g.c0fanInsInOrder;
	this->c1fanInTable = g.c1fanInTable;
	this->c1fanInsInOrder = g.c1fanInsInOrder;
} 

void gate::printLevel()
{
	cout << "LEVEL OF GATE " << this->gateDescription[ID] << ": " << this->gateDescription[LEVEL] << endl;
}


// this function serves to data consistency
void gate::setGateIOToX()
{
	table::iterator tit;
	for(tit = fanOutTable.begin(); tit != fanOutTable.end(); tit++){
		(*tit).second.setValue("X");
	}
	for(tit = c0fanInTable.begin(); tit != c0fanInTable.end(); tit++){
		(*tit).second.setValue("X");
	}
	for(tit = c1fanInTable.begin(); tit != c1fanInTable.end(); tit++){
		(*tit).second.setValue("X");
	}
}

void gate::printTable(table& t)
{
	table::iterator tit;
	for(tit = t.begin(); tit != t.end(); tit++){
		cout << (*tit).first << ": ";
		(*tit).second.print();
	}
	cout << endl;
}

void gate::updateFanOutEntry(int branchId, logicValue& v)
{
	fanOutTable[branchId] = v;
}

void gate::updateC0FanInEntry(int branchId, logicValue& v)
{
	c0fanInTable[branchId] = v;
}

void gate::updateC1FanInEntry(int branchId, logicValue& v)
{
	c1fanInTable[branchId] = v;
}

bool gate::isANDNOR()
{
	// AND 6, NOR 9
	if( (gateDescription[TYPE] == 6)||(gateDescription[TYPE] == 9) )
		return true;
	else
		return false;
}

bool gate::isORNAND()
{
	// NAND 7, OR 8
	if( (gateDescription[TYPE] == 7)||(gateDescription[TYPE] == 8) )
		return true;
	else
		return false;
}


bool gate::isEquiv()
{
	// XOR 3, XNOR 4
	if( (gateDescription[TYPE] == 3)||(gateDescription[TYPE] == 4) )
		return true;
	else
		return false;
}


// ********* void gate::printGate() ***********************************************
//	Purpose: 
//	Parameters: 
//	Returns: -
// *********************************************************************************
void gate::printGate()
{
	cout << "GATE " << gateDescription[0] << endl;	
	cout << "	TYPE  " << gateDescription[TYPE ] << endl;	
	cout << "	LEVEL " << gateDescription[LEVEL] << endl;	
	cout << "	NUMBER OF INPUTS " << gateDescription[NUMI] << endl;	
	cout << "	NUMBER OF OUTPUTS " << gateDescription[NUMO] << endl;	
//	cout << "	OBSERVABILITY  " << gateDescription[OBS] << endl;	
//	cout << "	0-CONTROLLABILITY " << gateDescription[C0] << endl;	
//	cout << "	1-CONTROLLABILITY " << gateDescription[C1] << endl;	
	value.print();
	cout << "	IS SCHEDULED " << isScheduled << endl;
	cout << "	FAN-OUT LIST ";
	printTable(fanOutTable);
	cout << "	FAN-IN LIST (C0) ";
	printTable(c0fanInTable);
//	cout << "	FAN-IN LIST (C1) ";
//	printTable(c1fanInTable);
}
//****************************** class gate ************************************//


//****************************** class structuralModule ************************************//
structuralModule::structuralModule(){
	maxLevel = -1;
	POList.clear();
	PIList.clear();
}

structuralModule::~structuralModule(){}

void structuralModule::clearDesign()
{
	POList.clear();
	PIList.clear();
	circuit.clear();
	levelCounter.clear();
	// make sure that the fault is preserved here!
}

void structuralModule::printGateData()
{
	circuitTable::iterator it;
	for(it = circuit.begin(); it != circuit.end(); it++){
		(*it).second.printLevel();
	}
}


// ********* void extractGate(string line)***************************************
//	Purpose: Filling in circuit table line by line by reading-in the .lev file
//	Parameters: string line: line of file containing gate data
//	Returns: -
// ******************************************************************************
void structuralModule::extractGate(string line){

	// NOTE gateData is a int vector of size 8: 
	//		node identifier|gate type|level|fan-in count|fan-out count|observability|C0|C1
	char* state;
	char* token;
	string s;
	// the counter for the tokens of the line read
	// the delimiter for the tokens is ""
	int tokenCount = 0;
	int gateId;
	gate gateRead;
	// this corresponds to a gate input or output
	pair<int, logicValue> p;
	p.second.setValue("X");	
	// control variables
	int fanInCount = 0;
	int fanOutCount;
	// converting the line read to a standard C string
	state = const_cast<char*>(line.c_str());
	// tokenizing the line read
	if ((token = strtok(state," ")) != NULL) {
		// the first token represents the node id
		// node ids start from 1, substract 1 not to waste memory:
		gateId = atoi(token)-1;	
		gateRead.gateDescription[ID] = gateId;
		tokenCount++;
		while((token = strtok(NULL, " ")) != NULL) {
			if(tokenCount < 4){		
				// gateRead.gateDescription.push_back( atoi(token) );
				
				if(tokenCount == 1){
					gateRead.gateDescription[TYPE] = atoi(token);
				}
				if(tokenCount == 2){
					gateRead.gateDescription[LEVEL] = atoi(token);
				}
				if(tokenCount == 3){
					gateRead.gateDescription[NUMI] = atoi(token);
				}
			}
			else{
				// reading-in the C0 fan-in list if the number of inputs is not zero
				if(tokenCount < (2*gateRead.gateDescription[NUMI] + 4)){
					if( !(fanInCount < gateRead.gateDescription[NUMI]) ){
						p.first = atoi(token)-1;
						gateRead.c1fanInTable.insert(p);
						gateRead.c1fanInsInOrder.push_back(atoi(token)-1);
						fanInCount++;
					}
					else{
						p.first = atoi(token)-1;
						gateRead.c0fanInTable.insert(p);
						gateRead.c0fanInsInOrder.push_back(atoi(token)-1);
						fanInCount++;
					}
				}// the processing of gate inputs is finished
				else{

					// read-in the number of outputs
					if(tokenCount == (2*gateRead.gateDescription[NUMI] + 4)){
						gateRead.gateDescription[NUMO] = atoi(token);
						fanOutCount =  atoi(token);
					//	gateRead.gateDescription.push_back( fanOutCount );
					} 
					else{ 
						// the line read is not a circuit output
						if(tokenCount < (2*gateRead.gateDescription[NUMI] + fanOutCount + 5)){
							p.first = atoi(token)-1;
							gateRead.fanOutTable.insert(p);

							gateRead.fanoutsInOrder.push_back(atoi(token)-1);
						}
						// read-in the observability value
						else if( tokenCount == (2*gateRead.gateDescription[NUMI] + fanOutCount + 5) ){
						//	gateRead.gateDescription[OBS] = atoi(token);
							gateRead.gateDescription.push_back( atoi(token) );
						}
						else if( tokenCount == (2*gateRead.gateDescription[NUMI] + fanOutCount + 6) ){
							s = token;
							if(s == "O"){
								POList.push_back(gateId);
							}
						}
						// read-in controllability values
						else if( tokenCount == (2*gateRead.gateDescription[NUMI] + fanOutCount + 7) ){
							gateRead.gateDescription[C0] = atoi(token);
						//	gateRead.gateDescription.push_back( atoi(token) );
						}
						else{
							gateRead.gateDescription[C1] = atoi(token);
						//	gateRead.gateDescription.push_back( atoi(token) );
						}
					} // the processing of outputs is finished				
				}// end else: if(tokenCount < (2*readIn[NUMI] + 4))
			} // end else: if(tokenCount < 4)
			// the processing of the token read finished			
			tokenCount++;
		}// end while: reading all tokens in the line		
     }// all tokens of the line read

	// initializing the scheduled flag and logicValue to false
	gateRead.isScheduled = false;
	// PROOFS convention: 
	// X	00
	// 0	10
	// 1	01
	// Z	11
	gateRead.value.mostSignificantBit = false;
	gateRead.value.leastSignificantBit = false;

	// updating PI lists if necessary
	if(gateRead.gateDescription[TYPE] == 1){
		PIList.push_back(gateId);
	}

	// updating maxLevel
	if(gateRead.gateDescription[LEVEL] > maxLevel){
		maxLevel = gateRead.gateDescription[LEVEL];
	}
			
	//******** enhancement to support sequential circuits *****
	// checking whether the gate corresponds to a FF
//	if(gateRead.gateDescription[TYPE] == 5){
//		logicValue v;
//		v.setValue("X");
//		FF[gateId] = v;
//	}	
	//*********************************************************

	// filling in the circuit table
	pair<int,gate> pC;
	pC.first = gateId;
	pC.second = gateRead;
	circuit.insert(pC);
}

// *********void levRead(string levFileName)*******************************
//	Purpose: Reading-in <circuit>.lev file to fill-in the table circuit
//	Parameters: string levFileName: Name of the file containing levelized 
//				circuit description
//	Returns: -
// **************************************************************************
void structuralModule::levRead(string levFileName)
{
	int lineCount = 0;
	// each line of the lev file corresponds to a gate definition
	string line;
	ifstream lev;
	lev.open(levFileName.c_str());
	if(!lev) throw "Cannot open .lev file...";
	maxLevel = -1;
	if(lev.is_open()){
		while(!lev.eof()){
			// the file is processed line by line
			getline(lev,line);
			if(line != ""){
				if(lineCount == 0){
					numGates = atoi(line.c_str());
				}
				// skipping the second line of the lev file: obsolote
				else if(lineCount != 1){
					// processing each line read-in
					extractGate(line);
				}
				lineCount++;
			}
		}
		lev.close();
	}	
	countLevels();
//	numPI = PIList.size();
}

// ********* void structuralModule::printTable(table& t)*****************************************
//	Purpose: 
//	Parameters: 
//	Returns: -
// **********************************************************************************
void structuralModule::printTable(gateData& g)
{
	gateData::iterator git;
	for(git = g.begin(); git != g.end(); git++){
			cout << " " << (*git);
	}
	cout << endl;
}


void structuralModule::setLinesToX()
{
	circuitTable::iterator cit;
	for(cit = circuit.begin(); cit != circuit.end(); cit++){
		(*cit).second.value.setValue("X");
		// set the fanin and fanout table entries of the gate under consideration to X as well
		(*cit).second.setGateIOToX();
	}
}



// ********* void structuralModule::printCircuit() ***********************************
//	Purpose: 
//	Parameters: 
//	Returns: 
// **************************************************************************
void structuralModule::printCircuit()
{
	cout << "CIRCUIT  TABLE: " << endl;
	circuitTable::iterator cit;
	for(cit = circuit.begin(); cit != circuit.end(); cit++){
		(*cit).second.printGate();
	}
	cout << endl << "	PIList ";	
	printTable(PIList);
	cout << "	POList ";
	printTable(POList);
	cout << "	NUM GATES " << numGates-1 << endl;
	cout << "	MAX LEVEL " << maxLevel << endl;
}

void structuralModule::countLevels(void){

	int level;
	gateData count((maxLevel + 1),0);
	// traverse the circuit table to get the number of gates per level
	circuitTable::iterator it;
	for(it = circuit.begin(); it != circuit.end(); it++){
		level = (*it).second.gateDescription[LEVEL];
		count[level]++;
	}
	levelCounter = count;
}

void structuralModule::printLevelCounter(void)
{
	cout << "Contents of the level counter: " << endl;
	// traverse the circuit table to get the number of gates per level
	gateData::iterator it;
	for(it = levelCounter.begin(); it != levelCounter.end(); it++){
		cout << (*it) << endl;
	}
}


void structuralModule::extractModuleName(string circuitFileName)
{
	char* circuitName;
 	char* state = const_cast<char*>(circuitFileName.c_str());
	circuitName     = strtok(state,".lev");
	moduleName = circuitName;
}


void structuralModule::updateValue(int gateId, logicValue v)
{
	circuit[gateId].value = v;
	table::iterator tit;
	if(circuit[gateId].gateDescription[TYPE] != 2){		
		for(tit = circuit[gateId].fanOutTable.begin(); tit != circuit[gateId].fanOutTable.end(); tit++){		
				circuit[(*tit).first].updateC0FanInEntry(gateId, v);
				circuit[(*tit).first].updateC1FanInEntry(gateId, v);
		}
	}
	if(circuit[gateId].gateDescription[TYPE] != 1){
		for(tit = circuit[gateId].c0fanInTable.begin(); tit != circuit[gateId].c0fanInTable.end(); tit++){
				circuit[(*tit).first].updateFanOutEntry(gateId, v);	
		}
	}
}
//****************************** class structuralModule ************************************//

