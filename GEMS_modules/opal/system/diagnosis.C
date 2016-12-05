#include "hfa.h"
#include "regfile.h"
#include "arf.h"
#include "opcode.h"
#include "dynamic.h"
#include "system.h"

#include "interface.h"
#include "pstate.h"
#include "diagnosis.h"
#include "crc16.h"

#define LXL_DIAG_DEBUG 0

#define MAX_CANDIDATES 1
#define MAX_NUM_MISMATCH 2
#define MAX_TERM_MISMATCH 5
#define MAX_TERM_CAND 2

#define ANALYSIS_INTERVAL 1000

#define FORWARD_INSTR_CNT 2000
#define LXL_BEYOND_FATAL_TRAP 0
#define MULTICORE_DIAGNOSIS_DEBUG 0

#define INTERRUPT_IN_EXEC 0

#define NUM_INSTR_TO_COMPARE 1
#define NUM_CYCLES_TO_COMPARE 2000
#define THRESHOLD_NUM_INTERRUPTS 20
#define LOGGING_THRESHOLD 1000000
#define DIAGNOSIS_GIVEUP_THRESHOLD 20000000


#define SIVA_LOAD_BUFFER_DEBUG 0

#define CRC_CHECK

diagnosis_t::diagnosis_t() 
{
	reg_mismatch_list.clear();
	instr_diag_list.clear();
	value_watch_list.clear();

	m_has_recovered=false;
	m_recovery_cycle=0;
	m_recovery_instr=0;

	m_recorded_mismatch=false;
	m_mismatch_cycle=0;
	m_mismatch_instr=0;

	m_last_mismatch_cycle=0;
	m_last_mismatch_instr=0;

	num_rollbacks=0;

	decoder_bin = new int[MAX_FETCH];
	rob_bin = new int[IWINDOW_ROB_SIZE];
	res_bus_bin = new int[RES_BUS_WIDTH];
	phy_reg_bin = new int[CONFIG_IREG_PHYSICAL];
	log_reg_bin = new int[CONFIG_IREG_LOGICAL];
	agu_bin = new int[NUM_AGU];

	int i;
	for (i=0;i<MAX_FETCH; i++) {
		decoder_bin[i]=0;
	}
	for (i=0;i<IWINDOW_ROB_SIZE; i++) {
		rob_bin[i]=0;
	}
	for (i=0;i<NUM_FU_TYPE; i++) {
		for (int j=0; j<NUM_FU_UNIT; j++) {
			alu_bin[i][j]=0;
		}
	}
	for (i=0;i<RES_BUS_WIDTH;i++) {
		res_bus_bin[i]=0;
	}
	for (i=0;i<NUM_AGU;i++) {
		agu_bin[i]=0;
	}


	for (i=0;i<CONFIG_IREG_PHYSICAL;i++) {
		phy_reg_bin[i]=0;
	}

	for (i=0;i<CONFIG_IREG_LOGICAL;i++) {
		log_reg_bin[i]=0;
	}

	max_mismatch=0;
	faulty_loc_list.clear();
	faulty_loc_list2.clear();
	mismatch_instructions.clear();
	mismatch_index.clear();

	//instruction_trace.clear();
	instruction_trace=vector<instruction_information_t>(MAX_INSTR_TRACE_SIZE);
	//printf("int trace %d\n",instruction_trace.size());
	trace_head_ptr = 0;
	trace_tail_ptr = 0;
	rule_out_datapath=false;
	is_rob_fault=false;
	instr_counter=0;
	mismatch_since_last_analysis=false;
	encounter_fatal_trap=false;
	last_processed_inst=-1;
	terminating_instr=-1;
	num_mismatch_instr=0;
	should_exit=false;
	num_candidates=-1;
	sec_symptom=0;
}

diagnosis_t::~diagnosis_t()
{
	reg_mismatch_list.clear();
	instr_diag_list.clear();
}

void diagnosis_t::insertInstrInfo(instruction_information_t &info)
{
	int cur_idx;

	instruction_trace[trace_head_ptr]=info;

	cur_idx=trace_head_ptr;

	if (trace_inc(trace_head_ptr)==trace_tail_ptr) {
		trace_head_ptr=trace_tail_ptr;
		trace_tail_ptr=trace_inc(trace_tail_ptr);
	} else {
		trace_head_ptr=trace_inc(trace_head_ptr);
	}

	if (0&&LXL_DIAG_DEBUG) {
		if ((info.VPC != info.good_inst.VPC)) {
			DEBUG_OUT("t %llx f %llx\n",info.VPC,info.good_inst.VPC);
		}
	}

	if (info.mismatch||info.isTrap||info.isStuck||!compareInstructions(cur_idx)) {
		mismatch_index.push_back(cur_idx);
		setMismatchInfo(0,info.seq_num);
		if (LXL_DIAG_DEBUG) {
			DEBUG_OUT("inserting %d : it %d ",cur_idx,info.isTrap);
			for (int i=0;i<SI_MAX_DEST; i++) {
				DEBUG_OUT("d%d m%d\t",i,info.dest[i].mismatch);
			}
			DEBUG_OUT("\n");
		}
	}

// 	if (!compareInstructions(cur_idx)) {
// 		DEBUG_OUT("instr %llu %s bad instr word\n",instruction_trace[cur_idx].seq_num,
// 				  decode_opcode(instruction_trace[cur_idx].opcode));
// 		DEBUG_OUT("instr mis %d itrp %d ist %d\n",info.mismatch,info.isTrap,info.isStuck);
// 	}

	instr_counter++;

	if (instr_counter>=ANALYSIS_INTERVAL) {
		analyzeTrace();
		instr_counter=0;
	} 

// else if (terminating_instr!=-1 && trace_inc(trace_head_ptr)==trace_tail_ptr) {
// 		analyzeTrace();
// 		instr_counter=0;
// 		//assert(0);
// 		DEBUG_OUT("hit terminating instr\n");
// 		HALT_SIMULATION;
// 	}

}

#define DECODER_DEBUG 1

bool diagnosis_t::compareInstructions(int cur_inst)
{
	instruction_information_t&cur_instr=instruction_trace[cur_inst];
	func_inst_info_t&good_inst=cur_instr.good_inst;

	if (DECODER_DEBUG&&LXL_DIAG_DEBUG) {
		if (cur_instr.VPC!=good_inst.VPC) 
			DEBUG_OUT("t %llx f %llx\n",cur_instr.VPC,good_inst.VPC);
	}

	if (cur_instr.opcode!=good_inst.opcode) {
		if (DECODER_DEBUG&&LXL_DIAG_DEBUG) {
			DEBUG_OUT("bad opcode %s\n",decode_opcode(good_inst.opcode));
			
		}
		return false;
	}
	if (cur_instr.immediate!=good_inst.imm) {
		if (DECODER_DEBUG&&LXL_DIAG_DEBUG) {
			DEBUG_OUT("bad imm\n");
		}
		return false;
	}
	// Logical register id the same?
	for (int i=0; i<SI_MAX_SOURCE; i++) {
		if (cur_instr.s_source[i].getVanilla()!=good_inst.source[i].rid.getVanilla()) {
			if (DECODER_DEBUG&&LXL_DIAG_DEBUG) {
				DEBUG_OUT("bad src id %d @ %lld\n",i,cur_instr.seq_num);
				DEBUG_OUT("cur %d good %d\n",cur_instr.s_source[i].getFlatRegister(),
						  good_inst.source[i].rid.getFlatRegister());
				DEBUG_OUT("type %d %d\n",cur_instr.s_source[i].getRtype(),good_inst.source[i].rid.getRtype());
				DEBUG_OUT("cur w%d:%d good w%d:%d \n",
						  cur_instr.s_source[i].getVanillaState(),
						  cur_instr.s_source[i].getVanilla(),
						  good_inst.source[i].rid.getVanillaState(),
						  good_inst.source[i].rid.getVanilla());


			}
			return false;
		}
	}

	// Destination register id the same?
	for (int i=0; i<SI_MAX_DEST; i++) {
		if (cur_instr.s_dest[i].getVanilla()!=good_inst.dest[i].rid.getVanilla()) {
			if (DECODER_DEBUG&&LXL_DIAG_DEBUG) {
				DEBUG_OUT("bad dst id\n");
				DEBUG_OUT("cur %d good %d\n",cur_instr.s_dest[i].getFlatRegister(),
						  good_inst.dest[i].rid.getFlatRegister());
				DEBUG_OUT("type %d %d\n",cur_instr.s_dest[i].getRtype(),good_inst.dest[i].rid.getRtype());
				DEBUG_OUT("cur w%d:%d good w%d:%d \n",
						  cur_instr.s_dest[i].getVanillaState(),
						  cur_instr.s_dest[i].getVanilla(),
						  good_inst.dest[i].rid.getVanillaState(),
						  good_inst.dest[i].rid.getVanilla());
			}
			return false;
		}
	}

	if (cur_instr.iword!=good_inst.iword) {
		if (DECODER_DEBUG&&LXL_DIAG_DEBUG)
			DEBUG_OUT("mismatched iword 0x%x\n",cur_instr.iword^good_inst.iword);
		return false;
	}

	if (0&&DECODER_DEBUG&&LXL_DIAG_DEBUG) {
		DEBUG_OUT("good instr word\n");
	}
	return true;

}

void diagnosis_t::analyzeTrace() {
#ifdef CHK_SYMPTOM
	return;
#endif

	if (LXL_DIAG_DEBUG) {
		if (last_processed_inst < (mismatch_index.size()-1)) {
			DEBUG_OUT("Analyzing trace ... %d mismatches\n",mismatch_index.size());
			DEBUG_OUT("last_processed %d\n",last_processed_inst);
		}
	}

	if (should_exit) return;

	int num_mismatches=mismatch_index.size();
	
	for (int i=last_processed_inst+1;i<num_mismatches; i++) {
		if (LXL_DIAG_DEBUG) DEBUG_OUT("processing index [%d]%d\n",i,mismatch_index[i]);
		processMismatchInstr(mismatch_index[i]);
		last_processed_inst=i;

		num_mismatch_instr++;

		//LXL: this used to be outside, and break was commented out
		if (checkExitCond()) {
			break;
		}
	}
}

void diagnosis_t::setExcludeDatapath() {
	if (!rule_out_datapath) {
		if (LXL_DIAG_DEBUG) DEBUG_OUT("rule out datapath first set, reanalyze\n");

		rule_out_datapath=true;
		last_processed_inst=-1;
		faulty_loc_list.clear();
		result_list.clear();
		max_mismatch=0;
		num_mismatch_instr=0;
		analyzeTrace();
	}
}

// Base on value first, enhance later, I suspect that it should work pretty well
// return true if a future culprit is found
bool diagnosis_t::findFutureCulprit(reg_loc_t&reg_loc) {
	reg_loc_t npw,nlw;

	nextPhysicalWriter(reg_loc,npw);

	if (npw.i_idx!=-1) {
		nextLogicalWriter(reg_loc, nlw);

		if (isLater(nlw,npw)) {
			if (LXL_DIAG_DEBUG) {
				DEBUG_OUT("thp %d cur %d nlw %d npw %d\n",trace_head_ptr,
						  reg_loc.i_idx,nlw.i_idx,npw.i_idx);
				DEBUG_OUT("cur l %d p %d\n",getLogical(reg_loc),getPhysical(reg_loc));
				DEBUG_OUT("nlw l %d p %d\n",getLogical(nlw),getPhysical(nlw));
				DEBUG_OUT("npw l %d p %d\n",getLogical(npw),getPhysical(npw));
				reg_loc_t npw_llw;
				lastLogicalWriter(npw, npw_llw);
				DEBUG_OUT("npw_llw %d\n",npw_llw.i_idx);
			}
			return true;
		} else 
			return false;
			
	}
	if (LXL_DIAG_DEBUG) {
		DEBUG_OUT("no next pw\n");
	}

	return false;

}

void diagnosis_t::updateFutureCulprit(reg_loc_t&reg_loc) {
	reg_loc_t npw;
	nextPhysicalWriter(reg_loc, npw);

	updateBin(LOGICAL_REG,getLogical(npw),-1);
	updateBin(LOGICAL_REG,getLogical(reg_loc),-1);

}


void diagnosis_t::processMismatchInstr(int cur_inst)
{
	
	instruction_information_t&info=instruction_trace[cur_inst];
	if (LXL_DIAG_DEBUG) DEBUG_OUT("Process mismatch instr %s\n",decode_opcode(info.opcode));

	// We encountered a mismatch, we know that the dest reg is
	// corrupted, but by whom?
	bool all_src_correct=true;
	bool not_datapath_fault = false;

	// First step, compares the opcode and instruction info of the
	// instruction word
	if (!compareInstructions(cur_inst)) {
		updateBin(DECODER, info.uarch_info.fetch_id, -1) ;
		return;
	}

	if (info.isStuck) {
		//		assert(0);
		processStuckInstr(cur_inst);
		return;
	}

	// First, check whether the sources are all correct
	for (int i=0;i<SI_MAX_SOURCE;i++) {
		reg_id_t&source=info.source[i].rid;
		if (!source.isZero()&&isCheckingType(source)) {

			if (LXL_DIAG_DEBUG) {
				DEBUG_OUT("check s%d t%d l%d p%d\n",i, source.getRtype(), source.getFlatRegister(), source.getPhysical());
			}
			if (info.source[i].value!=info.good_inst.source[i].value) {

				if (LXL_DIAG_DEBUG) {
					DEBUG_OUT("mis s%d rt %d %llx %llx\n",i, source.getRtype(),
							  info.source[i].value, info.good_inst.source[i].value);
				}
				reg_loc_t current_reg;
				updateRegLoc(current_reg,cur_inst,true,i);
				not_datapath_fault=updateSuspects(current_reg);
				all_src_correct=false;
			}
		}
	}

	if (not_datapath_fault) {
		setExcludeDatapath();
	}
	if (!all_src_correct) return;
	if (LXL_DIAG_DEBUG)DEBUG_OUT("all src correct\n");
	// First, check whether the sources are all correct

	bool mismatch_dest = false;

	// Now, check if there are any instruction before this writing to same register
	for (int i=0;i<SI_MAX_DEST;i++) {
		reg_id_t&dest=info.dest[i].rid;
		mismatch_dest = mismatch_dest||info.dest[i].mismatch;
		if (!dest.isZero() && info.dest[i].mismatch &&
			isCheckingType(dest)) {
			// given the mismatch dest, check range violation and mark
			// corresponding structs

			reg_loc_t cur_writer,last_physical_writer;
			
			if (LXL_DIAG_DEBUG) {
				DEBUG_OUT("mis d%d rt %d %llx %llx\n",i, dest.getRtype(),
						  info.dest[i].value, info.good_inst.dest[i].value);
			}
			updateRegLoc(cur_writer, cur_inst, false, i);

			//LXL:check range should be done to anything with
			//previous p writer, regardless whether previous
			//writer has a reader

			//Confirm overlapped live range
			if (!rule_out_datapath) {

				if (checkRangeOverlap(cur_writer) || findFutureCulprit(cur_writer)) {
					not_datapath_fault=true;
				}
			} else { 
				// rule_out_datapath is true
				if (checkRangeOverlap(cur_writer)) {
					if (LXL_DIAG_DEBUG) DEBUG_OUT("dest range overlap\n");
					lastPhysicalWriter(cur_writer,last_physical_writer);

					updateBin(LOGICAL_REG, getLogical(last_physical_writer), -1);
					updateBin(LOGICAL_REG, getLogical(cur_writer), -1);
				}
				if (findFutureCulprit(cur_writer)) {
					if (LXL_DIAG_DEBUG) 
						DEBUG_OUT("culprit_exist\n");

					updateFutureCulprit(cur_writer);
				}
			}

// 			if (checkRangeOverlap(cur_writer)) {
// 				if (LXL_DIAG_DEBUG) DEBUG_OUT("dest range overlap\n");
// 				lastPhysicalWriter(cur_writer,last_physical_writer);

// 				updateBin(LOGICAL_REG, getLogical(last_physical_writer), -1);
// 				updateBin(LOGICAL_REG, getLogical(cur_writer), -1);
// 				all_src_correct=false;
// 				not_datapath_fault=true;
// 			} else {
// 				bool culprit_exist = findFutureCulprit(cur_writer);
// 				if (LXL_DIAG_DEBUG) 
// 					DEBUG_OUT("culprit_exist %d\n",culprit_exist);
// 				if (culprit_exist)
// 					not_datapath_fault=true;
// 			}
// 			if (rule_out_datapath) {
// 				if (LXL_DIAG_DEBUG) 
// 					DEBUG_OUT("cur writer %d p%d\n",getLogical(cur_writer),getPhysical(cur_writer));
// 				findFutureCulprit(cur_writer);
// 			}
		}
	}

	if (not_datapath_fault) {
		setExcludeDatapath();
	}

	if (LXL_DIAG_DEBUG) {
		DEBUG_OUT("!dp %d arc %d it %d md %d\n",rule_out_datapath,all_src_correct,info.isTrap,mismatch_dest);
	}

	if (rule_out_datapath || !all_src_correct || (!info.isTrap&&!mismatch_dest)) return;
	if (LXL_DIAG_DEBUG) DEBUG_OUT("updating datapath\n");

	// If all the sources are correct, somewhere in the data path is
	// wrong

	uarch_info_t &u_info = info.uarch_info;
			
	updateBin(FU, u_info.exec_unit, u_info.exec_unit_id) ;
	updateBin(DBUS, u_info.res_bus, -1) ;
	if (info.isMem) {
		updateBin(AGU, u_info.agu, -1);
	}

	for (int j=0;j<SI_MAX_DEST; j++) {
		if (info.dest[j].mismatch) {
			reg_id_t&rid=info.dest[j].rid;
			if (rid.getRtype()==RID_INT || rid.getRtype()==RID_INT_GLOBAL) {
				updateBin(PHYSICAL_REG,rid.getPhysical(),-1);
			}
		}
	}
}

