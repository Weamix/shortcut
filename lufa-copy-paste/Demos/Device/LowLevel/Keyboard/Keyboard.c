/*
             LUFA Library
     Copyright (C) Dean Camera, 2021.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2021  Dean Camera (dean [at] fourwalledcubicle [dot] com)
  Copyright 2010  Denver Gingerich (denver [at] ossguy [dot] com)

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
 *  Main source file for the Keyboard demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#include "Keyboard.h"

/** Indicates what report mode the host has requested, true for normal HID reporting mode, \c false for special boot
 *  protocol reporting mode.
 */
static bool UsingReportProtocol = true;

/** Current Idle period. This is set by the host via a Set Idle HID class request to silence the device's reports
 *  for either the entire idle duration, or until the report status changes (e.g. the user presses a key).
 */
static uint16_t IdleCount = 500;

/** Current Idle period remaining. When the IdleCount value is set, this tracks the remaining number of idle
 *  milliseconds. This is separate to the IdleCount timer and is incremented and compared as the host may request
 *  the current idle period via a Get Idle HID class request, thus its value must be preserved.
 */
static uint16_t IdleMSRemaining = 0;

/** Byte storage for EP **/
// A matrix of 6 rows (keys on the embarqued system) and 15 columns (shortcuts for each key)
uint8_t EP_DataShortcutsMatrix[6][15];


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

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

	for (;;)
	{
		HID_Task();
        Handle_EP_IN();
        Handle_EP_OUT();
		USB_USBTask();

	}
}

void Handle_EP_IN(void)
{
    /* Select the IN Endpoint */
    Endpoint_SelectEndpoint(PGM_IN_EPADDR);

    /* Check if IN Endpoint Ready for Read/Write */
    if (Endpoint_IsReadWriteAllowed())
    {
        /* Write Keyboard Report Data */
        Endpoint_Write_8(0x00);

        /* Finalize the stream transfer to send the last packet */
        Endpoint_ClearIN();
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


// Leds en sortie
    DDRB |= 0xf0;
    PORTB &= ~0xf0;

// Boutons (colX) en entree
    DDRD &= ~0x78;
    PORTD |= 0x78;

// Boutons (rowX) en entree
    DDRB &= ~0x0f;
    PORTB |= 0x0f;
}

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
void EVENT_USB_Device_Connect(void)
{
	/* Indicate USB enumerating */
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);

	/* Default to report protocol on connect */
	UsingReportProtocol = true;
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs.
 */
void EVENT_USB_Device_Disconnect(void)
{
	/* Indicate USB not ready */
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host sets the current configuration
 *  of the USB device after enumeration, and configures the keyboard device endpoints.
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	/* Setup HID Report Endpoints */
	ConfigSuccess &= Endpoint_ConfigureEndpoint(KEYBOARD_IN_EPADDR, EP_TYPE_INTERRUPT, KEYBOARD_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(KEYBOARD_OUT_EPADDR, EP_TYPE_INTERRUPT, KEYBOARD_EPSIZE, 1);

    /* Setup Endpoints */
    ConfigSuccess &= Endpoint_ConfigureEndpoint(PGM_IN_EPADDR, EP_TYPE_INTERRUPT, PGM_INEPSIZE, 1);
    ConfigSuccess &= Endpoint_ConfigureEndpoint(PGM_OUT_EPADDR, EP_TYPE_INTERRUPT, PGM_OUTEPSIZE, 1);

	/* Turn on Start-of-Frame events for tracking HID report period expiry */
	USB_Device_EnableSOFEvents();

	/* Indicate endpoint configuration success or failure */
	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void)
{
	/* Handle HID Class specific requests */
	switch (USB_ControlRequest.bRequest)
	{
		case HID_REQ_GetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				USB_KeyboardReport_Data_t KeyboardReportData;

				/* Create the next keyboard report for transmission to the host */
				CreateKeyboardReport(&KeyboardReportData);

				Endpoint_ClearSETUP();

				/* Write the report data to the control endpoint */
				Endpoint_Write_Control_Stream_LE(&KeyboardReportData, sizeof(KeyboardReportData));
				Endpoint_ClearOUT();
			}

			break;
		case HID_REQ_SetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();

				/* Wait until the LED report has been sent by the host */
				while (!(Endpoint_IsOUTReceived()))
				{
					if (USB_DeviceState == DEVICE_STATE_Unattached)
					  return;
				}

				/* Read in the LED report from the host */
				uint8_t LEDStatus = Endpoint_Read_8();

				Endpoint_ClearOUT();
				Endpoint_ClearStatusStage();

				/* Process the incoming LED report */
				ProcessLEDReport(LEDStatus);
			}

			break;
		case HID_REQ_GetProtocol:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();

				/* Write the current protocol flag to the host */
				Endpoint_Write_8(UsingReportProtocol);

				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
		case HID_REQ_SetProtocol:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();

				/* Set or clear the flag depending on what the host indicates that the current Protocol should be */
				UsingReportProtocol = (USB_ControlRequest.wValue != 0);
			}

			break;
		case HID_REQ_SetIdle:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();

				/* Get idle period in MSB, IdleCount must be multiplied by 4 to get number of milliseconds */
				IdleCount = ((USB_ControlRequest.wValue & 0xFF00) >> 6);
			}

			break;
		case HID_REQ_GetIdle:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();

				/* Write the current idle duration to the host, must be divided by 4 before sent to host */
				Endpoint_Write_8(IdleCount >> 2);

				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
	}
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	/* One millisecond has elapsed, decrement the idle time remaining counter if it has not already elapsed */
	if (IdleMSRemaining)
	  IdleMSRemaining--;
}

