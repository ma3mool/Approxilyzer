
/*
    Copyright (C) 1999-2005 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Opal Timing-First Microarchitectural
    Simulator, a component of the Multifacet GEMS (General 
    Execution-driven Multiprocessor Simulator) software toolset 
    originally developed at the University of Wisconsin-Madison.

    Opal was originally developed by Carl Mauer based upon code by
    Craig Zilles.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Manoj Plakal, Daniel Sorin, Min Xu, and Luke Yen.

    --------------------------------------------------------------------

    If your use of this software contributes to a published paper, we
    request that you (1) cite our summary paper that appears on our
    website (http://www.cs.wisc.edu/gems/) and (2) e-mail a citation
    for your published paper to gems@cs.wisc.edu.

    If you redistribute derivatives of this software, we request that
    you notify us and either (1) ask people to register with us at our
    website (http://www.cs.wisc.edu/gems/) or (2) collect registration
    information and periodically send it to us.

    --------------------------------------------------------------------

    Multifacet GEMS is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    Multifacet GEMS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Multifacet GEMS; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307, USA

    The GNU General Public License is contained in the file LICENSE.

### END HEADER ###
*/
#ifndef _DECODE_H_
#define _DECODE_H_

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Count statistics related to the decoded instructions.
*
* @author  cmauer
* @version $Id: decode.h 1.24 06/02/13 18:49:15-06:00 mikem@maya.cs.wisc.edu $
*/
class decode_stat_t {

public:
  /// constructor
  decode_stat_t();
  /// destructor
  ~decode_stat_t();

  /// decoded dynamic instruction
  void    seenOp( enum i_opcode op ) {
    STAT_INC( m_op_seen[op] );
  }

  /// correctly! executed the op code
  void    successOp( enum i_opcode op ) {
    STAT_INC( m_op_succ[op] );
  }
  
  /// functional retirement
  void    functionalOp( enum i_opcode op ) {
    STAT_INC( m_op_functional[op] );
  }

  /// opcode caused a pipeline squash
  void    squashOp( enum i_opcode op ) {
    STAT_INC( m_op_squash[op] );
  }

  /// retire event, measuring its (total) memory latency (if any)
  void    opMemoryLatency( enum i_opcode op, uint16 latency );

  /// execute to ready to retire latency
  void    opExecuteLatency( enum i_opcode op, uint16 latency );
  
  /// non-compliant event, unattributable to single instruction
  void    incrementNoncompliant( enum i_opcode op ) {
    STAT_INC( m_op_noncompliant[op] );
  }

  /// print out table
  void    print( out_intf_t *io );
  
private:
  /// The number of unmatched instructions
  uint64             m_num_unmatched;

  /// An array representing the functional instructions.
  uint64            *m_op_functional;
  /// An array representing which instructions have been seen
  uint64            *m_op_seen;
  /// An array of which instructions have been correctly exectuted
  uint64            *m_op_succ;
  /// An array of which instructions squashed
  uint64            *m_op_squash;
  /// An array non-complaint instructions
  uint64            *m_op_noncompliant;

  /// An array of the minimum execute latency of these instructions
  uint64            *m_op_memory_counter;
  /// An array of the maximum execute of these instruction
  uint64            *m_op_memory_latency;

  /// An array of the minimum execute latencies (fetch->retire)
  uint64            *m_op_min_execute;
  /// An array of the maximum execute latencies
  uint64            *m_op_max_execute;
  /// An array of the minimum total execution latencies of each instructions
  uint64            *m_op_total_execute;
};

#endif  /* _DECODE_H_ */
