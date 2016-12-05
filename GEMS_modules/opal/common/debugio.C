
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
/*
 * FileName:  debugio.C
 * Synopsis:  formats, filters debugging log file messages.
 * Author:    
 * Version:   $Id: debugio.C 1.19 06/02/13 18:49:10-06:00 mikem@maya.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "fileio.h"
#include "debugio.h"
#include "system.h"

#include "structuralModule.h"
#include "faultSimulate.h"

extern structuralModule design;
extern faultSimulator fSim;

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

static const bool     g_compressed_output = false;
#define OUTPUT_WHEN_DIFF 1

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// declaration of system wide log file
FILE *debugio_t::m_logfp = NULL;

/// declaration for all faulty things that can happen :)
FILE *debugio_t::m_faultlogfp = NULL;
FILE *debugio_t::m_faultstatfp = NULL ;
FILE *debugio_t::m_faultretfp = NULL ;
FILE *debugio_t::m_tracefp = NULL;
char debugio_t::m_app_name[256]="\0";
string debugio_t::m_faulty_machine;
int debugio_t::m_faultstuckat=-1;
uint64 debugio_t::m_start_time=0;

uint64 debugio_t::m_global_cycle = 0;
debugio_t::shared_address_map_t debugio_t::shared_address_map[2];
debugio_t::corrupt_addresses_t debugio_t::corrupt_addresses[2];
uint32 debugio_t::m_faulty_core = 0;
int debugio_t::count_core[NUM_CORES] = {0};
bool debugio_t::state_mismatch = false;
bool debugio_t::symptomDetected[NUM_CORES] = {false};
uint32 debugio_t::m_num_procs = 1;
bool debugio_t::m_is_masked = false;

// Statics defs for mallc table objects
bool debugio_t::in_warmup = false ;
#ifdef BINARY_32_BIT
ireg_t debugio_t::min_tos = 0xffffffff ;
ireg_t debugio_t::min_heap = 0xffffffff ;
#else // BINARY_32_BIT
ireg_t debugio_t::min_tos = 0xffffffffffffffff ;
ireg_t debugio_t::min_heap = 0xffffffffffffffff ;
#endif // BINARY_32_BIT
ireg_t debugio_t::max_heap = 0x0 ;
ireg_t debugio_t::max_tos  = 0x0 ;
debugio_t::object_table_t debugio_t::object_table ;
debugio_t::prune_object_table_t debugio_t::prune_object_table ;
debugio_t::char_int_t debugio_t::stack_start ;
debugio_t::char_int_t debugio_t::stack_end ;
debugio_t::char_int_t debugio_t::heap_start ;
debugio_t::char_int_t debugio_t::heap_end ;
debugio_t::reg_object_table_t debugio_t::reg_object_table ;
debugio_t::mem_object_table_t debugio_t::mem_object_table ;

// debugio_t::inst_count_map_t debugio_t::fault_inst_list ;
/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* out_intf_t class                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
void out_intf_t::out_info( const char *fmt, ...)
{
    va_list  args;

    // DEBUG_FILTER
#ifdef DEBUG_FILTER
    if (!validDebug())
        return;
#endif
    //fprintf(stdout,  "[%d] ", m_id);
    if (m_logfp)
        fprintf(m_logfp, "[%d] ", m_id);
    va_start(args, fmt);
    int error_rt = vfprintf(stdout, fmt, args);
    va_end(args);
    ASSERT(error_rt >= 0);
    if (m_logfp){
        va_start(args, fmt);
        int error_rt = vfprintf(m_logfp, fmt, args);
        va_end(args);
        ASSERT(error_rt >= 0);
    }
}

// print which only goes to the file
//**************************************************************************
void out_intf_t::out_log( const char *fmt, ...)
{
    va_list  args;

    // DEBUG_FILTER
#ifdef DEBUG_FILTER
    if (!validDebug())
        return;
#endif
    if (!m_logfp)
        return;
    fprintf(m_logfp, "[%d] ", m_id);
    va_start(args, fmt);
    int error_rt = vfprintf(m_logfp, fmt, args);
    va_end(args);
    ASSERT(error_rt >= 0);
}

//**************************************************************************
void out_intf_t::setDebugTime(uint64 t)
{
    m_starting_cycle = t;
    debugio_t::setDebugTime( t );
}

//**************************************************************************
bool out_intf_t::validDebug( void )
{
    if ( m_starting_cycle > system_t::inst->getGlobalCycle() ) {
        return false;
    }
    return true;
}

/*------------------------------------------------------------------------*/
/* Global debug I/O methods                                               */
/*------------------------------------------------------------------------*/

//**************************************************************************
debugio_t::debugio_t()
{
    m_logfp = NULL;
}

//**************************************************************************
debugio_t::~debugio_t()
{
}

// system wide info
//**************************************************************************
void debugio_t::out_info( const char *fmt, ... )
{
    // DEBUG_FILTER
#ifdef DEBUG_FILTER
    if ( system_t::inst &&
         m_global_cycle > system_t::inst->getGlobalCycle() )
        return;
#endif
    va_list args;
    va_start(args, fmt);
    int error_rt = vfprintf(stdout, fmt, args);

    va_end(args);
    ASSERT(error_rt >= 0);
    if (m_logfp) {
        va_start(args, fmt);
        int error_rt = vfprintf(m_logfp, fmt, args);
        va_end(args);
        ASSERT(error_rt >= 0);
    }
}

// system wide log file
//**************************************************************************
void debugio_t::out_log( const char *fmt, ...)
{
    va_list args;
    if (m_logfp){
        va_start(args, fmt);
        int error_rt = vfprintf(m_logfp, fmt, args);
        va_end(args);
        ASSERT( error_rt >= 0);
    }
}

// system wide error
//**************************************************************************
void debugio_t::out_error( const char *fmt, ... )
{
    // out_error should never be filtered: we must know if something goes wrong
    va_list args;
    va_start(args, fmt);
    int error_rt = vfprintf(stderr, fmt, args);
    va_end(args);
    ASSERT( error_rt >= 0);
    if (m_logfp){
        va_start(args, fmt);
        int error_rt = vfprintf(m_logfp, fmt, args);
        va_end(args);
        ASSERT( error_rt >= 0 );
    }
}

// system wide fault log
//**************************************************************************
void debugio_t::out_fault( const char *fmt, ... )
{
    // out_error should never be filtered: we must know if something goes wrong
    va_list args;

    if (NO_FAULT_LOG) return;

    //FILE *f_logf = fopen( fileName, "a" ) ;
    va_start(args, fmt);
    int error_rt ; 
    if( m_faultlogfp ) {
        error_rt = vfprintf(m_faultlogfp, fmt, args);
    } else {
        error_rt = fprintf(stdout, fmt, args);
    }
    va_end(args);
    //fclose(f_logf) ;
    ASSERT( error_rt >= 0);
}

void debugio_t::out_trace( const char *fmt, ... )
{
    va_list args;

    va_start(args, fmt);
    int error_rt ; 
    if( m_tracefp ) {
        error_rt = vfprintf(m_tracefp, fmt, args);
    } else {
        error_rt = fprintf(stdout, fmt, args);
    }
    va_end(args);
    ASSERT( error_rt >= 0);
}

#define STORE_BRANCH_THRESHOLD 100000

void debugio_t::out_fault_ret( const char *fmt, ... )
{
    // LXL: disabled for this revision
    //if (store_branch_cnt > STORE_BRANCH_THRESHOLD)
#ifndef ALEX_FAULT_RET_DEBUG
    return ;
#endif

    va_list args;
    va_start(args, fmt);
    int error_rt ; 
    if( m_faultretfp ) {
        error_rt = vfprintf(m_faultretfp, fmt, args);
    } else {
        error_rt = fprintf(stdout, fmt, args);
    }
    va_end(args);
    ASSERT( error_rt >= 0);

}

void debugio_t::out_fault_stat( const char *fmt, ... )
{
    va_list args;
    va_start(args, fmt);
    int error_rt ; 
    if( m_faultstatfp ) {
        error_rt = vfprintf(m_faultstatfp, fmt, args);
    } else {
        error_rt = fprintf(stdout, fmt, args);
    }
    va_end(args);
    ASSERT( error_rt >= 0);
}

//**************************************************************************
void debugio_t::openLog( const char *logFileName )
{
    if ( g_compressed_output ) {
        char   cmd[FILEIO_MAX_FILENAME];
        sprintf( cmd, "gzip > %s.gz", logFileName );
        m_logfp = popen( cmd, "w" );
    } else {
        m_logfp = fopen( logFileName, "w" );
        if ( m_logfp == NULL ) {
            ERROR_OUT("error: unable to open log file %s\n", logFileName );
        }
    }
}

//**************************************************************************
void debugio_t::closeLog( void )
{
    if ( m_logfp != NULL && m_logfp != stdout ) {
        if ( g_compressed_output ) {
            pclose( m_logfp );
        } else {
            fclose( m_logfp );
        }
        m_logfp = NULL;
    }
}

//*************************************************************************
  // Functions for manipulating fault things
void debugio_t::openFaultLog( char *logname )
{
    char statname[100] ;
    char retname[100] ;
    if (!NO_FAULT_LOG) {
        m_faultlogfp = fopen( logname, "w" ) ;
    }
    strcpy(statname, logname) ;
    strcat(statname, ".stat") ;
    m_faultstatfp = fopen( statname, "w" ) ;
    strcpy(retname, logname) ;
#ifdef ALEX_FAULT_RET_DEBUG
     strcat(retname, ".ret") ;
     m_faultretfp = fopen( retname, "w" ) ;
#endif

#if (LXL_PROB_FM || \
		defined(RANGE_CHECK) || \
		defined(COLLECT_RANGE) || \
		defined(HARD_BOUND) || \
		defined(HARD_BOUND_BASE) || \
		defined(GEN_DATA_VALUE_INV) || \
		defined(CHECK_DATA_VALUE_INV) || \
		defined(CHECK_DATA_ONLY_VALUES))
     extractAppName(logname,m_app_name);
#endif

#ifdef VAR_TRACE
     string filename = logname ;
     string::size_type loc = filename.find(".fault_log",0) ;
     filename.replace(loc, filename.length(), ".dtrace") ;
     m_tracefp = fopen( filename.c_str(),"w") ;
     // Now insert a first record in to this trace, specifying all
     // the variables that we will be seeing
     TRACE_OUT("--- DECLARE_START ---\n") ;
     // Format is
     // <var_name,
     //  type ,
     //  number of bits>
     // <var_name...>
#ifdef FETCH_CHECKER
     TRACE_OUT("fetch_ready\n1\n1\n") ;
     TRACE_OUT("PC\n1\n64\n") ;
     TRACE_OUT("inst\n2\n32\n") ;
#endif // FETCH_CHECKER

#ifdef DECODER_CHECKER
     TRACE_OUT("inst_1\n1\n2\n") ;
     TRACE_OUT("inst_2\n1\n3\n") ;
     TRACE_OUT("inst_3\n1\n6\n") ;
     TRACE_OUT("inst_4\n1\n5\n") ;
     // TRACE_OUT("inst_5\n1\n5\n") ;
     // TRACE_OUT("inst_6\n1\n5\n") ;
     // TRACE_OUT("inst_7\n1\n5\n") ;
     // TRACE_OUT("inst_8\n1\n4\n") ;
     // TRACE_OUT("inst_9\n1\n4\n") ;
     // TRACE_OUT("inst_10\n1\n4\n") ;
     // TRACE_OUT("inst_11\n1\n3\n") ;
     // TRACE_OUT("inst_12\n1\n1\n") ;
     // TRACE_OUT("inst_13\n1\n8\n") ;
     TRACE_OUT("m_type\n2\n8\n") ;
     TRACE_OUT("m_futype\n2\n8\n") ;
     TRACE_OUT("m_opcode\n2\n16\n") ;
     TRACE_OUT("m_branch_type\n2\n8\n") ;
     TRACE_OUT("m_imm\n2\n64\n") ;
     TRACE_OUT("m_ccshift\n2\n8\n") ;
     // TRACE_OUT("m_access_size\n%d\n2\n",m_access_size ) ;
     // for(int i=0;i<8;i++) {
     //     TRACE_OUT("m_flags[%d]\n2\n1\n",i) ;
     // }

#endif // DECODER_CHECKER

#ifdef RENAME_CHECKER
     TRACE_OUT("l1\n1\n16\n") ;
     TRACE_OUT("l2\n1\n16\n") ;
     TRACE_OUT("p1\n2\n16\n") ;
     TRACE_OUT("p2\n2\n16\n") ;

#endif // RENAME_CHECKER

#ifdef FP_CHECKER
     // TRACE_OUT("type\n1\n1\n") ;
     TRACE_OUT("s1\n1\n64\n") ;
     TRACE_OUT("s2\n1\n64\n") ;
     TRACE_OUT("dest\n2\n64\n") ;

#endif // FP_CHECKER

     TRACE_OUT("--- DECLARE_END ---\n") ;
#endif // VAR_TRACE

     struct timeval tv;
     gettimeofday(&tv, NULL);
     m_start_time=tv.tv_sec*1e6+tv.tv_usec;
}

void debugio_t::processFMProfile()
{
#if LXL_PROB_FM
     char fm_filename[FILEIO_MAX_FILENAME];
     char tmp[100];
     //char phase_name[100]; // In case we want phase also

//     strcpy(fm_filename,"./uarch_model/");
//     strcat(fm_filename,m_app_name);
//     strcat(fm_filename,"-app_s");
//     sprintf(tmp,"%d_a",m_faultstuckat);
//     strcat(fm_filename,tmp);
//     strcat(fm_filename,m_faulty_machine.c_str());
//     strcat(fm_filename,".txt");
//
//     int act_rate = atoi(m_faulty_machine.c_str());
//
//     DEBUG_OUT("gonna open %s act rate %d\n",fm_filename,act_rate);
//
//     fSim.processFaultModelFile(fm_filename, act_rate);
//     //exit(1);

     strcpy(fm_filename,"./agen_wire_dir_model/");
     strcat(fm_filename,m_app_name);
     strcat(fm_filename,"-app_s");
     sprintf(tmp,"%d_w",m_faultstuckat);
     strcat(fm_filename,tmp);
     strcat(fm_filename,m_faulty_machine.c_str());
     strcat(fm_filename,".txt");

     int act_rate = 0 ;

     DEBUG_OUT("Profile from file %s\n",fm_filename) ;

     fSim.processFaultModelFile(fm_filename, act_rate);
     //exit(1);
#endif

}

void debugio_t::extractAppName(char* log_name,char* out_string)
{
    int length=strlen(log_name);
    char*log_ptr=log_name;
    int i;
    bool done=false;

    while (!done && log_ptr) {
        //DEBUG_OUT("log_ptr %c\n",log_ptr[0]);
        if (!strncmp(log_ptr,"-app",4)) {
            while (log_ptr[0]!='/') {
                log_ptr--;
            }
            log_ptr++;
            i=0;
            while (log_ptr[i]!='-') {
                out_string[i]=log_ptr[i];
                i++; 
            }
            out_string[i]='\0';
            done = true;
        }
        log_ptr++;
    }
}

