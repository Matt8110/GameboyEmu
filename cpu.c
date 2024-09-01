#include "cpu.h"
#include <stdlib.h>
#include <stdio.h>

#define FETCH_U16 (bus_read(cpu->bus, cpu->pc++) | (bus_read(cpu->bus, cpu->pc++) << 8))
#define FETCH_U8 bus_read(cpu->bus, cpu->pc++)
#define HALF_CARRY_SUB(a, b) (((a & 0xf) - (b & 0xf)) & 0x10) == 0x10//((a ^ (-b) ^ (a - b)) & 0x10)
#define HALF_CARRY_ADD(a, b) (((a & 0xf) + (b & 0xf)) & 0x10) == 0x10//((a >> 4) != ((a + b) >> 4))
#define HALF_CARRY_SUB_16(a, b) ((a ^ (-b) ^ (a - b)) & 0x1000)
#define HALF_CARRY_ADD_16(a, b) (((a & 0xFFF) + (b & 0xFFF)) & 0x1000) == 0x1000

#define TIMA 0xFF05
#define TMA 0xFF06
#define TAC 0xFF07

FILE* gbDoctorLog;

Z80_t* cpu_create(Bus_t* bus)
{
    Z80_t* cpu = malloc(sizeof(Z80_t));
    cpu->haltBug = 0;
    cpu->bus = bus;
    cpu->pc = 0x100;
    cpu->regs.A = 0x1;
    cpu->regs.B = 0x00;
    cpu->regs.C = 0x13;
    cpu->regs.D = 0x0;
    cpu->regs.E = 0xD8;
    cpu->regs.H = 0x01;
    cpu->regs.L = 0x4D;
    cpu->regs.z = 1;
    cpu->sp = 0xFFFE;

    bus_write(cpu->bus,0xFF05, 0x00 );
    bus_write(cpu->bus,0xFF06, 0x00 );
    bus_write(cpu->bus,0xFF07, 0x00 );
    bus_write(cpu->bus,0xFF10, 0x80 );
    bus_write(cpu->bus,0xFF11, 0xBF );
    bus_write(cpu->bus,0xFF12, 0xF3 );
    bus_write(cpu->bus,0xFF14, 0xBF );
    bus_write(cpu->bus,0xFF16, 0x3F );
    bus_write(cpu->bus,0xFF17, 0x00 );
    bus_write(cpu->bus,0xFF19, 0xBF );
    bus_write(cpu->bus,0xFF1A, 0x7F );
    bus_write(cpu->bus,0xFF1B, 0xFF );
    bus_write(cpu->bus,0xFF1C, 0x9F );
    bus_write(cpu->bus,0xFF1E, 0xBF );
    bus_write(cpu->bus,0xFF20, 0xFF );
    bus_write(cpu->bus,0xFF21, 0x00 );
    bus_write(cpu->bus,0xFF22, 0x00 );
    bus_write(cpu->bus,0xFF23, 0xBF );
    bus_write(cpu->bus,0xFF24, 0x77 );
    bus_write(cpu->bus,0xFF25, 0xF3 );
    bus_write(cpu->bus,0xFF26, 0xF1 );
    bus_write(cpu->bus,0xFF40, 0x91 );
    bus_write(cpu->bus,0xFF42, 0x00 );
    bus_write(cpu->bus,0xFF43, 0x00 );
    bus_write(cpu->bus,0xFF45, 0x00 );
    bus_write(cpu->bus,0xFF47, 0xFC );
    bus_write(cpu->bus,0xFF48, 0xFF );
    bus_write(cpu->bus,0xFF49, 0xFF );
    bus_write(cpu->bus,0xFF4A, 0x00 );
    bus_write(cpu->bus,0xFF4B, 0x00 );
    bus_write(cpu->bus,0xFFFF, 0x00 ); 

    // cpu->pc = 0;
    // cpu->regs.A = 0x0;
    // cpu->regs.B = 0x0;
    // cpu->regs.C = 0x0;
    // cpu->regs.D = 0x0;
    // cpu->regs.E = 0x0;
    // cpu->regs.H = 0x0;
    // cpu->regs.L = 0x0;
    // cpu->regs.z = 0;
    // cpu->sp = 0x0;

    cpu->cyclesToWait = 0;
    cpu->IME = 1;
    cpu->IMEDelayCount = 0;
    cpu->regs.h = 1;
    cpu->regs.c = 1;
    cpu->haltedFlag = 0;
    cpu->timerCycles = 0;

    gbDoctorLog = fopen("doctor.log", "w");

    return cpu;
}

void cpu_destroy(Z80_t* cpu)
{
    free(cpu);
}

void cpu_raiseInterrupt(Z80_t* cpu, uint8_t interruptType)
{
    bus_write(cpu->bus, 0xFF0F, bus_read(cpu->bus, 0xFF0F) | interruptType);
}

void cpu_lowerInterrupt(Z80_t* cpu, uint8_t interruptType)
{
    bus_write(cpu->bus, 0xFF0F, bus_read(cpu->bus, 0xFF0F) & ~interruptType);
}

void cpu_dispatchInterrupts(Z80_t* cpu)
{

    //printf("IE: %04x  IF: %04x IME: %d\n", bus_read(cpu->bus, 0xFFFF), bus_read(cpu->bus, 0xFF0F), cpu->IME);

    if (cpu->IME == 1)
    {
        //VBlank
        if ((bus_read(cpu->bus, 0xFF0F) & 0x1) && ((bus_read(cpu->bus, 0xFFFF)) & 0x1))
        {
            //printf("VBlank!\n");
            cpu_stack_push_16(cpu, cpu->pc);
            cpu->pc = 0x40;
            cpu->IME = 0;

            cpu_lowerInterrupt(cpu, INT_VBLANK);
            cpu->interruptLevel++;
        }

        //LCD
        else if (((bus_read(cpu->bus, 0xFF0F) >> 1) & 0x1) && ((bus_read(cpu->bus, 0xFFFF) >> 1) & 0x1))
        {
            //printf("LCD!\n");
            cpu_stack_push_16(cpu, cpu->pc);
            cpu->pc = 0x48;
            cpu->IME = 0;

            cpu_lowerInterrupt(cpu, INT_LCD);
            cpu->interruptLevel++;
        }

        //Timer
        else if (((bus_read(cpu->bus, 0xFF0F) >> 2) & 0x1) && ((bus_read(cpu->bus, 0xFFFF) >> 2) & 0x1))
        {
            //printf("Timer!\n");
            cpu_stack_push_16(cpu, cpu->pc);
            cpu->pc = 0x50;
            cpu->IME = 0;

            cpu_lowerInterrupt(cpu, INT_TIMER);
            cpu->interruptLevel++;
        }

        //Serial
        else if (((bus_read(cpu->bus, 0xFF0F) >> 3) & 0x1) && ((bus_read(cpu->bus, 0xFFFF) >> 3) & 0x1))
        {
            //printf("Serial!\n");
            cpu_stack_push_16(cpu, cpu->pc);
            cpu->pc = 0x58;
            cpu->IME = 0;

            cpu_lowerInterrupt(cpu, INT_SERIAL);
            cpu->interruptLevel++;
        }

        //Joypad
        else if (((bus_read(cpu->bus, 0xFF0F) >> 4) & 0x1) && ((bus_read(cpu->bus, 0xFFFF) >> 4) & 0x1))
        {
            //printf("Joypad!\n");
            cpu_stack_push_16(cpu, cpu->pc);
            cpu->pc = 0x60;
            cpu->IME = 0;

            cpu_lowerInterrupt(cpu, INT_JOYPAD);
            cpu->interruptLevel++;
        }
    }
}

void cpu_handleTimers(Z80_t* cpu)
{
    //Timers
    bus_updateDivTimer(cpu->bus);

    uint8_t clockSelect = bus_read(cpu->bus, TAC) & 3;
    uint8_t timerEnabled = (bus_read(cpu->bus, TAC) >> 2) & 1;
    uint16_t clocksPerTick = 0;

    if (clockSelect == 0) clocksPerTick = 1024;
    if (clockSelect == 1) clocksPerTick = 16;
    if (clockSelect == 2) clocksPerTick = 64;
    if (clockSelect == 3) clocksPerTick = 256;

    if (timerEnabled)
    {
        cpu->timerCycles++;

        if (cpu->timerCycles > clocksPerTick)
        {
            uint16_t timer = bus_read(cpu->bus, 0xFF05);

            if (timer >= 0xFF)
            {
                //printf("Ticking %d\n", timer);
                timer = bus_read(cpu->bus, TMA);

                //Interrupt
                cpu_raiseInterrupt(cpu, INT_TIMER);
            }

            
            bus_write(cpu->bus, TIMA, (uint8_t)timer + 1);
            

            cpu->timerCycles = 0;
        }
    }
}

