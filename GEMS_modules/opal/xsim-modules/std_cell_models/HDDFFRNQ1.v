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
// File Name:       HDDFFRNQ1.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:27:49
// ------------------------------------------------------------------------
//  
// $RCSfile: HDDFFRNQ1.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDDFFRNQ1.v,v $ 
// $Date: 2003/04/23 22:38:55 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version 5.1.4.3 production                                    */
/*  Created:  Thu Feb 20 16:25:08 2003 by sasana                             */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDDFFRNQ1(D, RB, CKB, Q);
input D;
input RB;
input CKB;
output Q;
reg notifier;
wire SMC_CK_IN;
not SMC_I0(SMC_CK_IN, CKB);
dff_p0 SMC_I1(.q(SMC_IQ), .d(D), .clk(SMC_CK_IN), .clear(RB), .preset(`VCC), .notifier(notifier));
//  output pins
buf SMC_I2(Q, SMC_IQ);
`ifdef no_tchk
`else
buf SMC_I3(shcheckCKBDhl, RB);
`endif
specify
// arc RB --> Q
  ( RB => Q ) = (1,1);
// arc CKB --> Q
  ( CKB => Q ) = (1,1);
`ifdef no_tchk
`else
  $setup( negedge D,  negedge CKB &&& (shcheckCKBDhl === 1'b1), 1, notifier);
  $setup( posedge D,  negedge CKB &&& (shcheckCKBDhl === 1'b1), 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBDhl === 1'b1),  posedge D, 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBDhl === 1'b1),  negedge D, 1, notifier);
  $recovery( posedge RB,  negedge CKB, 1, notifier);
  $hold( negedge CKB,  posedge RB, 1, notifier);
  $width( negedge CKB, 1, 0, notifier);
  $width( posedge CKB, 1, 0, notifier);
  $width( negedge RB, 1, 0, notifier);
  $period( posedge CKB, 0, notifier);
  $period( negedge CKB, 0, notifier);
`endif
endspecify
endmodule // HDDFFRNQ1 //
`disable_portfaults
`nosuppress_faults
`endcelldefine
