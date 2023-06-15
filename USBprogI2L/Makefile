#
# Makefile for USB projects
#

LIBUSB_OBJS = libusb.o
ECHO_OBJS = proj_echo.o
LIBS = -L . -lusb -lusb-1.0
CFLAGS += -Wall -I . -DVERBOSE -DVERBOSE_USB_TRANSFERT -DMODE_MANUAL

all: libusb.a proj_echo

#
# Cleaning target
#

clean:
	rm -f core *.o libusb.a proj_echo

#
# Library targets
#

libusb.o: libusb.c libusb.h

libusb.a: $(LIBUSB_OBJS)
	rm -f $@
	$(AR) rcs $@ $+

#
# Executable targets
#

proj_echo: $(ECHO_OBJS)
	$(CC) -o $@ $(ECHO_OBJS) $(LIBS)

proj_echo.o: proj_echo.c libusb.h
