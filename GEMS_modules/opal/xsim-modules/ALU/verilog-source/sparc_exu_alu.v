// ========== Copyright Header Begin ==========================================
// 
// OpenSPARC T1 Processor File: sparc_exu_alu.v
// Copyright (c) 2006 Sun Microsystems, Inc.  All Rights Reserved.
// DO NOT ALTER OR REMOVE COPYRIGHT NOTICES.
// 
// The above named program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License version 2 as published by the Free Software Foundation.
// 
// The above named program is distributed in the hope that it will be 
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public
// License along with this work; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// ========== Copyright Header End ============================================
////////////////////////////////////////////////////////////////////////
/*
//  Module Name: sparc_exu_alu
*/

module sparc_exu_alu
(
   // Outputs
   so, alu_byp_rd_data_e, exu_ifu_brpc_e, exu_lsu_ldst_va_e, 
   exu_lsu_early_va_e, exu_mmu_early_va_e, alu_ecl_add_n64_e, 
   alu_ecl_add_n32_e, alu_ecl_log_n64_e, alu_ecl_log_n32_e, 
   alu_ecl_zhigh_e, alu_ecl_zlow_e, exu_ifu_regz_e, exu_ifu_regn_e, 
   alu_ecl_adderin2_63_e, alu_ecl_adderin2_31_e, 
   alu_ecl_adder_out_63_e, alu_ecl_cout32_e, alu_ecl_cout64_e_l, 
   alu_ecl_mem_addr_invalid_e_l, 
   // Inputs
   rclk, se, si, byp_alu_rs1_data_e, byp_alu_rs2_data_e_l, 
   byp_alu_rs3_data_e, byp_alu_rcc_data_e, ecl_alu_cin_e, 
   ifu_exu_invert_d, ecl_alu_log_sel_and_e, ecl_alu_log_sel_or_e, 
   ecl_alu_log_sel_xor_e, ecl_alu_log_sel_move_e, 
   ecl_alu_out_sel_sum_e_l, ecl_alu_out_sel_rs3_e_l, 
   ecl_alu_out_sel_shift_e_l, ecl_alu_out_sel_logic_e_l, 
   shft_alu_shift_out_e, ecl_alu_sethi_inst_e, ifu_lsu_casa_e
   );
   input rclk;
   input se;
   input si;
   input [63:0] byp_alu_rs1_data_e;   // source operand 1
   input [63:0] byp_alu_rs2_data_e_l;  // source operand 2
   input [63:0] byp_alu_rs3_data_e;  // source operand 3
   input [63:0] byp_alu_rcc_data_e;  // source operand for reg condition codes
   input        ecl_alu_cin_e;            // cin for adder
   input        ifu_exu_invert_d;
   input  ecl_alu_log_sel_and_e;// These 4 wires are select lines for the logic
   input  ecl_alu_log_sel_or_e;// block mux.  They are active high and choose the
   input  ecl_alu_log_sel_xor_e;// output they describe.
   input  ecl_alu_log_sel_move_e;
   input  ecl_alu_out_sel_sum_e_l;// The following 4 are select lines for 
   input  ecl_alu_out_sel_rs3_e_l;// the output stage mux.  They are active high
   input  ecl_alu_out_sel_shift_e_l;// and choose the output of the respective block.
   input  ecl_alu_out_sel_logic_e_l;
   input [63:0] shft_alu_shift_out_e;// result from shifter
   input        ecl_alu_sethi_inst_e;
   input        ifu_lsu_casa_e; // this represents the selection input for the mux feeding the LSU
   
   output       so;
   output [63:0] alu_byp_rd_data_e;          // alu result
   
   output [47:0] exu_ifu_brpc_e;// branch pc output - corresponds to the output of the adder

   output [47:0] exu_lsu_ldst_va_e; // address for lsu (seems to be an adder output basically)
   output [10:3] exu_lsu_early_va_e; // faster bits for cache - TODO I do not quite get what this should be for, but it carries the remaining 
   					// bits of the 64 bit batch to be sent to lsu (however this does not represent the mux output bits, but 
					// the adder outputs - one of the data inputs of the mux)
   output [7:0]  exu_mmu_early_va_e; // the same above applies to these bits as well, they are just adder outputs 

   output        alu_ecl_add_n64_e; // regular outputs of the adder/subtractor - 64 and 32 bits
   output        alu_ecl_add_n32_e;
   
   output        alu_ecl_log_n64_e;
   output        alu_ecl_log_n32_e;
   output        alu_ecl_zhigh_e;
   output        alu_ecl_zlow_e;
   output    exu_ifu_regz_e;              /// set this output if reg condition codes are zero 
   output    exu_ifu_regn_e;		/// the MSB of reg condition codes TODO this was accepted as input - for what use?

   output    alu_ecl_adderin2_63_e;	// obviously the carry out for 32 and 64 bit portions of the adder
   output    alu_ecl_adderin2_31_e;


   // TODO how to filter these condition codes? Both go to ECL, obviously one set corresponds to adder outputs, the other set outputs
   // from other logic operations
   
   output    alu_ecl_adder_out_63_e;	// this corresponds to the MSB of the adder output - TODO how to use this?
   output    alu_ecl_cout32_e;       // To ecl of sparc_exu_ecl.v
   output    alu_ecl_cout64_e_l;       // To ecl of sparc_exu_ecl.v
   output    alu_ecl_mem_addr_invalid_e_l;// adder_out[63:48] not all 1 or all 0
                                
   wire         clk;
   wire [63:0] logic_out;       // result of logic block
   wire [63:0] adder_out;       // result of adder
   wire [63:0] spr_out;         // result of sum predict
   wire [63:0] zcomp_in;        // result going to zcompare
   wire [63:0] va_e;            // complete va
   wire [63:0] byp_alu_rs2_data_e;
   // wire        invert_e;
   wire        ecl_alu_out_sel_sum_e;
   wire        ecl_alu_out_sel_rs3_e;
   wire        ecl_alu_out_sel_shift_e;
   wire        ecl_alu_out_sel_logic_e;
   assign      clk = rclk;
   assign      byp_alu_rs2_data_e[63:0] = ~byp_alu_rs2_data_e_l[63:0];
   assign      ecl_alu_out_sel_sum_e = ~ecl_alu_out_sel_sum_e_l;
   assign      ecl_alu_out_sel_rs3_e = ~ecl_alu_out_sel_rs3_e_l;
   assign      ecl_alu_out_sel_shift_e = ~ecl_alu_out_sel_shift_e_l;
   assign      ecl_alu_out_sel_logic_e = ~ecl_alu_out_sel_logic_e_l;


   // TODO it seems there is a zero compare unit
   // checking the output of add/substract and logic units...


   //---------------------------------------------------------------------------------------------//
   // take a 64 bit register value, and see whther it is zero or not
   // 2 outputs to indicate whether the whole reg or lower 32 bits are zero
   // in this case, the 64 bit condition code is compared against zero, 
   // if zero set the ALU output exu_ifu_regz_e
   // TODO what does this indicate?
   //---------------------------------------------------------------------------------------------//
   // Zero comparison for exu_ifu_regz_e
   sparc_exu_aluzcmp64 regzcmp(.in(byp_alu_rcc_data_e[63:0]), .zero64(exu_ifu_regz_e));

   // exu_ifu_regn_e output carries the MSB of reg condition codes, but TODO what do this reg condition codes do exactly?
   assign     exu_ifu_regn_e = byp_alu_rcc_data_e[63];

   // TODO this looks very suspicious to me, you better use their synthesis scripts, what is the unit for this delay?


   // TODO the lsu va calculation - it must include a condition code that the result of operation belongs there
   // it seems they made use of synopsys templates as muxes, has this sth to do with alibs I have been using
   // double check their std cell library
   // the following is just a simple 2x1 mux: the selection (most probably comes from the ECL) ifu_lsu_casa_e, selects btw 
   // the output of the adder and a 63 bit wire - which I aasume corresponds to one of the inputs
   // the output of the mux represents a wire again, which should be assigned to an output
   // 48 bits of this value get assigned to an output - that represents the address for the LSU
   
   // TODO what is cas?
   
   // mux between adder output and rs1 (for casa) for lsu va
   dp_mux2es #(64)  lsu_va_mux(.dout(va_e[63:0]),
                               .in0(adder_out[63:0]),
                               .in1(byp_alu_rs1_data_e[63:0]),
                               .sel(ifu_lsu_casa_e));
   assign     exu_lsu_ldst_va_e[47:0] = va_e[47:0];
   // for bits 10:4 we have a separate bus that is not used for cas
   assign     exu_lsu_early_va_e[10:3] = adder_out[10:3];
   // mmu needs bits 7:0
   assign     exu_mmu_early_va_e[7:0] = adder_out[7:0];
   
   // it seems that the adder output is directly going to the branch predictor, no mux? TODO
   // how to know that the output from the ALU is to be used?

   // Adder
   assign     exu_ifu_brpc_e[47:0] = adder_out[47:0];
   assign     alu_ecl_adder_out_63_e = adder_out[63];

