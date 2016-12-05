module sparc_exu_aluaddsub
  (
   // Outputs
   adder_out, spr_out, alu_ecl_cout64_e_l, alu_ecl_cout32_e, 
   alu_ecl_adderin2_63_e, alu_ecl_adderin2_31_e, 
   // Inputs
   clk, se, byp_alu_rs1_data_e, byp_alu_rs2_data_e, ecl_alu_cin_e, 
   ifu_exu_invert_d
   );
   input clk;
   input se;
   input [63:0] byp_alu_rs1_data_e;   // 1st input operand
   input [63:0]  byp_alu_rs2_data_e;   // 2nd input operand
   input         ecl_alu_cin_e;           // carry in
   input         ifu_exu_invert_d;     // subtract used by adder

   output [63:0] adder_out; // result of adder
   output [63:0] spr_out;   // result of sum predict
   output         alu_ecl_cout64_e_l;
   output         alu_ecl_cout32_e;
   output       alu_ecl_adderin2_63_e;
   output       alu_ecl_adderin2_31_e;
   
   wire [63:0]  rs2_data;       // 2nd input to adder
   wire [63:0]  rs1_data;       // 1st input to adder
   wire [63:0]  subtract_d;
   // wire [63:0]  subtract_e;
   wire         cout64_e;
   
////////////////////////////////////////////
//  Module implementation
////////////////////////////////////////////
   assign       subtract_d[63:0] = {64{ifu_exu_invert_d}};
   // dff #(64) sub_dff(.din(subtract_d[63:0]), .clk(clk), .q(subtract_e[63:0]), .se(se),
   //                  .si(), .so());

   assign       rs1_data[63:0] = byp_alu_rs1_data_e[63:0];

   // assign       rs2_data[63:0] = byp_alu_rs2_data_e[63:0] ^ subtract_e[63:0];
   assign       rs2_data[63:0] = byp_alu_rs2_data_e[63:0] ^ subtract_d[63:0];
   
   assign      alu_ecl_adderin2_63_e = rs2_data[63];
   assign      alu_ecl_adderin2_31_e = rs2_data[31];
   sparc_exu_aluadder64 adder(.rs1_data(rs1_data[63:0]), .rs2_data(rs2_data[63:0]),
                              .cin(ecl_alu_cin_e), .adder_out(adder_out[63:0]),
                              .cout32(alu_ecl_cout32_e), .cout64(cout64_e));
   assign      alu_ecl_cout64_e_l = ~cout64_e;


   // sum predict
   sparc_exu_aluspr spr(.rs1_data(rs1_data[63:0]), .rs2_data(rs2_data[63:0]), .cin(ecl_alu_cin_e),
                        .spr_out(spr_out[63:0]));

endmodule // sparc_exu_aluaddsub
