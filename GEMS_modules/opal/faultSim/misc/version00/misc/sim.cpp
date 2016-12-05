// sim.cpp

// PROTOTYPE FAULT SIMULATOR FOR SAFs
// based on a Zero Delay Event-Driven Logic Simulator

// Input: A Levelized Circuit Description: <circuit>.lev

// TODO *********************************************************************************************** //
// - when done, the parent kills its child (see for optimization), maybe not necessary
// - because of the fault injection mechanism, the design should be read from the file repeatedly
// come up with a routine to rewind 
// - communication over 2 buffers
// - can part of a container be defined as static (exclude gates added fro fault injection)?
// - memoization
// **************************************************************************************************** //

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "module.h"
#include "faultSimulate.h"

//FIX
#define SIGNAL_NUM 10 
#define PARENT_SLEEP_TIME 1
#define INVOKE 3
#define NUM_KEYS 9

typedef u_int64_t data_type;
int KEY[NUM_KEYS];

module design;
faultSimulator fSim;

// catch signal SIGNUM
void signal_handler(int signal_number) {}

void sem_signal(int semid, int val)
{
    struct sembuf semaphor;

    semaphor.sem_num=0;
    semaphor.sem_op=val;
    semaphor.sem_flg=1;
    semop(semid, &semaphor,1);
}


void sem_wait(int semid, int val)
{
    struct sembuf semaphor;
    semaphor.sem_num=0;
    semaphor.sem_op=(-1*val);
    semaphor.sem_flg=1;
    semop(semid, &semaphor,1);
}

void generateKeys()
{
	for(int i = 0; i<NUM_KEYS; i++){
		KEY[i] = (int)ftok("/home/adve_s/rkarpu2/Desktop/adder/prototype/simulator/work", i+3);
	}
}