void cpu_clock(Z80_t* cpu)
{
    if (cpu->haltedFlag == 1)
    {
        if (bus_read(cpu->bus, 0xFFFF) & bus_read(cpu->bus, 0xFF0F))
        {
            cpu->haltedFlag = 0;
        }
        else
        {
            return;
        }
    }

    if (cpu->cyclesToWait > 0)
    {
        cpu->cyclesToWait -= 4;
        return;
    }


    cpu_handleTimers(cpu);
    cpu_dispatchInterrupts(cpu);

    uint8_t opcode = bus_read(cpu->bus, cpu->pc);

    if (cpu->haltBug > 0)
    {
        cpu->haltBug--;
    }
    else
    {
        cpu->pc++;
    }

    switch(opcode)
    {
        //NOP
    case 0x00:
        cpu->cyclesToWait += 4;
    break;

    //LD BC, u16
    case 0x01:
        INSTR_LD_REG16_IMM16(cpu, &cpu->regs.BC);
    break;

    //LD (BC), A
    case 0x02:
        INSTR_LD_REG16PTR_REG8(cpu, &cpu->regs.BC, &cpu->regs.A);
    break;

    //INC BC
    case 0x03:
        INSTR_INC_16(cpu, &cpu->regs.BC);
    break;

    //INC B
    case 0x04:
        INSTR_INC_8(cpu, &cpu->regs.B);
    break;

    //DEC B
    case 0x05:
        INSTR_DEC_8(cpu, &cpu->regs.B);
    break;

    //LD B, u8
    case 0x06:
        INSTR_LD_REG8_IMM8(cpu, &cpu->regs.B);
    break;

    //RLCA
    case 0x07:
        cpu->regs.A = (cpu->regs.A << 1) | (cpu->regs.A >> 7);

        cpu->regs.z = 0;
        cpu->regs.n = 0;
        cpu->regs.h = 0;
        cpu->regs.c = cpu->regs.A & 1;

        cpu->cyclesToWait = 4;
    break;

    //LD (u16), SP
    case 0x08:
        INSTR_LD_IMMPTR_REG16(cpu, &cpu->sp);
    break;

    //ADD HL, BC
    case 0x09:
    {
        INSTR_ADD_REG16_REG16(cpu, &cpu->regs.HL, &cpu->regs.BC);
    }
    break;

    //LD A, (BC)
    case 0x0A:
        INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.BC);
    break;

    //DEC BC
    case 0x0B:
        INSTR_DEC_16(cpu, &cpu->regs.BC);
    break;

    //INC C
    case 0x0C:
        INSTR_INC_8(cpu, &cpu->regs.C);
    break;

    //DEC C
    case 0x0D:
        INSTR_DEC_8(cpu, &cpu->regs.C);
    break;

    //LD C, u8
    case 0x0E:
        INSTR_LD_REG8_IMM8(cpu, &cpu->regs.C);
    break;

    //RRCA
    case 0x0F:

        cpu->regs.c = cpu->regs.A & 1;
        cpu->regs.A = (cpu->regs.A >> 1) | (cpu->regs.A << 7);

        cpu->regs.z = 0;
        cpu->regs.n = 0;
        cpu->regs.h = 0;

        cpu->cyclesToWait = 4;
    break;

    //STOP
    case 0x10:
        //printf("Stop!\n");//???
    break;

    //LD DE, u16
    case 0x11:
        INSTR_LD_REG16_IMM16(cpu, &cpu->regs.DE);
    break;

    //LD (DE), A
    case 0x12:
        INSTR_LD_REG16PTR_REG8(cpu, &cpu->regs.DE, &cpu->regs.A);
    break;

    //INC DE
    case 0x13:
        INSTR_INC_16(cpu, &cpu->regs.DE);
    break;

    //INC D
    case 0x14:
        INSTR_INC_8(cpu, &cpu->regs.D);
    break;

    //DEC D
    case 0x15:
        INSTR_DEC_8(cpu, &cpu->regs.D);
    break;

    //LD D, u8
    case 0x16:
        INSTR_LD_REG8_IMM8(cpu, &cpu->regs.D);
    break;

    //RLA
    case 0x17:
        cpu->regs.h = 0;
        cpu->regs.n = 0;
        cpu->regs.z = 0;

        uint8_t msb = (cpu->regs.A >> 7) && 1;

        cpu->regs.A = cpu->regs.A << 1 | cpu->regs.c;
        cpu->regs.c = msb;

        cpu->cyclesToWait = 4;
    break;

    //JR s8
    case 0x18:
        INSTR_JR_IMM8(cpu, 1);
    break;

    //ADD HL, DE
    case 0x19:
        INSTR_ADD_REG16_REG16(cpu, &cpu->regs.HL, &cpu->regs.DE);
    break;

    //LD A, (DE)
    case 0x1A:
        INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.DE);
    break;

    //DEC DE
    case 0x1B:
        INSTR_DEC_16(cpu, &cpu->regs.DE);
    break;

    //INC E
    case 0x1C:
        INSTR_INC_8(cpu, &cpu->regs.E);
    break;

    //DEC E
    case 0x1D:
        INSTR_DEC_8(cpu, &cpu->regs.E);
    break;

    //LD E, u8
    case 0x1E:
        INSTR_LD_REG8_IMM8(cpu, &cpu->regs.E);
    break;

    //RRA
    case 0x1F:
        cpu->regs.z = 0;
        cpu->regs.n = 0;
        cpu->regs.h = 0;

        //To be OR'd with A later
        uint8_t carryFlag = (cpu->regs.c << 7) & 0x80;
        cpu->regs.c = cpu->regs.A & 1;
        cpu->regs.A = (cpu->regs.A >> 1) | carryFlag;

        cpu->cyclesToWait = 4;
    break;

    //JR NZ, s8
    case 0x20:
        INSTR_JR_IMM8(cpu, cpu->regs.z == 0);
    break;
    
    //LD HL, u16
    case 0x21:
        INSTR_LD_REG16_IMM16(cpu, &cpu->regs.HL);
    break;

    //LD (HL+), A
    case 0x22:
        INSTR_LD_REG16PTR_REG8(cpu, &cpu->regs.HL, &cpu->regs.A);
        cpu->regs.HL++;
    break;

    //INC HL
    case 0x23:
        INSTR_INC_16(cpu, &cpu->regs.HL);
    break;

    //INC H
    case 0x24:
        INSTR_INC_8(cpu, &cpu->regs.H);
    break;

    //DEC H
    case 0x25:
        INSTR_DEC_8(cpu, &cpu->regs.H);
    break;

    //LD H, u8
    case 0x26:
        INSTR_LD_REG8_IMM8(cpu, &cpu->regs.H);
    break;

    //DAA
    case 0x27:
        uint8_t correction = 0;

        uint8_t setFlagC = 0;
        if (cpu->regs.h || (!cpu->regs.n && (cpu->regs.A & 0xf) > 9)) {
            correction |= 0x6;
        }

        if (cpu->regs.c || (!cpu->regs.n && cpu->regs.A > 0x99)) {
            correction |= 0x60;
            setFlagC = 1;
        }

        cpu->regs.A += cpu->regs.n ? -correction : correction;

        cpu->regs.A &= 0xff;

        uint8_t setFlagZ = cpu->regs.A == 0;

        cpu->regs.z = setFlagZ;
        cpu->regs.c = setFlagC;
        cpu->regs.h = 0;

        cpu->cyclesToWait = 4;
    break;

    //JR Z, s8
    case 0x28:
        INSTR_JR_IMM8(cpu, cpu->regs.z == 1);
    break;

    //ADD HL, HL
    case 0x29:
        INSTR_ADD_REG16_REG16(cpu, &cpu->regs.HL, &cpu->regs.HL);
    break;

    //LD, A, (HL+)
    case 0x2A:
        INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.HL);
        cpu->regs.HL++;
    break;

    //DEC HL
    case 0x2B:
        INSTR_DEC_16(cpu, &cpu->regs.HL);
    break;

    //INC L
    case 0x2C:
        INSTR_INC_8(cpu, &cpu->regs.L);
    break;

    //DEC L
    case 0x2D:
        INSTR_DEC_8(cpu, &cpu->regs.L);
    break;

    //LD L, u8
    case 0x2E:
        INSTR_LD_REG8_IMM8(cpu, &cpu->regs.L);
    break;

    //CPL
    case 0x2F:
        cpu->regs.A = ~cpu->regs.A;
        cpu->regs.n = 1;
        cpu->regs.h = 1;

        cpu->cyclesToWait = 4;
    break;

    //JR NC, i8
    case 0x30:
        INSTR_JR_IMM8(cpu, cpu->regs.c == 0);
    break;

    //LD SP, u16
    case 0x31:
        INSTR_LD_REG16_IMM16(cpu, &cpu->sp);
    break;

    //LD (HL-), A
    case 0x32:
        INSTR_LD_REG16PTR_REG8(cpu, &cpu->regs.HL, &cpu->regs.A);
        cpu->regs.HL--;
    break;

    //INC SP
    case 0x33:
        INSTR_INC_16(cpu, &cpu->sp);
    break;

    //INC (HL)
    case 0x34:
        INSTR_INC_REG16PTR(cpu, &cpu->regs.HL);
    break;

    //DEC (HL)
    case 0x35:
        INSTR_DEC_REG16PTR(cpu, &cpu->regs.HL);
    break;

    //LD (HL), u8
    case 0x36:
        INSTR_LD_REG16PTR_IMM8(cpu, &cpu->regs.HL);
    break;

    //SCF
    case 0x37:
        cpu->regs.n = 0;
        cpu->regs.h = 0;
        cpu->regs.c = 1;

        cpu->cyclesToWait = 4;
    break;

    //JR C, i8
    case 0x38:
        cpu->cyclesToWait = 8;

        int8_t val = (int8_t)bus_read(cpu->bus, cpu->pc++);

        if (cpu->regs.c == 1)
        {
            cpu->pc += val;
            cpu->cyclesToWait += 4;
        }
    break;

    //ADD HL, SP
    case 0x39:
        INSTR_ADD_REG16_REG16(cpu, &cpu->regs.HL, &cpu->sp);
    break; 

    //LD A, (HL-)
    case 0x3A:
        INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.HL);
        cpu->regs.HL--;
    break;

    //DEC SP
    case 0x3B:
        INSTR_DEC_16(cpu, &cpu->sp);
    break;

    //INC A
    case 0x3C:
        INSTR_INC_8(cpu, &cpu->regs.A);
    break;

    //DEC A
    case 0x3D:
        INSTR_DEC_8(cpu, &cpu->regs.A);
    break;

    //LD A, u8
    case 0x3E:
        INSTR_LD_REG8_IMM8(cpu, &cpu->regs.A);
    break;

    //CCF
    case 0x3F:
        cpu->regs.c = ~cpu->regs.c;
        cpu->regs.n = 0;
        cpu->regs.h = 0;

        cpu->cyclesToWait = 4;
    break;

    //LD B, B
    case 0x40:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.B, &cpu->regs.B);
    break;

    //LD B, C
    case 0x41:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.B, &cpu->regs.C);
    break;

    //LD B, D
    case 0x42:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.B, &cpu->regs.D);
    break;

    //LD B, E
    case 0x43:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.B, &cpu->regs.E);
    break;

    //LD B, H
    case 0x44:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.B, &cpu->regs.H);
    break;

    //LD B, L
    case 0x45:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.B, &cpu->regs.L);
    break;

    //LD B, (HL)
    case 0x46:
        INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.B, &cpu->regs.HL);
    break;

    //LD B, A
    case 0x47:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.B, &cpu->regs.A);
    break;

    //LD C, B
    case 0x48:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.C, &cpu->regs.B);
    break;

    //LD C, C
    case 0x49:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.C, &cpu->regs.C);
    break;

    //LD C, D
    case 0x4A:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.C, &cpu->regs.D);
    break;

    //LD C, E
    case 0x4B:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.C, &cpu->regs.E);
    break;

    //LD C, H
    case 0x4C:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.C, &cpu->regs.H);
    break;

    //LD C, L
    case 0x4D:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.C, &cpu->regs.L);
    break;

    //LD C, (HL)
    case 0x4E:
        INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.C, &cpu->regs.HL);
    break;

    //LD C, A
    case 0x4F:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.C, &cpu->regs.A);
    break;

    //LD D, B
    case 0x50:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.D, &cpu->regs.B);
    break;

    //LD D, C
    case 0x51:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.D, &cpu->regs.C);
    break;

    //LD D, D
    case 0x52:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.D, &cpu->regs.D);
    break;

    //LD D, E
    case 0x53:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.D, &cpu->regs.E);
    break;

    //LD D, H
    case 0x54:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.D, &cpu->regs.H);
    break;

    //LD D, L
    case 0x55:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.D, &cpu->regs.L);
    break;

    //LD D, (HL)
    case 0x56:
        INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.D, &cpu->regs.HL);
    break;

    //LD D, A
    case 0x57:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.D, &cpu->regs.A);
    break;

    //LD E, B
    case 0x58:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.E, &cpu->regs.B);
    break;

    //LD E, C
    case 0x59:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.E, &cpu->regs.C);
    break;

    //LD E, D
    case 0x5A:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.E, &cpu->regs.D);
    break;

    //LD E, E
    case 0x5B:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.E, &cpu->regs.E);
    break;

    //LD E, H
    case 0x5C:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.E, &cpu->regs.H);
    break;

    //LD E, L
    case 0x5D:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.E, &cpu->regs.L);
    break;

    //LD E, (HL)
    case 0x5E:
        INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.E, &cpu->regs.HL);
    break;

    //LD E, A
    case 0x5F:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.E, &cpu->regs.A);
    break;

    //LD H, B
    case 0x60:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.H, &cpu->regs.B);
    break;

    //LD H, C
    case 0x61:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.H, &cpu->regs.C);
    break;

    //LD H, D
    case 0x62:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.H, &cpu->regs.D);
    break;

    //LD H, E
    case 0x63:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.H, &cpu->regs.E);
    break;

    //LD H, H
    case 0x64:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.H, &cpu->regs.H);
    break;

    //LD H, L
    case 0x65:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.H, &cpu->regs.L);
    break;

    //LD H, (HL)
    case 0x66:
        INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.H, &cpu->regs.HL);
    break;

    //LD H, A
    case 0x67:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.H, &cpu->regs.A);
    break;

    //LD L, B
    case 0x68:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.L, &cpu->regs.B);
    break;

    //LD L, C
    case 0x69:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.L, &cpu->regs.C);
    break;

    //LD L, D
    case 0x6A:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.L, &cpu->regs.D);
    break;

    //LD L, E
    case 0x6B:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.L, &cpu->regs.E);
    break;

    //LD L, H
    case 0x6C:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.L, &cpu->regs.H);
    break;

    //LD L, L
    case 0x6D:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.L, &cpu->regs.L);
    break;

    //LD L, (HL)
    case 0x6E:
        INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.L, &cpu->regs.HL);
    break;

    //LD L, A
    case 0x6F:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.L, &cpu->regs.A);
    break;

    //LD (HL), B
    case 0x70:
        INSTR_LD_REG16PTR_REG8(cpu, &cpu->regs.HL, &cpu->regs.B);
    break;

    //LD (HL), C
    case 0x71:
        INSTR_LD_REG16PTR_REG8(cpu, &cpu->regs.HL, &cpu->regs.C);
    break;

    //LD (HL), D
    case 0x72:
        INSTR_LD_REG16PTR_REG8(cpu, &cpu->regs.HL, &cpu->regs.D);
    break;

    //LD (HL), E
    case 0x73:
        INSTR_LD_REG16PTR_REG8(cpu, &cpu->regs.HL, &cpu->regs.E);
    break;

    //LD (HL), H
    case 0x74:
        INSTR_LD_REG16PTR_REG8(cpu, &cpu->regs.HL, &cpu->regs.H);
    break;

    //LD (HL), L
    case 0x75:
        INSTR_LD_REG16PTR_REG8(cpu, &cpu->regs.HL, &cpu->regs.L);
    break;

    //HALT
    case 0x76:
        //printf("HALT! To be implemented.\n");
        cpu->haltedFlag = 1;
    break;

    //LD (HL), A
    case 0x77:
        INSTR_LD_REG16PTR_REG8(cpu, &cpu->regs.HL, &cpu->regs.A);
    break;

    //LD A, B
    case 0x78:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.B);
    break;

    //LD A, C
    case 0x79:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.C);
    break;

    //LD A, D
    case 0x7A:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.D);
    break;

    //LD A, E
    case 0x7B:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.E);
    break;

    //LD A, H
    case 0x7C:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.H);
    break;

    //LD A, L
    case 0x7D:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.L);
    break;
    
    //LD A, (HL)
    case 0x7E:
        INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.HL);
    break;

    //LD A, A
    case 0x7F:
        INSTR_LD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.A);
    break;

    //ADD A, B
    case 0x80:
        INSTR_ADD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.B);
    break;

    //ADD A, C
    case 0x81:
        INSTR_ADD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.C);
    break;

    //ADD A, D
    case 0x82:
        INSTR_ADD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.D);
    break;

    //ADD A, E
    case 0x83:
        INSTR_ADD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.E);
    break;

    //ADD A, H
    case 0x84:
        INSTR_ADD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.H);
    break;

    //ADD A, L
    case 0x85:
        INSTR_ADD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.L);
    break;

    //ADD A, (HL)
    case 0x86:
        INSTR_ADD_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.HL);
    break;

    //ADD A, A
    case 0x87:
        INSTR_ADD_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.A);
    break;

    //ADC A, B
    case 0x88:
        INSTR_ADC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.B);
    break;

    //ADC A, C
    case 0x89:
        INSTR_ADC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.C);
    break;

    //ADC A, D
    case 0x8A:
        INSTR_ADC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.D);
    break;

    //ADC A, E
    case 0x8B:
        INSTR_ADC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.E);
    break;

    //ADC A, H
    case 0x8C:
        INSTR_ADC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.H);
    break;

    //ADC A, L
    case 0x8D:
        INSTR_ADC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.L);
    break;

    //ADC A, (HL)
    case 0x8E:
        INSTR_ADC_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.HL);
    break;

    //ADC A, A
    case 0x8F:
        INSTR_ADC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.A);
    break;

    //SUB A, B
    case 0x90:
        INSTR_SUB_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.B);
    break;

    //SUB A, C
    case 0x91:
        INSTR_SUB_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.C);
    break;

    //SUB A, D
    case 0x92:
        INSTR_SUB_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.D);
    break;

    //SUB A, E
    case 0x93:
        INSTR_SUB_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.E);
    break;

    //SUB A, H
    case 0x94:
        INSTR_SUB_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.H);
    break;

    //SUB A, L
    case 0x95:
        INSTR_SUB_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.L);
    break;

    //SUB A, (HL)
    case 0x96:
        INSTR_SUB_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.HL);
    break;

    //SUB A, A
    case 0x97:
        INSTR_SUB_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.A);
    break;

    //SBC A, B
    case 0x98:
        INSTR_SBC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.B);
    break;

    //SBC A, C
    case 0x99:
        INSTR_SBC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.C);
    break;

    //SBC A, D
    case 0x9A:
        INSTR_SBC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.D);
    break;

    //SBC A, E
    case 0x9B:
        INSTR_SBC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.E);
    break;

    //SBC A, H
    case 0x9C:
        INSTR_SBC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.H);
    break;

    //SBC A, L
    case 0x9D:
        INSTR_SBC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.L);
    break;

    //SBC A, (HL)
    case 0x9E:
        INSTR_SBC_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.HL);
    break;

    //SBC A, A
    case 0x9F:
        INSTR_SBC_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.A);
    break;

    //AND A, B
    case 0xA0:
        INSTR_AND_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.B);
    break;

    //AND A, C
    case 0xA1:
        INSTR_AND_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.C);
    break;

    //AND A, D
    case 0xA2:
        INSTR_AND_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.D);
    break;

    //AND A, E
    case 0xA3:
        INSTR_AND_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.E);
    break;

    //AND A, H
    case 0xA4:
        INSTR_AND_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.H);
    break;

    //AND A, L
    case 0xA5:
        INSTR_AND_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.L);
    break;

    //AND A, (HL)
    case 0xA6:
        INSTR_AND_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.HL);
    break;

    //AND A, A
    case 0xA7:
        INSTR_AND_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.A);
    break;

    //XOR A, B
    case 0xA8:
        INSTR_XOR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.B);
    break;

    //XOR A, C
    case 0xA9:
        INSTR_XOR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.C);
    break;

    //XOR A, D
    case 0xAA:
        INSTR_XOR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.D);
    break;

    //XOR A, E
    case 0xAB:
        INSTR_XOR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.E);
    break;

    //XOR A, H
    case 0xAC:
        INSTR_XOR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.H);
    break;

    //XOR A, L
    case 0xAD:
        INSTR_XOR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.L);
    break;

    //XOR A, (HL)
    case 0xAE:
        INSTR_XOR_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.HL);
    break;

    //XOR A, A
    case 0xAF:
        INSTR_XOR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.A);
    break;




    //OR A, B
    case 0xB0:
        INSTR_OR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.B);
    break;

    //OR A, C
    case 0xB1:
        INSTR_OR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.C);
    break;

    //OR A, D
    case 0xB2:
        INSTR_OR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.D);
    break;

    //OR A, E
    case 0xB3:
        INSTR_OR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.E);
    break;

    //OR A, H
    case 0xB4:
        INSTR_OR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.H);
    break;

    //OR A, L
    case 0xB5:
        INSTR_OR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.L);
    break;

    //OR A, (HL)
    case 0xB6:
        INSTR_OR_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.HL);
    break;

    //OR A, A
    case 0xB7:
        INSTR_OR_REG8_REG8(cpu, &cpu->regs.A, &cpu->regs.A);
    break;

    //CP A, B
    case 0xB8:
        INSTR_CP_REG8(cpu, &cpu->regs.B);
    break;

    //CP A, C
    case 0xB9:
        INSTR_CP_REG8(cpu,&cpu->regs.C);
    break;

    //CP A, D
    case 0xBA:
        INSTR_CP_REG8(cpu, &cpu->regs.D);
    break;

    //CP A, E
    case 0xBB:
        INSTR_CP_REG8(cpu, &cpu->regs.E);
    break;

    //CP A, H
    case 0xBC:
        INSTR_CP_REG8(cpu, &cpu->regs.H);
    break;

    //CP A, L
    case 0xBD:
        INSTR_CP_REG8(cpu, &cpu->regs.L);
    break;

    //CP A, (HL)
    case 0xBE:
        INSTR_CP_REG8_REG16PTR(cpu, &cpu->regs.A, &cpu->regs.HL);
    break;

    //CP A, A
    case 0xBF:
        INSTR_CP_REG8(cpu, &cpu->regs.A);
    break;

    //RET NZ
    case 0xC0:
        INSTR_RET(cpu, cpu->regs.z == 0);
    break;

    //POP BC
    case 0xC1:
        cpu->regs.BC = cpu_stack_pop_16(cpu);
        cpu->cyclesToWait = 12;
    break;

    //JP NZ, u16
    case 0xC2:
        INSTR_JP_IMM16(cpu, cpu->regs.z == 0);
    break;

    //JMP u16
    case 0xC3:
        INSTR_JP_IMM16(cpu, 1);
    break;

    //CALL NZ, u16
    case 0xC4:
        INSTR_CALL_IMM16(cpu, cpu->regs.z == 0);
    break;

    //PUSH BC
    case 0xC5:
        cpu_stack_push_16(cpu, cpu->regs.BC);
        cpu->cyclesToWait = 16;
    break;

    //ADD A, u8
    case 0xC6:
    {
        uint8_t val = FETCH_U8;
        INSTR_ADD_REG8_REG8(cpu, &cpu->regs.A, &val);

        cpu->cyclesToWait = 8;
    }
    break;

    //RST 0
    case 0xC7:
        cpu_stack_push_16(cpu, cpu->pc);
        cpu->pc = 0;
        cpu->cyclesToWait = 16;
    break;

    //RET Z
    case 0xC8:
        INSTR_RET(cpu, cpu->regs.z == 1);
    break;

    //RET
    case 0xC9:
        INSTR_RET(cpu, 1);
    break;

    //JP C, u16
    case 0xCA:
        INSTR_JP_IMM16(cpu, cpu->regs.z);
    break;

    //Secondary instructions
    case 0xCB:
        
        //Secondary opcode
        uint8_t secondOpcode = FETCH_U8;

        switch(secondOpcode)
        {
            //RLC B
            case 0x00:
                INSTR_CB_RLC_REG8(cpu, &cpu->regs.B);
            break;

            //RLC C
            case 0x01:
                INSTR_CB_RLC_REG8(cpu, &cpu->regs.C);
            break;

            //RLC D
            case 0x02:
                INSTR_CB_RLC_REG8(cpu, &cpu->regs.D);
            break;

            //RLC E
            case 0x03:
                INSTR_CB_RLC_REG8(cpu, &cpu->regs.E);
            break;

            //RLC H
            case 0x04:
                INSTR_CB_RLC_REG8(cpu, &cpu->regs.H);
            break;

            //RLC L
            case 0x05:
                INSTR_CB_RLC_REG8(cpu, &cpu->regs.L);
            break;

            //RLC (HL)
            case 0x06:
                INSTR_CB_RLC_REG16PTR(cpu, &cpu->regs.HL);
            break;

            //RLC A
            case 0x07:
                INSTR_CB_RLC_REG8(cpu, &cpu->regs.A);
            break;

            //RRC B
            case 0x08:
                INSTR_CB_RRC_REG8(cpu, &cpu->regs.B);
            break;

            //RRC C
            case 0x09:
                INSTR_CB_RRC_REG8(cpu, &cpu->regs.C);
            break;

            //RRC D
            case 0x0A:
                INSTR_CB_RRC_REG8(cpu, &cpu->regs.D);
            break;

            //RRC E
            case 0x0B:
                INSTR_CB_RRC_REG8(cpu, &cpu->regs.E);
            break;

            //RRC H
            case 0x0C:
                INSTR_CB_RRC_REG8(cpu, &cpu->regs.H);
            break;

            //RRC L
            case 0x0D:
                INSTR_CB_RRC_REG8(cpu, &cpu->regs.L);
            break;

            //RRC (HL)
            case 0x0E:
                INSTR_CB_RRC_REG16PTR(cpu, &cpu->regs.HL);
            break;

            //RRC A
            case 0x0F:
                INSTR_CB_RRC_REG8(cpu, &cpu->regs.A);
            break;

            //RL B
            case 0x10:
                INSTR_CB_RL_REG8(cpu, &cpu->regs.B);
            break;

            //RL C
            case 0x11:
                INSTR_CB_RL_REG8(cpu, &cpu->regs.C);
            break;

            //RL D
            case 0x12:
                INSTR_CB_RL_REG8(cpu, &cpu->regs.D);
            break;

            //RL E
            case 0x13:
                INSTR_CB_RL_REG8(cpu, &cpu->regs.E);
            break;

            //RL H
            case 0x14:
                INSTR_CB_RL_REG8(cpu, &cpu->regs.H);
            break;

            //RL L
            case 0x15:
                INSTR_CB_RL_REG8(cpu, &cpu->regs.L);
            break;

            //RL (HL)
            case 0x16:
                INSTR_CB_RL_REG8(cpu, &cpu->bus->data[cpu->regs.HL]);
            break;

            //RL A
            case 0x17:
                INSTR_CB_RL_REG8(cpu, &cpu->regs.A);
            break;

            //RR B
            case 0x18:
                INSTR_CB_RR_REG8(cpu, &cpu->regs.B);
            break;

            //RR C
            case 0x19:
                INSTR_CB_RR_REG8(cpu, &cpu->regs.C);
            break;

            //RR D
            case 0x1A:
                INSTR_CB_RR_REG8(cpu, &cpu->regs.D);
            break;

            //RR E
            case 0x1B:
                INSTR_CB_RR_REG8(cpu, &cpu->regs.E);
            break;

            //RR H
            case 0x1C:
                INSTR_CB_RR_REG8(cpu, &cpu->regs.H);
            break;

            //RR L
            case 0x1D:
                INSTR_CB_RR_REG8(cpu, &cpu->regs.L);
            break;

            //RR (HL)
            case 0x1E:
                INSTR_CB_RR_REG8(cpu, &cpu->bus->data[cpu->regs.HL]);
            break;

            //RR L
            case 0x1F:
                INSTR_CB_RR_REG8(cpu, &cpu->regs.A);
            break;

            //SLA B
            case 0x20:
                INSTR_CB_SLA_REG8(cpu, &cpu->regs.B);
            break;

            //SLA C
            case 0x21:
                INSTR_CB_SLA_REG8(cpu, &cpu->regs.C);
            break;

            //SLA D
            case 0x22:
                INSTR_CB_SLA_REG8(cpu, &cpu->regs.D);
            break;

            //SLA E
            case 0x23:
                INSTR_CB_SLA_REG8(cpu, &cpu->regs.E);
            break;

            //SLA H
            case 0x24:
                INSTR_CB_SLA_REG8(cpu, &cpu->regs.H);
            break;

            //SLA L
            case 0x25:
                INSTR_CB_SLA_REG8(cpu, &cpu->regs.L);
            break;

            //SLA (HL)
            case 0x26:
                INSTR_CB_SLA_REG8(cpu, &cpu->bus->data[cpu->regs.HL]);
            break;

            //SLA A
            case 0x27:
                INSTR_CB_SLA_REG8(cpu, &cpu->regs.A);
            break;

            //SRA B
            case 0x28:
                INSTR_CB_SRA_REG8(cpu, &cpu->regs.B);
            break;

            //SRA C
            case 0x29:
                INSTR_CB_SRA_REG8(cpu, &cpu->regs.C);
            break;

            //SRA D
            case 0x2A:
                INSTR_CB_SRA_REG8(cpu, &cpu->regs.D);
            break;

            //SRA E
            case 0x2B:
                INSTR_CB_SRA_REG8(cpu, &cpu->regs.E);
            break;

            //SRA H
            case 0x2C:
                INSTR_CB_SRA_REG8(cpu, &cpu->regs.H);
            break;

            //SRA L
            case 0x2D:
                INSTR_CB_SRA_REG8(cpu, &cpu->regs.L);
            break;

            //SRA (HL)
            case 0x2E:
                INSTR_CB_SRA_REG8(cpu, &cpu->bus->data[cpu->regs.HL]);
            break;

            //SRA A
            case 0x2F:
                INSTR_CB_SRA_REG8(cpu, &cpu->regs.A);
            break;

            //SWAP B
            case 0x30:
                INSTR_CB_SWAP_REG8(cpu, &cpu->regs.B);
            break;

            //SWAP C
            case 0x31:
                INSTR_CB_SWAP_REG8(cpu, &cpu->regs.C);
            break;

            //SWAP D
            case 0x32:
                INSTR_CB_SWAP_REG8(cpu, &cpu->regs.D);
            break;

            //SWAP E
            case 0x33:
                INSTR_CB_SWAP_REG8(cpu, &cpu->regs.E);
            break;

            //SWAP H
            case 0x34:
                INSTR_CB_SWAP_REG8(cpu, &cpu->regs.H);
            break;

            //SWAP L
            case 0x35:
                INSTR_CB_SWAP_REG8(cpu, &cpu->regs.L);
            break;

            //SWAP (HL)
            case 0x36:
                INSTR_CB_SWAP_REG8(cpu, &cpu->bus->data[cpu->regs.HL]);
            break;

            //SWAP B
            case 0x37:
                INSTR_CB_SWAP_REG8(cpu, &cpu->regs.A);
            break;

            //SRL B
            case 0x38:
                INSTR_CB_SRL_REG8(cpu, &cpu->regs.B);
            break;

            //SRL C
            case 0x39:
                INSTR_CB_SRL_REG8(cpu, &cpu->regs.C);
            break;

            //SRL D
            case 0x3A:
                INSTR_CB_SRL_REG8(cpu, &cpu->regs.D);
            break;

            //SRL E
            case 0x3B:
                INSTR_CB_SRL_REG8(cpu, &cpu->regs.E);
            break;

            //SRL H
            case 0x3C:
                INSTR_CB_SRL_REG8(cpu, &cpu->regs.H);
            break;

            //SRL L
            case 0x3D:
                INSTR_CB_SRL_REG8(cpu, &cpu->regs.L);
            break;

            //SRL (HL)
            case 0x3E:
                INSTR_CB_SRL_REG8(cpu, &cpu->bus->data[cpu->regs.HL]);
            break;

            //SRL A
            case 0x3F:
                INSTR_CB_SRL_REG8(cpu, &cpu->regs.A);
            break;

            //BIT 0, B
            case 0x40:
                INSTR_BIT_IMM8_REG8(cpu, 0, &cpu->regs.B);
            break;

            //BIT 0, C
            case 0x41:
                INSTR_BIT_IMM8_REG8(cpu, 0, &cpu->regs.C);
            break;

            //BIT 0, D
            case 0x42:
                INSTR_BIT_IMM8_REG8(cpu, 0, &cpu->regs.D);
            break;

            //BIT 0, E
            case 0x43:
                INSTR_BIT_IMM8_REG8(cpu, 0, &cpu->regs.E);
            break;

            //BIT 0, H
            case 0x44:
                INSTR_BIT_IMM8_REG8(cpu, 0, &cpu->regs.H);
            break;

            //BIT 0, L
            case 0x45:
                INSTR_BIT_IMM8_REG8(cpu, 0, &cpu->regs.L);
            break;

            //BIT 0, (HL)
            case 0x46:
                INSTR_BIT_IMM8_REG8(cpu, 0, &cpu->bus->data[cpu->regs.HL]);
            break;

            //BIT 0, A
            case 0x47:
                INSTR_BIT_IMM8_REG8(cpu, 0, &cpu->regs.A);
            break;

            //BIT 1, B
            case 0x48:
                INSTR_BIT_IMM8_REG8(cpu, 1, &cpu->regs.B);
            break;

            //BIT 1, C
            case 0x49:
                INSTR_BIT_IMM8_REG8(cpu, 1, &cpu->regs.C);
            break;

            //BIT 1, D
            case 0x4A:
                INSTR_BIT_IMM8_REG8(cpu, 1, &cpu->regs.D);
            break;

            //BIT 1, E
            case 0x4B:
                INSTR_BIT_IMM8_REG8(cpu, 1, &cpu->regs.E);
            break;

            //BIT 1, H
            case 0x4C:
                INSTR_BIT_IMM8_REG8(cpu, 1, &cpu->regs.H);
            break;

            //BIT 1, L
            case 0x4D:
                INSTR_BIT_IMM8_REG8(cpu, 1, &cpu->regs.L);
            break;

            //BIT 1, (HL)
            case 0x4E:
                INSTR_BIT_IMM8_REG8(cpu, 1, &cpu->bus->data[cpu->regs.HL]);
            break;

            //BIT 1, A
            case 0x4F:
                INSTR_BIT_IMM8_REG8(cpu, 1, &cpu->regs.A);
            break;

            //BIT 2, B
            case 0x50:
                INSTR_BIT_IMM8_REG8(cpu, 2, &cpu->regs.B);
            break;

            //BIT 2, C
            case 0x51:
                INSTR_BIT_IMM8_REG8(cpu, 2, &cpu->regs.C);
            break;

            //BIT 2, D
            case 0x52:
                INSTR_BIT_IMM8_REG8(cpu, 2, &cpu->regs.D);
            break;

            //BIT 2, E
            case 0x53:
                INSTR_BIT_IMM8_REG8(cpu, 2, &cpu->regs.E);
            break;

            //BIT 2, H
            case 0x54:
                INSTR_BIT_IMM8_REG8(cpu, 2, &cpu->regs.H);
            break;

            //BIT 2, L
            case 0x55:
                INSTR_BIT_IMM8_REG8(cpu, 2, &cpu->regs.L);
            break;

            //BIT 2, (HL)
            case 0x56:
                INSTR_BIT_IMM8_REG8(cpu, 2, &cpu->bus->data[cpu->regs.HL]);
            break;

            //BIT 2, A
            case 0x57:
                INSTR_BIT_IMM8_REG8(cpu, 2, &cpu->regs.A);
            break;

            //BIT 3, B
            case 0x58:
                INSTR_BIT_IMM8_REG8(cpu, 3, &cpu->regs.B);
            break;

            //BIT 3, C
            case 0x59:
                INSTR_BIT_IMM8_REG8(cpu, 3, &cpu->regs.C);
            break;

            //BIT 3, D
            case 0x5A:
                INSTR_BIT_IMM8_REG8(cpu, 3, &cpu->regs.D);
            break;

            //BIT 3, E
            case 0x5B:
                INSTR_BIT_IMM8_REG8(cpu, 3, &cpu->regs.E);
            break;

            //BIT 3, H
            case 0x5C:
                INSTR_BIT_IMM8_REG8(cpu, 3, &cpu->regs.H);
            break;

            //BIT 3, L
            case 0x5D:
                INSTR_BIT_IMM8_REG8(cpu, 3, &cpu->regs.L);
            break;

            //BIT 3, (HL)
            case 0x5E:
                INSTR_BIT_IMM8_REG8(cpu, 3, &cpu->bus->data[cpu->regs.HL]);
            break;

            //BIT 3, A
            case 0x5F:
                INSTR_BIT_IMM8_REG8(cpu, 3, &cpu->regs.A);
            break;

            //BIT 4, B
            case 0x60:
                INSTR_BIT_IMM8_REG8(cpu, 4, &cpu->regs.B);
            break;

            //BIT 4, C
            case 0x61:
                INSTR_BIT_IMM8_REG8(cpu, 4, &cpu->regs.C);
            break;

            //BIT 4, D
            case 0x62:
                INSTR_BIT_IMM8_REG8(cpu, 4, &cpu->regs.D);
            break;

            //BIT 4, E
            case 0x63:
                INSTR_BIT_IMM8_REG8(cpu, 4, &cpu->regs.E);
            break;

            //BIT 4, H
            case 0x64:
                INSTR_BIT_IMM8_REG8(cpu, 4, &cpu->regs.H);
            break;

            //BIT 4, L
            case 0x65:
                INSTR_BIT_IMM8_REG8(cpu, 4, &cpu->regs.L);
            break;

            //BIT 4, (HL)
            case 0x66:
                INSTR_BIT_IMM8_REG8(cpu, 4, &cpu->bus->data[cpu->regs.HL]);
            break;

            //BIT 4, A
            case 0x67:
                INSTR_BIT_IMM8_REG8(cpu, 4, &cpu->regs.A);
            break;

            //BIT 5, B
            case 0x68:
                INSTR_BIT_IMM8_REG8(cpu, 5, &cpu->regs.B);
            break;

            //BIT 5, C
            case 0x69:
                INSTR_BIT_IMM8_REG8(cpu, 5, &cpu->regs.C);
            break;

            //BIT 5, D
            case 0x6A:
                INSTR_BIT_IMM8_REG8(cpu, 5, &cpu->regs.D);
            break;

            //BIT 5, E
            case 0x6B:
                INSTR_BIT_IMM8_REG8(cpu, 5, &cpu->regs.E);
            break;

            //BIT 5, H
            case 0x6C:
                INSTR_BIT_IMM8_REG8(cpu, 5, &cpu->regs.H);
            break;

            //BIT 5, L
            case 0x6D:
                INSTR_BIT_IMM8_REG8(cpu, 5, &cpu->regs.L);
            break;

            //BIT 5, (HL)
            case 0x6E:
                INSTR_BIT_IMM8_REG8(cpu, 5, &cpu->bus->data[cpu->regs.HL]);
            break;

            //BIT 5, A
            case 0x6F:
                INSTR_BIT_IMM8_REG8(cpu, 5, &cpu->regs.A);
            break;

            //BIT 6, B
            case 0x70:
                INSTR_BIT_IMM8_REG8(cpu, 6, &cpu->regs.B);
            break;

            //BIT 6, C
            case 0x71:
                INSTR_BIT_IMM8_REG8(cpu, 6, &cpu->regs.C);
            break;

            //BIT 6, D
            case 0x72:
                INSTR_BIT_IMM8_REG8(cpu, 6, &cpu->regs.D);
            break;

            //BIT 6, E
            case 0x73:
                INSTR_BIT_IMM8_REG8(cpu, 6, &cpu->regs.E);
            break;

            //BIT 6, H
            case 0x74:
                INSTR_BIT_IMM8_REG8(cpu, 6, &cpu->regs.H);
            break;

            //BIT 6, L
            case 0x75:
                INSTR_BIT_IMM8_REG8(cpu, 6, &cpu->regs.L);
            break;

            //BIT 6, (HL)
            case 0x76:
                INSTR_BIT_IMM8_REG8(cpu, 6, &cpu->bus->data[cpu->regs.HL]);
            break;

            //BIT 6, A
            case 0x77:
                INSTR_BIT_IMM8_REG8(cpu, 6, &cpu->regs.A);
            break;

            //BIT 7, B
            case 0x78:
                INSTR_BIT_IMM8_REG8(cpu, 7, &cpu->regs.B);
            break;

            //BIT 7, C
            case 0x79:
                INSTR_BIT_IMM8_REG8(cpu, 7, &cpu->regs.C);
            break;

            //BIT 7, D
            case 0x7A:
                INSTR_BIT_IMM8_REG8(cpu, 7, &cpu->regs.D);
            break;

            //BIT 7, E
            case 0x7B:
                INSTR_BIT_IMM8_REG8(cpu, 7, &cpu->regs.E);
            break;

            //BIT 7, H
            case 0x7C:
                INSTR_BIT_IMM8_REG8(cpu, 7, &cpu->regs.H);
            break;

            //BIT 7, L
            case 0x7D:
                INSTR_BIT_IMM8_REG8(cpu, 7, &cpu->regs.L);
            break;

            //BIT 7, (HL)
            case 0x7E:
                INSTR_BIT_IMM8_REG8(cpu, 7, &cpu->bus->data[cpu->regs.HL]);
            break;

            //BIT 7, A
            case 0x7F:
                INSTR_BIT_IMM8_REG8(cpu, 7, &cpu->regs.A);
            break;

            //RES 0, B
            case 0x80:
                INSTR_CB_RES_REG8(cpu, 0, &cpu->regs.B);
            break;

            //RES 0, C
            case 0x81:
                INSTR_CB_RES_REG8(cpu, 0, &cpu->regs.C);
            break;

            //RES 0, D
            case 0x82:
                INSTR_CB_RES_REG8(cpu, 0, &cpu->regs.D);
            break;

            //RES 0, E
            case 0x83:
                INSTR_CB_RES_REG8(cpu, 0, &cpu->regs.E);
            break;

            //RES 0, H
            case 0x84:
                INSTR_CB_RES_REG8(cpu, 0, &cpu->regs.H);
            break;

            //RES 0, L
            case 0x85:
                INSTR_CB_RES_REG8(cpu, 0, &cpu->regs.L);
            break;

            //RES 0, (HL)
            case 0x86:
                INSTR_CB_RES_REG8(cpu, 0, &cpu->bus->data[cpu->regs.HL]);
            break;

            //RES 0, A
            case 0x87:
                INSTR_CB_RES_REG8(cpu, 0, &cpu->regs.A);
            break;

            //RES 1, B
            case 0x88:
                INSTR_CB_RES_REG8(cpu, 1, &cpu->regs.B);
            break;

            //RES 1, C
            case 0x89:
                INSTR_CB_RES_REG8(cpu, 1, &cpu->regs.C);
            break;

            //RES 1, D
            case 0x8A:
                INSTR_CB_RES_REG8(cpu, 1, &cpu->regs.D);
            break;

            //RES 1, E
            case 0x8B:
                INSTR_CB_RES_REG8(cpu, 1, &cpu->regs.E);
            break;

            //RES 1, H
            case 0x8C:
                INSTR_CB_RES_REG8(cpu, 1, &cpu->regs.H);
            break;

            //RES 1, L
            case 0x8D:
                INSTR_CB_RES_REG8(cpu, 1, &cpu->regs.L);
            break;

            //RES 1, (HL)
            case 0x8E:
                INSTR_CB_RES_REG8(cpu, 1, &cpu->bus->data[cpu->regs.HL]);
            break;

            //RES 1, A
            case 0x8F:
                INSTR_CB_RES_REG8(cpu, 1, &cpu->regs.A);
            break;

            //RES 2, B
            case 0x90:
                INSTR_CB_RES_REG8(cpu, 2, &cpu->regs.B);
            break;

            //RES 2, C
            case 0x91:
                INSTR_CB_RES_REG8(cpu, 2, &cpu->regs.C);
            break;

            //RES 2, D
            case 0x92:
                INSTR_CB_RES_REG8(cpu, 2, &cpu->regs.D);
            break;

            //RES 2, E
            case 0x93:
                INSTR_CB_RES_REG8(cpu, 2, &cpu->regs.E);
            break;

            //RES 2, H
            case 0x94:
                INSTR_CB_RES_REG8(cpu, 2, &cpu->regs.H);
            break;

            //RES 2, L
            case 0x95:
                INSTR_CB_RES_REG8(cpu, 2, &cpu->regs.L);
            break;

            //RES 2, (HL)
            case 0x96:
                INSTR_CB_RES_REG8(cpu, 2, &cpu->bus->data[cpu->regs.HL]);

            break;

            //RES 2, A
            case 0x97:
                INSTR_CB_RES_REG8(cpu, 2, &cpu->regs.A);
            break;

            //RES 3, B
            case 0x98:
                INSTR_CB_RES_REG8(cpu, 3, &cpu->regs.B);
            break;

            //RES 3, C
            case 0x99:
                INSTR_CB_RES_REG8(cpu, 3, &cpu->regs.C);
            break;

            //RES 3, D
            case 0x9A:
                INSTR_CB_RES_REG8(cpu, 3, &cpu->regs.D);
            break;

            //RES 3, E
            case 0x9B:
                INSTR_CB_RES_REG8(cpu, 3, &cpu->regs.E);
            break;

            //RES 3, H
            case 0x9C:
                INSTR_CB_RES_REG8(cpu, 3, &cpu->regs.H);
            break;

            //RES 3, L
            case 0x9D:
                INSTR_CB_RES_REG8(cpu, 3, &cpu->regs.L);
            break;

            //RES 3, (HL)
            case 0x9E:
                INSTR_CB_RES_REG8(cpu, 3, &cpu->bus->data[cpu->regs.HL]);
            break;

            //RES 3, A
            case 0x9F:
                INSTR_CB_RES_REG8(cpu, 3, &cpu->regs.A);
            break;

            //RES 4, B
            case 0xA0:
                INSTR_CB_RES_REG8(cpu, 4, &cpu->regs.B);
            break;

            //RES 4, C
            case 0xA1:
                INSTR_CB_RES_REG8(cpu, 4, &cpu->regs.C);
            break;

            //RES 4, D
            case 0xA2:
                INSTR_CB_RES_REG8(cpu, 4, &cpu->regs.D);
            break;

            //RES 4, E
            case 0xA3:
                INSTR_CB_RES_REG8(cpu, 4, &cpu->regs.E);
            break;

            //RES 4, H
            case 0xA4:
                INSTR_CB_RES_REG8(cpu, 4, &cpu->regs.H);
            break;

            //RES 4, L
            case 0xA5:
                INSTR_CB_RES_REG8(cpu, 4, &cpu->regs.L);
            break;

            //RES 4, (HL)
            case 0xA6:
                INSTR_CB_RES_REG8(cpu, 4, &cpu->bus->data[cpu->regs.HL]);
            break;

            //RES 4, A
            case 0xA7:
                INSTR_CB_RES_REG8(cpu, 4, &cpu->regs.A);
            break;

            //RES 5, B
            case 0xA8:
                INSTR_CB_RES_REG8(cpu, 5, &cpu->regs.B);
            break;

            //RES 5, C
            case 0xA9:
                INSTR_CB_RES_REG8(cpu, 5, &cpu->regs.C);
            break;

            //RES 5, D
            case 0xAA:
                INSTR_CB_RES_REG8(cpu, 5, &cpu->regs.D);
            break;

            //RES 5, E
            case 0xAB:
                INSTR_CB_RES_REG8(cpu, 5, &cpu->regs.E);
            break;

            //RES 5, H
            case 0xAC:
                INSTR_CB_RES_REG8(cpu, 5, &cpu->regs.H);
            break;

            //RES 5, L
            case 0xAD:
                INSTR_CB_RES_REG8(cpu, 5, &cpu->regs.L);
            break;

            //RES 5, (HL)
            case 0xAE:
                INSTR_CB_RES_REG8(cpu, 5, &cpu->bus->data[cpu->regs.HL]);
            break;

            //RES 5, A
            case 0xAF:
                INSTR_CB_RES_REG8(cpu, 5, &cpu->regs.A);
            break;

            //RES 6, B
            case 0xB0:
                INSTR_CB_RES_REG8(cpu, 6, &cpu->regs.B);
            break;

            //RES 6, C
            case 0xB1:
                INSTR_CB_RES_REG8(cpu, 6, &cpu->regs.C);
            break;

            //RES 6, D
            case 0xB2:
                INSTR_CB_RES_REG8(cpu, 6, &cpu->regs.D);
            break;

            //RES 6, E
            case 0xB3:
                INSTR_CB_RES_REG8(cpu, 6, &cpu->regs.E);
            break;

            //RES 6, H
            case 0xB4:
                INSTR_CB_RES_REG8(cpu, 6, &cpu->regs.H);
            break;

            //RES 6, L
            case 0xB5:
                INSTR_CB_RES_REG8(cpu, 6, &cpu->regs.L);
            break;

            //RES 6, (HL)
            case 0xB6:
                INSTR_CB_RES_REG8(cpu, 6, &cpu->bus->data[cpu->regs.HL]);
            break;

            //RES 6, A
            case 0xB7:
                INSTR_CB_RES_REG8(cpu, 6, &cpu->regs.A);
            break;

            //RES 7, B
            case 0xB8:
                INSTR_CB_RES_REG8(cpu, 7, &cpu->regs.B);
            break;

            //RES 7, C
            case 0xB9:
                INSTR_CB_RES_REG8(cpu, 7, &cpu->regs.C);
            break;

            //RES 7, D
            case 0xBA:
                INSTR_CB_RES_REG8(cpu, 7, &cpu->regs.D);
            break;

            //RES 7, E
            case 0xBB:
                INSTR_CB_RES_REG8(cpu, 7, &cpu->regs.E);
            break;

            //RES 7, H
            case 0xBC:
                INSTR_CB_RES_REG8(cpu, 7, &cpu->regs.H);
            break;

            //RES 7, L
            case 0xBD:
                INSTR_CB_RES_REG8(cpu, 7, &cpu->regs.L);
            break;

            //RES 7, (HL)
            case 0xBE:
                INSTR_CB_RES_REG8(cpu, 7, &cpu->bus->data[cpu->regs.HL]);
            break;

            //RES 7, A
            case 0xBF:
                INSTR_CB_RES_REG8(cpu, 7, &cpu->regs.A);
            break;

            //SET 0, B
            case 0xC0:
                INSTR_CB_SET_REG8(cpu, 0, &cpu->regs.B);
            break;

            //SET 0, C
            case 0xC1:
                INSTR_CB_SET_REG8(cpu, 0, &cpu->regs.C);
            break;

            //SET 0, D
            case 0xC2:
                INSTR_CB_SET_REG8(cpu, 0, &cpu->regs.D);
            break;

            //SET 0, E
            case 0xC3:
                INSTR_CB_SET_REG8(cpu, 0, &cpu->regs.E);
            break;

            //SET 0, H
            case 0xC4:
                INSTR_CB_SET_REG8(cpu, 0, &cpu->regs.H);
            break;

            //SET 0, L
            case 0xC5:
                INSTR_CB_SET_REG8(cpu, 0, &cpu->regs.L);
            break;

            //SET 0, (HL)
            case 0xC6:
                INSTR_CB_SET_REG16PTR(cpu, 0, &cpu->regs.HL);
            break;

            //SET 0, A
            case 0xC7:
                INSTR_CB_SET_REG8(cpu, 0, &cpu->regs.A);
            break;

            //SET 1, B
            case 0xC8:
                INSTR_CB_SET_REG8(cpu, 1, &cpu->regs.B);
            break;

            //SET 1, C
            case 0xC9:
                INSTR_CB_SET_REG8(cpu, 1, &cpu->regs.C);
            break;

            //SET 1, D
            case 0xCA:
                INSTR_CB_SET_REG8(cpu, 1, &cpu->regs.D);
            break;

            //SET 1, E
            case 0xCB:
                INSTR_CB_SET_REG8(cpu, 1, &cpu->regs.E);
            break;

            //SET 1, H
            case 0xCC:
                INSTR_CB_SET_REG8(cpu, 1, &cpu->regs.H);
            break;

            //SET 1, L
            case 0xCD:
                INSTR_CB_SET_REG8(cpu, 1, &cpu->regs.L);
            break;

            //SET 1, (HL)
            case 0xCE:
                INSTR_CB_SET_REG16PTR(cpu, 1, &cpu->regs.HL);
            break;

            //SET 1, A
            case 0xCF:
                INSTR_CB_SET_REG8(cpu, 1, &cpu->regs.A);
            break;

            //SET 2, B
            case 0xD0:
                INSTR_CB_SET_REG8(cpu, 2, &cpu->regs.B);
            break;

            //SET 2, C
            case 0xD1:
                INSTR_CB_SET_REG8(cpu, 2, &cpu->regs.C);
            break;

            //SET 2, D
            case 0xD2:
                INSTR_CB_SET_REG8(cpu, 2, &cpu->regs.D);
            break;

            //SET 2, E
            case 0xD3:
                INSTR_CB_SET_REG8(cpu, 2, &cpu->regs.E);
            break;

            //SET 2, H
            case 0xD4:
                INSTR_CB_SET_REG8(cpu, 2, &cpu->regs.H);
            break;

            //SET 2, L
            case 0xD5:
                INSTR_CB_SET_REG8(cpu, 2, &cpu->regs.L);
            break;

            //SET 2, (HL)
            case 0xD6:
                INSTR_CB_SET_REG16PTR(cpu, 2, &cpu->regs.HL);
            break;

            //SET 2, A
            case 0xD7:
                INSTR_CB_SET_REG8(cpu, 2, &cpu->regs.A);
            break;

            //SET 3, B
            case 0xD8:
                INSTR_CB_SET_REG8(cpu, 3, &cpu->regs.B);
            break;

            //SET 3, C
            case 0xD9:
                INSTR_CB_SET_REG8(cpu, 3, &cpu->regs.C);
            break;

            //SET 3, D
            case 0xDA:
                INSTR_CB_SET_REG8(cpu, 3, &cpu->regs.D);
            break;

            //SET 3, E
            case 0xDB:
                INSTR_CB_SET_REG8(cpu, 3, &cpu->regs.E);
            break;

            //SET 3, H
            case 0xDC:
                INSTR_CB_SET_REG8(cpu, 3, &cpu->regs.H);
            break;

            //SET 3, L
            case 0xDD:
                INSTR_CB_SET_REG8(cpu, 3, &cpu->regs.L);
            break;

            //SET 3, (HL)
            case 0xDE:
                INSTR_CB_SET_REG16PTR(cpu, 3, &cpu->regs.HL);
            break;

            //SET 3, A
            case 0xDF:
                INSTR_CB_SET_REG8(cpu, 3, &cpu->regs.A);
            break;

            //SET 4, B
            case 0xE0:
                INSTR_CB_SET_REG8(cpu, 4, &cpu->regs.B);
            break;

            //SET 4, C
            case 0xE1:
                INSTR_CB_SET_REG8(cpu, 4, &cpu->regs.C);
            break;

            //SET 4, D
            case 0xE2:
                INSTR_CB_SET_REG8(cpu, 4, &cpu->regs.D);
            break;

            //SET 4, E
            case 0xE3:
                INSTR_CB_SET_REG8(cpu, 4, &cpu->regs.E);
            break;

            //SET 4, H
            case 0xE4:
                INSTR_CB_SET_REG8(cpu, 4, &cpu->regs.H);
            break;

            //SET 4, L
            case 0xE5:
                INSTR_CB_SET_REG8(cpu, 4, &cpu->regs.L);
            break;

            //SET 4, (HL)
            case 0xE6:
                INSTR_CB_SET_REG16PTR(cpu, 4, &cpu->regs.HL);
            break;

            //SET 4, A
            case 0xE7:
                INSTR_CB_SET_REG8(cpu, 4, &cpu->regs.A);
            break;

            //SET 5, B
            case 0xE8:
                INSTR_CB_SET_REG8(cpu, 5, &cpu->regs.B);
            break;

            //SET 5, C
            case 0xE9:
                INSTR_CB_SET_REG8(cpu, 5, &cpu->regs.C);
            break;

            //SET 5, D
            case 0xEA:
                INSTR_CB_SET_REG8(cpu, 5, &cpu->regs.D);
            break;

            //SET 5, E
            case 0xEB:
                INSTR_CB_SET_REG8(cpu, 5, &cpu->regs.E);
            break;

            //SET 5, H
            case 0xEC:
                INSTR_CB_SET_REG8(cpu, 5, &cpu->regs.H);
            break;

            //SET 5, L
            case 0xED:
                INSTR_CB_SET_REG8(cpu, 5, &cpu->regs.L);
            break;

            //SET 5, (HL)
            case 0xEE:
                INSTR_CB_SET_REG16PTR(cpu, 5, &cpu->regs.HL);
            break;

            //SET 5, A
            case 0xEF:
                INSTR_CB_SET_REG8(cpu, 5, &cpu->regs.A);
            break;

            //SET 6, B
            case 0xF0:
                INSTR_CB_SET_REG8(cpu, 6, &cpu->regs.B);
            break;

            //SET 6, C
            case 0xF1:
                INSTR_CB_SET_REG8(cpu, 6, &cpu->regs.C);
            break;

            //SET 6, D
            case 0xF2:
                INSTR_CB_SET_REG8(cpu, 6, &cpu->regs.D);
            break;

            //SET 6, E
            case 0xF3:
                INSTR_CB_SET_REG8(cpu, 6, &cpu->regs.E);
            break;

            //SET 6, H
            case 0xF4:
                INSTR_CB_SET_REG8(cpu, 6, &cpu->regs.H);
            break;

            //SET 6, L
            case 0xF5:
                INSTR_CB_SET_REG8(cpu, 6, &cpu->regs.L);
            break;

            //SET 6, (HL)
            case 0xF6:
                INSTR_CB_SET_REG16PTR(cpu, 6, &cpu->regs.HL);
            break;

            //SET 6, A
            case 0xF7:
                INSTR_CB_SET_REG8(cpu, 6, &cpu->regs.A);
            break;

            //SET 7, B
            case 0xF8:
                INSTR_CB_SET_REG8(cpu, 7, &cpu->regs.B);
            break;

            //SET 7, C
            case 0xF9:
                INSTR_CB_SET_REG8(cpu, 7, &cpu->regs.C);
            break;

            //SET 7, D
            case 0xFA:
                INSTR_CB_SET_REG8(cpu, 7, &cpu->regs.D);
            break;

            //SET 7, E
            case 0xFB:
                INSTR_CB_SET_REG8(cpu, 7, &cpu->regs.E);
            break;

            //SET 7, H
            case 0xFC:
                INSTR_CB_SET_REG8(cpu, 7, &cpu->regs.H);
            break;

            //SET 7, L
            case 0xFD:
                INSTR_CB_SET_REG8(cpu, 7, &cpu->regs.L);
            break;

            //SET 7, (HL)
            case 0xFE:
                INSTR_CB_SET_REG16PTR(cpu, 7, &cpu->regs.HL);
            break;

            //SET 7, A
            case 0xFF:
                INSTR_CB_SET_REG8(cpu, 7, &cpu->regs.A);
            break;

            default:
                printf("Unimplemented CB! %04x\n", secondOpcode);
            break;
        }

    break;

    //CALL Z, u16
    case 0xCC:
        INSTR_CALL_IMM16(cpu, cpu->regs.z == 1);
    break;

    //CALL u16
    case 0xCD:
        INSTR_CALL_IMM16(cpu, 1);
    break;

    //ADC A, d8
    case 0xCE:
    {
        uint8_t val = FETCH_U8;
        INSTR_ADC_REG8_REG8(cpu, &cpu->regs.A, &val);
    }
    break;

    //RST 8
    case 0xCF:
        cpu_stack_push_16(cpu, cpu->pc);
        cpu->pc = 0x8;
        cpu->cyclesToWait = 16;
    break;

    //RET NC
    case 0xD0:
        INSTR_RET(cpu, cpu->regs.c == 0);
    break;

    //POP DE
    case 0xD1:
        cpu->regs.DE = cpu_stack_pop_16(cpu);
        cpu->cyclesToWait = 12;
    break;

    //JP NC, u16
    case 0xD2:
        INSTR_JP_IMM16(cpu, cpu->regs.c == 0);
    break;

    //Undefined
    case 0xD3:
        printf("Undefined opcode 0xD3\n");
    break;

    //CALL NC, u16
    case 0xD4:
        INSTR_CALL_IMM16(cpu, cpu->regs.c == 0);
    break;

    //PUSH DE
    case 0xD5:
        cpu_stack_push_16(cpu, cpu->regs.DE);
        cpu->cyclesToWait = 16;
    break;

    //SUB u8
    case 0xD6:
    {
        uint8_t val = FETCH_U8;
        INSTR_SUB_REG8_REG8(cpu, &cpu->regs.A, &val);
    }
    break;

    //RST 10
    case 0xD7:
        cpu_stack_push_16(cpu, cpu->pc);
        cpu->pc = 0x10;
        cpu->cyclesToWait = 16;
    break;

    //RET C
    case 0xD8:
        INSTR_RET(cpu, cpu->regs.c == 1);
    break;

    //RETI
    case 0xD9:
        INSTR_RET(cpu, 1);
        //cpu->IMEDelayCount = 2;
        cpu->interruptLevel--;

        //Only re-enable interrupts if we aren't still in one
        if (cpu->interruptLevel == 0)
        {
            cpu->IME = 1;
        }
    break;

    //JP C, u16
    case 0xDA:
        INSTR_JP_IMM16(cpu, cpu->regs.c == 1);
    break;

    //Undefined
    case 0xDB:
        printf("Undefined opcode 0xDB\n");
    break;

    //CALL C, u16
    case 0xDC:
        INSTR_CALL_IMM16(cpu, cpu->regs.c == 1);
    break;

    //Undefined
    case 0xDD:
        printf("Undefined opcode 0xDD\n");
    break;

    //SBC A, d8
    case 0xDE:
    {
        uint8_t val = FETCH_U8;
        INSTR_SBC_REG8_REG8(cpu, &cpu->regs.A, &val);
    }
    break;

    //RST 0x18
    case 0xDF:
        cpu_stack_push_16(cpu, cpu->pc);
        cpu->pc = 0x18;
        cpu->cyclesToWait = 16;
    break;

    //LD (u8), A
    case 0xE0:
        INSTR_LD_IMM8PTR_REG8(cpu, &cpu->regs.A);
    break;

    //POP HL
    case 0xE1:
        cpu->regs.HL = cpu_stack_pop_16(cpu);
        cpu->cyclesToWait = 12;
    break;

    //LD (C), A
    case 0xE2:
        INSTR_LD_REG8PTR_REG8(cpu, &cpu->regs.C, &cpu->regs.A);
    break;

    //Undefined
    case 0xE3:
        printf("Undefined opcode 0xE3\n");
    break;

    //Undefined
    case 0xE4:
        printf("Undefined opcode 0xE4\n");
    break;

    //PUSH HL
    case 0xE5:
        cpu_stack_push_16(cpu, cpu->regs.HL);
        cpu->cyclesToWait = 16;
    break;

    //AND u8
    case 0xE6:
    {
        uint8_t val = FETCH_U8;
        INSTR_AND_REG8_REG8(cpu, &cpu->regs.A, &val);
    }
    break;

    //RST 0x20
    case 0xE7:
        cpu_stack_push_16(cpu, cpu->pc);
        cpu->pc = 0x20;
        cpu->cyclesToWait = 16;
    break;

    //ADD SP, u8
    case 0xE8:
        INSTR_ADD_REG16_IMM8(cpu, &cpu->sp);
    break;

    //JP HL
    case 0xE9:
        cpu->pc = cpu->regs.HL;
        cpu->cyclesToWait = 4;
    break;

    //LD (u16), A
    case 0xEA:
        INSTR_LD_IMM16PTR_REG8(cpu, &cpu->regs.A);
    break;

    //XOR u8
    case 0XEE:
    {
        uint8_t val = FETCH_U8;
        INSTR_XOR_REG8_REG8(cpu, &cpu->regs.A, &val);
    }
    break;

    //RST 0x28
    case 0xEF:
        cpu_stack_push_16(cpu, cpu->pc);
        cpu->pc = 0x28;
        cpu->cyclesToWait = 16;
    break;

    //LD A, (u8)
    case 0xF0:
    {
        uint8_t val = FETCH_U8;
        cpu->regs.A = bus_read(cpu->bus, 0xFF00 + val);
        cpu->cyclesToWait = 12;

    }
    break;

    //POP AF
    case 0xF1:
        cpu->regs.AF = cpu_stack_pop_16(cpu);
        cpu->regs.F &= 0xF0;
        cpu->cyclesToWait = 12;
    break;

    //LD A, (C)
    case 0xF2:
        cpu->regs.A = bus_read(cpu->bus, 0xFF00 + cpu->regs.C);
        cpu->cyclesToWait = 8;
    break;

    //DI
    case 0xF3:
        cpu->IME = 0;
        cpu->cyclesToWait = 4;
        //printf("Disabled! @ %04x\n", cpu->pc);
    break;

    //PUSH AF
    case 0xF5:
        cpu_stack_push_16(cpu, cpu->regs.AF);
        cpu->cyclesToWait = 16;
    break;

    //OR u8
    case 0xF6:
    {
        uint8_t val = FETCH_U8;
        INSTR_OR_REG8_REG8(cpu, &cpu->regs.A, &val);
    }
    break;

    //RST 0x30
    case 0xF7:
        cpu_stack_push_16(cpu, cpu->pc);
        cpu->pc = 0x30;
        cpu->cyclesToWait = 16;
    break;

    //LD HL, SP + u8
    case 0xF8:
    {
        int8_t val = (int8_t)FETCH_U8;

        cpu->regs.c = ((cpu->sp ^ (val) ^ ((cpu->sp + val) & 0xFFFF)) & 0x100) == 0x100;
        cpu->regs.HL = cpu->sp + val;
        cpu->regs.z = 0;
        cpu->regs.n = 0;
        //cpu->regs.c = ((uint32_t)(val + cpu->sp) > 0xFFFF); //Maybe?
        cpu->regs.h = HALF_CARRY_ADD(cpu->sp, val);
        cpu->cyclesToWait = 12;
    }
    break;

    //LD SP, HL
    case 0xF9:
        cpu->sp = cpu->regs.HL;
        cpu->cyclesToWait = 8;
    break;

    //LD A, (u16)
    case 0xFA:
    {
        uint16_t val = (bus_read(cpu->bus, cpu->pc++)) | (bus_read(cpu->bus, cpu->pc++) << 8);
        cpu->regs.A = bus_read(cpu->bus, val);
        cpu->cyclesToWait = 8;
        //INSTR_LD_REG8_REG16PTR(cpu, &cpu->regs.A, &val);
    }
    break;

    /*
        The IME flag is reset immediately after an interrupt occurs. The IME flag reset remains in effect if coontrol is returned from the interrupt routine by a RET instruction. However, if an EI instruction is executed in the interrupt routine, control is returned with IME = 1.
    */
    //EI
    case 0xFB:
        //cpu->IMEDelayCount = 2;
        //printf("Enabled\n");
        cpu->IME = 1;
        cpu->cyclesToWait = 4;
    break;

    //CP d8
    case 0xFE:
        {
            uint8_t val = FETCH_U8;
            INSTR_CP_REG8(cpu, &val);
            cpu->cyclesToWait = 8;
        }
    break;

    //RST
    case 0xFF:
        cpu_stack_push_16(cpu, cpu->pc);
        cpu->pc = 0x38;
    break;

    default:
        printf("Unknown opcode %02x @ %04x\n", opcode, cpu->pc-1);
    break;
    }
}