void diagnosis_t::setRecoveryInfo(tick_t cycle, uint64 instr) 
{

	num_rollbacks++;
	if (!m_has_recovered) {
		m_recovery_cycle=cycle;
		m_recovery_instr=instr;
		m_has_recovered=true;
		// m_bit_flips = debugio_t::getTotalInj();
	}
}

void diagnosis_t::setMismatchInfo(tick_t cycle, uint64 instr) 
{

	if (!m_recorded_mismatch) {
		m_mismatch_cycle=cycle;
		m_mismatch_instr=instr;
		m_recorded_mismatch=true;
	}
	m_last_mismatch_cycle=cycle;
	m_last_mismatch_instr=instr;
}

void diagnosis_t::printMismatchRegs()
{
	FAULT_OUT("\n==========Mismatch registers==========\n");
	for (int i=0; i<reg_mismatch_list.size(); i++) {
		reg_mismatch_info_t& info=reg_mismatch_list[i];
		FAULT_OUT("t%d l%d\tp%d\tbv%llx\tgv%llx\n",info.reg_type, info.logical_reg,
				  info.physical_reg, info.bad_value, info.good_value);
	}
}

void diagnosis_t::printInstrInfo()
{
	FAULT_OUT("\n==========Instruction Trace==========\n");
	for (int i=0; i<instruction_trace.size(); i++) {
		instruction_information_t &info=instruction_trace[i];

		FAULT_OUT("%d:",info.seq_num);
		FAULT_OUT("%s\ts",decode_opcode(info.opcode));
		for (int j=0;j<SI_MAX_SOURCE;j++) {
			register_info_t &rinfo =info.source[j];
			if (!rinfo.rid.isZero()) {
				FAULT_OUT("%d:%llx\t",rinfo.rid.getPhysical(),
						  rinfo.value);
			}
		}
		FAULT_OUT("d");
		for (int j=0;j<SI_MAX_DEST;j++) {
			register_info_t &rinfo =info.dest[j];
			if (!rinfo.rid.isZero()) {
				FAULT_OUT("%d:%llx\t",rinfo.rid.getPhysical(),
						  rinfo.value);
			}
		}
		FAULT_OUT("\n");
	}
}

void diagnosis_t::printMismatchInstr()
{
#if 1
	FAULT_OUT("\n==========Mismatch instr==========\n");
	for (int i=0; i<mismatch_index.size(); i++) {
		instruction_information_t &info=instruction_trace[mismatch_index[i]];
		uarch_info_t&u_info=info.uarch_info;

		FAULT_OUT("sn:%lld\t%s\tfid%d\twid%d\teu%d.%d\tbus%d\tagu%d\n",
				  info.seq_num, decode_opcode(info.opcode),
				  u_info.fetch_id, u_info.win_id, u_info.exec_unit,
				  u_info.exec_unit_id,u_info.res_bus,u_info.agu);
	}
#else
	// The following is deprecated
	FAULT_OUT("\n==========Mismatch instr==========\n");
	for (int i=0; i<instr_diag_list.size(); i++) {
		instr_diag_info_t &info=instr_diag_list[i];

		FAULT_OUT("pc0x%llx\t%s\tfid%d\twid%d\teu%d.%d\tbus%d\tagu%d\n",
				  info.VPC, decode_opcode(info.opcode),
				  info.fetch_id, info.win_id, info.exec_unit,
				  info.exec_unit_id,info.res_bus,info.agu);

		for (int j=0; j<info.mismatch_regs.size(); j++) {
			reg_mismatch_info_t& r_info=info.mismatch_regs[j];
			FAULT_OUT("t%d l%d\tp%d\tbv%llx\tgv%llx\n",r_info.reg_type, r_info.logical_reg,
					  r_info.physical_reg, r_info.bad_value, r_info.good_value);
		}
	}
#endif
}

void diagnosis_t::printDiagnosisResult()
{
	FAULT_STAT_OUT("Recovery time (cycle:instr) :\t%llu:%llu\n",
				   m_recovery_cycle, m_recovery_instr);
	FAULT_STAT_OUT("Recovery flips :\t%u\n",m_bit_flips);
	FAULT_STAT_OUT("# rollbacks      :\t%d\n",num_rollbacks);
	FAULT_STAT_OUT("First Mismatch time (cycle:instr) :\t%llu:%llu\n",
				   m_mismatch_cycle, m_mismatch_instr);
#ifdef CHK_SYMPTOM
	FAULT_STAT_OUT("Second Symptom :\t%d\n",sec_symptom);
	FAULT_STAT_OUT("Second Symptom Inst :\t%lld\n",sec_symptom_inst);
	return;
#endif

#if 1
	if (result_list.size()>0) {
		diag_result_t &res=result_list[result_list.size()-1];

		FAULT_STAT_OUT("Last Mismatch time (cycle:instr) :\t%llu:%llu\n",
					   res.first_mis_cyc, res.first_mis_ins);
		FAULT_STAT_OUT("# mismatch instr :\t%d\n",res.num_mismatch_ins);
		FAULT_STAT_OUT("Top mismatch freq :\t%d\n",res.freq);
		FAULT_STAT_OUT("# mismatch candidates :\t%d\n",res.num_candidates);
	
		for (int i=0;i<res.loc_list.size();i++) {

			FAULT_STAT_OUT("Top mismatch unit (struct:unit) :\t%d:",res.loc_list[i].mismatch_loc);

			if (res.loc_list[i].mismatch_loc!=FU) {
				FAULT_STAT_OUT("%d\n",res.loc_list[i].mismatch_unit);
			} else {
				FAULT_STAT_OUT("%d.%d\n",res.loc_list[i].mismatch_unit,res.loc_list[i].mismatch_unit_num);
			}
		}
	}

#else // #if 0
	FAULT_STAT_OUT("Last Mismatch time (cycle:instr) :\t%llu:%llu\n",
				   m_last_mismatch_cycle, m_last_mismatch_instr);
	FAULT_STAT_OUT("# mismatch instr :\t%d\n",num_mismatch_instr);//mismatch_index.size());
	FAULT_STAT_OUT("# rollbacks      :\t%d\n",num_rollbacks);
	FAULT_STAT_OUT("Top mismatch freq :\t%d\n",max_mismatch);
	FAULT_STAT_OUT("# mismatch candidates :\t%d\n",faulty_loc_list.size());

	for (int i=0;i<faulty_loc_list.size();i++) {

		FAULT_STAT_OUT("Top mismatch unit (struct:unit) :\t%d:",faulty_loc_list[i].mismatch_loc);

		if (faulty_loc_list[i].mismatch_loc!=FU) {
			FAULT_STAT_OUT("%d\n",faulty_loc_list[i].mismatch_unit);
		} else {
			FAULT_STAT_OUT("%d.%d\n",faulty_loc_list[i].mismatch_unit,faulty_loc_list[i].mismatch_unit_num);
		}
	}

	if (faulty_loc_list2.size()) {
		FAULT_STAT_OUT("# 2nd mismatch candidates :\t%d\n",faulty_loc_list2.size());

		for (int i=0;i<faulty_loc_list2.size();i++) {

			FAULT_STAT_OUT("2nd mismatch unit (struct:unit) :\t%d:",faulty_loc_list2[i].mismatch_loc);

			if (faulty_loc_list2[i].mismatch_loc!=FU) {
				FAULT_STAT_OUT("%d\n",faulty_loc_list2[i].mismatch_unit);
			} else {
				FAULT_STAT_OUT("%d.%d\n",faulty_loc_list2[i].mismatch_unit,faulty_loc_list2[i].mismatch_unit_num);
			}
		}
	}

#endif //end #if 1

	if (LXL_DIAG_DEBUG)
		printMismatchInstr();
}

void diagnosis_t::updateBin(loc_id_t target_bin, int unit, int unit_num) 
{
	int cur_cnt=0;


	if (target_bin==ROB) {
		if (!rule_out_datapath) return;
		is_rob_fault=true;
		cur_cnt = ++rob_bin[unit];
	} else if (target_bin==LOGICAL_REG) {
		if (is_rob_fault || !rule_out_datapath||unit>=CONFIG_IREG_LOGICAL) 
			return;

		cur_cnt = ++log_reg_bin[unit];
	} else if (target_bin==FU) {
		if ((unit<=0) || (unit >=FU_NUM_FU_TYPES) || unit==7 || unit==8)
			return;

		cur_cnt = ++alu_bin[unit][unit_num];
	} else if (target_bin==DECODER) {
		cur_cnt = ++decoder_bin[unit];
	} else if (target_bin==DBUS) {
		if ((unit<0) || (unit>=RES_BUS_WIDTH)) {
			return;
		}
		cur_cnt = ++res_bus_bin[unit];
	} else if (target_bin==PHYSICAL_REG) {
		if (unit>=CONFIG_IREG_PHYSICAL) 
			return;

		cur_cnt = ++phy_reg_bin[unit];
	} else if (target_bin==AGU) {
		cur_cnt = ++agu_bin[unit];
	} else {
		DEBUG_OUT("Error: Unknown Type!!\n");
		return;
	}

	if (LXL_DIAG_DEBUG)
		DEBUG_OUT("update %d unit %d unit_num %d\n",target_bin, unit, unit_num);


	if (cur_cnt>max_mismatch) {

		faulty_loc_t faulty_loc;

		max_mismatch=cur_cnt;
		faulty_loc.mismatch_loc=target_bin;
		faulty_loc.mismatch_unit=unit;
		faulty_loc.mismatch_unit_num=unit_num;

// 		faulty_loc_list2.clear();
// 		for (int i=0;i<faulty_loc_list.size();i++) {
// 			if (!sameFaultyLoc(faulty_loc,faulty_loc_list[i]))
// 				faulty_loc_list2.push_back(faulty_loc_list[i]);
// 		}
		faulty_loc_list.clear();

		faulty_loc_list.push_back(faulty_loc);
	} else if (cur_cnt==max_mismatch) {

		faulty_loc_t faulty_loc;
		faulty_loc.mismatch_loc=target_bin;
		faulty_loc.mismatch_unit=unit;
		faulty_loc.mismatch_unit_num=unit_num;

		faulty_loc_list.push_back(faulty_loc);
	}
}


void diagnosis_t::treatStuckInstr(dynamic_inst_t *d) 
{
	if (LXL_DIAG_DEBUG) {
		DEBUG_OUT("Treat stuck instr %lld %s\n",d->getSequenceNumber(),
				  decode_opcode(d->getStaticInst()->getOpcode()));
		//d->printDetail();
	}

	d->setStuck();

	setExcludeDatapath();

	if (0) {
		terminating_instr=trace_head_ptr+FORWARD_INSTR_CNT;
		if (terminating_instr>MAX_INSTR_TRACE_SIZE) {
			terminating_instr-=MAX_INSTR_TRACE_SIZE;
		}
	}

	return;
}

int diagnosis_t::insertFakeInstr(dynamic_inst_t* d)
{
	instruction_information_t fake_instr;

	for (int i=0;i<SI_MAX_SOURCE;i++) {
		fake_instr.source[i].rid=d->getSourceReg(i);
	}
	for (int i=0;i<SI_MAX_DEST;i++) {
		fake_instr.dest[i].rid=d->getDestReg(i);
	}

	// as far as I know, this is the only uarch struct used
	fake_instr.uarch_info.win_id=d->getWindowIndex();

	instruction_trace[trace_head_ptr]=fake_instr;
	
	int cur_idx=trace_head_ptr;

	if (trace_inc(trace_head_ptr)==trace_tail_ptr) {
		trace_head_ptr=trace_tail_ptr;
		trace_tail_ptr=trace_inc(trace_tail_ptr);
	} else {
		trace_head_ptr=trace_inc(trace_head_ptr);
	}

	//DEBUG_OUT("fake trace_head_ptr %d\n",trace_head_ptr);

	return cur_idx;
}

void diagnosis_t::removeFakeInstr() 
{
	trace_head_ptr=trace_dec(trace_head_ptr);
	//DEBUG_OUT("remove trace_head_ptr %d\n",trace_head_ptr);
}
 



bool diagnosis_t::checkExitCond()
{
	// fault_loc_list stores the running max mismatch locations

	if (isFewerCand()) {
		int old_num_cand=num_candidates;
		num_candidates=faulty_loc_list.size();

		//DEBUG_OUT("num cand is %d\n",num_candidates);

		if (num_candidates==MAX_CANDIDATES) {
			diag_result_t result;
			instruction_information_t&info=instruction_trace[mismatch_index[last_processed_inst]];

			result.num_candidates=num_candidates;
			result.freq=max_mismatch;
			result.first_mis_ins=info.seq_num;
			result.first_mis_cyc=0; //FIXME
			result.num_mismatch_ins=num_mismatch_instr;

			result.loc_list.clear();

			for (int i=0;i<num_candidates;i++) {
				result.loc_list.push_back(faulty_loc_list[i]);
			}

			result_list.push_back(result);

			if (max_mismatch<MAX_NUM_MISMATCH && 
					result.loc_list[0].mismatch_loc!=DECODER)
				return false;

			HALT_SIMULATION;
			should_exit=true;
			return true;
		} else {
			diag_result_t result;
			instruction_information_t&info=instruction_trace[mismatch_index[last_processed_inst]];

			result.num_candidates=num_candidates;
			result.freq=max_mismatch;
			result.first_mis_ins=info.seq_num;
			result.first_mis_cyc=0; //FIXME
			result.num_mismatch_ins=num_mismatch_instr;

			result.loc_list.clear();

			for (int i=0;i<num_candidates;i++) {
				result.loc_list.push_back(faulty_loc_list[i]);
			}

			result_list.push_back(result);
		}
	}

	return false;

#if 0
	if ((max_mismatch >= MAX_TERM_MISMATCH && faulty_loc_list.size()<=MAX_TERM_CAND) || 
			(faulty_loc_list.size()==MAX_CANDIDATES && max_mismatch>=MAX_NUM_MISMATCH)) {
		//if (max_mismatch>MAX_NUM_MISMATCH) {

		if (LXL_DIAG_DEBUG) 
			DEBUG_OUT("exit condition satisfied, mm %d list %d\n",max_mismatch,faulty_loc_list.size());

		HALT_SIMULATION;
		should_exit=true;

		return true;
	}
	return false;
	}
#endif

}
void diagnosis_t::checkBlackList(instruction_information_t&info)
{
#if 0
	int size=value_watch_list.size();

	if (!size) return;

	for (int i=0; i<SI_MAX_DEST; i++) {
		if (!info.dest[i].rid.isZero()) {
			for (int j=0;j<size;j++) {
				if (info.dest[i].value==value_watch_list[j]) {
					DEBUG_OUT("%lld match value %llx on list w/ log %d\n",
							info.seq_num, value_watch_list[j],
							info.dest[i].rid.getFlatRegister());
				}
			}
		}
	}
#endif
}

void diagnosis_t::fatalTrap()
{
	if (LXL_DIAG_DEBUG) 
		DEBUG_OUT("fatal trap, %d mismatches\n",mismatch_index.size());

	analyzeTrace();

	if (LXL_BEYOND_FATAL_TRAP) {
		terminating_instr=trace_head_ptr+FORWARD_INSTR_CNT;
		if (terminating_instr>MAX_INSTR_TRACE_SIZE) {
			terminating_instr-=MAX_INSTR_TRACE_SIZE;
		}
	}
	//HALT_SIMULATION;
	//checkExitCond();
}

void diagnosis_t::lastLogicalWriter(reg_loc_t &reg_loc, reg_loc_t &ret_loc)
{
	int cur=reg_loc.i_idx;
	int reg_idx=reg_loc.reg_idx;
	bool isSource=reg_loc.isSource;
	instruction_information_t&cur_info=instruction_trace[cur];

	register_info_t *cur_reg_info;

	ret_loc.i_idx=-1;

	if (isSource) {
		cur_reg_info=&cur_info.source[reg_idx];
	} else {
		cur_reg_info=&cur_info.dest[reg_idx];
	}

	if (cur!=trace_tail_ptr) {
		for (int i=trace_dec(cur); i!=trace_tail_ptr; i=trace_dec(i)) {
			instruction_information_t &info=instruction_trace[i];

			for (int j=0; j<SI_MAX_DEST; j++) {
				reg_id_t&dest=info.dest[j].rid;
				if (!dest.isZero()) {
					if (inSameRF(dest,cur_reg_info->rid) &&
							(dest.getFlatRegister()==cur_reg_info->rid.getFlatRegister())) {
						ret_loc.i_idx=i;
						ret_loc.isSource=false;
						ret_loc.reg_idx=j;
						return;
					}
				}
			}
		}
	}
}

