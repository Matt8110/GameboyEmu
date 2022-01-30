#include "CPU.h"

CPU::CPU(BUS* bus, Interrupts* interrupts)
{
    this->bus = bus;
    this->interrupts = interrupts;
}

void CPU::run()
{
    cyclesThisFrame = 0;

    while (running && cyclesThisFrame <= CLOCKS_PER_FRAME)
    {
        clock();
        cyclesThisFrame++;
    }
}

void CPU::clock()
{

    if (cycleCount == 0)
    {

        //Handle interrupts
        u16 intAdd = interrupts->handleInterrupts();
        if (intAdd > 0)
        {
            //Push current address
            //Call intAdd;
        }

        u8 opcode = bus->readByte(pc);
        currentInstruction = &instructionSet[opcode];
        cycleCount += executeInstruction();

        if (!jumped)
            pc += currentInstruction->length;
        else
            jumped = false;
    }

    cycleCount--;
}

u8 CPU::executeInstruction()
{
    switch(currentInstruction->opcode)
    {
        //NOP
        case 0x00:
            return currentInstruction->cycles;
        break;

        //LD BC, d16
        case 0x01:
            return LD_R16_D16(regs.BC);
        break;

        //LD (BC), A
        case 0x02:
            return LD_MEM_R16_R8(regs.BC, regs.A);
        break;

        //INC BC
        case 0x03:
            return INC_R16(regs.BC);
        break;

        //INC B
        case 0x04:
            return INC_R8(regs.B);
        break;

        //DEC B
        case 0x05:
            return DEC_R8(regs.B);
        break;

        //LD B, u8
        case 0x06:
            return LD_R8_D8(regs.B);
        break;

        //RLCA
        case 0x07:
            return RLCA();
        break;

        //LD (a16), SP
        case 0x08:
            return LD_MEM_A16_R16(sp);
        break;

        //ADD HL, BC
        case 0x09:
            return ADD_R16_R16(regs.HL, regs.BC);
        break;

        //LD, A, (BC)
        case 0x0A:
            return LD_R8_MEM_R16(regs.A, regs.BC);
        break;

        //DEC BC
        case 0x0B:
            return DEC_R16(regs.BC);
        break;

        //INC C
        case 0x0C:
            return INC_R8(regs.C);
        break;

        //DEC C
        case 0x0D:
            return DEC_R8(regs.C);
        break;

        //LD C, d8
        case 0x0E:
            return LD_R8_D8(regs.C);
        break;

        //RRCA
        case 0x0F:
            return RRCA();
        break;

        //STOP
        case 0x10:
            return currentInstruction->cycles; //Does nothing?
        break;

        //LD DE, d16
        case 0x11:
            return LD_R16_D16(regs.DE);
        break;

        //LD (DE), A
        case 0x12:
            return LD_MEM_R16_R8(regs.DE, regs.A);
        break;

        //INC DE
        case 0x13:
            return INC_R16(regs.DE);
        break;

        //INC D
        case 0x14:
            return INC_R8(regs.D);
        break;

        //DEC D
        case 0x15:
            return DEC_R8(regs.D);
        break;

        //LD D, d8
        case 0x16:
            return LD_R8_D8(regs.D);
        break;

        //RLA
        case 0x17:
            return RLA();
        break;

        //JR, d8 -- Signed
        case 0x18:
            return JR_D8();
        break;

        //ADD HL, DE
        case 0x19:
            return ADD_R16_R16(regs.HL, regs.DE);
        break;

        //LD A, (DE)
        case 0x1A:
            return LD_R8_MEM_R16(regs.A, regs.DE);
        break;

        //DEC DE
        case 0x1B:
            return DEC_R16(regs.DE);
        break;

        //INC E
        case 0x1C:
            return INC_R8(regs.E);
        break;

        //DEC E
        case 0x1D:
            return DEC_R8(regs.E);
        break;

        //LD E, d8
        case 0x1E:
            return LD_R8_D8(regs.E);
        break;

        //RRA
        case 0x1F:
            return RRA();
        break;

        //JR NZ, d8
        case 0x20:
            return JR_CC_D8(readFlag(F_Z) == 0);
        break;

        //LD HL, d16
        case 0x21:
            return LD_R16_D16(regs.HL);
        break;

        //LD (HL+), A
        case 0x22:
        {
             u8 c = LD_MEM_R16_R8(regs.HL, regs.A);
             regs.HL++;
             return c;
        }
        break;

        //INC HL
        case 0x23:
            return INC_R16(regs.HL);
        break;

        //INC H
        case 0x24:
            return INC_R8(regs.H);
        break;

        //DEC H
        case 0x25:
            return DEC_R8(regs.H);
        break;

        //LD H, d8
        case 0x26:
            return LD_R8_D8(regs.H);
        break;

        //DAA
        case 0x27:
            return DAA();
        break;

        //JR Z, D8
        case 0x28:
            return JR_CC_D8(readFlag(F_Z));
        break;

        //ADD HL, HL
        case 0x29:
            return ADD_R16_R16(regs.HL, regs.HL);
        break;

        //LD A, (HL+)
        case 0x2A:
        {
            u8 c = LD_R8_MEM_R16(regs.A, regs.HL);
            regs.HL++;
            return c;
        }
        break;

        //DEC HL
        case 0x2B:
            return DEC_R16(regs.HL);
        break;

        //INC L
        case 0x2C:
            return INC_R8(regs.L);
        break;

        //DEC L
        case 0x2D:
            return DEC_R8(regs.L);
        break;

        //LD L, d8
        case 0x2E:
            return LD_R8_D8(regs.L);
        break;

        //CPL
        case 0x2F:
            return CPL();
        break;

        //JR NC, d8
        case 0x30:
            return JR_CC_D8(readFlag(F_C) == 0);
        break;

        //LD SP, d16
        case 0x31:
            return LD_R16_D16(sp);
        break;

        //LD (HL-), A
        case 0x32:
        {
            u8 c = LD_MEM_R16_R8(regs.HL, regs.A);
            regs.HL--;
            return c;
        }
        break;

        //INC SP
        case 0x33:
            return INC_R16(sp);
        break;

        //INC (HL)
        case 0x34:
            return INC_MEM_R16(regs.HL);
        break;

        //DEC (HL)
        case 0x35:
            return DEC_MEM_R16(regs.HL);
        break;

        //LD (HL), d8
        case 0x36:
            return LD_MEM_R16_D8(regs.HL);
        break;

        //SCF
        case 0x37:
            return SCF();
        break;

        //JR C, d8
        case 0x38:
            return JR_CC_D8(readFlag(F_C));
        break;

        //ADD HL, SP
        case 0x39:
            return ADD_R16_R16(regs.HL, sp);
        break;

        //LD A, (HL-)
        case 0x3A:
        {
            u8 c = LD_R8_MEM_R16(regs.A, regs.HL);
            regs.HL--;
            return c;
        }
        break;

        //DEC SP
        case 0x3B:
            return DEC_R16(sp);
        break;

        //INC A
        case 0x3C:
            return INC_R8(regs.A);
        break;

        //DEC A
        case 0x3D:
            return DEC_R8(regs.A);
        break;

        //LD A, d8
        case 0x3E:
            return LD_R8_D8(regs.A);
        break;

        //CCF
        case 0x3F:
            return CCF();
        break;

        //LD B, B
        case 0x40:
            return LD_R8_R8(regs.B, regs.B);
        break;

        //LD B, C
        case 0x41:
            return LD_R8_R8(regs.B, regs.C);
        break;

        //LD B, D
        case 0x42:
            return LD_R8_R8(regs.B, regs.D);
        break;

        //LD B, E
        case 0x43:
            return LD_R8_R8(regs.B, regs.E);
        break;

        //LD B, H
        case 0x44:
            return LD_R8_R8(regs.B, regs.H);
        break;

        //LD B, L
        case 0x45:
            return LD_R8_R8(regs.B, regs.L);
        break;

        //LD B, (HL)
        case 0x46:
            return LD_R8_MEM_R16(regs.B, regs.HL);
        break;

        //LD B, A
        case 0x47:
            return LD_R8_R8(regs.B, regs.A);
        break;

        //LD C, B
        case 0x48:
            return LD_R8_R8(regs.C, regs.B);
        break;

        //LD C, C
        case 0x49:
            return LD_R8_R8(regs.C, regs.C);
        break;

        //LD C, D
        case 0x4A:
            return LD_R8_R8(regs.C, regs.D);
        break;

        //LD C, E
        case 0x4B:
            return LD_R8_R8(regs.C, regs.E);
        break;

        //LD C, H
        case 0x4C:
            return LD_R8_R8(regs.C, regs.H);
        break;

        //LD C, L
        case 0x4D:
            return LD_R8_R8(regs.C, regs.L);
        break;

        //LD C, (HL)
        case 0x4E:
            return LD_R8_MEM_R16(regs.C, regs.HL);
        break;

        //LD C, A
        case 0x4F:
            return LD_R8_R8(regs.C, regs.A);
        break;

        //LD D, B
        case 0x50:
            return LD_R8_R8(regs.D, regs.B);
        break;

        //LD D, C
        case 0x51:
            return LD_R8_R8(regs.D, regs.C);
        break;

        //LD D, D
        case 0x52:
            return LD_R8_R8(regs.D, regs.D);
        break;

        //LD D, E
        case 0x53:
            return LD_R8_R8(regs.D, regs.E);
        break;

        //LD D, H
        case 0x54:
            return LD_R8_R8(regs.D, regs.H);
        break;

        //LD D, L
        case 0x55:
            return LD_R8_R8(regs.D, regs.L);
        break;

        //LD D, (HL)
        case 0x56:
            return LD_R8_MEM_R16(regs.D, regs.HL);
        break;

        //LD D, A
        case 0x57:
            return LD_R8_R8(regs.D, regs.A);
        break;

        //LD E, B
        case 0x58:
            return LD_R8_R8(regs.E, regs.B);
        break;

        //LD E, C
        case 0x59:
            return LD_R8_R8(regs.E, regs.C);
        break;

        //LD E, D
        case 0x5A:
            return LD_R8_R8(regs.E, regs.D);
        break;

        //LD E, E
        case 0x5B:
            return LD_R8_R8(regs.E, regs.E);
        break;

        //LD E, H
        case 0x5C:
            return LD_R8_R8(regs.E, regs.H);
        break;

        //LD E, L
        case 0x5D:
            return LD_R8_R8(regs.E, regs.L);
        break;

        //LD E, (HL)
        case 0x5E:
            return LD_R8_MEM_R16(regs.E, regs.HL);
        break;

        //LD E, A
        case 0x5F:
            return LD_R8_R8(regs.E, regs.A);
        break;

        //LD H, B
        case 0x60:
            return LD_R8_R8(regs.H, regs.B);
        break;

        //LD H, C
        case 0x61:
            return LD_R8_R8(regs.H, regs.C);
        break;

        //LD H, D
        case 0x62:
            return LD_R8_R8(regs.H, regs.D);
        break;

        //LD H, E
        case 0x63:
            return LD_R8_R8(regs.H, regs.E);
        break;

        //LD H, H
        case 0x64:
            return LD_R8_R8(regs.H, regs.H);
        break;

        //LD H, L
        case 0x65:
            return LD_R8_R8(regs.H, regs.L);
        break;

        //LD H, (HL)
        case 0x66:
            return LD_R8_MEM_R16(regs.H, regs.HL);
        break;

        //LD H, A
        case 0x67:
            return LD_R8_R8(regs.H, regs.A);
        break;

        //LD L, B
        case 0x68:
            return LD_R8_R8(regs.L, regs.B);
        break;

        //LD L, C
        case 0x69:
            return LD_R8_R8(regs.L, regs.C);
        break;

        //LD L, D
        case 0x6A:
            return LD_R8_R8(regs.L, regs.D);
        break;

        //LD L, E
        case 0x6B:
            return LD_R8_R8(regs.L, regs.E);
        break;

        //LD L, H
        case 0x6C:
            return LD_R8_R8(regs.L, regs.H);
        break;

        //LD L, L
        case 0x6D:
            return LD_R8_R8(regs.L, regs.L);
        break;

        //LD L, (HL)
        case 0x6E:
            return LD_R8_MEM_R16(regs.L, regs.HL);
        break;

        //LD L, A
        case 0x6F:
            return LD_R8_R8(regs.L, regs.A);
        break;

        //LD (HL), B
        case 0x70:
            return LD_MEM_R16_R8(regs.HL, regs.B);
        break;

        //LD (HL), C
        case 0x71:
            return LD_MEM_R16_R8(regs.HL, regs.C);
        break;

        //LD (HL), D
        case 0x72:
            return LD_MEM_R16_R8(regs.HL, regs.D);
        break;

        //LD (HL), E
        case 0x73:
            return LD_MEM_R16_R8(regs.HL, regs.E);
        break;

        //LD (HL), H
        case 0x74:
            return LD_MEM_R16_R8(regs.HL, regs.H);
        break;

        //LD (HL), L
        case 0x75:
            return LD_MEM_R16_R8(regs.HL, regs.L);
        break;

        //HALT
        case 0x76:
            //To be implemented
        break;

        //LD (HL), A
        case 0x77:
            return LD_MEM_R16_R8(regs.HL, regs.A);
        break;

        //LD A, B
        case 0x78:
            return LD_R8_R8(regs.A, regs.B);
        break;

         //LD A, C
        case 0x79:
            return LD_R8_R8(regs.A, regs.C);
        break;

         //LD A, D
        case 0x7A:
            return LD_R8_R8(regs.A, regs.D);
        break;

         //LD A, E
        case 0x7B:
            return LD_R8_R8(regs.A, regs.E);
        break;

         //LD A, H
        case 0x7C:
            return LD_R8_R8(regs.A, regs.H);
        break;

         //LD A, L
        case 0x7D:
            return LD_R8_R8(regs.A, regs.L);
        break;

         //LD A, (HL)
        case 0x7E:
            return LD_R8_MEM_R16(regs.A, regs.HL);
        break; 
        
        //LD A, A
        case 0x7F:
            return LD_R8_R8(regs.A, regs.A);
        break;

        //ADD A, B
        case 0x80:
            return ADD_A_R8(regs.B);
        break;

        //ADD A, C
        case 0x81:
            return ADD_A_R8(regs.C);
        break;

        //ADD A, D
        case 0x82:
            return ADD_A_R8(regs.D);
        break;

        //ADD A, E
        case 0x83:
            return ADD_A_R8(regs.E);
        break;

        //ADD A, H
        case 0x84:
            return ADD_A_R8(regs.H);
        break;

        //ADD A, L
        case 0x85:
            return ADD_A_R8(regs.L);
        break;

        //ADD A, (HL)
        case 0x86:
            return ADD_A_R8(bus->readByte(regs.HL));
        break;

        //ADD A, A
        case 0x87:
            return ADD_A_R8(regs.A);
        break;

        //ADC A, B
        case 0x88:
            return ADC_A_R8(regs.B);
        break;

        //ADC A, C
        case 0x89:
            return ADC_A_R8(regs.C);
        break;

        //ADC A, D
        case 0x8A:
            return ADC_A_R8(regs.D);
        break;

        //ADC A, E
        case 0x8B:
            return ADC_A_R8(regs.E);
        break;

        //ADC A, H
        case 0x8C:
            return ADC_A_R8(regs.H);
        break;

        //ADC A, L
        case 0x8D:
            return ADC_A_R8(regs.L);
        break;

        //ADC A, (HL)
        case 0x8E:
            return ADC_A_R8(bus->readByte(regs.HL));
        break;

        //ADC A, A
        case 0x8F:
            return ADC_A_R8(regs.A);
        break;

        //SUB B
        case 0x90:
            return SUB_A_R8(regs.B);
        break;

        //XOR A
        case 0xAF:
            return XOR_R8(regs.A);
        break;

        //OR B
        case 0xB0:
            return OR_R8(regs.B);
        break;

        //JP a16
        case 0xC3:
            return JP_A16();
        break;

        default:
            std::cout << "Unknown opcode " << std::hex << (int)currentInstruction->opcode << std::endl;
            running = false;
            return 1;
        break;
    };

    std::cout << "ERROR: someone didn't return" << std::endl;

    return 1;
}