void cpu_stack_push_8(Z80_t* cpu, uint8_t val)
{
    bus_write(cpu->bus, cpu->sp--, val);
}

void cpu_stack_push_16(Z80_t* cpu, uint16_t val)
{
    cpu->sp--;
    bus_write(cpu->bus, cpu->sp, (val >> 8) & 0xFF);
    cpu->sp--;
    bus_write(cpu->bus, cpu->sp, (val) & 0xFF);

    //printf("PUSHED %04x\n", val);
    
}

uint8_t cpu_stack_pop_8(Z80_t* cpu)
{
    cpu->sp++;
    return bus_read(cpu->bus, cpu->sp);
}

uint16_t cpu_stack_pop_16(Z80_t* cpu)
{
    uint16_t retVal;

    retVal = (bus_read(cpu->bus, cpu->sp) & 0x00FF);
    cpu->sp++;
    retVal |= (bus_read(cpu->bus, cpu->sp) << 8);
    cpu->sp++;

    //cpu->sp += 2;

    return retVal;
}

void INSTR_INC_16(Z80_t* cpu, uint16_t* reg)
{
    *reg = *reg + 1;

    cpu->cyclesToWait = 8;
}

void INSTR_INC_8(Z80_t* cpu, uint8_t* reg)
{
    //cpu->regs.h = HALF_CARRY_ADD(*reg, 1);

    uint8_t nibBefore = *reg >> 4;

    *reg = *reg + 1;

    uint8_t nibAfter = *reg >> 4;

    cpu->regs.h = nibBefore != nibAfter;

    cpu->regs.z = *reg == 0;
    cpu->regs.n = 0;

    cpu->cyclesToWait = 4;

}