void diagnosis_t::nextPhysicalWriter(reg_loc_t &reg_loc, reg_loc_t &ret_loc)
{
	int cur=reg_loc.i_idx;
	int reg_idx=reg_loc.reg_idx;
	bool isSource=reg_loc.isSource;
	instruction_information_t&cur_info=instruction_trace[cur];

	register_info_t *cur_reg_info;

	ret_loc.i_idx=-1;

	if (isSource) {
		cur_reg_info=&cur_info.source[reg_idx];
	} else {
		cur_reg_info=&cur_info.dest[reg_idx];
	}

	if (cur!=trace_head_ptr) {

		for (int i=trace_inc(cur); i!=trace_head_ptr; i=trace_inc(i)) {
			instruction_information_t &info=instruction_trace[i];

			for (int j=0; j<SI_MAX_DEST; j++) {
				reg_id_t&dest=info.dest[j].rid;
				if (!dest.isZero()) {
					if (inSameRF(dest,cur_reg_info->rid) &&
							(dest.getPhysical()==cur_reg_info->rid.getPhysical())) {
						ret_loc.i_idx=i;
						ret_loc.isSource=false;
						ret_loc.reg_idx=j;
						return;
					}
				}
			}
		}
	}
}

void diagnosis_t::nextLogicalWriter(reg_loc_t &reg_loc, reg_loc_t &ret_loc)
{
	int cur=reg_loc.i_idx;
	int reg_idx=reg_loc.reg_idx;
	bool isSource=reg_loc.isSource;
	instruction_information_t&cur_info=instruction_trace[cur];
	int start_pt;

	register_info_t *cur_reg_info;

	ret_loc.i_idx=-1;

	if (isSource) {
		cur_reg_info=&cur_info.source[reg_idx];
		start_pt=cur;
	} else {
		cur_reg_info=&cur_info.dest[reg_idx];
		start_pt=trace_inc(cur);
	}

	if (cur!=trace_head_ptr) {
		for (int i=start_pt; i!=trace_head_ptr; i=trace_inc(i)) {
			instruction_information_t &info=instruction_trace[i];

			for (int j=0; j<SI_MAX_DEST; j++) {
				reg_id_t&dest=info.dest[j].rid;
				if (!dest.isZero()) {
					if (inSameRF(dest,cur_reg_info->rid) &&
							(dest.getFlatRegister()==cur_reg_info->rid.getFlatRegister())) {
						ret_loc.i_idx=i;
						ret_loc.isSource=false;
						ret_loc.reg_idx=j;
						return;
					}
				}
			}
		}
	}
}

void diagnosis_t::nextLogicalReader(reg_loc_t &reg_loc, reg_loc_t &ret_loc)
{
	int cur=reg_loc.i_idx;
	int reg_idx=reg_loc.reg_idx;
	bool isSource=reg_loc.isSource;
	instruction_information_t&cur_info=instruction_trace[cur];

	register_info_t *cur_reg_info;

	ret_loc.i_idx=-1;

	if (isSource) {
		cur_reg_info=&cur_info.source[reg_idx];
	} else {
		cur_reg_info=&cur_info.dest[reg_idx];
	}

	if (cur!=trace_head_ptr) {
		for (int i=trace_inc(cur); i!=trace_head_ptr; i=trace_inc(i)) {
			instruction_information_t &info=instruction_trace[i];
			//if (LXL_DIAG_DEBUG) DEBUG_OUT("nlr %d thp %d\n",i,trace_head_ptr);

			for (int j=0; j<SI_MAX_SOURCE; j++) {
				reg_id_t&source=info.source[j].rid;
				if (!source.isZero()) {
					if (inSameRF(source,cur_reg_info->rid) &&
							(source.getFlatRegister()==cur_reg_info->rid.getFlatRegister())) {
						ret_loc.i_idx=i;
						ret_loc.isSource=true;
						ret_loc.reg_idx=j;
						return;
					}
				}
			}
		}
	}
}

void diagnosis_t::lastPhysicalWriter(reg_loc_t &reg_loc, reg_loc_t &ret_loc)
{
	int cur=reg_loc.i_idx;
	int reg_idx=reg_loc.reg_idx;
	bool isSource=reg_loc.isSource;
	instruction_information_t&cur_info=instruction_trace[cur];

	register_info_t *cur_reg_info;

	ret_loc.i_idx=-1;

	if (isSource) {
		cur_reg_info=&cur_info.source[reg_idx];
	} else {
		cur_reg_info=&cur_info.dest[reg_idx];
	}

	if (cur!=trace_tail_ptr) {
		for (int i=trace_dec(cur); i!=trace_tail_ptr; i=trace_dec(i)) {
			instruction_information_t &info=instruction_trace[i];

			for (int j=0; j<SI_MAX_DEST; j++) {
				reg_id_t&dest=info.dest[j].rid;
				if (!dest.isZero()) {
					if (inSameRF(dest,cur_reg_info->rid) &&
							(dest.getPhysical()==cur_reg_info->rid.getPhysical())) {
						ret_loc.i_idx=i;
						ret_loc.isSource=false;
						ret_loc.reg_idx=j;
						return;
					}
				}
			}
		}
	}

}

// updateSuspects look at given dependency info and update bin for ROB
// or RAT entry.
// stuck_reg has a bad value
bool diagnosis_t::updateSuspects(reg_loc_t&stuck_reg)
{

	int inst_win_entry;
	int target_idx;

	inst_win_entry = instruction_trace[stuck_reg.i_idx].uarch_info.win_id;

	// The producing ROB entry could also cause this
	// The dest entry of producer is corrupted to point to another phys reg

	reg_loc_t producer;
	lastLogicalWriter(stuck_reg, producer);

	// Is it possible to not find a producer?
	if (producer.i_idx!=-1) {
		// There is a producer, are they pointing to same physical reg?
		if (samePhysical(stuck_reg,producer)) {
			// Yes, there are using the same phy reg. What could happen?
			// Someone else corrupts it?

			if (LXL_DIAG_DEBUG) {
				DEBUG_OUT("same phys\n");
				assert(producer.isSource==false);
				DEBUG_OUT("producer mismatch %d ",
						getInstrInfo(producer).dest[producer.reg_idx].mismatch);//?"true":"false");
				DEBUG_OUT("p g %llx b %llx c %llx\n",getGoodValue(producer),
						getBadValue(producer),
						getValue(stuck_reg));
			}


			// same phys reg but different value, somone else
			// corrupts this or this is incorrectly mapped to
			// other reg

			reg_loc_t last_physical_writer;

			lastPhysicalWriter(stuck_reg,last_physical_writer);

			if (last_physical_writer.i_idx!=producer.i_idx) {
				if (LXL_DIAG_DEBUG) {
					DEBUG_OUT("last writer bad %d\n",last_physical_writer.i_idx);
				}
				// of course value is different, someone else wrote it

				// not likely a rob fault in last_physical_writer

				// question is, who's messing who up?

				// stuck_reg has bad RAT mapping, then last_phys_writer freed it?
				// or last_phys_writer has bad mapping, then free this?
				if (getValue(last_physical_writer)==getValue(stuck_reg)) {
					// This is to confirm that the last physical
					// writer is responsible for the corruption.
					reg_loc_t a, b;

					nextLogicalReader(last_physical_writer, a);
					if (a.i_idx!=-1) {
						lastLogicalWriter(a, b);
						assert(b.i_idx!=-1);
						if (b.i_idx==last_physical_writer.i_idx) {
							if (samePhysical(last_physical_writer,a)) {
								if (LXL_DIAG_DEBUG) {
									DEBUG_OUT("I got here now lpw %d sr %d\n",getInstrInfo(b).uarch_info.win_id,
											getInstrInfo(stuck_reg).uarch_info.win_id);
								}
								updateBin(LOGICAL_REG,getLogical(last_physical_writer),-1);
								updateBin(LOGICAL_REG,getLogical(stuck_reg),-1);
							} else {
								updateBin(ROB,getInstrInfo(last_physical_writer).uarch_info.win_id,-1 );
								updateBin(ROB,getInstrInfo(a).uarch_info.win_id,-1 );
							}
						}
					} else {
						// No next logical reader, conservatively blame RAT
						updateBin(LOGICAL_REG,getLogical(last_physical_writer),-1);
						updateBin(LOGICAL_REG,getLogical(stuck_reg),-1);
					}
				} else {
					// If it is a mismatch, then someone in the future may be responsible
					// so trace forward
					if (findFutureCulprit(stuck_reg)) {
						updateFutureCulprit(stuck_reg);
					}
				}

				return true;
			}
			if (LXL_DIAG_DEBUG) {
				DEBUG_OUT("check range\n");
			}

			// Is this register overlapping with someone else's live range?
			if (checkRangeOverlap(producer)) {
				reg_loc_t lpw;

				lastPhysicalWriter(producer,lpw);

				updateBin(LOGICAL_REG,getLogical(lpw),-1);
				updateBin(LOGICAL_REG,getLogical(producer),-1);
				if (findFutureCulprit(stuck_reg)) {
					updateFutureCulprit(stuck_reg);
				}
				return true;
			} else {
				if (rule_out_datapath) {
					if (findFutureCulprit(producer)) {
						updateFutureCulprit(producer);
					}
					if (LXL_DIAG_DEBUG)
						DEBUG_OUT("stuck value %llx\n",getBadValue(producer));
					return true;
				} else {
					if (getRID(producer).getRtype()==RID_INT || 
							getRID(producer).getRtype()==RID_INT_GLOBAL) {
						updateBin(PHYSICAL_REG,getPhysical(producer),-1);
					}
					return false;
				}
			}


		} else {
			if (LXL_DIAG_DEBUG) {
				DEBUG_OUT("diff phys\n");
				DEBUG_OUT("thp %d ttp %d sr %d prod %d\n",trace_head_ptr,trace_tail_ptr,stuck_reg.i_idx,producer.i_idx);
				DEBUG_OUT("log  sr %d prod %d\n",getLogical(stuck_reg),getLogical(producer));
				DEBUG_OUT("phys sr %d prod %d\n",getPhysical(stuck_reg),getPhysical(producer));
			}

			// different physical registers
			// Either the producer has bad ROB dest entry or this has bad ROB src entry

			if (getPhysical(producer)!=PSEQ_INVALID_REG) {
				updateBin(ROB, inst_win_entry,-1);

				updateBin(ROB, instruction_trace[producer.i_idx].uarch_info.win_id,-1);
				return true;
			}

			//this is an evidence that only ROB has problem, but no one else.
		}
	} else {
		if (LXL_DIAG_DEBUG) {
			DEBUG_OUT("no producer\n");
		}
		// Could not find a producer
		if (checkRangeOverlap(stuck_reg)) {
			reg_loc_t last_physical_writer;

			lastPhysicalWriter(stuck_reg,last_physical_writer);

			if (getValue(last_physical_writer)==getValue(stuck_reg)) {
				// This is to confirm that the last physical
				// writer is responsible for the corruption.
				reg_loc_t a, b;

				nextLogicalReader(last_physical_writer, a);
				if (a.i_idx!=-1) {
					lastLogicalWriter(a, b);
					assert(b.i_idx!=-1);
					if (b.i_idx==last_physical_writer.i_idx) {
						if (samePhysical(last_physical_writer,a)) {
							if (LXL_DIAG_DEBUG) {
								DEBUG_OUT("I got here now lpw %d sr %d\n",getInstrInfo(b).uarch_info.win_id,
										getInstrInfo(stuck_reg).uarch_info.win_id);
							}
							updateBin(LOGICAL_REG,getLogical(last_physical_writer),-1);
							updateBin(LOGICAL_REG,getLogical(stuck_reg),-1);
						} else {
							updateBin(ROB,getInstrInfo(last_physical_writer).uarch_info.win_id,-1 );
							updateBin(ROB,getInstrInfo(a).uarch_info.win_id,-1 );
						}
					}
				} else {
					// No next logical reader, conservatively blame RAT
					updateBin(LOGICAL_REG,getLogical(last_physical_writer),-1);
					updateBin(LOGICAL_REG,getLogical(stuck_reg),-1);
				}
			} else {
				// If it is a mismatch, then someone in the future may be responsible
				// so trace forward
				if (findFutureCulprit(stuck_reg)) {
					updateFutureCulprit(stuck_reg);
				}
			}
			return true;

		} else {
			if (findFutureCulprit(stuck_reg)) {
				if (LXL_DIAG_DEBUG) DEBUG_OUT("found future culprit\n");
				updateFutureCulprit(stuck_reg);
				return true;
			}
			return false;
		}
	}

}

void diagnosis_t::processStuckInstr(int cur_inst) 
{
	instruction_information_t&info=instruction_trace[cur_inst];
	uint16 wait_reg_id=info.stuckSrc;
	reg_loc_t suspect;

	if (LXL_DIAG_DEBUG)
		DEBUG_OUT("processing stuck\n");
	updateRegLoc(suspect, cur_inst, true, wait_reg_id);
	updateSuspects(suspect);
}

bool diagnosis_t::checkRangeOverlap(reg_loc_t&reg_loc)
{
	reg_loc_t lpw,lpw_nlw;

	// Does this register overlap with previous live ranges
	lastPhysicalWriter(reg_loc,lpw);

	if (lpw.i_idx!=-1) {
		nextLogicalWriter(lpw,lpw_nlw);

		if (isLater(lpw_nlw,reg_loc)) {
			// live range is violated, blame reg_loc
			if (LXL_DIAG_DEBUG) {
				DEBUG_OUT("lpw_nlw %d reg_loc %d\n",lpw_nlw.i_idx,reg_loc.i_idx);
				DEBUG_OUT("thp %d ttp %d\n",trace_head_ptr,trace_tail_ptr);
			}
			return true;

		} else {
			if (LXL_DIAG_DEBUG) DEBUG_OUT("live range ok\n");
			return false;
		}
	} else {
		if (LXL_DIAG_DEBUG) DEBUG_OUT("no last pw\n");
		// do nothing
		return false;
	}
}

bool diagnosis_t::isLater(reg_loc_t&A,reg_loc_t&B)
{
	if (A.i_idx==-1) return true;

	// Whoever closest to head_ptr is the later one

	int dist_a,dist_b;

	if (trace_head_ptr>A.i_idx) {
		dist_a=trace_head_ptr-A.i_idx;
	} else {
		dist_a=trace_head_ptr+MAX_INSTR_TRACE_SIZE-A.i_idx;
	}

	if (trace_head_ptr>B.i_idx) {
		dist_b=trace_head_ptr-B.i_idx;
	} else {
		dist_b=trace_head_ptr+MAX_INSTR_TRACE_SIZE-B.i_idx;
	}
	return (dist_a<dist_b);

}



/***************************************************************************
  Functions of llb_t

	LLB_IMPLEMENTATION_1 : simple dictionary 
	LLB_IMPLEMENTATION_2 : dictionary holds 56 (most-significant) bits of value and 
			 queue elements holds 8 (least-significant) bits
			64bit value = 56bit dictionary_value : 8bit llb_entry
	LLB_IMPLEMENTATION_3 : dictionary holds 32 bits and 
			 queue elements holds 2 indices 
			64bit value = 32 bit dictionary_value : 32 bit dictionary_value
	LLB_IMPLEMENTATION_4 : dictionary holds 32 bits and 
			 queue elements holds 1 index 
			if is_frequent == 1
				index points to low 32 bits
			if is_frequent == 0
				index points to a 32 bit value, next value would be low 32 bits
 ***************************************************************************/
llb_t::llb_t() {

	dictionary = new uint64[LLB_DICTIONARY_SIZE];
	llb_queue = new struct llb_queue_entry [LLB_QUEUE_SIZE];
	front = 0;
	last = 0;
	llb_queue_size = 0;
	dictionary_size = 0;
	read_pointer = 0;
}

llb_t::~llb_t() 
{
	//delete[] dictionary;
	//delete[] llb_queue;
}
void llb_t::clearDictionary()
{
	for(int i=0; i<LLB_DICTIONARY_SIZE; i++) 
		dictionary[i] = -1;

	dictionary_size = 0;
}
void llb_t::clearQueue()
{
	for(int i=0; i<LLB_QUEUE_SIZE; i++)  {
#ifdef LLB_WITH_ADDRESS
		llb_queue[i].addr = -1;
#endif
		llb_queue[i].addr_parity = false;
		llb_queue[i].index = -1;
#ifdef LLB_IMPLEMENTATION_2
		llb_queue[i].offset = -1;
#endif
#ifdef LLB_IMPLEMENTATION_3
		llb_queue[i].index2 = -1;
#endif
#ifdef LLB_IMPLEMENTATION_4
		llb_queue[i].frequent_index = -1;
#endif
	}
	front = 0;
	last = 0;
	llb_queue_size = 0;

	read_pointer = 0;
}

void llb_t::clearLLB()
{
	clearDictionary();
	clearQueue();
}

void llb_t::copyLLB(llb_t *llb)
{
	clearLLB();

	//copy directory
	for(int i=0; i<LLB_DICTIONARY_SIZE; i++) {
		dictionary[i] = llb->dictionaryElementAt(i);
	}
	dictionary_size = llb->getDictionarySize();

	//copy llb_queue
	for(int i=0; i<LLB_QUEUE_SIZE; i++) {
		llb_queue[i] = llb->llbQueueElementAt(i);
	}
	llb_queue_size = llb->getLLBQueueSize();
	front = 0;
	read_pointer = 0;
	last = llb->getLast();
	//DEBUG_OUT(" last = %d\n", last);
}