void debugio_t::out_trap_info(uint64 cyc, uint64 inst, const char* trap_name, 
                            unsigned int tt, la_t tpc, int priv, la_t thpc, int tl,int coreId)
{
    out_info( "coreId:%d:", coreId ) ;
#ifdef NEW_TRAP_OUTPUT
    out_info( "%u:", cyc ) ;
    out_info( "%u:", inst ) ;
    out_info( "0x%x:", tt ) ;
    out_info( "0x%x:", tpc ) ;
    out_info( "%d\n", priv ) ;
    //    out_info( "0x%x:", thpc ) ;
    //    out_info( "%u\n", tl ) ;
#else
    out_info( "[%u]\t", cyc ) ;
    out_info( "[%u]\t", inst ) ;
    out_info( "\t%s", trap_name ) ;
    out_info( "(0x%x) ", tt ) ;
    out_info( "pc:0x%x", tpc ) ;
    out_info( "(%d)\t", priv ) ;
    out_info( "\ttrappc:0x%x", thpc ) ;
    out_info( "(%u)\n", tl ) ;
#endif
    
}

void debugio_t::closeFaultLog( void )
{
    if (m_faultlogfp)
        fclose(m_faultlogfp) ;
    if (m_faultstatfp)
        fclose(m_faultstatfp) ;
#ifdef ALEX_FAULT_RET_DEBUG
    if (m_faultretfp) 
        fclose(m_faultretfp) ;
#endif
#ifdef VAR_TRACE
    if(m_tracefp) 
        fclose(m_tracefp) ;
#endif // VAR_TRACE
}

void debugio_t::setSymptomDetected() {
    if(m_num_procs >= NUM_CORES)
        out_info(" WARNING: Num cores is hardcoded to %d\n", NUM_CORES);

    for(int i=0; i<NUM_CORES; i++) {
        symptomDetected[i] = true;
    }
}
// Returns whether the given address has been written in to
bool debugio_t::isShared(uint64 physical_address, int priv)
{
    return (shared_address_map[priv].find(physical_address) != shared_address_map[priv].end());
}

// Returns that code id of the writer of this phys addr
uint32 debugio_t::getMappedCoreID(uint64 physical_address, int priv)
{
    if(isShared(physical_address, priv)) {
        return shared_address_map[priv][physical_address][0].core_id;
    } else {
        return 100;    //it should never come here
    }
}

//void debugio_t::updateMap(uint64 physical_address, uint32 core_id, int priv)
//{
//    shared_address_map[priv][physical_address] = core_id;
//}

void debugio_t::clear_corrupted_address_map()
{
    shared_address_map[0].clear();    //app addresses
    shared_address_map[1].clear();    //os addresses
}

// Add an entry for this shared address
void debugio_t::add_shared_address(uint64 physical_address, int priv, struct addr_info info)
{
    shared_address_map[priv][physical_address].push_back(info);
}

bool debugio_t::isCorruptedNow(uint64 physical_address, int priv) 
{
    return shared_address_map[priv][physical_address].back().corrupted;
}

void debugio_t::printSharedAddresses(int map_id)
{
    long num_shared = 0;
    long num_addresses = 0;
    long num_other_core = 0;

    map<uint64, vector<struct addr_info> >::const_iterator it;
    for(it = shared_address_map[map_id].begin(); it != shared_address_map[map_id].end(); ++it) {
        string line="";
        char temp[100];
        bool flag_fault_free = false;    //for fault popagation, fault read from faulty and then by fault free core
        bool flag_faulty = false;    // for access by faulty core
        bool flag_other_core_access = false; // for address accessed by other core, only in the case of TRACK_AKWAYS_MEM_SHARING

        if(it->second.size() != 1) {
            num_addresses++;
            sprintf(temp, "%llx: ",it->first);    //address
            line = "";
            line.append(temp,strlen(temp));

            for (int i = 0; i < it->second.size(); i++) {

#ifdef TRACK_ALWAYS_MEM_SHARING
                if(it->second[i].core_id != getFaultyCore()) {
                    flag_other_core_access = true;
                }
                if((it->second[i].core_id == getFaultyCore()) && (it->second[i].rw == 1)) {
                    flag_faulty = true;
                }
#else
                flag_faulty = true;
#endif

                //those which have read from the non-faulty core
                // if TRACK_ALWAYS_MEM_SHARING is def then count only ones that are written by faulty core

                if((it->second[i].core_id != getFaultyCore()) && flag_faulty) {
#ifndef TRACK_ALWAYS_MEM_SHARING
                    if(it->second[i].corrupted)
#endif
                        flag_fault_free = true;
                }
            }
            //print everything as the map has only corrupted addresses
            //if( flag_fault_free ) 
            {
                for (int i = 0; i < it->second.size(); i++) {
                    //print <core_id>:<pc>:<corrupted>:<read/write>
                    sprintf(temp, "%d:%llx:%d:%d ",it->second[i].core_id, it->second[i].pc, it->second[i].corrupted, it->second[i].rw);
                    line.append(temp,strlen(temp));
                }
                if( flag_fault_free ) 
                    num_shared++;
            }

#ifdef TRACK_ALWAYS_MEM_SHARING
            if(flag_other_core_access)
                num_other_core++;
#endif

            sprintf(temp, "\n");
            line.append(temp,strlen(temp));
        }
        if(flag_fault_free && false)
            out_info("%s",line.c_str());
    }
    out_info("Num Propagated [%ld] = %d\n", map_id, num_shared);
    out_info("Num Addresses [%ld] = %d\n", map_id, num_addresses);

#ifdef TRACK_ALWAYS_MEM_SHARING
    out_info("Num Other core [%ld] = %d\n", map_id, num_other_core);
#endif

}

void fault_stats::addFaultyInstruction(const char *op)
{
    // DEBUG_OUT("Adding faulty inst %s\n", op) ;
    fault_inst_list[op] ++ ;
}
void fault_stats::printFaultInstList()
{
    FAULT_STAT_OUT("### Faulty instruction types ###\n") ;
    for(inst_count_map_t::iterator I=fault_inst_list.begin(),
            E=fault_inst_list.end(); I!=E; I++) {
        const char *op = (*I).first ;
        FAULT_STAT_OUT("%s\t: %d\n", op, fault_inst_list[op]) ;
    }
}

void fault_stats::recordInstMix(const char *type, bool has_fault) {
    if(stat_total_inject>0) { // should we record this information after the fault is injected?
        inst_mix[type] ++ ;
        if(has_fault) {
            faulty_inst_mix[type] ++ ;
        }
    }
}

void fault_stats::printInstMix()
{
    FAULT_STAT_OUT("\n### Instruction Mix ###\n") ;
    FAULT_STAT_OUT("ctrl Inst\t%d\t%d\n", inst_mix["ctrl"], faulty_inst_mix["ctrl"]) ;
    FAULT_STAT_OUT("store Inst\t%d\t%d\n", inst_mix["store"], faulty_inst_mix["store"]) ;
    FAULT_STAT_OUT("load Inst\t%d\t%d\n", inst_mix["load"], faulty_inst_mix["load"]) ;
    FAULT_STAT_OUT("other Inst\t%d\t%d\n", inst_mix["other"], faulty_inst_mix["other"]) ;
    FAULT_STAT_OUT("### End Instruction Mix ###\n\n") ;
}

// Functions to maniulate ctrl statements -> targets mapping
void fault_stats::addInstTargets(ireg_t pc, ireg_t target)
{
    // DEBUG_OUT("Adding 0x%llx -> 0x%llx\n", pc, target) ;
    inst_targets[pc].insert(target) ;
}

void fault_stats::printInstTargets()
{
    DEBUG_OUT("### Targets of ctrl instructions ###\n");
    for(inst_targets_t::iterator I=inst_targets.begin(), E=inst_targets.end();
            I!=E; I++) {
        ireg_t pc = (*I).first ;
        DEBUG_OUT("0x%llx -> ", pc) ;
        targets_t targets = inst_targets[pc] ;
        for(targets_t::iterator II=targets.begin(), EE=targets.end();
                II!=EE; II++) {
            DEBUG_OUT("0x%llx, ", *II) ;
        }
        DEBUG_OUT("\n") ;
    }
}

// Functions to maniulate mem inst -> target range mapping
void fault_stats::buildInstRange(int priv, ireg_t pc, ireg_t target)
{
    // DEBUG_OUT("0x%llx accesses 0x%llx\n", pc, target) ;
    if(priv) {
        if(os_addr_range.find(pc) == os_addr_range.end()) { 
            addr_range_t range ;
            range.first = range.second = target ;
            os_addr_range[pc] = range ;
        } else { // This pc was seen before
            addr_range_t range = os_addr_range[pc] ;
            if(target < range.first) {
                os_addr_range[pc].first = target ;
            } else if( target > range.second ) {
                os_addr_range[pc].second = target ;
            }
        }
    } else {
        if(app_addr_range.find(pc) == app_addr_range.end()) { 
            addr_range_t range ;
            range.first = range.second = target ;
            app_addr_range[pc] = range ;
        } else { // This pc was seen before
            addr_range_t range = app_addr_range[pc] ;
            if(target < range.first) {
                app_addr_range[pc].first = target ;
            } else if( target > range.second ) {
                app_addr_range[pc].second = target ;
            }
        }
    }
    // DEBUG_OUT("0x%llx new range - 0x%llx -> 0x%llx\n",
    //         pc, inst_addr_range[pc].first, inst_addr_range[pc].second) ;
}

void fault_stats::printInstRanges()
{
    // DEBUG_OUT("### Ranges of addresses of App instructions ###\n");
    DEBUG_OUT("### Ranges of addresses for instructions ###\n");
    for(inst_addr_range_t::iterator I=app_addr_range.begin(), E=app_addr_range.end();
            I!=E; I++) {
        ireg_t pc = (*I).first ;
        addr_range_t range = app_addr_range[pc] ;
        // DEBUG_OUT("0x%llx: 0x%llx -> 0x%llx\n",
        DEBUG_OUT("0x%llx\t0x%llx\t0x%llx\n",
                pc, range.first, range.second) ; 
    }

    DEBUG_OUT("\n") ;

    // DEBUG_OUT("### Ranges of addresses of OS instructions ###\n");
    // for(inst_addr_range_t::iterator I=os_addr_range.begin(), E=os_addr_range.end();
    //         I!=E; I++) {
    //     ireg_t pc = (*I).first ;
    //     addr_range_t range = os_addr_range[pc] ;
    //     // DEBUG_OUT("0x%llx: 0x%llx -> 0x%llx\n",
    //     DEBUG_OUT("0x%llx\t0x%llx\t0x%llx\n",
    //             pc, range.first, range.second) ; 
    // }
    DEBUG_OUT("### End of address Ranges ###\n") ;
}

// "app.ranges" has all the ranges of a given application.
// assumed format: pc <tab> min <tab> max <end>
void fault_stats::readInstRangesFile()
{
    char name[FILEIO_MAX_FILENAME] = "" ;
    strcpy(name, "./golden_ranges/") ;
    strcat(name, debugio_t::getAppName()) ;
    strcat(name, ".ranges") ;
    DEBUG_OUT("Range file: %s\n", name) ;
    FILE *f_ranges = fopen(name, "r") ;
    if(!f_ranges) {
        ERROR_OUT("File not found: %s\n", name) ;
        assert(0) ;
    }

    bool start = false ;
    bool end = false ;
    char token[256], *token_ptr ;
    char line[256], *line_ptr ;
    line_ptr = &line[0] ;
    while(!end && !feof(f_ranges)) {
        fgets(line,256,f_ranges) ;
        if(!strcmp(line,"### Ranges of addresses for instructions ###\n")) {
            start = true ;
        } else if(!strcmp(line,"### End of address Ranges ###\n")) {
            end = true ;
        } else if(start && strcmp(line,"\n")) {
            token_ptr = strtok(line_ptr,"\t") ;
            ireg_t pc, min, max ;
            pc = strtoul(token_ptr, NULL, 16) ;
            token_ptr = strtok(NULL,"\t") ;
            min = strtoul(token_ptr, NULL, 16) ;
            token_ptr = strtok(NULL,"\t") ;
            max = strtoul(token_ptr, NULL, 16) ;
            // Do this for only array accesses.
            // FIXME - Remove comment for only array accesses
            if(min!=max) { 
                addr_range_t this_range ;
                this_range.first = min ;
                this_range.second = max ;
                golden_range[pc] = this_range ;
            }
        }
    }
    // for(inst_addr_range_t::iterator I=golden_range.begin(), E=golden_range.end();
    //         I!=E; I++) {
    //     ireg_t pc = (*I).first ;
    //     addr_range_t range = golden_range[pc] ;
    //     // DEBUG_OUT("0x%llx: 0x%llx -> 0x%llx\n",
    //     DEBUG_OUT("0x%llx\t0x%llx\t0x%llx\n",
    //              pc, range.first, range.second) ; 
    // }
}

// check whether in the golden run, this address was accessed by this
// pc. If so, then we're ok. If not, this is out-of-bounds access
bool fault_stats::rangeCheck(ireg_t pc, ireg_t addr) {
    if(golden_range.find(pc) == golden_range.end()) {
        // Then we have no history of this pc. be conservative
        return true ;
    }
    addr_range_t range = golden_range[pc] ;
    if( (addr>=range.first) && (addr<=range.second) ) {
        // Then this is a valid range
        return true ;
    }
#ifdef STACK_ONLY
    if(addr<=0xffffffff7f000000 ) { // stack ends here, always!
        return true ;
    }
#endif //STACK_ONLY
    
    return false ;
}

void fault_stats::readDataOnlyValuesList()
{
    char name[FILEIO_MAX_FILENAME] = "" ;
    strcpy(name, "./test_golden/10m_data_only_pcs/") ;
    strcat(name, debugio_t::getAppName()) ;
    strcat(name, ".txt") ;
    DEBUG_OUT("Data Only Values file: %s\n", name) ;
    FILE *f_det = fopen(name, "r") ;
    if(!f_det) {
        ERROR_OUT("File not found: %s\n", name) ;
        assert(0) ;
    }

    bool start = false ;
    bool end = false ;
    char token[256], *token_ptr ;
    char line[256], *line_ptr ;
    line_ptr = &line[0] ;
    while(!end && !feof(f_det)) {
        fgets(line,256,f_det) ;
        if(strcmp(line,"\n")) {
            token_ptr = strtok(line_ptr,"\t") ;
            ireg_t pc, min, max ;
            pc = strtoul(token_ptr, NULL, 16) ;
			data_only_values.insert(pc) ;
        }
    }

    // for(detector_list_t::iterator I=data_only_values.begin(), E=data_only_values.end();
    //         I!=E; I++) {
    //     ireg_t pc = *I ;
    //     DEBUG_OUT("PC = 0x%llx\n", pc) ;
    // }
	DEBUG_OUT("Number of Data Only Values = %d\n", data_only_values.size()) ;
}

bool fault_stats::isDataOnly(ireg_t pc)
{
	return (data_only_values.find(pc)!=data_only_values.end()) ;
}