// the add-subtract unit: 
   sparc_exu_aluaddsub addsub(.adder_out(adder_out[63:0]),
                              // Outputs
			      // TODO result of sum predict
                              .spr_out  (spr_out[63:0]),
                              .alu_ecl_cout64_e_l(alu_ecl_cout64_e_l),
                              .alu_ecl_cout32_e(alu_ecl_cout32_e),
                              .alu_ecl_adderin2_63_e(alu_ecl_adderin2_63_e),
                              .alu_ecl_adderin2_31_e(alu_ecl_adderin2_31_e),
                              // Inputs
                              .clk      (clk),
		// this should just distinguish btw addition or substraction TODO
                              .se       (se),
			      // usual inputs...
                              .byp_alu_rs1_data_e(byp_alu_rs1_data_e[63:0]),
                              .byp_alu_rs2_data_e(byp_alu_rs2_data_e[63:0]),
                              .ecl_alu_cin_e(ecl_alu_cin_e),
			      // TODO does this indicate whether the output should be inverted or not?
			      // this involves substraction - 2's complement etc staff?
                              .ifu_exu_invert_d(ifu_exu_invert_d));

			      // TODO some logic here seems to be redundant -> for instance the sum prediction: I have to make sure that 
			      // no fault gets injected into this portions
			      
			      // is this sum_predict a LUT kind of mechanism? There is a DFF at the output of this unit, does this correspond
			      // to the pipeline latch at the output of the exec unit? Maybe, but the prediction involves some logic operations   	
			      // TODO what is the use of a dff here? invert_d serves what exactly? to keep track of the operation for the 
			      // current clock?

   // Logic/pass rs2_data
   // dff invert_d2e(.din(ifu_exu_invert_d), .clk(clk), .q(invert_e), .se(se), .si(), .so());
   sparc_exu_alulogic logic(.rs1_data(byp_alu_rs1_data_e[63:0]),
                            .rs2_data(byp_alu_rs2_data_e[63:0]),
                            .isand(ecl_alu_log_sel_and_e),
                            .isor(ecl_alu_log_sel_or_e),
                            .isxor(ecl_alu_log_sel_xor_e),
                            .pass_rs2_data(ecl_alu_log_sel_move_e),
                            // .inv_logic(invert_e), .logic_out(logic_out[63:0]),
                            .inv_logic(ifu_exu_invert_d), .logic_out(logic_out[63:0]),
                            .ifu_exu_sethi_inst_e(ecl_alu_sethi_inst_e));

		    
		    // TODO what is zcc exactly?

		    // TODO confused again - the output of the logic unit is compared against the output of the sum predict, then 
		    // the output of the mux is sent to zcomp unit? what does the selection signal indicate exactly? 
		    
   // Mux between sum predict and logic outputs for zcc
   dp_mux2es #(64)  zcompmux(.dout(zcomp_in[63:0]),
                           .in0(logic_out[63:0]),
                           .in1(spr_out[63:0]),
                           .sel(ecl_alu_out_sel_sum_e));

   // Zero comparison for zero cc