//checks only the address parity
bool llb_t::isLLBHit(uint64 physical_address, int core_id)
{
	if(llb_queue_size <= 0 ) {
		if(MULTICORE_DIAGNOSIS_VERBOSE)
			DEBUG_OUT("%d:Empty: LLB Miss\n",core_id);
		return false;
	}
#ifdef LLB_WITH_ADDRESS
	//if(llb_queue.at(0).addr != physical_address)
	//	DEBUG_OUT("%d: Check address: %llx %llx\n",core_id, llb_queue.at(0).addr, physical_address);
#endif
	//parity computation
	uint64 address = physical_address;
	bool parity = address%2;
	for(int i=1; i<64; i++) {
		address = address>>1;
		parity = (address%2) ^ parity;
	}

#ifdef LLB_WITH_ADDRESS
	if(llb_queue[front].addr == physical_address) {
		return true;
	} else  {
		if(MULTICORE_DIAGNOSIS_VERBOSE)
			DEBUG_OUT("%d: LLB Miss\n",core_id);
		return false;
	}
#endif

	if(parity == llb_queue[front].addr_parity) // hit
		return true;
	else  {

		if(MULTICORE_DIAGNOSIS_VERBOSE)
			DEBUG_OUT("%d: LLB Miss\n",core_id);
		return false;
	}
}

bool llb_t::isLLBFull()
{
	if(llb_queue_size >= LLB_QUEUE_SIZE || dictionary_size >= LLB_DICTIONARY_SIZE)
		return true;
	else 
		return false;
}

bool llb_t::isLLBEmpty()
{
	if(llb_queue_size <= 0)
		return true;
	else 
		return false;
}

//doesnt pop the top element
uint64 llb_t::readFront()
{
	if(llb_queue_size <= 0) 
		return -1;
	if(front > last) {
		//DEBUG_OUT(" readFront:somethings wrong\n");
		return -1;
	}

	int index = llb_queue[front].index; //get index
	uint64 return_val = dictionary[index]; //get value from dictionary

#ifdef LLB_IMPLEMENTATION_2
	uint8 offset = llb_queue[front].offset; //get offset
	return_val = (return_val<<8);	//left shift the value from deictionary by 8
	return_val += offset;
#endif
#ifdef LLB_IMPLEMENTATION_3
	uint16 index2 = llb_queue[front].index2; //get index
	return_val = (return_val<<32); //get value from dictionary
	return_val += (uint32)dictionary[index2];
#endif
#ifdef LLB_IMPLEMENTATION_4
	bool is_frequent = (llb_queue[front].frequent_index <= 2);
	if(!is_frequent) {
		return_val = (return_val<<32); //get value from dictionary
		return_val += (uint32)dictionary[index+1];
	} else {
		uint64 freq_value = getFrequentValue(llb_queue[front].frequent_index);
		return_val += (freq_value<<32);
	}
#endif

	return return_val;
}

void llb_t::pop()
{
	if(llb_queue_size <= 0 ) 
		return;
	front++;
	llb_queue_size--;
}

//doesnt pop the top element
//ith from top
uint64 llb_t::readFront(int i)
{
	if(llb_queue_size <= 0) 
		return -1;
	if(front+i > last) {
		DEBUG_OUT(" readFront:index:somethings wrong\n");
		return -1;
	}
	//DEBUG_OUT(" readFront: %d ", front+i);

	int index = llb_queue[front+i].index; //get index
	uint64 return_val = dictionary[index]; //get value from dictionary

#ifdef LLB_IMPLEMENTATION_2
	uint8 offset = llb_queue[front+i].offset; //get offset
	return_val = (return_val<<8);	//left shift the value from deictionary by 8
	//DEBUG_OUT(" after shift=%llx ", return_val);
	return_val += offset;
#endif
#ifdef LLB_IMPLEMENTATION_3
	uint16 index2 = llb_queue[front+i].index2; //get index
	return_val = (return_val<<32); 
	uint32 temp_val = dictionary[index2];
	return_val += temp_val; //get value from dictionary
#endif
#ifdef LLB_IMPLEMENTATION_4
	bool is_frequent = (llb_queue[front+i].frequent_index <= 2);
	if(!is_frequent) {
		return_val = (return_val<<32); //get value from dictionary
		return_val += (uint32)dictionary[index+1];
	} else {
		uint64 freq_value = getFrequentValue(llb_queue[front+i].frequent_index);
		return_val += (freq_value<<32);
	}
#endif

	return return_val;
}


uint64 llb_t::readFromPointer(int llb_pointer)
{
	if(llb_pointer > last || llb_pointer < front) {
		//DEBUG_OUT(" readFromPointer:somethings wrong\n");
		return -1;
	}
	//DEBUG_OUT(" readFromPointer: %d ", llb_pointer);

	int index = llb_queue[llb_pointer].index; //get index
	uint64 return_val = dictionary[index]; //get value from dictionary

	//DEBUG_OUT(" index=%d ", index);
	//DEBUG_OUT(" dict_value=%llx ", return_val);

#ifdef LLB_IMPLEMENTATION_2
	uint8 offset = llb_queue[llb_pointer].offset; //get offset
	return_val = (return_val<<8);	//left shift the value from deictionary by 8
	//DEBUG_OUT(" after shift=%llx ", return_val);
	return_val += offset;
#endif
#ifdef LLB_IMPLEMENTATION_3
	uint16 index2 = llb_queue[llb_pointer].index2; //get index
	return_val = (return_val<<32); 
	return_val += (uint32)dictionary[index2];
#endif
#ifdef LLB_IMPLEMENTATION_4
	bool is_frequent = (llb_queue[llb_pointer].frequent_index <= 2);
	if(!is_frequent) {
		return_val = (return_val<<32); //get value from dictionary
		return_val += (uint32)dictionary[index+1];
	} else {
		uint64 freq_value = getFrequentValue(llb_queue[llb_pointer].frequent_index);
		return_val += (freq_value<<32);
	}
#endif

	return return_val;
}

int llb_t::getReadPointer()
{
	if(llb_queue_size <= 0) 
		return -1;
	if(read_pointer > last || read_pointer < front) {
		//DEBUG_OUT(" getReadPointer:somethings wrong\n");
		resetReadPointer();
		return read_pointer;
	}

	return read_pointer;
}

void llb_t::updateReadPointer(byte_t index)
{
	read_pointer += index;
}


void llb_t::resetReadPointer()
{
	read_pointer = front;
}

//returns if the llb is full after adding 
bool llb_t::addLLBEntry(uint64 address, uint64 value)
{
	if(isLLBFull()) {
		//if(MULTICORE_DIAGNOSIS_VERBOSE) 
			DEBUG_OUT(" LLB is Full\n");
		return true;
	}
		
	struct llb_queue_entry llb_entry;

#ifdef LLB_WITH_ADDRESS
	llb_entry.addr = address;
#endif

	//parity computation
	bool parity = address%2;
	for(int i=1; i<64; i++) {
		address = address>>1;
		parity = (address%2) ^ parity;
	}

	//DEBUG_OUT(" :0x%llx", value>>32);
	uint32 temp = value;
	//DEBUG_OUT(" :0x%llx", temp);
	//DEBUG_OUT("\n");
#ifdef LLB_IMPLEMENTATION_2
	uint8 offset = value % 256;
	value = (value>>8);
#endif //LLB_IMPLEMENTATION_2
#ifdef LLB_IMPLEMENTATION_3
	uint32 value2 = value; //get index
	value = (value>>32);
#endif //LLB_IMPLEMENTATION_3
#ifdef LLB_IMPLEMENTATION_4
	uint32 upper_val = value>>32;
	if(getFrequentIndex(upper_val) == 3) {
		int index = -1;
		for(int i=0; i<dictionary_size-1; i++) {
			if(dictionary[i] == upper_val) {
				if(dictionary[i+1] == (uint32)value) {
					index = i;
					break;
				}
			}
		}
		if(index == -1) {
			dictionary[dictionary_size] = upper_val;
			dictionary[dictionary_size+1] = (uint32)value;
			dictionary_size += 2;
			if(dictionary_size == 0) 
				index = 0;
			else 
				index = dictionary_size - 2;
		}

		llb_entry.frequent_index = 3;
		llb_entry.index = index;
	} else {
		int index = -1;
		for(int i=0; i<dictionary_size; i++) {
			if(dictionary[i] == (uint32)value) {
				index = i;
				break;
			}
		}
		if(index == -1) {
			dictionary[dictionary_size] = (uint32)value;
			dictionary_size++;
			if(dictionary_size == 0) 
				index = 0;
			else 
				index = dictionary_size - 1;
		}
		llb_entry.frequent_index = getFrequentIndex(upper_val);
		llb_entry.index = index;
	}
#else //LLB_IMPLEMENTATION_4

	//nomal directory based system
	//index in dictionary
	int index = -1;
	for(int i=0; i<dictionary_size; i++) {
		if(dictionary[i] == value) {
			index = i;
			//DEBUG_OUT("check0 index=%d, dictionary_size=%d, dictionary_val=%llx\n", index, dictionary_size, dictionary[index]);
			break;
		}
	}
	if(index == -1) {
		dictionary[dictionary_size] = (value);
		dictionary_size++;
		if(dictionary_size == 0) 
			index = 0;
		else 
			index = dictionary_size - 1;
		//DEBUG_OUT("check1 index=%d, dictionary_size=%d, dictionary_val=%llx\n", index, dictionary_size, dictionary[index]);
	}
	
	llb_entry.index = index;
#endif //LLB_IMPLEMENTATION_4

	llb_entry.addr_parity = parity;

#ifdef LLB_IMPLEMENTATION_2
	llb_entry.offset = offset;
#endif
#ifdef LLB_IMPLEMENTATION_3

	//index in dictionary
	int index2 = -1;
	for(int i=0; i<dictionary_size; i++) {
		if(dictionary[i] == value2) {
			index2 = i;
			break;
		}
	}
	if(index2 == -1) {
		dictionary[dictionary_size] = (value2);
		dictionary_size++;
		if(dictionary_size == 0) 
			index2 = 0;
		else 
			index2 = dictionary_size - 1;
	}
	llb_entry.index2 = index2;
#endif
	
	llb_queue[last] = llb_entry;
	last++;
	llb_queue_size++;

	//DEBUG_OUT("added llb_queue=%d llb_index=%d,", last-1, llb_entry.index);
	//DEBUG_OUT(" dictionary_val=%llx\n", dictionary[llb_entry.index]);
	//DEBUG_OUT("value =%llx\n", value);
	if(isLLBFull()) {
		if(MULTICORE_DIAGNOSIS_VERBOSE) 
			DEBUG_OUT(" LLB is Full\n");
		return true;
	}
	return false;
}

#ifdef LLB_IMPLEMENTATION_4
int llb_t::getFrequentIndex(uint32 value)
{
	if(value == 0)
		return 0;
	else
	if(value == 1)
		return 1;
	else
	if(value == 0xffffffff )
		return 2;
	else 
		return 3;
}
int llb_t::getFrequentValue(int freq_index)
{
	if(freq_index == 0)
		return 0;
	else
	if(freq_index == 1)
		return 1;
	else
	if(freq_index == 2 )
		return 0xffffffff;
	else 
		return -1;
}

#endif

/***************************************************************************
  Functions of multicore_diagnosis_t
 ***************************************************************************/

multicore_diagnosis_t::multicore_diagnosis_t(system_t *sys, pstate_t **pstate, uint32 num_cores) 
{
	m_sys = sys;
	m_state = pstate;
	numProcs = num_cores;
	num_instructions = new int[numProcs];
	current_inst_count = new int[numProcs];
	num_inst_to_run = new int[numProcs];
	squashing_address = new uint64[numProcs];	//address on which the core is squashing
	num_squashes = new uint64[numProcs];
	num_load_instructions = new uint64[numProcs];

	llb = new llb_t*[numProcs];
	func_inst_buffer = new func_inst_buffer_t[numProcs];
	compare_log = new vector<compare_log_element_t>[numProcs];
	compare_log_logging = new vector<compare_log_element_t>[numProcs];
	llb_miss = new bool[numProcs];
	llb_miss_instr = new uint64[numProcs];
	symptom_raised = new bool[numProcs];
	symptom_raised_logging = new bool[numProcs];
	previous_pc = new uint64[numProcs];
	crc_logging = new unsigned short[numProcs];
	crc_tmr = new unsigned short[numProcs];
	crc_tmr_older = new unsigned short[numProcs];
	tlb_state = new tlb_state_t[numProcs];
	core_state = new core_state_t[numProcs];
	tlb_state_main = new tlb_state_t[numProcs];
	core_state_main = new core_state_t[numProcs];
	tlb_state_previous = new tlb_state_t[numProcs];
	core_state_previous = new core_state_t[numProcs];

	trigger_logging_step = new bool[numProcs];
	trigger_tmr_step = new bool[numProcs];
	checked_core = new bool[numProcs];
	current_cycle_count = new int[numProcs];
	missing_addresses_in_core = new bool[numProcs];
	core_stopped_in_tmr = new bool[numProcs];

	num_llb_entries = new uint64[numProcs];

	start_local_cycle = new uint64[numProcs];
	start_sequence_number = new uint64[numProcs];

	mmu = new conf_object_t*[numProcs];
	
	no_physical_address = new bool[numProcs];

	tmr_stop_requested = false;
	current_tmr_core = -1;
	diagnosis_started = false;
	logging_step = false;
	last_replay = false;
	tmr_phase = false;
	dont_run_core = -1;
	dont_continue_core = -1;
	num_diagnosis_steps = 0;
	symptom_causing_core = -1;
	symptom_core_logging_phase = -1;
	m_num_interrupts = 0;
	num_symptom_detected = 0;
	num_logging_phases = 0;
	diagnosis_latency_instr = 0;
	avg_logging_length = 0;
	total_logging_instructions = new uint64[numProcs];
	diagnosis_latency_cycles = 0;
	logging_latency_cycles = 0;
	expected_diagnosis_latency_cycles= 0;
	total_logging_latency_cycles= 0;
	stop_diagnosis = false;
	is_replay_phase = false;
	
	num_branch_instr = new int[numProcs];
	num_store_instr = new int[numProcs];
	num_load_instr = new int[numProcs];
	total_num_branch_instr = new int[numProcs];
	total_num_store_instr = new int[numProcs];
	total_num_load_instr = new int[numProcs];

	no_tmr_after = new uint64[numProcs];

	faulty_core_suspects = -1;
	faulty_core_found = false;
	suspect_found = false;

	reason_for_logging_phase = -1;

	llb_miss_last_replay = false;
	llb_miss_last_replay_instr = -1;
	symptom_raised_last_replay = false;
	CRC16_InitChecksum(crc_last_replay);
			
	//address_not_found_info.core_id = -1;
	//address_not_found_info.current_inst_number = -1;

	for(int i=0; i<numProcs; i++) {

		previous_pc[i] = -1;
		llb_miss[i] = false;
		llb_miss_instr[i] = -1;
		symptom_raised[i] = false;
		CRC16_InitChecksum(crc_logging[i]);
		CRC16_InitChecksum(crc_tmr[i]);
		symptom_raised_logging[i] = false;
		llb[i] = new llb_t();
		no_physical_address[i] = false;
		start_local_cycle[i] = 0;
		start_sequence_number[i] = 0;
		core_stopped_in_tmr[i] = false;
		missing_addresses_in_core[i] = false;

		current_cycle_count[i] = 0;

		trigger_logging_step[i] = false;
		trigger_tmr_step[i] = false;
		num_instructions[i] = 0;
		num_inst_to_run[i] = 0;
		current_inst_count[i] = 0;
		checked_core[i] = false;
		squashing_address[i] = -1;

		num_llb_entries[i] = 0;

		num_squashes[i] = 0;
		num_load_instructions[i] = 0;
		num_branch_instr[i] = 0;
		num_store_instr[i] = 0;
		num_load_instr[i] = 0;
		total_num_branch_instr[i] = 0;
		total_num_store_instr[i] = 0;
		total_num_load_instr[i] = 0;
		total_logging_instructions[i] = 0;

		no_tmr_after[i] = 100000000;

	}

}

multicore_diagnosis_t::~multicore_diagnosis_t() 
{

	for(int i=0; i<numProcs; i++) {
		llb[i]->clear();
		func_inst_buffer[i].clear();
		compare_log[i].clear();
		compare_log_logging[i].clear();
	}
	
	delete[] core_stopped_in_tmr;
	delete[] num_instructions ;
	delete[] current_inst_count ;
	delete[] num_inst_to_run ;
	delete[] squashing_address ;
	delete[] num_squashes ;
	delete[] num_load_instructions;
	delete[] no_tmr_after;

	delete[] func_inst_buffer ;
	delete[] compare_log ;
	delete[] compare_log_logging ;
	delete[] tlb_state ;
	delete[] core_state ;
	delete[] core_state_main ;
	delete[] tlb_state_main ;
	delete[] tlb_state_previous ;
	delete[] core_state_previous ;

	delete[] trigger_logging_step ;
	delete[] trigger_tmr_step ;
	delete[] checked_core ;
	delete[] current_cycle_count ;

	delete[] num_llb_entries ;
}

void multicore_diagnosis_t::reportLoadInstruction(int core_id)
{
	if(isTMRPhase()) {
		num_load_instructions[core_id]++;
	}
	if(isLoggingPhase()) {
		num_load_instr[core_id]++;
	}
}

void multicore_diagnosis_t::reportSquash(int core_id)
{
	if(isTMRPhase()) {
		num_squashes[core_id]++;
	}
}