void fault_stats::readDetectorList()
{
    char name[FILEIO_MAX_FILENAME] = "" ;
    strcpy(name, "./detectors/") ;
    strcat(name, debugio_t::getAppName()) ;
    strcat(name, ".detectors") ;
    DEBUG_OUT("Detectors file: %s\n", name) ;
    FILE *f_det = fopen(name, "r") ;
    if(!f_det) {
        ERROR_OUT("File not found: %s\n", name) ;
        assert(0) ;
    }

    bool start = false ;
    bool end = false ;
    char token[256], *token_ptr ;
    char line[256], *line_ptr ;
    line_ptr = &line[0] ;
    while(!end && !feof(f_det)) {
        fgets(line,256,f_det) ;
        if(strcmp(line,"\n")) {
            token_ptr = strtok(line_ptr,"\t") ;
            ireg_t pc, min, max ;
            pc = strtoul(token_ptr, NULL, 16) ;
			detector_list.insert(pc) ;
        }
    }

    // for(detector_range_t::iterator I=detector_range.begin(), E=detector_range.end();
    //         I!=E; I++) {
    //     ireg_t pc = (*I).first ;
    //     DEBUG_OUT("PC = 0x%llx\n", pc) ;
    // }
	DEBUG_OUT("Number of detectors = %d\n", detector_list.size()) ;
	detector_values.clear() ;
}

bool fault_stats::hasDetector(ireg_t pc)
{
    return (detector_list.find(pc)!=detector_list.end()) ;
}

void fault_stats::addToRange(ireg_t pc, ireg_t value)
{
	addToRange(pc, (float64)value) ;
}

void fault_stats::addToRange(ireg_t pc, float32 value)
{
	addToRange(pc, (float64)value) ;
}

void fault_stats::addToRange(ireg_t pc, float64 value)
{
	if(!hasDetector(pc)) {
		return ;
	}
	if(detector_range.find(pc)==detector_range.end()) {
		value_range_t range(DBL_MAX,-DBL_MAX) ;
		detector_range[pc] = range ;
	}
	value_range_t &range = detector_range[pc] ;
#ifdef GEN_DATA_VALUE_INV
	if(DEBUG_GEN_DATA_VALUE_INV) {
		DEBUG_OUT("Old range = %.4f - %.4f\n",
				detector_range[pc].first, detector_range[pc].second) ;
		DEBUG_OUT("Value = %.4f\n", value) ;
	}
#endif // GEN_DATA_VALUE_INV
	if(value < range.first) {
		range.first = value ;
	}
	if(value > range.second) {
		range.second = value ;
	}
#ifdef GEN_DATA_VALUE_INV
	if(DEBUG_GEN_DATA_VALUE_INV) {
		DEBUG_OUT("New range = %.4f - %.4f\n",
				detector_range[pc].first, detector_range[pc].second) ;
	}
	if(PRINT_DATA_VALUES) {
		detector_values[pc].push_back(value) ;
	}
#endif // GEN_DATA_VALUE_INV
}

// "app.ranges" has all the ranges of a given application.
// assumed format: pc <tab> min <tab> max <end>
void fault_stats::readDetectorRanges()
{
    char name[FILEIO_MAX_FILENAME] = "" ;
    strcpy(name, "./detectors/") ;
    strcat(name, debugio_t::getAppName()) ;
    strcat(name, ".new_ranges") ;
    DEBUG_OUT("Reading Range file: %s\n", name) ;
    FILE *f_ranges = fopen(name, "r") ;
    if(!f_ranges) {
        ERROR_OUT("File not found: %s\n", name) ;
        assert(0) ;
    }

    char token[256], *token_ptr ;
    char line[256], *line_ptr ;
    line_ptr = &line[0] ;
    while(!feof(f_ranges)) {
        fgets(line,256,f_ranges) ;
        if(strcmp(line,"\n")) {
            token_ptr = strtok(line_ptr,"\t") ;
            ireg_t pc;
			float64 min, max ;
            pc = strtoul(token_ptr, NULL, 16) ;
            token_ptr = strtok(NULL,"\t") ;
            min = strtod(token_ptr, NULL) ;
            token_ptr = strtok(NULL,"\t") ;
            max = strtod(token_ptr, NULL) ;
			// DEBUG_OUT("0x%llx\t%.4f\t%.4f\n", pc, min, max) ;
            // if(min>max) {
            //     ERROR_OUT("Error: 0x%llx has min=%llu, max=%llu\n", pc,min,max) ;
            //     assert(0) ; 
            // }
            if(min<=max) { 
                value_range_t this_range ;
                this_range.first = min ;
                this_range.second = max ;
                detector_range[pc] = this_range ;
				detector_list.insert(pc) ;
            }
        }
    }
    // for(detector_range_t::iterator I=detector_range.begin(), E=detector_range.end();
    //         I!=E; I++) {
    //     ireg_t pc = (*I).first ;
    //     value_range_t range = detector_range[pc] ;
    //     // DEBUG_OUT("0x%llx: 0x%llx -> 0x%llx\n",
    //     DEBUG_OUT("0x%llx\t0x%llx\t0x%llx\n",
    //              pc, range.first, range.second) ; 
    // }
	DEBUG_OUT("Number of detectors = %d\n", detector_list.size()) ;
}

bool fault_stats::isInRange(ireg_t pc, ireg_t value)
{
	return isInRange(pc, (float64)value) ;
}

bool fault_stats::isInRange(ireg_t pc, float32 value)
{
	return isInRange(pc, (float64)value) ;
}

bool fault_stats::isInRange(ireg_t pc, float64 value)
{
	char s_value[100] ;
	sprintf(s_value, "%.4f", value) ;
	float64 round_value = strtod(s_value, NULL) ;
    if(hasDetector(pc)) {
        value_range_t range = detector_range[pc] ;
        if(round_value>=range.first && round_value<=range.second) {
#ifdef CHECK_DATA_VALUE_INV
            if(DEBUG_CHECK_DATA_VALUE_INV) {
                DEBUG_OUT("0x%llx: %.4f < \"%.4f\" < %.4f\n", pc, range.first, round_value, range.second) ;
            }
#endif // CHECK_DATA_VALUE_INV
            return true ;
        } else {
            DEBUG_OUT("%.4f is out of [%.4f,%.4f] for PC 0x%llx\n", round_value, range.first, range.second, pc) ;
            return false ;
        }
    } else {
		// Then there is no detector
		return true ;
	}
}

void fault_stats::printDetectorRanges()
{
	if(!(detector_range.empty())) {
		DEBUG_OUT("\n#### Detector Ranges ####\n") ;
		for(detector_range_t::iterator I=detector_range.begin(), E=detector_range.end();
				I!=E; I++) {
			ireg_t pc = (*I).first ;
			value_range_t range = (*I).second ;
			DEBUG_OUT("0x%llx\t%.4f\t%.4f\n", pc, range.first, range.second) ;
		}
		DEBUG_OUT("#### End Detector Ranges ####\n") ;
	}

#ifdef GEN_DATA_VALUE_INV
	if(PRINT_DATA_VALUES) {
		DEBUG_OUT("\n#### Detector Values ####\n") ;
		for(detector_values_t::iterator I=detector_values.begin(), E=detector_values.end();
				I!=E; I++) {
			ireg_t pc = (*I).first ;
			DEBUG_OUT("0x%llx", pc) ;
			value_list_t data_values = (*I).second ;
			for(value_list_t::iterator I=data_values.begin(), E=data_values.end(); I!=E; I++) {
				float64 value = *I ;
				DEBUG_OUT("\t%.4f", value) ;
			}
			DEBUG_OUT("\n" );
		}
		DEBUG_OUT("#### End Detector Values ####\n\n") ;
	}
#endif // GEN_DATA_VALUE_INV
}


void fault_stats::buildObjectTable(ireg_t base, ireg_t offset)
{
    if(offset>base) {
        ireg_t temp = base ;
        base=offset ;
        offset=temp ;
    }
    ireg_t end ;
    if(base > 0xffffffff7f000000) { // then this is stack
        end = base - offset ;
    } else { // Heap object
        end = base + offset ;
    }
    if(fake_object_table.find(base) == fake_object_table.end()) {
        fake_object_table[base] = end ;
        num_references[base] = 1 ;
        offsets_t off ;
        off.insert(offset) ;
        base_offsets[base] = off ;
    } else {
        if(end > fake_object_table[base]) {
            fake_object_table[base] = end ;
        }
        base_offsets[base].insert(offset) ;
        num_references[base] ++ ;
    }
}

// It may sometimes be necessary to find ranges that overlap and prune
// them in to one big monolythic range
void fault_stats::pruneObjectTable()
{
    makeRangesSane() ;
    bool change = false ;
    // for(fake_object_table_t::iterator I=fake_object_table.begin(),E=fake_object_table.end();
    //         I!=E;I++) {
    //     ireg_t base1 = (*I).first ;
    //     ireg_t end1 = fake_object_table[base1] ;
    // }
}

// Function to make sure base > end in fake_object_table. Also update
// associated strucutres to ensure consistency.
void fault_stats::makeRangesSane() {
    // for(object_table_t::iterator I=fake_object_table.begin(),E=fake_object_table.end();
    //         I!=E;I++) {
    //     ireg_t base = (*I).first ;
    //     ireg_t end = fake_object_table[base] ;
    //     if(base > end) { // For stack objects. So, reverse them!
    //         fake_object_table[base].erase() ;
    //         offsets_t offsets = base_offsets[base] ;
    //         base_offsets[base].erase() ;
    //         // If the end has alrady been a base, combine the ranges
    //         if(fake_object_table.find(end)==fake_object_table.end()) {
    //             fake_object_table[end] = base ;
    //             base_offsets[end] = offsets ;
    //         } else {
    //             if(base > fake_object_table[end]) {
    //                 fake_object_table[end] = base ;
    //             }
    //             for(offsets_t::iterator II=offsets.begin(),EE=offsets.end();
    //                     II!=EE; II++) {
    //                 fake_object_table[end].insert(*II) ;
    //             }
    //         }
    //         num_offsets[end] = base_offsets[end].size() ;
    //     } else { // 
    //         num_offsets[base] = base_offsets[base].size() ;
    //     }
    // }
}
ireg_t fault_stats::getObjectEnd(ireg_t addr)
{
    // Common case is if one of the bases is this addr
    if(fake_object_table.find(addr) != fake_object_table.end()) {
        return fake_object_table[addr] ;
    } else { // Walk through the map to find the best fit
        for(fake_object_table_t::iterator I=fake_object_table.begin(),E=fake_object_table.end();
                I!=E;I++) {
            ireg_t base = (*I).first ;
            ireg_t end = fake_object_table[base] ;
            if( (addr>=base) && (addr<=end) ) {
                return end ;
            }
        }
        // If not, we don't know anything about this object. return 0
        return 0 ;
    }
}

void fault_stats::printObjectTable()
{
    // pruneObjectTable() ;
    DEBUG_OUT("#### Object table ####\n") ;
    for(fake_object_table_t::iterator I=fake_object_table.begin(),E=fake_object_table.end();
            I!=E;I++) {
        ireg_t base = (*I).first ;
        ireg_t end = fake_object_table[base] ;
        DEBUG_OUT("0x%llx\t0x%llx\t%d\n", base, end, base_offsets[base].size()) ;
    }
    // DEBUG_OUT("#### Offsets table ####\n") ;
    // for(base_offsets_t::iterator I=base_offsets.begin(),E=base_offsets.end();
    //         I!=E;I++) {
    //     ireg_t base = (*I).first ;
    //     offsets_t offsets = base_offsets[base] ;
    //     DEBUG_OUT("0x%llx -> ", base) ;
    //     for(offsets_t::iterator I=offsets.begin(), E=offsets.end(); I!=E; I++) {
    //         DEBUG_OUT("0x%llx, ", *I) ;
    //     }
    //     DEBUG_OUT("\n") ;
    // }
}

// Functions to maniulate mem statements -> target addresses
void fault_stats::addMemAccess(ireg_t pc, ireg_t address)
{
    // PC of 0x0 is simply unimplemented instructions in opal.
    // Should really disregard them
    if(pc==0x0) {
        return ;
    }
    // DEBUG_OUT("Adding 0x%llx -> 0x%llx\n", pc, target) ;
    inst_targets[pc].insert(address) ;
    if(pc<min_target) {
        min_target = pc ;
    }
    if(pc>max_target) {
        max_target = pc ;
    }
}

void fault_stats::printMemAccess()
{
    DEBUG_OUT("### Addresses accessed by Memory inst ###\n");
    DEBUG_OUT("Range: 0x%llx\t0x%llx\n", min_target, max_target) ;
    for(inst_targets_t::iterator I=inst_targets.begin(), E=inst_targets.end();
            I!=E; I++) {
        ireg_t pc = (*I).first ;
        DEBUG_OUT("0x%llx\t", pc) ;
        // DEBUG_OUT("0x%llx -> ", pc) ;
        targets_t targets = inst_targets[pc] ;
        DEBUG_OUT("%d", targets.size()) ;
        // for(targets_t::iterator II=targets.begin(), EE=targets.end();
        //         II!=EE; II++) {
        //     DEBUG_OUT("0x%llx, ", *II) ;
        // }
        DEBUG_OUT("\n") ;
    }
}

// functions to manipulate the prefetcher
// getPrefetch returns the prefetch address if the entry for this
// pc is stable. If not, it returns 0x0
ireg_t fault_stats::getPrefetch(ireg_t pc, ireg_t addr)
{
    ireg_t ret_addr ;
    struct prefetch_data_t *data ;
    if(DEBUG_PREFETCH) {
        DEBUG_OUT("Getting prefetch for 0x%llx, 0x%llx\n", pc, addr) ;
    }
    if(prefetch_table.find(pc) != prefetch_table.end()) {
        // We have seen this pc before
        if(DEBUG_PREFETCH) {
            DEBUG_OUT("Found pc\n") ;
        }
        ret_addr = prefetch_table[pc]->update(addr) ;
    } else {
        if(DEBUG_PREFETCH) {
            DEBUG_OUT("PC not foudn\n") ;
        }
        prefetch_table[pc] = new prefetch_data_t(addr);
        // DEBUG_OUT("0x%llx\n", prefetch_table[pc]) ;
        ret_addr = 0x0 ;
    }
    return ret_addr ;
}

void fault_stats::printPrefetchTable()
{
}

// Functions to manipulate the corrupt_addresses map
bool debugio_t::isCorruptAddress(uint64 addr, int p)
{
    if(corrupt_addresses[p].find(addr) == corrupt_addresses[p].end()) {
        return false ;
    } else {
        return true ;
    }
}

void debugio_t::addCorruptAddress(uint64 addr, int p)
{
    corrupt_addresses[p].insert(addr) ;
    // DEBUG_OUT("%d: %llx corrupt\n", p, addr) ;
}

void debugio_t::removeCorruptAddress(uint64 addr, int p)
{
    corrupt_addresses[p].erase(addr) ;
}

void debugio_t::printCorruptAddress()
{
    DEBUG_OUT("### Faulty Addresses ###\n") ;
    for(corrupt_addresses_t::iterator I=corrupt_addresses[0].begin(),
            E=corrupt_addresses[0].end(); I!=E; I++) {
        DEBUG_OUT("0:%llx\n", *I) ;
    }
    for(corrupt_addresses_t::iterator I=corrupt_addresses[1].begin(),
            E=corrupt_addresses[1].end(); I!=E; I++) {
        DEBUG_OUT("1:%llx\n", *I) ;
    }
}

