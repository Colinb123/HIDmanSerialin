#ifndef __SERIAL_INPUT_H__
#define __SERIAL_INPUT_H__

void HandleSerialKeys(void);
void InitSerialHID(void);
void ResetSerialHID(void); // <--- THIS IS REQUIRED

// Debug helpers
void UART1_Tx(char c);
void UART1_Str(char *s);
void UART1_PrintHex(uint8_t val);

#endif