void INSTR_INC_REG16PTR(Z80_t* cpu, uint16_t* reg)
{
    uint8_t mem = bus_read(cpu->bus, *reg);
    cpu->regs.h = HALF_CARRY_ADD(mem, 1);
    cpu->regs.n = 0;
    mem++;
    cpu->regs.z = mem == 0;

    bus_write(cpu->bus, *reg, mem);

    cpu->cyclesToWait = 12;
}

void INSTR_DEC_REG16PTR(Z80_t* cpu, uint16_t* reg)
{
    uint8_t mem = bus_read(cpu->bus, *reg);

    uint8_t old = mem;

    //cpu->regs.h = HALF_CARRY_SUB(mem, 1);
    cpu->regs.n = 1;
    mem--;

    cpu->regs.z = mem == 0;

    bus_write(cpu->bus, *reg, mem);

    if (((old >> 4) & 1) != ((mem >> 4) & 1))
    {
        cpu->regs.h = 1;
    }
    else
    {
        cpu->regs.h = 0;
    }

    cpu->cyclesToWait = 12;
}

void INSTR_DEC_16(Z80_t* cpu, uint16_t* reg)
{
    *reg = *reg - 1;

    cpu->cyclesToWait = 8;
}

void INSTR_DEC_8(Z80_t* cpu, uint8_t* reg)
{
    //cpu->regs.h = HALF_CARRY_SUB(*reg, 1);

    uint8_t oldVal = *reg;

    *reg = *reg - 1;

    if (((oldVal >> 4) & 1) != ((*reg >> 4) & 1))
    {
        cpu->regs.h = 1;
    }
    else
    {
        cpu->regs.h = 0;
    }

    cpu->regs.z = *reg == 0;
    cpu->regs.n = 1;

    cpu->cyclesToWait = 4;
}