u8 CPU::LD_R16_D16(u16& reg)
{
    reg = bus->readWord(pc + 1);
    return currentInstruction->cycles;
}

u8 CPU::LD_R8_D8(u8& reg)
{
    reg = bus->readByte(pc + 1);
    return currentInstruction->cycles;
}

u8 CPU::LD_R8_R8(u8& regA, u8& regB)
{
    regA = regB;
    return currentInstruction->cycles;
}

u8 CPU::LD_MEM_R16_R8(u16& reg16, u8& reg8)
{
    bus->writeByte(reg16, reg8);
    return currentInstruction->cycles;
}

u8 CPU::LD_MEM_R16_D8(u16& reg)
{
    bus->writeByte(reg, bus->readByte(pc + 1));
    return currentInstruction->cycles;
}

u8 CPU::LD_R8_MEM_R16(u8& reg8, u16& reg16)
{
    reg8 = bus->readByte(reg16);
    return currentInstruction->cycles;
}

u8 CPU::INC_R16(u16& reg)
{
    reg++;
    return currentInstruction->cycles;
}

u8 CPU::INC_MEM_R16(u16& reg)
{
    bus->writeByte(reg, bus->readByte(reg) + 1);

    setFlag(F_Z, bus->readByte(reg) == 0);
    setFlag(F_H, (bus->readByte(reg) & 0xF) == 0);
    setFlag(F_N, 0);
    return currentInstruction->cycles;
}

