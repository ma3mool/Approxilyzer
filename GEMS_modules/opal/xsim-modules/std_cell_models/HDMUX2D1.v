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
// File Name:       HDMUX2D1.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:28:33
// ------------------------------------------------------------------------
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version 5.1.4.3 production                                    */
/*  Created:  Wed Feb 26 16:09:51 2003 by fredn                              */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDMUX2D1(Z, A0, A1, SL);
input A0;
input A1;
input SL;
output Z;
 HDMUX2D1_UDPZ(Z, A0, A1, SL);
specify
// arc A0 --> Z
  if (A1 === 1'b1 && SL === 1'b0) ( A0 => Z ) = (1,1);
  if (A1 === 1'b0 && SL === 1'b0) ( A0 => Z ) = (1,1);
  ifnone ( A0 => Z ) = (1,1);
// arc A1 --> Z
  if (A0 === 1'b1 && SL === 1'b1) ( A1 => Z ) = (1,1);
  if (A0 === 1'b0 && SL === 1'b1) ( A1 => Z ) = (1,1);
  ifnone ( A1 => Z ) = (1,1);
// arc SL --> Z
  if (A0 === 1'b0 && A1 === 1'b1) ( SL => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0) ( SL => Z ) = (1,1);
  ifnone ( SL => Z ) = (1,1);
endspecify
endmodule // HDMUX2D1 //

primitive HDMUX2D1_UDPZ(Z, A0, A1, SL);
output Z;
input A0;
input A1;
input SL;

table
// A0  A1  SL Z
   0    0    ?   : 0   ; // reducing unknowns
   1    1    ?   : 1   ; // reducing unknowns
   ?    1    1   : 1   ;
   1    ?    0   : 1   ;
   0    ?    0   : 0   ;
   ?    0    1   : 0   ;
endtable
endprimitive
`disable_portfaults
`nosuppress_faults
`endcelldefine