void INSTR_ADD_REG16_REG16(Z80_t* cpu, uint16_t* regA, uint16_t* regB)
{
    uint16_t val = (*regA & 0xFFF) + (*regB & 0xFFF);
    cpu->regs.h = (val & (1 << 12)) != 0;

    uint16_t oldVal = *regA;
    *regA = *regA + *regB;

    cpu->regs.c = oldVal > *regA;
    cpu->regs.n = 0;

    //cpu->regs.h = HALF_CARRY_ADD_16(oldVal, *regB);

    cpu->cyclesToWait = 8;
}

void INSTR_ADD_REG16_IMM8(Z80_t* cpu, uint16_t* regA)
{
    int8_t immVal = (int8_t)bus_read(cpu->bus, cpu->pc++);
    uint16_t finalVal = *regA + immVal;

    cpu->regs.z = 0;
    cpu->regs.n = 0;
    cpu->regs.h = HALF_CARRY_ADD(*regA & 0xFF, immVal);
    cpu->regs.c = ((*regA ^ immVal ^ (finalVal & 0xFFFF)) & 0x100) == 0x100;

    *regA = finalVal;

    cpu->cyclesToWait = 16;
}

void INSTR_ADD_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB)
{
    uint8_t oldVal = *regA;
    *regA = *regA + *regB;

    cpu->regs.z = *regA == 0;
    cpu->regs.c = oldVal > *regA;
    cpu->regs.n = 0;

    // if (((oldVal >> 4)) != ((*regA >> 4)))
    // {
    //     cpu->regs.h = 1;
    // }
    // else
    // {
    //     cpu->regs.h = 0;
    // }

    cpu->regs.h = (((oldVal & 0xf) + (*regB & 0xf)) & 0x10) == 0x10;

    cpu->cyclesToWait = 4;
}

