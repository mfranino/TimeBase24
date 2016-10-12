#ifdef DPLLV1
// dpll ki dela
SIGNAL( SIG_OUTPUT_COMPARE1A ) {
#ifdef DPLL
    if (dpllEnable) {
        PORTC^=8;       // toggle PORT C 3
        phaseDetInt();
    }
#endif
}
#endif


#ifdef DPLLV1
//+
// Version 1 of DPLL control routine
//-
SIGNAL( SIG_INTERRUPT0 ) {
    unsigned int cnt;
    unsigned char cntl,cnth;
    PORTB ^= 1; // diagnostic pin
#ifndef DPLL
    phaseDetInt();
#else
    if (!dpllEnable) {
        phaseDetInt();
        dpllInit=0;
        TCCR1B = 0x08; // clear on OC1A, stopped
        return;
    }
    // dpll
    if (dpllInit<3) {
        TCNT1H = 0;
        TCNT1L = 0;
        if (!dpllInit) {
            // start counter here
            TCCR1B = 0x0a; // CTC,8 prescaler
        }
        dpllInit++;
    } else {
        cnt=0;
        cntl=TCNT1L; cnth=TCNT1H;
        cnt=(cnth<<8)+cntl;
        if (cnt<10000u) {
            dpllval+=DPLLCONST;
            OCR1AH=dpllval>>8;
            OCR1AL=dpllval&0xff;
        } else {
            if (cnt<(dpllval-(DPLLCONST<<1))) {
                dpllval-=DPLLCONST;
                OCR1AH=dpllval>>8;
                OCR1AL=dpllval&0xff;
            }
        }
    }
#endif
}
#endif
