#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <pthread.h>
#include "cpu.h"
#include "bus.h"
#include "rom.h"
#include "ppu.h"

#define SCREEN_MULTIPLIER 8

#define CPU_SPEED 4194304/60

Z80_t* cpu;
Ppu_t* ppu;
Bus_t* bus;
pthread_t cpuThread;
int running = 1;
int breakPoint = 0xc6EC;
int paused = 0;

void* cpuThreadFunc(void* args)
{
    uint32_t clocks;
    double lastTimeClocks = GetTime();
    double lastTime = GetTime();

    uint8_t clockSkipPPU = 0;

    while(running)
    {
          if (clocks >= CPU_SPEED && GetTime() - lastTime >= 1.0 / 60.0)
         {
            lastTime = GetTime();
            //printf("Clock speed achieved: %.02fMHz\n", clocks * 60.0f / 1000000.0f);
            clocks = 0;
         }

         while(clocks <= CPU_SPEED && !paused)
         {
             cpu_clock(cpu);
             ppu_clock(ppu);
             clocks++;
         }
    }
}

int main()
{
    InitWindow(160 * SCREEN_MULTIPLIER, 144 * SCREEN_MULTIPLIER, "Gameboy Emulator");
    SetTargetFPS(60);
    //uint8_t frontBuffer[256][256];

    Rom_t* rom = rom_load("roms/zelda.gb");
 
    bus = bus_create(rom);
    cpu = cpu_create(bus);
    ppu = ppu_create(bus);

    pthread_create(&cpuThread, NULL, cpuThreadFunc, NULL);

    uint8_t screenBuff[160][144];

    while (!WindowShouldClose())
    {

        /* Debugging
        if (IsKeyPressed(KEY_SPACE))
        {
            cpu_clock(cpu);
        }

        if (IsKeyDown(KEY_Q))
        {
            paused = 1;
        }
        if (IsKeyDown(KEY_R))
        {
            paused = 0;
        }*/

        BeginDrawing();
        ClearBackground(RAYWHITE);

        Color colorA = (Color){0xDF, 0xDF, 0xDF, 255};
        Color colorB = (Color){0x9F, 0x9F, 0x9F, 255};
        Color colorC = (Color){0x5F, 0x5F, 0x5F, 255};
        Color colorD = (Color){0x10, 0x10, 0x10, 255};

        for (int y = 0; y < 144; y++)
        {
            for (int x = 0; x < 160; x++)
            {
                uint8_t color = ppu->screenBuffer[x][y];

                Color realColor;

                if (color == 0) realColor = colorA;
                if (color == 1) realColor = colorB;
                if (color == 2) realColor = colorC;
                if (color == 3) realColor = colorD;

                if (color == 4) realColor = colorC;
                if (color == 5) realColor = colorB;
                if (color == 6) realColor = colorD;
                if (color == 7) realColor = colorA;

                DrawRectangle(x*SCREEN_MULTIPLIER, y*SCREEN_MULTIPLIER, SCREEN_MULTIPLIER, SCREEN_MULTIPLIER, realColor);
            }
        }

        //For debugging
        /*for (int i = 0; i < 50; i++)
        {
            char str[64];
            sprintf(str, "0x%04x : 0x%02x\n", cpu->pc - 26 + i, bus_read(bus, cpu->pc - 26 + i));
            DrawText(str, 16, 16 + i * 16, 20, i == 26 ? RED : GREEN);
        }

        char str[128];
        sprintf(str, "BC: %04x DE: %04x HL: %04x A: %02x z: %d n: %d h: %d c: %d AF: %04x SP: %04x\n", cpu->regs.BC, cpu->regs.DE, cpu->regs.HL, cpu->regs.A, cpu->regs.z, cpu->regs.n, cpu->regs.h, cpu->regs.c, cpu->regs.AF, cpu->sp);
        DrawText(str, 170, 16 , 20, GREEN);*/

        EndDrawing();
    }

    running = false;
    pthread_join(cpuThread, NULL);

    cpu_destroy(cpu);
    bus_destroy(bus);

    CloseWindow();

    return 0;
}