void INSTR_ADC_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB)
{
    uint16_t result = *regA + *regB + cpu->regs.c;

    cpu->regs.n = 0;
    cpu->regs.h = (((*regA & 0xf) + (*regB & 0xf) + (cpu->regs.c & 0xf)) & 0x10) == 0x10;
    cpu->regs.c = (result & 0xFF00) ? 1 : 0;

    *regA = result & 0xFF;
    cpu->regs.z = *regA == 0;

    cpu->cyclesToWait = 4;
}


void INSTR_ADD_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB)
{
    uint8_t oldVal = *regA;
    *regA = *regA + bus_read(cpu->bus, *regB);

    cpu->regs.z = *regA == 0;
    cpu->regs.c = oldVal > *regA;
    cpu->regs.n = 0;
    cpu->regs.h = HALF_CARRY_ADD(oldVal, bus_read(cpu->bus, *regB));

    cpu->cyclesToWait = 8;
}

void INSTR_ADC_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB)
{
    uint8_t val = bus_read(cpu->bus, *regB);

    cpu->regs.n = 0;
    cpu->regs.h = (((*regA & 0xf) + (val & 0xf) + (cpu->regs.c & 0xf)) & 0x10) == 0x10;

    uint8_t oldRegA = *regA;
    *regA = *regA + val + cpu->regs.c;

    cpu->regs.c = ((uint16_t)(oldRegA + val + cpu->regs.c)) & 0xFF00 ? 1 : 0;
    cpu->regs.z = *regA == 0;

    cpu->cyclesToWait = 8;
}

