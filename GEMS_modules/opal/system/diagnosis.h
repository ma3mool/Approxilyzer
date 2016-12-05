#ifndef DIAGNOSIS_H
#define DIAGNOSIS_H

#include <vector>
#include <queue>
#include <set>
//#include "regbox.h"
#include "statici.h"

#define RES_BUS_WIDTH 16
#define NUM_AGU 4
#define NUM_FU_TYPE 10
#define NUM_FU_UNIT 4
#define MAX_INSTR_TRACE_SIZE 10000

//reason for tmr phase
#define LLB_FULL 0
#define SYMPTOM_IN_LOGGING_PHASE 1
#define LOGGING_THRESHOLD_REACHED 2
#define DIAGNOSIS_THRESHOLD 100000 // FIXME - Is this the right threshold to be using?

//LLB parameters
#define LLB_QUEUE_SIZE 1024*5000//*50000
#define LLB_DICTIONARY_SIZE 512*5000//256//128//*50000

//#define LLB_WITH_ADDRESS 1

enum loc_id_t {INVALID,
			   DECODER,
			   ROB,
			   FU,
			   DBUS,
			   LOGICAL_REG,
			   PHYSICAL_REG,
			   AGU,
			   NUM_STRUCT};

// This could possibly be deprecated
typedef struct {
	int id;
    int reg_type;
    int logical_reg;
    int physical_reg;
    ireg_t bad_value;
    ireg_t good_value;
} reg_mismatch_info_t ;

// This is doing binning. I probably should call a subset of them
// uarch info. I should phase this out
typedef struct {
	tick_t cycle;
	uint64 instr;
    la_t VPC;
    half_t opcode;
	bool isMem;
    int fetch_id;
    uint16 win_id;
    int32 exec_unit;
    byte_t exec_unit_id;
    int res_bus;
	int agu;
	vector<reg_mismatch_info_t> mismatch_regs;
} instr_diag_info_t;

typedef struct {
	reg_id_t rid;
	ireg_t value;
} func_reg_info_t;

typedef struct {
	uint64 VPC;
	uint32 iword;
	half_t opcode;
	ireg_t imm;
	func_reg_info_t source[SI_MAX_SOURCE];
	func_reg_info_t dest[SI_MAX_DEST];
} func_inst_info_t;

// Microarchitectural info of a given instruction
typedef struct {
    int fetch_id;
    uint16 win_id;
    int32 exec_unit;
    byte_t exec_unit_id;
    int res_bus;
	int agu;
} uarch_info_t;

typedef struct {
	reg_id_t rid;
	ireg_t value;
	ireg_t good_value;
    bool mismatch;
} register_info_t;

// THIS is the main struct for storing information!!!!!!!!!!!!!!!!!!!
typedef struct{
	uint64 seq_num;
	uint64 VPC;
	half_t opcode;
	bool isMem;
	bool isTrap;
	bool isStuck;
	uint16 stuckSrc;
	ireg_t immediate;
	uint32 iword;

	register_info_t source[SI_MAX_SOURCE];
	register_info_t dest[SI_MAX_DEST];

	reg_id_t s_source[SI_MAX_SOURCE];
	reg_id_t s_dest[SI_MAX_DEST];

	func_inst_info_t good_inst;

	bool mismatch;
	uarch_info_t uarch_info;
} instruction_information_t;

// After diagnosis, this holds the info of the suspected faulty unit
typedef struct {
	loc_id_t mismatch_loc;
	int mismatch_unit;
	int mismatch_unit_num;
} faulty_loc_t;

typedef struct {
	int i_idx;
	bool isSource;
	int reg_idx;
} reg_loc_t;

typedef struct {
	int num_candidates;
	uint freq;
	uint num_mismatch_ins;
	vector<faulty_loc_t> loc_list;
	uint64 first_mis_ins;
	uint64 first_mis_cyc;
} diag_result_t;

class diagnosis_t {

    vector<reg_mismatch_info_t> reg_mismatch_list;
    vector<instr_diag_info_t> instr_diag_list;
	vector<diag_result_t> result_list;

	set<ireg_t> value_watch_list;

