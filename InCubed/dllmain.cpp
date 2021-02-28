#include "pch.h"
#include "memUtils.h"
#include "processUtils.h"
#include "assaultCubeInfo.h"

static FILE* consoleFile;
static uintptr_t baseAddress;

// Inject this hook at baseAddress + AC_ROF_HOOK_OFF
static BYTE rofHookCode[] = {
	0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,						// call dword ptr [0x00000000]			# (0x00) <- replace call address with a pointer that points to rofShellcode address (+0x2)
	0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90				// NOPs									# (0x07)
}; 

// will this be executable memory? might need to alloc it somewhere instead? maybe just need to give it execute permission?
// We overwrite 13 bytes of code with the hooking code, we still need that calculation done though so it's moved to the shellcode.
static BYTE rofShellcode[] = { 
	0x8B, 0x46, 0x0C,										// mov eax,[esi+0C]						# (0x00) code moved from hook location
	0x0F, 0xBF, 0x88, 0x0A, 0x01, 0x00, 0x00,				// movsx ecx,word ptr [eax+0000010A]	# (0x03) code moved from hook location
	0x8B, 0x56, 0x18,										// mov edx,[esi+18]						# (0x0A) code moved from hook location
	0xD1, 0xE9,												// shr ecx,1							# (0x0D) divide ecx (fire delay timer) by 2 by shifting right once
	0xC3													// ret									# (0x0F) return
};

// Inject this shellcode at baseAddress + AC_RECOIL_FUNCTION_OFF
static BYTE recoilShellcode[] = {
	0xC2, 0x08, 0x00										// ret 0008								# we overwrite the first instruction of the recoil function with a return
};

static bool infiniteAmmo;
static bool infiniteGrenades;
static bool infiniteHealth;
static bool infiniteArmor;
static bool infiniteAkimbo;
static bool recoilHack;
static bool rofHack;

const wchar_t* onOff(bool b)
{
	if (b)
		return L"<ON>";
	else
		return L"<OFF>";
}

void Draw(bool& update)
{
	if (consoleFile && update)
	{
		system("cls");
		std::wcout << L"============================================================" << std::endl;
		std::wcout << L"|     InCubed - Assault Cube Internal Hack                 |" << std::endl;
		std::wcout << L"============================================================" << std::endl;
		std::wcout << std::endl;
		wprintf_s(L"\tBase Address:\t\t0x%08X\n", baseAddress);
		wprintf_s(L"\tPlayer Entity Pointer:\t0x%08X\n", baseAddress + AC_PLAYER_ENTITY_OFF);
		std::wcout << std::endl;
		std::wcout << L"\t + NUMPAD_0 - Infinite Ammo\t->\t" << onOff(infiniteAmmo) << std::endl;
		std::wcout << L"\t + NUMPAD_1 - Infinite Grenades\t->\t" << onOff(infiniteGrenades) << std::endl;
		std::wcout << L"\t + NUMPAD_2 - Infinite Health\t->\t" << onOff(infiniteHealth) << std::endl;
		std::wcout << L"\t + NUMPAD_3 - Infinite Armor\t->\t" << onOff(infiniteArmor) << std::endl;
		std::wcout << L"\t + NUMPAD_4 - Infinite Akimbo\t->\t" << onOff(infiniteAkimbo) << std::endl;
		std::wcout << L"\t + NUMPAD_5 - No Recoil\t\t->\t" << onOff(recoilHack) << std::endl;
		std::wcout << L"\t + NUMPAD_6 - Double RoF\t->\t" << onOff(rofHack) << std::endl;
		std::wcout << std::endl;
		std::wcout << L"\t + END      - Unhook and exit" << std::endl;

		update = false;
	}
}