bool multicore_diagnosis_t::isSquashingOnAddress(uint64 addr, int from_core_id)
{
	for(int i=0; i<numProcs; i++)
		if(i != from_core_id)
			if(((squashing_address[i]-16) <= addr) &&  (addr <= (squashing_address[i]+16))) {
				if(SIVA_LOAD_BUFFER_DEBUG)
				DEBUG_OUT(" squashing addr= %llx, addr=%llx\n", squashing_address[i], addr);
				return true;
			}
	return false;
}

void multicore_diagnosis_t::reportSquashingOnAddress(uint64 addr, int core_id)
{
	reportSquash(core_id);
	if(core_id != dont_run_core) {
		if(SIVA_LOAD_BUFFER_DEBUG)
			DEBUG_OUT("%d: reporting squash on addr= %llx\n", core_id, addr);
		
		squashing_address[core_id] = addr;
	}
}

void multicore_diagnosis_t::clearSquashingOnAddress(int core_id)
{
	squashing_address[core_id] = -1;
}

void multicore_diagnosis_t::getMMUObjects()
{
	for(int i=0; i<numProcs; i++) {
		char *mmuname = new char[64];
		pstate_t::getMMUName( i, mmuname, 64 );
		mmu[i] = SIMICS_get_object( mmuname );
		//DEBUG_OUT(" mmu=%d mmuname=%s\n", mmu[i], mmuname);
		delete[] mmuname;
	}
}

void multicore_diagnosis_t::startMulticoreDiagnosis()
{
	if(!diagnosis_started) {
		DEBUG_OUT("\n Starting Multicore Diagnosis \n\n");

		//copying the checkpoint to the main checkpoint
		//the checkpoint will be overwritten by the new checkpoints at the end of the logging steps
		for(int i=0; i<numProcs; i++) {
			core_state_main[i] = core_state[i];
			tlb_state_main[i] = tlb_state[i];
		}
		getMMUObjects();

		setFirstTimeRollback();
		triggerLoggingPhase();
		diagnosis_started = true;
	}
}

int multicore_diagnosis_t::max( int * num_instr, int n)
{
	int max = 0;
	for(int i=0; i<n; i++) {
		if(max <= num_instr[i]) 
			max = num_instr[i];
	}
	return max;
}
	

//LastReplay is a part of the TRMPhase
void multicore_diagnosis_t::triggerLastReplay()
{
	unsetLoggingStep();
	setLastReplay();


	for(int i=0; i<numProcs; i++) {
		CRC16_FinishChecksum(crc_tmr[i]);
	}
		
	resetStartLocalCycle();
	CRC16_InitChecksum(crc_last_replay);
	tmr_stop_requested = false;
	llb_miss_last_replay = false;
	llb_miss_last_replay_instr = -1;
	symptom_raised_last_replay = false;
	dont_run_core = -1;
	dont_continue_core = -1;
	if(CONCISE_VERBOSE)
		DEBUG_OUT("\nLast Replay triggered\n");

	int execute_core = (faulty_core_suspects+2)%numProcs;

	core_stopped_in_tmr[execute_core] = false;
	current_cycle_count[execute_core] = 0;
	no_physical_address[execute_core] = no_physical_address[(faulty_core_suspects+1)%numProcs];
	no_tmr_after[execute_core] = no_tmr_after[(faulty_core_suspects+1)%numProcs];

	// collect the number of instructions to run in the tmr step for each processor
	num_inst_to_run[execute_core] = num_instructions[faulty_core_suspects];
	DEBUG_OUT(" num_instructions=%d \n", num_instructions[faulty_core_suspects]);

	//stat collection
	current_inst_count[execute_core] = 0;	//clear for tmr step

	clearAddressNotFound();
	freeCompareLog(execute_core);

	DEBUG_OUT("faulty_core_suspects=%d", faulty_core_suspects);
	DEBUG_OUT("execute=%d\n", execute_core);
	loadCheckpoint(faulty_core_suspects, execute_core);
	putLog(faulty_core_suspects, execute_core);

	debugio_t::clearSymptomDetected();
	symptom_causing_core = -1;
}

void multicore_diagnosis_t::triggerLoggingPhase()
{

	if(reason_for_tmr_phase == LLB_FULL) {
		//if(num_logging_phases >= 100) {
		//	DEBUG_OUT("You asked me to stop! \n\n");
		//	printStats();
		//	HALT_SIMULATION;
		//}
		if(min(total_logging_instructions, numProcs) <= DIAGNOSIS_GIVEUP_THRESHOLD) {
			if(CONCISE_VERBOSE)
				DEBUG_OUT(" Rolling back to do the Logging step again\n");
			//triggerLoggingPhase();
		} else {
			DEBUG_OUT("\n Diagnosis has given up after %d instructions\n", DIAGNOSIS_GIVEUP_THRESHOLD);
			DEBUG_OUT("\n Done with Multicore Diagnosis \n");
			DEBUG_OUT("No Divergence is detected\n\n");
			printStats();
			HALT_SIMULATION;
			return;
		}
	} else {
		if(num_diagnosis_steps < DIAGNOSIS_THRESHOLD) {
			DEBUG_OUT(" Restarting Diagnosis: No divergence was found this time\n");
			num_diagnosis_steps++;

			//restore the main checkpoint
			for(int i=0; i<numProcs; i++) {
				core_state[i] = core_state_main[i];
				tlb_state[i] = tlb_state_main[i];
			}
			//need to notify memory state rollback also
			setFirstTimeRollback();	//while triggering the logging phase, this function would make memory rollback

			//trigger Logging Step
			//triggerLoggingPhase();

		} else {
			if(reason_for_tmr_phase == SYMPTOM_IN_LOGGING_PHASE) {
				DEBUG_OUT(" Symptom in Logging step and no divergence in TMR phase\n");
				DEBUG_OUT(" Faulty core is %d\n", symptom_core_logging_phase);
				printStats();
				HALT_SIMULATION;
				return;
			}
			// else {
			//	DEBUG_OUT("\n Done with Multicore Diagnosis \n");
			//	DEBUG_OUT("No Divergence is detected\n\n");
			//	printStats();
			//	HALT_SIMULATION;
			//	return;
			//}
		}
	}

	setLoggingStep();
	unsetTMRPhase();
	unsetLastReplay();

	resetStartSequenceNumber();
	resetNoPhysicalAddress();
	resetStartLocalCycle();
	tmr_stop_requested = false;
	reason_for_tmr_phase = -1;
	dont_run_core = -1;
	dont_continue_core = -1;
	if(CONCISE_VERBOSE) {
		DEBUG_OUT("\nLogging Step triggered\n");
	}

	if(reason_for_logging_phase == INTERRUPT_IN_EXEC) {
		reason_for_logging_phase = -1;
	} else {
		//copy the core_state to core_state_previous
		//there are 2 checkpoints stores, one for the previous, and one for the current. You always rollback to the previous
		copyBackCheckpoint();	//this has to be done before Load checkpoint NOTE: Ordering is important
	}

	//diagnosis latency computation
	for(int i=0; i<numProcs; i++) {
		// new Logging Phase is going to start, so add the number of instructions from each TMR step in the previous TMR phase 
		diagnosis_latency_instr += num_instructions[i]; 
		no_tmr_after[i] = 100000000;
	}
	//for the previous logging step
	diagnosis_latency_instr += max(num_instructions, numProcs);

	for(int i=0; i<numProcs; i++) {
		CRC16_InitChecksum(crc_logging[i]);
		symptom_raised_logging[i] = false;
		core_stopped_in_tmr[i] = false;
		m_sys->clearLLB(i);
		clearCompareLog(i);
		trigger_logging_step[i] = true;	
		current_inst_count[i] = 0;		//clear current inst count for counter number of instructions in logging step
		checked_core[i] = false;
		num_instructions[i] = 0;	//reset: number of instructions in each core during logging step
	}

	for(int i=0; i<numProcs; i++) {
		//DEBUG_OUT("Rolling back for Logging Step Core %d\n", i);
		loadCheckpoint(i,i);
		//m_sys->disableInterrupts(i);
	}
	if(MULTICORE_DIAGNOSIS_VERBOSE)
		DEBUG_OUT("All the cores rolled back for Logging Step\n");

	debugio_t::clearSymptomDetected();
	symptom_causing_core = -1;
	symptom_core_logging_phase = -1;

	num_logging_phases++;

}

void multicore_diagnosis_t::clearCompareLog(int core_id)
{
	compare_log_logging[core_id].clear();
	compare_log[core_id].clear();
}

void multicore_diagnosis_t::copyBackCheckpoint()
{
	for(int i=0; i<numProcs; i++) {
		//processor state
		for(int j=0; j<MAX_GLOBAL_REGS; j++)
			core_state_previous[i].global_regs[j] = core_state[i].global_regs[j];
		for(int j=0; j<MAX_INT_REGS; j++)
			core_state_previous[i].int_regs[j] = core_state[i].int_regs[j];
		for(int j=0; j<MAX_FLOAT_REGS; j++)
			core_state_previous[i].fp_regs[j] = core_state[i].fp_regs[j];
		for(int j=0; j<MAX_CTL_REGS; j++)
			core_state_previous[i].ctl_regs[j] = core_state[i].ctl_regs[j];


		//TLB state
		for(int j=0; j<49; j++) {
			SIM_free_attribute(tlb_state_previous[i].tlb_reg[j]);
			tlb_state_previous[i].tlb_reg[j] = tlb_state[i].tlb_reg[j];
		}
	}
}

void multicore_diagnosis_t::getLog(int from, int to)
{
	//load buffer
	llb[to]->clear();
	llb[to]->copyLLB(m_sys->getLLB(from));

	// size of llb
	num_llb_entries[to] = llb[to]->size();

	if(SIVA_LOAD_BUFFER_DEBUG)
		DEBUG_OUT("Copied %d elements of load buffer from=%d to diagnosis buffer at=%d \n",llb[to]->size(), from, to);

	//log for functional instructions
	func_inst_buffer[to].clear();

	func_inst_buffer_t func_inst_buf = m_sys->getFuncLog(from);

	func_inst_buffer[to].insert(func_inst_buffer[to].end(), func_inst_buf.begin(),  func_inst_buf.end());

	func_inst_buf.clear();

}

void multicore_diagnosis_t::putLog(int from, int to)
{
	m_sys->putLLB(llb[from], to);
	m_sys->putFuncLog(func_inst_buffer[from], to);
}

void multicore_diagnosis_t::triggerTMRPhase()
{

	if(reason_for_tmr_phase == -1) 
		DEBUG_OUT(" There should be a reason for starting TMR phase\n");

	setTMRPhase();
	unsetLoggingStep();
	unsetLastReplay();
	resetStartLocalCycle();
	tmr_stop_requested = false;
	dont_continue_core = -1;
	uint64 average_logging_instr = 0;
	for(int i=0; i<numProcs; i++) {
		CRC16_InitChecksum(crc_tmr[i]);
		CRC16_FinishChecksum(crc_logging[i]);
		llb_miss[i] = false;
		llb_miss_instr[i] = -1;
		symptom_raised[i] = false;
		core_stopped_in_tmr[i] = false;
		current_cycle_count[i] = 0;
		trigger_tmr_step[i] = true;
		m_sys->takeCheckpoint(i); //checkpoint processor state and the TLB after logging step to restart the logging 
		getLog(i,i);		//getLLB after Logging Step

		// collect the number of instructions to run in the tmr step for each processor
		num_instructions[i] = current_inst_count[i];	

		//stat collection
		average_logging_instr += num_instructions[i];
		total_logging_instructions[i] += num_instructions[i];
		total_num_store_instr[i] += num_store_instr[i];
		total_num_branch_instr[i] += num_branch_instr[i];
		total_num_load_instr[i] += num_load_instr[i];
		if(CONCISE_VERBOSE) {
			DEBUG_OUT("%d: num instructions = %d\n",i, num_instructions[i]); 
			DEBUG_OUT("%d: num load instructions = %d\n",i, num_load_instr[i]); 
			DEBUG_OUT("%d: num store instructions = %d\n",i, num_store_instr[i]); 
			DEBUG_OUT("%d: num branch instructions = %d\n",i, num_branch_instr[i]); 
		}
		num_load_instr[i] = 0;
		num_store_instr[i] = 0;
		num_branch_instr[i] = 0;
		current_inst_count[i] = 0;	//clear for tmr step

	}

	if(reason_for_tmr_phase == LLB_FULL)
		avg_logging_length += average_logging_instr/4;
	clearAddressNotFound();
	if(CONCISE_VERBOSE)
		DEBUG_OUT("\n TMR Step triggered\n");
	
	//tmrPolicy();
	replayPolicy();
}

void multicore_diagnosis_t::replayPolicy()
{
	current_tmr_core = -1;

	//release all the cores
	for(int i=0; i<numProcs; i++) 
		core_stopped_in_tmr[i] = false;

	for(int i=0; i<numProcs; i++) {
		int next = (i+1)%numProcs;

		freeCompareLog(i);
		if(SIVA_LOAD_BUFFER_DEBUG) {
			DEBUG_OUT(" compre log [%d]= %d\n", i, compare_log[i].size());
		}
		current_cycle_count[i] = 0;
		current_inst_count[i] = 0;	//clear for tmr step
		loadCheckpoint(i,next);
		putLog(i,next);
		num_inst_to_run[next] = num_instructions[i];
		//DEBUG_OUT("num instructions to run on core next=%d is %d\n", next, num_inst_to_run[next]);
		m_sys->disableInterrupts(next);

		clearAddressNotFound();
		debugio_t::clearSymptomDetected();
		symptom_causing_core = -1;
		if(MULTICORE_DIAGNOSIS_VERBOSE)
			DEBUG_OUT("All the cores rolled back to the checkpoints \n");
	}

}
void multicore_diagnosis_t::tmrPolicy()
{
	bool done = true;
	current_tmr_core = -1;
	//release all the cores
	for(int i=0; i<numProcs; i++) 
		core_stopped_in_tmr[i] = false;

//#define FAULTY_CORE_FIRST_POLICY
#ifdef FAULTY_CORE_FIRST_POLICY
	
	int i = debugio_t::getFaultyCore();
	
	for(; true; i=(i+1)%numProcs ) {
		if(!checked_core[i]) {	//for the unchecked cores
	if(CONCISE_VERBOSE)
			DEBUG_OUT("\nTMR Step for Core %d\n", i);
			current_tmr_core = i;

			dont_run_core = (i+2)%numProcs;
			if(dont_run_core == debugio_t::getFaultyCore()) {
				dont_run_core = (dont_run_core+1)%numProcs;
			}
			core_stopped_in_tmr[dont_run_core] = true;

			for(int j=0; j<numProcs; j++) {	
				freeCompareLog(j);
				if(SIVA_LOAD_BUFFER_DEBUG) {
					DEBUG_OUT(" compre log [%d]= %d\n", j, compare_log[j].size());
					DEBUG_OUT(" Previous step in core%d has %d instructions \n",i, current_inst_count[i]); 
				}
				current_cycle_count[j] = 0;
				current_inst_count[j] = 0;	//clear for tmr step
				//if(dont_run_core != j) {
				loadCheckpoint(i,j);
				//put log in to each core (j)
				putLog(i,j);
				//}
				num_inst_to_run[j] = num_instructions[i];
				m_sys->disableInterrupts(j);
			}

			clearAddressNotFound();
			debugio_t::clearSymptomDetected();
			symptom_causing_core = -1;
			//DEBUG_OUT("All the cores rolled back to the checkpoint of Core %d\n", i);
			//clearing unnecessary logs
			for(int j=0; j<numProcs; j++) {
				if(checked_core[j]) {
					llb[j]->clear();	//no need to keep a copy of load buffer of checked cores
					freeFuncInstBuffer(j);
				}
			}
			checked_core[i] = true; //check-mark the current cores, after the above clear loop 
			done = false;
			break;
		}
	}


#else //FAULTY_CORE_FIRST_POLICY

	// The following starts from the symptom core first and does the round robin

	int i = 0; //= symptom_causing_core;
	if(reason_for_tmr_phase == SYMPTOM_IN_LOGGING_PHASE) 
		i = symptom_causing_core;

	if(i == -1) 
		i = 0;
	
	for(; true; i=(i+1)%numProcs ) {
		//check if all the cores are checked, if yes then terminate the tmr phase
		bool all_checked = true;
		for(int k=0; k<numProcs; k++) {
			if(!checked_core[k])
				all_checked = false;	
		}
		if(all_checked)
			break;
		
		if(!checked_core[i]) {
	if(CONCISE_VERBOSE)
			DEBUG_OUT("TMR Step for Core %d\n", i);
			current_tmr_core = i;

			dont_run_core = (i+3)%numProcs;
			core_stopped_in_tmr[dont_run_core] = true;
			//DEBUG_OUT("Dont run core = %d\n",dont_run_core);
			assert(numProcs == 4);
			
			for(int j=0; j<numProcs; j++) {	
				freeCompareLog(j);
				if(SIVA_LOAD_BUFFER_DEBUG) {
					DEBUG_OUT(" compre log [%d]= %d\n", j, compare_log[j].size());
					DEBUG_OUT(" Previous step in core%d has %d instructions \n",i, current_inst_count[i]); 
				}
				current_cycle_count[j] = 0;
				current_inst_count[j] = 0;	//clear for tmr step
				loadCheckpoint(i,j);
				putLog(i,j);
				num_inst_to_run[j] = num_instructions[i];
				m_sys->disableInterrupts(j);
			}

			clearAddressNotFound();
			debugio_t::clearSymptomDetected();
			symptom_causing_core = -1;
			if(MULTICORE_DIAGNOSIS_VERBOSE)
				DEBUG_OUT("All the cores rolled back to the checkpoint of Core %d\n", i);
			//clearing unnecessary logs
			for(int j=0; j<numProcs; j++) {
				if(checked_core[j]) {
					llb[j]->clear();	//no need to keep a copy of load buffer of checked cores
					freeFuncInstBuffer(j);
				}
			}
			checked_core[i] = true; //check-mark the current cores, after the above clear loop 
			done = false;
			break;
		}
	}

#endif //FAULTY_CORE_FIRST_POLICY

	if(done) {
		dont_run_core = -1;

		//If the tmr phase was triggered due to LLB_FULL, then trigger the Logging Step again
		if(reason_for_tmr_phase == LLB_FULL) {
			//if(num_logging_phases >= 100) {
			//	DEBUG_OUT("You asked me to stop! \n\n");
			//	printStats();
			//	HALT_SIMULATION;
			//}
			if(min(total_logging_instructions, numProcs) <= DIAGNOSIS_GIVEUP_THRESHOLD) {
				if(CONCISE_VERBOSE)
					DEBUG_OUT(" Rolling back to do the Logging step again\n");
				triggerLoggingPhase();
			} else {
				DEBUG_OUT("\n Diagnosis has given up after %d instructions\n", DIAGNOSIS_GIVEUP_THRESHOLD);
				DEBUG_OUT("\n Done with Multicore Diagnosis \n");
				DEBUG_OUT("No Divergence is detected\n\n");
				printStats();
				HALT_SIMULATION;
			}

		} else {
			if(num_diagnosis_steps < DIAGNOSIS_THRESHOLD) {
				DEBUG_OUT(" Restarting Diagnosis: No divergence was found this time\n");
				num_diagnosis_steps++;

				//restore the main checkpoint
				for(int i=0; i<numProcs; i++) {
					core_state[i] = core_state_main[i];
					tlb_state[i] = tlb_state_main[i];
				}
				//need to notify memory state rollback also
				setFirstTimeRollback();	//while triggering the logging phase, this function would make memory rollback

				//trigger Logging Step
				triggerLoggingPhase();

			} else {
				if(reason_for_tmr_phase == SYMPTOM_IN_LOGGING_PHASE) {
					DEBUG_OUT(" Symptom in Logging step and no divergence in TMR phase\n");
					DEBUG_OUT(" Faulty core is %d\n", symptom_core_logging_phase);
					printStats();
					HALT_SIMULATION;
				} else {
					DEBUG_OUT("\n Done with Multicore Diagnosis \n");
					DEBUG_OUT("No Divergence is detected\n\n");
					printStats();
					HALT_SIMULATION;
				}
			}
		}
	}

}

