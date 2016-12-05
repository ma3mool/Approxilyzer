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
// File Name:       HDADHALFD1.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:26:38
// ------------------------------------------------------------------------
//  
// $RCSfile: HDADHALFD1.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDADHALFD1.v,v $ 
// $Date: 2003/04/23 22:37:56 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version 5.1.4.3 production                                    */
/*  Created:  Thu Feb 20 16:10:16 2003 by sasana                             */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDADHALFD1(A, B, CO, S);
input A;
input B;
output CO;
output S;
and SMC_I0(CO, A, B);
xor SMC_I1(S, A, B);
specify
// arc A --> CO
  ( A => CO ) = (1,1);
// arc B --> CO
  ( B => CO ) = (1,1);
// arc A --> S
  if (B === 1'b0) ( A => S ) = (1,1);
  if (B === 1'b1) ( A => S ) = (1,1);
  ifnone ( A => S ) = (1,1);
// arc B --> S
  if (A === 1'b0) ( B => S ) = (1,1);
  if (A === 1'b1) ( B => S ) = (1,1);
  ifnone ( B => S ) = (1,1);
endspecify
endmodule // HDADHALFD1 //
`disable_portfaults
`nosuppress_faults
`endcelldefine
