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
// File Name:       HDSDFRSNB2.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:30:13
// ------------------------------------------------------------------------
//  
// $RCSfile: HDSDFRSNB2.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDSDFRSNB2.v,v $ 
// $Date: 2003/04/23 22:40:42 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version 5.1.4.3 production                                    */
/*  Created:  Thu Feb 20 16:32:45 2003 by sasana                             */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDSDFRSNB2(D, RB, SB, CKB, SD, SE, Q, QB);
input D;
input RB;
input SB;
input CKB;
input SD;
input SE;
output Q;
output QB;
reg notifier;
wire SMC_CK_IN;
wire SMC_NS_IN;
not SMC_I0(SMC_CK_IN, CKB);
inv_clr0 SMC_I1(.qn(SMC_IQN), .clr(RB), .pre(SB), .inp(SMC_IQ));
not SMC_I2(SMC_DIS_SCAN, SE);
not SMC_I3(SMC_EN_SCAN, SMC_DIS_SCAN);
and SMC_I4(SMC_D_DIS, SMC_DIS_SCAN, D);
and SMC_I5(SMC_D_SCAN, SMC_EN_SCAN, SD);
or SMC_I6(SMC_NS_IN, SMC_D_DIS, SMC_D_SCAN);
dff_p1 SMC_I7(.q(SMC_IQ), .d(SMC_NS_IN), .clk(SMC_CK_IN), .clear(RB), .preset(SB), .notifier(notifier));
//  output pins
buf SMC_I8(Q, SMC_IQ);
buf SMC_I9(QB, SMC_IQN);
`ifdef no_tchk
`else
not SMC_I10(SE_bar, SE);
and SMC_I11(shcheckCKBDhl, RB, SB, SE_bar);
buf SMC_I12(shcheckCKBRBhl, SB);
buf SMC_I13(shcheckCKBSBhl, `VCC);
and SMC_I14(shcheckCKBSDhl, RB, SB, SE);
and SMC_I15(shcheckCKBSEhl, RB, SB);
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
  $setup( negedge D,  negedge CKB &&& (shcheckCKBDhl === 1'b1), 1, notifier);
  $setup( posedge D,  negedge CKB &&& (shcheckCKBDhl === 1'b1), 1, notifier);
  $setup( negedge SD,  negedge CKB &&& (shcheckCKBSDhl === 1'b1), 1, notifier);
  $setup( posedge SD,  negedge CKB &&& (shcheckCKBSDhl === 1'b1), 1, notifier);
  $setup( negedge SE,  negedge CKB &&& (shcheckCKBSEhl === 1'b1), 1, notifier);
  $setup( posedge SE,  negedge CKB &&& (shcheckCKBSEhl === 1'b1), 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBDhl === 1'b1),  posedge D, 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBDhl === 1'b1),  negedge D, 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBSDhl === 1'b1),  posedge SD, 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBSDhl === 1'b1),  negedge SD, 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBSEhl === 1'b1),  posedge SE, 1, notifier);
  $hold( negedge CKB &&& (shcheckCKBSEhl === 1'b1),  negedge SE, 1, notifier);
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
endmodule // HDSDFRSNB2 //
`disable_portfaults
`nosuppress_faults
`endcelldefine