//   sparc_exu_aluzcmp64 zcccmp(.in(zcomp_in[63:0]), .zero64(alu_ecl_z64_e),
//                          .zero32(alu_ecl_z32_e));
   assign        alu_ecl_zlow_e = ~(|zcomp_in[31:0]);
   assign        alu_ecl_zhigh_e = ~(|zcomp_in[63:32]);

   // Get Negative ccs
   assign   alu_ecl_add_n64_e = adder_out[63];
   assign   alu_ecl_add_n32_e = adder_out[31];
   assign   alu_ecl_log_n64_e = logic_out[63];
   assign   alu_ecl_log_n32_e = logic_out[31];

   // so, the most significant bits serve for sign extraction only
   
   // The ALU does not contain the shifter, but the last stage does...
   // I collected all the code referenced but most of the code does not corerespond to the main ALU block!
   // see the shifter output entering the output mux e.g.!
   
   // Mux for output
   mux4ds #(64) output_mux(.dout(alu_byp_rd_data_e[63:0]), 
                         .in0(adder_out[63:0]),
                         .in1(byp_alu_rs3_data_e[63:0]),
                         .in2(shft_alu_shift_out_e[63:0]),
                         .in3(logic_out[63:0]), 
                         .sel0(ecl_alu_out_sel_sum_e),
                         .sel1(ecl_alu_out_sel_rs3_e),
                         .sel2(ecl_alu_out_sel_shift_e),
                         .sel3(ecl_alu_out_sel_logic_e));

		 // TODO this looks like a XOR tree based difference algorithm, but again I am not sure about how to use this...
		 // it just determines one of the outputs of the alu block

   // memory address checks
   sparc_exu_alu_16eql chk_mem_addr(.equal(alu_ecl_mem_addr_invalid_e_l),
                                    .in(va_e[63:47]));
   
endmodule  // sparc_exu_alu
