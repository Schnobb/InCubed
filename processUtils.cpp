#include "pch.h"
#include "processUtils.h"

namespace processUtils
{
    DWORD GetProcessId(const wchar_t* processName)
    {
        DWORD pid = 0;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (hSnapshot == INVALID_HANDLE_VALUE)
            return 0;

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hSnapshot, &pe32))
        {
            CloseHandle(hSnapshot);
            return 0;
        }

        do
        {
            if (!_wcsicmp(processName, pe32.szExeFile))
            {
                pid = pe32.th32ProcessID;
                break;
            }

        } while (Process32Next(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        return pid;
    }

    HANDLE GetProcessHandle(DWORD pid)
    {
        return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }

    HANDLE GetProcessHandle(const wchar_t* processName)
    {
        DWORD pid = GetProcessId(processName);
        return GetProcessHandle(pid);
    }

    uintptr_t GetModuleBaseAddress(DWORD pid, const wchar_t* moduleName)
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);

        if (hSnapshot == INVALID_HANDLE_VALUE)
            return 0;

        MODULEENTRY32 me32;
        me32.dwSize = sizeof(MODULEENTRY32);

        if (!Module32First(hSnapshot, &me32))
        {
            CloseHandle(hSnapshot);
            return 0;
        }

        uintptr_t baseAddress = 0;
        do
        {
            if (!_wcsicmp(moduleName, me32.szModule))
            {
                baseAddress = (uintptr_t)me32.modBaseAddr;
                break;
            }
        } while (!Module32Next(hSnapshot, &me32));

        CloseHandle(hSnapshot);
        return baseAddress;
    }

    uintptr_t GetModuleBaseAddress(const wchar_t* moduleName)
    {
        return reinterpret_cast<uintptr_t>(GetModuleHandle(moduleName));
    }

    std::list<MODULEENTRY32>* ListModules(DWORD pid)
    {
        std::list<MODULEENTRY32>* modules = new std::list<MODULEENTRY32>();
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);

        if (hSnapshot == INVALID_HANDLE_VALUE)
            return modules;

        MODULEENTRY32 me32;
        me32.dwSize = sizeof(MODULEENTRY32);

        if (!Module32First(hSnapshot, &me32))
        {
            CloseHandle(hSnapshot);
            return modules;
        }

        while (true)
        {
            MODULEENTRY32 mod;
            mod.dwSize = sizeof(MODULEENTRY32);

            if (!Module32Next(hSnapshot, &mod))
                break;

            modules->push_back(mod);
        }

        CloseHandle(hSnapshot);
        return modules;
    }
}