u8 CPU::DEC_R16(u16& reg)
{
    reg--;
    return currentInstruction->cycles;
}

u8 CPU::DEC_MEM_R16(u16& reg)
{
    bus->writeByte(reg, bus->readByte(reg) - 1);

    setFlag(F_Z, bus->readByte(reg) == 0);
    setFlag(F_H, (bus->readByte(reg) & 0xF) == 0xF);
    setFlag(F_N, 1);
    return currentInstruction->cycles;
}

u8 CPU::INC_R8(u8& reg)
{
    reg++;

    setFlag(F_Z, reg == 0);
    setFlag(F_H, (reg & 0xF) == 0);
    setFlag(F_N, 0);
    return currentInstruction->cycles;
}

u8 CPU::DEC_R8(u8& reg)
{
    reg--;

    setFlag(F_Z, reg == 0);
    setFlag(F_H, (reg & 0xF) == 0xF);
    setFlag(F_N, 1);
    return currentInstruction->cycles;
}

u8 CPU::RLCA()
{
    u8 droppedBit = (regs.A >> 7);
    regs.A = (regs.A << 1) | droppedBit;

    setFlag(F_C, droppedBit);
    setFlag(F_Z, 0);
    setFlag(F_N, 0);
    setFlag(F_H, 0);
    return currentInstruction->cycles;
}

