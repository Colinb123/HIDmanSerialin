#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ch559.h"
#include "usbhost.h"
#include "uart.h"
#include "ps2protocol.h"
#include "ps2.h"
#include "parsedescriptor.h"
#include "menu.h"
#include "mouse.h"
#include "xt.h"
#include "amstrad.h"
#include "pwm.h"
#include "keyboardled.h"
#include "dataflash.h"
#include "settings.h"
#include "system.h"
#include "serial_input.h" 
#include "scancode.h"

// Variables to fix linker errors
volatile uint8_t KeyBatCounter = 0;
volatile uint8_t MouseBatCounter = 0;
extern uint16_t MenuRateLimit;
// Fix for SDCC 4.x Crash
void mTimer0Interrupt(void) __interrupt(INT_NO_TMR0);

uint8_t UsbUpdateCounter = 0;

void EveryMillisecond(void) {
	SoftWatchdog++;
	if (SoftWatchdog > 5000) { while(1); }
	WDOG_COUNT = 0x00;

	if (UsbUpdateCounter == 4) s_CheckUsbPort0 = TRUE;
	else if (UsbUpdateCounter == 8){ s_CheckUsbPort1 = TRUE; UsbUpdateCounter = 0; }
	UsbUpdateCounter++;

	inputProcess(); 

	if (KeyBatCounter){ KeyBatCounter--; if (!KeyBatCounter) SimonSaysSendKeyboard(KEY_BATCOMPLETE); }
	if (MouseBatCounter){ MouseBatCounter--; if (!MouseBatCounter) { SimonSaysSendMouse1(0xAA); SimonSaysSendMouse1(0x00); } }
	if (MenuRateLimit) { MenuRateLimit--; }

	// Turn current LED on if we haven't seen any activity in a while
	if (LEDDelayMs) {
		LEDDelayMs--;
	} else {
#if defined(BOARD_MICRO)
		SetPWM2Dat(0x10);
#elif defined (BOARD_PS2)
		P0 |= 0b01110000;

		switch (FlashSettings->KeyboardMode) {
			case MODE_PS2:
				// blue
				P0 &= ~0b01000000;
			break;
			case MODE_XT:
				// orange
				P0 &= ~0b00010000;
				P0 &= ~0b00100000;
			break;
			case MODE_AMSTRAD:
				// white
				P0 &= ~0b00010000;
				P0 &= ~0b00100000;
				P0 &= ~0b01000000;
			break;
		}

#else
			SetPWM1Dat(0x00);
			SetPWM2Dat(0x00);
			T3_FIFO_L = 0;
			T3_FIFO_H = 0;

			switch (FlashSettings->KeyboardMode){
				case MODE_PS2:
					// blue
					T3_FIFO_L = 0xFF;
					T3_FIFO_H = 0;
				break;
				case MODE_XT:
					// orange
					SetPWM2Dat(0x10);
					SetPWM1Dat(0x40);
				break;
				case MODE_AMSTRAD:
					// white
					SetPWM1Dat(0x30);
					SetPWM2Dat(0x20);
					T3_FIFO_L = 0x3F;
					T3_FIFO_H = 0;
				break;

			}
#endif
	}
}

void mTimer0Interrupt(void) __interrupt(INT_NO_TMR0) {
	if (OutputsEnabled) {
        // --- FORCE PS/2 OUTPUT ---
        // This ensures the buffer gets emptied to the PC
        PS2ProcessPort(PORT_KEY); 
		PS2ProcessPort(PORT_MOUSE);
		
        static uint8_t repeatDiv = 0;
		if (++repeatDiv == 4) { repeatDiv = 0; RepeatTimer(); }
	}
	static uint8_t msDiv = 0;
	if (++msDiv == 60) { msDiv = 0; EveryMillisecond(); }
}

int main(void) {
	bool WatchdogReset = 0;
	if (!(PCON & bRST_FLAG0) && (PCON & bRST_FLAG1)){ WatchdogReset = 1; }

	GPIOInit();
	UsbUpdateCounter = 255; while (--UsbUpdateCounter);

#if defined(OSC_EXTERNAL)
	if (!(P3 & (1 << 4))) runBootloader();
#endif

	ClockInit();
    mDelaymS(500); 
	CH559UART1Init(20, 1, 1, BAUD_RATE, 8);
#if !defined(BOARD_MICRO)
#endif
	InitUsbData();
	InitUsbHost();
	InitPWM();
	InitPS2Ports();

	TMOD = (TMOD & 0xf0) | 0x02; TH0 = 0xBD;
	TR0 = 1; ET0 = 1; EA = 1; 



	memset(SendBuffer, 0, 32); 
	memset(MouseBuffer, 0, MOUSE_BUFFER_SIZE);

	InitSettings(WatchdogReset);

	WDOG_COUNT = 0x00; SAFE_MOD = 0x55; SAFE_MOD = 0xAA; GLOBAL_CFG |= bWDOG_EN; WDOG_COUNT = 0x00;

	OutputsEnabled = 1;
	DEBUGOUT("ok\n");

	while (1) {
		SoftWatchdog = 0;
		if (MenuActive) Menu_Task();
		ProcessUsbHostPort(); 
        
        HandleSerialKeys();   // <--- SERIAL INPUT
        
		HandleMouse();       
		ProcessKeyboardLed();
		HandleRepeats();
	}
}