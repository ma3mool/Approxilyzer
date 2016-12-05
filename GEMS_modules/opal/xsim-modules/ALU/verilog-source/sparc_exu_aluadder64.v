module sparc_exu_aluadder64
  (
   rs1_data,
   rs2_data,
   cin,
   adder_out,
   cout32,
   cout64
   );

   input [63:0]  rs1_data;   // 1st input operand
   input [63:0]  rs2_data;   // 2nd input operand
   input         cin;           // carry in

   output [63:0] adder_out; // result of adder
   output         cout32;         // Cout from lower 32 bit add
   output         cout64;         // cout from 64 bit add


////////////////////////////////////////////
//  Module implementation
////////////////////////////////////////////

   assign      {cout32, adder_out[31:0]} = rs1_data[31:0]+rs2_data[31:0]+
                                           cin;
   assign      {cout64, adder_out[63:32]} = rs1_data[63:32] 
               + rs2_data[63:32] + cout32;

endmodule // sparc_exu_aluadder64
