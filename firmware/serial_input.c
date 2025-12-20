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

__xdata uint16_t LastSerialActivity = 0; 
__xdata BOOL SerialTimedOut = TRUE; 

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

    while ((SER1_LSR & bLSR_DATA_RDY) && (loopCount < 16)) {
        uint8_t byte = SER1_RBR;
        SerialBuf[SerialBufIdx++] = byte;
        LastSerialActivity = 0; 
        SerialTimedOut = FALSE;

        if (SerialBufIdx >= HID_REPORT_SIZE) {
            ParseReport(&SerialIntf, 64, SerialBuf);
            SerialBufIdx = 0;
        }
        loopCount++;
    }

    if (!SerialTimedOut) {
        if (LastSerialActivity < 60000) {
            LastSerialActivity++;
        } else {
            SerialTimedOut = TRUE;
            memset(SerialBuf, 0, HID_REPORT_SIZE);
            ParseReport(&SerialIntf, 64, SerialBuf);
            SerialBufIdx = 0;       
        }
    }
}