#include "Interrupts.h"

Interrupts::Interrupts(BUS* bus)
{
    this->bus = bus;
}

u16 Interrupts::handleInterrupts()
{
    //if (IME)
    {
        if (readIntFlag(F_VBLANK) && readEnableFlag(F_VBLANK))
        {
            setIntFlag(F_VBLANK, 0);
            IME = 0;
            return 0x40;
        }
        else if (readIntFlag(F_LCD) && readEnableFlag(F_LCD))
        {
            setIntFlag(F_LCD, 0);
            IME = 0;
            return 0x48;
        }
        else if (readIntFlag(F_TIMER) && readEnableFlag(F_TIMER))
        {
            setIntFlag(F_TIMER, 0);
            IME = 0;
            return 0x50;
        }
        else if (readIntFlag(F_SERIAL) && readEnableFlag(F_SERIAL))
        {
            setIntFlag(F_SERIAL, 0);
            IME = 0;
            return 0x58;
        }
        else if (readIntFlag(F_JOYPAD) && readEnableFlag(F_JOYPAD))
        {
            setIntFlag(F_JOYPAD, 0);
            IME = 0;
            return 0x60;
        }
    }

    return 0;
}


u8 Interrupts::readIntFlag(u8 flag)
{
    return (bus->readByte(FLAGS) >> flag) & 0x1;
}

void Interrupts::setIntFlag(u8 flag, u8 val)
{
    if (!readIntFlag(flag))
        bus->writeByte(FLAGS, bus->readByte(FLAGS) | (val << flag));
    else
        bus->writeByte(FLAGS, bus->readByte(FLAGS) ^ (val << flag));
}

u8 Interrupts::readEnableFlag(u8 flag)
{
    return (bus->readByte(ENABLE) >> flag) & 0x1;
}

void Interrupts::setEnableFlag(u8 flag, u8 val)
{
    if (!readIntFlag(flag))
        bus->writeByte(ENABLE, bus->readByte(ENABLE) | (val << flag));
    else
        bus->writeByte(ENABLE, bus->readByte(ENABLE) ^ (val << flag));
}