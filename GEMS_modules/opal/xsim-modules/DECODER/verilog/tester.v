module tester ;

parameter clkPeriod = 0.5 ;
// reg 	   reset;
reg [15:0] reset_cnt;
reg rclk ;
   
reg [31:0] instruction; //32-bit instruction that has been fetched
wire [5:0] o_rd;
wire [3:0] o_rd_type;
wire o_rd_valid;
wire [5:0] o_rd2;
wire [3:0] o_rd2_type;
wire o_rd2_valid;
wire [5:0] o_rs1;
wire [3:0] o_rs1_type;
wire o_rs1_valid;
wire [5:0] o_rs2;
wire [3:0] o_rs2_type;
wire o_rs2_valid;
wire [5:0] o_rs3;
wire [3:0] o_rs3_type;
wire o_rs3_valid;
wire [5:0] o_rs4;
wire [3:0] o_rs4_type;
wire o_rs4_valid;
wire [2:0] o_m_type; //
wire [3:0] o_m_futype;
wire [8:0] o_m_opcode;
wire [3:0] o_m_branch_type;
wire [63:0] o_m_imm;
wire [7:0] o_m_ccshift;
wire [7:0] o_m_access_size;
wire [7:0] o_m_flags;
wire o_fail;

decode_unit DUT(
	.instruction(instruction),
	// .reset(reset),
	.o_rd(o_rd),
	.o_rd_type(o_rd_type),
	.o_rd_valid(o_rd_valid),
	.o_rd2(o_rd2),
	.o_rd2_type(o_rd2_type),
	.o_rd2_valid(o_rd2_valid),
	.o_rs1(o_rs1),
	.o_rs1_type(o_rs1_type),
	.o_rs1_valid(o_rs1_valid),
	.o_rs2(o_rs2),
	.o_rs2_type(o_rs2_type),
	.o_rs2_valid(o_rs2_valid),
	.o_rs3(o_rs3),
	.o_rs3_type(o_rs3_type),
	.o_rs3_valid(o_rs3_valid),
	.o_rs4(o_rs4),
	.o_rs4_type(o_rs4_type),
	.o_rs4_valid(o_rs4_valid),
	.o_m_type(o_m_type),
	.o_m_futype(o_m_futype),
	.o_m_opcode(o_m_opcode),
	.o_m_branch_type(o_m_branch_type),
	.o_m_imm(o_m_imm),
	.o_m_ccshift(o_m_ccshift),
	.o_m_access_size(o_m_access_size),
	.o_m_flags(o_m_flags),
	.o_fail(o_fail)
	) ;

	initial begin
		$sdf_annotate("/dev/null", DUT) ;
	end

	always begin
		$readStimuli(instruction) ;
		// $display("VLOG: Input = %x", instruction) ;
		#clkPeriod rclk = 1 ;
		#clkPeriod rclk = 0 ;
		$writeLatch(
				o_rd,
				o_rd_type,
				o_rd_valid,
				o_rd2,
				o_rd2_type,
				o_rd2_valid,
				o_rs1,
				o_rs1_type,
				o_rs1_valid,
				o_rs2,
				o_rs2_type,
				o_rs2_valid,
				o_rs3,
				o_rs3_type,
				o_rs3_valid,
				o_rs4,
				o_rs4_type,
				o_rs4_valid,
				o_m_type,
				o_m_futype,
				o_m_opcode,
				o_m_branch_type,
				o_m_imm,
				o_m_ccshift,
				o_m_access_size,
				o_m_flags,
				o_fail
		) ;
		// $display("rd            %x\t%x\t%x", o_rd, o_rd_type, o_rd_valid);
		// $display("rd2           %x\t%x\t%x", o_rd2, o_rd2_type, o_rd2_valid);
		// $display("rs1           %x\t%x\t%x", o_rs1, o_rs1_type, o_rs1_valid);
		// $display("rs2           %x\t%x\t%x", o_rs2, o_rs2_type, o_rs2_valid);
		// $display("rs3           %x\t%x\t%x", o_rs3, o_rs3_type, o_rs3_valid);
		// $display("rs4           %x\t%x\t%x", o_rs4, o_rs4_type, o_rs4_valid);
		// $display("m_type        %x", o_m_type); //
		// $display("m_futype      %x", o_m_futype);
		// $display("m_opcode      %x", o_m_opcode);
		// $display("m_branch_type %x", o_m_branch_type);
		// $display("m_imm         %x", o_m_imm);
		// $display("m_ccshift     %x", o_m_ccshift);
		// $display("m_access_size %x", o_m_access_size);
		// $display("m_flags       %x", o_m_flags);
		// $display("fail          %x", o_fail);
	end

	/* Comment out for real simulation
	*
	* initial begin

	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;

	* instruction = 32'h808b0008 ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* $display("\ninstruction   %x", instruction); 
	* $display("rd            %x\t%x\t%x", o_rd, o_rd_type, o_rd_valid);
	* $display("rd2           %x\t%x\t%x", o_rd2, o_rd2_type, o_rd2_valid);
	* $display("rs1           %x\t%x\t%x", o_rs1, o_rs1_type, o_rs1_valid);
	* $display("rs2           %x\t%x\t%x", o_rs2, o_rs2_type, o_rs2_valid);
	* $display("rs3           %x\t%x\t%x", o_rs3, o_rs3_type, o_rs3_valid);
	* $display("rs4           %x\t%x\t%x", o_rs4, o_rs4_type, o_rs4_valid);
	* $display("m_type        %x", o_m_type); //
	* $display("m_futype      %x", o_m_futype);
	* $display("m_opcode      %x", o_m_opcode);
	* $display("m_branch_type %x", o_m_branch_type);
	* $display("m_imm         %x", o_m_imm);
	* $display("m_ccshift     %x", o_m_ccshift);
	* $display("m_access_size %x", o_m_access_size);
	* $display("m_flags       %x", o_m_flags);
	* $display("fail          %x", o_fail);

	* instruction = 32'h86102001 ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* $display("\ninstruction   %x", instruction); 
	* $display("rd            %x\t%x\t%x", o_rd, o_rd_type, o_rd_valid);
	* $display("rd2           %x\t%x\t%x", o_rd2, o_rd2_type, o_rd2_valid);
	* $display("rs1           %x\t%x\t%x", o_rs1, o_rs1_type, o_rs1_valid);
	* $display("rs2           %x\t%x\t%x", o_rs2, o_rs2_type, o_rs2_valid);
	* $display("rs3           %x\t%x\t%x", o_rs3, o_rs3_type, o_rs3_valid);
	* $display("rs4           %x\t%x\t%x", o_rs4, o_rs4_type, o_rs4_valid);
	* $display("m_type        %x", o_m_type); //
	* $display("m_futype      %x", o_m_futype);
	* $display("m_opcode      %x", o_m_opcode);
	* $display("m_branch_type %x", o_m_branch_type);
	* $display("m_imm         %x", o_m_imm);
	* $display("m_ccshift     %x", o_m_ccshift);
	* $display("m_access_size %x", o_m_access_size);
	* $display("m_flags       %x", o_m_flags);
	* $display("fail          %x", o_fail);

	* instruction = 32'h5000006 ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* $display("\ninstruction   %x", instruction); 
	* $display("rd            %x\t%x\t%x", o_rd, o_rd_type, o_rd_valid);
	* $display("rd2           %x\t%x\t%x", o_rd2, o_rd2_type, o_rd2_valid);
	* $display("rs1           %x\t%x\t%x", o_rs1, o_rs1_type, o_rs1_valid);
	* $display("rs2           %x\t%x\t%x", o_rs2, o_rs2_type, o_rs2_valid);
	* $display("rs3           %x\t%x\t%x", o_rs3, o_rs3_type, o_rs3_valid);
	* $display("rs4           %x\t%x\t%x", o_rs4, o_rs4_type, o_rs4_valid);
	* $display("m_type        %x", o_m_type); //
	* $display("m_futype      %x", o_m_futype);
	* $display("m_opcode      %x", o_m_opcode);
	* $display("m_branch_type %x", o_m_branch_type);
	* $display("m_imm         %x", o_m_imm);
	* $display("m_ccshift     %x", o_m_ccshift);
	* $display("m_access_size %x", o_m_access_size);
	* $display("m_flags       %x", o_m_flags);
	* $display("fail          %x", o_fail);

	* instruction = 32'h9928c00b ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* $display("\ninstruction   %x", instruction); 
	* $display("rd            %x\t%x\t%x", o_rd, o_rd_type, o_rd_valid);
	* $display("rd2           %x\t%x\t%x", o_rd2, o_rd2_type, o_rd2_valid);
	* $display("rs1           %x\t%x\t%x", o_rs1, o_rs1_type, o_rs1_valid);
	* $display("rs2           %x\t%x\t%x", o_rs2, o_rs2_type, o_rs2_valid);
	* $display("rs3           %x\t%x\t%x", o_rs3, o_rs3_type, o_rs3_valid);
	* $display("rs4           %x\t%x\t%x", o_rs4, o_rs4_type, o_rs4_valid);
	* $display("m_type        %x", o_m_type); //
	* $display("m_futype      %x", o_m_futype);
	* $display("m_opcode      %x", o_m_opcode);
	* $display("m_branch_type %x", o_m_branch_type);
	* $display("m_imm         %x", o_m_imm);
	* $display("m_ccshift     %x", o_m_ccshift);
	* $display("m_access_size %x", o_m_access_size);
	* $display("m_flags       %x", o_m_flags);
	* $display("fail          %x", o_fail);

	* instruction = 32'h1240004f ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* $display("\ninstruction   %x", instruction); 
	* $display("rd            %x\t%x\t%x", o_rd, o_rd_type, o_rd_valid);
	* $display("rd2           %x\t%x\t%x", o_rd2, o_rd2_type, o_rd2_valid);
	* $display("rs1           %x\t%x\t%x", o_rs1, o_rs1_type, o_rs1_valid);
	* $display("rs2           %x\t%x\t%x", o_rs2, o_rs2_type, o_rs2_valid);
	* $display("rs3           %x\t%x\t%x", o_rs3, o_rs3_type, o_rs3_valid);
	* $display("rs4           %x\t%x\t%x", o_rs4, o_rs4_type, o_rs4_valid);
	* $display("m_type        %x", o_m_type); //
	* $display("m_futype      %x", o_m_futype);
	* $display("m_opcode      %x", o_m_opcode);
	* $display("m_branch_type %x", o_m_branch_type);
	* $display("m_imm         %x", o_m_imm);
	* $display("m_ccshift     %x", o_m_ccshift);
	* $display("m_access_size %x", o_m_access_size);
	* $display("m_flags       %x", o_m_flags);
	* $display("fail          %x", o_fail);

	* instruction = 32'h b4102001 ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* #clkPeriod rclk = 1 ;
	* #clkPeriod rclk = 0 ;
	* $display("\ninstruction   %x", instruction); 
	* $display("rd            %x\t%x\t%x", o_rd, o_rd_type, o_rd_valid);
	* $display("rd2           %x\t%x\t%x", o_rd2, o_rd2_type, o_rd2_valid);
	* $display("rs1           %x\t%x\t%x", o_rs1, o_rs1_type, o_rs1_valid);
	* $display("rs2           %x\t%x\t%x", o_rs2, o_rs2_type, o_rs2_valid);
	* $display("rs3           %x\t%x\t%x", o_rs3, o_rs3_type, o_rs3_valid);
	* $display("rs4           %x\t%x\t%x", o_rs4, o_rs4_type, o_rs4_valid);
	* $display("m_type        %x", o_m_type); //
	* $display("m_futype      %x", o_m_futype);
	* $display("m_opcode      %x", o_m_opcode);
	* $display("m_branch_type %x", o_m_branch_type);
	* $display("m_imm         %x", o_m_imm);
	* $display("m_ccshift     %x", o_m_ccshift);
	* $display("m_access_size %x", o_m_access_size);
	* $display("m_flags       %x", o_m_flags);
	* $display("fail          %x", o_fail);

	end

	always @(instruction)
	begin
	end
	*/

endmodule

