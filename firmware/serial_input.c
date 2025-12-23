#include <stdio.h>
#include <string.h>
#include "type.h"
#include "ch559.h"
#include "uart.h"
#include "ps2.h"
#include "scancode.h"
#include "settings.h"
#include "system.h"
#include "usbhost.h"
#include "usbdef.h"
#include "preset.h"
#include "andyalloc.h"
#include "serial_input.h"
#include "linkedlist.h"       
#include "parsedescriptor.h"  
#include "data.h"             

// External reference to the main loop's LED timer
extern volatile uint8_t LEDDelayMs;

// External reference to the main parser function
bool ParseReport(__xdata INTERFACE *interface, uint32_t len, __xdata uint8_t *report);

__xdata INTERFACE SerialIntf;
__xdata BOOL SerialIntfInitialized = FALSE;

#define HID_REPORT_SIZE 8
__xdata uint8_t SerialBuf[HID_REPORT_SIZE];
__xdata uint8_t SerialBufIdx = 0;

void ResetSerialHID(void) {
    SerialIntfInitialized = FALSE;
    SerialBufIdx = 0;
}

void InitSerialHID(void) {
    if (SerialIntfInitialized) return;
    memset(&SerialIntf, 0, sizeof(INTERFACE));
    
    // Fake a Standard Keyboard Interface
    SerialIntf.InterfaceProtocol = HID_PROTOCOL_KEYBOARD; 
    SerialIntf.InterfaceClass = USB_DEV_CLASS_HID;
    SerialIntf.Reports = NULL;
    
    // Use the standard descriptor so the parser knows what to do with the 8 bytes
    ParseReportDescriptor(StandardKeyboardDescriptor, 63, &SerialIntf);
    SerialIntfInitialized = TRUE;
}

void HandleSerialKeys(void) {
    uint8_t loopCount = 0;

    if (!SerialIntfInitialized) {
        InitSerialHID();
        return;
    }

    // Check UART1 Data Ready (DB9 RX Pin)
    // We limit loopCount to prevent getting stuck here if data floods in
    while ((SER1_LSR & bLSR_DATA_RDY) && (loopCount < 16)) {
        
        uint8_t byte = SER1_RBR; // Read one byte from hardware
        
        SerialBuf[SerialBufIdx++] = byte;

        // If we have a full packet (8 bytes)
        if (SerialBufIdx >= HID_REPORT_SIZE) {
            
            // 1. FLASH THE LED (Visual Feedback)
            // This restores the "Blue Flash" behavior you wanted
            LEDDelayMs = 10; 

            // 2. PROCESS THE HID DATA
            // This treats the 8 bytes exactly like a USB Keyboard Report
            ParseReport(&SerialIntf, 64, SerialBuf);
            
            // 3. RESET
            SerialBufIdx = 0;
        }
        loopCount++;
    }
}