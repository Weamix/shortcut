/*
             LUFA Library
     Copyright (C) Dean Camera, 2021.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2021  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the Echo demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#define  INCLUDE_FROM_ECHO_C
#include "Echo.h"


/** Main program entry point. This routine configures the hardware required by the application, then
 *  enters a loop to run the application tasks in sequence.
 */
int main(void)
{
	CLKSEL0 = 0b00010101;   // sélection de l'horloge externe
	CLKSEL1 = 0b00001111;   // minimum de 8Mhz
	CLKPR = 0b10000000;     // modification du diviseur d'horloge (CLKPCE=1)
	CLKPR = 0;              // 0 pour pas de diviseur (diviseur de 1)

	SetupHardware();

	GlobalInterruptEnable();

	for (;;)
	{
		Handle_EP_IN();
		Handle_EP_OUT();
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
	/* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
	XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
	XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

	/* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
	XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
	XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
#endif

	/* Hardware Initialization */
	USB_Init();
}

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs. */
void EVENT_USB_Device_Connect(void)
{
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs.
 */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host set the current configuration
 *  of the USB device after enumeration - the device endpoints are configured.
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	/* Setup Endpoints */
	Endpoint_ConfigureEndpoint(ECHO_IN_EPADDR, EP_TYPE_INTERRUPT, ECHO_EPSIZE, 1);
	Endpoint_ConfigureEndpoint(ECHO_OUT_EPADDR, EP_TYPE_INTERRUPT, ECHO_EPSIZE, 1);
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void)
{
}

/** Byte storage for EP **/
uint8_t EP_DataKey;
uint8_t[15] EP_DataShortcut;

void Handle_EP_IN(void)
{
	/* Select the IN Endpoint */
	Endpoint_SelectEndpoint(ECHO_IN_EPADDR);

	/* Check if IN Endpoint Ready for Read/Write */
	if (Endpoint_IsReadWriteAllowed())
	{
		/* Write Keyboard Report Data */
		Endpoint_Write_8(EP_DataKey);

		/* Finalize the stream transfer to send the last packet */
		Endpoint_ClearIN();
	}
}

void Handle_EP_OUT(void)
{
	/* Select the OUT Endpoint */
	Endpoint_SelectEndpoint(ECHO_OUT_EPADDR);

	/* Check if Endpoint contains a packet */
	if (Endpoint_IsOUTReceived())
	{
		/* Check to see if the packet contains data */
		if (Endpoint_IsReadWriteAllowed())
		{
			/* Read in the LED report from the host */
			EP_DataKey = Endpoint_Read_8();

			// read the 15 bytes of the shortcut (fill with 0 if not 15 bytes)
			for (int i = 0; i < 15; i++) {
				if (Endpoint_BytesInEndpoint() > 0) {
					EP_DataShortcut[i] = Endpoint_Read_8();
				} else {
					EP_DataShortcut[i] = 0;
				}
			}
		}

		/* Handshake the OUT Endpoint - clear endpoint */
		Endpoint_ClearOUT();
	}
}
