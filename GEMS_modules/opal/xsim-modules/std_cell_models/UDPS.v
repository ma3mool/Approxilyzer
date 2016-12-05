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
// File Name:       UDPS.v
// Library Name:    umce13h210t3
// Library Release: 1.2
// Product:         High Density Standard Cells
// Process:         UMC L130E-HS-FSG
// Generated:       07/10/2003 14:49:18
// ------------------------------------------------------------------------
// All cells assume preset and clear are active low


///////////////////////////////////////////////////////////////////////////
// This dff cell has a reset priority over preset

`ifdef TETRAMAX
primitive udp_dff_p0(q, d, clk, clear, preset, notifier);
        output q;
        input  clk, clear, d, preset, notifier;
        reg q; /* declaring output as reg*/

        table
        // d  clk  clear  preset  notifier : state : q

           ?   ?     0      ?        ?     :   ?   : 0 ;  // if clear, set out to 0
           ?   ?     1      0        ?     :   ?   : 1 ;  // if not clear, preset to 1
           0   r     1      1        ?     :   ?   : 0 ;  // on rising edge transmit data
           1   r     1      1        ?     :   ?   : 1 ;  //  same as above

           1   r     1      ?        ?     :   ?   : 1 ;  //  reduce pessimism
           0   r     ?      1        ?     :   ?   : 0 ;  //

           1   *     1      ?        ?     :   1   : 1 ;  //  reduce pessimism
           0   *     ?      1        ?     :   0   : 0 ;  //

           ?   n     1      1        ?     :   ?   : - ;  // on negedge, do nothing
           *   ?     1      1        ?     :   ?   : - ;  // on steady clock, ignore data trans.

           *   b     ?      1        ?     :   0   : 0 ;  //  reduce pessimism
           *   b     1      ?        ?     :   1   : 1 ;  //  reduce pessimism

           ?   ?     1     (01)      ?     :   ?   : - ;  // ignore positive edge of preset
           ?   ?     1     (X1)      ?     :   ?   : - ;  //

           ?   b     *      1        ?     :   0   : 0 ;  // cover all transitions of clear
           0   x     *      1        ?     :   0   : 0 ;  // cover all transitions of clear
           ?   b     1      *        ?     :   1   : 1 ;  // cover all transitions of preset
           0   x     1      *        ?     :   1   : 1 ;  // cover all transitions of preset
        endtable
endprimitive

module dff_p0(q, d, clk, clear, preset, notifier);
   output q;
   input  clk, clear, d, preset, notifier;

   udp_dff_p0 D1(q, d, clk, clear, preset, notifier);
endmodule // dff_p0

`else
primitive udp_dff_p0(q, d, clk, clear, preset, notifier);
	output q;
	input  clk, clear, d, preset, notifier;
	reg q; /* declaring output as reg*/
	
	table
	// d  clk  clear  preset  notifier : state : q

	   ?   ?     0      ?        ?     :   ?   : 0 ;  // if clear, set out to 0
	   ?   ?     1      0        ?     :   ?   : 1 ;  // if not clear, preset to 1  
	   0   r     1      1        ?     :   ?   : 0 ;  // on rising edge transmit data
	   1   r     1      1        ?     :   ?   : 1 ;  //  same as above
	   1   *     1      ?        ?     :   1   : 1 ;  //  reduce pessimism
	   0   *     ?      1        ?     :   0   : 0 ;  //  reduce pessimism
	   ?   n     1      1        ?     :   ?   : - ;  // on negedge, do nothing
	   *   ?     1      1        ?     :   ?   : - ;  // on steady clock, ignore data trans.
           *   b     ?      1        ?     :   0   : 0 ;  //  reduce pessimism
           *   b     1      ?        ?     :   1   : 1 ;  //  reduce pessimism
	   ?   ?     p      1        ?     :   ?   : - ;  // ignore positive edge of clear 
	   ?   b     *      1        ?     :   0   : 0 ;  // cover all transitions of clear
	   0   x     *      1        ?     :   0   : 0 ;  // cover all transitions of clear
	   ?   ?     1      p        ?     :   ?   : - ;  // ignore positive edge of preset
	   ?   b     1      *        ?     :   1   : 1 ;  // cover all transitions of preset
	   0   x     1      *        ?     :   1   : 1 ;  // cover all transitions of preset
           ?   ?     ?      ?        *     :   ?   : x ;  // on any notifier event output x
	endtable
