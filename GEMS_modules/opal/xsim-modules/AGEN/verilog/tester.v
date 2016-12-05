module tester ;
parameter clkPeriod = 0.5 ;
reg [63:0] op1 ;
reg [63:0] s1 ;
reg use_imm ;
reg [63:0] imm ;
   reg 	   reset;
   reg [15:0] reset_cnt;
   

wire [63:0] addr ;
reg rclk ;

sparc_agen DUT(.address(addr),
		.op1(op1),
		.s1(s1),
		.imm(imm),
		.use_imm(use_imm)
		) ;

   initial begin
	  $sdf_annotate("/dev/null", DUT) ;

	  op1 = 64'h0 ;
	  s1 = 64'h0 ;
	  use_imm = 1'b0 ;
	  imm = 64'h0 ;
	  rclk = 1;
	  reset = 1;
	  reset_cnt=5;
   end

   always begin
	  if (reset) begin
		 reset_cnt <= reset_cnt -1;
		 #clkPeriod ;
		 rclk = 1 ;
		 #clkPeriod ;
		 rclk = 0 ;
		 if (reset_cnt==16'b0) reset <=0;
	  end
	  else begin
	     //$display("addr %x,", addr) ;
		 $readStimuli( op1, s1, imm, use_imm) ;
		 #clkPeriod ;
		 rclk = 1 ;
		 #clkPeriod ;
		 rclk = 0 ;
		 //$display("addr %x,", addr) ;
		 $writeLatch(addr) ;
	  end
   end

endmodule
