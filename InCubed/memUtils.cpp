#include "pch.h"
#include "memUtils.h"

namespace memUtils
{
	// External Process Functions
	void ReadMemoryEx(HANDLE hProcess, uintptr_t address, void* buffer, unsigned int size)
	{
		ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), buffer, size, nullptr);
	}

	void WriteMemoryEx(HANDLE hProcess, uintptr_t address, void* buffer, unsigned int size)
	{
		DWORD oldProtect;
		VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect);
		WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(address), buffer, size, nullptr);
		VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(address), size, oldProtect, &oldProtect);
	}

	void NopMemoryEx(HANDLE hProcess, uintptr_t address, unsigned int size)
	{
		BYTE* nopArray = new BYTE[size];
		memset(nopArray, X86_NOP, size);
		WriteMemoryEx(hProcess, address, nopArray, size);
		delete[] nopArray;
	}

	DWORD SetMemoryExecuteEx(HANDLE hProcess, uintptr_t address, unsigned int size)
	{
		DWORD oldProtect;
		VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect);
		return oldProtect;
	}

	uintptr_t FindDMAAddressEx(HANDLE hProcess, uintptr_t baseAddress, std::vector<unsigned int> offsets)
	{
		uintptr_t address = baseAddress;
		for (int i = 0; i < offsets.size(); i++)
		{
			ReadMemoryEx(hProcess, address, &address, sizeof(address));
			address += offsets[i];
		}

		return address;
	}

	// Internal Process Functions
	void ReadMemory(uintptr_t address, void* buffer, unsigned int size)
	{
		memcpy_s(buffer, size, (void*)address, size);
	}

	void WriteMemory(uintptr_t address, void* buffer, unsigned int size)
	{
		DWORD oldProtect;
		VirtualProtect(reinterpret_cast<LPVOID>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect);
		memcpy_s((void*)address, size, buffer, size);
		VirtualProtect(reinterpret_cast<LPVOID>(address), size, oldProtect, &oldProtect);
	}

	void NopMemory(uintptr_t address, unsigned int size)
	{
		BYTE* nopArray = new BYTE[size];
		memset(nopArray, X86_NOP, size);
		WriteMemory(address, nopArray, size);
		delete[] nopArray;
	}

	DWORD SetMemoryExecute(uintptr_t address, unsigned int size)
	{
		DWORD oldProtect;
		VirtualProtect(reinterpret_cast<LPVOID>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect);
		return oldProtect;
	}

	uintptr_t FindDMAAddress(uintptr_t baseAddress, std::vector<unsigned int> offsets)
	{
		uintptr_t address = baseAddress;
		for (int i = 0; i < offsets.size(); i++)
		{
			address = *reinterpret_cast<uintptr_t*>(address);
			address += offsets[i];
		}

		return address;
	}
}