// +
// Digital HE TimeBase main program
//
// iz@elsis.si
// 2003.06.21
//
// -

#include <avr/io.h>
#include <avr/signal.h>
#include <avr/interrupt.h>
#include <string.h>
#include <avr/wdt.h>

#include "TimeBase.h"
#include "Scan.h"
#include "Spi.h"


int main() {
    unsigned char i;
    cbi( PORTC, 3 );
    sbi( DDRC, 3 );
    scanInit();
    spiInit();
    sei();
    for ( ; ; ) {
        for ( i = 0; i < 100; i++ )
            asm( "nop\n" );
        for ( i = 0; i < 100; i++ )
            asm( "nop\n" );
    }
}
