#pragma once

#include "VarTypes.h"
#include "BUS.h"
#include "Interrupts.h"
#include <iostream>

#define F_Z 7
#define F_N 6
#define F_H 5
#define F_C 4
#define CPU_SPEED 4.194304
#define FPS 60
#define CLOCKS_PER_FRAME FPS/CPU_SPEED

struct Instruction_t
{
    std::string name;
    u8 opcode;
    u8 length;
    u8 cycles;
    u8 altCycles;
};

union Registers_t
{
    struct
    {
        u8 A;
        u8 F;
        u8 B;
        u8 C;
        u8 D;
        u8 E;
        u8 H;
        u8 L;
    };

    struct 
    {
        u16 AF;
        u16 BC;
        u16 DE;
        u16 HL;
    };
    
};

class CPU
{
public:
    CPU(BUS* bus, Interrupts* interrupts);
    void setFlag(u8 flag, u8 set);
    u8 readFlag(u8 flag);
    void run();
    void clock();
    u8 executeInstruction(); //Returns cycles used

public:
    Registers_t regs;
    u16 pc = 0x100;
    u16 sp = 0;
    BUS* bus;
    Interrupts* interrupts;
    bool running = true;
    u32 cycleCount = 0;
    u32 cyclesThisFrame = 0;
    Instruction_t* currentInstruction;
    bool jumped = false;

private:
    Instruction_t instructionSet[256]
    {
        {"NOP", 0, 1, 4, 0},
		{"LD BC, d16", 1, 3, 12, 0},
		{"LD (BC), A", 2, 1, 8, 0},
		{"INC BC", 3, 1, 8, 0},
		{"INC B", 4, 1, 4, 0},
		{"DEC B", 5, 1, 4, 0},
		{"LD B, 0x%02X\n", 6, 1, 8, 0},
		{"RICA", 7, 1, 4, 0},
		{"LD (a16), SP", 8, 3, 20, 0},
		{"ADD HL, BC", 9, 1, 8, 0},
		{"LD A, (BC)", 10, 1, 8, 0},
		{"DEC BC", 11, 1, 8, 0},
		{"INC C", 12, 1, 4, 0},
		{"DEC C", 13, 1, 4, 0},
		{"LD C, 0x%02X\n", 14, 2, 8, 0},
		{"RRCA", 15, 1, 4, 0},
		{"STOP d8", 16, 2, 4, 0},
		{"LD DE, 0x%04X\n", 17, 3, 12, 0},
		{"LD (DE), A", 18, 1, 8, 0},
		{"INC DE", 19, 1, 8, 0},
		{"INC D", 20, 1, 4, 0},
		{"DEC D", 21, 1, 4, 0},
		{"LD D, d8", 22, 2, 8, 0},
		{"RLA", 23, 1, 4, 0},
		{"JR r8", 24, 2, 12, 0},
		{"ADD HL, DE", 25, 1, 8, 0},
		{"LD A, (DE)", 26, 1, 8, 0},
		{"DEC DE", 27, 1, 8, 0},
		{"INC E", 28, 1, 4, 0},
		{"DEC E", 29, 1, 4, 0},
		{"LD E, d8", 30, 2, 8, 0},
		{"RRA", 31, 1, 4, 0},
		{"JR NZ, %d\n", 32, 2, 12, 8},
		{"LD HL, 0x%04X\n", 33, 3, 12, 0},
		{"LD (HL+), A", 34, 1, 8, 0},
		{"INC HL", 35, 1, 8, 0},
		{"INC H", 36, 1, 4, 0},
		{"DEC H", 37, 1, 4, 0},
		{"LD H, d8", 38, 2, 8, 0},
		{"DAA", 39, 1, 4, 0},
		{"JR Z, r8", 40, 2, 12, 8},
		{"ADD HL, HL", 41, 1, 8, 0},
		{"LD A, (HL+)", 42, 1, 8, 0},
		{"DEC HL", 43, 1, 8, 0},
		{"INC L", 44, 1, 4, 0},
		{"DEC L", 45, 1, 4, 0},
		{"LD L, d8", 46, 2, 8, 0},
		{"CPL", 47, 1, 4, 0},
		{"JR NC, r8", 48, 2, 12, 8},
		{"LD SP, 0x%04X\n", 49, 3, 12, 0},
		{"LD (HL-), A", 50, 1, 8, 0},
		{"INC SP", 51, 1, 8, 0},
		{"INC (HL)", 52, 1, 12, 0},
		{"DEC (HL)", 53, 1, 12, 0},
		{"LD (HL), d8", 54, 2, 12, 0},
		{"SCF", 55, 1, 4, 0},
		{"JR C, r8", 56, 2, 12, 8},
		{"ADD HL, SP", 57, 1, 8, 0},
		{"LD A, (HL-)", 58, 1, 8, 0},
		{"DEC SP", 59, 1, 8, 0},
		{"INC A", 60, 1, 4, 0},
		{"DEC A", 61, 1, 4, 0},
		{"LD A, 0x%02X\n", 62, 2, 8, 0},
		{"CCF", 63, 1, 4, 0},
		{"LD B, B", 64, 1, 4, 0},
		{"LD B, C", 65, 1, 4, 0},
		{"LD B, D", 66, 1, 4, 0},
		{"LD B, E", 67, 1, 4, 0},
		{"LD B, H", 68, 1, 4, 0},
		{"LD B, L", 69, 1, 4, 0},
		{"LD B, (HL)", 70, 1, 8, 0},
		{"LD B, A", 71, 1, 4, 0},
		{"LD C, B", 72, 1, 4, 0},
		{"LD C, C", 73, 1, 4, 0},
		{"LD C, D", 74, 1, 4, 0},
		{"LD C, E", 75, 1, 4, 0},
		{"LD C, H", 76, 1, 4, 0},
		{"LD C, L", 77, 1, 4, 0},
		{"LD C, (HL)", 78, 1, 8, 0},
		{"LD C, A", 79, 1, 4, 0},
		{"LD D, B", 80, 1, 4, 0},
		{"LD D, C", 81, 1, 4, 0},
		{"LD D, D", 82, 1, 4, 0},
		{"LD D, E", 83, 1, 4, 0},
		{"LD D, H", 84, 1, 4, 0},
		{"LD D, L", 85, 1, 4, 0},
		{"LD D, (HL)", 86, 1, 8, 0},
		{"LD D, A", 87, 1, 4, 0},
		{"LD E, B", 88, 1, 4, 0},
		{"LD E, C", 89, 1, 4, 0},
		{"LD E, D", 90, 1, 4, 0},
		{"LD E, E", 91, 1, 4, 0},
		{"LD E, H", 92, 1, 4, 0},
		{"LD E, L", 93, 1, 4, 0},
		{"LD E, (HL)", 94, 1, 8, 0},
		{"LD E, A", 95, 1, 4, 0},
		{"LD H, B", 96, 1, 4, 0},
		{"LD H, C", 97, 1, 4, 0},
		{"LD H, D", 98, 1, 4, 0},
		{"LD H, E", 99, 1, 4, 0},
		{"LD H, H", 100, 1, 4, 0},
		{"LD H, L", 101, 1, 4, 0},
		{"LD H, (HL)", 102, 1, 8, 0},
		{ "LD H, A", 103, 1, 4, 0 },
		{ "LD L, B", 104, 1, 4, 0 },
		{ "LD L, C", 105, 1, 4, 0 },
		{ "LD L, D", 106, 1, 4, 0 },
		{ "LD L, E", 107, 1, 4, 0 },
		{ "LD L, H", 108, 1, 4, 0 },
		{ "LD L, L", 109, 1, 4, 0 },
		{ "LD L, (HL)", 110, 1, 8, 0 },
		{ "LD L, A", 111, 1, 4, 0 },
		{ "LD (HL), B", 112, 1, 8, 0 },
		{ "LD (HL), C", 113, 1, 8, 0 },
		{ "LD (HL), D", 114, 1, 8, 0 },
		{ "LD (HL), E", 115, 1, 8, 0 },
		{ "LD (HL), H", 116, 1, 8, 0 },
		{ "LD (HL), L", 117, 1, 8, 0 },
		{ "HALT", 118, 1, 4, 0 },
		{ "LD (HL), A", 119, 1, 8, 0 },
		{ "LD A, B", 120, 1, 4, 0 },
		{ "LD A, C", 121, 1, 4, 0 },
		{ "LD A, D", 122, 1, 4, 0 },
		{ "LD A, E", 123, 1, 4, 0 },
		{ "LD A, H", 124, 1, 4, 0 },
		{ "LD A, L", 125, 1, 4, 0 },
		{ "LD A, (HL)", 126, 1, 8, 0 },
		{ "LD A, A", 127, 1, 4, 0 },
		{ "ADD A, B", 128, 1, 4, 0 },
		{ "ADD A, C", 129, 1, 4, 0 },
		{ "ADD A, D", 130, 1, 4, 0 },
		{ "ADD A, E", 131, 1, 4, 0 },
		{ "ADD A, H", 132, 1, 4, 0 },
		{ "ADD A, L", 133, 1, 4, 0 },
		{ "ADD A, (HL)", 134, 1, 8, 0 },
		{ "ADD A, A", 135, 1, 4, 0 },
		{ "ADC A, B", 136, 1, 4, 0 },
		{ "ADC A, C", 137, 1, 4, 0 },
		{ "ADC A, D", 138, 1, 4, 0 },
		{ "ADC A, E", 139, 1, 4, 0 },
		{ "ADC A, H", 140, 1, 4, 0 },
		{ "ADC A, L", 141, 1, 4, 0 },
		{ "ADC A, (HL)", 142, 1, 8, 0 },
		{ "ADC A, A", 143, 1, 4, 0 },
		{ "SUB B", 144, 1, 4, 0 },
		{ "SUB C", 145, 1, 4, 0 },
		{ "SUB D", 146, 1, 4, 0 },
		{ "SUB E", 147, 1, 4, 0 },
		{ "SUB H", 148, 1, 4, 0 },
		{ "SUB L", 149, 1, 4, 0 },
		{ "SUB (HL)", 150, 1, 8, 0 },
		{ "SUB A", 151, 1, 4, 0 },
		{ "SBC A, B", 152, 1, 4, 0 },
		{ "SBC A, C", 153, 1, 4, 0 },
		{ "SBC A, D", 154, 1, 4, 0 },
		{ "SBC A, E", 155, 1, 4, 0 },
		{ "SBC A, H", 156, 1, 4, 0 },
		{ "SBC A, L", 157, 1, 4, 0 },
		{ "SBC A, (HL)", 158, 1, 8, 0 },
		{ "SBC A, A", 159, 1, 4, 0 },
		{ "AND B", 160, 1, 4, 0 },
		{ "AND C", 161, 1, 4, 0 },
		{ "AND D", 162, 1, 4, 0 },
		{ "AND E", 163, 1, 4, 0 },
		{ "AND H", 164, 1, 4, 0 },
		{ "AND L", 165, 1, 4, 0 },
		{ "AND (HL)", 166, 1, 8, 0 },
		{ "AND A", 167, 1, 4, 0 },
		{ "XOR B", 168, 1, 4, 0 },
		{ "XOR C", 169, 1, 4, 0 },
		{ "XOR D", 170, 1, 4, 0 },
		{ "XOR E", 171, 1, 4, 0 },
		{ "XOR H", 172, 1, 4, 0 },
		{ "XOR L", 173, 1, 4, 0 },
		{ "XOR (HL)", 174, 1, 8, 0 },
		{ "XOR A", 175, 1, 4, 0 },
		{ "OR B", 176, 1, 4, 0 },
		{ "OR C", 177, 1, 4, 0 },
		{ "OR D", 178, 1, 4, 0 },
		{ "OR E", 179, 1, 4, 0 },
		{ "OR H", 180, 1, 4, 0 },
		{ "OR L", 181, 1, 4, 0 },
		{ "OR (HL)", 182, 1, 8, 0 },
		{ "OR A", 183, 1, 4, 0 },
		{ "CP B", 184, 1, 4, 0 },
		{ "CP C", 185, 1, 4, 0 },
		{ "CP D", 186, 1, 4, 0 },
		{ "CP E", 187, 1, 4, 0 },
		{ "CP H", 188, 1, 4, 0 },
		{ "CP L", 189, 1, 4, 0 },
		{ "CP (HL)", 190, 1, 8, 0 },
		{ "CP A", 191, 1, 4, 0 },
		{ "RET NZ", 192, 1, 20, 8 },
		{ "POP BC", 193, 1, 12, 0 },
		{ "JP NZ, a16", 194, 3, 16, 12 },
		{ "JP 0x%04X\n", 195, 3, 16, 0 },
		{ "CALL NZ, a16", 196, 3, 24, 12 },
		{ "PUSH BC", 197, 1, 16, 0 },
		{ "ADD A, d8", 198, 2, 8, 0 },
		{ "RST 00H", 199, 1, 16, 0 },
		{ "RET Z", 200, 1, 20, 8 },
		{ "RET", 201, 1, 16, 0 },
		{ "JP Z, a16", 202, 3, 16, 12 },
		{ "PREFIX", 203, 1, 4, 0 },
		{ "CALL Z, a16", 204, 3, 24, 12 },
		{ "CALL a16", 205, 3, 24, 0 },
		{ "ADC A, d8", 206, 2, 8, 0 },
		{ "RST 08H", 207, 1, 16, 0 },
		{ "RET NC", 208, 1, 20, 8 },
		{ "POP DE", 209, 1, 12, 0 },
		{ "JP NC, a16", 210, 3, 16, 12 },
		{ "N/A", 211, 1, 4, 0 },
		{ "CALL NC, a16", 212, 3, 24, 12 },
		{ "PUSH DE", 213, 1, 16, 0 },
		{ "SUB d8", 214, 2, 8, 0 },
		{ "RST 10H", 215, 1, 16, 0 },
		{ "RET C", 216, 1, 20, 8 },
		{ "RETI", 217, 1, 16, 0 },
		{ "JP C, a16", 218, 3, 16, 12 },
		{ "N/A", 219, 1, 4, 0 },
		{ "CALL C, a16", 220, 3, 24, 12 },
		{ "N/A", 221, 1, 4, 0 },
		{ "SBC A, d8", 222, 2, 8, 0 },
		{ "RST 18H", 223, 1, 16, 0 },
		{ "LDH (a8), A", 224, 2, 12, 0 },
		{ "POP HL", 225, 1, 12, 0 },
		{ "LD (C), A", 226, 1, 8, 0 },
		{ "N/A", 227, 1, 4, 0 },
		{ "N/A", 228, 1, 4, 0 },
		{ "PUSH HL", 229, 1, 16, 0 },
		{ "AND d8", 230, 2, 8, 0 },
		{ "RST 20H", 231, 1, 16, 0 },
		{ "ADD SP, r8", 232, 2, 16, 0 },
		{ "JP HL", 233, 1, 4, 0 },
		{ "LD (a16), A", 234, 3, 16, 0 },
		{ "N/A", 235, 1, 4, 0 },
		{ "N/A", 236, 1, 4, 0 },
		{ "N/A", 237, 1, 4, 0 },
		{ "XOR d8", 238, 2, 8, 0 },
		{ "RST 28H", 239, 1, 16, 0 },
		{ "LDH A, (a8)", 240, 2, 12, 0 },
		{ "POP AF", 241, 1, 12, 0 },
		{ "LD A, (C)", 242, 1, 8, 0 },
		{ "DI", 243, 1, 4, 0 },
		{ "N/A", 244, 1, 4, 0 },
		{ "PUSH AF", 245, 1, 16, 0 },
		{ "OR d8", 246, 2, 8, 0 },
		{ "RST 30H", 247, 1, 16, 0 },
		{ "LD HL, SP + r8", 248, 2, 12, 0 },
		{ "LD SP, HL", 249, 1, 8, 0 },
		{ "LD A, (a16)", 250, 3, 16, 0 },
		{ "EI", 251, 1, 4, 0 },
		{ "N/A", 252, 1, 4, 0 },
		{ "N/A", 253, 1, 4, 0 },
		{ "CP 0x%02X\n", 254, 2, 8, 0 },
		{ "RST 38H", 255, 1, 16, 0 }

    };

