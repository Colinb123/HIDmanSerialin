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
#include "linkedlist.h"       // Defines LinkedList struct
#include "parsedescriptor.h"  // Defines ParseReportDescriptor
#include "data.h"             // Defines StandardKeyboardDescriptor

// Manually declare ParseReport if missing from headers
bool ParseReport(__xdata INTERFACE *interface, uint32_t len, __xdata uint8_t *report);

// Global "Virtual" Interface to track state
__xdata INTERFACE SerialIntf;
__xdata BOOL SerialIntfInitialized = FALSE;

// Buffer to hold incoming serial bytes
#define HID_REPORT_SIZE 8
__xdata uint8_t SerialBuf[HID_REPORT_SIZE];
__xdata uint8_t SerialBufIdx = 0;

// Safety timer to auto-release keys if serial disconnects
__xdata uint16_t LastSerialActivity = 0; 

// Initialize the Virtual Interface
void InitSerialHID(void) {
    if (SerialIntfInitialized) return;

    // Clear memory for the interface
    memset(&SerialIntf, 0, sizeof(INTERFACE));

    // Force Protocol to Keyboard so ParseReport treats it correctly
    SerialIntf.InterfaceProtocol = HID_PROTOCOL_KEYBOARD; 
    
    // Initialize the list to NULL (Empty List)
    // FIX: Removed ListCreate() which does not exist
    SerialIntf.Reports = NULL;

    // Use the descriptor from data.h
    // This tells the parser: Byte 0=Mods, Byte 2-7=Keys
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

        // Reset safety timer on activity
        LastSerialActivity = 0; 

        // If we have a full 8-byte report...
        if (SerialBufIdx >= HID_REPORT_SIZE) {
            // Process it! 
            // 64 = length in bits (8 bytes * 8)
            ParseReport(&SerialIntf, 64, SerialBuf);
            
            // Reset buffer for next packet
            SerialBufIdx = 0;
        }
    }

    // 3. Safety Watchdog (Stuck Key Prevention)
    // Simple timer increment (called approx every loop)
    if (LastSerialActivity < 60000) LastSerialActivity++;

    // If no data for ~500ms (approx tuning) and we aren't at index 0 (mid-packet)
    if (LastSerialActivity > 20000) { 
        // Force an empty report (All Keys Up)
        memset(SerialBuf, 0, HID_REPORT_SIZE);
        ParseReport(&SerialIntf, 64, SerialBuf);
        LastSerialActivity = 0; // Don't spam
        SerialBufIdx = 0;       // Reset sync
    }
}