// Object table functions
void debugio_t::readObjectTable()
{
    char name[FILEIO_MAX_FILENAME] = "" ;
    // strcpy(name, "./object_table/") ;

    if(strcmp(debugio_t::getAppName(), "sshd") == 0) {
        // give sshd's heap start, end, stack end addr
        heap_start[getAppName()] = 0x0008b09c ;
        heap_end[getAppName()]   = 0x004f0000 ;
        stack_end[getAppName()]  = 0xfe0f0000 ;
    } else if (strcmp(debugio_t::getAppName(), "httpd")==0) {
        // give httpd's heap start, end, stack end addr
        heap_start[getAppName()] = 0x00016480 ;
        heap_end[getAppName()]   = 0x001f0000 ;
        stack_end[getAppName()]  = 0xfd000000 ;
    } else if (strcmp(debugio_t::getAppName(), "mysql")==0) {
        // give mysql's heap start, end, stack end addr
        heap_start[getAppName()] = 0x000100e8 ;
        heap_end[getAppName()]   = 0x01f00000 ;
        stack_end[getAppName()]  = 0xfe000000 ;
    } else if (strcmp(debugio_t::getAppName(), "squid")==0) {
        // give squid's heap start, end, stack end addr
        heap_start[getAppName()] = 0x00016484 ;
        heap_end[getAppName()]   = 0x004fffff ;
        stack_end[getAppName()]  = 0xf0000000 ;
    } else {
        strcpy(name, "./all_object_table/") ;
        strcat(name, debugio_t::getAppName()) ;
        strcat(name, ".table") ;
        DEBUG_OUT("Object file: %s\n", name) ;
        FILE *f_ranges = fopen(name, "r") ;
        if(!f_ranges) {
            ERROR_OUT("File not found: %s\n", name) ;
            assert(0) ;
        }

        char token[256], *token_ptr ;
        char line[256], *line_ptr ;
        line_ptr = &line[0] ;
        ireg_t min_base = 0xffffffffffffffff ;
        ireg_t max_bound = 0x0 ;
        while(!feof(f_ranges)) {
            fgets(line,256,f_ranges) ;
            // DEBUG_OUT("Line = %s\n") ;
            if(strcmp(line,"\n")) {
                token_ptr = strtok(line_ptr,"\t") ;
                ireg_t base, bound ;
                base = strtoul(token_ptr, NULL, 16) ;
                token_ptr = strtok(NULL,"\t") ;
                bound = strtoul(token_ptr, NULL, 16) ;
                // DEBUG_OUT("Line = %s Obj = 0x%llx\t0x%llx\n", line, base, bound) ;
                // Do this for only array accesses.
                // FIXME - Remove comment for only array accesses
                if(base>0x100000000 && bound>=base) { // The heap starts after 4GB for 64-bit apps
                    addr_range_t this_obj ;
                    this_obj.first = base ;
                    this_obj.second = bound ;
                    // object_table.push_back(this_obj) ;
                    // Check for overlap
                    bool inserted = false ;
#ifdef HARD_BOUND
                    // Only for the more sophisticated one, we do this whole object merging
                    for(int i=0;i<object_table.size();i++) {
                        addr_range_t this_range = object_table[i] ;
                        if(this_range.first<=this_obj.first && this_range.second>=this_obj.second) {
                            // Then current object is a sub range
                            inserted = true ;
                            // DEBUG_OUT("(0x%llx,0x%llx) is subrange of (0x%llx,0x%llx)\n",
                            //         this_obj.first, this_obj.second,
                            //         this_range.first, this_range.second) ;
                        } else if(this_obj.first>=this_range.first &&
                                this_obj.first<=this_range.second &&
                                this_obj.second>=this_range.second) {
                            // Then, this object makes this range bigger
                            object_table[i].second = this_obj.second ;
                            inserted = true ;
                            // DEBUG_OUT("(0x%llx,0x%llx) stretches bound of (0x%llx,0x%llx)\n",
                            //         this_obj.first, this_obj.second,
                            //         this_range.first, this_range.second) ;
                        } else if(this_obj.first<=this_range.first &&
                                this_obj.second>=this_range.first &&
                                this_obj.second<=this_range.second) {
                            // Then, the object makes the base smaller
                            object_table[i].first = this_obj.first ;
                            inserted = true ;
                            // DEBUG_OUT("(0x%llx,0x%llx) stretches base of (0x%llx,0x%llx)\n",
                            //         this_obj.first, this_obj.second,
                            //         this_range.first, this_range.second) ;
                        } else if(this_obj.first<=this_range.first &&
                                this_obj.second>=this_range.second) { // obj overshadows range
                            object_table[i].first = this_obj.first ;
                            object_table[i].second = this_obj.second ;
                            inserted = true ;
                            // DEBUG_OUT("(0x%llx,0x%llx) overshadows of (0x%llx,0x%llx)\n",
                            //         this_obj.first, this_obj.second,
                            //         this_range.first, this_range.second) ;
                        }
                        if(inserted) {
                            break ;
                        }
                    }
#endif // HARD_BOUND
                    if(!inserted) {
                        // DEBUG_OUT("New range: (0x%llx, 0x%llx)\n", this_obj.first, this_obj.second) ;
                        object_table.push_back(this_obj) ;
                    }
                    if(base<min_base) {
                        min_base = base ;
                    }
                    if(bound>max_bound) {
                        max_bound = bound ;
                    }
                }
            } else { // Aritifically put a '\n' at the end
                break ;
            }
        }
        // for(object_table_t::iterator I=object_table.begin(), E=object_table.end();
        //         I!=E; I++) {
        //     addr_range_t addr = *I ;
        //     prune_object_table[addr.first] = addr ;
        // }
        // for(object_table_t::iterator I=object_table.begin(), E=object_table.end();
        //         I!=E; I++) {
        //     addr_range_t addr = *I ;
        //     DEBUG_OUT("0x%llx\t0x%llx\n", addr.first, addr.second) ;
        // }


        // Record the heap and stack. Got heap start from pmap of process
        // Got stack end from profiling golden phases

        // heap_start["ammp"]   = 0x100128000 ;
        // heap_start["art"]    = 0x100108000 ;
        // heap_start["mesa"]   = 0x1001BC000 ;
        // heap_start["equake"] = 0x10010A000 ;
        // heap_start["gcc"]    = 0x1002C8000 ;
        // heap_start["mcf"]    = 0x100106000 ;
        // heap_start["gzip"]   = 0x100116000 ;
        // heap_start["bzip2"]  = 0x10010E000 ;
        // heap_start["parser"] = 0x100126000 ;
        // heap_start["twolf"]  = 0x100150000 ;
        // addr_range_t globals_obj ;
        // globals_obj.first = heap_start[getAppName()] ;
        // globals_obj.second = min_base - 0x1 ;
        // object_table.push_back(globals_obj) ;

        heap_start[getAppName()] = min_base ;
        heap_end[debugio_t::getAppName()] = max_bound ;

        stack_end["gcc"]     = 0xffffffff7f35d2adULL ;
        stack_end["equake"]  = 0xffffffff7f4851d0ULL ;
        stack_end["ammp"]    = 0xffffffff7f41dbe0ULL ;
        stack_end["gzip"]    = 0xffffffff7f35d2adULL ;
        stack_end["mcf"]     = 0xffffffff7ffffb40ULL ;
        stack_end["bzip2"]   = 0xffffffff7fffe5d0ULL ;
        stack_end["twolf"]   = 0xffffffff7ffff718ULL ;
        stack_end["art"]     = 0xffffffff7f41dbe0ULL ;
        stack_end["parser"]  = 0xffffffff7fffe8a0ULL ;
        stack_end["mesa"]    = 0xffffffff7f41b228ULL ;
        stack_end["crafty"]  = 0xffffffff7f700328ULL ;
        stack_end["eon"]     = 0xffffffff7e000000ULL ; // TODO - This is not really tos. fixup
        stack_end["gap"]     = 0xffffffff7fffe780ULL ;
        stack_end["perlbmk"] = 0xffffffff7ffff4e0ULL ;
        stack_end["vortex"]  = 0xffffffff7f35d2adULL ;
        stack_end["vpr"]     = 0xffffffff7f48aae0ULL ;

        // stack_end is always 0xffffffff80000000ULL ;

        if(DEBUG_HARD_BOUND) {
            DEBUG_OUT("Object table size = %d\n", object_table.size()) ;
            DEBUG_OUT("Heap = 0x%llx -> 0x%llx\n",
                    heap_start[getAppName()], heap_end[getAppName()]) ;
            DEBUG_OUT("Stack = 0x%llx -> 0xffffffff80000000\n",
                    stack_end[getAppName()]) ;
        }
        DEBUG_OUT("Done reading object table\n") ;
    }
}

// Is this address accesing a valid object?
// Assume here there is no overlap between the objects
bool debugio_t::isValidObject(ireg_t addr)
{
    // Check if given address is between the start and end of heap/stack
    char app_name[100] = "" ;
    strcpy(app_name, debugio_t::getAppName()) ;
    if( addr>=heap_start[app_name] && addr<=heap_end[app_name] ) { // Heap address
        // for(object_table_t::iterator I=object_table.begin(), E=object_table.end() ;
        //         I!=E; I++) {
        //     addr_range_t this_obj = *I ;
        //     if(addr>=this_obj.first && addr<=this_obj.second) {
        //         // DEBUG_OUT("0x%llx accesses 0x%llx->0x%llx\n",
        //         //         addr, this_obj.first, this_obj.second) ;
        //         return true ;
        //     }
        // }
        return true ;
#ifdef BINARY_32_BIT
    } else if(addr>=stack_end[app_name] && addr<=0xffc00000) { // stack access
#else // BINARY_32_BIT
    } else if(addr>=stack_end[app_name] && addr<=0xffffffff7fffffff) { // stack access
#endif // BINARY_32_BIT
        return true ;
    }
    // Library functions may access 64-bit addresses. Profile to find these and mask :)
    if( (strcmp(getAppName(),"sshd")==0) ) {
        if( ( addr==0x70000085d44 ) ||
            ( addr>=0x1ffbf6a00 && addr<=0x1ffbf6b00 ) ||
            ( addr>=0x1feef5000 && addr<=0x1ffbfffff ) ||
            ( addr>=0x1cfefe5000 && addr<=0x1cfefe7000 ) ||
            ( addr>=0x1a0fefe6000 && addr<=0x1f0feffffff ) ||
            ( addr>=0x1a2cfefe6000 && addr<=0x1a58feffffff ) ) {
            return true ;
        }
    } else if( (strcmp(getAppName(),"httpd")==0) ) {
        if( addr>=0xff28d27c && addr<=0x1ff28ce7c ) {
            return true ;
        }
    } else if( (strcmp(getAppName(),"mysql")==0) ) {
        if( addr>=0xff28d27c && addr<=0x1ff28ce7c ) {
            return true ;
        }
    } else if( (strcmp(getAppName(),"squid")==0) ) {
        if( addr<=0x1ffffffff ) {
            return true ;
        }
    }
    return false ;
}

bool debugio_t::isHeapAddress(ireg_t addr)
{
    char app_name[100] = "" ;
    strcpy(app_name, debugio_t::getAppName()) ;
    // DEBUG_OUT("Heap start = 0x%llx\n", heap_start[app_name]) ;
    if(addr>=heap_start[app_name] && addr<0xffffffff7f000000) {
        return true ;
    }
    return false ;
}

bool debugio_t::isHeapAccess(ireg_t addr)
{
    char app_name[100] = "" ;
    strcpy(app_name, debugio_t::getAppName()) ;
    // DEBUG_OUT("Heap start = 0x%llx\n", heap_start[app_name]) ;
    if(addr>=heap_start[app_name] && addr<=heap_end[app_name]) {
        return true ;
    }
    return false ;
}

#define PRINT_FAIL 1
// Assumption: Assume that dest, src1, etc. are all global/int regs and their
// uniqueness is guaranteed by opal
// We split this into 2 phases
// 1. If src is a ptr to an object, address should be in the range of the object
// 2. If src is not a valid object, and
//        a. In warmup, check if addr is an object. Then src pts to that obj
// 3. If mem is a ptr to a valid object, then destination now pts to that object
int debugio_t::handleObjectLoad(half_t dest, half_t src1, half_t src2, ireg_t addr, int priv, int in_trap)
{
    // SHouldn't do this check here because there then we never do propagation.
    // if(!isHeapAccess(addr)) {
    //     return 0 ;
    // }
    // DEBUG_OUT("Load: r%d<-r%d, 0x%llx\n", dest, src1, addr) ;
    if(DEBUG_HARD_BOUND && isHeapAccess(addr)) {
        DEBUG_OUT("Loading from heap 0x%llx (%d)\n", addr, in_trap) ;
    }
    half_t base ;
    if(src1==PSEQ_FLATREG_NWIN) {
        base = src2 ;
    } else if(src2==PSEQ_FLATREG_NWIN) {
        base = src1 ;
    } else { // BOth are valid registers. Assume src1 (the larger value) is base
        base = src1 ;
    }
    if(reg_object_table.find(src1)!=reg_object_table.end()) {
        addr_range_t obj = reg_object_table[src1] ;
        if(addr<obj.first || addr>obj.second) {
            if(PRINT_FAIL || DEBUG_HARD_BOUND) {
                DEBUG_OUT("addr out of range: 0x%llx  (0x%llx, 0x%llx)\n",
                        addr, obj.first, obj.second) ;
            }
            return 1 ;
        }
    } else if(reg_object_table.find(src2)!=reg_object_table.end()) { // src2 is the ptr then
        addr_range_t obj = reg_object_table[src2] ;
        if(addr<obj.first || addr>obj.second) {
            if(PRINT_FAIL || DEBUG_HARD_BOUND) {
                DEBUG_OUT("addr out of range: 0x%llx  (0x%llx, 0x%llx)\n",
                        addr, obj.first, obj.second) ;
            }
            return 1 ;
        }
    } else { // Sources are not ptrs to objects
        if(!HARD_BOUND_WARMUP || isinWarmup()) {
            addr_range_t this_object = getObject(addr) ;
            if(this_object.first==0x0) { // invalid obj
                // Removing this from here for now because the caller does
                // the check, and is part of the basic hard_bound check
                // if(!priv && isHeapAddress(addr)) { // isHeapAccess(addr)) // FIXME - Should this be heapaccess?
                //     if(PRINT_FAIL || DEBUG_HARD_BOUND) {
                //         DEBUG_OUT("0x%llx invalid heap address\n") ;
                //     }
                //     return 1 ;
                // }
            } else {
                if(!in_trap) {
                    reg_object_table[base] = this_object ;
                    if(DEBUG_HARD_BOUND) {
                        DEBUG_OUT("r%d pts to (0x%llx, 0x%llx)\n",
                                base, this_object.first, this_object.second) ;
                    }
                }
            }
        }
    }
    if(!in_trap) {
        if(mem_object_table.find(addr)==mem_object_table.end()) {
            if(reg_object_table.find(dest)!=reg_object_table.end()) {
                reg_object_table.erase(dest) ;
                if(DEBUG_HARD_BOUND) {
                    DEBUG_OUT("Removing %d\n", dest) ;
                }
            }
        } else {
            addr_range_t obj = mem_object_table[addr] ;
            if(dest!=PSEQ_FLATREG_NWIN) { // "g0" doesn't mean anything
                reg_object_table[dest] = obj ;
                if(DEBUG_HARD_BOUND) {
                    DEBUG_OUT("r%d pts to (0x%llx, 0x%llx)\n",
                            dest, obj.first, obj.second) ;
                }
            }
        }
    }
    return 0 ;
}