void multicore_diagnosis_t::loadTLB(int from, int to)
{
	if(SIVA_LOAD_BUFFER_DEBUG)
		DEBUG_OUT("setting attribute\n");

	char *cheetah_reg_names[] = {
		"ctxt_nucleus", 
		"ctxt_primary", 
		"ctxt_secondary", 
		"dcu_ctrl",
		"dsfar",
		"dsfsr",
		"dtag_access",
		"dtag_access_ex", 
		"dtag_target",
		"dtlb_2w_daccess",
		"dtlb_2w_tagread", 
		"dtlb_fa_daccess", 
		"dtlb_fa_tagread", 
		"dtsb",
		"dtsb_nx",
		"dtsb_px",
		"dtsb_sx", "dtsbp64k", "dtsbp8k", "dtsbpd", "ec_control", "isfsr",
		"itag_access", "itag_target", "itlb_2w_daccess", "itlb_2w_tagread", "itlb_fa_daccess",
		"itlb_fa_tagread", "itsb", "itsb_nx", "itsb_px", "itsbp64k", "itsbp8k",
		"last_etag_write", "lfsr", "lsu_ctrl", "madr_1", "madr_2", "madr_3", "madr_4",
		"mem_address_control", "mem_tmg_1", "mem_tmg_2", "mem_tmg_3", "mem_tmg_4", "mem_tmg_5",
		"pa_watchpoint", "trace", "va_watchpoint"
	};

	//LXL: Restore all mmu regs/tlb entries
	for (int i=0; i<49; i++) {
		SIM_set_attribute(mmu[to],cheetah_reg_names[i], &tlb_state_previous[from].tlb_reg[i]);
	}

	if(SIVA_LOAD_BUFFER_DEBUG)
		DEBUG_OUT("done setting attribute\n");
}

void multicore_diagnosis_t::loadRegState(int from, int to)
{
	//DEBUG_OUT("\nloading checkpoint to %d\n", to);
	m_state[to]->recoverState(&core_state_previous[from]);
}

void multicore_diagnosis_t::loadCheckpoint(int from, int to)
{
	m_sys->prepareRollback(to);
	loadTLB(from, to);
	loadRegState(from, to);
	m_sys->postRollback(to);
}


//the multicore diagnosis algorithm
// This is called every cycle by each of the cores
void multicore_diagnosis_t::runMulticoreDiagnosisAlgorithm(int core_id)
{
	if(num_symptom_detected > 100) {
		DEBUG_OUT(" more than 100 symptom are detected, you are better off by shutting the system down!\n");
		printStats();
		HALT_SIMULATION;
	}
	
	if(debugio_t::checkSymptomDetected(core_id)) {		//if the symptom is detected
#ifdef REPLAY_PHASE
		copyBackCheckpoint();	//this has to be done before Load checkpoint NOTE: Ordering is important
		for(int i=0; i<numProcs; i++) {
			DEBUG_OUT("Rolling back for Replay Phase %d\n", i);
			//loadCheckpoint(i,i);
        
			m_sys->prepareRollback(i);
			//loadTLB(from, to);
			loadRegState(i, i);
			m_sys->postRollback(i);
		}
		debugio_t::clearSymptomDetected();
		is_replay_phase = true;
		return;
#endif
		if(!hasDiagnosisStarted() ) 	//if diagnosis has not started start it
			startMulticoreDiagnosis();
		else {
			if(isLoggingPhase()) {

				//trigger TMR phase when the symptom is detected or when the LLB is full
				bool trigger = false;
				if(symptom_causing_core != -1) { //symptom_causing_core is set when the symptom is detected in Logging step
					trigger = true;
					reason_for_tmr_phase = SYMPTOM_IN_LOGGING_PHASE;
					symptom_core_logging_phase = symptom_causing_core;
				}
				//if symptom is detected in logging step then start the tmr step
				if(trigger) {
					DEBUG_OUT("TMR Phase is triggered due to Symptom in Logging Phase\n");
					computeDiagnosisLatency();
					triggerTMRPhase();
				} else 
					dont_continue_core = symptom_causing_core;	
			}
		}
	} else if(isLoggingPhase()) {

		bool trigger_tmr = false;

		//if the LLB is full in any one core, and no symptom is detected, then trigger TMR phase
		for(int i=0; i<numProcs; i++) {
//#define LLB_LIMIT
#ifdef LLB_LIMIT
			if(m_sys->isLLBFull(i))
#else
			if(current_inst_count[i] >= 100000)
#endif
			{
				if(CONCISE_VERBOSE)
					DEBUG_OUT("TMR Phase is triggered due to LLB Full\n");
				trigger_tmr = true;
				reason_for_tmr_phase = LLB_FULL; 
				break;
			}
		}

		if(trigger_tmr) {
			computeDiagnosisLatency();
			triggerTMRPhase();
		}
	}
	
	if(isTMRPhase()) {

		tmrPhase(core_id);
	}
}

void multicore_diagnosis_t::tmrPhase(int core_id)
{
	current_cycle_count[core_id]++;
	bool stop = false;
//#define COMPARE_EVERY_CYCLE
#ifdef COMPARE_EVERY_CYCLE
	compareCompareLog();
#endif

	//stop if the symptom is detected in all the cores
	bool all_cores_stopped = true;
	for(int i=0; i<numProcs; i++) {
		if(dont_run_core != i) { 
			//DEBUG_OUT("%d: stopped:%d ",i, core_stopped_in_tmr[i]);
			all_cores_stopped = all_cores_stopped & core_stopped_in_tmr[i];
		}
	}
	
	//DEBUG_OUT(" all stopped: %d\n", all_cores_stopped);
	stop = stop | all_cores_stopped;

	if(tmr_stop_requested)
		stop = true;

	if(stop) {
		if(MULTICORE_DIAGNOSIS_VERBOSE) {
			for(int i=0; i<numProcs; i++)
				if(dont_run_core != i)
					DEBUG_OUT(" current_inst_count[%d]=%d, num_inst_to_run[%d]=%d \n", i, current_inst_count[i], i, num_inst_to_run[i]);

			// do the comparision of the compareLog at the end of one TMR step in the TMR Phase
			DEBUG_OUT("This TMR Step is done\n");
		}

		//compareCompareLog();
		if(isLastReplay())
			compareLastReplayCompareLog();
		else
			compareReplayCompareLog();
		computeDiagnosisLatency(); 	//at the end of each tmr step compute diagnosis Latency
		//if stop diagnosis is called, then stop the diagnosis. Mem is courrupted and it might result in wrong diagnosis
		if(stop_diagnosis) {
			DEBUG_OUT(" Diagnosis is stopped!\n");
			DEBUG_OUT("No Divergence is detected\n\n");
			printStats();
			HALT_SIMULATION;
			return;
		}

		if(faulty_core_found) {
			return;
		} else if(suspect_found) {
			//start next phase
			triggerLastReplay();
		} else {
			//tmrPolicy();
			triggerLoggingPhase();
		}
	}
}

void multicore_diagnosis_t::compareLastReplayCompareLog()
{
	CRC16_FinishChecksum(crc_last_replay);

	//compare faulty_core_suspects, faulty_core_suspects+1, and faulty_core_suspects+2
	//we know that faulty_core_suspects+2 is good

	int good_core = (faulty_core_suspects+2)%numProcs;
	int suspect1 = faulty_core_suspects;
	int suspect2 = (faulty_core_suspects+1)%numProcs;

	DEBUG_OUT("good = %d, suspect1=%d, suspect2=%d\n", good_core, suspect1, suspect2);

	//comparing suspect2 and good

	//compare stores and pcs
	bool no_mismatch = true;
#ifdef CRC_CHECK
	if(llb_miss_last_replay || llb_miss[suspect2]) {
		if(llb_miss_last_replay && llb_miss[suspect2]) {
			if(llb_miss_last_replay_instr != llb_miss_instr[suspect2]) {
				DEBUG_OUT(" Faulty core is %d\n", suspect2);
				faulty_core_found = true;
				printStats();
				HALT_SIMULATION;
				return;
			} else {
				DEBUG_OUT("new:MM case\n");
				DEBUG_OUT(" Faulty core is %d\n", suspect1);
				faulty_core_found = true;
				printStats();
				HALT_SIMULATION;
				return;
			}
		} else if(llb_miss[suspect2]) {
			DEBUG_OUT(" Faulty core is %d\n", suspect2);
			faulty_core_found = true;
			printStats();
			HALT_SIMULATION;
			return;
		}
	} else if(symptom_raised_last_replay && (reason_for_tmr_phase==LLB_FULL)) {
		if(symptom_raised[suspect2]) {
			DEBUG_OUT("new:SS case\n");
			DEBUG_OUT(" Faulty core is %d\n", suspect1);
			faulty_core_found = true;
			printStats();
			HALT_SIMULATION;
			return;
		}
	}
	 else if(crc_last_replay != crc_tmr[suspect2]) {

		if(compare_log[suspect2].size() != compare_log_last_replay.size()) {
			DEBUG_OUT(" Check this case\n");
		}
		DEBUG_OUT("new: Faulty core is %d", suspect2);
		faulty_core_found = true;
		printStats();
		HALT_SIMULATION;
		return;
	} 
#else //CRC_CHECK
	//find out the numbers of log elements to compare
	int num_elements_to_compare = compare_log[suspect2].size();
	if(num_elements_to_compare > compare_log_last_replay.size()) {
		num_elements_to_compare = compare_log_last_replay.size();
	}

	for(int j=0; j<num_elements_to_compare; j++) {

		compare_log_element_t *temp;
		temp = new compare_log_element_t[2];

		temp[0] = compare_log[suspect2].at(j);	//element at position j from core i
		temp[1] = compare_log_last_replay.at(j);


		if(!areCompareLogElementsSame(temp[0], temp[1])) {	// if the elements are not same; a != b
			no_mismatch = false;	//there is a mismatch
			//if(CONCISE_VERBOSE)
			//	DEBUG_OUT(" compare_log_size = %d, compare_log_size_last_replay = %d\n", compare_log[suspect2].size(), compare_log_last_replay.size());
			if(CONCISE_VERBOSE)
				DEBUG_OUT(" mismatch at %d out of %d\n", j, num_elements_to_compare);
			DEBUG_OUT(" compareLog elements didnt match in core %d and %d\n", suspect2, good_core);
			if(temp[0].isNormal) DEBUG_OUT(" A is a normal\n");
			if(temp[0].isStore) DEBUG_OUT(" A is a store\n");
			if(temp[0].isBranch) DEBUG_OUT(" A is a branch\n");
			if(temp[0].isSymptom) DEBUG_OUT(" A is a symptom\n");

			DEBUG_OUT(" Faulty core is %d", suspect2);
			faulty_core_found = true;
			printStats();
			HALT_SIMULATION;
			return;
		} else {
			if(temp[0].pc == 0x6477) {
				DEBUG_OUT("MM case\n");
				DEBUG_OUT(" Faulty core is %d\n", suspect1);
				faulty_core_found = true;
				printStats();
				HALT_SIMULATION;
				return;
			}
			//special case: if reason_for_tmr_phase= LLB_FULL & the compareLog element is a symptom, then treat it similar to miss
			//TODO: Cross check this, i suspect something is missing here!!!
			if(temp[0].isSymptom && (reason_for_tmr_phase==LLB_FULL)) { // || reason_for_tmr_phase==SYMPTOM_IN_LOGGING_PHASE)) 
				DEBUG_OUT(" SS Case\n");
				DEBUG_OUT(" Faulty core is %d\n", suspect1);
				faulty_core_found = true;
				printStats();
				HALT_SIMULATION;
				return;
			} 
		}
		delete[] temp;
	}
	if(CONCISE_VERBOSE)
		DEBUG_OUT("compared %d elements\n", num_elements_to_compare);
#endif //CRC_CHECK

	if(!faulty_core_found) {
		DEBUG_OUT("Faulty core is %d\n", suspect1);
		faulty_core_found = true;
		printStats();
		HALT_SIMULATION;
		return;
	}
}