u8 CPU::RLA()
{
    u8 droppedBit = (regs.A >> 7);
    regs.A = (regs.A << 1) | readFlag(F_C);

    setFlag(F_C, droppedBit);
    setFlag(F_Z, 0);
    setFlag(F_N, 0);
    setFlag(F_H, 0);
    return currentInstruction->cycles;
}

u8 CPU::RRCA()
{
    u8 droppedBit = (regs.A & 0x01);
    regs.A = (regs.A >> 1) | (droppedBit << 7);

    setFlag(F_C, droppedBit);
    setFlag(F_Z, 0);
    setFlag(F_N, 0);
    setFlag(F_H, 0);
    return currentInstruction->cycles;
}

u8 CPU::RRA()
{
    u8 droppedBit = (regs.A & 0x01);
    regs.A = (regs.A >> 1) | (readFlag(F_C) << 7);

    setFlag(F_C, droppedBit);
    setFlag(F_Z, 0);
    setFlag(F_N, 0);
    setFlag(F_H, 0);
    return currentInstruction->cycles;
}

u8 CPU::DAA()
{
    if (!readFlag(F_N)) {
		if (readFlag(F_C) || regs.A > 0x99) {
			regs.A = (regs.A + 0x60) & 0xFF;
			setFlag(F_C, 1);
		}
		if (readFlag(F_H) || (regs.A & 0xF) > 0x9) {
			regs.A = (regs.A + 0x06) & 0xFF;
			setFlag(F_H, 0);
		}
	}
	else if (readFlag(F_C) && readFlag(F_H)) {
		regs.A = (regs.A + 0x9A) & 0xFF;
        setFlag(F_H, 0);
	}
	else if (readFlag(F_C)) {
		regs.A = (regs.A + 0xA0) & 0xFF;
	}
	else if (readFlag(F_H)) {
		regs.A = (regs.A + 0xFA) & 0xFF;
         setFlag(F_H, 0);
	}

	setFlag(F_Z, (regs.A == 0));
    return currentInstruction->cycles;
}

