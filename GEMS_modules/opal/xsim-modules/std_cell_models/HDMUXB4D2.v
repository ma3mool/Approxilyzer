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
// File Name:       HDMUXB4D2.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/16/2003 15:28:40
// ------------------------------------------------------------------------
/*****************************************************************************/
/*                                                                           */
/*  CellRater, version CR-4.9.2.1 production                                 */
/*  Created:  Wed Dec 04 13:45:14 2002 by dharman                            */
/*    for Verilog Simulator:  verilog-xl                                     */
/*                                                                           */
/*****************************************************************************/
`timescale 1 ns / 1 ps
`define VCC 1'b1
`define VSS 0
`celldefine
`suppress_faults
`enable_portfaults
module HDMUXB4D2(Z, A0, A1, A2, A3, SL0, SL1);
input A0;
input A1;
input A2;
input A3;
input SL0;
input SL1;
output Z;
 HDMUXB4D2_UDPZ(Z, A0, A1, A2, A3, SL0, SL1);
specify
// arc A0 --> Z
  if (A1 === 1'b1 && A2 === 1'b1 && A3 === 1'b1 && SL0 === 1'b0 && SL1 === 1'b0) ( A0 => Z ) = (1,1);
  if (A1 === 1'b1 && A2 === 1'b1 && A3 === 1'b0 && SL0 === 1'b0 && SL1 === 1'b0) ( A0 => Z ) = (1,1);
  if (A1 === 1'b1 && A2 === 1'b0 && A3 === 1'b1 && SL0 === 1'b0 && SL1 === 1'b0) ( A0 => Z ) = (1,1);
  if (A1 === 1'b1 && A2 === 1'b0 && A3 === 1'b0 && SL0 === 1'b0 && SL1 === 1'b0) ( A0 => Z ) = (1,1);
  if (A1 === 1'b0 && A2 === 1'b1 && A3 === 1'b1 && SL0 === 1'b0 && SL1 === 1'b0) ( A0 => Z ) = (1,1);
  if (A1 === 1'b0 && A2 === 1'b1 && A3 === 1'b0 && SL0 === 1'b0 && SL1 === 1'b0) ( A0 => Z ) = (1,1);
  if (A1 === 1'b0 && A2 === 1'b0 && A3 === 1'b1 && SL0 === 1'b0 && SL1 === 1'b0) ( A0 => Z ) = (1,1);
  if (A1 === 1'b0 && A2 === 1'b0 && A3 === 1'b0 && SL0 === 1'b0 && SL1 === 1'b0) ( A0 => Z ) = (1,1);
  ifnone ( A0 => Z ) = (1,1);
// arc A1 --> Z
  if (A0 === 1'b1 && A2 === 1'b1 && A3 === 1'b1 && SL0 === 1'b1 && SL1 === 1'b0) ( A1 => Z ) = (1,1);
  if (A0 === 1'b1 && A2 === 1'b1 && A3 === 1'b0 && SL0 === 1'b1 && SL1 === 1'b0) ( A1 => Z ) = (1,1);
  if (A0 === 1'b1 && A2 === 1'b0 && A3 === 1'b1 && SL0 === 1'b1 && SL1 === 1'b0) ( A1 => Z ) = (1,1);
  if (A0 === 1'b1 && A2 === 1'b0 && A3 === 1'b0 && SL0 === 1'b1 && SL1 === 1'b0) ( A1 => Z ) = (1,1);
  if (A0 === 1'b0 && A2 === 1'b1 && A3 === 1'b1 && SL0 === 1'b1 && SL1 === 1'b0) ( A1 => Z ) = (1,1);
  if (A0 === 1'b0 && A2 === 1'b1 && A3 === 1'b0 && SL0 === 1'b1 && SL1 === 1'b0) ( A1 => Z ) = (1,1);
  if (A0 === 1'b0 && A2 === 1'b0 && A3 === 1'b1 && SL0 === 1'b1 && SL1 === 1'b0) ( A1 => Z ) = (1,1);
  if (A0 === 1'b0 && A2 === 1'b0 && A3 === 1'b0 && SL0 === 1'b1 && SL1 === 1'b0) ( A1 => Z ) = (1,1);
  ifnone ( A1 => Z ) = (1,1);
// arc A2 --> Z
  if (A0 === 1'b1 && A1 === 1'b1 && A3 === 1'b1 && SL0 === 1'b0 && SL1 === 1'b1) ( A2 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b1 && A3 === 1'b0 && SL0 === 1'b0 && SL1 === 1'b1) ( A2 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A3 === 1'b1 && SL0 === 1'b0 && SL1 === 1'b1) ( A2 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A3 === 1'b0 && SL0 === 1'b0 && SL1 === 1'b1) ( A2 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A3 === 1'b1 && SL0 === 1'b0 && SL1 === 1'b1) ( A2 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A3 === 1'b0 && SL0 === 1'b0 && SL1 === 1'b1) ( A2 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b0 && A3 === 1'b1 && SL0 === 1'b0 && SL1 === 1'b1) ( A2 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b0 && A3 === 1'b0 && SL0 === 1'b0 && SL1 === 1'b1) ( A2 => Z ) = (1,1);
  ifnone ( A2 => Z ) = (1,1);
// arc A3 --> Z
  if (A0 === 1'b1 && A1 === 1'b1 && A2 === 1'b1 && SL0 === 1'b1 && SL1 === 1'b1) ( A3 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b1 && A2 === 1'b0 && SL0 === 1'b1 && SL1 === 1'b1) ( A3 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b1 && SL0 === 1'b1 && SL1 === 1'b1) ( A3 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b0 && SL0 === 1'b1 && SL1 === 1'b1) ( A3 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b1 && SL0 === 1'b1 && SL1 === 1'b1) ( A3 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b0 && SL0 === 1'b1 && SL1 === 1'b1) ( A3 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b0 && A2 === 1'b1 && SL0 === 1'b1 && SL1 === 1'b1) ( A3 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b0 && A2 === 1'b0 && SL0 === 1'b1 && SL1 === 1'b1) ( A3 => Z ) = (1,1);
  ifnone ( A3 => Z ) = (1,1);
// arc SL0 --> Z
  if (A0 === 1'b1 && A1 === 1'b1 && A2 === 1'b1 && A3 === 1'b0 && SL1 === 1'b1) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b1 && A3 === 1'b1 && SL1 === 1'b0) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b1 && A3 === 1'b0 && SL1 === 1'b1) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b1 && A3 === 1'b0 && SL1 === 1'b0) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b0 && A3 === 1'b1 && SL1 === 1'b0) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b0 && A3 === 1'b0 && SL1 === 1'b0) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b1 && A3 === 1'b0 && SL1 === 1'b1) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b0 && A2 === 1'b1 && A3 === 1'b0 && SL1 === 1'b1) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b1 && A2 === 1'b0 && A3 === 1'b1 && SL1 === 1'b1) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b0 && A3 === 1'b1 && SL1 === 1'b1) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b1 && A3 === 1'b1 && SL1 === 1'b0) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b1 && A3 === 1'b0 && SL1 === 1'b0) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b0 && A3 === 1'b1 && SL1 === 1'b1) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b0 && A3 === 1'b1 && SL1 === 1'b0) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b0 && A3 === 1'b0 && SL1 === 1'b0) ( SL0 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b0 && A2 === 1'b0 && A3 === 1'b1 && SL1 === 1'b1) ( SL0 => Z ) = (1,1);
  ifnone ( SL0 => Z ) = (1,1);
// arc SL1 --> Z
  if (A0 === 1'b1 && A1 === 1'b1 && A2 === 1'b1 && A3 === 1'b0 && SL0 === 1'b1) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b1 && A2 === 1'b0 && A3 === 1'b1 && SL0 === 1'b0) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b1 && A2 === 1'b0 && A3 === 1'b0 && SL0 === 1'b1) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b1 && A2 === 1'b0 && A3 === 1'b0 && SL0 === 1'b0) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b0 && A3 === 1'b1 && SL0 === 1'b0) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b0 && A3 === 1'b0 && SL0 === 1'b0) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b1 && A3 === 1'b0 && SL0 === 1'b1) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b0 && A3 === 1'b0 && SL0 === 1'b1) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b1 && A3 === 1'b1 && SL0 === 1'b1) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b1 && A1 === 1'b0 && A2 === 1'b0 && A3 === 1'b1 && SL0 === 1'b1) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b1 && A3 === 1'b1 && SL0 === 1'b0) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b1 && A2 === 1'b1 && A3 === 1'b0 && SL0 === 1'b0) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b0 && A2 === 1'b1 && A3 === 1'b1 && SL0 === 1'b1) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b0 && A2 === 1'b1 && A3 === 1'b1 && SL0 === 1'b0) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b0 && A2 === 1'b1 && A3 === 1'b0 && SL0 === 1'b0) ( SL1 => Z ) = (1,1);
  if (A0 === 1'b0 && A1 === 1'b0 && A2 === 1'b0 && A3 === 1'b1 && SL0 === 1'b1) ( SL1 => Z ) = (1,1);
  ifnone ( SL1 => Z ) = (1,1);
endspecify
endmodule // HDMUXB4D2 //

primitive HDMUXB4D2_UDPZ(Z, A0, A1, A2, A3, SL0, SL1);
output Z;
input A0;
input A1;
input A2;
input A3;
input SL0;
input SL1;

table
// A0  A1  A2  A3  SL0  SL1 Z
   0    0    0    0    ?    ?   : 1   ; // reducing unknowns
   1    1    1    1    ?    ?   : 0   ; // reducing unknowns
   ?    0    ?    0    1    ?   : 1   ; // reducing unknowns
   ?    1    ?    1    1    ?   : 0   ; // reducing unknowns
   ?    ?    0    0    ?    1   : 1   ; // reducing unknowns
   ?    ?    1    1    ?    1   : 0   ; // reducing unknowns
   0    ?    0    ?    0    ?   : 1   ; // reducing unknowns
   1    ?    1    ?    0    ?   : 0   ; // reducing unknowns
   0    0    ?    ?    ?    0   : 1   ; // reducing unknowns
   1    1    ?    ?    ?    0   : 0   ; // reducing unknowns
   1    ?    ?    ?    0    0   : 0   ;
   ?    ?    1    ?    0    1   : 0   ;
   ?    1    ?    ?    1    0   : 0   ;
   ?    ?    ?    1    1    1   : 0   ;
   ?    ?    0    ?    0    1   : 1   ;
   ?    ?    ?    0    1    1   : 1   ;
   0    ?    ?    ?    0    0   : 1   ;
   ?    0    ?    ?    1    0   : 1   ;
endtable
endprimitive
`disable_portfaults
`nosuppress_faults
`endcelldefine
