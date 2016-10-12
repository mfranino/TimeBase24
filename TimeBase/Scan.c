// +
// Input output SCAN
//
// iz@elsis.si
// 2003.02.21
//
// -

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <string.h>

#include "Scan.h"


// Digital output data
volatile OutDatat OutDat;
unsigned char dirMask = 0x00;
static unsigned char cTime = 0;

// Dpll data
static unsigned char dpllInit=0; //dpll init flag
unsigned char dpllEnable=0x10;   //dpll enable flag
static unsigned int dpllval=20000u;

// A/D converter data
volatile OutDatat OutVoltage, OutCurrent;

unsigned char tmr2Start=0, phDetCnt=0;

//+
// actual zero cross routine
//-
void inline phaseDetInt()
{
    cbi( TIMSK, OCIE2 ); // disable interrupts
    TCCR2 = 0x08; // CTC mode, stop
    TCNT2 = 0; // reset
    sbi( SFIOR, PSR2 ); // and reset prescaler
    TCCR2 = 0x0a; // CTC mode, /8 prescaler =>run
    sbi( TIMSK, OCIE2 ); // enable interrupts
    //
    cTime = 0;
}

//+
// Digital output timer
//-

SIGNAL( SIG_OUTPUT_COMPARE2 ) {
    sbi( PORTB, 1 ); // diagnostic pin
    //
    //
    if (tmr2Start) {
        phaseDetInt();
        OCR2 = TIMER2CNT;       // output compare constant
        tmr2Start=0;
        cbi( PORTB, 1);
        return;
    }
    if ( cTime > TRIAC_CLOSURE ) { // cut off at the end if not direkt!!!
        cbi( TIMSK, OCIE2 ); // disable interrupts
        TCCR2 = 0x08; // CTC mode, stop
        asm volatile(
            "push r24\n""lds r24,dirMask\n"
            "sbrs r24,0\n""cbi 0x15,5\n" // portc
            "sbrs r24,1\n""cbi 0x12,0\n" // portd ...
            "sbrs r24,2\n""cbi 0x12,1\n"
            "sbrs r24,3\n""cbi 0x12,3\n"
            "sbrs r24,4\n""cbi 0x12,4\n"
            "sbrs r24,5\n""cbi 0x12,5\n"
            "sbrs r24,6\n""cbi 0x12,6\n"
            "sbrs r24,7\n""cbi 0x12,7\n"
            "pop r24\n" );
        cbi( PORTB, 1 );
        return;
    }
    //
    cTime++;
    if ( cTime > OutDat.bv.b0 ) sbi( PORTC, 5 );
    else cbi( PORTC, 5 );
    if ( cTime > OutDat.bv.b1 ) sbi( PORTD, 0 );
    else cbi( PORTD, 0 );
    if ( cTime > OutDat.bv.b2 ) sbi( PORTD, 1 );
    else cbi( PORTD, 1 );
    if ( cTime > OutDat.bv.b3 ) sbi( PORTD, 3 );
    else cbi( PORTD, 3 );
    if ( cTime > OutDat.bv.b4 ) sbi( PORTD, 4 );
    else cbi( PORTD, 4 );
    if ( cTime > OutDat.bv.b5 ) sbi( PORTD, 5 );
    else cbi( PORTD, 5 );
    if ( cTime > OutDat.bv.b6 ) sbi( PORTD, 6 );
    else cbi( PORTD, 6 );
    if ( cTime > OutDat.bv.b7 ) sbi( PORTD, 7 );
    else cbi( PORTD, 7 );
    //
    cbi( PORTB, 1 ); // diagnostic pin
}


//+
// Dpll interrupt
//-

#ifdef DPLL
SIGNAL( SIG_OUTPUT_COMPARE1A ) {
    if (dpllEnable) {
        PORTC^=8;       // toggle PORT C 3
        cbi( TIMSK, OCIE2 ); // disable interrupts
        TCCR2 = 0x08; // CTC mode, stop
        TCNT2 = 0; // reset
        OCR2=PHASEDELAY;
        sbi( SFIOR, PSR2 ); // and reset prescaler
        TCCR2 = 0x0b; // CTC mode, /32 prescaler =>run
        sbi( TIMSK, OCIE2 ); // enable interrupts
        tmr2Start=1;
        if (phDetCnt>2) {  // is phase detect interrupt comming ?
            phDetCnt=0;
            dpllInit=0;
        }
        else phDetCnt++;
   }
}
#endif

//+
// Version 1 and 2 of DPLL control routine
//   at phase detector interrupt
//-

