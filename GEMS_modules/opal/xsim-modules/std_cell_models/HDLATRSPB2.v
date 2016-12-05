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
// File Name:       HDLATRSPB2.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:28:32
// ------------------------------------------------------------------------
//  
// $RCSfile: HDLATRSPB2.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDLATRSPB2.v,v $ 
// $Date: 2003/04/23 22:39:26 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version 5.1.4.3 production                                    */
/*  Created:  Thu Feb 20 16:12:03 2003 by sasana                             */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDLATRSPB2(D, RB, SB, GB, Q, QB);
input D;
input RB;
input SB;
input GB;
output Q;
output QB;
reg notifier;
wire SMC_EN_IN;
not SMC_I0(SMC_EN_IN, GB);
inv_clr0 SMC_I1(.qn(SMC_IQN), .clr(RB), .pre(SB), .inp(SMC_IQ));
ldlatch_p0 SMC_I2(.q(SMC_IQ), .d(D), .en(SMC_EN_IN), .clear(RB), .preset(SB), .notifier(notifier));
//  output pins
buf SMC_I3(Q, SMC_IQ);
buf SMC_I4(QB, SMC_IQN);
`ifdef no_tchk
`else
and SMC_I5(shcheckGBDlh, RB, SB);
buf SMC_I6(shcheckGBRBlh, `VCC);
buf SMC_I7(shcheckGBSBlh, `VCC);
`endif
specify
// arc D --> Q
  ( D => Q ) = (1,1);
// arc D --> QB
  ( D => QB ) = (1,1);
// arc RB --> Q
  ( RB => Q ) = (1,1);
// arc RB --> QB
  ( RB => QB ) = (1,1);
// arc SB --> Q
  ( SB => Q ) = (1,1);
// arc SB --> QB
  ( SB => QB ) = (1,1);
// arc GB --> Q
  ( GB => Q ) = (1,1);
// arc GB --> QB
  ( GB => QB ) = (1,1);
`ifdef no_tchk
`else
  $setup( negedge D,  posedge GB &&& (shcheckGBDlh === 1'b1), 1, notifier);
  $setup( posedge D,  posedge GB &&& (shcheckGBDlh === 1'b1), 1, notifier);
  $hold( posedge GB &&& (shcheckGBDlh === 1'b1),  posedge D, 1, notifier);
  $hold( posedge GB &&& (shcheckGBDlh === 1'b1),  negedge D, 1, notifier);
  $recovery( posedge RB,  posedge GB &&& (shcheckGBRBlh === 1'b1), 1, notifier);
  $recovery( posedge RB,  posedge SB, 1, notifier);
  $recovery( posedge SB,  posedge GB &&& (shcheckGBSBlh === 1'b1), 1, notifier);
  $hold( posedge GB &&& (shcheckGBRBlh === 1'b1),  posedge RB, 1, notifier);
  $hold( posedge GB &&& (shcheckGBSBlh === 1'b1),  posedge SB, 1, notifier);
  $hold( posedge SB,  posedge RB, 1, notifier);
  $width( negedge GB, 1, 0, notifier);
  $width( negedge RB, 1, 0, notifier);
  $width( negedge SB, 1, 0, notifier);
  $width( posedge GB, 1, 0, notifier);
  $period( posedge GB, 0, notifier);
  $period( negedge GB, 0, notifier);
`endif
endspecify
endmodule // HDLATRSPB2 //
`disable_portfaults
`nosuppress_faults
`endcelldefine
