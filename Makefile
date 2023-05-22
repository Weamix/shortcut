CC = avr-gcc
OBJCOPY = avr-objcopy
DFU_PROGRAMMER = dfu-programmer

CFLAGS = -mmcu=at90usb162 -DF_CPU=8000000UL -c -Wall -I. -Os
LDFLAGS = -mmcu=at90usb162 -g -lm -Wl,--gc-sections

all: timer.hex

timer.o: timer.c
	$(CC) $(CFLAGS) $< -o $@

timer.elf: timer.o
	$(CC) $(LDFLAGS) $< -o $@

timer.hex: timer.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

erase:
	$(DFU_PROGRAMMER) at90usb162 erase

flash: erase timer.hex
	$(DFU_PROGRAMMER) at90usb162 flash $<

reset:
	$(DFU_PROGRAMMER) at90usb162 reset

clean:
	rm -f timer.o timer.elf timer.hex
