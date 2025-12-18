#include <stdio.h>
#include <string.h>
#include "type.h"       // Defines UINT8/UINT16 needed by other headers
#include "ch559.h"
#include "uart.h"
#include "ps2.h"
#include "scancode.h"
#include "settings.h"
#include "system.h"     // <--- ADDED: Defines mDelaymS
#include "serial_input.h"

// Helper to select the correct scancode set (PS/2 uses Set 2, XT uses Set 1)
__code uint8_t* GetMakeCode(__code uint8_t* set1, __code uint8_t* set2) {
    if (FlashSettings->KeyboardMode == MODE_PS2) return set2;
    return set1;
}

void HandleSerialKeys(void) {
    // Check if data is waiting in the UART buffer
    // SER1_LSR and bLSR_DATA_RDY are defined in ch559.h
    if (SER1_LSR & bLSR_DATA_RDY) { 
        char cmd = SER1_RBR; // Read the received byte directly from register
        
        __code uint8_t *make = 0;
        __code uint8_t *break_code = 0;

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
            case '1': // F1
                make = GetMakeCode(KEY_SET1_F1_MAKE, KEY_SET2_F1_MAKE);
                break_code = GetMakeCode(KEY_SET1_F1_BREAK, KEY_SET2_F1_BREAK);
                break;
        }

        // If we found a valid key match
        if (make != 0 && break_code != 0) {
            SendKeyboard(make);       // Press the key
            mDelaymS(20);             // Hold it briefly (20ms)
            SendKeyboard(break_code); // Release the key
        }
    }
}