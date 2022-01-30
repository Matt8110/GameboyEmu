#include <iostream>
#include "Window.h"
#include "BUS.h"
#include "CPU.h"
#include "Interrupts.h"

int main()
{
    Window* window = Window::get();
    window->init(1280, 720, "GB Emulator");

    BUS bus;
    Interrupts interrupts(&bus);
    CPU cpu(&bus, &interrupts);

    bus.loadRomIntoMemory("/home/matt/Desktop/Tetris.gb");

    while (window->isOpen())
    {
        cpu.run();

        window->update();
    }

    return 0;
}