// For store,
// 1. If sources are ptrs to objects, address should be in the object range.
// 2. Else, but in warmup, see if address is of an objects and sources will have ptr
//       to that object
// 3. If destination is a ptr to an object, mem now points to that obj. Else, clear mem
int debugio_t::handleObjectStore(half_t dest, half_t src1, half_t src2, ireg_t addr, int priv, int in_trap)
{
    if(DEBUG_HARD_BOUND && isHeapAccess(addr)) {
        DEBUG_OUT("Storing to heap 0x%llx (%d)\n", addr, in_trap) ;
    }
    half_t base ;
    if(src1==PSEQ_FLATREG_NWIN) {
        base = src2 ;
    } else if(src2==PSEQ_FLATREG_NWIN) {
        base = src1 ;
    } else { // BOth are valid registers. Assume src1 (the larger value) is base
        base = src1 ;
    }
    if(reg_object_table.find(src1)!=reg_object_table.end()) {
        addr_range_t obj = reg_object_table[src1] ;
        if(addr<obj.first || addr>obj.second) {
            if(PRINT_FAIL || DEBUG_HARD_BOUND) {
                DEBUG_OUT("addr out of range: 0x%llx  (0x%llx, 0x%llx)\n",
                        addr, obj.first, obj.second) ;
            }
            return 1 ;
        }
    } else if(reg_object_table.find(src2)!=reg_object_table.end()) { // src2 is the ptr then
        addr_range_t obj = reg_object_table[src2] ;
        if(addr<obj.first || addr>obj.second) {
            if(PRINT_FAIL || DEBUG_HARD_BOUND) {
                DEBUG_OUT("addr out of range: 0x%llx  (0x%llx, 0x%llx)\n",
                        addr, obj.first, obj.second) ;
            }
            return 1 ;
        }
    } else { // Sources are not ptrs
        if(!HARD_BOUND_WARMUP || isinWarmup()) {
            addr_range_t this_object = getObject(addr) ;
            if(this_object.first==0x0) { // in valid obj
                // Removing this from here for now because the caller does
                // the check, and is part of the basic hard_bound check
                // if(!priv && isHeapAddress(addr)) { // isHeapAccess(addr)) // FIXME - sHould this be heapAccess?
                //     if(PRINT_FAIL || DEBUG_HARD_BOUND) {
                //         DEBUG_OUT("0x%llx invalid heap address\n") ;
                //     }
                //     return 1 ;
                // }
            } else {
                if(!in_trap) {
                    reg_object_table[base] = this_object ;
                    if(DEBUG_HARD_BOUND) {
                        DEBUG_OUT("r%d pts to (0x%llx, 0x%llx)\n",
                                base, this_object.first, this_object.second) ;
                    }
                }
            }
        }
    }
    if(!in_trap) {
        if(reg_object_table.find(dest)==reg_object_table.end()) {
            if(mem_object_table.find(addr)!=mem_object_table.end()) {
                mem_object_table.erase(addr) ;
                if(DEBUG_HARD_BOUND) {
                    DEBUG_OUT("Removing 0x%llx\n", addr) ;
                }
            }
        } else {
            addr_range_t obj = reg_object_table[dest] ;
            if(addr!=0x0) {
                mem_object_table[addr] = obj ;
                if(DEBUG_HARD_BOUND) {
                    DEBUG_OUT("0x%llx pts to (0x%llx, 0x%llx)\n",
                            addr, obj.first, obj.second) ;
                }
            }
        }
    }
    return 0 ;
}

// If src1 or src1 point to objects, then dest points to that object.
// If both point to an object, dest has no object TODO - this loses opportunity
// If neither point to an object, then dest points to no object
int debugio_t::handleObjectPropagation(half_t dest, half_t src1, half_t src2, int priv, int in_trap)
{
    if(in_trap==1) {
        ERROR_OUT("handleObjectPropagation() called when in trap. Not possible\n") ;
        assert(0) ;
    }
    addr_range_t src1_obj, src2_obj ;
    addr_range_t dest_obj ;
    src1_obj.first = src1_obj.second = 0x0 ;
    src2_obj.first = src2_obj.second = 0x0 ;
    dest_obj.first = dest_obj.second = 0x0 ;
    if(reg_object_table.find(src1)!=reg_object_table.end()) {
        src1_obj = reg_object_table[src1] ;
    }
    if(reg_object_table.find(src2)!=reg_object_table.end()) {
        src2_obj = reg_object_table[src2] ;
    }
    if(src1_obj.first!=0x0 && src2_obj.first==0x0) {
        dest_obj = src1_obj ;
    } else if(src1_obj.first==0x0 && src2_obj.second!=0x0) {
        dest_obj = src2_obj ;
    } else if(src1_obj.first==0x0 && src2_obj.first==0x0) { // Then just leave dest obj empty
        
    } else { // Both have pointers to objects
        if(src1_obj.first==src2_obj.first && src1_obj.second==src2_obj.second) {
               // C lets you do arithmetic (sub) of two ptrs if they are to same obj
            // TODO - Should the dest also be an object? Are you sure?
            // dest_obj = src1_obj ;
        } else {
            if(dest!=PSEQ_FLATREG_NWIN) { // You can compare any two ptrs, dest'll be %g0
                // Can we have both sources with objects in this case??
                DEBUG_OUT("Both src1 and src2 pt to objects (0x%llx, 0x%llx), (0x%llx, 0x%llx\n",
                        src1_obj.first, src1_obj.second, src2_obj.first, src2_obj.second) ;
                return 1 ;
            }
        }
    }
    if(dest_obj.first == 0x0) { //Neither point to an object
        if(reg_object_table.find(dest)!=reg_object_table.end()) {
            reg_object_table.erase(dest) ;
            if(DEBUG_HARD_BOUND) {
                DEBUG_OUT("Removing %d\n", dest) ;
            }
        }
    } else {
        if(dest!=PSEQ_FLATREG_NWIN) { // "%g0" doesn't mean anything
            reg_object_table[dest] = dest_obj ;
            if(DEBUG_HARD_BOUND) {
                DEBUG_OUT("r%d pts to (0x%llx, 0x%llx)\n",
                        dest, dest_obj.first, dest_obj.second) ;
            }
        }
    }
    return 0 ;
}

// Which object is this address accessing?
debugio_t::addr_range_t debugio_t::getObject(ireg_t addr)
{
    addr_range_t obj ;
    obj.first = obj.second = 0x0 ;
    // if(prune_object_table.find(addr)!=prune_object_table.end()) {
    //     obj = prune_object_table[addr] ;
    // } else {
    //     for(prune_object_table_t::iterator I=prune_object_table.begin(),
    //             E=prune_object_table.end(); I!=E; I++) {
    //         ireg_t base = (*I).first ;
    //         addr_range_t this_obj = prune_object_table[base] ;
    //         if(addr>=this_obj.first && addr<=this_obj.second) {
    //             obj = this_obj ;
    //             break ;
    //         }
    //     }
    // }
    // Initial implemetnation was with vectors, now using maps
    for(object_table_t::iterator I=object_table.begin(), E=object_table.end() ;
            I!=E; I++) {
        addr_range_t this_obj = *I ;
        if(addr>=this_obj.first && addr<=this_obj.second) {
            obj = this_obj ;
            break ;
        }
    }
    if(obj.first!=0x0) {
        if(DEBUG_HARD_BOUND) {
            DEBUG_OUT("0x%llx is in (0x%llx, 0x%llx)\n", addr, obj.first, obj.second) ;
        }
    }
    return obj ;
}

// If this destination has a object ptr, check that the value (given)
// is within the bounds of the object 
bool debugio_t::isRegValueinObj(half_t dest, ireg_t value)
{
    if(reg_object_table.find(dest)!=reg_object_table.end()) {
        addr_range_t obj = reg_object_table[dest] ;
        if(value<obj.first || value>obj.second) {
            DEBUG_OUT("0x%llx not within object\n", value, obj.first, obj.second) ;
            return false ;
        }
    }
    return true ;
}

bool debugio_t::isMemValueinObj(ireg_t addr, ireg_t value)
{
    if(mem_object_table.find(addr)!=mem_object_table.end()) {
        addr_range_t obj = mem_object_table[addr] ;
        if(value<obj.first || value>obj.second) {
            DEBUG_OUT("0x%llx not within object\n", value, obj.first, obj.second) ;
            return false ;
        }
    }
    return true ;
}

void debugio_t::printRegObjectTable()
{
    DEBUG_OUT("### Register Objects Table ###\n") ;
    for(reg_object_table_t::iterator I=reg_object_table.begin(), E=reg_object_table.end();
            I!=E; I++) {
        half_t reg = (*I).first ;
        addr_range_t obj = reg_object_table[reg] ;
        DEBUG_OUT("r%d -> (0x%llx, 0x%llx)\n", reg, obj.first, obj.second) ;
    }
    DEBUG_OUT("### End Table ###\n") ;
}

void debugio_t::printMemObjectTable()
{
    DEBUG_OUT("### Memory Objects Table ###\n") ;
    for(mem_object_table_t::iterator I=mem_object_table.begin(), E=mem_object_table.end();
            I!=E; I++) {
        ireg_t mem = (*I).first ;
        addr_range_t obj = mem_object_table[mem] ;
        DEBUG_OUT("0x%llx -> (0x%llx, 0x%llx)\n", mem, obj.first, obj.second) ;
    }
    DEBUG_OUT("### End Table ###\n") ;
}

void debugio_t::clearForwardProgress()
{
    if(m_num_procs > 16) {
        out_info("WARNING:   Check this and fix the number of cores\n");
    }
    if(m_num_procs < 16 ) { 
        for(int i=0; i<m_num_procs; i++) {
            count_core[i] = 0;
        }
    }
}
    
void debugio_t::updateForwardProgress(int core_id, bool priv)
{
    if(priv) {
        count_core[core_id]++;
    } else {
        count_core[core_id] = 0;
    }
}

//Checks if all the cores are retiring os instructions from FORWARD_PROGRESS_THRESHOLD
//please check the size of count_cores[]
bool debugio_t::makingForwardProgress()
{
    //FIXME: remember, the size of count_core is 8 (hardcoded)
    if(m_num_procs > 16) {
        out_info("WARNING:   Check this and fix the number of cores\n");
    }
    if(m_num_procs < 16 ) { 
        int counter = 0;
        for(int i=0; i<m_num_procs; i++) {
            if(count_core[i] > FORWARD_PROGRESS_THRESHOLD)
                counter++;
            else
                break;
        }
        if(counter == m_num_procs)
            return false;
        else
            return true;
    } 
    else {
        out_info("I am from common/debugio.C: The size of the array count_core is hardcoded to 8\n");
        assert(0);
    }
}

//This is used for debug puropose
bool debugio_t::suspectForwardProgress()
{
    int counter = 0;
    for(int i=0; i<m_num_procs; i++) {
        if(count_core[i] > FORWARD_PROGRESS_THRESHOLD - 500)
            counter++;
        else
            break;
    }
    if(counter == m_num_procs)
        return true;
    else
        return false;
}



/**************************************************************************
Functions of class fault_stats
**************************************************************************/
fault_stats::fault_stats(unsigned int core_id)
{
 m_core_id = core_id;
 m_faulttype = -1 ;
 m_faultbit = -1 ;
 m_fault_src_dst = -1 ;
 m_faultstuckat = -1 ;
 m_faultyreg = -1 ;
 m_srcdest_reg = -1 ;
 m_start_logging_inst = -1 ;
 m_compare_point = -1 ;
 m_inf_loop_start = -1 ;
 m_fault_inj_inst = 0;
 m_fault_inj_pc = 0x0 ;
 strcpy(m_fault_inj_op, "") ;
 m_trans_injected = false ;
 m_trans_inj_cyc = 0 ;
 m_fault_inj_cycle = 0;

 do_fault_injection = true ;
 curr_cycle = 0 ;
 curr_inst = 0 ;
 ret_inst = 0 ;
 ret_priv = 0 ;
 priv = 0 ;
 in_func_trap = 0 ;
 non_ret_trap = 0 ;
 stat_non_ret_trap = 0 ;

 stat_app_arch_state_mismatch = 0 ;
 stat_app_first_arch_state_c = 0 ;
 stat_app_first_arch_state_i = 0 ;
 stat_os_arch_state_mismatch = 0 ;
 stat_os_first_arch_state_c = 0 ;
 stat_os_first_arch_state_i = 0 ;

 stat_app_arch_state_inuse_mismatch = 0 ;
 stat_app_first_arch_state_inuse_c = 0 ;
 stat_app_first_arch_state_inuse_i = 0 ;
 stat_os_arch_state_inuse_mismatch = 0 ;
 stat_os_first_arch_state_inuse_c = 0 ;
 stat_os_first_arch_state_inuse_i = 0 ;

 stat_app_mem_state_mismatch = 0 ;
 stat_app_first_mem_state_c = 0 ;
 stat_app_first_mem_state_i = 0 ;
 stat_os_mem_state_mismatch = 0 ;
 stat_os_first_mem_state_c = 0 ;
 stat_os_first_mem_state_i = 0 ;

 ctrl_corrupted = false ;
 stat_ctrl_corrupt_c = 0 ;
 stat_ctrl_corrupt_i = 0 ;

 stat_first_inject_c = 0;
 stat_first_inject_i = 0;

 stat_total_inject = 0 ;
 stat_total_mask = 0 ;
 stat_total_traps = 0 ;
 stat_fatal_traps = 0 ;
 stat_read_inject = 0 ;
 stat_read_mask = 0 ;

 stat_app_cycles = 0;
 stat_app_instr = 0;
 stat_os_cycles = 0;
 stat_os_instr = 0;
 last_switch_cycle = 0;
 last_switch_instr = 0;
 max_os_instrs = 0;
 os_start_instr = 0;
 cur_os_instrs = 0;
 num_loop_instr = 0;
 last_priv_mode = 0;
 last_priv_mode_set = 0;

 // Low level faults stats
 m_faulty_gate_id = -1 ;
 m_faulty_fanout_id = -1 ;

 store_branch_cnt = 0;

 detection_cycle = -1;
 detection_inst = -1;
 symptom = 0;
 inv_id = -1;
 symptom_pc = 0;
 symptom_priv = 0;

 stat_os_instr_to_detection = 0;
 stat_os_app_cross=0;
 stat_os_app_cross_inuse=0;

 fault_inst_list.clear() ;
 inst_targets.clear() ;
 mem_access.clear()  ;
 prefetch_table.clear() ;

 inst_mix.clear() ;
 faulty_inst_mix.clear() ;
 inst_mix["store"] = faulty_inst_mix["store"] = 0 ;
 inst_mix["load"] = faulty_inst_mix["load"] = 0 ;
 inst_mix["ctrl"] = faulty_inst_mix["ctrl"] = 0 ;
 inst_mix["other"] = faulty_inst_mix["other"] = 0 ;

 max_target = 0x0 ;
 min_target = 0xffffffffffffffff ;
/// declaration of system wide debug cycle
// m_global_cycle = 0;

 detector_list.clear() ;
 detector_range.clear() ;
 detector_values.clear() ;

 data_only_values.clear() ;
}

