#ifndef _CPU_H_
#define _CPU_H_

#include <stdint.h>
#include "bus.h"

enum
{
    INT_VBLANK = 1,
    INT_LCD = (1 << 1),
    INT_TIMER = (1 << 2),
    INT_SERIAL = (1 << 3),
    INT_JOYPAD = (1 << 4)
};

typedef struct
{
    union
    {
        struct
        {
            uint16_t AF;
            uint16_t BC;
            uint16_t DE;
            uint16_t HL;
        };
        struct
        {
            union
            {
                uint8_t F;

                struct
                {
                    uint8_t un0 : 1;
                    uint8_t un1 : 1;
                    uint8_t un2 : 1;
                    uint8_t un3 : 1;
                    uint8_t c : 1;
                    uint8_t h : 1;
                    uint8_t n : 1;
                    uint8_t z : 1;
                };
            };

            uint8_t A;

            uint8_t C;
            uint8_t B;
            uint8_t E;
            uint8_t D;
            uint8_t L;
            uint8_t H;
        };
    };
}Registers_t;

typedef struct
{
    Bus_t* bus;
    Registers_t regs;
    uint16_t pc;
    uint16_t sp;
    uint8_t cyclesToWait;
    uint8_t IME;
    uint8_t IMEDelayCount;
    uint8_t haltedFlag;
    uint32_t timerCycles;
    uint8_t interruptLevel; //How many interrupts deep are we
    uint8_t haltBug;
}Z80_t;

//extern Instruction_t instructions[256];

Z80_t* cpu_create(Bus_t* bus);
void cpu_destroy(Z80_t* cpu);
void cpu_clock(Z80_t* cpu);

void cpu_stack_push_8(Z80_t* cpu, uint8_t val);
void cpu_stack_push_16(Z80_t* cpu, uint16_t val);
uint8_t cpu_stack_pop_8(Z80_t* cpu);
uint16_t cpu_stack_pop_16(Z80_t* cpu);

void cpu_dispatchInterrupts(Z80_t* cpu);
void cpu_raiseInterrupt(Z80_t* cpu, uint8_t interruptType);
void cpu_lowerInterrupt(Z80_t* cpu, uint8_t interruptType);

void cpu_handleTimers(Z80_t* cpu);

uint8_t rotateRight(uint8_t val);
uint8_t rotateLeft(uint8_t val);

void INSTR_INC_16(Z80_t* cpu, uint16_t* reg);
void INSTR_INC_8(Z80_t* cpu, uint8_t* reg);
void INSTR_INC_REG16PTR(Z80_t* cpu, uint16_t* reg);
void INSTR_DEC_REG16PTR(Z80_t* cpu, uint16_t* reg);
void INSTR_DEC_16(Z80_t* cpu, uint16_t* reg);
void INSTR_DEC_8(Z80_t* cpu, uint8_t* reg);
void INSTR_ADD_REG16_REG16(Z80_t* cpu, uint16_t* regA, uint16_t* regB);
void INSTR_ADD_REG16_IMM8(Z80_t* cpu, uint16_t* regA);
void INSTR_ADD_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB);
void INSTR_ADC_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB);
void INSTR_ADD_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB);
void INSTR_ADC_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB);
void INSTR_SUB_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB);
void INSTR_SUB_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB);
void INSTR_SBC_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB);
void INSTR_SBC_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB);
void INSTR_LD_REG8_IMM8(Z80_t* cpu, uint8_t* reg);
void INSTR_LD_IMMPTR_REG16(Z80_t* cpu, uint16_t* reg);
void INSTR_LD_IMM16PTR_REG8(Z80_t* cpu, uint8_t* reg);
void INSTR_LD_IMM8PTR_REG8(Z80_t* cpu, uint8_t* reg);
void INSTR_LD_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB);
void INSTR_LD_REG16_IMM16(Z80_t* cpu, uint16_t* reg);
void INSTR_LD_REG16PTR_REG8(Z80_t* cpu, uint16_t* regA, uint8_t* regB);
void INSTR_LD_REG8_IMM8(Z80_t* cpu, uint8_t* reg);
void INSTR_LD_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB);
void INSTR_LD_REG16PTR_IMM8(Z80_t* cpu, uint16_t* reg);
void INSTR_LD_REG8PTR_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB);
void INSTR_JR_IMM8(Z80_t* cpu, uint8_t condition);
void INSTR_AND_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB);
void INSTR_AND_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB);
void INSTR_XOR_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB);
void INSTR_XOR_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB);
void INSTR_OR_REG8_REG8(Z80_t* cpu, uint8_t* regA, uint8_t* regB);
void INSTR_OR_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB);
void INSTR_CP_REG8(Z80_t* cpu, uint8_t* reg);
void INSTR_CP_REG8_REG16PTR(Z80_t* cpu, uint8_t* regA, uint16_t* regB);
void INSTR_RET(Z80_t* cpu, uint8_t condition);
void INSTR_JP_IMM16(Z80_t* cpu, uint8_t condition);
void INSTR_CALL_IMM16(Z80_t* cpu, uint8_t condition);

void INSTR_CB_RR_REG8(Z80_t* cpu, uint8_t* reg);
void INSTR_CB_RL_REG8(Z80_t* cpu, uint8_t* reg);
void INSTR_CB_SRL_REG8(Z80_t* cpu, uint8_t* reg);
void INSTR_CB_SWAP_REG8(Z80_t* cpu, uint8_t* reg);
void INSTR_CB_RES_REG8(Z80_t* cpu, uint8_t bit, uint8_t* reg);
void INSTR_CB_RLC_REG8(Z80_t* cpu, uint8_t* reg);
void INSTR_CB_RLC_REG16PTR(Z80_t* cpu, uint16_t* reg);
void INSTR_CB_RRC_REG8(Z80_t* cpu, uint8_t* reg);
void INSTR_CB_RRC_REG16PTR(Z80_t* cpu, uint16_t* reg);
void INSTR_BIT_IMM8_REG8(Z80_t* cpu, uint8_t bit, uint8_t* reg);
void INSTR_CB_SLA_REG8(Z80_t* cpu, uint8_t* reg);
void INSTR_CB_SRA_REG8(Z80_t* cpu, uint8_t* reg);
void INSTR_CB_SET_REG8(Z80_t* cpu, uint8_t bit, uint8_t* reg);
void INSTR_CB_SET_REG16PTR(Z80_t* cpu, uint8_t bit, uint16_t* reg);

#endif