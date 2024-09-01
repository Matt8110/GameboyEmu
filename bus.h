#ifndef _BUS_H_
#define _BUS_H_

#include <stdint.h>
#include "rom.h"

typedef struct
{
    uint8_t data[0x10000];
    uint8_t bankRAM[0x8000];
    uint8_t bankRamNum;
    uint8_t* bankROM;
    uint16_t bankRomNum;
    Rom_t* rom;
}Bus_t;

Bus_t* bus_create(Rom_t* rom);
void bus_destroy(Bus_t* bus);
void bus_write(Bus_t* bus, uint16_t addr, uint8_t byte);
uint8_t bus_read(Bus_t* bus, uint16_t addr);
void bus_updateDivTimer(Bus_t* bus);

void bus_switchRomBank(Bus_t* bus, uint8_t bank);
void bus_switchRamBank(Bus_t* bus, uint8_t bank);

#endif