//**************************************************************************

fault_stats::~fault_stats()
{
}

void fault_stats::printFaultStats()
{

#ifdef FAULT_LOG
    FAULT_STAT_OUT( "\n#### FAULT STATS #####\n" ) ;
    FAULT_STAT_OUT( "\nCore Number:             %u\n", m_core_id ) ;
    FAULT_STAT_OUT( "Fault type:                %u\n", getFaultType());//GET_FAULT_TYPE() ) ;
#ifdef LL_INT_ALU || LL_AGEN
    char faultyNet[101];
    string test = getFaultyMachine();
    strcpy(faultyNet,test.c_str());
    FAULT_STAT_OUT( "Faulty net:                %s\n", faultyNet ) ;
#else 
    FAULT_STAT_OUT( "Fault bit:                 %u\n", getFaultBit() ) ;
#endif
    FAULT_STAT_OUT( "Fault stuck@:              %u\n", getFaultStuckat());//GET_FAULT_STUCKAT() ) ;
    FAULT_STAT_OUT( "Fault reg:                 %u\n", getFaultyReg());//GET_FAULTY_REG() ) ;
    FAULT_STAT_OUT( "Fault inj preset inst:     %u\n",  m_fault_inj_inst ) ;

//    FAULT_STAT_OUT("CPU NUMBER %d\n",m_pseq->getID());

    FAULT_STAT_OUT( "\n#### INJECTION INFO #####\n" ) ;
    FAULT_STAT_OUT( "First inj cycle:           %u\n", stat_first_inject_c ) ;
    FAULT_STAT_OUT( "First inj inst:            %u\n", stat_first_inject_i ) ;
    FAULT_STAT_OUT( "Non Retiring Trap:         %u\n", non_ret_trap) ;
    FAULT_STAT_OUT( "Fault inj PC:              0x%llx\n", m_fault_inj_pc ) ;
    FAULT_STAT_OUT( "Fault inj op:              %s\n", m_fault_inj_op ) ;

    FAULT_STAT_OUT( "\n#### TIME INFO #####\n" ) ;
    FAULT_STAT_OUT( "Cycles in app:             %llu\n", stat_app_cycles );
    FAULT_STAT_OUT( "Instructions in app:       %llu\n", stat_app_instr ) ;
    FAULT_STAT_OUT( "Cycles in os:              %llu\n", stat_os_cycles );
    FAULT_STAT_OUT( "Instructions in os:        %llu\n", stat_os_instr ) ;

    FAULT_STAT_OUT( "\n#### ARCH STATE INFO #####\n" ) ;
    FAULT_STAT_OUT( "Arch state mis(app):       %u\n",
            stat_app_arch_state_mismatch ) ; // FIXME - What is arch state? stat_app_arch_state_mismatch );
    FAULT_STAT_OUT( "Arch state mis cycle(app): %u\n",
            stat_app_first_arch_state_c ) ;  // FIXME - stat_app_first_arch_state_c ) ;
    FAULT_STAT_OUT( "Arch state mis inst(app):  %u\n",
            stat_app_first_arch_state_i ) ;
    FAULT_STAT_OUT( "Arch state mis(os):        %u\n",
            stat_os_arch_state_mismatch );
    FAULT_STAT_OUT( "Arch state mis cycle(os):  %u\n",
            stat_os_first_arch_state_c ) ;
    FAULT_STAT_OUT( "Arch state mis inst(os):   %u\n",
            stat_os_first_arch_state_i ) ;

    FAULT_STAT_OUT( "\n#### MEM STATE INFO #####\n" ) ;
    FAULT_STAT_OUT( "Mem state mis(app):       %u\n",
            stat_app_mem_state_mismatch );
    FAULT_STAT_OUT( "Mem state mis cycle(app): %u\n",
            stat_app_first_mem_state_c ) ;
    FAULT_STAT_OUT( "Mem state mis inst(app):  %u\n",
            stat_app_first_mem_state_i ) ;
    FAULT_STAT_OUT( "Mem state mis(os):        %u\n",
            stat_os_mem_state_mismatch );
    FAULT_STAT_OUT( "Mem state mis cycle(os):  %u\n",
            stat_os_first_mem_state_c ) ;
    FAULT_STAT_OUT( "Mem state mis inst(os):   %u\n",
            stat_os_first_mem_state_i ) ;
    // FAULT_STAT_OUT( "Ctrl state mis cycle:  %u\n",
    //         stat_ctrl_corrupt_c ) ;
    // FAULT_STAT_OUT( "Ctrl state mis inst:  %u\n",
    //         stat_ctrl_corrupt_i ) ;

    FAULT_STAT_OUT( "\n#### MISC INFO #####\n" ) ;
    FAULT_STAT_OUT( "Total bit-flips:           %u\n", stat_total_inject ) ;
    FAULT_STAT_OUT( "Total mask:                %u\n", stat_total_mask ) ;

 #ifdef COMPARE_TO_LL 
    FAULT_STAT_OUT( "Total bit-flips: (retired):           %u\n", fSim.numCorruptRetire);
    FAULT_STAT_OUT( "Total mask (retired):                %u\n", fSim.numMaskRetire ) ;
    FAULT_STAT_OUT( "Total injections (retired):                %u\n", fSim.numInvokeRetire ) ;
    // DEBUG
//    FAULT_STAT_OUT( "Total injections:                %u\n", fSim.counter ) ;
 #endif

    // FAULT_STAT_OUT( "Stores after arch mis:     %u\n", store_branch_cnt) ;
    FAULT_STAT_OUT( "Stores injected in LSQ:     %u\n", store_branch_cnt) ;
    FAULT_STAT_OUT( "Max OS instrs:             %u\n", max_os_instrs) ;
    if( (getFaultType()==REG_FILE_FAULT) || (getFaultType()==FP_REG_FAULT) || 
            (getFaultType()==REG_DATA_LINE_FAULT) ) {
        FAULT_STAT_OUT( "Read bit-flips:            %u\n", stat_read_inject ) ;
        FAULT_STAT_OUT( "Read  mask:                %u\n", stat_read_mask ) ;
    }
    FAULT_STAT_OUT( "Total traps:               %u\n", stat_total_traps ) ;

#ifdef MEASURE_FP
    FAULT_STAT_OUT( "FP ADD:    %u\t%u\t%u\t%u\n", system_t::inst->getFP(4,0),
                    system_t::inst->getFP(4,1),
                    system_t::inst->getFP(4,2),
                    system_t::inst->getFP(4,3)
                    ) ;
    FAULT_STAT_OUT( "FP MUL:    %u\t%u\n", system_t::inst->getFP(5,0),
                    system_t::inst->getFP(5,1)
                    ) ;
    FAULT_STAT_OUT( "FP DIV:    %u\t%u\n", system_t::inst->getFP(6,0),
                    system_t::inst->getFP(6,1)
                    ) ;
#endif
#endif

    FAULT_STAT_OUT("Detection Cycle : %lld\n",detection_cycle);
    FAULT_STAT_OUT("Detection Inst  : %lld\n",detection_inst);
    FAULT_STAT_OUT("Detected Symp   : 0x%x\n",symptom);
    FAULT_STAT_OUT("Violated Invariant Id   : %d\n",inv_id);
    FAULT_STAT_OUT("Symp PC (priv)  : 0x%llx (%d)\n",symptom_pc,symptom_priv);
    FAULT_STAT_OUT("OS Instr to Det : %lld\n",stat_os_instr_to_detection);
    FAULT_STAT_OUT("OS-App Crosses (in_use) : %lld (%lld)\n",stat_os_app_cross,stat_os_app_cross_inuse);


#ifdef PRINT_LL_INFO
    FAULT_STAT_OUT( "\n#### LL FLIPS INFO #####\n" ) ;

    FAULT_STAT_OUT("1 bit-flips:\t%d\n", fSim.getFlipCounters(1));
    FAULT_STAT_OUT("2 bit-flips:\t%d\n", fSim.getFlipCounters(2));
    FAULT_STAT_OUT("3 bit-flips:\t%d\n", fSim.getFlipCounters(3));
    FAULT_STAT_OUT("4 bit-flips:\t%d\n", fSim.getFlipCounters(4));
    FAULT_STAT_OUT("5 bit-flips:\t%d\n", fSim.getFlipCounters(5));
    FAULT_STAT_OUT("5B bit-flips:\t%d\n", fSim.getFlipCounters(6));
    FAULT_STAT_OUT("Bit Inv:\t0x%llx\n", fSim.getAnalysis(1));
    FAULT_STAT_OUT("Distance (min:max:avg):\t%lld:%lld:%lld\n", 
            fSim.getAnalysis(2),fSim.getAnalysis(3),fSim.getAnalysis(4));

    //     FAULT_STAT_OUT( "Total corruptions (retired):           %u\n", fSim.numCorruptRetire);
    //     FAULT_STAT_OUT("Multiple bit-flips observed (retired):\t%d\n", fSim.multiBitCorruptionRetire);
    //     FAULT_STAT_OUT( "Total mask (retired):                %u\n", fSim.numMaskRetire ) ;
    //     FAULT_STAT_OUT( "Total injections (retired):                %u\n", fSim.numInvokeRetire ) ;

    FAULT_STAT_OUT("1 bit-flips (retired):\t%d\n", fSim.getFlipCountersRetire(1));
    FAULT_STAT_OUT("2 bit-flips (retired):\t%d\n", fSim.getFlipCountersRetire(2));
    FAULT_STAT_OUT("3 bit-flips (retired):\t%d\n", fSim.getFlipCountersRetire(3));
    FAULT_STAT_OUT("4 bit-flips (retired):\t%d\n", fSim.getFlipCountersRetire(4));
    FAULT_STAT_OUT("5 bit-flips (retired):\t%d\n", fSim.getFlipCountersRetire(5));
    FAULT_STAT_OUT("5B bit-flips (retired):\t%d\n", fSim.getFlipCountersRetire(6));
    FAULT_STAT_OUT("Bit Inv (retired):\t0x%llx\n", fSim.getRetireAnalysis(1));
    FAULT_STAT_OUT("Distance min:max:avg (retired):\t%lld:%lld:%lld\n", 
    fSim.getRetireAnalysis(2),fSim.getRetireAnalysis(3),fSim.getRetireAnalysis(4));
    if( getFaultType() == INTALU_OUTPUT_FAULT) {
        FAULT_STAT_OUT("# instr using Bad ALU:\t%llu\n", fSim.useBadALU);
    } else if (getFaultType() == AGEN_FAULT) {
        FAULT_STAT_OUT("# instr using Bad AGEN:\t%llu\n", fSim.useBadALU);
    }

#if    LXL_COLLECT_PATTERN
    for (pattern_map_t::iterator I=fSim.getPatternList().begin(), E=fSim.getPatternList().end();
         I!=E; ++I) {
        const pattern_t &this_pattern = I->first;
        uint64 value = I->second;

        FAULT_STAT_OUT("P:%llx\t%llx\t%lld\n", this_pattern.mask, this_pattern.dir, value);
    }
#endif // LXL_COLLECT_PATTERN


//     FAULT_STAT_OUT("Logic-0:\t%d\n", fSim.logic_zero);
//     FAULT_STAT_OUT("Logic-1:\t%d\n", fSim.logic_one);
//     FAULT_STAT_OUT("Logic-0 (retired):\t%d\n", fSim.logic_zero_retire);
//     FAULT_STAT_OUT("Logic-1 (retired):\t%d\n", fSim.logic_one_retire);

//     FAULT_STAT_OUT("Num arith:\t%d\n", fSim.inst_arith_count);
//     FAULT_STAT_OUT("Num logic:\t%d\n", fSim.inst_logic_count);
//     //  ELEMENTS OF indexMeter:
//     FAULT_STAT_OUT("\nINDEXMETER:\n");
//     int i;
//     for(i=0; i<REG_WIDTH-1; i++){
//         FAULT_STAT_OUT("%d\n", fSim.indexMeter[i]);
//     }

//     //  ELEMENTS OF retired indexMeter:
//     FAULT_STAT_OUT("\nRETIRED INDEXMETER:\n");
//     for(i=0; i<REG_WIDTH-1; i++){
//         FAULT_STAT_OUT("%d\n", fSim.indexMeterRetire[i]);
//     }

#endif // PRINT_LL_INFO

    struct timeval tv;
    gettimeofday(&tv,NULL);
    uint64 finish_time = tv.tv_sec*1e6+tv.tv_usec;
    FAULT_STAT_OUT("Total Elapsed Time:\t%llu\n", finish_time-debugio_t::getStartTime());
    // DEBUG_OUT("start time %llu finish time %llu\n",debugio_t::getStartTime(),finish_time);
    // DEBUG_OUT("difference %llu\n",finish_time-debugio_t::getStartTime());

    // debugio_t::printCorruptAddress() ;
    // printFaultInstList() ;
    // printInstTargets() ;
    // printMemAccess() ;
    if(INST_MIX) {
        printInstMix() ;
    }

#ifdef FIND_MIN_STACK_ADDR
    DEBUG_OUT("Min stack address = 0x%llx\n", debugio_t::getMinTos()) ;
    DEBUG_OUT("Max stack address = 0x%llx\n", debugio_t::getMaxTos()) ;
    DEBUG_OUT("Min heap address = 0x%llx\n", debugio_t::getMinHeap()) ;
    DEBUG_OUT("Max heap address = 0x%llx\n", debugio_t::getMaxHeap()) ;
#endif // FIND_MIN_STACK_ADDR

#ifdef COLLECT_RANGE
    printInstRanges() ;
#endif // COLLECT_RANGE

#ifdef BUILD_OBJECT_TABLE
    printObjectTable() ;
#endif // BUILD_OBJECT_TABLE

#ifdef HARD_BOUND
    debugio_t::printRegObjectTable() ;
    debugio_t::printMemObjectTable() ;
#endif // HARD_BOUND
}

