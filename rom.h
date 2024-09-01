#ifndef _ROM_H_
#define _ROM_H_

#include <stdint.h>

typedef struct
{
    char name[64];
    uint8_t* data;
    uint32_t size;
    uint16_t bankCount;
    uint8_t ramBankCount;
}Rom_t;

Rom_t* rom_load(const char* romFilename);

#endif