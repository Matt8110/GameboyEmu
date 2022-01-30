#include "BUS.h"

void BUS::writeByte(u16 address, u8 byte)
{
	if (address < 0x8000)
	{
		printf("Error: Attempting to write to ROM!\n");
		return;
	}
	//Video RAM
	else if (address >= 0x8000 && address <= 0x9FFF)
	{
		videoRam[address - 0x8000] = byte;
	}
	//External RAM (Switchable)
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		externalRam[(address - 0xA000) + (currentExtRamBank * 0x2000)] = byte;
	}
	//Work RAM bank A
	else if (address >= 0xC000 && address <= 0xCFFF)
	{
		wRamBankA[address - 0xC000] = byte;
	}
	//Work RAM bank B (Switchable)
	else if (address >= 0xD000 && address <= 0xDFFF)
	{
		wRamBankB[(address - 0xD000) + (currentWRamBank * 0x1000)] = byte;
	}
	//Echo of Work Ram Bank A
	else if (address >= 0xE000 && address <= 0xFDFF)
	{
		wRamBankA[address - 0xE000] = byte;
	}
	//Various things
	else if (address >= 0xFE00 && address <= 0xFFFF)
	{
		remainingRAM[address - 0xFE00] = byte;
	}
}

u8 BUS::readByte(u16 address)
{
	//ROM Bank A
	if (address <= 0x3FFF)
	{
		return romBankA[address];
	}
	//ROM Bank B (Switchable)
	else if (address >= 0x4000 && address <= 0x7FFF)
	{
		return romBankB[(address - 0x4000) + (currentRomBank * 0x4000)];
	}
	//Video RAM
	else if (address >= 0x8000 && address <= 0x9FFF)
	{
		return videoRam[address - 0x8000];
	}
	//External RAM
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		return externalRam[(address - 0xA000) + (currentExtRamBank * 0x2000)];
	}
	//Work RAM bank A
	else if (address >= 0xC000 && address <= 0xCFFF)
	{
		return wRamBankA[address - 0xC000];
	}
	//Work RAM bank B (Switchable)
	else if (address >= 0xD000 && address <= 0xDFFF)
	{
		return wRamBankB[(address - 0xD000) + (currentWRamBank * 0x1000)];
	}
	//Echo of Work Ram Bank A
	else if (address >= 0xE000 && address <= 0xFDFF)
	{
		return wRamBankA[address - 0xE000];
	}
	//Various things
	else if (address >= 0xFE00 && address <= 0xFFFF)
	{
		return remainingRAM[address - 0xFE00];
	}

    return 0;
}

void BUS::writeWord(u16 address, u16 word)
{
	writeByte(address + 1, (word >> 8) & 0x00FF);
	writeByte(address, word & 0x00FF);
}

u16 BUS::readWord(u16 address)
{
	u16 ret = readByte(address + 1);
	ret = (ret << 8) | readByte(address);

	return ret;
}

std::string BUS::loadRomIntoMemory(const std::string& romPath)
{
	std::ifstream romFile(romPath, std::ios::binary);

	if (!romFile.is_open())
	{
		printf("Failed to open file %s\n", romPath.c_str());
		return "FAILED";
	}

	romFile.read((char*)romBankA, 0x4000);
	romFile.read((char*)romBankB, fileSize(romPath) - 0x4000);

	romFile.close();

	std::string gameName = "";

	for (int i = 0; i < 16; i++)
	{
		gameName += readByte(0x134 + i);
	}

	return gameName;

}

long long BUS::fileSize(const std::string& filePath) {

	std::streampos fsize = 0;
	std::ifstream file(filePath, std::ios::binary);

	fsize = file.tellg();
	file.seekg(0, std::ios::end);
	fsize = file.tellg() - fsize;
	file.close();

	return fsize;
}

u8 BUS::getBit(u16 address, u8 bit)
{
	u8 mem = readByte(address);
	mem = (mem >> bit) & 0x01;

	return mem;
}