	bool   m_has_recovered;
	tick_t m_recovery_cycle;
	uint64 m_recovery_instr;
	unsigned int m_bit_flips;

	bool   m_recorded_mismatch;
	tick_t m_mismatch_cycle;
	uint64 m_mismatch_instr;

	tick_t m_last_mismatch_cycle;
	uint64 m_last_mismatch_instr;

	uint num_rollbacks;


	// Bins for various microarchitectural structures
	int *decoder_bin;
	int *rob_bin;
	int alu_bin[NUM_FU_TYPE][NUM_FU_UNIT];
	int *res_bus_bin;
	int *phy_reg_bin;
	int *log_reg_bin;
	int *agu_bin;

	// Out of all structures, which was used by the most number of
	// mismatch instructions
	int max_mismatch;

	// Ideally, one faulty location should be identified. But
	// sometimes, we have multiple structures that tie
	vector<faulty_loc_t> faulty_loc_list;
	vector<faulty_loc_t> faulty_loc_list2;

	vector<instruction_information_t> instruction_trace;
	int trace_head_ptr;
	int trace_tail_ptr;
	int terminating_instr;

	bool rule_out_datapath;
	bool is_rob_fault;

	int instr_counter;
	bool mismatch_since_last_analysis;
	bool encounter_fatal_trap;
	int last_processed_inst;

	uint num_mismatch_instr;

	bool should_exit;

	int num_candidates;

	int sec_symptom;
	uint64 sec_symptom_inst;
	

	vector<instruction_information_t> mismatch_instructions;
	vector<int>mismatch_index;

	void updateBin(loc_id_t target_bin, int unit, int unit_num);
	bool updateSuspects(reg_loc_t& stuck_reg);

	void processMismatchInstr(int cur_inst);

	int trace_inc(int index) {
		return (index==(MAX_INSTR_TRACE_SIZE-1))?0:(index+1);
	}
	int trace_dec(int index) {
		return (index==0)?(MAX_INSTR_TRACE_SIZE-1):(index-1);
	}

	bool inSameRF(reg_id_t&A, reg_id_t&B) {
		if (A.getRtype()==RID_INT || A.getRtype()==RID_INT_GLOBAL) {
			return (B.getRtype()==RID_INT || B.getRtype()==RID_INT_GLOBAL);
		}
		if (A.getRtype()==RID_SINGLE || A.getRtype()==RID_DOUBLE) {
			return (B.getRtype()==RID_SINGLE || B.getRtype()==RID_DOUBLE);
		}
		return false;
	}

	bool sameFaultyLoc(faulty_loc_t &A, faulty_loc_t&B) {
		return ((A.mismatch_loc==B.mismatch_loc) &&
				(A.mismatch_unit==B.mismatch_unit) &&
				(A.mismatch_unit_num==B.mismatch_unit_num));
	}

	void updateRegLoc(reg_loc_t& reg_loc, int index, bool isSource, int reg_idx) {
		reg_loc.i_idx = index;
		reg_loc.isSource = isSource;
		reg_loc.reg_idx = reg_idx;
	}

	bool samePhysical(reg_loc_t&A, reg_loc_t&B) {
		reg_id_t *AReg,*BReg;

		if (A.isSource) {
			AReg=&instruction_trace[A.i_idx].source[A.reg_idx].rid;
		} else {
			AReg=&instruction_trace[A.i_idx].dest[A.reg_idx].rid;
		}

		if (B.isSource) {
			BReg=&instruction_trace[B.i_idx].source[B.reg_idx].rid;
		} else {
			BReg=&instruction_trace[B.i_idx].dest[B.reg_idx].rid;
		}

		return (AReg->getPhysical()==BReg->getPhysical());
	}

	instruction_information_t& getInstrInfo(reg_loc_t&reg_loc) {
		return instruction_trace[reg_loc.i_idx];
	}