endprimitive

module dff_p0(q, d, clk, clear, preset, notifier);
   output q;
   input  clk, clear, d, preset, notifier;

   udp_dff_p0 D1(q, d, clk, clear, preset, notifier);
endmodule // dff_p0

`endif

///////////////////////////////////////////////////////////////////////////
// This cell has a preset priority over reset

`ifdef TETRAMAX
primitive udp_dff_p1(q, d, clk, clear, preset, notifier);
        output q;
        input  clk, clear, d, preset, notifier;
        reg q; /* declaring output as reg*/

        table
        // d  clk  clear  preset  notifier : state : q

           ?   ?     ?      0        ?     :   ?   : 1 ;  // if preset, set out to 1
           ?   ?     0      1        ?     :   ?   : 0 ;  // if not preset, clear to 0

           0   r     1      1        ?     :   ?   : 0 ;  // on rising edge transmit data
           1   r     1      1        ?     :   ?   : 1 ;  //  same as above

           1   r     1      ?        ?     :   ?   : 1 ;  //  reduce pessimism
           0   r     ?      1        ?     :   ?   : 0 ;  //  reduce pessimism

           1   *     1      ?        ?     :   1   : 1 ;  //  reduce pessimism
           0   *     ?      1        ?     :   0   : 0 ;  //  reduce pessimism

           ?   n     1      1        ?     :   ?   : - ;  // on negedge, do nothing
           *   ?     1      1        ?     :   ?   : - ;  // on steady clock, ignore data trans.

           *   b     ?      1        ?     :   0   : 0 ;  //  reduce pessimism
           *   b     1      ?        ?     :   1   : 1 ;  //  reduce pessimism

           ?   ?    (01)    1        ?     :   ?   : - ;  // ignore positive edge of clear
           ?   ?    (X1)    1        ?     :   ?   : - ;  // ignore positive edge of clear

           ?   b     *      1        ?     :   0   : 0 ;  // cover all transitions of clear
           0   x     *      1        ?     :   0   : 0 ;  // cover all transitions of clear

           ?   b     1      *        ?     :   1   : 1 ;  // cover all transitions of preset

           0   x     1      *        ?     :   1   : 1 ;  // cover all transitions of preset
        endtable
endprimitive

module dff_p1(q, d, clk, clear, preset, notifier);
   output q;
   input  clk, clear, d, preset, notifier;

   udp_dff_p1 D1(q, d, clk, clear, preset, notifier);
endmodule // dff_p1

`else
primitive udp_dff_p1(q, d, clk, clear, preset, notifier);
	output q;
	input  clk, clear, d, preset, notifier;
	reg q; /* declaring output as reg*/
	
	table
	// d  clk  clear  preset  notifier : state : q

	   ?   ?     ?      0        ?     :   ?   : 1 ;  // if preset, set out to 1
	   ?   ?     0      1        ?     :   ?   : 0 ;  // if not preset, clear to 0  
	   0   r     1      1        ?     :   ?   : 0 ;  // on rising edge transmit data
	   1   r     1      1        ?     :   ?   : 1 ;  //  same as above
	   1   *     1      ?        ?     :   1   : 1 ;  //  reduce pessimism
	   0   *     ?      1        ?     :   0   : 0 ;  //  reduce pessimism
	   ?   n     1      1        ?     :   ?   : - ;  // on negedge, do nothing
	   *   ?     1      1        ?     :   ?   : - ;  // on steady clock, ignore data trans.
	   ?   ?     p      1        ?     :   ?   : - ;  // ignore positive edge of clear 
           *   b     ?      1        ?     :   0   : 0 ;  //  reduce pessimism
           *   b     1      ?        ?     :   1   : 1 ;  //  reduce pessimism
	   ?   b     *      1        ?     :   0   : 0 ;  // cover all transitions of clear
	   0   x     *      1        ?     :   0   : 0 ;  // cover all transitions of clear
	   ?   ?     1      p        ?     :   ?   : - ;  // ignore positive edge of preset
	   ?   b     1      *        ?     :   1   : 1 ;  // cover all transitions of preset
	   0   x     1      *        ?     :   1   : 1 ;  // cover all transitions of preset
	   ?   ?     ?      ?        *     :   ?   : x ;  // on any notifier event output x
	endtable
endprimitive

module dff_p1(q, d, clk, clear, preset, notifier);
   output q;
   input  clk, clear, d, preset, notifier;

   udp_dff_p1 D1(q, d, clk, clear, preset, notifier);
endmodule // dff_p1

`endif