u8 CPU::CPL()
{
    regs.A ^= 0xFF;
    setFlag(F_H, 1);
    setFlag(F_N, 1);
    return currentInstruction->cycles;
}

u8 CPU::SCF()
{
    setFlag(F_C, 1);
    setFlag(F_H, 0);
    setFlag(F_N, 0);

    return currentInstruction->cycles;
}

u8 CPU::CCF()
{
    setFlag(F_C, !readFlag(F_C));
    setFlag(F_N, 0);
    setFlag(F_H, 0);

    return currentInstruction->cycles;
}

u8 CPU::LD_MEM_A16_R16(u16& reg)
{
    bus->writeWord(pc + 1, reg);
    return currentInstruction->cycles;
}

u8 CPU::ADD_R16_R16(u16& regA, u16& regB)
{
    u32 sum = regA + regB;

    setFlag(F_H, (regA & 0xFFF) > (sum & 0xFFF));
    setFlag(F_C, sum > 0xFFFF);
    setFlag(F_N, 0);

    regA = sum & 0xFFFF;
    return currentInstruction->cycles;
}

u8 CPU::ADD_A_R8(u8 reg)
{
    u16 sum = regs.A + reg;

    setFlag(F_C, sum > 0xFF);
    setFlag(F_H, (sum & 0xF) < (regs.A & 0xF));
    setFlag(F_N, 0);
    setFlag(F_Z, regs.A == 0);
    regs.A = sum & 0xFF;

    return currentInstruction->cycles;
}