	ireg_t getBadValue(reg_loc_t&reg_loc) {
		if (reg_loc.isSource) 
			return getInstrInfo(reg_loc).source[reg_loc.reg_idx].value;
		else
			return getInstrInfo(reg_loc).dest[reg_loc.reg_idx].value;
	}
	ireg_t getValue(reg_loc_t&reg_loc) {
		if (reg_loc.isSource) {
			if (getInstrInfo(reg_loc).source[reg_loc.reg_idx].mismatch)
				return getInstrInfo(reg_loc).source[reg_loc.reg_idx].good_value;
			else
				return getInstrInfo(reg_loc).source[reg_loc.reg_idx].value;
		} else {
			if (getInstrInfo(reg_loc).dest[reg_loc.reg_idx].mismatch)
				return getInstrInfo(reg_loc).dest[reg_loc.reg_idx].good_value;
			else
				return getInstrInfo(reg_loc).dest[reg_loc.reg_idx].value;
		}
	}

	ireg_t getGoodValue(reg_loc_t&reg_loc) {
		if (reg_loc.isSource) 
			return getInstrInfo(reg_loc).source[reg_loc.reg_idx].good_value;
		else
			return getInstrInfo(reg_loc).dest[reg_loc.reg_idx].good_value;
	}

	bool getMismatch(reg_loc_t&reg_loc) {
		if (reg_loc.isSource) 
			return getInstrInfo(reg_loc).source[reg_loc.reg_idx].mismatch;
		else
			return getInstrInfo(reg_loc).dest[reg_loc.reg_idx].mismatch;
	}

	half_t getLogical(reg_loc_t&reg_loc) {
		if (reg_loc.isSource) 
			return getInstrInfo(reg_loc).source[reg_loc.reg_idx].rid.getFlatRegister();
		else
			return getInstrInfo(reg_loc).dest[reg_loc.reg_idx].rid.getFlatRegister();
	}

	half_t getPhysical(reg_loc_t&reg_loc) {
		if (reg_loc.isSource) 
			return getInstrInfo(reg_loc).source[reg_loc.reg_idx].rid.getPhysical();
		else
			return getInstrInfo(reg_loc).dest[reg_loc.reg_idx].rid.getPhysical();
	}

	reg_id_t&getRID(reg_loc_t&reg_loc) {
		if (reg_loc.isSource) 
			return getInstrInfo(reg_loc).source[reg_loc.reg_idx].rid;
		else
			return getInstrInfo(reg_loc).dest[reg_loc.reg_idx].rid;
	}


	bool isCheckingType(reg_id_t&rid) {
		rid_type_t rtype=rid.getRtype();
		switch(rtype) {
		case RID_INT:                 // integer identifier (windowed)
		case RID_INT_GLOBAL:          // integer identifier (global)
		case RID_SINGLE:              // floating point, single precision
		case RID_DOUBLE:              // floating point, double precision
			return true;
		default:
			return false;
		}
		return false;
	}

	bool isFewerCand() {
		int list_size=faulty_loc_list.size();

		if (list_size==0) return false;
		if (num_candidates==-1) return true;
		if (result_list.size()>0) {
			if (result_list[result_list.size()-1].freq != max_mismatch)
				return true;
		}

		return (list_size!=num_candidates) ;
	}

	void lastLogicalWriter(reg_loc_t &reg_loc, reg_loc_t &ret_loc);
	void nextLogicalWriter(reg_loc_t &reg_loc, reg_loc_t &ret_loc);
	void nextLogicalReader(reg_loc_t &reg_loc, reg_loc_t &ret_loc);
	void nextPhysicalWriter(reg_loc_t &reg_loc, reg_loc_t &ret_loc);
	void lastPhysicalWriter(reg_loc_t &reg_loc, reg_loc_t &ret_loc);
	int  insertFakeInstr(dynamic_inst_t* d);
	void removeFakeInstr();
	bool checkRangeOverlap(reg_loc_t&reg_loc);
	bool isLater(reg_loc_t&A,reg_loc_t&B);
	bool compareInstructions(int cur_inst);
	void analyzeTrace();
	bool findFutureCulprit(reg_loc_t&reg_loc);
	void updateFutureCulprit(reg_loc_t&reg_loc);


 public:
    diagnosis_t();
    ~diagnosis_t();

	void setExcludeDatapath() ;