/** Fills the given HID report data structure with the next HID report to send to the host.
 *
 *  \param[out] ReportData  Pointer to a HID report data structure to be filled
 */
void CreateKeyboardReport(USB_KeyboardReport_Data_t* const ReportData)
{
    uint8_t UsedKeyCodes = 0;

    /* Clear the report contents */
	memset(ReportData, 0, sizeof(USB_KeyboardReport_Data_t));

    // Add shortcuts for copy, paste, and delete

    // SCHEMA : https://wiki-se.plil.fr/mediawiki/images/6/6b/I2L-2022-G4-schema.pdf
    // PB3 à PB0  - PD3 à PD6: touches
    // LEDS : PB7 à PB4

    if (!(PIND & 0x40)) // PD6
    {
        for (int i = 0; i < 4; i++) {
			// TODO: change the 0 to the correct row
			ReportData->KeyCode[UsedKeyCodes++] = EP_DataShortcutsMatrix[0][i];
        }
    }

    if (!(PIND & 0x20)) // PD3
    {
        for (int i = 0; i < 4; i++) {
			// TODO: change the 1 to the correct row
			ReportData->KeyCode[UsedKeyCodes++] = EP_DataShortcutsMatrix[1][i];
        }
    }

   	if (!(PINB & 0x08)) // PB3
   	{
		for (int i = 0; i < 4; i++) {
			// TODO: change the 2 to the correct row
			ReportData->KeyCode[UsedKeyCodes++] = EP_DataShortcutsMatrix[2][i];
        }
	}

}

/** Processes a received LED report, and updates the board LEDs states to match.
 *
 *  \param[in] LEDReport  LED status report from the host
 */
void ProcessLEDReport(const uint8_t LEDReport)
{
	uint8_t LEDMask = LEDS_LED2;

	if (LEDReport & HID_KEYBOARD_LED_NUMLOCK)
	  LEDMask |= LEDS_LED1;

	if (LEDReport & HID_KEYBOARD_LED_CAPSLOCK)
	  LEDMask |= LEDS_LED3;

	if (LEDReport & HID_KEYBOARD_LED_SCROLLLOCK)
	  LEDMask |= LEDS_LED4;

	/* Set the status LEDs to the current Keyboard LED status */
	LEDs_SetAllLEDs(LEDMask);
}

/** Sends the next HID report to the host, via the keyboard data endpoint. */
void SendNextReport(void)
{
	static USB_KeyboardReport_Data_t PrevKeyboardReportData;
	USB_KeyboardReport_Data_t        KeyboardReportData;
	bool                             SendReport = false;

	/* Create the next keyboard report for transmission to the host */
	CreateKeyboardReport(&KeyboardReportData);

	/* Check if the idle period is set and has elapsed */
	if (IdleCount && (!(IdleMSRemaining)))
	{
		/* Reset the idle time remaining counter */
		IdleMSRemaining = IdleCount;

		/* Idle period is set and has elapsed, must send a report to the host */
		SendReport = true;
	}
	else
	{
		/* Check to see if the report data has changed - if so a report MUST be sent */
		SendReport = (memcmp(&PrevKeyboardReportData, &KeyboardReportData, sizeof(USB_KeyboardReport_Data_t)) != 0);
	}

	/* Select the Keyboard Report Endpoint */
	Endpoint_SelectEndpoint(KEYBOARD_IN_EPADDR);

	/* Check if Keyboard Endpoint Ready for Read/Write and if we should send a new report */
	if (Endpoint_IsReadWriteAllowed() && SendReport)
	{
		/* Save the current report data for later comparison to check for changes */
		PrevKeyboardReportData = KeyboardReportData;

		/* Write Keyboard Report Data */
		Endpoint_Write_Stream_LE(&KeyboardReportData, sizeof(KeyboardReportData), NULL);

		/* Finalize the stream transfer to send the last packet */
		Endpoint_ClearIN();
	}
}

