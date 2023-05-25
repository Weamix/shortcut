CC = avr-gcc
OBJCOPY = avr-objcopy
DFU_PROGRAMMER = dfu-programmer

CFLAGS = -mmcu=atmega16u2 -DF_CPU=8000000UL -c -Wall -I. -Os
LDFLAGS = -mmcu=atmega16u2 -g -lm -Wl,--gc-sections

all: timer.hex

timer.o: timer.c
	$(CC) $(CFLAGS) $< -o $@

timer.elf: timer.o
	$(CC) $(LDFLAGS) $< -o $@

timer.hex: timer.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

erase:
	$(DFU_PROGRAMMER) atmega16u2 erase

flash: timer.hex
	$(DFU_PROGRAMMER) atmega16u2 flash $<

reset:
	$(DFU_PROGRAMMER) atmega16u2 reset

clean:
	rm -f timer.o timer.elf timer.hex
