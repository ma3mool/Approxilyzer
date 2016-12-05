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
// File Name:       HDDFERSNBL.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:27:43
// ------------------------------------------------------------------------
//  
// $RCSfile: HDDFERSNBL.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDDFERSNBL.v,v $ 
// $Date: 2003/04/23 22:38:47 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version 5.1.4.3 production                                    */
/*  Created:  Thu Feb 20 16:55:51 2003 by sasana                             */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDDFERSNBL(CEB, D, RB, SB, CKB, Q, QB);
input CEB;
input D;
input RB;
input SB;
input CKB;
output Q;
output QB;
reg notifier;
wire SMC_CK_IN;
wire SMC_NS_IN;
not SMC_I0(SMC_CK_IN, CKB);
inv_clr0 SMC_I1(.qn(SMC_IQN), .clr(RB), .pre(SB), .inp(SMC_IQ));
mux21 SMC_I2(SMC_NS_IN, SMC_IQ, D, CEB);
dff_p1 SMC_I3(.q(SMC_IQ), .d(SMC_NS_IN), .clk(SMC_CK_IN), .clear(RB), .preset(SB), .notifier(notifier));
//  output pins
buf SMC_I4(Q, SMC_IQ);
buf SMC_I5(QB, SMC_IQN);
`ifdef no_tchk
`else
and SMC_I6(shcheckCKBCEBhl, RB, SB);
not SMC_I7(CEB_bar, CEB);
and SMC_I8(shcheckCKBDhl, CEB_bar, RB, SB);
buf SMC_I9(shcheckCKBRBhl, SB);
buf SMC_I10(shcheckCKBSBhl, `VCC);
`endif
specify
// arc RB --> Q
  ( RB => Q ) = (1,1);
// arc RB --> QB
  ( RB => QB ) = (1,1);
// arc SB --> Q
  ( SB => Q ) = (1,1);
// arc SB --> QB
  ( SB => QB ) = (1,1);
// arc CKB --> Q
  ( CKB => Q ) = (1,1);
// arc CKB --> QB
  ( CKB => QB ) = (1,1);
`ifdef no_tchk
`else
  $setup( negedge CEB,  negedge CKB &&& (shcheckCKBCEBhl === 1'b1), 1, notifier);
  $setup( posedge CEB,  negedge CKB &&& (shcheckCKBCEBhl === 1'b1), 1, notifier);
  $setup( negedge D,  negedge CKB &&& (shcheckCKBDhl === 1'b1), 1, notifier);
  $setup( posedge D,  negedge CKB &&& (shcheckCKBDhl === 1'b1), 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBCEBhl === 1'b1),  posedge CEB, 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBDhl === 1'b1),  posedge D, 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBDhl === 1'b1),  negedge D, 1, notifier);
  $recovery( posedge RB,  negedge CKB &&& (shcheckCKBRBhl === 1'b1), 1, notifier);
  $recovery( posedge SB,  negedge CKB &&& (shcheckCKBSBhl === 1'b1), 1, notifier);
  $recovery( posedge SB,  posedge RB, 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBRBhl === 1'b1),  posedge RB, 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBSBhl === 1'b1),  posedge SB, 1, notifier);
  $hold( posedge RB,  posedge SB, 1, notifier);
  $width( negedge CKB, 1, 0, notifier);
  $width( posedge CKB, 1, 0, notifier);
  $width( negedge RB, 1, 0, notifier);
  $width( negedge SB, 1, 0, notifier);
  $period( posedge CKB, 0, notifier);
  $period( negedge CKB, 0, notifier);
`endif
endspecify
endmodule // HDDFERSNBL //
`disable_portfaults
`nosuppress_faults
`endcelldefine
