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
static unsigned int cTime = 0;

// Dpll data
static unsigned char dpllInit=0; //dpll init flag
unsigned char dpllEnable=0x00;   //dpll enable flag - DPLL NORMALLY OFF!!!
static unsigned int dpllval=20000u;

// A/D converter data
volatile OutDatat OutVoltage, OutCurrent;

unsigned char tmr2Start=0;
char phDetCnt=-1;
unsigned int var_close = TRIAC_CLOSURE; //initial period cut-off time (approx. 9.24ms)

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
//output compare: TCNT2 & OCR2
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
//    if ( cTime > TRIAC_CLOSURE ) { // cut off at the end if not direkt!!!
    if ( cTime > var_close ) { // cut off at the end if not direkt!!!
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
        PORTC^=8;       // toggle PORT C3
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
	static unsigned int mycntSum[3] = {20000u,20000u,20000u};

    if (!dpllEnable) {
    //    phaseDetInt(); //timer2 is reset in new code segment
/* new code */
	//    cbi( TIMSK, OCIE1A ); // disable TMR1 interrupt
		// add 0.5ms delay 
        cbi( TIMSK, OCIE2 ); // disable interrupts
        TCCR2 = 0x08; // CTC mode, stop
        TCNT2 = 0; // reset
        OCR2=PHASEDELAY; //approx. 0.5ms
        sbi( SFIOR, PSR2 ); // and reset prescaler
        TCCR2 = 0x0b; // CTC mode, /32 prescaler =>run
        sbi( TIMSK, OCIE2 ); // enable interrupts
        tmr2Start=1;
		cTime = 0;	

		if(dpllInit>0) {
			OCR1A = 0xffff; // overflow on maximal counter value
			phDetCnt=-1;
		}
		if(phDetCnt<0) var_close = TRIAC_CLOSURE;		
		else {
		    cnt=TCNT1L+(TCNT1H<<8);
			if( (cnt > MAXFREQ) && (cnt < MINFREQ) ) { //if measured value between 45-55 Hz halfperiod
				mycntSum[0]=mycntSum[1];
				mycntSum[1]=mycntSum[2];
				mycntSum[2]=cnt;
				for(i=0,cntAverage=0;i<3;i++) cntAverage+=(unsigned int) mycntSum[i]/3;
				var_close = ((unsigned int) cntAverage/TIMER2CNT) - HPERIODCUTOFF - 13u; //calc cutoff time 
				// subtract (13u ~ 0.5ms) to compensate initial delay
			    PORTB &= 0xfe; // clear diagnostic pin RB0
			}
		}
   	    TCCR1B = 0x08; // clear on OC1A, stopped
		TCNT1 = 0; //reset timer count
   	    TCCR1B = 0x0a; // CTC, 8 prescaler, start

/* end new code */
        dpllInit=0;
		phDetCnt=0; 
//        TCCR1B = 0x08; // clear on OC1A, stopped
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
			var_close=TRIAC_CLOSURE; //restore initial closure time
			OCR1A = 20000u; //restore output compare match value
			TCCR1B = 0x0a; // CTC,8 prescaler, start
		    sbi( TIMSK, OCIE1A ); // enable TMR1 interrupt	
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
/*    // DPLL timer
    TCCR1A = 0;
    OCR1A = 20000u; // time costant (16Mhz/8/20000 = 100Hz)
    TCNT1 = 0x80;
    TCCR1B = 0x08; // clear on OC1A, stopped
    sbi( TIMSK, OCIE1A ); // enable its interrupt
*/
/* new code */
    TCCR1A = 0;
    OCR1A = 0xffff; //MAX value 
    TCNT1 = 0x00;
    TCCR1B = 0x08; // clear on OC1A, stopped
    sbi( TIMSK, OCIE1A ); // enable its interrupt (let it run)
/* end new code */
    // phase detect = INT 0
#if (!defined DPLL) || (!defined SINGLEEDGEDET)
    cbi( MCUCR, ISC01 );    // any logical change generates interrupt
#else
    sbi( MCUCR, ISC01 );    // only rising edge generates interrupt
#endif
    sbi( MCUCR, ISC00 );
    sbi( GIMSK, INT0 );     // and enable interrupts
}
