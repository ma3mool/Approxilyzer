# README for verilog files of the DECODER
# 11/18/2008 - Pradeep Ramachandran

Verilog files
-------------
	verilog/decode_unit.v : Behavioral module of decode unit
	verilog/macrof_defs.h : Defines all the macros for the decode unit
	verilog/decode_unit_flat.v : Flattened gate level netlist of synthesized decoder unit
	verilog/decode_unit_flag.sdf : SDF file of flattened netlist
	verilog/process_sdf.sh : Shell script to remove posedge/negedge from the
								.SDF file. The max delay is now 1ns in the
								design.

VPI files
---------
	vpi/hand_shake.c : The interface is there in this file. Remember that the width of the
						arrays that send values across the VPI must be size in verilog+1 to
						account for the extra '\0' character

Other comments
--------------

1. The map between opal's register and simics' register is still a look-up
table. Since the params of this lookup vary by instruction, I was not able to
remove these.

2. I don't think that we can do P-model and PD-model simulations for the
decoder as it has multiple outputs, as opposed to the single output of the AGEN
and the ALU. Although, one could do this for some particular fields. So, I
haven't yet put in any code that generates the patterns, etc. Similarly, the
problem comes with the exec and the retiring bit flips.
