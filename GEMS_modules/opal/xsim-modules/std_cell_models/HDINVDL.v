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
// File Name:       HDINVDL.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:28:15
// ------------------------------------------------------------------------
//  
// $RCSfile: HDINVDL.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDINVDL.v,v $ 
// $Date: 2003/04/23 22:39:16 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version 5.1.4.3 production                                    */
/*  Created:  Thu Feb 20 17:05:48 2003 by sasana                             */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDINVDL(A, Z);
input A;
output Z;
not SMC_I0(Z, A);
specify
// arc A --> Z
  ( A => Z ) = (1,1);
endspecify
endmodule // HDINVDL //
`disable_portfaults
`nosuppress_faults
`endcelldefine