DWORD WINAPI InjectedThread(HANDLE hModule)
{
	consoleFile = nullptr;
	baseAddress = (uintptr_t)nullptr;

	bool drawUpdate = false;
	if (AllocConsole())
	{
		freopen_s(&consoleFile, "CONOUT$", "w", stdout);
		system("mode 60, 17");
		drawUpdate = true;
	}

	// Toggles
	infiniteAmmo = false;
	infiniteGrenades = false;
	infiniteHealth = false;
	infiniteArmor = false;
	infiniteAkimbo = false;
	recoilHack = false;
	rofHack = false;

	// Freeze values
	UINT32 grenadeCount = AC_MAX_GRENADE;
	UINT32 health = AC_MAX_HEALTH;
	UINT32 armor = AC_MAX_ARMOR;
	UINT32 akimboAmmoMag = AC_MAX_AKIMBO_AMMO_MAG;
	UINT32 akimboAmmoInv = AC_MAX_AKIMBO_AMMO_INV;
	BYTE akimboEnabled = TRUE;
	BYTE akimboDisabled = FALSE;

	// Find addresses
	baseAddress = processUtils::GetModuleBaseAddress(AC_MODULE_NAME);

	uintptr_t grenadesAddress = memUtils::FindDMAAddress(baseAddress + AC_PLAYER_ENTITY_OFF, AC_PLAYER_GRENADES_OFF);
	uintptr_t healthAddress = memUtils::FindDMAAddress(baseAddress + AC_PLAYER_ENTITY_OFF, AC_PLAYER_HEALTH_OFF);
	uintptr_t armorAddress = memUtils::FindDMAAddress(baseAddress + AC_PLAYER_ENTITY_OFF, AC_PLAYER_ARMOR_OFF);

	uintptr_t akimboAmmoMagAddress = memUtils::FindDMAAddress(baseAddress + AC_PLAYER_ENTITY_OFF, AC_PLAYER_AKIMBO_AMMO_MAG_OFF);
	uintptr_t akimboAmmoInvAddress = memUtils::FindDMAAddress(baseAddress + AC_PLAYER_ENTITY_OFF, AC_PLAYER_AKIMBO_AMMO_INV_OFF);
	uintptr_t akimboEnabledAddress = memUtils::FindDMAAddress(baseAddress + AC_PLAYER_ENTITY_OFF, AC_PLAYER_AKIMBO_ENABLED_OFF);

	// Update shellcodes
	uintptr_t* rofShellcodeAddress = (uintptr_t*)&rofShellcode;
	uintptr_t* addr = (uintptr_t*)&rofShellcodeAddress;
	memcpy_s(rofHookCode + 0x2, sizeof(addr), &addr, sizeof(addr));

	// Make &rofShellcode executable
	memUtils::SetMemoryExecute((uintptr_t)&rofShellcode, sizeof(rofShellcode));

	while (TRUE)
	{
		Draw(drawUpdate);
		// Exit hack
		if (GetAsyncKeyState(VK_END) & 1)
			break;

		// Infinite ammo patch
		if (GetAsyncKeyState(VK_NUMPAD0) & 1)
		{
			drawUpdate = true;
			infiniteAmmo = !infiniteAmmo;

			if (infiniteAmmo)
				memUtils::NopMemory(baseAddress + AC_AMMO_FUNCTION_OFFSET, AC_AMMO_FUNCTION_SIZE);
			else
				memUtils::WriteMemory(baseAddress + AC_AMMO_FUNCTION_OFFSET, (void*)AC_AMMO_FUNCTION_ORIGINAL, AC_AMMO_FUNCTION_SIZE);
		}

		// Freeze value toggles
		if (GetAsyncKeyState(VK_NUMPAD1) & 1)
		{
			drawUpdate = true;
			infiniteGrenades = !infiniteGrenades;
		}

		if (GetAsyncKeyState(VK_NUMPAD2) & 1)
		{
			drawUpdate = true;
			infiniteHealth = !infiniteHealth;
		}

		if (GetAsyncKeyState(VK_NUMPAD3) & 1)
		{
			drawUpdate = true;
			infiniteArmor = !infiniteArmor;
		}

		// Infinite akimbo stuff
		if (GetAsyncKeyState(VK_NUMPAD4) & 1)
		{
			drawUpdate = true;
			infiniteAkimbo = !infiniteAkimbo;

			if (infiniteAkimbo)
			{
				memUtils::WriteMemory(akimboAmmoMagAddress, &akimboAmmoMag, sizeof(akimboAmmoMag));
				memUtils::WriteMemory(akimboAmmoInvAddress, &akimboAmmoInv, sizeof(akimboAmmoInv));
			}
			else
				memUtils::WriteMemory(akimboEnabledAddress, &akimboDisabled, sizeof(akimboDisabled));
		}

		// No recoil hack
		if (GetAsyncKeyState(VK_NUMPAD5) & 1)
		{
			drawUpdate = true;
			recoilHack = !recoilHack;

			if (recoilHack)
				memUtils::WriteMemory(baseAddress + AC_RECOIL_FUNCTION_OFF, recoilShellcode, sizeof(recoilShellcode));
			else
				memUtils::WriteMemory(baseAddress + AC_RECOIL_FUNCTION_OFF, (void*)AC_RECOIL_FUNCTION_ORIGINAL, AC_RECOIL_FUNCTION_SIZE);
		}

		// Rate of fire hack
		if (GetAsyncKeyState(VK_NUMPAD6) & 1)
		{
			drawUpdate = true;
			rofHack = !rofHack;

			if (rofHack)
				memUtils::WriteMemory(baseAddress + AC_ROF_HOOK_OFF, rofHookCode, sizeof(rofHookCode));
			else
				memUtils::WriteMemory(baseAddress + AC_ROF_HOOK_OFF, (void*)AC_ROF_FUNCTION_ORIGINAL, AC_ROF_FUNCTION_SIZE);
		}

		// Freeze values
		if (infiniteGrenades)
			memUtils::WriteMemory(grenadesAddress, &grenadeCount, sizeof(grenadeCount));
		if (infiniteHealth)
			memUtils::WriteMemory(healthAddress, &health, sizeof(health));
		if (infiniteArmor)
			memUtils::WriteMemory(armorAddress, &armor, sizeof(armor));
		if (infiniteAkimbo)
			memUtils::WriteMemory(akimboEnabledAddress, &akimboEnabled, sizeof(akimboEnabled));

		Sleep(5);
	}

	// Cleanup
	if (infiniteAmmo)
		memUtils::WriteMemory(baseAddress + AC_AMMO_FUNCTION_OFFSET, (void*)AC_AMMO_FUNCTION_ORIGINAL, AC_AMMO_FUNCTION_SIZE);
	if (infiniteAkimbo)
		memUtils::WriteMemory(akimboEnabledAddress, &akimboDisabled, sizeof(akimboDisabled));
	if (recoilHack)
		memUtils::WriteMemory(baseAddress + AC_RECOIL_FUNCTION_OFF, (void*)AC_RECOIL_FUNCTION_ORIGINAL, AC_RECOIL_FUNCTION_SIZE);
	if (rofHack)
		memUtils::WriteMemory(baseAddress + AC_ROF_HOOK_OFF, (void*)AC_ROF_FUNCTION_ORIGINAL, AC_ROF_FUNCTION_SIZE);

	if (consoleFile)
	{
		fclose(consoleFile);
		FreeConsole();
	}

	FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(hModule), 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InjectedThread, hModule, 0, nullptr);
		if (hThread)
			CloseHandle(hThread);
	}

	return TRUE;
}