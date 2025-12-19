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
#include "serial_input.h" // Includes our serial logic

uint8_t UsbUpdateCounter = 0;

void EveryMillisecond(void) {
	SoftWatchdog++;
	if (SoftWatchdog > 5000) { while(1); }
	WDOG_COUNT = 0x00;

	if (UsbUpdateCounter == 4) s_CheckUsbPort0 = TRUE;
	else if (UsbUpdateCounter == 8){ s_CheckUsbPort1 = TRUE; UsbUpdateCounter = 0; }
	UsbUpdateCounter++;

	// Check input (now disabled in menu.c)
	inputProcess();

	if (KeyBatCounter){ KeyBatCounter--; if (!KeyBatCounter) SimonSaysSendKeyboard(KEY_BATCOMPLETE); }
	if (MouseBatCounter){ MouseBatCounter--; if (!MouseBatCounter) { SimonSaysSendMouse1(0xAA); SimonSaysSendMouse1(0x00); } }
	if (MenuRateLimit) { MenuRateLimit--; }

	if (LEDDelayMs) {
		LEDDelayMs--;
	} else {
        // --- HEARTBEAT LOGIC ---
        // Blink Orange every 500ms to show code is running
        static uint16_t Heartbeat = 0;
        Heartbeat++;
        
        if (Heartbeat > 500) {
            // Force Orange LED (XT Color)
            #if defined(BOARD_PS2)
                P0 |= 0b01110000; // Turn off RGB
                P0 &= ~0b00010000; P0 &= ~0b00100000; // Turn on Orange bits
            #endif
            
            // Reset counter after 50ms blink
            if (Heartbeat > 550) Heartbeat = 0;
            
        } else {
            // Normal Operation (Solid Color based on Mode)
            #if defined(BOARD_MICRO)
                SetPWM2Dat(0x10);
            #elif defined (BOARD_PS2)
                P0 |= 0b01110000;
                switch (FlashSettings->KeyboardMode) {
                    case MODE_PS2: P0 &= ~0b01000000; break;     // Blue
                    case MODE_XT:  P0 &= ~0b00010000; P0 &= ~0b00100000; break; // Orange
                    case MODE_AMSTRAD: P0 &= ~0b00010000; P0 &= ~0b00100000; P0 &= ~0b01000000; break; // White
                }
            #else
                SetPWM1Dat(0x00); SetPWM2Dat(0x00); T3_FIFO_L = 0; T3_FIFO_H = 0;
                switch (FlashSettings->KeyboardMode){
                    case MODE_PS2: T3_FIFO_L = 0xFF; T3_FIFO_H = 0; break;
                    case MODE_XT: SetPWM2Dat(0x10); SetPWM1Dat(0x40); break;
                    case MODE_AMSTRAD: SetPWM1Dat(0x30); SetPWM2Dat(0x20); T3_FIFO_L = 0x3F; T3_FIFO_H = 0; break;
                }
            #endif
        }
	}
}

void mTimer0Interrupt(void) __interrupt(INT_NO_TMR0)
{
	if (OutputsEnabled) {
		switch (FlashSettings->KeyboardMode) {
			case (MODE_PS2): PS2ProcessPort(PORT_KEY); break;
			case (MODE_XT): XTProcessPort(); break;
			case (MODE_AMSTRAD): AmstradProcessPort(); break;
		}
		PS2ProcessPort(PORT_MOUSE);
		static uint8_t repeatDiv = 0;
		if (++repeatDiv == 4) { repeatDiv = 0; RepeatTimer(); }
	}
	static uint8_t msDiv = 0;
	if (++msDiv == 60) { msDiv = 0; EveryMillisecond(); }
}

int main(void)
{
	bool WatchdogReset = 0;
	if (!(PCON & bRST_FLAG0) && (PCON & bRST_FLAG1)){ WatchdogReset = 1; }

	GPIOInit();
	UsbUpdateCounter = 255; while (--UsbUpdateCounter);

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

	TMOD = (TMOD & 0xf0) | 0x02; TH0 = 0xBD;
	TR0 = 1; ET0 = 1; EA = 1; 

    // --- SETUP SERIAL 115200 ---
	CH559UART1Init(20, 1, 1, 115200, 8);

	memset(SendBuffer, 0, 255);
	memset(MouseBuffer, 0, MOUSE_BUFFER_SIZE);

	InitSettings(WatchdogReset);

	WDOG_COUNT = 0x00; SAFE_MOD = 0x55; SAFE_MOD = 0xAA; GLOBAL_CFG |= bWDOG_EN; WDOG_COUNT = 0x00;

	OutputsEnabled = 1;
	DEBUGOUT("ok\n");

	MouseBatCounter = 250;
	KeyBatCounter = 250;

	while (1)
	{
		SoftWatchdog = 0;
		if (MenuActive) Menu_Task();
		ProcessUsbHostPort(); 
        
        HandleSerialKeys();   // <--- SERIAL INPUT
        
		HandleMouse();       
		ProcessKeyboardLed();
		HandleRepeats();
	}
}