void multicore_diagnosis_t::compareReplayCompareLog()
{
	suspect_found = false;
	bool faulty_core_found = false;
	//compare logs on all the cores
	for(int i=0; i<numProcs; i++) {
		int previous;
		if(i-1 < 0) {
			previous = numProcs-1;
		} else {
			previous = i-1;
		}

		//find out the numbers of log elements to compare
		int num_elements_to_compare = compare_log[i].size();
		if(num_elements_to_compare > compare_log_logging[previous].size()) {
			num_elements_to_compare = compare_log_logging[previous].size();
		}

		//if(CONCISE_VERBOSE) {
		//	DEBUG_OUT("compare_log_size = %d, compare_log_size_logging = %d\n", compare_log[i].size(), compare_log_logging[previous].size());
		//}


		//compare stores and pcs
		bool no_mismatch = true;
		bool check = false;
#ifdef CRC_CHECK
		if(llb_miss[i]) {
			DEBUG_OUT(" llb miss in core %d and %d\n", i, previous);
			check = true;
		} else if(symptom_raised[i] && (reason_for_tmr_phase==LLB_FULL)) {
			DEBUG_OUT(" symptom in replay core %d and %d\n", i, previous);
			check = true;
		}
		 else if(crc_logging[previous] != crc_tmr[i]) {
			DEBUG_OUT(" crc mismatch in core %d and %d\n", i, previous);
			check = true;
		}
#else //CRC_CHECK
			for(int j=0; j<num_elements_to_compare; j++) {

				compare_log_element_t *temp;
				temp = new compare_log_element_t[2];

				temp[0] = compare_log[i].at(j);	//element at position j from core i
				temp[1] = compare_log_logging[previous].at(j);


				if(!areCompareLogElementsSame(temp[0], temp[1])) {	// if the elements are not same; a != b
					no_mismatch = false;	//there is a mismatch
					if(CONCISE_VERBOSE)
						DEBUG_OUT(" compare_log_size = %d, compare_log_size_logging = %d\n", compare_log[i].size(), compare_log_logging[previous].size());
					if(CONCISE_VERBOSE)
						DEBUG_OUT(" mismatch at %d out of %d\n", j, num_elements_to_compare);
					DEBUG_OUT(" compareLog elements didnt match in core %d and %d\n", i, previous);
					if(temp[0].isNormal) DEBUG_OUT(" A is a normal\n");
					if(temp[0].isStore) DEBUG_OUT(" A is a store\n");
					if(temp[0].isBranch) DEBUG_OUT(" A is a branch\n");
					if(temp[0].isSymptom) DEBUG_OUT(" A is a symptom\n");

					check = true;
					break;
				}

				delete[] temp;
			}
			if(CONCISE_VERBOSE)
				DEBUG_OUT("compared %d elements\n", num_elements_to_compare);
#endif //CRC_CHECK

		if(check) {
			if(suspect_found) {

				int faulty_core = -1;
				//suspects are fault_core_susptects, (fault_core_susptects+1)%numProcs, previous, and i
				if((faulty_core_suspects+1)%numProcs == previous) {
					faulty_core = previous;
					faulty_core_found = true;
				} else if(faulty_core_suspects == i) {
					faulty_core = i;
					faulty_core_found = true;
				} else {
					DEBUG_OUT(" Somethings is wrong\n");
				}
				if(faulty_core_found) {
					DEBUG_OUT("\n 2 mismatches found in this replay\n", faulty_core);
					DEBUG_OUT(" Faulty core is %d\n", faulty_core);
					printStats();
					HALT_SIMULATION;
					return;
				}

			} else {
				faulty_core_suspects = previous;
				suspect_found = true;
				//sanity check
				if( debugio_t::getFaultyCore() != faulty_core_suspects && debugio_t::getFaultyCore() != ((faulty_core_suspects+1)%numProcs) )  {
					DEBUG_OUT(" faulty core is not among suspects\n");
					DEBUG_OUT(" faulty=%d, suspects= %d, %d\n", debugio_t::getFaultyCore(), faulty_core_suspects, (faulty_core_suspects+1)%numProcs);
					printStats();
					HALT_SIMULATION;
					return;
				}
			}
		}
	}
}
void multicore_diagnosis_t::compareCompareLog()
{
	int num_elements_to_compare = 0;

	//find out the numbers of log elements to compare
	int min = 10000000;
	int min_core = -1;
	for(int i=0; i<numProcs; i++) {
		if(dont_run_core != i) {
			if(min > compare_log[i].size()) {
				min = compare_log[i].size();
				min_core = i;
				//DEBUG_OUT("min=%d, current_inst_count=%d, min_core=%d\n", min, compare_log[i].size(), min_core);
			}
		}
	}
	num_elements_to_compare = min;

	int suspect_core = -1;
	int num_suspects = 0;
	for(int i=0; i<numProcs; i++) {
		if(dont_run_core != i) {
			if(min != compare_log[i].size()) {
				suspect_core = i;
				num_suspects++;
			}
		}
	}

	if(SIVA_LOAD_BUFFER_DEBUG) {
		DEBUG_OUT("compare_log_size = %d, current_inst_count = %d\n", compare_log[0].size(), current_inst_count[0]);
		DEBUG_OUT("compare_log_size = %d, current_inst_count = %d\n", compare_log[1].size(), current_inst_count[1]);
		DEBUG_OUT("compare_log_size = %d, current_inst_count = %d\n", compare_log[2].size(), current_inst_count[2]);
		DEBUG_OUT("compare_log_size = %d, current_inst_count = %d\n", compare_log[3].size(), current_inst_count[3]);
	}

	//if(compare_now) { }
	{

		//compare stores and pcs
		//dont compare beyond a point when then cores are not supposed to run
		bool no_mismatch = true;
		for(int i=0; i<num_elements_to_compare; i++) {

			compare_log_element_t *temp;
			temp = new compare_log_element_t[numProcs];

			for(int i=0; i<numProcs; i++) {
				if(dont_run_core != i) {
					temp[i] = compare_log[i].front();
					compare_log[i].erase(compare_log[i].begin());
				}
			}

			//comparing a, b and c
			int a = (dont_run_core+1)%numProcs;
			int b = (dont_run_core+2)%numProcs;
			int c = (dont_run_core+3)%numProcs;

			//special case: if a is store and address == -1, then treat it as miss only if there was no such case during logging
			// need not check value in this case
			if(temp[a].isStore && (temp[a].address == -1)) {
				if(temp[b].isStore && (temp[b].address == -1)) {
					// no physical address found in the store instruction during logging
					if(!no_physical_address[current_tmr_core]) {
						if(temp[c].isStore && (temp[c].address == -1)) 
							DEBUG_OUT("NNN"); // no physical address in store in all the three cores during TMR
						else  // a != c, a == b
							DEBUG_OUT(" NN Case\n");
						DEBUG_OUT(" Faulty core is %d\n", current_tmr_core);
						printStats();
						HALT_SIMULATION;
						return;
					}
				}
			} else 

			if(!areCompareLogElementsSame(temp[a], temp[b])) {	// if the elements are not same; a != b
				no_mismatch = false;
				DEBUG_OUT(" compareLog elements didnt match in core %d and %d\n", a, b);
				if(temp[a].isNormal) DEBUG_OUT(" A is a normal\n");
				if(temp[a].isStore) DEBUG_OUT(" A is a store\n");
				if(temp[a].isBranch) DEBUG_OUT(" A is a branch\n");
				if(temp[a].isSymptom) DEBUG_OUT(" A is a symptom\n");

				if(areCompareLogElementsSame(temp[a], temp[c])) { //if a == c
					// 0x6477 says that there was a miss in the load buffer 
					if(temp[a].pc == 0x6477) {	//a==c but a != b
						DEBUG_OUT("MMH case\n");
						DEBUG_OUT(" Faulty core is %d\n", current_tmr_core);
					} else {	//a==c but a != b	a!= miss and c!= miss "b can be a miss" could be HHM case
						DEBUG_OUT(" compareLog elements match in core %d and %d\n", a, c);
						if(temp[b].pc == 0x6477) {
							DEBUG_OUT(" HHM Case\n");
						} else {
							DEBUG_OUT(" A == C A != B \n");
							if(temp[a].isSymptom) {
								DEBUG_OUT(" Reason: Symptom\n");
							} else {
								if(temp[b].isNormal || temp[b].isStore || temp[b].isBranch) 
									DEBUG_OUT(" Reason: Control\n");
								else
									if(temp[b].isSymptom)
										DEBUG_OUT(" Reason: Symptom\n");
							}
						}
						DEBUG_OUT(" Faulty core is %d\n", b);
					}
				}
				else { //a != b and a!=c
					DEBUG_OUT(" compareLog elements didnt match in core %d and %d\n", a, c);
					if(!areCompareLogElementsSame(temp[b] , temp[c]))	//b != c, a != b, a != c
						DEBUG_OUT(" WARNING: compareLog elements didnt match match in core %d and %d\n", b, c);
					else {
						DEBUG_OUT(" compareLog elements match in core %d and %d\n", b, c);
						if(temp[b].pc == 0x6477) 	//b==c but a != b
							DEBUG_OUT("MMH case\n");
						else  {
							if(temp[a].pc == 0x6477) {
								DEBUG_OUT(" HHM Case\n");
							} else {
								DEBUG_OUT(" B == C A != B \n");
								if(temp[b].isSymptom) {
									DEBUG_OUT(" Reason: Symptom\n");
								} else {
									if(temp[a].isNormal || temp[a].isStore || temp[a].isBranch) 
										DEBUG_OUT(" Reason: Control\n");
									else
										if(temp[a].isSymptom)
											DEBUG_OUT(" Reason: Symptom\n");
								}
							}
						}
	
					}
					DEBUG_OUT(" Faulty core is %d\n", a);
				}
				printStats();
				HALT_SIMULATION;
				return;
			} else {	//a == b
				if(temp[a].pc == 0x6477) {
					DEBUG_OUT("compareLog elements match in core %d and %d\n", a, b);
					if(areCompareLogElementsSame(temp[a], temp[c]))  // a == c , a == b
						DEBUG_OUT(" MMM Case\n");
					else  // a != c, a == b
						DEBUG_OUT(" MMH Case\n");
					DEBUG_OUT(" Faulty core is %d\n", current_tmr_core);
					printStats();
					HALT_SIMULATION;
					return;
				} else {
					if(!areCompareLogElementsSame(temp[a], temp[c])) { // a!= miss, b!= miss, a == b, a != c	
											// c could be a miss, this can be HHM case
						DEBUG_OUT("compareLog elements match in core %d and %d\n", a, b);
						DEBUG_OUT(" compareLog elements didnt match in core %d and %d\n", a, c);
						if(temp[c].pc == 0x6477) {
							DEBUG_OUT(" HHM Case\n");
						} else {
							if(temp[a].isSymptom) {
								DEBUG_OUT(" Reason: Symptom\n");
							} else {
								if(temp[c].isNormal || temp[c].isStore || temp[c].isBranch) 
									DEBUG_OUT(" Reason: Control\n");
								else
									if(temp[c].isSymptom)
										DEBUG_OUT(" Reason: Symptom\n");
							}
						}

						DEBUG_OUT(" Faulty core is %d\n", c);
						printStats();
						HALT_SIMULATION;
						return;
					}
				}

				//special case: if reason_for_tmr_phase= LLB_FULL & the compareLog element is a symptom, then treat it similar to miss
				if(temp[a].isSymptom && (reason_for_tmr_phase==LLB_FULL || reason_for_tmr_phase==SYMPTOM_IN_LOGGING_PHASE)) {
					DEBUG_OUT("Symptom: compareLog elements match in core %d and %d\n", a, b);
					if(areCompareLogElementsSame(temp[a], temp[c]))  // a == c , a == b
						DEBUG_OUT(" SSS Case\n");
					else  // a != c, a == b
						DEBUG_OUT(" SS Case\n");
					DEBUG_OUT(" Faulty core is %d\n", current_tmr_core);
					printStats();
					HALT_SIMULATION;
					return;
				} 
			}

			

			delete[] temp;
		}
		if(CONCISE_VERBOSE)
			DEBUG_OUT("compared %d elements\n", num_elements_to_compare);
		if(no_mismatch) {
			if(SIVA_LOAD_BUFFER_DEBUG) {
				DEBUG_OUT("\n No mismatch in last %d instructions!!\n", num_elements_to_compare);
				for(int i=0; i<numProcs; i++)
					DEBUG_OUT(" current_inst_count[%d]=%d, num_inst_to_run[%d]=%d, compare_log_size=%d \n", i, current_inst_count[i], i, num_inst_to_run[i], compare_log[i].size());
			}
		}
	}

#ifndef COMPARE_EVERY_CYCLE
	if(num_suspects == 1) {
		DEBUG_OUT("suspected a faulty core\n");
		DEBUG_OUT(" Faulty core is %d\n", suspect_core);
		printStats();
		HALT_SIMULATION;
		return;
	} else if(num_suspects == 2) {
		DEBUG_OUT("Two compareLogs are not equal to min\n");
		DEBUG_OUT(" Faulty core is %d\n", min_core);
		printStats();
		HALT_SIMULATION;
		return;
	}
#endif// COMPARE_EVERY_CYCLE
}

//compare compareLog entries during the TMR phase
#define MULTICORE_DIAGNOSIS_DEBUG 0
bool multicore_diagnosis_t::areCompareLogElementsSame(compare_log_element_t a, compare_log_element_t b)
{
	if(a.isStore) {	// a is a store instruction
		if(b.isStore) {
			if(a.pc == b.pc) {
				if((a.address == b.address) && (a.value == b.value)) {
					if(MULTICORE_DIAGNOSIS_DEBUG)
						DEBUG_OUT("matched store\n");
					return true;
				} 
			} 
			//DEBUG_OUT("Store mismatch: pcA=%llx, pcB=%llx, addressA=%llx, addressB=%llx, valueA=%llx, valueB=%llx\n", a.pc, b.pc, a.address, b.address, a.value, b.value);
		}
		DEBUG_OUT("Store mismatch: pcA=%llx, pcB=%llx, addressA=%llx, addressB=%llx, valueA=%llx, valueB=%llx\n", a.pc, b.pc, a.address, b.address, a.value, b.value);

	} else if(a.isBranch) {	// a is a branch instruciton
		if(b.isBranch) {
			if(a.pc == b.pc) {
				if(a.address == b.address) {
					if(MULTICORE_DIAGNOSIS_DEBUG)
						DEBUG_OUT(" target matched\n");
					return true;
				}
			}
			//DEBUG_OUT(" branch mismatch\n");
		} 
		DEBUG_OUT("Branch mismatch: pcA=%llx, pcB=%llx, addressA=%llx, addressB=%llx\n", a.pc, b.pc, a.address, b.address);

	} else if(a.isSymptom) { // a is a symptom
		if(b.isSymptom) {
			if(a.pc == b.pc) {
				if(MULTICORE_DIAGNOSIS_DEBUG)
					DEBUG_OUT(" Symptoms matched\n");
				return true;
			}
			DEBUG_OUT(" Symptom mismatched\n");
		} 
		DEBUG_OUT("Symptom mismatch: pcA=%llx, pcB=%llx\n", a.pc, b.pc);
	} else
	if(a.isNormal) {	// a is a normal instruction
		if(b.isNormal) {
			if(a.pc == b.pc) {
				if((a.value == b.value)) {
					if(MULTICORE_DIAGNOSIS_DEBUG)
						DEBUG_OUT("matched inst\n");
					return true;
				} 
			} 
			//DEBUG_OUT("Inst mismatch : pcA=%llx, pcB=%llx, valueA=%llx, valueB=%llx\n", a.pc, b.pc, a.value, b.value);
		}
		DEBUG_OUT("Inst mismatch : pcA=%llx, pcB=%llx, valueA=%llx, valueB=%llx\n", a.pc, b.pc, a.value, b.value);
	}
	return false;
}

void multicore_diagnosis_t::reportSequenceNumber(int core_id, uint64 pc)
{
	if(hasDiagnosisStarted()) {
		if(isLoggingPhase()) {
			//if(!m_sys->isLLBFull(core_id)) 	//corner case: When the llb is full and there are 
								//still instrucitons retiring from pseq.C then dont 
								//count those because they should not be run in TMR phase
				current_inst_count[core_id]++;
				//DEBUG_OUT("%d: seq_number = %d\n", core_id, current_inst_count[core_id]);
		} else {
			current_inst_count[core_id]++;
			
			if(current_inst_count[core_id] > num_inst_to_run[core_id]) {
				if(CONCISE_VERBOSE) {
					DEBUG_OUT("%d:Stopping because of instr limit %d ", core_id, current_inst_count[core_id]);
					DEBUG_OUT(" %d\n", num_inst_to_run[core_id]);
				}
				core_stopped_in_tmr[core_id] = true;
			}
			if(current_inst_count[core_id] >= no_tmr_after[core_id]) {
				if(CONCISE_VERBOSE) {
					DEBUG_OUT("%d:Stopping because of no tmr after %d \n", core_id, current_inst_count[core_id]);
				}
				core_stopped_in_tmr[core_id] = true;
			}
		}
	}
}
void multicore_diagnosis_t::notReportingSequenceNumber(int core_id, uint64 pc)
{
	previous_pc[core_id] = pc;
}

//#define COLLECT_DEST
//for collecting compare log of normal
void multicore_diagnosis_t::collectCompareLog(uint64 pc, uint64 value, int core_id, bool isNormal)
{
#ifdef COLLECT_DEST
	if(isNormal) {
		compare_log_element_t temp;
		temp.pc = pc;
		temp.isStore = false;
		temp.isBranch = false;
		temp.isNormal = true;
		temp.isSymptom = false;
		temp.value = value;
		temp.address = 0;

		if(isTMRPhase() && dont_run_core != core_id && !core_stopped_in_tmr[core_id]){
			if(current_inst_count[core_id] < num_inst_to_run[core_id]) {

				if(SIVA_LOAD_BUFFER_DEBUG)
					if(CONCISE_VERBOSE)
						DEBUG_OUT("%d: compare_log current_inst_number=%d pc=%llx, value=%llx, size=%d\n", core_id, current_inst_count[core_id], pc,value, compare_log[core_id].size() );

				if(isLastReplay()) {
					if(previous_pc[core_id] == pc && pc != -1) {		//hack because some times the same instruction is replayed twice
						DEBUG_OUT("%d:Normal:Correcting crc\n", core_id);
						crc_last_replay = crc_last_replay_older;
						compare_log_last_replay.pop_back();
					}
					compare_log_last_replay.push_back(temp);
					crc_last_replay_older = crc_last_replay;
					CRC16_UpdateChecksum(crc_last_replay, (void *)&value, 4);
				} else {
					if(previous_pc[core_id] == pc && pc != -1) {		//hack because some times the same instruction is replayed twice
						DEBUG_OUT("%d:Normal:Correcting crc\n", core_id);
						crc_tmr[core_id] = crc_tmr_older[core_id];
						compare_log[core_id].pop_back();
					}
					compare_log[core_id].push_back(temp);
					crc_tmr_older[core_id] = crc_tmr[core_id];
					CRC16_UpdateChecksum(crc_tmr[core_id], (void *)&value, 4);
				}
				if(SIVA_LOAD_BUFFER_DEBUG)
					DEBUG_OUT("%d: compare_log current_inst_count=%d pc=%llx, size=%d\n",core_id, current_inst_count[core_id], pc, compare_log[core_id].size());
			}
		}
		if(isLoggingPhase()){
			if(current_inst_count[core_id] < no_tmr_after[(core_id+1)%numProcs] || (no_tmr_after[(core_id+1)%numProcs] == -1))  {
				if(SIVA_LOAD_BUFFER_DEBUG)
					if(CONCISE_VERBOSE)
						DEBUG_OUT("%d: compare_log current_inst_number=%d pc=%llx, value=%llx, size=%d\n", core_id, current_inst_count[core_id], pc,value, compare_log_logging[core_id].size() );

				if(previous_pc[core_id] == pc && pc != -1) {		//hack because some times the same instruction is replayed twice
					compare_log_logging[core_id].pop_back();
				} 
				compare_log_logging[core_id].push_back(temp);
				CRC16_UpdateChecksum(crc_logging[core_id], (void *)&value, 4);
			}
		}
	}
#endif // COLLECT_DEST
}
	

