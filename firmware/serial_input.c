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
          
bool ParseReport(__xdata INTERFACE *interface, uint32_t len, __xdata uint8_t *report);

__xdata INTERFACE SerialIntf;
__xdata BOOL SerialIntfInitialized = FALSE;

#define HID_REPORT_SIZE 8
__xdata uint8_t SerialBuf[HID_REPORT_SIZE];
__xdata uint8_t SerialBufIdx = 0;

// --- NEW HELPER FUNCTIONS FOR SENDING DATA BACK ---
void UART1_Tx(char c) {
    // Wait for Transmit Buffer to be empty (bLSR_T_FIFO_EMP)
    while ((SER1_LSR & bLSR_T_FIFO_EMP) == 0);
    SER1_THR = c;
}

void UART1_Str(char *s) {
    while(*s) {
        UART1_Tx(*s++);
    }
}

void ResetSerialHID(void) {
    SerialIntfInitialized = FALSE;
    SerialBufIdx = 0;
}

void InitSerialHID(void) {
    if (SerialIntfInitialized) return;
    memset(&SerialIntf, 0, sizeof(INTERFACE));
    SerialIntf.InterfaceProtocol = HID_PROTOCOL_KEYBOARD; 
    SerialIntf.Reports = NULL;
    ParseReportDescriptor(StandardKeyboardDescriptor, 63, &SerialIntf);
    SerialIntfInitialized = TRUE;
}

void HandleSerialKeys(void) {
    uint8_t loopCount = 0;

    if (!SerialIntfInitialized) {
        InitSerialHID();
        return;
    }

    // Check UART1 Data Ready (DB9 Port)
    while ((SER1_LSR & bLSR_DATA_RDY) && (loopCount < 16)) {
        
        uint8_t byte = SER1_RBR; // Read byte
        
        SerialBuf[SerialBufIdx++] = byte;

        // If we have a full packet (8 bytes)
        if (SerialBufIdx >= HID_REPORT_SIZE) {
            
            // --- FEEDBACK TO USER ---
            // This proves the packet was received and aligned correctly
            UART1_Str("KEY RX\r\n");

            // Process the HID data (Convert to PS/2)
            ParseReport(&SerialIntf, 64, SerialBuf);
            
            // Reset buffer
            SerialBufIdx = 0;
        }
        loopCount++;
    }
}