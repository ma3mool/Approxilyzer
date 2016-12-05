void dynamic_inst_t::Execute( void ) {
	ireg_t result; // golden result
	ireg_t inj_result; // injected result [low level simulator return value]
	ireg_t op1;
	ireg_t op2;
	ireg_t source; // source register for immediate instructions 
	bool infected_instr;

	char * ptr; //strtoull
	char injResultVec[REG_WIDTH];
	char op1Vec[REG_WIDTH];
	char op2Vec[REG_WIDTH];
	char op3Vec[REG_WIDTH];
	char opShiftVec[REG_WIDTH];
	char rccVec[REG_WIDTH];
	int invert;
	int se;
	int cin;
	type opShiftVal;
	type op3Val;
	type rccVal;			
	int and_sel;
	int or_sel;
	int xor_sel;
	int move_sel;
	int null_32;
	int lsu;
	int scan_in;
	// Output mux select control
	int sel_sum; 
	int sel_shift; 
	int sel_rs3;	 
	int sel_logic;

	int ecl_shft_op32_e;
	int ecl_shft_shift4_e;
	int ecl_shft_shift1_e;
	int ecl_shft_enshift_e_l;
	int ecl_shft_extendbit_e;
	int ecl_shft_extend32bit_e_l;
	int ecl_shft_lshift_e_l;


	// record when execution takes place
	m_event_times[EVENT_TIME_EXECUTE_DONE] = m_pseq->getLocalCycle() - m_fetch_cycle;

	// call the appropriate function
	static_inst_t *si = getStaticInst();
	// execute the instruction using the jump table
	pmf_dynamicExecute pmf = m_jump_table[si->getOpcode()];

	// LXL: Check to see if dest are ready
	checkDestReadiness();

	// If the exec unit type matches the fault type and the exec unit num is 1, inject the fault
	// exec_n is the ALU number to corrupt (now set to 1)
	// exec_u is numbered from 0, look at config/config.defaults
	bool inject_me = false; 
	int32 exec_u = getExecUnit() ;
	byte_t exec_n = getExecUnitNum() ;

	// FIXME ONLY INT ALU FAULTS CONDIDERED, MORE FAULT TYPES TO BE INCLUDED
	inject_me =  ( (m_fs->getFaultType() == INTALU_OUTPUT_FAULT) &&
			(exec_u == 1) &&
			(exec_n == m_fs->getFaultyReg()) ); 

	bool andType=false ;
	bool nandType = false ;
	bool norType = false ;
	bool orType = false ;
	bool xorType = false ;
	bool xnorType = false ;
	bool addType = false ;
	bool addcType = false ;
	bool subType = false ;
	bool subcType= false ;
	bool logic = false ;
	bool arithmetic= false ;
	bool shift=false;

	switch( si->getOpcode() ) {

		case i_and : 
		case i_andcc :
			andType = 1 ; logic = 1; break ;

		case i_or : 
		case i_orcc :
			orType = 1 ; logic = 1 ; break ;

		case i_andn : 
		case i_andncc :
			nandType = 1 ; logic = 1 ; break ;

		case i_orn : 
		case i_orncc :
			norType = 1 ; logic = 1 ; break ;

		case i_xor : 
		case i_xorcc :
			xorType = 1 ; logic = 1 ; break ;

		case i_xnor : 
		case i_xnorcc :
			xnorType = 1 ; logic = 1 ; break ;

		case i_add : 
		case i_addcc :
			addType = 1 ; arithmetic = 1 ; break ;

		case i_addc : 
		case i_addccc :
			addcType = 1 ; arithmetic = 1 ; break ;

		case i_sub : 
		case i_subcc :
			subType = 1 ; arithmetic = 1 ; break ;

		case i_subc : 
		case i_subccc :
			subcType = 1 ; arithmetic = 1 ; break ;

		case i_not:
		case i_mov:
			logic = 1; break;

		case i_sllx:
		case i_sll:
		case i_srlx:
		case i_srl:
		case i_srax:
		case i_sra:
			shift = 1; break;
	}

	infected_instr = arithmetic | logic | shift;
	inject_me = inject_me & infected_instr;

	this->inst_arith = arithmetic; 
	this->inst_logic = logic;

	if (inject_me) {
		reg_id_t &dest=getDestReg(0);
		if (!dest.isZero()) {
			dest.getARF()->setRFWakeup(false);
		}
	}

	(this->*pmf)();

	// The other thing to check if is the dest ready. In the case of 
	// func trap, the dest value is not yet set (even in the case of
	// cache miss). Then, we should wait till the dests become ready before
	// injecting the fault and waking up deps.
	bool dest_ready = false ;
	for( int i=0; i< SI_MAX_DEST; i++ ) {
		reg_id_t &dest = getDestReg(i) ;
		if( !dest.isZero() ) {
			dest_ready |= dest.getARF()->getReady(dest) ;
		}
	}
	// If dest is not ready, unset inject me, and undo damange with the 
	// setRFWakeup function()
	if (!dest_ready) {
		inject_me = false ;
		reg_id_t &dest=getDestReg(0);
		if (!dest.isZero()) {
			dest.getARF()->setRFWakeup(true);
		}
	}
	if( inject_me ) {

		// Inject a fault in the output register
		if( GSPOT_DEBUG ) { 
			DEBUG_OUT( "Injecting into " ) ;
			DEBUG_OUT( " instr %d\n",getSequenceNumber());
		}

		for(int i=0; i< SI_MAX_DEST; i++) { // LXL: keep upper limit to MAX_DEST, only inject 1 reg since i==0 
			reg_id_t &dest = getDestReg(i) ;            	
			if( !dest.isZero() ) { // Inject only if the destination is a valid register 

				if (i==0) {
					result = dest.getARF()->readInt64( dest ) ;	
					if( !m_fs->getDoFaultInjection() ){
						inj_result = result;
					}else if( m_fs->getFaultInjInst() >  m_fs->getRetInst() ){
						inj_result = result;
					} else { // Fault to be injected...
						// This just sets the current instruction (which is then to be used when 
						// collecting log info for the faulty instruction) stat_first_inject_i 
						m_fs->setCurrInst( getSequenceNumber() ) ;

						// Conduct custom saf simulation - this applies to i-add only!
						// I. Open pipes:
#if LXL_PROB_FM
						pattern_t fp = fSim.genPattern() ;
						if(PRADEEP_DEBUG_LL) {
							DEBUG_OUT("Generated pattern\n") ;
						}
						// DEBUG_OUT("Value = %llx ",result) ;
						// DEBUG_OUT("(%llx) (%llx)", fp.mask,fp.dir) ;
#if LXL_PROB_DIR_FM
						inj_result = (result&(~fp.mask))|fp.dir ;
#else
						inj_result = result^fp.mask ;
#endif
						// DEBUG_OUT(" -> %llx\n",inj_result) ;
#else //Vlog simulation
						if(fSim.counter == 0){
							char stimFIFO[NAME_SIZE];
							char responseFIFO[NAME_SIZE];
							string stim = m_fs->getStimPipe();
							string response = m_fs->getResponsePipe();

							strcpy(stimFIFO,stim.c_str());
							strcpy(responseFIFO,response.c_str());

							strcpy(fSim.writeFIFO,stimFIFO);
							strcpy(fSim.readFIFO,responseFIFO);

							fSim.fdWrite = open(fSim.writeFIFO, O_WRONLY|O_ASYNC);
							fSim.fdRead = open(fSim.readFIFO, O_RDWR|O_ASYNC);
						}

						// FIXME include all instrs applicable, filter redundancies

						// II. Depending on the instr at hand, prepare operands:
						op1 = getSourceReg(0).getARF()->readInt64( getSourceReg(0) );
						source = getSourceReg(1).getARF()->readInt64( getSourceReg(1) );
						op2 = (getStaticInst()->getFlag(SI_ISIMMEDIATE) ? getStaticInst()->getImmediate() : source);

						// Default values. Values over-written by instructions based on need
						invert = 0;
						cin = 0;							
						se = 0;
						opShiftVal = type(0);
						op3Val = type(0);
						rccVal = type(0);			
						and_sel = 0;
						or_sel = 0;
						xor_sel = 0;
						move_sel = 0;
						null_32 = 0;
						lsu = 0;
						scan_in = 0;	
						// Output mux select control
						// LXL: select signals are active low
						sel_sum = 1; 
						sel_shift = 1; 
						sel_rs3 = 1;	 
						sel_logic = 1;

						ecl_shft_op32_e=0;
						ecl_shft_shift4_e=0;
						ecl_shft_shift1_e=0;
						ecl_shft_enshift_e_l=1;
						ecl_shft_extendbit_e=0;
						ecl_shft_extend32bit_e_l=0;
						ecl_shft_lshift_e_l=0;


						if(addType){
							// invert op2
							// sparc ALU inverts the second operand automatically
							// xor with 64'hf basically
							invert = 0;
							and_sel = 1;
							sel_sum = 0; 
						} else if(addcType) { 
							// no cc change, use ICC_C 
							// ICC_C
							cin = (getSourceReg(2).getARF()->readInt64( getSourceReg(2) ) & 0x1 );
							invert = 0;						
							and_sel = 1;
							sel_sum = 0; 
						} else if(subType){ 
							// Subtraction conducted by 2's complement addition
							// since inversion of op2 is default, invert not to be set
							//LXL: to subtrace invert to 1, cin=0
							invert=1;
							cin = 1; // to enforce 2's complement add		
							and_sel = 1;
							sel_sum = 0; 
						} else if(subcType) {
							op2 += ( getSourceReg(2).getARF()->readInt64( getSourceReg(2) ) & 0x1 );
							// Subtraction conducted by 2's complement addition
							// since inversion of op2 is default, invert not to be set
							invert=1;
							cin = 1; // to enforce 2's complement add			
							and_sel = 1;
							sel_sum = 0; 
						} else if(andType) {
							// since inversion of op2 is default, invert to be set
							invert = 0;
							// Dummy 
							and_sel = 1;
							sel_logic = 0;	
						} else if(orType) {
							// since inversion of op2 is default, invert to be set
							invert = 0;
							or_sel = 1;
							sel_logic = 0;
						} else if(xorType) {
							// since inversion of op2 is default, invert to be set
							invert = 0;
							xor_sel = 1;
							sel_logic = 0;
						} else if(xnorType) {
							// since inversion of op2 is default, invert not to be set
							invert = 1;
							xor_sel = 1;
							sel_logic = 0;
						} if(si->getOpcode() == i_not) {
							// i_not is resolved as an xnor, having no immediate op2 and op2 = 0
							// since inversion of op2 is default, invert not to be set
							invert = 1;
							xor_sel = 1;
							sel_logic = 0;
						} else if(nandType) {
							// since inversion of op2 is default
							invert = 1;
							and_sel = 1;
							sel_logic = 0;		
						} else if(norType) {
							// since inversion of op2 is default
							invert = 1;
							or_sel = 1;
							sel_logic = 0;
						} else if(si->getOpcode() == i_mov) {
							invert = 1;
							move_sel = 1;
							sel_logic = 0;
						} else if(shift) {
							half_t opcode=si->getOpcode();
							bool arith_shift=0;
							bool extend64bit=0;

							ecl_shft_shift4_e= 1 << ((op2 & 0xc)>>2);
							ecl_shft_shift1_e= 1 << (op2&0x3);
							ecl_shft_enshift_e_l=0;

							if (!(opcode==i_sllx || opcode==i_sll)) {
								ecl_shft_lshift_e_l=1;
							}

							if (opcode==i_srax || opcode==i_sra) {
								arith_shift = 1;
							}

							if (opcode==i_sll || opcode==i_srl || opcode==i_sra) {
								ecl_shft_op32_e=1;
							} 

							ecl_shft_extend32bit_e_l = ~(ecl_shft_op32_e & ((op1>>31)&0x1) & 
									arith_shift) & 0x1;

							extend64bit = arith_shift & ((op1>>63)&0x1) & ~ecl_shft_op32_e;
							ecl_shft_extendbit_e = (extend64bit | ~ecl_shft_extend32bit_e_l) & 0x1;
							sel_shift = 0;

						}

						fSim.lluToBinStr(op1Vec, op1,REG_WIDTH);
						fSim.lluToBinStr(op2Vec, op2,REG_WIDTH);
						fSim.lluToBinStr(op3Vec, op3Val,REG_WIDTH);
						fSim.lluToBinStr(opShiftVec, opShiftVal,REG_WIDTH);
						fSim.lluToBinStr(rccVec, rccVal,REG_WIDTH);

						char shift4Vec[5], shift1Vec[5];
						fSim.lluToBinStr(shift4Vec, ecl_shft_shift4_e, 5);
						fSim.lluToBinStr(shift1Vec, ecl_shft_shift1_e, 5);
						// send stimuli over the write pipe				
						//fSim.nwrite = write(fSim.fdWrite, &se, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &op1Vec, REG_WIDTH);
						fSim.nwrite = write(fSim.fdWrite, &op2Vec, REG_WIDTH);
						fSim.nwrite = write(fSim.fdWrite, &cin, sizeof(int));
						//fSim.nwrite = write(fSim.fdWrite, &opShiftVec, REG_WIDTH);
						//fSim.nwrite = write(fSim.fdWrite, &op3Vec, REG_WIDTH);
						fSim.nwrite = write(fSim.fdWrite, &invert, sizeof(int));	
						fSim.nwrite = write(fSim.fdWrite, &and_sel, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &or_sel, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &xor_sel, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &move_sel, sizeof(int));	
						fSim.nwrite = write(fSim.fdWrite, &sel_sum, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &sel_shift, sizeof(int));
						//fSim.nwrite = write(fSim.fdWrite, &sel_rs3, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &sel_logic, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &null_32, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &ecl_shft_op32_e, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &shift4Vec, 5);
						fSim.nwrite = write(fSim.fdWrite, &shift1Vec, 5);
						fSim.nwrite = write(fSim.fdWrite, &ecl_shft_enshift_e_l, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &ecl_shft_extendbit_e, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &ecl_shft_extend32bit_e_l, sizeof(int));
						fSim.nwrite = write(fSim.fdWrite, &ecl_shft_lshift_e_l, sizeof(int));

						//fSim.nwrite = write(fSim.fdWrite, &rccVec, REG_WIDTH);
						//fSim.nwrite = write(fSim.fdWrite, &lsu, sizeof(int));
						//fSim.nwrite = write(fSim.fdWrite, &scan_in, sizeof(int));
						// ncsim handles stimuli sent and starts simulation
						// it is already invoked as an independent process		
						// wait for results over the read pipe
						fSim.nread = read(fSim.fdRead, &injResultVec, REG_WIDTH);
						inj_result = strtoull(injResultVec, &ptr, 2);
#endif //LXL_PROB_FM


#define GEN_TB 0
						if (GEN_TB) {
							DEBUG_OUT("byp_alu_rs1_data_e_latch = 64'h%llx;\n",op1);
							DEBUG_OUT("byp_alu_rs2_data_e_l_latch = 64'h%llx;\n ",op2);
							DEBUG_OUT("ecl_alu_cin_e_latch = 1'b%d;\n",cin);
							DEBUG_OUT("shft_alu_shift_out_e_latch = 64'h%llx;\n",opShiftVal);
							DEBUG_OUT("byp_alu_rs3_data_e_latch = 64'h%llx;\n",op3Val);
							DEBUG_OUT("ifu_exu_invert_d = 1'b%d;\n",invert);	
							DEBUG_OUT("ecl_alu_log_sel_and_e_latch = 1'b%d;\n",and_sel);
							DEBUG_OUT("ecl_alu_log_sel_or_e_latch = 1'b%d;\n",or_sel);
							DEBUG_OUT("ecl_alu_log_sel_xor_e_latch  = 1'b%d;\n    ",xor_sel);
							DEBUG_OUT("ecl_alu_log_sel_move_e_latch  = 1'b%d;\n   ",move_sel);	
							DEBUG_OUT("ecl_alu_out_sel_sum_e_l_latch  = 1'b%d;\n  ",sel_sum);
							DEBUG_OUT("ecl_alu_out_sel_shift_e_l_latch  = 1'b%d;\n",sel_shift);
							DEBUG_OUT("ecl_alu_out_sel_rs3_e_l_latch  = 1'b%d;\n  ",sel_rs3);
							DEBUG_OUT("ecl_alu_out_sel_logic_e_l_latch  = 1'b%d;\n",sel_logic);
							DEBUG_OUT("byp_alu_rcc_data_e_latch   = 64'h%llx;\n",rccVec);
							DEBUG_OUT("ifu_lsu_casa_e_latch = 1'b%d;\n",lsu);
							DEBUG_OUT("#clkPeriod  rclk = ~rclk;\n");
							DEBUG_OUT("#clkPeriod  rclk = ~rclk;\n");
							DEBUG_OUT("$display(\"rs1 %%x rs2 %%x\",byp_alu_rs1_data_e,byp_alu_rs2_data_e_l);\n");
							DEBUG_OUT("$display(\"result %%x\",alu_byp_rd_data_e);\n\n");
						}
						// keep number of invocations
						fSim.counter++;						
						// TO BE USED FOR DEBUGGING
#ifdef COMPARE
						ireg_t golden = result;
						ireg_t hls_result;
						int bit = 0;
						if ( bit > (sizeof(uint64)<<3) ) {
							hls_result = golden;	
						} else {
							hls_result = golden & (~((uint64)(1)<<bit));
						}
						if( result != inj_result ) {
							//DEBUG_OUT("priv %d seq_num %lld\t",m_priv,seq_num);
							DEBUG_OUT("//send %s 0x%llx 0x%llx over pipe\n",decode_opcode(s->getOpcode()),op1,op2);
							DEBUG_OUT("Golden value = %llx\n", result) ;
							DEBUG_OUT("low level = %llx\n", inj_result) ;
							// 							DEBUG_OUT("High level = %llu\n", hls_result) ;

							// HALT_SIMULATION;
						} else {
							DEBUG_OUT("PASSED!\n\n", golden);
						}
#endif // end_compare
						this->infectedResult = inj_result;
						this->goldenResult = result; 

						// If there is a fault, mark this instruction and the physical reg as faulty
						if( result != inj_result ) {
							if( PRADEEP_DEBUG_LL ) {
								DEBUG_OUT("dest t%d flat%d phys%d\n",dest.getRtype(),dest.getFlatRegister(),dest.getPhysical());

								DEBUG_OUT("%llx\t", result) ;
								DEBUG_OUT("%llx\n", inj_result) ;
							}
							// FIXME: Correlation btw bits to be accounted for here
							fSim.numBitFlips(result,inj_result);
							//FAULT_STAT_OUT("I:%llx\n", getVPC()) ;
							m_fs->incTotalInj(); // an injection-counter gets updated only
							m_fs->injectSafFault();
#ifndef ACT_STATS	
							this->setFault() ;
							dest.getARF()->writeInt64( dest, inj_result ) ;
							dest.getARF()->setFault(dest);

							this->infectedALU = true;
#endif // ACT_STATS
						} else {
							dest.getARF()->unSetCorrupted(dest) ;
							dest.getARF()->clearFault(dest);
							dest.getARF()->clearReadFault(dest);

							m_fs->incTotalMask(); // a masked-counter gets incremented only

							//------------- TO DIFFERENTIATE BTW RETIRING AND SQUASHED INSTR
							this->uMaskedALU = true;
							this->infectedALU = true;
							//}
						}
					}
				}
				dest.getARF()->setRFWakeup(true);
				dest.getARF()->wakeDependents(dest);
			} // end: destination was a valid reg
		}
	} else {// end if(inject_me)
		for( int i=0; i< SI_MAX_DEST ; i++ ) { //Changed from MAX_DST to 1
			reg_id_t &dest = getDestReg(i) ;
			if( !dest.isZero() ) { // Inject only if the destination is a valid register
				dest.getARF()->unSetCorrupted(dest) ;
				dest.getARF()->clearFault(dest);
				dest.getARF()->clearReadFault(dest);
			}
		}
	}

	// check that this instruction has written its destination registers
	if ( getTrapType() == Trap_NoTrap ) {
		ASSERT( getDestReg(0).getARF()->isReady( getDestReg(0) ));
		// stxa's write the control registers at retire time (when they become 
		// non-speculative). So they need a conditional check.
		// See memory_inst_t::Retire() for more information (9/12/2002)
		if (getStaticInst()->getOpcode() != i_stxa) {
			ASSERT( getDestReg(1).getARF()->isReady( getDestReg(1) ));
		}
	}
	propagateFault();
	SetStage(COMPLETE_STAGE);
}


