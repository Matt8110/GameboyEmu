#include "ppu.h"
#include <stdlib.h>
#include <stdio.h>
#include <raylib.h>

#define SET_LCD_Y_COORD_REG(a) bus_write(ppu->bus, 0xFF44, a);

Ppu_t* ppu_create(Bus_t* bus)
{
    Ppu_t* ppu = malloc(sizeof(Ppu_t));
    ppu->bus = bus;
    ppu->currentLine = 0;
    ppu->clocksElapsed = 0;

    ppu->screenBufferLocked = 0;

    for (int y = 0; y < 160; y++)
    for (int x = 0; x < 144; x++)
    {
        ppu->screenBuffer[x][y] = 0;
    }

    return ppu;
}

void ppu_clock(Ppu_t* ppu)
{
    LCDControlReg_t* lcdControl = ((LCDControlReg_t*)&ppu->bus->data[0xFF40]);
    LCDStatReg_t* lcdStat = ((LCDStatReg_t*)&ppu->bus->data[0xFF41]);

    ppu->clocksElapsed++;

    switch(lcdStat->ppuMode)
    {
        case MODE_OAMSCAN:

            if (ppu->clocksElapsed >= 160)
            {

                lcdStat->ppuMode = MODE_DRAWING;

                    //Sprites
                    if (lcdControl->sprEnable)
                    {
                        
                        for (int i = 0; i < 40; i++)
                        {
                            uint8_t yPos = bus_read(ppu->bus, 0xFE00 + (i * 4));
                            uint8_t xPos = bus_read(ppu->bus, 0xFE00 + (i * 4) + 1);
                            uint8_t tileIndex = bus_read(ppu->bus, 0xFE00 + (i * 4) + 2);
                            uint8_t attributes = bus_read(ppu->bus, 0xFE00 + (i * 4) + 3);
                            uint8_t xFlip = (attributes >> 5) & 1;
                            uint8_t yFlip = (attributes >> 6) & 1;

                            uint8_t palette = (attributes >> 4) & 1;

                            if (xPos > 0 || yPos)
                            {
                                uint16_t yBase = yPos - 16;

                                if (ppu->currentLine >= yBase && ppu->currentLine < yBase + (lcdControl->sprSize ? 16 : 8))
                                {
                                    uint8_t y = ppu->currentLine - yBase;
                                    uint8_t byteA = bus_read(ppu->bus, 0x8000 + ((uint8_t)tileIndex * 16) + y * 2);
                                    uint8_t byteB = bus_read(ppu->bus, 0x8000 + ((uint8_t)tileIndex * 16) + y * 2 + 1);

                                    for (int x = 0; x < 8; x++)
                                    {
                                        uint8_t bitA = (byteA >> (xFlip ? x : 7 - x)) & 1;
                                        uint8_t bitB = (byteB >> (xFlip ? x : 7 - x)) & 1;

                                        uint8_t color = 0;
                                        if (bitA == 0 && bitB == 0) color = palette == 1 ? 0 : 4;
                                        if (bitA == 0 && bitB == 1) color = palette == 1 ? 1 : 5;
                                        if (bitA == 1 && bitB == 0) color = palette == 1 ? 2 : 6;
                                        if (bitA == 1 && bitB == 1) color = palette == 1 ? 3 : 7;

                                        uint16_t X = xPos + x - 8;
                                        uint16_t Y = yFlip ? yPos - (y - 16) - (lcdControl->sprSize ? 16 : 8) : yPos + y - 16;

                                        if (color != 0 && color != 4 && X >= 0 && X < 160 && Y >= 0 && Y < 144)
                                        {
                                            ppu->backBuffer[X][Y] = color;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (lcdControl->bgWinEnable)
                    {
                        //Window
                        if (lcdControl->winEnable)
                        {
                            for (int i = 0; i < 32; i++)
                            {
                                uint8_t winX = bus_read(ppu->bus, 0xFF4B);
                                uint8_t winY = bus_read(ppu->bus, 0xFF4A);

                                //Fetch tile ID
                                uint16_t tileIDY = (ppu->currentLine / 8);
                                uint16_t tileIDX = i;
                                uint16_t tileIDAddr = tileIDX + ((tileIDY) * 32);
                                uint16_t tileID = bus_read(ppu->bus, (lcdControl->winTileMapSelect ? 0x9C00 : 0x9800) + tileIDAddr);

                                uint8_t tileY = ppu->currentLine % 8;

                                uint8_t tileByteA = bus_read(ppu->bus, lcdControl->tileDataSelect ? 0x8000 + ((uint8_t)tileID * 16) + tileY * 2 : 0x9000 + ((int8_t)tileID * 16) + tileY * 2);
                                uint8_t tileByteB = bus_read(ppu->bus, lcdControl->tileDataSelect ? 0x8000 + ((uint8_t)tileID * 16) + tileY * 2 + 1 : 0x9000 + ((int8_t)tileID * 16) + tileY * 2 + 1);

                                uint8_t paletteSelect = bus_read(ppu->bus, 0xFF47);
                                uint8_t pal0 = paletteSelect >> 6;
                                uint8_t pal1 = (paletteSelect >> 4) & 3;
                                uint8_t pal2 = (paletteSelect >> 2) & 3;
                                uint8_t pal3 = paletteSelect & 3;

                                for (int j = 0; j < 8; j++)
                                {
                                    uint8_t bitA = (tileByteA >> (7 - j)) & 1;
                                    uint8_t bitB = (tileByteB >> (7 - j)) & 1;

                                    uint8_t color = 0;


                                    if (bitA == 0 && bitB == 0) color = pal3;//Pal0 = black
                                    if (bitA == 0 && bitB == 1) color = pal1;//Pal1 = white
                                    if (bitA == 1 && bitB == 0) color = pal2;//Pal2 = gray
                                    if (bitA == 1 && bitB == 1) color = pal0;//Pal3 = dark gray
                                    

                                    uint16_t X = (i*8 + j) + winX - 7;
                                    uint16_t Y = ppu->currentLine + winY;

                                    if (X >= 0 && X < 160 && Y >= 0 && Y < 144)
                                    {
                                        ppu->backBuffer[X][Y] = color;
                                    }
                                }
                            }
                        }


                        //Background
                        for (int i = 0; i < 32; i++)
                        {
                                uint16_t scrollX = bus_read(ppu->bus, 0xFF43);
                                uint16_t scrollY = bus_read(ppu->bus, 0xFF42);

                                //Fetch tile ID
                                //uint16_t tileIDY = ((ppu->currentLine) / 8);
                                //uint16_t tileIDX = (i);
                                uint16_t tileIDY = ((ppu->currentLine / 8) + scrollY / 8 & 0x1F);
                                uint16_t tileIDX = (i + scrollX / 8) & 0x1F;
                                uint16_t tileIDAddr = tileIDX + ((tileIDY) * 32);
                                uint16_t tileID = bus_read(ppu->bus, (lcdControl->bgTileMapSelect ? 0x9C00 : 0x9800) + tileIDAddr);

                                uint8_t tileY = ppu->currentLine % 8;

                                uint8_t tileByteA = bus_read(ppu->bus, lcdControl->tileDataSelect ? 0x8000 + ((uint8_t)tileID * 16) + tileY * 2 : 0x9000 + ((int8_t)tileID * 16) + tileY * 2);
                                uint8_t tileByteB = bus_read(ppu->bus, lcdControl->tileDataSelect ? 0x8000 + ((uint8_t)tileID * 16) + tileY * 2 + 1 : 0x9000 + ((int8_t)tileID * 16) + tileY * 2 + 1);

                                uint8_t paletteSelect = bus_read(ppu->bus, 0xFF47);
                                uint8_t pal0 = paletteSelect >> 6;
                                uint8_t pal1 = (paletteSelect >> 4) & 3;
                                uint8_t pal2 = (paletteSelect >> 2) & 3;
                                uint8_t pal3 = paletteSelect & 3;

                                for (int j = 0; j < 8; j++)
                                {
                                    uint8_t bitA = (tileByteA >> (7 - j)) & 1;
                                    uint8_t bitB = (tileByteB >> (7 - j)) & 1;

                                    uint8_t color = 0;


                                    if (bitA == 0 && bitB == 0) color = pal3;//Pal0 = black
                                    if (bitA == 0 && bitB == 1) color = pal1;//Pal1 = white
                                    if (bitA == 1 && bitB == 0) color = pal2;//Pal2 = gray
                                    if (bitA == 1 && bitB == 1) color = pal0;//Pal3 = dark gray
                                    

                                    uint16_t X = (i*8 + j) - scrollX % 8;
                                    uint16_t Y = ppu->currentLine - scrollY % 8;

                                    if (X >= 0 && X < 160 && Y >= 0 && Y < 144 && ppu->backBuffer[X][Y] == 255)
                                    {
                                        ppu->backBuffer[X][Y] = color;
                                    }
                                }
                                
                            }
                    }

            }
        break;

        case MODE_DRAWING:

            if (ppu->clocksElapsed >= 168)
            {
                lcdStat->ppuMode = MODE_HBLANK;

                if (lcdStat->mode0IntEnable)
                {
                    //Request LCD interrupt
                    bus_write(ppu->bus, 0xFF0F, bus_read(ppu->bus, 0xFF0F) | (1 << 1));
                    //lcdStat->mode0IntEnable = 0;
                }

                //Coincidence interrupt
                lcdStat->coincidence = ppu->currentLine == bus_read(ppu->bus, 0xFF45);
                if (lcdStat->lyIntEnable == 1)
                {
                    if (lcdStat->coincidence)
                    {
                        //Request LCD interrupt
                        bus_write(ppu->bus, 0xFF0F, bus_read(ppu->bus, 0xFF0F) | (1 << 1));
                        printf("Coincidence!\n");
                        lcdStat->coincidence = 0;
                    }
                }

                ppu->currentLine++;
                bus_write(ppu->bus, 0xFF44, ppu->currentLine);
            }
                
            
        break;

        case MODE_HBLANK:
            if (ppu->currentLine > 144)
            {
                if (lcdStat->mode1IntEnable)
                {
                    //Request LCD interrupt
                    bus_write(ppu->bus, 0xFF0F, bus_read(ppu->bus, 0xFF0F) | (1 << 1));
                }
                //Request VBlank interrupt
                bus_write(ppu->bus, 0xFF0F, bus_read(ppu->bus, 0xFF0F) | 1);
                lcdStat->ppuMode = MODE_VBLANK;
                ppu->currentLine = 0x94;
                ppu->clocksElapsed = 0;

                for (int y = 0; y < 144; y++)
                {
                    for (int x = 0; x < 160; x++)
                    {
                        ppu->screenBuffer[x][y] = ppu->backBuffer[x][y];
                    }
                }

                ppu_clearScreen(ppu);
            }
            else if (ppu->clocksElapsed >= 456)
            {
                ppu->clocksElapsed = 0;
                lcdStat->ppuMode = MODE_OAMSCAN;
                if (lcdStat->mode2IntEnable)
                {
                    //Request LCD interrupt
                    bus_write(ppu->bus, 0xFF0F, bus_read(ppu->bus, 0xFF0F) | (1 << 1));
                }
            }
        break;

        case MODE_VBLANK:
            if (ppu->clocksElapsed >= 4560)
            {
                lcdStat->ppuMode = MODE_OAMSCAN;

                if (lcdStat->mode2IntEnable)
                {
                    //Request LCD interrupt
                    bus_write(ppu->bus, 0xFF0F, bus_read(ppu->bus, 0xFF0F) | (1 << 1));
                }
                //ppu->currentLine
                ppu->clocksElapsed = 0;
                ppu->currentLine = 0;

            }
        break;
    }
    
}

void ppu_clearScreen(Ppu_t* ppu)
{
    for (int y = 0; y < 144; y++)
    {
        for (int x = 0; x < 160; x++)
        {
            ppu->backBuffer[x][y] = 255;
        }
    }
}