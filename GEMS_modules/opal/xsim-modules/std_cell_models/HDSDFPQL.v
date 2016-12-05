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
// File Name:       HDSDFPQL.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:30:07
// ------------------------------------------------------------------------
//  
// $RCSfile: HDSDFPQL.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDSDFPQL.v,v $ 
// $Date: 2003/04/23 22:40:37 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version 5.1.4.3 production                                    */
/*  Created:  Thu Feb 20 16:15:10 2003 by sasana                             */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDSDFPQL(D, CK, SD, SE, Q);
input D;
input CK;
input SD;
input SE;
output Q;
reg notifier;
wire SMC_NS_IN;
not SMC_I0(SMC_DIS_SCAN, SE);
not SMC_I1(SMC_EN_SCAN, SMC_DIS_SCAN);
and SMC_I2(SMC_D_DIS, SMC_DIS_SCAN, D);
and SMC_I3(SMC_D_SCAN, SMC_EN_SCAN, SD);
or SMC_I4(SMC_NS_IN, SMC_D_DIS, SMC_D_SCAN);
dff_p0 SMC_I5(.q(SMC_IQ), .d(SMC_NS_IN), .clk(CK), .clear(`VCC), .preset(`VCC), .notifier(notifier));
//  output pins
buf SMC_I6(Q, SMC_IQ);
`ifdef no_tchk
`else
not SMC_I7(shcheckCKDlh, SE);
buf SMC_I8(shcheckCKSDlh, SE);
`endif
specify
// arc CK --> Q
  ( CK => Q ) = (1,1);
`ifdef no_tchk
`else
  $setup( negedge D,  posedge CK &&& (shcheckCKDlh === 1'b1), 1, notifier);
  $setup( posedge D,  posedge CK &&& (shcheckCKDlh === 1'b1), 1, notifier);
  $setup( negedge SD,  posedge CK &&& (shcheckCKSDlh === 1'b1), 1, notifier);
  $setup( posedge SD,  posedge CK &&& (shcheckCKSDlh === 1'b1), 1, notifier);
  $setup( negedge SE,  posedge CK, 1, notifier);
  $setup( posedge SE,  posedge CK, 1, notifier);
  $hold( posedge CK &&& (shcheckCKDlh === 1'b1),  posedge D, 1, notifier);
  $hold( posedge CK &&& (shcheckCKDlh === 1'b1),  negedge D, 1, notifier);
  $hold( posedge CK &&& (shcheckCKSDlh === 1'b1),  posedge SD, 1, notifier);
  $hold( posedge CK &&& (shcheckCKSDlh === 1'b1),  negedge SD, 1, notifier);
  $hold( posedge CK,  posedge SE, 1, notifier);
  $hold( posedge CK,  negedge SE, 1, notifier);
  $width( negedge CK, 1, 0, notifier);
  $width( posedge CK, 1, 0, notifier);
  $period( posedge CK, 0, notifier);
  $period( negedge CK, 0, notifier);
`endif
endspecify
endmodule // HDSDFPQL //
`disable_portfaults
`nosuppress_faults
`endcelldefine
