module sparc_exu_aluspr(
   // Outputs
   spr_out, 
   // Inputs
   rs1_data, rs2_data, cin
   );

input [63:0] rs1_data;
input [63:0] rs2_data;
   input     cin;
output [63:0] spr_out;

wire [63:0] rs1_data_xor_rs2_data;
wire [62:0] rs1_data_or_rs2_data;
wire [63:0] shift_or;

assign rs1_data_xor_rs2_data[63:0] = rs1_data[63:0] ^ rs2_data[63:0];
assign rs1_data_or_rs2_data[62:0] = rs1_data[62:0] | rs2_data[62:0];
assign shift_or[63:0] = {rs1_data_or_rs2_data[62:0],cin};

assign spr_out[63:0] = rs1_data_xor_rs2_data[63:0] ^ shift_or[63:0];

endmodule  // sparc_exu_aluspr
