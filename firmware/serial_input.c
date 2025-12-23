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

extern volatile uint8_t LEDDelayMs;

// External parser reference
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
    
    DEBUGOUT("Init Serial HID...\n");
    memset(&SerialIntf, 0, sizeof(INTERFACE));
    
    SerialIntf.InterfaceProtocol = HID_PROTOCOL_KEYBOARD; 
    SerialIntf.InterfaceClass = USB_DEV_CLASS_HID;
    SerialIntf.Reports = NULL;
    
    // This function tries to MALLOC memory for the report structure.
    // If RAM is full, Reports will remain NULL.
    ParseReportDescriptor(StandardKeyboardDescriptor, 63, &SerialIntf);
    
    if (SerialIntf.Reports == NULL) {
        DEBUGOUT("FAIL: Serial malloc failed! Free more RAM.\n");
    } else {
        SerialIntfInitialized = TRUE;
        DEBUGOUT("Serial HID Ready.\n");
    }
}

void HandleSerialKeys(void) {
    uint8_t loopCount = 0;
    uint8_t i;

    if (!SerialIntfInitialized) {
        InitSerialHID();
        return;
    }

    // Process up to 16 bytes per loop
    while ((SER1_LSR & bLSR_DATA_RDY) && (loopCount < 16)) {
        
        uint8_t byte = SER1_RBR; 
        
        SerialBuf[SerialBufIdx++] = byte;

        if (SerialBufIdx >= HID_REPORT_SIZE) {
            
            // Debug Echo to verify data arrival
            DEBUGOUT("S0 L8- ");
            for (i = 0; i < HID_REPORT_SIZE; i++) {
                DEBUGOUT("%02X ", SerialBuf[i]);
            }
            DEBUGOUT("\n");

            LEDDelayMs = 10; 

            // Only parse if we have memory allocated
            if (SerialIntf.Reports != NULL) {
                 ParseReport(&SerialIntf, 64, SerialBuf);
            } else {
                 DEBUGOUT("ERR: No Report Mem\n");
                 SerialIntfInitialized = FALSE; // Try to re-init next time
            }
            
            SerialBufIdx = 0;
        }
        loopCount++;
    }
}