#include <stdint.h>
#include <stdbool.h>
#include "menu.h"

// Globals needed by linker
uint16_t MenuRateLimit = 0;
__xdata bool MenuActive = 0;
__xdata char SendBuffer[32]; 
char *currchar;

// Empty functions
void Menu_Task(void) { }
void inputProcess(void) { }
void Sendbuffer_Task(void) { }
void SendKeyboardBuffer(void) { }
void Menu_Press_Key(uint8_t key) { (void)key; }