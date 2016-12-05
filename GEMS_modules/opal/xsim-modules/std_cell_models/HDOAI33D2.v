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
// File Name:       HDOAI33D2.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:29:47
// ------------------------------------------------------------------------
//  
// $RCSfile: HDOAI33D2.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDOAI33D2.v,v $ 
// $Date: 2003/04/23 22:40:22 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version CR-4.9.2.1 production                                 */
/*  Created:  Wed Dec 04 13:51:22 2002 by dharman                            */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDOAI33D2(A1, A2, A3, B1, B2, B3, Z);
input A1;
input A2;
input A3;
input B1;
input B2;
input B3;
output Z;
not SMC_I0(A1_bar, A1);
not SMC_I1(A2_bar, A2);
not SMC_I2(A3_bar, A3);
and SMC_I3(OUT0, A1_bar, A2_bar, A3_bar);
not SMC_I4(B1_bar, B1);
not SMC_I5(B2_bar, B2);
not SMC_I6(B3_bar, B3);
and SMC_I7(OUT1, B1_bar, B2_bar, B3_bar);
or SMC_I8(Z, OUT0, OUT1);
specify
// arc A1 --> Z
  ( A1 => Z ) = (1,1);
// arc A2 --> Z
  ( A2 => Z ) = (1,1);
// arc A3 --> Z
  ( A3 => Z ) = (1,1);
// arc B1 --> Z
  ( B1 => Z ) = (1,1);
// arc B2 --> Z
  ( B2 => Z ) = (1,1);
// arc B3 --> Z
  ( B3 => Z ) = (1,1);
endspecify
endmodule // HDOAI33D2 //
`disable_portfaults
`nosuppress_faults
`endcelldefine
