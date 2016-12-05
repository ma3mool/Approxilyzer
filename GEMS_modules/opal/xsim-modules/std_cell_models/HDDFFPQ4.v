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
// File Name:       HDDFFPQ4.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:27:50
// ------------------------------------------------------------------------
//  
// $RCSfile: HDDFFPQ4.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDDFFPQ4.v,v $ 
// $Date: 2003/04/23 22:38:54 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version 5.1.4.3 production                                    */
/*  Created:  Thu Feb 20 16:38:12 2003 by sasana                             */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDDFFPQ4(D, CK, Q);
input D;
input CK;
output Q;
reg notifier;
dff_p0 SMC_I0(.q(SMC_IQ), .d(D), .clk(CK), .clear(`VCC), .preset(`VCC), .notifier(notifier));
//  output pins
buf SMC_I1(Q, SMC_IQ);
specify
// arc CK --> Q
  ( CK => Q ) = (1,1);
`ifdef no_tchk
`else
  $setup( negedge D,  posedge CK, 1, notifier);
  $setup( posedge D,  posedge CK, 1, notifier);
  $hold( posedge CK,  posedge D, 1, notifier);
  $hold( posedge CK,  negedge D, 1, notifier);
  $width( negedge CK, 1, 0, notifier);
  $width( posedge CK, 1, 0, notifier);
  $period( posedge CK, 0, notifier);
  $period( negedge CK, 0, notifier);
`endif
endspecify
endmodule // HDDFFPQ4 //
`disable_portfaults
`nosuppress_faults
`endcelldefine
