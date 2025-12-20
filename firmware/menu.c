#include <stdint.h>
#include <stdbool.h>
#include "menu.h"

// --- DEFINITIONS REQUIRED BY SYSTEM/LINKER ---
// These must exist even if the menu is disabled
uint16_t MenuRateLimit = 0;
__xdata bool MenuActive = 0;
__xdata bool KeyboardDebugOutput = 0; // Fixes "Undefined Global" linker error
__xdata char SendBuffer[32]; 
char *currchar;

// --- DUMMY FUNCTIONS ---
void Menu_Task(void) { }
void inputProcess(void) { }
void Sendbuffer_Task(void) { }
void SendKeyboardBuffer(void) { }
void Menu_Press_Key(uint8_t key) { (void)key; }