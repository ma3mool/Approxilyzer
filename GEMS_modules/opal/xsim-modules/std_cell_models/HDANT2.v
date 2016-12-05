// 
// Copyright (C) 2003 Virtual Silicon Technology Inc. All Rights Reserved.
// Silicon Ready, The Heart of Great Silicon, and the Virtual Silicon
// logo are registered trademarks of Virtual Silicon Technology Inc.  All
// other trademarks are the property of their respective owner.
// 
// Virtual Silicon Technology Inc.
// 1322 Orleans Drive
// Sunnyvale, CA 94089-1116
// Phone : (408) 548-2700
// Fax   : (408) 548-2750
// Web Site : http://www.virtual-silicon.com
// 
// File Name:       HDANT2.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:26:40
// ------------------------------------------------------------------------
//  
// $RCSfile: HDANT2.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDANT2.v,v $ 
// $Date: 2003/04/23 22:38:01 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
// verigen patch-level 1.132-m 02/27/2000 13:49:14
// Clock Load Cell
// DUMMY = A
`celldefine
`suppress_faults
`enable_portfaults
module HDANT2(A);
input A;
`ifdef no_tchk
`else
buf _i0(DUMMY, A);
`endif
endmodule
`disable_portfaults
`nosuppress_faults
`endcelldefine