    void insertInstrInfo(instruction_information_t &info);
	void setRecoveryInfo(tick_t cycle, uint64 instr);
	void setMismatchInfo(tick_t cycle, uint64 instr);
    void printMismatchRegs();
    void printMismatchInstr();
    void printInstrInfo();
	void printDiagnosisResult();
	void analyze();
	vector<faulty_loc_t> &getFaultyList() {return faulty_loc_list;}
	void treatStuckInstr(dynamic_inst_t *d) ;
	void processStuckInstr(int cur_inst);
	bool checkExitCond();
	void checkBlackList(instruction_information_t&info);
	void fatalTrap();
	void symptomAgain(uint64 trap_inst, int traptype) {
		if (sec_symptom == 0 ) {
			sec_symptom=traptype;
			sec_symptom_inst=trap_inst;
		}
	}


	void printListSizes() {
		DEBUG_OUT("reg_mismatch_list %d\n",reg_mismatch_list.size());
		DEBUG_OUT("instr_diag_list %d\n",instr_diag_list.size());
		DEBUG_OUT("faulty_loc_list %d\n",faulty_loc_list.size());
		DEBUG_OUT("instruction_trace %d\n",instruction_trace.size());
	}
	void resetInstrTrace() {
		trace_head_ptr=0;
		trace_tail_ptr=0;
/* 		instruction_trace.clear(); */
		//DEBUG_OUT("trace size is now %d\n",instruction_trace.size());
		//DEBUG_OUT("resetting trace\n");
	}
};


//********************************************************************************
// Multicore Diagnosis 
//********************************************************************************

typedef struct core_state {

  /// The global registers (4 sets)
  ireg_t   global_regs[MAX_GLOBAL_REGS];
  /// The windowed (not including global) registers
  ireg_t   int_regs[MAX_INT_REGS];
  /// The floating point registers in the processor
  freg_t   fp_regs[MAX_FLOAT_REGS];
  /// The control registers in the processor
  ireg_t   ctl_regs[MAX_CTL_REGS];

} core_state_t;

struct load_buffer_cmp {
	bool operator() ( uint64 a, uint64 b) const {
		return (a < b);
	}
};

struct load_info {
	uint64 physical_address;
	vector <uint64> value;

	//uint64 value[MEMOP_MAX_SIZE];
	//uint64 pc;
};
typedef struct load_info load_info_t;

struct func_inst_info {
	ireg_t value[SI_MAX_DEST];
};

typedef struct func_inst_info func_inst_buffer_element_t;

typedef vector<struct load_info> load_buffer_t;	//queue of load_info storing value and address of load instructions

typedef vector <struct func_inst_info> func_inst_buffer_t;

//********************************************************************************
// LLB  

struct llb_queue_entry {
	bool addr_parity;
	int index;
#ifdef LLB_IMPLEMENTATION_2
	uint8 offset;
#endif
#ifdef LLB_IMPLEMENTATION_3
	uint16 index2;
#endif
#ifdef LLB_IMPLEMENTATION_4
	int frequent_index;
#endif
#ifdef LLB_WITH_ADDRESS
	uint64 addr;
#endif
};
typedef struct llb_queue_entry llb_queue_entry_t;

class llb_t {
	
//	vector <uint64> dictionary;
//	vector <struct llb_queue_entry> llb_queue;
	uint64 * dictionary;
	struct llb_queue_entry * llb_queue;

	int llb_queue_size;
	int dictionary_size;
	int front;
	int last;

	int read_pointer;
	
	public:

		llb_t();
		~llb_t();

		bool addLLBEntry(uint64 address, uint64 value);
		bool isLLBFull();
		bool isLLBEmpty();
		bool isLLBHit(uint64, int);
		uint64 readFront();
		uint64 readFront(int);
		int getReadPointer();
		void updateReadPointer(byte_t);
		uint64 readFromPointer(int llb_pointer);
		void pop();
		void resetReadPointer();
		void copyLLB(llb_t* llb);

		void clearDictionary();
		void clearQueue();
		void clearLLB(); 
		void clear() { clearLLB(); }
		int size() { return llb_queue_size; }
	
		int getDictionarySize() { return dictionary_size;}
		int getLLBQueueSize() { return llb_queue_size;}

