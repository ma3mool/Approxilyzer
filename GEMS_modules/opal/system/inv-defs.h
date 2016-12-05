#ifndef _INV-DEF_H_
#define _INV-DEF_H_

#define MAX_PROGRAM_POINTS 30000
#define MAX_INVARIANTS_PER_PROGRAM_POINT 1
#define MAX_CALL_DEPTH 10000
#define MAX_NUM_RANGES 5

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

/* Change all long long to int64 etc. */
struct Invariant {
  union {
    long long longMin;
    unsigned long long ulongMin;
    double doubleMin;
  }Min;
  union {
    long long longMax;
    unsigned long long ulongMax;
    double doubleMax;
  }Max;
  union {
    long long longChange;
    unsigned long long ulongChange;
    double doubleChange;    
  }Change;
  long long NoOfUpdation;
  long long Count;  
  char Type[10];
  char DetailedType[10];
  char InstName[10];
  int ProgramPtId; // later add variables, operations, position/pgm point etc.
  char *Fname;
};


struct MulRangeInvariant {
  union {
    long long longMin[MAX_NUM_RANGES];
    unsigned long long ulongMin[MAX_NUM_RANGES];
    double doubleMin[MAX_NUM_RANGES];
  }Min;
  union {
    long long longMax[MAX_NUM_RANGES];
    unsigned long long ulongMax[MAX_NUM_RANGES];
    double doubleMax[MAX_NUM_RANGES];
  }Max;
  union {
    long long longChange;
    unsigned long long ulongChange;
    double doubleChange;    
  }Change;
  int occupied[MAX_NUM_RANGES] ;
  int num_ranges;
  long long NoOfUpdation;
  long long Count;  
  char Type[10];
  char DetailedType[10];
  char InstName[10];
  int ProgramPtId; // later add variables, operations, position/pgm point etc.
  char *Fname;
};


/* struct Invariant *Map[MAX_PROGRAM_POINTS][MAX_INVARIANTS_PER_PROGRAM_POINT] = { NULL } ;
int MapIndex[MAX_PROGRAM_POINTS] = { 0 } ;
*/
typedef map<int,Invariant*> InvMap;  
typedef map<int,MulRangeInvariant*> MulRangeInvMap;  

struct SVAAddr {
  char appl_name[100];
  uint64 start;
  uint64 end;
};

typedef map<string, SVAAddr*> SVAAddrMap;  

typedef union
{
  long long longValue;
  unsigned long long ulongValue;
  double    doubleValue;
}StoreValueArray;


#endif  /* _INV-DEF_H_ */

