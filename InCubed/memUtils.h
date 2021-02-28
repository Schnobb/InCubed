#pragma once
#include "pch.h"

#define X86_NOP 0x90

namespace memUtils
{
	// External Process Functions
	void ReadMemoryEx(HANDLE hProcess, uintptr_t address, void* buffer, unsigned int size);
	void WriteMemoryEx(HANDLE hProcess, uintptr_t address, void* buffer, unsigned int size);
	void NopMemoryEx(HANDLE hProcess, uintptr_t address, unsigned int size);
	DWORD SetMemoryExecuteEx(HANDLE hProcess, uintptr_t address, unsigned int size);

	uintptr_t FindDMAAddressEx(HANDLE hProcess, uintptr_t baseAddress, std::vector<unsigned int> offsets);

	// Internal Process Functions
	void ReadMemory(uintptr_t address, void* buffer, unsigned int size);
	void WriteMemory(uintptr_t address, void* buffer, unsigned int size);
	void NopMemory(uintptr_t address, unsigned int size);
	DWORD SetMemoryExecute(uintptr_t address, unsigned int size);

	uintptr_t FindDMAAddress(uintptr_t baseAddress, std::vector<unsigned int> offsets);
}