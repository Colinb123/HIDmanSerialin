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

uint8_t UsbUpdateCounter = 0;

void EveryMillisecond(void) {

	// Soft watchdog is to get around the fact that the real watchdog runs too fast
	SoftWatchdog++;
	if (SoftWatchdog > 5000) {
		// if soft watchdog overflows, just go into an infinite loop and we'll trigger the real watchdog
		DEBUGOUT("Soft overflow\n");
		while(1);
	}

	// otherwise, reset the real watchdog
	WDOG_COUNT = 0x00;
	// every 4 milliseconds (250hz), check one or the other USB port (so each gets checked at 125hz)
	if (UsbUpdateCounter == 4)
		s_CheckUsbPort0 = TRUE;
	else if (UsbUpdateCounter == 8){
		s_CheckUsbPort1 = TRUE;
		UsbUpdateCounter = 0;
	}

	UsbUpdateCounter++;
	

	// handle serial mouse
	#if defined(OPT_SERIAL_MOUSE)
		static uint8_t RTSHighCounter = 0;
		static __xdata uint8_t serialMousePrevMode = SERIAL_MOUSE_MODE_OFF;

		// High toggle (> 50ms) of RTS (P0.4) means host is resetting mouse.  Wait until falling edge and send mouse identification.

		if (P0 & 0b00010000) { // RTS is high (mouse is resetting)
			if (serialMouseMode != SERIAL_MOUSE_MODE_RESET) {
				serialMousePrevMode = serialMouseMode;
				serialMouseMode = SERIAL_MOUSE_MODE_RESET;
			}
			if (RTSHighCounter < 255) RTSHighCounter++;
			
		} else { // RTS is low
		if (serialMouseMode == SERIAL_MOUSE_MODE_RESET) {
				if (RTSHighCounter > 50) { // Check if RTS was high long enough to indicate reset...
					serialMouseMode = SERIAL_MOUSE_MODE_INIT;
				} else {
					serialMouseMode = serialMousePrevMode;
				}
				RTSHighCounter = 0;
			}
		}
	#endif

	// check the button
	inputProcess();

	// Deal with BAT (which requires a delay)

	if (KeyBatCounter){
		KeyBatCounter--;
		if (!KeyBatCounter)
			SimonSaysSendKeyboard(KEY_BATCOMPLETE);
	}

	if (MouseBatCounter){
		MouseBatCounter--;
		if (!MouseBatCounter) {
			SimonSaysSendMouse1(0xAA); // POST OK
			SimonSaysSendMouse1(0x00); // Squeek Squeek I'm a mouse
		}
	}

	if (MenuRateLimit) {
		MenuRateLimit--;
	}


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



// timer should run at 48MHz divided by (0xFFFF - (TH0TL0))
// i.e. 60khz
void mTimer0Interrupt(void) __interrupt(INT_NO_TMR0)
{

	if (OutputsEnabled) {

		switch (FlashSettings->KeyboardMode) {
			case (MODE_PS2):
				PS2ProcessPort(PORT_KEY);
				break;

			case (MODE_XT):
				XTProcessPort();
				break;

			case (MODE_AMSTRAD):
				AmstradProcessPort();
				break;
		}

		// May as well do this even in XT mode, can't hurt
		PS2ProcessPort(PORT_MOUSE);

		// Handle keyboard typematic repeat timers
		// (divide timer down to 15KHz to make maths easier)
		static uint8_t repeatDiv = 0;
		if (++repeatDiv == 4) {
			repeatDiv = 0;
			RepeatTimer();
		}
	}

	static uint8_t msDiv = 0;
	if (++msDiv == 60) {
		msDiv = 0;
		EveryMillisecond();
	}

}

// --- START OF NEW CODE ---

// Helper to select the correct scancode set (PS/2 uses Set 2, XT uses Set 1)
__code uint8_t* GetMakeCode(__code uint8_t* set1, __code uint8_t* set2) {
    if (FlashSettings->KeyboardMode == MODE_PS2) return set2;
    return set1;
}

// Function to check serial port and press keys
void HandleSerialKeys(void) {
    // Check if data is waiting in the UART buffer
    // (Using low-level register check from uart1.c logic)
    if (SER1_LSR & bLSR_DATA_RDY) { 
        char cmd = SER1_RBR; // Read the received byte directly from register
        
        __code uint8_t *make = NULL;
        __code uint8_t *break_code = NULL;

        // Map characters to HIDman scancodes defined in scancode.c
        switch (cmd) {
            case 'U': // Up Arrow
                make = GetMakeCode(KEY_SET1_UP_MAKE, KEY_SET2_UP_MAKE);
                break_code = GetMakeCode(KEY_SET1_UP_BREAK, KEY_SET2_UP_BREAK);
                break;
            case 'D': // Down Arrow
                make = GetMakeCode(KEY_SET1_DOWN_MAKE, KEY_SET2_DOWN_MAKE);
                break_code = GetMakeCode(KEY_SET1_DOWN_BREAK, KEY_SET2_DOWN_BREAK);
                break;
            case 'L': // Left Arrow
                make = GetMakeCode(KEY_SET1_LEFT_MAKE, KEY_SET2_LEFT_MAKE);
                break_code = GetMakeCode(KEY_SET1_LEFT_BREAK, KEY_SET2_LEFT_BREAK);
                break;
            case 'R': // Right Arrow
                make = GetMakeCode(KEY_SET1_RIGHT_MAKE, KEY_SET2_RIGHT_MAKE);
                break_code = GetMakeCode(KEY_SET1_RIGHT_BREAK, KEY_SET2_RIGHT_BREAK);
                break;
            case 'I': // Insert
                make = GetMakeCode(KEY_SET1_INSERT_MAKE, KEY_SET2_INSERT_MAKE);
                break_code = GetMakeCode(KEY_SET1_INSERT_BREAK, KEY_SET2_INSERT_BREAK);
                break;
            case 'E': // Escape
                make = GetMakeCode(KEY_SET1_ESCAPE_MAKE, KEY_SET2_ESCAPE_MAKE);
                break_code = GetMakeCode(KEY_SET1_ESCAPE_BREAK, KEY_SET2_ESCAPE_BREAK);
                break;
            case 'N': // Enter (Newline)
                make = GetMakeCode(KEY_SET1_ENTER_MAKE, KEY_SET2_ENTER_MAKE);
                break_code = GetMakeCode(KEY_SET1_ENTER_BREAK, KEY_SET2_ENTER_BREAK);
                break;
        }

        // If we found a valid key match
        if (make != NULL && break_code != NULL) {
            SendKeyboard(make);       // Press the key
            mDelaymS(20);             // Hold it briefly (20ms) so machine detects it
            SendKeyboard(break_code); // Release the key
        }
    }
}
// --- END OF NEW CODE ---

int main(void)
{
	bool WatchdogReset = 0;

	// Watchdog happened, go to "safe mode"
	if (!(PCON & bRST_FLAG0) && (PCON & bRST_FLAG1)){
		WatchdogReset = 1;
	}

	GPIOInit();

	//delay a bit, without using the builtin functions
	UsbUpdateCounter = 255;
	while (--UsbUpdateCounter);

#if defined(OSC_EXTERNAL)
	if (!(P3 & (1 << 4))) runBootloader();
#endif

	ClockInit();
    mDelaymS(500);   
	
#if !defined(BOARD_MICRO)
	InitUART0();
#endif

	InitUsbData();
	InitUsbHost();

	InitPWM();

	InitPS2Ports();

	// timer0 setup
	TMOD = (TMOD & 0xf0) | 0x02; // mode 1 (8bit auto reload)
	TH0 = 0xBD;					 // 60khz

	TR0 = 1; // start timer0
	ET0 = 1; //enable timer0 interrupt;

	EA = 1;	 // enable all interrupts
	// set serial port to recieve characters
	CH559UART1Init(20, 1, 1, 112500, 8);

	memset(SendBuffer, 0, 255);
	memset(MouseBuffer, 0, MOUSE_BUFFER_SIZE);

	if (WatchdogReset) DEBUGOUT("Watchdog reset detected (%x), entering safemode\n", PCON);

	InitSettings(WatchdogReset);

	// enable watchdog
	WDOG_COUNT = 0x00;
	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;
	GLOBAL_CFG |= bWDOG_EN;

	WDOG_COUNT = 0x00;

	//while(1);

	OutputsEnabled = 1;

	DEBUGOUT("ok\n");

	// after 750ms output a BAT OK code on keyboard and mouse
	// there's a 500ms delay earlier so do 250 now
	MouseBatCounter = 250;
	KeyBatCounter = 250;

	// main loop
	while (1)
	{
		// reset watchdog
		SoftWatchdog = 0;
		if (MenuActive)
			Menu_Task();
		ProcessUsbHostPort();
		HandleSerialKeys();   // NEW: Handles Serial from Pi
		HandleMouse(); // used to process the serial commands 
		ProcessKeyboardLed();
		HandleRepeats();
		//P0 ^= 0b00100000;
		
	}
}