void INSTR_SUB_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB)
{
    uint16_t oldVal = *regA;
    *regA = *regA - *regB;

    cpu->regs.c = oldVal < *regB;
    cpu->regs.n = 1;

    //cpu->regs.h = !HALF_CARRY_SUB(oldVal, *regB);

    cpu->regs.z = *regA == 0;

    cpu->regs.h = HALF_CARRY_SUB(oldVal, *regB);

    cpu->cyclesToWait = 4;
}

void INSTR_SUB_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB)
{
    uint16_t oldVal = *regA;
    *regA = *regA - bus_read(cpu->bus, *regB);

    cpu->regs.c = oldVal < bus_read(cpu->bus, *regB);
    cpu->regs.n = 1;
    cpu->regs.h = HALF_CARRY_SUB(oldVal, bus_read(cpu->bus, *regB));
    cpu->regs.z = *regA == 0;

    cpu->cyclesToWait = 8;
}

void INSTR_SBC_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB)
{
    uint16_t oldVal = *regA;
    *regA = *regA - (*regB + cpu->regs.c);

    cpu->regs.n = 1;
    cpu->regs.h = (((oldVal & 0xf) - (*regB & 0xf) - (cpu->regs.c)) & 0x10) == 0x10;
    cpu->regs.z = *regA == 0;
    cpu->regs.c = oldVal < *regB + cpu->regs.c;

    cpu->cyclesToWait = 4;
}

