module dff(din, clk, q, se, si, so);
// synopsys template

parameter SIZE = 1;

input	[SIZE-1:0]	din ;	// data in
input			clk ;	// clk or scan clk

output	[SIZE-1:0]	q ;	// output

input			se ;	// scan-enable
input	[SIZE-1:0]	si ;	// scan-input
output	[SIZE-1:0]	so ;	// scan-output

reg 	[SIZE-1:0]	q ;

`ifdef NO_SCAN
always @ (posedge clk)
  q[SIZE-1:0]  <= din[SIZE-1:0] ;
`else

always @ (posedge clk)

	q[SIZE-1:0]  <= (se) ? si[SIZE-1:0]  : din[SIZE-1:0] ;

assign so[SIZE-1:0] = q[SIZE-1:0] ;

`endif

endmodule // dff