		uint64 dictionaryElementAt(int i) { return dictionary[i]; }
		llb_queue_entry_t llbQueueElementAt(int i) { return llb_queue[i]; }
		int getLast() {return last;}
		int getFront() {
			//DEBUG_OUT("get llb_queue %d\n", front);
			return front;
		}
#ifdef LLB_IMPLEMENTATION_4
		int getFrequentIndex(uint32 value);
		int getFrequentValue(int freq_index);
#endif
};
	

typedef struct {
	attr_value_t tlb_reg[49];
} tlb_state_t;

typedef struct {
	bool isStore;
	bool isSymptom;
	bool isBranch;
	bool isNormal;
	uint64 pc;
	uint64 address; // for branch this is considered as target
	uint64 value;	
} compare_log_element_t;


class multicore_diagnosis_t {
	private:
		bool diagnosis_started;
		bool logging_step;
		bool tmr_phase;
		bool last_replay;
		bool *trigger_logging_step;
		bool *trigger_tmr_step;
		int numProcs;
		int current_tmr_core;
		llb_t **llb;
		func_inst_buffer_t* func_inst_buffer;

		int *num_instructions;
		int *num_inst_to_run;
		int *current_inst_count;

		bool *checked_core;
		uint64 *squashing_address;

		//statistics
		uint64 *num_squashes;
		uint64 *num_load_instructions;
		uint64 *num_llb_entries;
		int *current_cycle_count;
		uint64 avg_logging_length;
		uint64 *total_logging_instructions;

		int dont_run_core;
		int num_diagnosis_steps;
		int num_logging_phases;
		int m_num_interrupts;

		int symptom_causing_core;
		int symptom_core_logging_phase;
		int dont_continue_core;
		int diagnosis_latency_instr;

		//stats collection for compareLog size
		int *num_branch_instr;
		int *num_store_instr;
		int *num_load_instr;
		int *total_num_branch_instr;
		int *total_num_store_instr;
		int *total_num_load_instr;
		

		int num_symptom_detected;

		bool *missing_addresses_in_core;
		bool first_time_rollback;

		bool tmr_stop_requested;
		int reason_for_tmr_phase;
		int reason_for_logging_phase;
		bool *core_stopped_in_tmr;

		bool is_replay_phase;

		//state to check if the address is mismatched in all the cores running tmr

		//address_not_found_info_t address_not_found_info;

		system_t *m_sys;
		pstate_t **m_state;
		vector<compare_log_element_t> *compare_log;
		vector<compare_log_element_t> *compare_log_logging;
		vector<compare_log_element_t> compare_log_last_replay;
		bool *llb_miss;
		uint64 *llb_miss_instr;
		bool llb_miss_last_replay;
		uint64 llb_miss_last_replay_instr;
		bool *symptom_raised;
		bool *symptom_raised_logging;
		bool symptom_raised_last_replay;

		unsigned short *crc_logging;
		unsigned short crc_last_replay;
		unsigned short *crc_tmr;
		unsigned short crc_last_replay_older;
		unsigned short *crc_tmr_older;
		uint64 *previous_pc;

		conf_object_t **mmu;

		uint64 *start_sequence_number;
		uint64 *start_local_cycle;
		uint64 diagnosis_latency_cycles;
		uint64 expected_diagnosis_latency_cycles;
		uint64 total_logging_latency_cycles;
		uint64 logging_latency_cycles;

		bool *no_physical_address;
		uint64 *no_tmr_after;

		bool stop_diagnosis;

		int faulty_core_suspects;		//current_core and (current_core+1)%numProcs are the suspects
		int suspect_found;
		bool faulty_core_found;
	public:
		tlb_state_t *tlb_state;
		core_state_t *core_state;
		tlb_state_t *tlb_state_main;
		core_state_t *core_state_main;
		tlb_state_t *tlb_state_previous;
		core_state_t *core_state_previous;

		multicore_diagnosis_t(system_t *sys, pstate_t **pstate, uint32 num_cores);
		~multicore_diagnosis_t();

		void startMulticoreDiagnosis();
		bool hasDiagnosisStarted() {return diagnosis_started; }
		void getMMUObjects();