/** Reads the next LED status report from the host from the LED data endpoint, if one has been sent. */
void ReceiveNextReport(void)
{
	/* Select the Keyboard LED Report Endpoint */
	Endpoint_SelectEndpoint(KEYBOARD_OUT_EPADDR);

	/* Check if Keyboard LED Endpoint contains a packet */
	if (Endpoint_IsOUTReceived())
	{
		/* Check to see if the packet contains data */
		if (Endpoint_IsReadWriteAllowed())
		{
			/* Read in the LED report from the host */
			uint8_t LEDReport = Endpoint_Read_8();

			/* Process the read LED report from the host */
			ProcessLEDReport(LEDReport);
		}

		/* Handshake the OUT Endpoint - clear endpoint and ready for next report */
		Endpoint_ClearOUT();
	}
}

/** Function to manage HID report generation and transmission to the host, when in report mode. */
void HID_Task(void)
{
	/* Device must be connected and configured for the task to run */
	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

	/* Send the next keypress report to the host */
	SendNextReport();

	/* Process the LED report sent from the host */
	ReceiveNextReport();
}

unsigned char convert(unsigned char ascii){
    switch(ascii){
        case 'a': return HID_KEYBOARD_SC_A;
        case 'b': return HID_KEYBOARD_SC_B;
        case 'c': return HID_KEYBOARD_SC_C;
        case 'd': return HID_KEYBOARD_SC_D;
        case 'e': return HID_KEYBOARD_SC_E;
        case 'f': return HID_KEYBOARD_SC_F;
        case 'g': return HID_KEYBOARD_SC_G;
        case 'h': return HID_KEYBOARD_SC_H;
        case 'i': return HID_KEYBOARD_SC_I;
        case 'j': return HID_KEYBOARD_SC_J;
        case 'k': return HID_KEYBOARD_SC_K;
        case 'l': return HID_KEYBOARD_SC_L;
        case 'm': return HID_KEYBOARD_SC_M;
        case 'n': return HID_KEYBOARD_SC_N;
        case 'o': return HID_KEYBOARD_SC_O;
        case 'p': return HID_KEYBOARD_SC_P;
        case 'q': return HID_KEYBOARD_SC_Q;
        case 'r': return HID_KEYBOARD_SC_R;
        case 's': return HID_KEYBOARD_SC_S;
        case 't': return HID_KEYBOARD_SC_T;
        case 'u': return HID_KEYBOARD_SC_U;
        case 'v': return HID_KEYBOARD_SC_V;
        case 'w': return HID_KEYBOARD_SC_W;
        case 'x': return HID_KEYBOARD_SC_X;
        case 'y': return HID_KEYBOARD_SC_Y;
        case 'z': return HID_KEYBOARD_SC_Z;
        case '0': return HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS;
        case '1': return HID_KEYBOARD_SC_1_AND_EXCLAMATION;
        case '2': return HID_KEYBOARD_SC_2_AND_AT;
        case '3': return HID_KEYBOARD_SC_3_AND_HASHMARK;
        case '4': return HID_KEYBOARD_SC_4_AND_DOLLAR;
        case '5': return HID_KEYBOARD_SC_5_AND_PERCENTAGE;
        case '6': return HID_KEYBOARD_SC_6_AND_CARET;
        case '7': return HID_KEYBOARD_SC_7_AND_AMPERSAND;
        case '8': return HID_KEYBOARD_SC_8_AND_ASTERISK;
        case '9': return HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS;
        default: return  HID_KEYBOARD_SC_SPACE;
    }

}

void Handle_EP_OUT(void)
{
    /* Select the OUT Endpoint */
    Endpoint_SelectEndpoint(PGM_OUT_EPADDR);

    /* Check if Endpoint contains a packet */
    if (Endpoint_IsOUTReceived())
    {
        /* Check to see if the packet contains data */
        if (Endpoint_IsReadWriteAllowed())
        {
            /* Read in the LED report from the host */
            uint8_t EP_DataKey = Endpoint_Read_8();
			int row = EP_DataKey & 0x0f;

			// read the 15 bytes of the shortcut and put it in a temporary variable
			//uint8_t EP_DataShortcut[15];
            if(row<3){
                PORTB |= (1<<(row+5));
                for (int i = 0; i < 15; i++) {
                    uint8_t key = Endpoint_Read_8();
                    if(key==0){
                        break;
                    }
                    if (Endpoint_BytesInEndpoint() > 0) {
                        if(i<4){
                            EP_DataShortcutsMatrix[row][i] = convert(key);
                        }
                    } else {
                        EP_DataShortcutsMatrix[row][i] = 0;
                    }
                }
            }
        }

        /* Handshake the OUT Endpoint - clear endpoint */
        Endpoint_ClearOUT();
    }
}