//for collecting compare log of branch instructions
void multicore_diagnosis_t::collectCompareLog(uint64 pc, uint64 target, int core_id)
{
	compare_log_element_t temp;
	temp.pc = pc;
	temp.isNormal = false;
	temp.isStore = false;
	temp.isBranch = true;
	temp.isSymptom = false;
	temp.value = 0;
	temp.address = target;

	if(isTMRPhase() && dont_run_core != core_id && !core_stopped_in_tmr[core_id]){
		if(current_inst_count[core_id] < num_inst_to_run[core_id]) {
			if(SIVA_LOAD_BUFFER_DEBUG)
				if(CONCISE_VERBOSE)
					DEBUG_OUT("%d: compare_log current_inst_number=%d pc=%llx, addr=%llx, size=%d\n", core_id, current_inst_count[core_id], pc, target, compare_log[core_id].size() );

			if(isLastReplay()) {
				compare_log_last_replay.push_back(temp);
				if(previous_pc[core_id] == pc && pc != -1) {		//hack because some times the same instruction is replayed twice
					DEBUG_OUT("%d:Branch:Correcting crc\n", core_id);
					crc_last_replay = crc_last_replay_older;
				}
				crc_last_replay_older = crc_last_replay;
				CRC16_UpdateChecksum(crc_last_replay, (void *)&target, 4);
			} else {
				compare_log[core_id].push_back(temp);
				if(previous_pc[core_id] == pc && pc != -1) {		//hack because some times the same instruction is replayed twice
					DEBUG_OUT("%d:Branch:Correcting crc\n", core_id);
					crc_tmr[core_id] = crc_tmr_older[core_id];
				}
				crc_tmr_older[core_id] = crc_tmr[core_id];
				CRC16_UpdateChecksum(crc_tmr[core_id], (void *)&target, 4);
			}
			if(SIVA_LOAD_BUFFER_DEBUG)
				DEBUG_OUT("%d: compare_log current_inst_count=%d pc=%llx, size=%d\n",core_id, current_inst_count[core_id], pc, compare_log[core_id].size());
		}
	}

	if(isLoggingPhase()){
		if(current_inst_count[core_id] < no_tmr_after[(core_id+1)%numProcs] || (no_tmr_after[(core_id+1)%numProcs] == -1))  {
			compare_log_logging[core_id].push_back(temp);
			if(SIVA_LOAD_BUFFER_DEBUG)
			if(CONCISE_VERBOSE)
				DEBUG_OUT("%d: compare_log current_inst_number=%d pc=%llx, addr=%llx, size=%d\n", core_id, current_inst_count[core_id], pc, target, compare_log_logging[core_id].size() );
			CRC16_UpdateChecksum(crc_logging[core_id], (void *)&target, 4);
			num_branch_instr[core_id]++;
		}
	}
}
	

//for collecting compare log  for symptom
void multicore_diagnosis_t::collectCompareLog(uint64 pc, int core_id)
{
	compare_log_element_t temp;
	temp.pc = pc;
	temp.isStore = false;
	temp.isNormal = false;
	temp.isSymptom = true;
	temp.isBranch = false;

	if(isTMRPhase() && dont_run_core != core_id){
		if(current_inst_count[core_id] < num_inst_to_run[core_id]) {
			if(SIVA_LOAD_BUFFER_DEBUG)
			if(CONCISE_VERBOSE)
				DEBUG_OUT("%d: compare_log current_inst_count=%d pc=%llx, size=%d, num_inst_to_run=%d\n",core_id, current_inst_count[core_id], pc, compare_log[core_id].size(), num_inst_to_run[core_id]);
			if(isLastReplay()) {
				compare_log_last_replay.push_back(temp);
				if(pc == 0x6477) {
					llb_miss_last_replay = true;
					llb_miss_last_replay_instr = current_inst_count[core_id];
				} else 
					symptom_raised_last_replay = true;
			} else {
				compare_log[core_id].push_back(temp);
				if(pc == 0x6477) {
					llb_miss[core_id] = true;
					llb_miss_instr[core_id] = current_inst_count[core_id];
				} else 
					symptom_raised[core_id] = true;
			}
		}
	}
	if(isLoggingPhase()){
		compare_log_logging[core_id].push_back(temp);
		//llb miss is not possible in logging phase
		symptom_raised_logging[core_id] = true;
	}
}

//for collecting compare log  for stores
void multicore_diagnosis_t::collectCompareLog(uint64 pc, uint64 address, uint64 value, int core_id)
{
	compare_log_element_t temp;
	temp.pc = pc;
	temp.isNormal = false;
	temp.isStore = true;
	temp.isBranch = false;
	temp.isSymptom = false;
	temp.value = value;
	temp.address = address;

	if(isTMRPhase() && dont_run_core != core_id && !core_stopped_in_tmr[core_id]){
		if(current_inst_count[core_id] < num_inst_to_run[core_id]) {
				if(SIVA_LOAD_BUFFER_DEBUG)
			if(CONCISE_VERBOSE)
			DEBUG_OUT("%d: compare_log current_inst_number=%d pc=%llx, addr=%llx, value=%llx, size=%d\n", core_id, current_inst_count[core_id], pc,address, value, compare_log[core_id].size() );
			if(isLastReplay()) {
				compare_log_last_replay.push_back(temp);
				if(previous_pc[core_id] == pc && pc != -1) {		//hack because some times the same instruction is replayed twice
					DEBUG_OUT("%d:Stores:Correcting crc %llx\n", core_id, pc);
					crc_last_replay = crc_last_replay_older;
				}
				crc_last_replay_older = crc_last_replay;
				CRC16_UpdateChecksum(crc_last_replay, (void *)&value, 4);
				CRC16_UpdateChecksum(crc_last_replay, (void *)&address, 4);
			} else {
				compare_log[core_id].push_back(temp);
				if(previous_pc[core_id] == pc && pc != -1) {		//hack because some times the same instruction is replayed twice
					DEBUG_OUT("%d:Stores:Correcting crc %llx\n", core_id, pc);
					crc_tmr[core_id] = crc_tmr_older[core_id];
				}
				crc_tmr_older[core_id] = crc_tmr[core_id];
				CRC16_UpdateChecksum(crc_tmr[core_id], (void *)&value, 4);
				CRC16_UpdateChecksum(crc_tmr[core_id], (void *)&address, 4);
			}
		}
	}
	if(isLoggingPhase()){
		if((current_inst_count[core_id] < no_tmr_after[(core_id+1)%numProcs]) || (no_tmr_after[(core_id+1)%numProcs] == -1))  {
				if(SIVA_LOAD_BUFFER_DEBUG)
			if(CONCISE_VERBOSE)
			DEBUG_OUT("%d: compare_log current_inst_number=%d pc=%llx, addr=%llx, value=%llx, size=%d\n", core_id, current_inst_count[core_id], pc,address, value, compare_log_logging[core_id].size() );
			compare_log_logging[core_id].push_back(temp);
			CRC16_UpdateChecksum(crc_logging[core_id], (void *)&value, 4);
			CRC16_UpdateChecksum(crc_logging[core_id], (void *)&address, 4);
			num_store_instr[core_id]++;
		}
	}
}

void multicore_diagnosis_t::printStats()
{
	DEBUG_OUT("\nLogging Step:\n");
	for(int i=0; i<numProcs; i++) {
		DEBUG_OUT("Core %d: Num instructions = %d ", i, num_instructions[i]);
		DEBUG_OUT("llb size = %d\n", num_llb_entries[i]);

	}

	int num_tmr_steps = 0;
	for(int i=0; i<numProcs; i++) {
		if(checked_core[i]) {
			num_tmr_steps++;
		}
	}
	DEBUG_OUT("\nTMR Steps: %d\n", num_tmr_steps);
	for(int i=0; i<numProcs; i++) {
		if(checked_core[i]) {
			DEBUG_OUT(" TMR Step: %d ", i);
			if(current_tmr_core == i) {
				diagnosis_latency_instr += current_inst_count[i];
				DEBUG_OUT("Num instructions = %d\n", current_inst_count[i]);
			} else {
				diagnosis_latency_instr += num_instructions[i];
				DEBUG_OUT("Num instructions = %d\n", num_instructions[i]);
			}
		}
	}

	diagnosis_latency_instr += max(num_instructions, numProcs);

	DEBUG_OUT("\n Summary:\n\n");
	DEBUG_OUT(" Number of Logging Phases: %d\n", num_logging_phases);
	if(num_logging_phases > 1) {
		int temp_avg_logging_length = avg_logging_length / (num_logging_phases - 1);
		DEBUG_OUT(" Average Logging Length (instructions): %d\n", temp_avg_logging_length);
	}
	DEBUG_OUT(" Diagnosis Latency (instructions): %d\n", diagnosis_latency_instr);
	DEBUG_OUT(" Diagnosis Latency (cycles): %d\n", diagnosis_latency_cycles);
	DEBUG_OUT(" Diagnosis Latency Logging (cycles): %d\n", total_logging_latency_cycles);
	DEBUG_OUT(" Diagnosis Latency TMR (cycles): %d\n", diagnosis_latency_cycles-total_logging_latency_cycles);

	float overhead = (((float)diagnosis_latency_cycles/expected_diagnosis_latency_cycles) - 1)*100;
	DEBUG_OUT(" Squashing overhead (cycles): %f\n", overhead);

	DEBUG_OUT("\n");
	for(int i=0; i<numProcs; i++) {
		DEBUG_OUT(" Number of Sqashes: Core %d: %d\n", i, num_squashes[i]);
		DEBUG_OUT(" Num Load Instructions: Core %d: %d\n", i, total_num_load_instr[i]);
		DEBUG_OUT(" Num Branch Instructions: Core %d: %d\n", i, total_num_branch_instr[i]);
		DEBUG_OUT(" Num Store Instructions: Core %d: %d\n", i, total_num_store_instr[i]);
	}
	DEBUG_OUT("\n");
	DEBUG_OUT(" Total instructions in Logging phases\n");
	for(int i=0; i<numProcs; i++) {
		DEBUG_OUT("Logging instructions in Core %d: %d\n", i, total_logging_instructions[i]);
	}
	DEBUG_OUT("\n");
}

void multicore_diagnosis_t::reportAddressNotFound(int core_id, uint64 physical_address)
{
	//DEBUG_OUT("%d: reporting address not found\n", core_id);
	//DEBUG_OUT("%d: current_inst_count=%d num_inst_to_run=%d\n", core_id, current_inst_count[core_id], num_inst_to_run[core_id]);
	if(current_inst_count[core_id] < num_inst_to_run[core_id]) {	
		if(core_id != dont_run_core) {
			//if(SIVA_LOAD_BUFFER_DEBUG)
				DEBUG_OUT("%d: reporting address not found\n", core_id);
			DEBUG_OUT("%d: LLB Miss: (%d, %d)\n", core_id, current_inst_count[core_id], num_inst_to_run[core_id]);
			collectCompareLog(0x6477, core_id);	// 6477 - miss for load buffer miss
			missing_addresses_in_core[core_id] = true;
			core_stopped_in_tmr[core_id] = true; //stop the core since the faulty core should be identified by this info
			//DEBUG_OUT("%d:stoppting core \n", core_id);
			runMulticoreDiagnosisAlgorithm(core_id);
		}
	}
}

void multicore_diagnosis_t::reportFuncBufferMiss(int core_id)
//Treat func buffer empty similar to llb miss
{
	if(current_inst_count[core_id] < num_inst_to_run[core_id]) {	
		if(core_id != dont_run_core) {
			collectCompareLog(0x6477, core_id);	// 6477 - miss for load buffer miss
			//if(SIVA_LOAD_BUFFER_DEBUG)
				DEBUG_OUT("%d: reporting address not found\n", core_id);
			core_stopped_in_tmr[core_id] = true; //stop the core since the faulty core should be identified by this info
			missing_addresses_in_core[core_id] = true;
			//DEBUG_OUT("%d:stoppting core \n", core_id);
			runMulticoreDiagnosisAlgorithm(core_id);
		}
	}
}


void multicore_diagnosis_t::clearAddressNotFound() {
	for(int i=0; i<numProcs; i++)
		missing_addresses_in_core[i] = false;
	
}

void multicore_diagnosis_t::reportInterrupt(int core_id)
{
	if(m_num_interrupts > THRESHOLD_NUM_INTERRUPTS ) {
		DEBUG_OUT(" threshold on number of interrupts reached\n");
		HALT_SIMULATION;
	}
	if(isLoggingPhase()) {
		m_num_interrupts++;
		reason_for_logging_phase = INTERRUPT_IN_EXEC;
		triggerLoggingPhase();
	} else if(isTMRPhase()) {
		if(dont_run_core_id() != core_id) {
			m_num_interrupts++;
			DEBUG_OUT("%d:Triggering Logging phase on viewing an Interrupt\n", core_id);
			//checked_core[current_tmr_core] = false;
			//tmrPolicy();
			reason_for_logging_phase = INTERRUPT_IN_EXEC;
			triggerLoggingPhase();
		} else {
			DEBUG_OUT("%d:Ignoring the Interrupt since its on a dont_run_core\n", core_id);
		}
			
	}
}	

bool multicore_diagnosis_t::dont_continue(int core_id)
{
	if(isLoggingPhase()) {
		if(dont_continue_core == core_id)
			return true;
	}
	if(isTMRPhase()) {
		if(core_id == dont_run_core)
			return true;
		if(core_stopped_in_tmr[core_id])
			return true;
	}
	if(isLastReplay()) {
		if(core_id != (faulty_core_suspects+2)%numProcs)
			return true;
	}
	//DEBUG_OUT(" running core-id:%d\n", core_id);
	return false;
}

void multicore_diagnosis_t::freeCompareLog(int core_id)
{
	compare_log[core_id].clear();
	vector<compare_log_element_t>().swap(compare_log[core_id]);
}

void multicore_diagnosis_t::freeFuncInstBuffer(int core_id)
{
	func_inst_buffer[core_id].clear();
	vector <struct func_inst_info>().swap(func_inst_buffer[core_id]);
}

void multicore_diagnosis_t::setSymptomCausingCore(int core_id, uint64 trap_type ) 
{
	if(isLoggingPhase()) {
		num_symptom_detected++; 
		symptom_causing_core = core_id;
		DEBUG_OUT("%d: symptom : %d\n", core_id, current_inst_count[core_id]);
		collectCompareLog(trap_type, core_id);
	}
	if(isTMRPhase()) {
		if(dont_run_core != core_id) {
			if(current_inst_count[core_id] < num_inst_to_run[core_id]) {
				num_symptom_detected++; 
				symptom_causing_core = core_id;
				DEBUG_OUT("%d: symptom : %d, %d\n", core_id, current_inst_count[core_id], num_inst_to_run[core_id]);
				core_stopped_in_tmr[core_id] = true;
				//DEBUG_OUT("%d:stoppting core \n", core_id);
				collectCompareLog(trap_type, core_id);
				runMulticoreDiagnosisAlgorithm(core_id);
			}
		}
	}
	reportSequenceNumber(core_id);  //treat a symptom as an instruction retire. This is done for the cases when the last instruction is a symptom and this might cause TMR to not terminate
}

void multicore_diagnosis_t::resetStartLocalCycle()
{
	for(int i=0; i<numProcs; i++) {
		start_local_cycle[i] = m_sys->getLocalCycle(i);
	}
}

void multicore_diagnosis_t::resetStartSequenceNumber()
{
	for(int i=0; i<numProcs; i++) {
		start_sequence_number[i] = 0;//m_sys->getSequenceNumber(i);
	}
}

void multicore_diagnosis_t::resetNoPhysicalAddress()
{
	for(int i=0; i<numProcs; i++) {
		no_physical_address[i] = false;
	}
}

void multicore_diagnosis_t::computeDiagnosisLatency()
{
	if(isTMRPhase()) {
		uint64 max = 0;
		for(int i=0; i<numProcs; i++) {
			if(max < (m_sys->getLocalCycle(i) - start_local_cycle[i]))
				max = m_sys->getLocalCycle(i) - start_local_cycle[i];
		}

		diagnosis_latency_cycles += max;
		if(max < logging_latency_cycles)
			expected_diagnosis_latency_cycles += max;
		else
			expected_diagnosis_latency_cycles += logging_latency_cycles;
			
	} else
		if(isLoggingPhase()) {
			diagnosis_latency_cycles += m_sys->getLocalCycle(0) - start_local_cycle[0];
			logging_latency_cycles = m_sys->getLocalCycle(0) - start_local_cycle[0];
			expected_diagnosis_latency_cycles += logging_latency_cycles;
			total_logging_latency_cycles += logging_latency_cycles;
		}
}

void multicore_diagnosis_t::stopCore( int core_id)
{
	if(isTMRPhase()) {
		core_stopped_in_tmr[core_id] = true;
		//DEBUG_OUT("%d:stoppting core \n", core_id);
		if(!no_physical_address[core_id]) {
			collectCompareLog((uint64)0, (uint64)-1, (uint64)-1, (int)core_id);
		}
		reportSequenceNumber(core_id);
		runMulticoreDiagnosisAlgorithm(core_id);
	}
}

uint64 multicore_diagnosis_t::min(uint64 *list, int size)
{
	uint64 min = list[0];
	for(int i=1; i<size; i++) {
		if(list[i] < min)
			min = list[i];
	}
	return min;
}