		bool isLoggingPhase() { return logging_step; }
		bool isLastReplay() { return last_replay; }
		bool isReplayPhase() { return is_replay_phase; }
		void setLoggingStep() {
			//DEBUG_OUT("\nLogging Step started\n");
			logging_step = true; 
		}
		void setLastReplay() {
			last_replay = true; 
		}
		void triggerLastReplay();

		void unsetLastReplay(){ last_replay = false; }
		void unsetLoggingStep(){ logging_step = false; }
		void triggerLoggingPhase();
		void loggingStepTriggered(uint32 core_id) {trigger_logging_step[core_id] = false;}
		bool isTriggerLoggingStep(uint32 core_id) {return trigger_logging_step[core_id];}

		void copyBackCheckpoint();


		bool isTMRPhase() { return tmr_phase; }
		void setTMRPhase() { 
			if(CONCISE_VERBOSE)
				DEBUG_OUT("TRM Step started\n");
			 tmr_phase = true; 
		}
		void unsetTMRPhase() { tmr_phase = false; }

		void triggerTMRPhase();
		void TMRStepTriggered(uint32 core_id) {trigger_tmr_step[core_id] = false;}
		bool isTriggerTMRStep(uint32 core_id) {return trigger_tmr_step[core_id];}
		void runMulticoreDiagnosisAlgorithm(int core_id);
		void tmrPolicy();
		void replayPolicy();
		void tmrPhase(int core_id);

		int dont_run_core_id() { return dont_run_core; }


		void loadTLB(int from, int to);
		void loadRegState(int from, int to);
		void loadCheckpoint(int from, int to);
		void getLog(int from, int to);
		void putLog(int from, int to);

		void reportSequenceNumber(int core_id, uint64 pc=-1);
		void notReportingSequenceNumber(int core_id, uint64 pc);
		void collectCompareLog(uint64, uint64, int, bool);
		void collectCompareLog(uint64, uint64, uint64, int);
		void collectCompareLog(uint64, uint64, int);
		void collectCompareLog(uint64, int);
		bool areCompareLogElementsSame(compare_log_element_t, compare_log_element_t);
		void compareCompareLog();
		void compareReplayCompareLog();
		void compareLastReplayCompareLog();
		void clearCompareLog(int i);
		bool isSquashingOnAddress(uint64, int);

		void reportSquashingOnAddress(uint64, int);
		void reportFuncBufferMiss(int);
		void clearSquashingOnAddress(int);

		void reportSquash(int);
		void reportLoadInstruction(int);
		void reportAddressNotFound(int, uint64);
		void printStats();
		int max(int *, int);
		void clearAddressNotFound();
		void reportInterrupt(int);
		bool dont_continue(int);
		void setSymptomCausingCore(int core_id, uint64 trap_type);

		bool isFirstTimeRollback() { return first_time_rollback; }
		void setFirstTimeRollback() { first_time_rollback = true; }
		void clearFirstTimeRollback() { first_time_rollback = false; }

		void setTMRStopRequest() {	tmr_stop_requested = true; }
		void unseTMRStopRequest() {	tmr_stop_requested = false; }

		void freeCompareLog(int);
		void freeFuncInstBuffer(int);
	
		int currentTMRCore() { return current_tmr_core; }
		void resetStartLocalCycle();
		uint64 getCurrentInstNum(int core_id) { return current_inst_count[core_id]; }
		void computeDiagnosisLatency();

		void noPhysicalAddress(int core_id) { 
			int next  = (core_id+1)%numProcs;
			no_physical_address[next] = true; 
			//record the instruction number where the physical address = -1 is seen. 
			//Do not continue beyond this point when doing tmr for this core
			if(no_tmr_after[next] > current_inst_count[core_id])
				no_tmr_after[next] = current_inst_count[core_id];
		}
		void resetNoPhysicalAddress();
		void resetStartSequenceNumber();
		void stopDiagnosis() { stop_diagnosis = true; }
		void stopCore(int);
		int reasonForTMR() { return reason_for_tmr_phase; }
		int getNumLoggingPhases() { return num_logging_phases; }
		bool isCoreStoppedInTMR(int core_id) { return core_stopped_in_tmr[core_id]; }
		uint64 min(uint64 *list, int size);

};

#endif //DIAGNOSIS_H
