
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
#ifndef _CACHE_H_
#define _CACHE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "mshr.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

const byte_t CACHE_BLK_VALID  = 0x1;
const byte_t CACHE_BLK_DIRTY  = 0x2;
const byte_t CACHE_BLK_LOCKED = 0x4;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Base virtual class of all cache.
*
* These caches don't control their bandwidth; that is controlled by either
* the "Scheduler" or an MSHR (each cache should be under the domain of one
* of these).
* 
* @see     generic_cache_template
* @author  zilles
* @version $Id: cache.h 1.36 06/02/13 18:49:14-06:00 mikem@maya.cs.wisc.edu $
*/

class cache_t {

public:
  /// constructor
  cache_t(const char *n, mshr_t *mshr, scheduler_t *eventQueue);

  /// Check tag in the tag array
  virtual bool TagCheck(pa_t a) = 0;

  /// Fill a cache block from mshr
  virtual pa_t Fill(pa_t a, bool dirty, bool *writeback, bool prefetched) = 0;

  /// processor read data
  virtual bool Read(pa_t a, waiter_t *w, bool data_request = true,
                    bool *primary = NULL) = 0;

  /// processor write data
  virtual bool Write(pa_t a, waiter_t *w) = 0;

  /// Prefetch data (request from processor)
  virtual void Prefetch(pa_t a) = 0;

  /// print out statistics
  virtual void printStats( pseq_t *pseq );

  /// Accessor
  const char  *GetName( void ) const { return name; }
  mshr_t      *getMSHR( void ) { return mshr; }
  scheduler_t *getEventQueue( void ) { return m_eventQueue; }

protected:
  /** cache name */
  const char  *name;

  /* MSHR (for handling misses) */
  mshr_t      *mshr;

  /** scheduler: local time domain for the caches */
  scheduler_t *m_eventQueue;

  /** @name per-cache stats */
  //@{
  /** total number of reads */
  uint64 reads;
  /** total number of read hits */
  uint64 read_hit;
  /** total number of read misses */
  uint64 read_miss;
  /** total number of read accesses */
  uint64 accesses;

  /** total number of writes */
  uint64 writes;
  /** total number of write hits */
  uint64 write_hit;
  /** number of writes missing at retire */
  uint64 write_miss;
  /** write prefetch count (writes which miss at address generate) */
  uint64 write_prefetches;

  /** total number of replacements at misses */
  uint64 replacements;
  /** total number of writebacks at misses */
  uint64 writebacks;
  //@}
};
  
/**
 * Represents a cache line in a generic cache.
 */
class generic_cache_block_t {
public:
  /** the tag of this cache block.
   *  the low order bits of this field contain the state of the cache block:
   *  CACHE_BLK_VALID, _DIRTY, or _LOCKED, so the mask for these bits is 0x7
   */
  pa_t      address_state;
  
  /** the cycle of the last access to this cache line */
  tick_t     last_access;

  /** prefetched block, reset after the first demand access */
  bool      prefetched;

};

/**
* level n cache.
*
* @see     cache_t
* @author  zilles
* @version $Id: cache.h 1.36 06/02/13 18:49:14-06:00 mikem@maya.cs.wisc.edu $
*/

/* this is a cache which can be used for any level cache. 
   Since lower levels of the cache hierarchies tend to be
   higher associativity, this cache is managed through
   hash-tables. The "ideal" flag turns a cache into always
   hit, and no traffic is generated throught it to lower
   level caches. */

template <class BlockType>
class generic_cache_template : public cache_t {
public:
  /**
   * @name constructor & destructor
   */
  //@{
  generic_cache_template(const char *n, mshr_t *m, scheduler_t *eventQueue,
             uint32 set_bits, uint32 assoc, uint32 _block_size,
             bool ideal);
  virtual ~generic_cache_template();
  //@}

  /// get block size in byte
  /*inline*/ uint32 getBlockSize() { return block_size; };

  /// comparing two PCs, return true if they are in the same line
  inline bool same_line(pa_t pc1, pa_t pc2) {
    return (BlockAddress(pc1) == BlockAddress(pc2));
  };

  /// check tag array for an address
  inline bool TagCheck(pa_t a) {
    return TagSearch(a, false, false, /* block ptr */NULL, /* way */ NULL);
  };

  /// interface used by the procssor
  bool Read(pa_t a, waiter_t *w, bool data_request = true,
            bool *primary = NULL);
  bool Write(pa_t a, waiter_t *w);
  void Prefetch(pa_t a);
  void Warmup(pa_t a);

  /** This function is called when mshr is ready to fill the incoming
   *  cache line
   */
  pa_t Fill(pa_t a, bool dirty, bool *writeback, bool prefetched);
  /** This function is called when upper level mshr want to access
   *  a cache line, and it is convienient to use a miss object
   */
  bool Access(miss_t *m);
  /** This function is called when upper level mshr want to oracle
   *  access a block
   */
  void OracleAccess(pa_t a);

  /// This interface is provided _only_ for saving the state (via confio)
  int  registerCheckpoint( confio_t *conf );
  
protected:

  /// interface used by simics
  static attr_value_t get_cache_data( void *ptr, void *obj );
  static set_error_t  set_cache_data( void *ptr, void *obj, attr_value_t *value );

  /// search for a block with address
  bool TagSearch(pa_t a, bool setMRU, bool setDirty,
                 BlockType** block, uint32* way);

  /// get LRU way of an address
  uint32       GetLRU(BlockType* set);

  /// handler called before a block is replaced
  virtual void DoBeforeReplace(BlockType *b, uint32 way);

  /// handler called after a block is filled
  virtual void DoAfterFill(BlockType *b, uint32 way);

  /// handler called when cache access hit
  virtual void DoWhenAccessHit(BlockType *b, bool read, bool data);

  /// handler called when cache access miss
  virtual void DoWhenAccessMiss(my_addr_t a, bool read, bool data);

  /** @name Cache line state accessors:
   *        gets the cache state (readable, writeable, etc.)
   */
  //@{

  /** get state bits of the block */
  inline uint32 getState(BlockType &cb) { return (cb.address_state & 0x7); }
  /** check if a block is valid */
  inline bool IsValid(BlockType &cb) { 
    return ((cb.address_state & CACHE_BLK_VALID) != 0); 
  }
  /** check if a block is dirty */
  inline bool IsDirty(BlockType &cb) { 
    return ((cb.address_state & CACHE_BLK_DIRTY) != 0);
  }

  /** convert the address 'a' to a location (set) in cache */
  inline uint32 Set(pa_t a) { return (set_mask & (pa_t) (a >> block_bits));  }

  /** convert the address 'a' to tag, it is shifted */
  inline pa_t   Tag(pa_t a) { return (a >> (set_bits + block_bits));  }

  /** convert the address 'a' to the block address, not shifted */
  inline pa_t   BlockAddress(pa_t a) { return (a & ~(pa_t)block_mask); }

  /** get the block address of a cache block */
  inline pa_t   getBlockAddress(BlockType &cb) { return (cb.address_state & ~0x7); }

  //@}  

  /**
   * @name Private Variables
   */
  //@{
  /* Cache sets */
  BlockType *cache;

  /** number of sets */
  uint32 n_sets;
  /** block size in bytes */
  uint32 block_size;
  /** cache associativity */
  uint32 m_assoc;

  /*
   * derived data for fast decoding
   */

  /** number of bits in the set */
  uint32 set_bits;
  uint32 set_mask;

  /** number of bits in the tags */
  uint32 block_bits;
  uint32 block_mask;

  /** ideal flag */
  bool m_ideal;
  //@}
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _CACHE_H_ */