int main (void)
{
    int f;
    
    int fullLH;
    int emptyLH;
    int mutexLH;
    int fullHL;
    int emptyHL;
    int mutexHL;

    int done;
    
    int bufferIdLH;
    data_type* bufferLH;
    data_type dataLH;

    int bufferIdHL;
    data_type* bufferHL;
    data_type dataHL;
    
    void (*sig_ptr)(int);

    try{
    	sig_ptr = &signal_handler;
 	signal(SIGNAL_NUM, sig_ptr);
	
	f = fork();
	if( f == -1) throw "Fork failed.\n";

	// generating key
	generateKeys();

	// TEST
//	for(int test = 0; test < NUM_KEYS; test++){
//		cout << KEY[test] << " ";
//		cout << endl;
//	}
  
	if(f==0) /* Child process: low level simulator */
    	{
		data_type result, op1, op2;
//		bool first_call = true;
	    
		pause();

		fullLH = semget(KEY[1], 1, 0);
     	  	emptyLH = semget(KEY[2], 1, 0);
     	 	mutexLH = semget(KEY[3], 1, 0);
		done = semget(KEY[0], 1, 0);

		bufferIdLH = shmget (KEY[8], sizeof(data_type), 0);
		bufferLH = (data_type*) shmat (bufferIdLH, 0, 0);

		fullHL = semget(KEY[4], 1, 0);
     	  	emptyHL = semget(KEY[5], 1, 0);
     	 	mutexHL = semget(KEY[6], 1, 0);

		bufferIdHL = shmget (KEY[7], sizeof(data_type), 0);
		bufferHL = (data_type*) shmat (bufferIdHL, 0, 0);
	
		printf("Child awake...\n");
	        
   		for(int p = 0; p < INVOKE; p++)
//		while(1)
		{
			// wait for the parent to send an operand
			sem_wait(fullHL, 1);
			sem_wait(mutexHL, 1);
			dataHL = *bufferHL;
			printf("Child: Read operand 1 as %llu\n", dataHL);

			op1 = dataHL;
			sem_signal(mutexHL, 1);
			sem_signal(emptyHL, 1);

			sem_wait(fullHL, 1);
			sem_wait(mutexHL, 1);
			dataHL = *bufferHL;
			printf("Child: Read operand 2 as %llu\n", dataHL);

			op2 = dataHL;
			sem_signal(mutexHL, 1);
			sem_signal(emptyHL, 1);
		
// ********************* invoke low level simulator ******************************************* //
//		if(first_call){ // first invocation of the low level simulator
//			first_call = false;
		
			// read design into memory:
			// read <circuit>.lev file 
			design.levRead("RAC.lev");
			// extracting the module name - 
//    			design.extractModuleName("RAC.lev");
			// filling-in the table: level counter
			design.countLevels();
			// determine the fault site and stuck at value
			fSim.injectRandomFault();
			// Read input stimulus
			fSim.vecRead(op1,op2);
			// initializing (memory allocation) the event table
			fSim.initEvents();
//		}
//		else{
			// Read input stimulus
//			fSim.vecRead(op1,op2);
			// initializing (memory allocation) the event table
//			fSim.initEvents();
//		}
			// TEST
//			design.printCircuit();
//			cout << "Child: FAULT SIM INITIALIZATION DONE!" << endl;

			// TEST
			cout << "fSim.numInvoke" << fSim.numInvoke << endl;
			result = fSim.faultSimulate();

			//patch
			fSim.clearSim();
			design.clearDesign();

			// TEST
//			design.printCircuit();
// ************************************************************************************************* //

			// sending result
			sem_wait(emptyLH, 1);
			sem_wait(mutexLH, 1);
			dataLH = result;
			*bufferLH = dataLH;
			printf("Child: result sent as %llu...\n", result);
			sem_signal(mutexLH, 1);
			sem_signal(fullLH, 1);

			sleep(PARENT_SLEEP_TIME);
		}
		shmdt(bufferHL);
		shmdt(bufferLH);
		sem_signal(done, 1);
		exit(0);
    	}
    	else /* Parent - high level simulator */
    	{
    		data_type op1, op2;
		    
		fullHL = semget(KEY[4], 1, 0700|IPC_CREAT);
        	semctl(fullHL, 0, SETVAL, 0);
		emptyHL = semget(KEY[5], 1, 0700|IPC_CREAT);
       		semctl(emptyHL, 0, SETVAL, 1);
		mutexHL = semget(KEY[6], 1, 0700|IPC_CREAT);
        	semctl(mutexHL, 0, SETVAL, 1);

		fullLH = semget(KEY[1], 1, 0700|IPC_CREAT);
        	semctl(fullLH, 0, SETVAL, 0);
		emptyLH = semget(KEY[2], 1, 0700|IPC_CREAT);
       		semctl(emptyLH, 0, SETVAL, 1);
		mutexLH = semget(KEY[3], 1, 0700|IPC_CREAT);
        	semctl(mutexLH, 0, SETVAL, 1);

		done = semget(KEY[0], 1, 0700|IPC_CREAT);
        	semctl(done, 0, SETVAL, 0);
	
    		/* shared mem space for buffer */
    		bufferIdLH = shmget (KEY[8], sizeof(data_type), 0700|IPC_CREAT); 
    		bufferLH = (data_type*) shmat (bufferIdLH, 0, 0);
		bufferIdHL = shmget (KEY[7], sizeof(data_type), 0700|IPC_CREAT); 
    		bufferHL = (data_type*) shmat (bufferIdHL, 0, 0);
    	
		/*DEBUG*/
		printf("\nProducer done with prep...\n");
		
		// notify the child:
		kill(f,SIGNAL_NUM);
		sleep(PARENT_SLEEP_TIME);
	
		for(int c = 0; c < INVOKE; c++)
//		while(1)
    		{
//			if(c%2){
			op1 = (u_int64_t)1359;
			op2 = (u_int64_t)89786;
//			}
//			else{
			op1 = (u_int64_t)8;
			op2 = (u_int64_t)10;
//			}
			sem_wait(emptyHL, 1);
			sem_wait(mutexHL, 1);
			dataHL = op1;
			*bufferHL = dataHL;
			printf("Parent: op1 sent...\n");
			sem_signal(mutexHL, 1);
			sem_signal(fullHL, 1);
	
			sem_wait(emptyHL, 1);
			sem_wait(mutexHL, 1);
			dataHL = op2;
			*bufferHL = dataHL;
			printf("Parent: op2 sent...\n");
			sem_signal(mutexHL, 1);
			sem_signal(fullHL, 1);

			// reading the result
			sem_wait(fullLH, 1);
			sem_wait(mutexLH, 1);
			dataLH = *bufferLH;
			printf("Parent: Read result as %llu\n", dataLH);
			sem_signal(mutexLH, 1);
			sem_signal(emptyLH, 1);
		}
		shmdt(bufferLH);
		shmdt(bufferHL);

		sem_wait(done, 1);

		shmctl(bufferIdLH,IPC_RMID,0);
		semctl(fullLH,0,IPC_RMID,0);
        	semctl(emptyLH,0,IPC_RMID,0);
		semctl(mutexLH,0,IPC_RMID,0);
	
		shmctl(bufferIdHL,IPC_RMID,0);
		semctl(fullHL,0,IPC_RMID,0);
        	semctl(emptyHL,0,IPC_RMID,0);
		semctl(mutexHL,0,IPC_RMID,0);

		semctl(done,0,IPC_RMID,0);
   
		exit(0);
    	}
    }	
    catch (string msg) {
	cout << "ERROR :" << msg << endl;
	exit(1);
    }

 }