SIGNAL( SIG_INTERRUPT0 ) {
#ifndef DPLL
    PORTB ^= 1; // diagnostic pin
    phaseDetInt();
#else
    unsigned int cnt;
    unsigned char i, down, up;
    static unsigned int oldcnt, counter[ZTF];
    unsigned int cntSum;
    unsigned int cntAverage;
    static unsigned char counterPtr;

    if (!dpllEnable) {
        phaseDetInt();
        dpllInit=0;
        TCCR1B = 0x08; // clear on OC1A, stopped
        cbi( MCUCR, ISC01 );    // any logical change generates interrupt
        return;
    }
    PORTB ^= 1; // diagnostic pin
    phDetCnt=0;                 // phase detector timeout
    // dpll initialization
    if (dpllInit<3) {
    #ifndef SINGLEEDGEDET
        cbi( MCUCR, ISC01 );    // any logical change generates interrupt
    #else
        sbi( MCUCR, ISC01 );    // only rising edge generates interrupt
    #endif
        TCNT1H = DPLLINITVAL>>8;
        TCNT1L = DPLLINITVAL&0xff;
        if (!dpllInit) {
            // start counter here
            TCCR1B = 0x0a; // CTC,8 prescaler
        }
        up=down=0;
        oldcnt=(DPLLINITVAL*ZTF);
//        oldcnt=DPLLINITVAL;
        dpllInit++;
        for(i=0; i<ZTF; i++) counter[i]=DPLLINITVAL;
        counterPtr=0;
        return;
    }
    // dpll
    cnt=TCNT1L+(TCNT1H<<8);
    if (cnt>10000) cnt=0;
    counter[counterPtr]=cnt;
    if (counterPtr<(ZTF-1)) counterPtr++;
    else counterPtr=0;
    for (i=0,cntSum=0; i<ZTF; i++) cntSum+=counter[i];
    if (oldcnt>cntSum) {
        down=1;
        up=0;
    }
    else if (oldcnt<cntSum){
        down=0;
        up=1;
    }
    else {
        up=0;
        down=0;
    }
    cntAverage=(cntSum>>ZLSHIFT);
    if ((cntAverage>DPLLINITVAL)&&up) {
#ifdef DPLLV1
        if (cntAverage>(DPLLINITVAL+DPLLCONST/2)) dpllval+=DPLLCONST;
        else dpllval+=DPLLCONST/2;
#else
        dpllval+=DPLLCONST;
#endif
        OCR1AH=dpllval>>8;
        OCR1AL=dpllval&0xff;
    }
    else if ((cntAverage<DPLLINITVAL)&&down) {
#ifdef DPLLV1
        if (cntAverage<(DPLLINITVAL-DPLLCONST/2)) dpllval-=DPLLCONST;
        else dpllval-=DPLLCONST/2;
#else
        dpllval-=DPLLCONST;
#endif
        OCR1AH=dpllval>>8;
        OCR1AL=dpllval&0xff;
    }
    oldcnt=cntSum;
#endif
}

//
// Scan initialization
//

void scanInit() {
    memset( (unsigned char *)& OutDat.bv.b0, 0xff, sizeof( OutDatat ) );
    memset( (unsigned char *)& OutVoltage.bv.b0, 0x00, sizeof( OutDatat ) );
    memset( (unsigned char *)& OutCurrent.bv.b0, 0x00, sizeof( OutDatat ) );
    // diagnostic pins
    cbi( PORTB, 0 );
    cbi( PORTB, 1 );
    sbi( DDRB, 0 );
    sbi( DDRB, 1 );
    // output pins
    cbi( PORTC, 5 );
    sbi( DDRC, 5 );
    PORTD = 0;
    DDRD = 0xFB;
    // Timer 2
    TCCR2 = 0x08;           // CTC mode /8 prescaler
    OCR2 = TIMER2CNT;       // output compare constant
    cbi( TIMSK, OCIE2 );    // disable interrupts
    // DPLL timer
    TCCR1A = 0;
    OCR1A = 20000u; // time costant (16Mhz/8/2000 = 100Hz)
    TCNT1 = 0x80;
    TCCR1B = 0x08; // clear on OC1A, stopped
    sbi( TIMSK, OCIE1A ); // enable its interrupt
    // phase detect = INT 0
#if (!defined DPLL) || (!defined SINGLEEDGEDET)
    cbi( MCUCR, ISC01 );    // any logical change generates interrupt
#else
    sbi( MCUCR, ISC01 );    // only rising edge generates interrupt
#endif
    sbi( MCUCR, ISC00 );
    sbi( GIMSK, INT0 );     // and enable interrupts
}