//--------------------- GATE LEVEL SAF SIM ----------------------------------------//
void debugio_t::printSafFaultStats()
{
#ifdef SAF_FAULT_LOG
    FAULT_STAT_OUT( "\n#### SAF FAULT STATS #####\n" ) ; 
    FAULT_STAT_OUT( "Fault type:                %u\n", GET_FAULT_TYPE() ) ;
    FAULT_STAT_OUT( "Faulty gate id:                 %u\n", GET_FAULTY_GATE() ) ;
    // zero corresponds to the stem, branches are enumerated starting from 1
    FAULT_STAT_OUT( "Faulty fanout id:                 %u\n", GET_FAULTY_FANOUT() ) ;
    FAULT_STAT_OUT( "Fault stuck@:              %u\n", GET_FAULT_STUCKAT() ) ;
    FAULT_STAT_OUT( "Fault inj preset inst:     %u\n",  m_fault_inj_inst ) ;

    // stat_first's indicate the cycle of fault injection and the PC(?) of the corresponding instruction...
    // the bug there does not seem to be fixed though, since the retiring instr is assigned somehow, and not the 
    // current instruction...
    // the assignment to these variables takes place in inject_fault routines...
    FAULT_STAT_OUT( "\n#### INJECTION INFO #####\n" ) ;
    FAULT_STAT_OUT( "First inj cycle:           %u\n", stat_first_inject_c ) ;
    FAULT_STAT_OUT( "First inj inst:            %u\n", stat_first_inject_i ) ;
    // TODO I did not know how to interpret this...
    FAULT_STAT_OUT( "Non Retiring Trap:         %u\n", non_ret_trap) ;

    //-------------------------------------- number of LLS invocations -------------------------------------------------//
    FAULT_STAT_OUT("LLS CALLS:\t%d\n", fSim.numInvoke+1+fSim.LUTReference);
    FAULT_STAT_OUT("CORRUPTIONS:\t%d\n", fSim.numCorrupt);
   
    // TODO to be figured out...
    
    FAULT_STAT_OUT( "\n#### TIME INFO #####\n" ) ;
    FAULT_STAT_OUT( "Cycles in app:             %llu\n", stat_app_cycles );
    FAULT_STAT_OUT( "Instructions in app:       %llu\n", stat_app_instr ) ;
    FAULT_STAT_OUT( "Cycles in os:              %llu\n", stat_os_cycles );
    FAULT_STAT_OUT( "Instructions in os:        %llu\n", stat_os_instr ) ;

    FAULT_STAT_OUT( "\n#### ARCH STATE INFO #####\n" ) ;
    // FAULT_STAT_OUT( "Arch state mis(app):       %u\n",
    //         stat_app_arch_state_inuse_mismatch ) ; // FIXME - What is arch state? stat_app_arch_state_mismatch );
    // FAULT_STAT_OUT( "Arch state mis cycle(app): %u\n",
    //         stat_app_first_arch_state_inuse_c ) ;  // FIXME - stat_app_first_arch_state_c ) ;
    // FAULT_STAT_OUT( "Arch state mis inst(app):  %u\n",
    //         stat_app_first_arch_state_inuse_i ) ;
    // FAULT_STAT_OUT( "Arch state mis(os):        %u\n",
    //         stat_os_arch_state_inuse_mismatch );
    // FAULT_STAT_OUT( "Arch state mis cycle(os):  %u\n",
    //         stat_os_first_arch_state_inuse_c ) ;
    // FAULT_STAT_OUT( "Arch state mis inst(os):   %u\n",
    //         stat_os_first_arch_state_inuse_i ) ;
    FAULT_STAT_OUT( "Arch state mis(app):       %u\n",
            stat_app_arch_state_mismatch );
    FAULT_STAT_OUT( "Arch state mis cycle(app): %u\n",
            stat_app_first_arch_state_c ) ;
    FAULT_STAT_OUT( "Arch state mis inst(app):  %u\n",
            stat_app_first_arch_state_i ) ;
    FAULT_STAT_OUT( "Arch state mis(os):        %u\n",
            stat_os_arch_state_mismatch );
    FAULT_STAT_OUT( "Arch state mis cycle(os):  %u\n",
            stat_os_first_arch_state_c ) ;
    FAULT_STAT_OUT( "Arch state mis inst(os):   %u\n",
            stat_os_first_arch_state_i ) ;

    FAULT_STAT_OUT( "\n#### MEM STATE INFO #####\n" ) ;
    FAULT_STAT_OUT( "Mem state mis(app):       %u\n",
            stat_app_mem_state_mismatch );
    FAULT_STAT_OUT( "Mem state mis cycle(app): %u\n",
            stat_app_first_mem_state_c ) ;
    FAULT_STAT_OUT( "Mem state mis inst(app):  %u\n",
            stat_app_first_mem_state_i ) ;
    FAULT_STAT_OUT( "Mem state mis(os):        %u\n",
            stat_os_mem_state_mismatch );
    FAULT_STAT_OUT( "Mem state mis cycle(os):  %u\n",
            stat_os_first_mem_state_c ) ;
    FAULT_STAT_OUT( "Mem state mis inst(os):   %u\n",
            stat_os_first_mem_state_i ) ;

    FAULT_STAT_OUT( "\n#### MISC INFO #####\n" ) ;
    FAULT_STAT_OUT( "Total bit-flips:           %u\n", stat_total_inject ) ;
    FAULT_STAT_OUT( "Total mask:                %u\n", stat_total_mask ) ;
    // FAULT_STAT_OUT( "Stores after arch mis:     %u\n", store_branch_cnt) ;
    FAULT_STAT_OUT( "Stores injected in LSQ:     %u\n", store_branch_cnt) ;
    FAULT_STAT_OUT( "Max OS instrs:             %u\n", max_os_instrs) ;
    FAULT_STAT_OUT( "Total traps:               %u\n", stat_total_traps ) ;
#endif
}
//--------------------- GATE LEVEL SAF SIM ----------------------------------------//


void fault_stats::setRetInst( uint64 i ) {
    ret_inst = i ;
    if( (TRANSIENT_MASK==1) && (m_faultstuckat==4)) {
        // NOTE: The markInjTransFault() function lets RAT and
        // REG fault to mark the first injection. 

        // If the injected transient fault didn't cause arch state
        // mismatch in TRANSIENT_MASK_THRESHOLD instructions,
        // it was probably masked!
        if (stat_first_inject_i > 0 ) { // If the injection did happen
            if( !getArchMismatch() &&
                ((ret_inst - stat_first_inject_i) > TRANSIENT_MASK_THRESHOLD) &&
                (m_faulttype != RAT_FAULT) ) {
                // NOTE: We make special case for RAT_FAULT because
                // the fault can be domant for a long time because the
                // RETIRE map is not corrupted. So a corrupted logical
                // reg can be unused for a long time, which does not
                // affect arch state because checks are based on
                // retire map not decode map.

#ifdef FAULT_LOG
                FAULT_OUT( "Transient_Masked\n" ) ;
#endif
		debugio_t::setMasked();
                HALT_SIMULATION ;
            }
        } else {

#ifndef ARCH_TRANS_FAULT
            if (getClock() > m_fault_inj_cycle) {
                // LXL: Please re-read the following comment. If the
                // current clock has passed the preset cycle, NO
                // transients will occur!!

                // if preset inj cycle has passed but no injection occurred, 
                // architecturally masked.
#ifdef FAULT_LOG
                FAULT_OUT( "Transient_Masked\n" ) ;
#endif
		debugio_t::setMasked();
                HALT_SIMULATION ;
            }
#endif // ARCH_TRANS_FAULT
        }
    }
}


// Main fault injector
byte_t fault_stats::injectFault
( byte_t value)
{
   if(m_core_id != debugio_t::getFaultyCore()) {
    return value;
   }

    byte_t old_value = value ;
    int bit = m_faultbit;
    int stuck_at = m_faultstuckat;
        
    if( GSPOT_DEBUG ) {
        DEBUG_OUT("fault_stats::injectFault\n" ) ;
    }

    if ((bit > (sizeof(byte_t)<<3)) ||
        (m_fault_inj_inst > ret_inst))
        return value;

    if( !do_fault_injection )
        return value ;

    switch( stuck_at ) {
    case 0 : // Stuck at 0
        {
            value = old_value & (~(1<<bit)) ;
        }
        break ;

    case 1 : // Stuck at 1
        {
            value = old_value | (stuck_at << bit ) ;
        }
        break ;

    case 2 : // Dominant-0 Bridging (B-Gnd)
        {
            byte_t bridge_bits = (old_value & (0x3 << bit)) >> bit;
            if (!((bridge_bits == 0) || (bridge_bits ==0x3))) {
                value = old_value & (~(0x3 << bit));
            }
        }
        break ;

    case 3 : // Dominant-1 Bridging (B-Vcc)
        {
            byte_t bridge_bits = (old_value & (0x3 << bit)) >> bit;
            if (!((bridge_bits == 0) || (bridge_bits ==0x3))) {
                value = old_value | (0x3 << bit);
            }
        }
        break ;
    case 4 : // Transient fault
        {
            // Inject the fault if the instruction count is reached
            // and the transient is not already injected

            // LXL: The instruction-based injection is incorrect as it
            // WILL increase the fault rate if ret_inst remains the
            // same for more than a few cycles
            //if( !getTransInjected() && (ret_inst == m_fault_inj_inst) ) 

            bool inject_me = true ;
#ifndef ARCH_TRANS_FAULT // Else, the check is done in system/pseq.C
            inject_me = (getClock() == m_fault_inj_cycle) ;
#endif // ARCH_TRANS_FAULT
            if( !getTransInjected() && inject_me ) {
                if( !FIXED_TRANSIENT ) {
                    srand(clock()) ;
                    float r = (float)rand() / RAND_MAX ;
                    bit = (unsigned)(r * sizeof(byte_t) * 8) ; 
                }
                setFaultBit(bit) ;
                value = old_value ^ (1 << bit ) ;
                if( PRADEEP_DEBUG_TRANSIENT ) {
                    DEBUG_OUT( "Ret = %d\n", ret_inst ) ;
                    DEBUG_OUT( "Bit = %d\n", bit ) ;
                    DEBUG_OUT( "Old = %x\n", old_value ) ;
                    DEBUG_OUT( "New = %x\n", value ) ;
                }
#ifdef ARCH_REGISTER_INJECTION 
		// for AGEN faults there is a special case we want to inject it
		// in every occurance of the target instruction. This should
		// (is) taken care of at the AGEN injection point in memop.C
                if (getFaultType() != AGEN_FAULT) 
	                setTransInjected() ;
#else 
                setTransInjected() ;
#endif
            }
        }
        break ;
    default : // Default case
        DEBUG_OUT( "Fault model %d not defined\n", stuck_at ) ;
        HALT_SIMULATION ;
    }

    if( GSPOT_DEBUG ) {
        DEBUG_OUT("Byte_t\t") ;
        DEBUG_OUT("Stuck at = %u\t", stuck_at ) ;
        DEBUG_OUT("old value = %llx\t", old_value ) ;
        DEBUG_OUT("new value = %llx\n", value ) ;
    }

    if( value != old_value ) {
        if( OUTPUT_WHEN_DIFF ) {
            DEBUG_OUT("Byte_t\t") ;
            DEBUG_OUT("Stuck at = %u\t", stuck_at ) ;
            DEBUG_OUT("old value = %llx\t", old_value ) ;
            DEBUG_OUT("new value = %llx\n", value ) ;
        }
#ifdef FAULT_LOG
        if(!stat_first_inject_c) {
            stat_first_inject_c = getClock() ;
            // FIXME - Has to be curr inst
            stat_first_inject_i = getRetInst() ;        
        }
        incTotalInj/*INC_TOTAL_INJ*/() ;
    } else {
        incTotalMask/*INC_TOTAL_MASK*/() ;
#endif
    }
    return value ;
}

half_t fault_stats::injectFault( half_t value)
{
   if(m_core_id != debugio_t::getFaultyCore()) {
       return value;
   }
    half_t old_value = value ;
    int bit = m_faultbit;
    int stuck_at = m_faultstuckat;


    if( GSPOT_DEBUG ) {
        DEBUG_OUT("fault_stats::injectFault\n" ) ;
    }

    if ((bit > (sizeof(half_t)<<3)) ||
        (m_fault_inj_inst > ret_inst))
        return value;

    if( !do_fault_injection )
        return value ;

    switch( stuck_at ) {
    case 0 : // Stuck at 0
        {
            value = old_value & (~(1<<bit)) ;
        }
        break ;

    case 1 : // Stuck at 1
        {
            value = old_value | (stuck_at << bit ) ;
        }
        break ;

    case 2 : // Dominant-0 Bridging (B-Gnd)
        {
            half_t bridge_bits = (old_value & (0x3 << bit)) >> bit;
            if (!((bridge_bits == 0) || (bridge_bits ==0x3))) {
                value = old_value & (~(0x3 << bit));
            }
        }
        break ;

    case 3 : // Dominant-1 Bridging (B-Vcc)
        {
            half_t bridge_bits = (old_value & (0x3 << bit)) >> bit;
            if (!((bridge_bits == 0) || (bridge_bits ==0x3))) {
                value = old_value | (0x3 << bit);
            }
        }
        break ;
    case 4 : // Transient fault
        {
            // Inject the fault if the instruction count is reached
            // and the transient is not already injected

            // LXL: The instruction-based injection is incorrect as it
            // WILL increase the fault rate if ret_inst remains the
            // same for more than a few cycles
            bool inject_me = true ;
#ifndef ARCH_TRANS_FAULT // Else, the check is done in system/pseq.C
            inject_me = (getClock() == m_fault_inj_cycle) ;
#endif // ARCH_TRANS_FAULT
            if( !getTransInjected() && inject_me ) {
                if( !FIXED_TRANSIENT ) {
                    srand(clock()) ;
                    float r = (float)rand() / RAND_MAX ;
                    bit = (unsigned)(r * sizeof(byte_t) * 8) ; 
                }
                setFaultBit(bit) ;
                value = old_value ^ (1 << bit ) ;
                //if( PRADEEP_DEBUG_TRANSIENT ) {
                    DEBUG_OUT( "Ret = %d\n", ret_inst ) ;
                    DEBUG_OUT( "Bit = %d\n", bit ) ;
                    DEBUG_OUT( "Old = %x\n", old_value ) ;
                    DEBUG_OUT( "New = %x\n", value ) ;
                //}
#ifdef ARCH_REGISTER_INJECTION 
		// for AGEN faults there is a special case we want to inject it
		// in every occurance of the target instruction. This should
		// (is) taken care of at the AGEN injection point in memop.C
                if (getFaultType() != AGEN_FAULT) 
	                setTransInjected() ;
#else 
                setTransInjected() ;
#endif
            }
        }
        break ;
    default : // Default case
        DEBUG_OUT( "Fault model %d not defined\n", stuck_at ) ;
        HALT_SIMULATION ;
    }

    if( GSPOT_DEBUG ) {
        DEBUG_OUT("half_t\t") ;
        DEBUG_OUT("Stuck at = %u\t", stuck_at ) ;
        DEBUG_OUT("old value = %x\t", old_value ) ;
        DEBUG_OUT("new value = %x\n", value ) ;
    }

    if( value != old_value ) {
        if( OUTPUT_WHEN_DIFF ) {
            DEBUG_OUT("half_t\t") ;
            DEBUG_OUT("Stuck at = %u\t", stuck_at ) ;
            DEBUG_OUT("old value = %x\t", old_value ) ;
            DEBUG_OUT("new value = %x\n", value ) ;
        }
#ifdef FAULT_LOG
        if(!stat_first_inject_c) {
            stat_first_inject_c = getClock() ;
            stat_first_inject_i = getRetInst() ; // FIXME - Has to be curr inst
        }
        incTotalInj/*INC_TOTAL_INJ*/() ;
    } else {
        incTotalMask/*INC_TOTAL_MASK*/() ;
#endif
    }
    return value ;
}

