	    CC = avr-gcc
#-mcall-prologues
	    AS = avr-as
	    LD = avr-ld
	LFLAGS = -L/usr/avr/lib/avr5 -mmcu=atmega8
	CFLAGS = -g -O2 -Wall -I/usr/avr/include -mmcu=atmega8

all:TimeBase.elf

%.o : %.c
	avr-gcc -c $(CFLAGS) $<


Scan.o: Scan.c Scan.h TimeBase.h
TimeBase.o:	TimeBase.c TimeBase.h
Spi.o: Spi.c Spi.h Scan.h

TimeBase.elf: TimeBase.o Scan.o Spi.o
	avr-gcc -o TimeBase.elf -g TimeBase.o Scan.o Spi.o -Wl,-Map=TimeBase.map,--cref $(LFLAGS)
	$(LD) -mavr5 --oformat=ihex TimeBase.elf -o TimeBase.hex
	$(LD) -mavr5 --oformat=binary TimeBase.elf -o TimeBase.bin

prg:
	$(LD) -mavr5 --oformat=srec TimeBase.elf -o TimeBase.srec
	sudo uisp -dprog=dapa --erase
	sudo uisp -dprog=dapa --upload if=TimeBase.srec -v=3

init:
	sudo uisp -dprog=dapa --segment=fuse --upload if=fuse.srec

clean:
	rm -f *.o *.elf t *.i.* *.i *.c.*

rebuild: clean all
