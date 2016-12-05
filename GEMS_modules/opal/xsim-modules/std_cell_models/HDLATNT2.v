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
// File Name:       HDLATNT2.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:28:24
// ------------------------------------------------------------------------
//  
// $RCSfile: HDLATNT2.v,v $ 
// $Source: /syncdisk/rcs/common/verilog/5.1.4.3/sc/RCS/HDLATNT2.v,v $ 
// $Date: 2003/04/23 22:39:20 $ 
// $Revision: 1.2 $ 
//  
// ---------------------- 
// Verilog dump Timing Insertion Version 1.5

// Verilog dump veralc Version 1.9
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version 5.1.4.3 production                                    */
/*  Created:  Thu Feb 20 15:58:35 2003 by sasana                             */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDLATNT2(D, EN, G, Z);
input D;
input EN;
input G;
output Z;
reg notifier;
`ifdef no_tchk
`else
not SMC_I0(SMC_IQN, SMC_IQ);
`endif
ldlatch_p0 SMC_I1(.q(SMC_IQ), .d(D), .en(G), .clear(`VCC), .preset(`VCC), .notifier(notifier));
//  output pins
buf SMC_I2(SMC_DINZ, SMC_IQ);
not SMC_I3(SMC_ZENZ, EN);
bufif0 SMC_I4(Z, SMC_DINZ, SMC_ZENZ);
`ifdef no_tchk
`else
buf SMC_I5(shcheckGDhl, EN);
buf SMC_I6(shcheckGENhl, EN);
buf SMC_I7(shcheckG, EN);
`endif
specify
// arc D --> Z
  ( D => Z ) = (1,1);
// arc EN --> Z
  ( EN => Z ) = (1,1,1,1,1,1);
// arc G --> Z
  ( G => Z ) = (1,1);
`ifdef no_tchk
`else
  $setup( negedge D,  negedge G &&& (shcheckGDhl === 1'b1), 1, notifier);
  $setup( posedge D,  negedge G &&& (shcheckGDhl === 1'b1), 1, notifier);
  $hold( negedge G &&& (shcheckGDhl === 1'b1),  posedge D, 1, notifier);
  $hold( negedge G &&& (shcheckGDhl === 1'b1),  negedge D, 1, notifier);
  $width( posedge G, 1, 0, notifier);
  $width( negedge G, 1, 0, notifier);
  $period( posedge G, 0, notifier);
  $period( negedge G, 0, notifier);
`endif
endspecify
endmodule // HDLATNT2 //
`disable_portfaults
`nosuppress_faults
`endcelldefine