///////////////////////////////////////////////////////////////////////////
//

primitive udp_dff_px(q, d, clk, clear, preset, notifier);
	output q;
	input  clk, clear, d, preset, notifier;
	reg q; /* declaring output as reg*/

	table
	// d  clk  clear  preset  notifier : state : q

	   ?   ?     0      0        ?     :   ?   : x ;  // if clear and preset, set out to x
	   ?   ?     0      1        ?     :   ?   : 0 ;  // if clear, set out to 0
	   ?   ?     1      0        ?     :   ?   : 1 ;  // if not clear, preset to 1
	   0   r     1      1        ?     :   ?   : 0 ;  // on rising edge transmit data
	   1   r     1      1        ?     :   ?   : 1 ;  //  same as above
	   1   *     1      ?        ?     :   1   : 1 ;  //  reduce pessimism
	   0   *     ?      1        ?     :   0   : 0 ;  //  reduce pessimism
	   ?   n     1      1        ?     :   ?   : - ;  // on negedge, do nothing
	   *   ?     1      1        ?     :   ?   : - ;  // on steady clock, ignore data trans.

	   ?   b     *      1        ?     :   0   : 0 ;  // cover all transitions of clear
	   0   x     *      1        ?     :   0   : 0 ;  // cover all transitions of clear
	   ?   ?     1      p        ?     :   ?   : - ;  // ignore positive edge of preset
	   ?   b     1      *        ?     :   1   : 1 ;  // cover all transitions of preset
	   0   x     1      *        ?     :   1   : 1 ;  // cover all transitions of preset
	   ?   ?     ?      ?        *     :   ?   : x ;  // on any notifier event output x
	endtable
endprimitive

module dff_px(q, d, clk, clear, preset, notifier);
   output q;
   input  clk, clear, d, preset, notifier;

   udp_dff_px D1(q, d, clk, clear, preset, notifier);
endmodule // dff_px


///////////////////////////////////////////////////////////////////////////
//
`ifdef TETRAMAX
primitive udp_ldlatch_p0(q, d, en, clear, preset, notifier);
        output q;
        input  d, en, clear, preset, notifier;
        reg q;
        table
        // d  en  clear  preset  notifier : state : q
           ?   ?    0      ?        ?     :   ?   : 0 ;  // clear overrides
           ?   ?    1      0        ?     :   ?   : 1 ;  // set to 1
           0   1    1      1        ?     :   ?   : 0 ;  // on enable, transmit d
           1   1    1      1        ?     :   ?   : 1 ;  // on enable, transmit d
           ?   0    1      1        ?     :   ?   : - ;  // disable, no change

           1   x    1      1        ?     :   1   : 1 ; // reduce pessimism
           0   x    1      1        ?     :   0   : 0 ; // reduce pessimism
           0   1    x      1        ?     :   ?   : 0 ;
           ?   0    x      1        ?     :   0   : 0 ;
           0   ?    x      1        ?     :   0   : 0 ;
           1   1    1      x        ?     :   ?   : 1 ;
           ?   0    1      x        ?     :   1   : 1 ;
           1   ?    1      x        ?     :   1   : 1 ;
        endtable
endprimitive

module ldlatch_p0(q, d, en, clear, preset, notifier);
   output q;
   input  clear, preset, d, en, notifier;

   udp_ldlatch_p0 P1 (q, d, en, clear, preset, notifier);
endmodule // ldlatch_p0

`else
primitive udp_ldlatch_p0(q, d, en, clear, preset, notifier);
        output q;
        input  d, en, clear, preset, notifier;
        reg q;
        table
        // d  en  clear  preset  notifier : state : q
           ?   ?    0      ?        ?     :   ?   : 0 ;  // clear overrides
           ?   ?    1      0        ?     :   ?   : 1 ;  // set to 1
           0   1    1      1        ?     :   ?   : 0 ;  // on enable, transmit d
           1   1    1      1        ?     :   ?   : 1 ;  // on enable, transmit d
           ?   0    1      1        ?     :   ?   : - ;  // disable, no change

           1   x    1      1        ?     :   1   : 1 ; // reduce pessimism
           0   x    1      1        ?     :   0   : 0 ; // reduce pessimism
           0   1    x      1        ?     :   ?   : 0 ;
           ?   0    x      1        ?     :   0   : 0 ;
           0   ?    x      1        ?     :   0   : 0 ;
           1   1    1      x        ?     :   ?   : 1 ;
           ?   0    1      x        ?     :   1   : 1 ;
           1   ?    1      x        ?     :   1   : 1 ;
           ?   ?    ?      ?        *     :   ?   : x ;  // on any notifier event output x
        endtable
