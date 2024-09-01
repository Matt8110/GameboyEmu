#ifndef _PPU_H_
#define _PPU_H_

#include <stdint.h>
#include "bus.h"

enum
{
    MODE_HBLANK = 0,
    MODE_VBLANK,
    MODE_OAMSCAN,
    MODE_DRAWING
};

typedef struct
{   
    union
    {
        uint8_t reg;
        struct
        {
            // uint8_t enable : 1;
            // uint8_t winTileMapSelect : 1;
            // uint8_t winEnable : 1;
            // uint8_t tileDataSelect : 1;
            // uint8_t bgTileMapSelect : 1;
            // uint8_t sprSize : 1;
            // uint8_t sprEnable : 1;
            // uint8_t bgWinEnable : 1;

            uint8_t bgWinEnable : 1;
            uint8_t sprEnable : 1;
            uint8_t sprSize : 1;
            uint8_t bgTileMapSelect : 1;
            uint8_t tileDataSelect : 1;
            uint8_t winEnable : 1;
            uint8_t winTileMapSelect : 1;
            uint8_t enable : 1;
        };
    };
}LCDControlReg_t;

typedef struct
{   
    union
    {
        uint8_t reg;
        struct
        {
            // uint8_t unused : 1;
            // uint8_t lyIntEnable : 1;
            // uint8_t mode2IntEnable : 1;
            // uint8_t mode1IntEnable : 1;
            // uint8_t mode0IntEnable : 1;
            // uint8_t coincidence : 1;
            // uint8_t ppuMode : 2;

            uint8_t ppuMode : 2;
            uint8_t coincidence : 1;
            uint8_t mode0IntEnable : 1;
            uint8_t mode1IntEnable : 1;
            uint8_t mode2IntEnable : 1;
            uint8_t lyIntEnable : 1;
            uint8_t unused : 1;
        };
    };
}LCDStatReg_t;

typedef struct
{
    uint8_t y;
    uint8_t x;
    uint8_t tileID;
    uint8_t flags;
}OAMEntry_t;

typedef struct
{
    Bus_t* bus;
    uint16_t currentLine;
    uint16_t clocksElapsed;
    uint8_t screenBuffer[160][144];
    uint8_t backBuffer[160][144];
    uint8_t screenBufferLocked;
}Ppu_t;

Ppu_t* ppu_create(Bus_t* bus);
void ppu_clock(Ppu_t* ppu);
void ppu_clearScreen(Ppu_t* ppu);

#endif