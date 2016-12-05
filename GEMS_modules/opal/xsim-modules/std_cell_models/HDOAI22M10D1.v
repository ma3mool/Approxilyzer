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
// File Name:       HDOAI22M10D1.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:29:37
// ------------------------------------------------------------------------
//  
// $RCSfile: HDOAI22M10D1.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDOAI22M10D1.v,v $ 
// $Date: 2003/04/23 22:40:16 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version 5.1.4.3 production                                    */
/*  Created:  Thu Feb 20 16:13:58 2003 by sasana                             */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDOAI22M10D1(A1, A2, B1, B2, Z);
input A1;
input A2;
input B1;
input B2;
output Z;
not SMC_I0(A2_bar, A2);
and SMC_I1(OUT0, A1, A2_bar);
not SMC_I2(B1_bar, B1);
not SMC_I3(B2_bar, B2);
and SMC_I4(OUT1, B1_bar, B2_bar);
or SMC_I5(Z, OUT0, OUT1);
specify
// arc A1 --> Z
  ( A1 => Z ) = (1,1);
// arc A2 --> Z
  ( A2 => Z ) = (1,1);
// arc B1 --> Z
  ( B1 => Z ) = (1,1);
// arc B2 --> Z
  ( B2 => Z ) = (1,1);
endspecify
endmodule // HDOAI22M10D1 //
`disable_portfaults
`nosuppress_faults
`endcelldefine
