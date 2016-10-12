#ifndef _SACN_H_
  #define _SCAN_H_
// +
// Scan definitions
//
// iz@elsis.si
// 2003.02.21
//
// -
  #define DPLL
  #define DPLLV2    // Balkar za enkrat
  #define TRIAC_CLOSURE (unsigned char)240
  #define TIMER2CNT (unsigned char)(77)

  #ifdef DPLLV1     // for normal use
    #undef SINGLEEDGEDET    // faz detektor tKO dela le vsako periodo !!!
    #define ZTF 2 // number of samples averaging (Low pass transfer function)
    #define ZLSHIFT 1  //shift count for average divider
    #define DPLLCONST 10u // 0.05% of 20000
    #define DPLLINITVAL 900u  // DPLL offset position
    #define PHASEDELAY 245u   // phase delay after DPLL int
  #endif

  #ifdef DPLLV2     // for BALKAR and other fluo
    #define SINGLEEDGEDET    // faz detektor tKO dela le vsako periodo !!!
    #define ZTF 4 // number of samples averaging (Low pass transfer function)
    #define ZLSHIFT 2  //shift count for average divider
    #define DPLLCONST 1u  // DPLL inc/dec constant
    #define DPLLINITVAL 900u  // DPLL offset position
    #define PHASEDELAY 245u   // phase delay after DPLL int
  #endif

// output data defininition


typedef struct bv_ {
  unsigned char b0, b1, b2, b3, b4, b5, b6, b7, b8;
}
bV;



typedef union outdatat_ {
  unsigned char ov[8];
  bV bv;
}
OutDatat;

extern volatile OutDatat OutDat;
extern unsigned char dirMask;
extern volatile OutDatat OutVoltage;
extern volatile OutDatat OutCurrent;
extern unsigned char dpllEnable;
void scanInit();

#endif
