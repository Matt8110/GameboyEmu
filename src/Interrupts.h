#pragma once

#include "VarTypes.h"
#include "BUS.h"

#define ENABLE 0xFFFF
#define FLAGS 0xFF0F

#define F_VBLANK 0
#define F_LCD 1
#define F_TIMER 2
#define F_SERIAL 3
#define F_JOYPAD 4

class Interrupts
{
public:
    Interrupts(BUS* bus);
    u8 readIntFlag(u8 flag);
    void setIntFlag(u8 flag, u8 val);
    u8 readEnableFlag(u8 flag);
    void setEnableFlag(u8 flag, u8 val);

    //Returns interrupt address for CPU...0 for none
    u16 handleInterrupts();

public:
    bool IME = false;
    BUS* bus;
};