#include "rom.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <raylib.h>

Rom_t* rom_load(const char* romFilename)
{
    FILE* romFile = fopen(romFilename, "rb");
    fseek(romFile, 0, SEEK_END);
    int size = ftell(romFile);
    fseek(romFile, 0, SEEK_SET);

    Rom_t* rom = malloc(sizeof(Rom_t));
    rom->size = size;
    rom->data = malloc(size);
    fread(rom->data, 1, size, romFile);

    strncpy(rom->name, &rom->data[0x134], 0xF);

    uint8_t mbcType = rom->data[0x147];

    switch(mbcType)
    {
        case 0x00: break;
        case 0x01: printf("MBC1\n"); break;
        case 0x02: printf("MBC1 + RAM\n"); break;
        case 0x03: printf("MBC1 + RAM + BATTERY\n"); break;
        default: printf("Unknown MCB\n");
    }

    uint8_t romSize = rom->data[0x148];
    switch(romSize)
    {
        case 0x00: rom->bankCount = 1; break;
        case 0x01: rom->bankCount = 3; break;
        case 0x02: rom->bankCount = 7; break;
        case 0x03: rom->bankCount = 15; break;
        case 0x04: rom->bankCount = 31; break;
        case 0x05: rom->bankCount = 63; break;
        case 0x06: rom->bankCount = 127; break;
        case 0x07: rom->bankCount = 255; break;
        case 0x08: rom->bankCount = 511; break;
        case 0x52: rom->bankCount = 71; break;
        case 0x53: rom->bankCount = 79; break;
        case 0x54: rom->bankCount = 95; break;
    };

    //printf("%d\n", rom->bankCount);

    uint8_t ramSize = rom->data[0x148];
    switch(romSize)
    {
        case 0x00: rom->ramBankCount = 0; break;
        case 0x02: rom->ramBankCount = 1; break;
        case 0x03: rom->ramBankCount = 4; break;
        case 0x04: rom->ramBankCount = 16; break;
        case 0x05: rom->ramBankCount = 8; break;
    };

    strncpy(rom->name, &rom->data[0x134], 0xF);
    rom->name[0xF] = 0;

    SetWindowTitle(rom->name);

    fclose(romFile);

    //romFile = fopen("roms/boot.gb", "rb");

    //fread(rom->data, 1, 256, romFile);

    //fclose(romFile);

   return rom; 
}