void INSTR_SBC_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB)
{
    uint8_t imm = bus_read(cpu->bus, *regB);
    uint16_t oldVal = *regA;

    cpu->regs.h = (*regA ^ imm ^ (*regA - imm - cpu->regs.c)) & 0x10 ? 1 : 0;

    *regA = *regA - imm - cpu->regs.c;

    cpu->regs.c = oldVal < (imm + cpu->regs.c); //Probably right
    cpu->regs.n = 1;
    cpu->regs.z = *regA == 0;

    cpu->cyclesToWait = 8;
}

void INSTR_LD_REG8_IMM8(Z80_t* cpu, uint8_t* reg)
{
    *reg = bus_read(cpu->bus, cpu->pc++);
    cpu->cyclesToWait = 8;
}

void INSTR_LD_IMMPTR_REG16(Z80_t* cpu, uint16_t* reg)
{
    uint16_t addr = (bus_read(cpu->bus, cpu->pc + 1) << 8) | bus_read(cpu->bus, cpu->pc);
    bus_write(cpu->bus, addr, *reg & 0xFF);
    bus_write(cpu->bus, addr + 1, *reg >> 8);
    cpu->pc += 2;
    cpu->cyclesToWait = 20;
}

void INSTR_LD_IMM16PTR_REG8(Z80_t* cpu, uint8_t* reg)
{
    bus_write(cpu->bus, FETCH_U16, *reg);
    cpu->cyclesToWait = 16;
}

void INSTR_LD_IMM8PTR_REG8(Z80_t* cpu, uint8_t* reg)
{
    uint8_t val = bus_read(cpu->bus, cpu->pc++);
    bus_write(cpu->bus, 0xFF00 + val, *reg);
    cpu->cyclesToWait = 12;
}

void INSTR_LD_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB)
{
    *regA = bus_read(cpu->bus, *regB);
    cpu->cyclesToWait = 8;
}

void INSTR_LD_REG16_IMM16(Z80_t* cpu, uint16_t* reg)
{
    *reg = FETCH_U16;
    cpu->cyclesToWait += 12;
}

void INSTR_LD_REG16PTR_REG8(Z80_t* cpu, uint16_t* regA, uint8_t* regB)
{
    bus_write(cpu->bus, *regA, *regB);
    cpu->cyclesToWait += 8;
}

void INSTR_LD_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB)
{
    *regA = *regB;
    cpu->cyclesToWait = 4;
}

void INSTR_LD_REG8PTR_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB)
{
    bus_write(cpu->bus, 0xFF00 + *regA, *regB);
    cpu->cyclesToWait = 8;
}

void INSTR_LD_REG16PTR_IMM8(Z80_t* cpu, uint16_t* reg)
{
    bus_write(cpu->bus, *reg, bus_read(cpu->bus, cpu->pc++));
    cpu->cyclesToWait = 12;
}

void INSTR_JR_IMM8(Z80_t* cpu, uint8_t condition)
{
    cpu->cyclesToWait = 8;
    if (condition == 1)
    {
        cpu->pc += (int8_t) bus_read(cpu->bus, cpu->pc++);//Hmmmmm
        //printf("%d\n", (int8_t) bus_read(cpu->bus, cpu->pc - 1));
        cpu->cyclesToWait += 4;
    }
    else
    {
        cpu->pc++;
    }
}

void INSTR_AND_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB)
{
    *regA = *regA & *regB;
    cpu->regs.z = *regA == 0;
    cpu->regs.n = 0;
    cpu->regs.h = 1;
    cpu->regs.c = 0;

    cpu->cyclesToWait = 4;
}


void INSTR_AND_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB)
{
    *regA = *regA & bus_read(cpu->bus, *regB);
    cpu->regs.z = *regA == 0;
    cpu->regs.n = 0;
    cpu->regs.h = 1;
    cpu->regs.c = 0;

    cpu->cyclesToWait = 4;
}

void INSTR_XOR_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB)
{
    *regA = *regA ^ *regB;
    cpu->regs.z = *regA == 0;
    cpu->regs.n = 0;
    cpu->regs.h = 0;
    cpu->regs.c = 0;

    cpu->cyclesToWait = 4;
}


void INSTR_XOR_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB)
{
    *regA = *regA ^ bus_read(cpu->bus, *regB);
    cpu->regs.z = *regA == 0;
    cpu->regs.n = 0;
    cpu->regs.h = 0;
    cpu->regs.c = 0;

    cpu->cyclesToWait = 4;
}

void INSTR_OR_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB)
{
    *regA = *regA | *regB;
    cpu->regs.z = *regA == 0;
    cpu->regs.n = 0;
    cpu->regs.h = 0;
    cpu->regs.c = 0;

    cpu->cyclesToWait = 4;
}


void INSTR_OR_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB)
{
    *regA = *regA | bus_read(cpu->bus, *regB);
    cpu->regs.z = *regA == 0;
    cpu->regs.n = 0;
    cpu->regs.h = 0;
    cpu->regs.c = 0;

    cpu->cyclesToWait = 4;
}

void INSTR_CP_REG8(Z80_t* cpu, uint8_t* reg)
{
    cpu->regs.c = *reg > cpu->regs.A;
    cpu->regs.n = 1;
    //cpu->regs.h = HALF_CARRY_SUB(cpu->regs.A, *reg);
    cpu->regs.z = (cpu->regs.A - *reg) == 0;


    uint8_t aNib = cpu->regs.A & 0xF;
    uint8_t regNib = *reg & 0xF;

    cpu->regs.h = regNib > aNib;

    cpu->cyclesToWait = 4;
}

void INSTR_CP_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB)
{
    cpu->regs.c = bus_read(cpu->bus, *regB) > *regA;
    cpu->regs.n = 1;
    cpu->regs.h = HALF_CARRY_SUB(*regA, bus_read(cpu->bus, *regB));
    cpu->regs.z = (*regA - bus_read(cpu->bus, *regB)) == 0;

    cpu->cyclesToWait = 8;
}

void INSTR_RET(Z80_t* cpu, uint8_t condition)
{
    cpu->cyclesToWait = 8;

    if (condition > 0)
    {
        cpu->pc = cpu_stack_pop_16(cpu);
        //printf("Return to %04x\n", cpu->pc);
        cpu->cyclesToWait = 20;
    }
    else
    {
        //cpu->pc += 2; //HMMMMMMMMM
    }
}

void INSTR_JP_IMM16(Z80_t* cpu, uint8_t condition)
{
    cpu->cyclesToWait = 12;

    if (condition > 0)
    {
        cpu->pc = FETCH_U16;
        cpu->cyclesToWait = 16;
    }
    else
    {
        cpu->pc += 2;
    }
}

void INSTR_CALL_IMM16(Z80_t* cpu, uint8_t condition)
{
    cpu->cyclesToWait = 12;

    if (condition > 0)
    {
        cpu_stack_push_16(cpu, cpu->pc + 2);
        cpu->pc = FETCH_U16;
        cpu->cyclesToWait = 24;
    }
    else
    {
        cpu->pc += 2;
    }
}

uint8_t rotateRight(uint8_t val)
{
    uint8_t lsb = val & 1;

    uint8_t ret = val >> 1;
    ret = ret | (lsb << 7);

    return ret;
}

uint8_t rotateLeft(uint8_t val)
{
    uint8_t ret = val;
    ret = (ret << 1) | (ret >> 7);

    return ret;
}

void INSTR_CB_RR_REG8(Z80_t* cpu, uint8_t* reg)
{
    uint8_t carry = cpu->regs.c;
    cpu->regs.c = *reg & 1;

    *reg = *reg >> 1;
    *reg |= (carry << 7);

    cpu->regs.n = 0;
    cpu->regs.h = 0;
    cpu->regs.z = *reg == 0;

    cpu->cyclesToWait = 8;
}

void INSTR_CB_RL_REG8(Z80_t* cpu, uint8_t* reg)
{
    uint8_t carry = cpu->regs.c;
    cpu->regs.c = (*reg >> 7) & 1;

    *reg = *reg << 1;
    *reg |= carry;

    cpu->regs.n = 0;
    cpu->regs.h = 0;
    cpu->regs.z = *reg == 0;

    cpu->cyclesToWait = 8;
}

void INSTR_CB_SRL_REG8(Z80_t* cpu, uint8_t* reg)
{
    cpu->regs.c = *reg & 1;

    *reg = *reg >> 1;

    cpu->regs.n = 0;
    cpu->regs.h = 0;
    cpu->regs.z = *reg == 0;

    cpu->cyclesToWait = 8;
}

void INSTR_CB_SWAP_REG8(Z80_t* cpu, uint8_t* reg)
{
    uint8_t lowNibble = (*reg & 0xF) << 4;
    *reg = *reg >> 4;
    *reg = *reg | lowNibble;

    cpu->regs.n = 0;
    cpu->regs.h = 0;
    cpu->regs.c = 0;
    cpu->regs.z = *reg == 0;

    cpu->cyclesToWait = 8;
}

void INSTR_CB_RES_REG8(Z80_t* cpu, uint8_t bit, uint8_t* reg)
{
    *reg &= ~(1 << bit);

    cpu->cyclesToWait = 8;
}

void INSTR_CB_RLC_REG8(Z80_t* cpu, uint8_t* reg)
{
    cpu->regs.c = (*reg >> 7) & 1;
    *reg = (*reg << 1) | cpu->regs.c;
    cpu->regs.z = *reg == 0;
    cpu->regs.n = 0;
    cpu->regs.h = 0;

    cpu->cyclesToWait = 8;
}

void INSTR_CB_RLC_REG16PTR(Z80_t* cpu, uint16_t* reg)
{
    uint8_t val = bus_read(cpu->bus, *reg);
    cpu->regs.c = (val >> 7) & 1;
    uint8_t result = ((val << 1) | cpu->regs.c);
    cpu->regs.z = result == 0;
    bus_write(cpu->bus, *reg, result);
     cpu->regs.n = 0;
    cpu->regs.h = 0;

    cpu->cyclesToWait = 16;
}

void INSTR_CB_RRC_REG8(Z80_t* cpu, uint8_t* reg)
{
    cpu->regs.c = *reg & 1;
    *reg = (*reg >> 1) | (*reg << 7);
    cpu->regs.z = *reg == 0;
    cpu->regs.n = 0;
    cpu->regs.h = 0;

    cpu->cyclesToWait = 8;
}

void INSTR_CB_RRC_REG16PTR(Z80_t* cpu, uint16_t* reg)
{
    uint8_t val = bus_read(cpu->bus, *reg);
    cpu->regs.c = val & 1;
    uint8_t result = ((val >> 1) | (cpu->regs.c << 7));
    cpu->regs.z = result == 0;
    bus_write(cpu->bus, *reg, result);
    cpu->regs.n = 0;
    cpu->regs.h = 0;

    cpu->cyclesToWait = 16;
}

void INSTR_BIT_IMM8_REG8(Z80_t* cpu, uint8_t bit, uint8_t* reg)
{
    cpu->regs.z = !((*reg >> bit) & 1);
    cpu->regs.n = 0;
    cpu->regs.h = 1;

    cpu->cyclesToWait = 8;
}

void INSTR_CB_SLA_REG8(Z80_t* cpu, uint8_t* reg)
{
    cpu->regs.c = (*reg >> 7) & 1;
    cpu->regs.n = 0;
    cpu->regs.h = 0;
    *reg = *reg << 1;
    cpu->regs.z = *reg == 0;

    cpu->cyclesToWait = 8;
}

void INSTR_CB_SRA_REG8(Z80_t* cpu, uint8_t* reg)
{
    cpu->regs.c = *reg & 1;
    cpu->regs.n = 0;
    cpu->regs.h = 0;
    *reg = (*reg >> 1) | (*reg & 0x80);
    cpu->regs.z = *reg == 0;

    cpu->cyclesToWait = 8;
}

void INSTR_CB_SET_REG8(Z80_t* cpu, uint8_t bit, uint8_t* reg)
{
    *reg = *reg | (1 << bit);
}

void INSTR_CB_SET_REG16PTR(Z80_t* cpu, uint8_t bit, uint16_t* reg)
{
    uint8_t val = bus_read(cpu->bus, *reg);
    bus_write(cpu->bus, *reg, val | (1 << bit));
}