endprimitive

module ldlatch_p0(q, d, en, clear, preset, notifier);
   output q;
   input  clear, preset, d, en, notifier;

   udp_ldlatch_p0 P1 (q, d, en, clear, preset, notifier);
endmodule // ldlatch_p0

`endif

///////////////////////////////////////////////////////////////////////////
//

primitive udp_ldlatch_p1(q, d, en, clear, preset, notifier);
	output q;
	input  clear, preset, d, en, notifier;
	reg q;

	table
	// d  en  clear	 preset	 notifier : state : q


	   ?   ?    ?      0        ?     :   ?   : 1 ;  // preset to 1
	   ?   ?    0      1        ?     :   ?   : 0 ;  // clear to 0	

	   0   1    1      1        ?     :   ?   : 0 ;  // on enable, transmit d
	   1   1    1      1        ?     :   ?   : 1 ;  // on enable, transmit d

	   0   *    1      1        ?     :   0   : 0 ; // reduce pessimism
	   1   *    1      1        ?     :   1   : 1 ; // reduce pessimism
	   
	   
	   ?   0    1      1        ?     :   ?   : - ;  // not enable, no change


	   ?   1    1  	   *        ?     :   0   : 0 ;  // cover all preset transitions
	   0   1    1  	   *        ?     :   1   : 1 ;  // cover all preset transitions

	   0   1    p      1        ?     :   ?   : - ;
	   1   1    ?      p        ?     :   ?   : - ;
	   ?   1    *  	   1        ?     :   0   : 0 ;  // cover all clear transitions
	   0   1    *  	   1        ?     :   1   : 1 ;  // cover all clear transitions
	   
	   ?   ?    ?      ?        *     :   ?   : x ;  // on any notifier event output x
	endtable
endprimitive

module ldlatch_p1(q, d, en, clear, preset, notifier);
   output q;
   input clear, preset, d, en, notifier;

   udp_ldlatch_p1 P1 (q, d, en, clear, preset, notifier);
endmodule // ldlatch_p1


///////////////////////////////////////////////////////////////////////////
//

primitive udp_ldlatch_px(q, d, en, clear, preset, notifier);
	output q;
	input  clear, preset, d, en, notifier;
	reg q;

	table
	// d  en  clear	 preset	 notifier : state : q


	   ?   ?    0      0        ?     :   ?   : x ;   // indeterminate

	   ?   ?    1      0        ?     :   ?   : 1 ;  // preset to 1
	   ?   ?    0      1        ?     :   ?   : 0 ;  // clear to 0	

	   0   1    1      1        ?     :   ?   : 0 ;  // on enable, transmit d
	   1   1    1      1        ?     :   ?   : 1 ;  // on enable, transmit d

	   0   *    1      1        ?     :   0   : 0 ; // reduce pessimism
	   1   *    1      1        ?     :   1   : 1 ; // reduce pessimism
	   
	   
	   ?   0    1      1        ?     :   ?   : - ;  // not enable, no change
	   
	   ?   ?    p  	   1        ?     :   ?   : - ;  // ignore positive edge of clear        
	   ?   ?    1  	   p        ?     :   ?   : - ;  // ignore positive edge of preset   
	   ?   1    1  	   *        ?     :   0   : 0 ;  // cover all preset transitions
	   0   1    1  	   *        ?     :   1   : 1 ;  // cover all preset transitions

	   ?   1    *  	   1        ?     :   0   : 0 ;  // cover all clear transitions
	   0   1    *  	   1        ?     :   1   : 1 ;  // cover all clear transitions
	   
	   ?   ?    ?      ?        *     :   ?   : x ;  // on any notifier event output x
	endtable
endprimitive

module ldlatch_px(q, d, en, clear, preset, notifier);
   output q;
   input  clear, preset, d, en, notifier;

   udp_ldlatch_px P1 (q, d, en, clear, preset, notifier);
endmodule // ldlatch_px


///////////////////////////////////////////////////////////////////////////
//

primitive udp_jk_ff_sr1 (q, ck, j, k, preset, clear, notifier);
	output q;
	input  ck, j, k, preset, clear, notifier;
	reg q;

	table

	// ck   j   k   p   c  notifier : state : q
	//preset logic
	   ?    ?   ?   0   ?     ?     :   ?   : 1 ;
	   ?    ?   ?   *   1     ?     :   1   : 1 ;
	// clear logic
	   ?    ?   ?   1   0     ?     :   ?   : 0 ;
	   ?    ?   ?   1   *     ?     :   0   : 0 ;
	// normal clocking cases
//	   r    0   0   0   0     ?     :   0   : 1 ;  // check this up
	   r    0   0   1   1     ?     :   ?   : - ;
	   r    0   1   1   1     ?     :   ?   : 0 ;
	   r    1   0   1   1     ?     :   ?   : 1 ;
	   r    1   1   1   1     ?     :   0   : 1 ;
	   r    1   1   1   1     ?     :   1   : 0 ;
	   f    ?   ?   ?   ?     ?     :   ?   : - ;  // ignore falling edges
	// j and k transitions
	   b    *   ?   ?   ?     ?     :   ?   : - ;
	   b    ?   *   ?   ?     ?     :   ?   : - ;
	
	   p    0   0   1   1     ?     :   ?   : - ;
	   p    0   ?   1   ?     ?     :   0   : - ;
	   p    ?   0   ?   1     ?     :   1   : - ;
	  (?0)  ?   ?   ?   ?     ?     :   ?   : - ;
	  (1x)  0   0   1   1     ?     :   ?   : - ;
	  (1x)  0   ?   1   ?     ?     :   0   : - ;
	  (1x)  ?   0   ?   1     ?     :   1   : - ;
	   x    *   0   ?   1     ?     :   1   : - ;
	   x    ?   *   1   ?     ?     :   0   : - ;
	   ?    ?   ?   ?   ?     *     :   ?   : x ;
	endtable
endprimitive

module jk_ff_sr1 (q, ck, j, k, preset, clear, notifier);
   output q;
   input  ck, j, k, preset, clear, notifier;
   
   udp_jk_ff_sr1 I1 (q, ck, j, k, preset, clear, notifier);
endmodule // jk_ff_sr1


///////////////////////////////////////////////////////////////////////////
//

primitive udp_jk_ff_sr0 (q, ck, j, k, preset, clear, notifier);
	output q;
	input  ck, j, k, preset, clear, notifier;
	reg q;

	table

	// ck   j   k   p   c  notifier : state : q
	//preset logic
	   ?    ?   ?   0   1     ?     :   ?   : 1 ;
	   ?    ?   ?   *   1     ?     :   1   : 1 ;
	// clear logic
	   ?    ?   ?   ?   0     ?     :   ?   : 0 ;
	   ?    ?   ?   1   *     ?     :   0   : 0 ;
	// normal clocking cases
//	   r    0   0   0   0     ?     :   0   : 1 ;  // check this up
	   r    0   0   1   1     ?     :   ?   : - ;
	   r    0   1   1   1     ?     :   ?   : 0 ;
	   r    1   0   1   1     ?     :   ?   : 1 ;
	   r    1   1   1   1     ?     :   0   : 1 ;
	   r    1   1   1   1     ?     :   1   : 0 ;
	   f    ?   ?   ?   ?     ?     :   ?   : - ;  // ignore falling edges
	// j and k transitions
	   b    *   ?   ?   ?     ?     :   ?   : - ;
	   b    ?   *   ?   ?     ?     :   ?   : - ;
	
	   p    0   0   1   1     ?     :   ?   : - ;
	   p    0   ?   1   ?     ?     :   0   : - ;
	   p    ?   0   ?   1     ?     :   1   : - ;
	  (?0)  ?   ?   ?   ?     ?     :   ?   : - ;
	  (1x)  0   0   1   1     ?     :   ?   : - ;
	  (1x)  0   ?   1   ?     ?     :   0   : - ;
	  (1x)  ?   0   ?   1     ?     :   1   : - ;
	   x    *   0   ?   1     ?     :   1   : - ;
	   x    ?   *   1   ?     ?     :   0   : - ;
	   ?    ?   ?   ?   ?     *     :   ?   : x ;
	endtable
endprimitive

module jk_ff_sr0 (q, ck, j, k, preset, clear, notifier);
   output q; 
   input  ck, j, k, preset, clear, notifier;
   
   udp_jk_ff_sr0 I1 (q, ck, j, k, preset, clear, notifier);
endmodule // jk_ff_sr0


///////////////////////////////////////////////////////////////////////////
//

primitive udp_inv_clr0 (qn, clr, pre, inp);
	output qn;
	input  clr, pre, inp;

	table

	//  	clr 	pre 	inp	: qn
		0	0	?	: 0;
		1	?	0	: 1;
		1	?	1	: 0;
		?	1	0	: 1;
		?	1	1	: 0;
		x	x	1	: 0;
		x	x	0	: 1;
	endtable
endprimitive

module inv_clr0 (qn, clr, pre, inp);
   output qn;
   input  clr, pre, inp; 

   udp_inv_clr0 (qn, clr, pre, inp);
endmodule // inv_clr0


///////////////////////////////////////////////////////////////////////////
//

primitive udp_inv_clr1 (qn, clr, pre, inp);
	output qn;
	input  clr, pre, inp;

	table

	//  	clr 	pre 	inp	: qn
		0	0	?	: 1;
		1	?	0	: 1;
		1	?	1	: 0;
		?	1	0	: 1;
		?	1	1	: 0;
		x	x	1	: 0;
		x	x	0	: 1;
	endtable
endprimitive
		
module inv_clr1 (qn, clr, pre, inp);
   output qn;
   input  clr, pre, inp;

   udp_inv_clr1 m1(qn, clr, pre, inp);
endmodule // inv_clr1




///////////////////////////////////////////////////////////////////////////
//

primitive udp_mux21 (q, data1, data0, dselect);
    output q;
    input data1, data0, dselect;

// FUNCTION :  TWO TO ONE MULTIPLEXER
table
//data1 data0 dselect :   q
        0     0       ?   :   0 ;
        1     1       ?   :   1 ;

        0     ?       1   :   0 ;
        1     ?       1   :   1 ;

        ?     0       0   :   0 ;
        ?     1       0   :   1 ;
endtable
endprimitive


module mux21 (q, data1, data0, dselect);
   output q;
   input  data1, data0, dselect;

   udp_mux21 m1(q, data1, data0, dselect);
endmodule // mux21



//////////////////////////////////////////////////////////////////////
//
primitive udp_glat (out, in, hold, clr_, set_, NOTIFIER);
   output out;
   input  in, hold, clr_, set_, NOTIFIER;
   reg    out;

   table

// in  hold  clr_   set_  NOT  : Qt : Qt+1
//
  (?1)  0     1      ?    ?    : ?  :  1  ; //
  (?0)  0     ?      1    ?    : ?  :  0  ; //
   0   (?0)   1      1    ?    : ?  :  0  ; // adder to ensure transition
   1   (?0)   1      1    ?    : ?  :  1  ; // adder to ensure transition
   1    *     1      ?    ?    : 1  :  1  ; // reduce pessimism
   0    *     ?      1    ?    : 0  :  0  ; // reduce pessimism
   *    1     ?      ?    ?    : ?  :  -  ; // no changes when in switches
   ?    ?     ?     (?0)  ?    : ?  :  1  ; // set output
   ?    1     1      *    ?    : 1  :  1  ; // cover all transistions on set_
   1    ?     1      *    ?    : 1  :  1  ; // cover all transistions on set_
   ?    ?    (?0)    1    ?    : ?  :  0  ; // reset output
   ?    1     *      1    ?    : 0  :  0  ; // cover all transistions on clr_
   0    ?     *      1    ?    : 0  :  0  ; // cover all transistions on clr_
   ?    ?     ?      ?    *    : ?  :  x  ; // any notifier changed

   endtable
endprimitive // udp_glat

module glat (out, in, hold, clr, set, notifier);
   output out;
   input  in,hold,clr,set,notifier;

   udp_glat m1(out,in,hold,clr,set,notifier);
endmodule // glat
