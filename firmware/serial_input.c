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
#include "processreport.h"
#include "preset.h"
#include "andyalloc.h"
#include "serial_input.h"
#include "linkedlist.h"       
#include "parsedescriptor.h"  
#include "data.h"             

// Manually declare ParseReport if missing from headers
bool ParseReport(__xdata INTERFACE *interface, uint32_t len, __xdata uint8_t *report);

// Global "Virtual" Interface to track state
__xdata INTERFACE SerialIntf;
__xdata BOOL SerialIntfInitialized = FALSE;

// Buffer to hold incoming serial bytes
#define HID_REPORT_SIZE 8
__xdata uint8_t SerialBuf[HID_REPORT_SIZE];
__xdata uint8_t SerialBufIdx = 0;

// Safety timer variables
__xdata uint16_t LastSerialActivity = 0; 
__xdata BOOL SerialTimedOut = TRUE; // Start timed out so we don't spam on boot

// Initialize the Virtual Interface
void InitSerialHID(void) {
    if (SerialIntfInitialized) return;

    memset(&SerialIntf, 0, sizeof(INTERFACE));
    SerialIntf.InterfaceProtocol = HID_PROTOCOL_KEYBOARD; 
    SerialIntf.Reports = NULL;

    ParseReportDescriptor(StandardKeyboardDescriptor, 63, &SerialIntf);

    SerialIntfInitialized = TRUE;
}

void HandleSerialKeys(void) {
    // 1. One-time Initialization
    if (!SerialIntfInitialized) {
        InitSerialHID();
        return;
    }

    // 2. Read Serial Data
    while (SER1_LSR & bLSR_DATA_RDY) {
        uint8_t byte = SER1_RBR;
        
        SerialBuf[SerialBufIdx++] = byte;

        // Reset safety state because we received data!
        LastSerialActivity = 0; 
        SerialTimedOut = FALSE;

        // If we have a full 8-byte report...
        if (SerialBufIdx >= HID_REPORT_SIZE) {
            // Process it! 
            ParseReport(&SerialIntf, 64, SerialBuf);
            SerialBufIdx = 0;
        }
    }

    // 3. Safety Watchdog (Stuck Key Prevention)
    // Only count up if we haven't timed out yet
    if (!SerialTimedOut) {
        if (LastSerialActivity < 60000) {
            LastSerialActivity++;
        } else {
            // Timeout Reached!
            // 1. Mark as timed out so we don't do this again until new data comes
            SerialTimedOut = TRUE;
            
            // 2. Force an empty report (All Keys Up) ONCE
            memset(SerialBuf, 0, HID_REPORT_SIZE);
            ParseReport(&SerialIntf, 64, SerialBuf);
            
            // 3. Reset index to re-sync framing
            SerialBufIdx = 0;       
        }
    }
}