uint32 fault_stats::injectFault( uint32 value)
{
   if(m_core_id != debugio_t::getFaultyCore()) {
    return value;
   }
    uint32 old_value = value ;
    int bit = m_faultbit;
    int stuck_at = m_faultstuckat;


    if( GSPOT_DEBUG ) {
        DEBUG_OUT("fault_stats::injectFault\n" ) ;
    }

    if ((bit > (sizeof(uint32)<<3)) ||
        (m_fault_inj_inst > ret_inst))
        return value;

    if( !do_fault_injection )
        return value ;

    switch( stuck_at ) {
    case 0 : // Stuck at 0
        {
            value = old_value & (~(1<<bit)) ;
        }
        break ;

    case 1 : // Stuck at 1
        {
            value = old_value | (stuck_at << bit ) ;
        }
        break ;

    case 2 : // Dominant-0 Bridging (B-Gnd)
        {
            uint32_t bridge_bits = (old_value & (0x3 << bit)) >> bit;
            if (!((bridge_bits == 0) || (bridge_bits ==0x3))) {
                value = old_value & (~(0x3 << bit));
            }
        }
        break ;

    case 3 : // Dominant-1 Bridging (B-Vcc)
        {
            uint32_t bridge_bits = (old_value & (0x3 << bit)) >> bit;
            if (!((bridge_bits == 0) || (bridge_bits ==0x3))) {
                value = old_value | (0x3 << bit);
            }
        }
        break ;
    case 4 : // Transient fault
        {
            // Inject the fault if the instruction count is reached
            // and the transient is not already injected

            // LXL: The instruction-based injection is incorrect as it
            // WILL increase the fault rate if ret_inst remains the
            // same for more than a few cycles
            //if( !getTransInjected() && (ret_inst == m_fault_inj_inst) ) 

            bool inject_me = true ;
#ifndef ARCH_TRANS_FAULT // Else, the check is done in system/pseq.C
            inject_me = (getClock() == m_fault_inj_cycle) ;
#endif // ARCH_TRANS_FAULT
            if( !getTransInjected() && inject_me ) {

                if( !FIXED_TRANSIENT ) {
                    srand(clock()) ;
                    float r = (float)rand() / RAND_MAX ;
                    bit = (unsigned)(r * sizeof(byte_t) * 8) ; 
                }
                setFaultBit(bit) ;
                value = old_value ^ (1 << bit ) ;
                if( PRADEEP_DEBUG_TRANSIENT ) {
                    DEBUG_OUT( "Ret = %d\n", ret_inst ) ;
                    DEBUG_OUT( "Bit = %d\n", bit ) ;
                    DEBUG_OUT( "Old = %x\n", old_value ) ;
                    DEBUG_OUT( "New = %x\n", value ) ;
                }
#ifdef ARCH_REGISTER_INJECTION 
		// for AGEN faults there is a special case we want to inject it
		// in every occurance of the target instruction. This should
		// (is) taken care of at the AGEN injection point in memop.C
                if (getFaultType() != AGEN_FAULT) 
	                setTransInjected() ;
#else 
                setTransInjected() ;
#endif
            }
        }
        break ;
    default : // Default case
        DEBUG_OUT( "Fault model %d not defined\n", stuck_at ) ;
        HALT_SIMULATION ;
    }

    if( GSPOT_DEBUG ) {
        DEBUG_OUT("uint32\t" ) ;
        DEBUG_OUT("Stuck at = %u\t", stuck_at ) ;
        DEBUG_OUT("old value = %llx\t", old_value ) ;
        DEBUG_OUT("new value = %llx\n", value ) ;
    }

    if( value != old_value ) {
        if (OUTPUT_WHEN_DIFF) {
            DEBUG_OUT("uint32\t" ) ;
            DEBUG_OUT("Stuck at = %u\t", stuck_at ) ;
            DEBUG_OUT("old value = %llx\t", old_value ) ;
            DEBUG_OUT("new value = %llx\n", value ) ;
        }
#ifdef FAULT_LOG
        if(!stat_first_inject_c) {
            stat_first_inject_c = getClock() ;
            // FIXME - Has to be curr inst
            stat_first_inject_i = getRetInst() ;        
        }
        incTotalInj/*INC_TOTAL_INJ*/() ;
    } else {
        incTotalMask/*INC_TOTAL_MASK*/() ;
#endif
    }
    return value ;
}

uint64 fault_stats::injectFault(uint64 value)
{
   if(m_core_id != debugio_t::getFaultyCore()) {
    return value;
   }
    uint64 old_value = value ;
    int bit = m_faultbit;
    int stuck_at = m_faultstuckat;


    if( GSPOT_DEBUG ) {
        DEBUG_OUT("fault_stats::injectFault\n" ) ;
    }

    //if ((bit > (sizeof(uint64)<<3)) ||
    //    (m_fault_inj_inst > ret_inst)) {
    //    return value;
    //}
    if (bit > (sizeof(uint64)<<3)) 
        return value;

    if( !do_fault_injection ) {
        return value ;
    }

    switch( stuck_at ) {
    case 0 : // Stuck at 0
        {
            value = old_value & (~(uint64(1)<<bit)) ;
        }
        break ;

    case 1 : // Stuck at 1
        {
            value = old_value | (uint64(stuck_at) << bit ) ;
        }
        break ;

    case 2 : // Dominant-0 Bridging (B-Gnd)
        {
            uint64_t bridge_bits = (old_value & (0x3 << bit)) >> bit;
            if (!((bridge_bits == 0) || (bridge_bits ==0x3))) {
                value = old_value & (~(0x3 << bit));
            }
        }
        break ;

    case 3 : // Dominant-1 Bridging (B-Vcc)
        {
            uint64_t bridge_bits = (old_value & (0x3 << bit)) >> bit;
            if (!((bridge_bits == 0) || (bridge_bits ==0x3))) {
                value = old_value | (0x3 << bit);
            }
        }
        break ;
    case 4 : // Transient fault
        {
            // Inject the fault if the instruction count is reached
            // and the transient is not already injected

            // LXL: The instruction-based injection is incorrect as it
            // WILL increase the fault rate if ret_inst remains the
            // same for more than a few cycles
            //if( !getTransInjected() && (ret_inst == m_fault_inj_inst) ) 

            bool inject_me = true ;
#ifndef ARCH_TRANS_FAULT // Else, the check is done in system/pseq.C
            inject_me = (getClock() == m_fault_inj_cycle) ;
#endif // ARCH_TRANS_FAULT
            if( !getTransInjected() && inject_me ) {
                if( !FIXED_TRANSIENT ) {
                    srand(clock()) ;
                    float r = (float)rand() / RAND_MAX ;
                    bit = (unsigned)(r * sizeof(byte_t) * 8) ; 
                }
                setFaultBit(bit) ;
                value = old_value ^ (((uint64)1) << bit ) ;
                if( PRADEEP_DEBUG_TRANSIENT ) {
                    DEBUG_OUT( "Ret = %d\n", ret_inst ) ;
                    DEBUG_OUT( "Bit = %d\n", bit ) ;
                    DEBUG_OUT( "Old = %llx\n", old_value ) ;
                    DEBUG_OUT( "New = %llx\n", value ) ;
                }
#ifdef ARCH_REGISTER_INJECTION 
		// for AGEN faults there is a special case we want to inject it
		// in every occurance of the target instruction. This should
		// (is) taken care of at the AGEN injection point in memop.C
                if (getFaultType() != AGEN_FAULT) 
	                setTransInjected() ;
#else 
                setTransInjected() ;
#endif
            }
        }
        break ;
    default : // Default case
        DEBUG_OUT( "Fault model %d not defined\n", stuck_at ) ;
        HALT_SIMULATION ;
    }
        
    if( GSPOT_DEBUG ) {
        DEBUG_OUT("uint64\t" ) ;
        DEBUG_OUT("Stuck at = %u\t", stuck_at ) ;
        DEBUG_OUT("old value = %llx\t", old_value ) ;
        DEBUG_OUT("new value = %llx\n", value ) ;
    }

    if( value != old_value ) {
        if( OUTPUT_WHEN_DIFF ) {
            DEBUG_OUT("uint64\t" ) ;
            DEBUG_OUT("Stuck at = %u\t", stuck_at ) ;
            DEBUG_OUT("old value = %llx\t", old_value ) ;
            DEBUG_OUT("new value = %llx\n", value ) ;
        }
#ifdef FAULT_LOG
        if(!stat_first_inject_c) {
            stat_first_inject_c = getClock() ;
            stat_first_inject_i = getRetInst() ;        
        }
        incTotalInj/*INC_TOTAL_INJ*/() ;
    } else {
        incTotalMask/*INC_TOTAL_MASK*/() ;
#endif
    }
    return value ;
}


void fault_stats::trackCycle(uint64 cur_cycle, uint64 seq_num, bool priv, bool isIdleLoopInstr )
{
    if (!last_priv_mode_set) {
        last_priv_mode = priv;
        last_priv_mode_set = true;
    } else {
        if (priv != last_priv_mode) {        // switch from os-to-app or app-to-os
            if (priv && !last_priv_mode) {    //if priv mode is 1
                // switch from app to os
                // DEBUG_OUT("%d\t", seq_num) ;
                os_start_instr = seq_num;
                stat_app_cycles += (cur_cycle - last_switch_cycle);
                stat_app_instr += (seq_num - last_switch_instr);
            } else if (!priv && last_priv_mode) {    //if priv mode is 0
                // switch from os to app
                uint64 os_cycles = cur_cycle - last_switch_cycle;
                uint64 os_instrs = seq_num - last_switch_instr;

                stat_os_cycles += os_cycles;
                stat_os_instr += os_instrs;
                cur_os_instrs = seq_num - os_start_instr;
                if (cur_os_instrs > max_os_instrs) {
                    max_os_instrs=cur_os_instrs;
                    // FIXME This should really be there - otherwise, as soon as arch mismatch happens,
                    // if OS count crosses, we are toast!
                    // if(!getArchMismatch()) {
                    //     max_os_instrs = 0 ;
                    // }
                }
                // DEBUG_OUT("%d\n", cur_os_instrs) ;
            } else {
                assert(0&&"priv and last_priv same unexpected!!\n");
            }

            if (stat_os_arch_state_mismatch) {
                stat_os_app_cross++;
                //DEBUG_OUT("switch %d %d\n",last_priv_mode,priv);
            }

            if (stat_os_arch_state_inuse_mismatch) {
                stat_os_app_cross_inuse++;
                //DEBUG_OUT("iu_switch %d %d\n",last_priv_mode,priv);
            }

            last_switch_cycle = cur_cycle;
            last_switch_instr = seq_num;
            last_priv_mode = priv;


        } else {            // no switch
            if(isIdleLoopInstr && priv)
                os_start_instr = seq_num;

            //if ((cur_cycle - last_switch_cycle) > 1000) {
            if (priv) {        // if os instr
                stat_os_cycles += (cur_cycle - last_switch_cycle);
                stat_os_instr += (seq_num - last_switch_instr);
                cur_os_instrs = (seq_num - os_start_instr);
                //debugio_t::out_info("-- %d\n", cur_os_instrs);
            } else {
                stat_app_cycles += (cur_cycle - last_switch_cycle);
                stat_app_instr += (seq_num - last_switch_instr);
            }

            last_switch_cycle = cur_cycle;
            last_switch_instr = seq_num;
            last_priv_mode = priv;
            //}
        }
    }

    debugio_t::updateForwardProgress(m_core_id, priv);

    //global
    if (debugio_t::hasStateMismatch() && priv) {
        stat_os_instr_to_detection++;
    }
    if (!debugio_t::hasStateMismatch() && m_core_id != debugio_t::getFaultyCore()) {
        stat_first_inject_i = seq_num;
    }
}

#define NEW_TRAP_OUTPUT


// void fault_stats::outTrapInfo(uint64 cyc, unsigned int inst, const char* trap_name, 
//                             unsigned int tt, la_t tpc, int priv, la_t thpc, int tl)
// {
// #ifdef NEW_TRAP_OUTPUT
//     FAULT_OUT( "%u:", cyc ) ;
//     FAULT_OUT( "%u:", inst ) ;
//     FAULT_OUT( "0x%x:", tt ) ;
//     FAULT_OUT( "0x%llx:", tpc ) ;
//     FAULT_OUT( "%d\n", priv ) ;
// #else
//     FAULT_OUT( "[%u]\t", cyc ) ;
//     FAULT_OUT( "[%u]\t", inst ) ;
//     FAULT_OUT( "\t%s", trap_name ) ;
//     FAULT_OUT( "(0x%x) ", tt ) ;
//     FAULT_OUT( "pc:0x%x", tpc ) ;
//     FAULT_OUT( "(%d)\t", priv ) ;
//     FAULT_OUT( "\ttrappc:0x%x", thpc ) ;
//     FAULT_OUT( "(%u)\n", tl ) ;
// #endif
//     incTotalTraps();
// }

void fault_stats::outSymptomInfo(uint64 cyc, uint64 inst, const char* trap_name, 
    unsigned int tt, la_t tpc, int priv, la_t thpc, int tl,int coreId)
{

//     FAULT_OUT( "coreId:%d:", coreId ) ;
// #ifdef NEW_TRAP_OUTPUT
//     FAULT_OUT( "%u:", cyc ) ;
//     FAULT_OUT( "%u:", inst ) ;
//     FAULT_OUT( "0x%x:", tt ) ;
//     FAULT_OUT( "0x%x:", tpc ) ;
//     FAULT_OUT( "%d\n", priv ) ;
// #else
//     FAULT_OUT( "[%u]\t", cyc ) ;
//     FAULT_OUT( "[%u]\t", inst ) ;
//     FAULT_OUT( "\t%s", trap_name ) ;
//     FAULT_OUT( "(0x%x) ", tt ) ;
//     FAULT_OUT( "pc:0x%x", tpc ) ;
//     FAULT_OUT( "(%d)\t", priv ) ;
//     FAULT_OUT( "\ttrappc:0x%x", thpc ) ;
//     FAULT_OUT( "(%u)\n", tl ) ;
// #endif
    detection_cycle = cyc;
    detection_inst = inst;
    symptom = tt;
        inv_id = -1;
    symptom_pc = tpc;
    symptom_priv = priv;

    //incTotalTraps();
    
}

void fault_stats::outSymptomInfo(uint64 cyc, uint64 inst, const char* trap_name, 
    unsigned int tt, la_t tpc, int priv, la_t thpc, int tl,int coreId, int InvId)
{
    detection_cycle = cyc;
    detection_inst = inst;
    symptom = tt;
        inv_id = InvId;
    symptom_pc = tpc;
    symptom_priv = priv;
}

void fault_stats::markInjTransFault()
{
    if(!stat_first_inject_c) {
        stat_first_inject_c = getClock() ;
        // FIXME - Has to be curr inst
        stat_first_inject_i = getRetInst() ;        
    }
    // incTotalInj/*INC_TOTAL_INJ*/() ;
}


/*-----------------------------------------------------------------------
 multi_core_diagnosis_t functions
-----------------------------------------------------------------------*/
/*
void multi_core_diagnosis_t::startMulticoreDiagnosis()
{
    diagnosis_started = true;
    triggerLoggingStep();
}

void multi_core_diagnosis_t::triggerLoggingStep()
{
    setLoggingStep();
    unsetTMRStep();
    DEBUG_OUT("Logging Step triggered\n");
    for(int i=0; i<NUM_CORES; i++) {
        trigger_logging_step[i] = true;    
    }
}

void multi_core_diagnosis_t::triggerTMRStep()
{
    setTMRStep();
    unsetLoggingStep();
    DEBUG_OUT("TMR Step ended\n");
    for(int i=0; i<NUM_CORES; i++) {
        trigger_tmr_step[i] = true;    
    }
}
*/



/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/


/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************

