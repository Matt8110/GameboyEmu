#pragma once

#include <stdio.h>
#include <fstream>
#include <string>
#include "VarTypes.h"

#define FLAG_Z 7;
#define FLAG_N 6;
#define FLAG_H 5;
#define FLAG_C 4;

class BUS
{
public:
	void writeByte(u16 address, u8 byte);
	u8 readByte(u16 address);
	void writeWord(u16 address, u16 word);
	u16 readWord(u16 address);
	std::string loadRomIntoMemory(const std::string& romPath);
	u8 getBit(u16 address, u8 bit);

private:
	long long fileSize(const std::string& filePath);

public:

	u8 currentRomBank = 0;
	u8 currentWRamBank = 0;
	u8 currentExtRamBank = 0;

	u8 numberOfRomBanks = 0;
	u8 numberOfRamBanks = 0;

private:
	u8 romBankA[0x4000];
	u8 romBankB[0x200000];//All banks, 0x4000 per bank
	u8 videoRam[0x2000];
	u8 externalRam[0x8000];//All banks, 0x2000 per bank
	u8 wRamBankA[0x1000];
	u8 wRamBankB[0x8000]; //All banks, 0x1000 per bank
	u8 remainingRAM[0x200];
};
