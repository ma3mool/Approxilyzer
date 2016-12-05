
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


#include "hfa.h"
#include "hfacore.h"
#include "fileio.h"
#include "decode.h"

/// a constant to indicate uninitialized min latency
const uint64  DECODE_DEFAULT_MIN_LATENCY = 10000;

/// constructor
//***************************************************************************
decode_stat_t::decode_stat_t()
{
  m_num_unmatched = 0;
  m_op_seen = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_succ = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_functional = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_squash = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_noncompliant = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );

  m_op_memory_counter = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_memory_latency = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_min_execute = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_max_execute = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_total_execute = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );

  for ( int i = 0; i < i_maxcount; i++ ) {
    m_op_seen[i] = 0;
    m_op_succ[i] = 0;
    m_op_functional[i] = 0;
    m_op_squash[i] = 0;
    m_op_noncompliant[i] = 0;

    m_op_min_execute[i] = DECODE_DEFAULT_MIN_LATENCY;
    m_op_max_execute[i] = 0;
    m_op_total_execute[i] = 0;

    m_op_memory_counter[i] = 0;
    m_op_memory_latency[i] = 0;
  }
}

/// destructor
//***************************************************************************
decode_stat_t::~decode_stat_t()
{
  free( m_op_seen );
  free( m_op_succ );
  free( m_op_functional );
  free( m_op_squash );
  free (m_op_noncompliant);

  free (m_op_min_execute);
  free (m_op_max_execute);
  free (m_op_total_execute);

  free (m_op_memory_counter);
  free (m_op_memory_latency);
}

//***************************************************************************
void decode_stat_t::opMemoryLatency( enum i_opcode op, uint16 latency )
{
  m_op_memory_counter[op]++;
  m_op_memory_latency[op] += latency;
}

//***************************************************************************
void decode_stat_t::opExecuteLatency( enum i_opcode op, uint16 latency )
{
  if (m_op_min_execute[op] > latency) {
    m_op_min_execute[op] = latency;
  }
  if (m_op_max_execute[op] < latency) {
    m_op_max_execute[op] = latency;
  }
  m_op_total_execute[op] += latency;
}

/// print out table
//***************************************************************************
void    decode_stat_t::print( out_intf_t *io )
{
  int64 total_seen = 0;
  int64 total_succ = 0;
  int64 total_functional = 0;
  int64 total_squash = 0;
  int64 total_noncompliant = 0;
  char  buf[40], buf1[40], buf2[40], buf3[40];

  io->out_info("###: decode              seen        success function     fail\n");
  io->out_info( "Unmatched     %lld\n", m_num_unmatched);
  for (int i = 0; i < (int) i_maxcount; i++) {
    if ( m_op_seen[i] || m_op_succ[i] || m_op_functional[i]) {
      io->out_info(  "%03d: %-9.9s %14.14s %14.14s %8.8s %8.8s\n",
               i, decode_opcode((enum i_opcode) i), 
               commafmt( m_op_seen[i], buf, 40 ),
               commafmt( m_op_succ[i] - m_op_noncompliant[i], buf1, 40 ),
               commafmt( m_op_functional[i], buf2, 40 ),
               commafmt( (m_op_seen[i] - m_op_succ[i] + m_op_noncompliant[i]), buf3, 40 ) );
      total_seen += m_op_seen[i];
      total_succ += m_op_succ[i];
      total_functional += m_op_functional[i];
      total_noncompliant += m_op_noncompliant[i];
    }
  }

  io->out_info( "TOTALI    : %14.14s %14.14s %8.8s %8.8s\n",
                  commafmt( total_seen, buf, 40 ),
                  commafmt( (total_succ - total_noncompliant), buf1, 40 ),
                  commafmt( total_functional, buf2, 40 ),
                  commafmt( (total_seen - total_succ + total_noncompliant),
                            buf3, 40 ) );
  io->out_info( "NON_COMP  : %14.14s\n",
                  commafmt( total_noncompliant, buf, 40 ) );
  io->out_info( "Percent functional: %f\n",
                  100.0 * ((float) (total_functional) / (float) total_seen));
  io->out_info( "Percent correct      : %f\n",
                  100.0 * ((float) (total_succ - total_noncompliant) / (float) total_seen));
  
  io->out_info( "\n*** Latency and Squashes\n");
  io->out_info( "\n000: %-9.9s %10.10s %10.10s mem latency  min max avg exec\n", "opcode", "# squashed", "# non-comp", "min" );
  for (int i = 0; i < (int) i_maxcount; i++) {
    // 
    if ( m_op_seen[i] &&
         m_op_min_execute[i] != DECODE_DEFAULT_MIN_LATENCY ) {
      double average_mem_latency = 0.0;
      if (m_op_memory_counter[i] != 0) {
        average_mem_latency = (float) m_op_memory_latency[i] / (float) m_op_memory_counter[i];
      }
      double average_exec_latency = 0.0;
      if (m_op_seen[i] != 0) {
        average_exec_latency = ((float) m_op_total_execute[i]/(float)m_op_seen[i]);
      }
      io->out_info(  "%03d: %-9.9s %10.10s %10.10s %6lld %6.1f  %3lld %3lld %6.1f\n",
                       i, decode_opcode((enum i_opcode) i), 
                       commafmt( m_op_squash[i], buf, 40 ),
                       commafmt( m_op_noncompliant[i], buf1, 40 ),
                       m_op_memory_counter[i],
                       average_mem_latency,
                       
                       m_op_min_execute[i],
                       m_op_max_execute[i],
                       average_exec_latency );
      total_squash += m_op_squash[i];
    }
  }
  io->out_info( "SQUASHED  : %14.14s\n", 
                  commafmt( total_squash, buf, 40 ) );
}
