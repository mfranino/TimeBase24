// +
// Spi communication
//
// iz@elsis.si
// 2003.06.01
//
// -

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <string.h>

#include "Scan.h"
#include "Spi.h"

#define WAIT (unsigned char)0x80
#define ACCEPT (unsigned char)0x00

static unsigned char spiState = WAIT;
static unsigned char flagCurVol = 0;
static unsigned char spiCnt = 0;

// +
// SPI interrupt
//-


INTERRUPT( SIG_SPI ) {
    unsigned char b;
    b = SPSR;
    b = SPDR;
    if ( spiState == WAIT ) {
        if ( (b&0x0f) == 0x0a ) {
            flagCurVol = 1;
            spiCnt = 0;
            SPDR = OutVoltage.ov[0];
            spiState = ACCEPT;
            dpllEnable=b&0x10;
            return;
        }
        if ( (b&0x0f) == 0x05 ) {
            flagCurVol = 0;
            spiCnt = 0;
            spiState = ACCEPT;
            dpllEnable=b&0x10;
            SPDR = OutCurrent.ov[0];
            return;
        }
        return;
    }
    if ( spiCnt < 8 ) {
        OutDat.ov[spiCnt] = ~b; // read the value
        if ( spiCnt < 7 ) {
            if ( flagCurVol ) SPDR = OutVoltage.ov[spiCnt + 1];
            else
                SPDR = OutCurrent.ov[spiCnt + 1];
        }
        spiCnt++;
    }
    else {
        dirMask = b; // get direct mask byte
        spiState = WAIT; // wait for next packet
    }
}



//+
// Initializaton code
//-

void spiInit() {
    SPCR = 0xC7; // 1/128 CLK, slave
}
