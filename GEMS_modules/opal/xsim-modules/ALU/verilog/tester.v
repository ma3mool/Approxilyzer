`timescale 1ns/1ps
module tester;
parameter clkPeriod = 0.5;
//parameter numPipeLineStages = 1;
reg rclk;
reg se;
reg si;
reg [63:0] byp_alu_rs1_data_e;
reg [63:0] byp_alu_rs2_data_e;
reg ecl_alu_cin_e;
reg [63:0] byp_alu_rs3_data_e;
reg [63:0] byp_alu_rcc_data_e;
reg ifu_exu_invert_d;
reg  ecl_alu_log_sel_and_e;
reg  ecl_alu_log_sel_or_e;
reg  ecl_alu_log_sel_xor_e;
reg  ecl_alu_log_sel_move_e; 
reg  ecl_alu_out_sel_sum_e_l;
reg  ecl_alu_out_sel_rs3_e_l;
reg  ecl_alu_out_sel_shift_e_l;
reg  ecl_alu_out_sel_logic_e_l;
reg [63:0] shft_alu_shift_out_e;
reg ecl_alu_sethi_inst_e;
reg ifu_lsu_casa_e;
   reg ecl_shft_op32_e;		   
   reg [3:0] ecl_shft_shift4_e;	   
   reg [3:0] ecl_shft_shift1_e;	   
   reg ecl_shft_enshift_e_l;	   
   reg ecl_shft_extendbit_e;	   
   reg ecl_shft_extend32bit_e_l;
   reg ecl_shft_lshift_e_l;

wire [63:0] alu_byp_rd_data_e;

   sparc_exu DUT(/*AUTOINST*/
                 // Outputs
                 .alu_byp_rd_data_e (alu_byp_rd_data_e[63:0]),
                 // Inputs
                 .byp_alu_rs1_data_e(byp_alu_rs1_data_e[63:0]),
                 .byp_alu_rs2_data_e(byp_alu_rs2_data_e[63:0]),
                 .ecl_alu_cin_e     (ecl_alu_cin_e),
                 .ifu_exu_invert_d  (ifu_exu_invert_d),
                 .ecl_alu_log_sel_and_e(ecl_alu_log_sel_and_e),
                 .ecl_alu_log_sel_or_e(ecl_alu_log_sel_or_e),
                 .ecl_alu_log_sel_xor_e(ecl_alu_log_sel_xor_e),
                 .ecl_alu_log_sel_move_e(ecl_alu_log_sel_move_e),
                 .ecl_alu_out_sel_sum_e_l(ecl_alu_out_sel_sum_e_l),
				 .ecl_alu_out_sel_shift_e_l(ecl_alu_out_sel_shift_e_l),
                 .ecl_alu_out_sel_logic_e_l(ecl_alu_out_sel_logic_e_l),
                 .ecl_alu_sethi_inst_e(ecl_alu_sethi_inst_e),
				 .ecl_shft_op32_e(ecl_shft_op32_e),		   
				 .ecl_shft_shift4_e(ecl_shft_shift4_e),	   
				 .ecl_shft_shift1_e(ecl_shft_shift1_e),	   
				 .ecl_shft_enshift_e_l(ecl_shft_enshift_e_l),	   
				 .ecl_shft_extendbit_e(ecl_shft_extendbit_e),	   
				 .ecl_shft_extend32bit_e_l(ecl_shft_extend32bit_e_l),
				 .ecl_shft_lshift_e_l(ecl_shft_lshift_e_l));

   
initial begin
   $sdf_annotate("/dev/null",DUT);
   se = 1'b0;
   byp_alu_rs1_data_e = 64'h0;
   byp_alu_rs2_data_e = 64'h0;
   ecl_alu_cin_e = 1'b0;
   shft_alu_shift_out_e = 64'h0;
   byp_alu_rs3_data_e = 64'h0;
   ifu_exu_invert_d = 1'b1;
   ecl_alu_log_sel_and_e = 1'b0;
   ecl_alu_log_sel_or_e = 1'b1;
   ecl_alu_log_sel_xor_e  = 1'b0;
   ecl_alu_log_sel_move_e  = 1'b0;
   ecl_alu_out_sel_sum_e_l  = 1'b0;
   ecl_alu_out_sel_shift_e_l  = 1'b1;
   ecl_alu_out_sel_rs3_e_l  = 1'b1;
   ecl_alu_out_sel_logic_e_l  = 1'b1;
   ifu_lsu_casa_e = 1'b0;
   si = 1'b0;
   rclk = 1;
   #clkPeriod  rclk = ~rclk;
   #clkPeriod  rclk = ~rclk;
   #clkPeriod  rclk = ~rclk;
   #clkPeriod  rclk = ~rclk;
end // initial begin



   always begin
	  $readStimuli(
				   //se,
				   byp_alu_rs1_data_e,
 				   byp_alu_rs2_data_e,
				   ecl_alu_cin_e, 
				   //shft_alu_shift_out_e, 
				   //byp_alu_rs3_data_e, 
 				   ifu_exu_invert_d, 
				   ecl_alu_log_sel_and_e, 
				   ecl_alu_log_sel_or_e, 
				   ecl_alu_log_sel_xor_e, 
				   ecl_alu_log_sel_move_e, 
				   ecl_alu_out_sel_sum_e_l, 
				   ecl_alu_out_sel_shift_e_l, 
				   //ecl_alu_out_sel_rs3_e_l, 
				   ecl_alu_out_sel_logic_e_l, 
				   ecl_alu_sethi_inst_e,
				   //byp_alu_rcc_data_e,
				   //ifu_lsu_casa_e,
				   //si
				   ecl_shft_op32_e,		   
				   ecl_shft_shift4_e,	   
				   ecl_shft_shift1_e,	   
				   ecl_shft_enshift_e_l,	   
				   ecl_shft_extendbit_e,	   
				   ecl_shft_extend32bit_e_l,
				   ecl_shft_lshift_e_l);
	  //repeat(numPipeLineStages) begin
	  #clkPeriod;
	  rclk = 1;
	  #clkPeriod;
	  rclk = 0;
	  //end
	  //$display("rs1 %x rs2 %x",byp_alu_rs1_data_e,byp_alu_rs2_data_e_l);
	  //$display("writing latch %x",alu_byp_rd_data_e);
	  $writeLatch(
				  alu_byp_rd_data_e
				  );
   end
endmodule