u8 CPU::ADC_A_R8(u8 reg)
{
    u16 sum = regs.A + reg + readFlag(F_C);

    setFlag(F_C, sum > 0xFF);
    setFlag(F_H, (sum & 0xF) < (regs.A & 0xF));
    setFlag(F_N, 0);
    setFlag(F_Z, regs.A == 0);
    regs.A = sum & 0xFF;

    return currentInstruction->cycles;
}

u8 CPU::SUB_A_R8(u8 reg)
{
    s16 sub = regs.A - reg;

    setFlag(F_H, (regs.A & 0xF) < (sub & 0xF));
    setFlag(F_C, sub < 0);
    setFlag(F_Z, sub == 0);
    setFlag(F_N, 1);
    regs.A = sub & 0xFF;

    return currentInstruction->cycles;
}

//Relative to the end of this instruction, doesn't need to set "jumped"
u8 CPU::JR_D8()
{
    pc += bus->readByte(pc + 1);
    return currentInstruction->cycles;
}

u8 CPU::JR_CC_D8(u8 condition)
{
    if (condition)
    {
        pc += bus->readByte(pc + 1);
        return currentInstruction->altCycles;
    }
    
    return currentInstruction->cycles;
}

u8 CPU::JP_A16()
{
    jumped = true;
    pc = bus->readWord(pc + 1);
    return currentInstruction->cycles;
}

u8 CPU::XOR_R8(u8& reg)
{
    regs.A ^= reg;
    setFlag(F_Z, regs.A == 0);
    setFlag(F_N, 0);
    setFlag(F_H, 0);
    setFlag(F_C, 0);
    return currentInstruction->cycles;
}

u8 CPU::OR_R8(u8& reg)
{
    regs.A |= reg;
    setFlag(F_Z, regs.A == 0);
    setFlag(F_N, 0);
    setFlag(F_H, 0);
    setFlag(F_C, 0);
    return currentInstruction->cycles;
}

u8 CPU::AND_R8(u8& reg)
{
    regs.A &= reg;
    setFlag(F_Z, regs.A == 0);
    setFlag(F_N, 0);
    setFlag(F_H, 1);
    setFlag(F_C, 0);
    return currentInstruction->cycles;
}

void CPU::setFlag(u8 flag, u8 set)
{
	if (set)
	{
		if (flag == 7) regs.F |= 0b01000000;
		if (flag == 6) regs.F |= 0b00100000;
		if (flag == 5) regs.F |= 0b00010000;
		if (flag == 4) regs.F |= 0b00001000;
	}
	else
	{
		if (flag == 7) regs.F ^= 0b01000000;
		if (flag == 6) regs.F ^= 0b00100000;
		if (flag == 5) regs.F ^= 0b00010000;
		if (flag == 4) regs.F ^= 0b00001000;
	}
}

u8 CPU::readFlag(u8 flag)
{
	if (flag == 7) return (regs.F & 0b01000000) >> 6;
	if (flag == 6) return (regs.F & 0b00100000) >> 5;
	if (flag == 5) return (regs.F & 0b00010000) >> 4;
	if (flag == 4) return (regs.F & 0b00001000) >> 3;

    return 0;
}