#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "ch559.h"
#include "system.h"
#include "defs.h"
#include "usbdef.h"
#include "usbhost.h"
#include "andyalloc.h"

// --- AGGRESSIVE RAM FIX ---
// Reduced to 0x0C00 (3KB) to guarantee space for Serial/Stack
#ifdef MEMPOOLMAXSIZE
#undef MEMPOOLMAXSIZE
#endif
#define MEMPOOLMAXSIZE 0x0C00 

__xdata uint8_t MemPool[MEMPOOLMAXSIZE];
__xdata uint8_t *MemPoolPtr = MemPool;
__xdata uint8_t *tmp;

uint16_t MemoryUsed(void) { return MemPoolPtr - MemPool; }
uint16_t MemoryFree(void) { return MEMPOOLMAXSIZE - MemoryUsed(); }

void __xdata *andyalloc(size_t size) {
    if (MemoryFree() <= size) {
        DEBUGOUT("Memory Exhausted");
        ET0 = 0;
        while (1);
    }
    tmp = MemPoolPtr;
    MemPoolPtr += size;
    return tmp;
}

void andyclearmem(void) { MemPoolPtr = MemPool; }
void printhexval(uint8_t x){ printf("sp %X\n", x); }

void printstackpointer(void) {
    uint8_t dumdum = 0;
    printhexval((uint8_t)((uint16_t)(void*)&dumdum));
}