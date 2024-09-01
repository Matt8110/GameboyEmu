#include "bus.h"
#include <stdlib.h>
#include <stdio.h>
#include <raylib.h>

uint32_t divRegTime = 0;

Bus_t* bus_create(Rom_t* rom)
{
    Bus_t* bus = malloc(sizeof(Bus_t));

    //Clear bus
    for (int i = 0; i < 0xFFFF; i++)
    {
        bus->data[i] = 0;
    }

    bus->bankROM = malloc(sizeof(uint8_t) * rom->bankCount * 0x4000);

    if (rom != NULL)
    {
        bus->rom = rom;

        for (int i = 0; i < 0x8000; i++)
        {
            bus->data[i] = rom->data[i];
        }

        for (int i = 0; i < rom->bankCount * 0x4000; i++)
        {
            bus->bankROM[i] = rom->data[i + 0x4000];
        }
    }

    bus->bankRomNum = 0;
    bus->bankRamNum = 0;

    printf("%02x\n", bus->data[0]);

    return bus;
}

void bus_destroy(Bus_t* bus)
{
    free(bus->bankROM);
    free(bus);
}

void bus_switchRomBank(Bus_t* bus, uint8_t bank)
{
    uint8_t realBank = bank;

    if (realBank == 0 || realBank == 0x20 || realBank == 0x40 || realBank == 0x60)
    {
        realBank++;
    }

    bus->bankRomNum = realBank - 1;
}

void bus_switchRamBank(Bus_t* bus, uint8_t bank)
{
    bus->bankRamNum = bank;
}

void bus_write(Bus_t* bus, uint16_t addr, uint8_t byte)
{
    if (addr >= 0x0000 && addr <= 0xFFFF)
    {
        if (addr < 0x8000)
        {
            if (addr > 0x1FFF && addr < 0x4000)
            {
                bus_switchRomBank(bus, (byte & 0x1F));

            }
            else if (addr >= 0x4000 && addr <= 0x5FFF)
            {
                bus_switchRamBank(bus, (byte & 3) % bus->rom->bankCount);
            }
        }
        else if (addr > 0x9FFF && addr < 0xC000)
        {
            bus->bankRAM[addr - 0xA000] = byte;
        }
        else if (addr == 0xFF04)
        {
            bus->data[0xFF04] = 0;
        }
        else if (addr == 0xFF46)
        {
            uint16_t address = byte << 8;
            for (int i = 0; i < 160; i++)
            {
                bus->data[0xFE00 + i] = bus_read(bus, address + i);
            }
        }

        else if (addr == 0xFF00) //Joypad
        {
            uint8_t joySwitch = byte >> 4;

            uint8_t joypad = byte | 0xF;

            if (joySwitch == 1)
            {
                if (IsKeyDown(KEY_X)) joypad &= ~(1<<0);//A
                if (IsKeyDown(KEY_C)) joypad &= ~(1<<1);//B
                if (IsKeyDown(KEY_L)) joypad &= ~(1<<2);//Select
                if (IsKeyDown(KEY_ENTER)) joypad &= ~(1<<3);//Start
            }

            else if (joySwitch == 2)
            {
                if (IsKeyDown(KEY_RIGHT)) joypad &= ~(1<<0);//Right
                if (IsKeyDown(KEY_LEFT)) joypad &= ~(1<<1);//Left
                if (IsKeyDown(KEY_UP)) joypad &= ~(1<<2);//Up
                if (IsKeyDown(KEY_DOWN)) joypad &= ~(1<<3);//Down
            }

           bus->data[0xFF00] = joypad;
        }
        else
        {
            bus->data[addr] = byte;
        }

        if(addr == 0xFF02 && bus->data[0xFF02] == 0x81) 
        { 
            printf("%c", bus->data[0xFF01]); 
            bus->data[0xFF02] = 0;
        }


    }
}

uint8_t bus_read(Bus_t* bus, uint16_t addr)
{
    if (addr >= 0x0000 && addr <= 0xFFFF)
    {
        //Echo RAM
        if (addr >= 0xE000 && addr <= 0xFDFF)
        {
            return bus->data[addr - 0x2000];
        }

        else if (addr > 0x3FFF && addr < 0x8000)
        {
            return bus->bankROM[(bus->bankRomNum * 0x4000) + (addr - 0x4000)];
        }

        else if (addr > 0x9FFF && addr < 0xC000)
        {
            return bus->bankRAM[addr - 0xA000];
        }

        return bus->data[addr];
    }

    return 0x0;
}

uint16_t divCounter = 0;

void bus_updateDivTimer(Bus_t* bus)
{
    bus->data[0xFF04]++;
}