    //Instruction implementations
    u8 LD_R16_D16(u16& reg);
    u8 LD_R8_D8(u8& reg);
    u8 LD_R8_R8(u8& regA, u8& regB);
    u8 LD_MEM_R16_R8(u16& reg16, u8& reg8);
    u8 LD_MEM_R16_D8(u16& reg);
    u8 LD_MEM_A16_R16(u16& reg);
    u8 LD_R8_MEM_R16(u8& reg8, u16& reg16);
    u8 INC_R16(u16& reg);
    u8 INC_MEM_R16(u16& reg);
    u8 DEC_R16(u16& reg);
    u8 DEC_MEM_R16(u16& reg);
    u8 INC_R8(u8& reg);
    u8 DEC_R8(u8& reg);
    u8 RLCA();
    u8 RLA();
    u8 RRCA();
    u8 RRA();
    u8 DAA();
    u8 CPL();
    u8 SCF();
    u8 CCF();
    u8 ADD_R16_R16(u16& regA, u16& regB);
    u8 ADD_A_R8(u8 reg);
    u8 ADC_A_R8(u8 reg);
    u8 SUB_A_R8(u8 reg);
    u8 JR_D8(); //D8 is signed
    u8 JR_CC_D8(u8 condition); //D8 is signed
    u8 JP_A16();
    u8 XOR_R8(u8& reg);
    u8 OR_R8(u8& reg);
    u8 AND_R8(u8& reg);
};