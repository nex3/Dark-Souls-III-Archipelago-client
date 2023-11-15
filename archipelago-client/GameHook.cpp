#include "GameHook.h"

#include "mem/module.h"
#include "mem/pattern.h"

DWORD64 qItemEquipComms = 0;

DWORD64 rItemRandomiser = 0;
DWORD64 rAutoEquip = 0;
DWORD64 rNoWeaponRequirements = 0;
DWORD64 rEquipLock = 0;

LPVOID itemGibDataCodeCave;

extern CItemRandomiser* ItemRandomiser;
extern CArchipelago* ArchipelagoInterface;
extern CCore* Core;
extern CGameHook* GameHook;

// A Dark Souls 3 struct representing an assignment from an inventory slot to an equipment slot.
struct SEquipBuffer {
	uint8_t unk00[0x8];

	// The slot into which the item should go.
	EquipSlot dEquipSlot;

	uint8_t unk01[0x2C];

	// The index in the player's inventory of the item to equip.
	DWORD dInventorySlot;

	uint8_t unk02[0x60];
};

// All of the following hardcoded addresses should really be converted into AOBs that are known to
// be compatible with DS3 1.15 _and_ DS3 1.15.2.

// A singleton object used by DS3 code involving items.
LPVOID* mapItemMan =
	ResolveMov(FindPattern("MapItemMan", "48 8B 0D ?? ?? ?? ?? BB ?? ?? ?? ?? 41 BC"))
	.as<LPVOID*>();

// The internal DS3 function that looks up the current localization's message for the given ID. We
// override this to support custom messages with custom IDs.
wchar_t* (*GetActionEventInfoFmgOriginal)(LPVOID messages, DWORD messageId);

// The function that allocates a bunch of in-game singletons like WorldChrMan. Once this runs, it's
// generally safe to make in-game changes.
LPVOID (*OnWorldLoadedOriginal)(ULONGLONG unknown1, ULONGLONG unknown2, DWORD unknown3,
	DWORD unknown4, DWORD unknown5);

// The deallocator dual of OnWorldLoadedOriginal. Once this runs, it's no longer safe to make
// in-game changes.
void (*OnWorldUnloadedOriginal)(ULONGLONG unknown1, ULONGLONG unknown2, ULONGLONG unknown3,
	ULONGLONG unknown4);

// A singleton class with informatino about installed DLCs.
struct CSDlc : public FD4Singleton<CSDlc, "CSDlc"> {
	void** vftable_ptr;
	uint8_t pad00[0x09];

	// Whether Ashes of Ariandel is installed.
	bool dlc1Installed;

	// Whether The Ringed City is installed.
	bool dlc2Installed;
};

// A singleton class with information about the event system.
struct SprjEventFlagMan : public FD4Singleton<SprjEventFlagMan, "SprjEventFlagMan"> {
	// An array of bit flags that correspond to various states of the local game world.
	uint8_t* worldFlags;
};

GameDataMan* GameDataMan::instance() {
	static GameDataMan* static_address = [] {
		return ResolveMov(FindPattern(
			"GameDataMan",
			"48 8B 05 ?? ?? ?? ?? 48 85 C0 ?? ?? 48 8B 40 ?? C3"
		)).as<GameDataMan*>();
	}();
	return static_address;
}

/*
* Check if a basic hook is working on this version of the game  
*/
BOOL CGameHook::preInitialize() {
	Core->Logger("CGameHook::preInitialize", true, false);

	try {
		if (MH_Initialize() != MH_OK) return false;
	} catch (const std::exception&) {
		Core->Logger("Cannot initialize MinHook");
		return false;
	}

	auto onWorldLoadedAddress = FindPattern(
		"OnWorldLoaded",
		"0f 10 00 0f 29 44 24 50 0f 10 48 10 0f 29 4c 24 60 0f 10 40 20 0f 29 44 24 70 0f 10 48 "
		"30 0f 29 8c 24 80 00 00 00",
		-0x60);
	if (!onWorldLoadedAddress) return false;
	
	auto onWorldUnloadedAddress = FindPattern(
		"OnWorldUnloaded",
		"48 8b 35 ?? ?? ?? ?? 33 ed 48 8b f9 48 85 f6 74 27",
		-0x14);

	try {
		return Hook(0x1407BBA80, (DWORD64)&tItemRandomiser, &rItemRandomiser, 5)
			&& SimpleHook((LPVOID)0x14058aa20, (LPVOID)&fOnGetItem, (LPVOID*)&ItemRandomiser->OnGetItemOriginal)
			&& SimpleHook((LPVOID)0x140e0c690, (LPVOID)&HookedGetActionEventInfoFmg, (LPVOID*)&GetActionEventInfoFmgOriginal)
			&& SimpleHook(onWorldLoadedAddress.as<LPVOID>(), (LPVOID)&HookedOnWorldLoaded, (LPVOID*)&OnWorldLoadedOriginal)
			&& SimpleHook(onWorldUnloadedAddress.as<LPVOID>(), (LPVOID)&HookedOnWorldUnloaded, (LPVOID*)&OnWorldUnloadedOriginal);
	} catch (const std::exception&) {
		Core->Logger("Cannot hook the game");
	}
	return false;
}

BOOL CGameHook::initialize() {
	Core->Logger("CGameHook::initialize", true, false);

	return true;
}

BOOL CGameHook::applySettings() {
	BOOL bReturn = true;

	if (dIsNoWeaponRequirements) { bReturn &= Hook(0x140C073B9, (DWORD64)&tNoWeaponRequirements, &rNoWeaponRequirements, 7); }
	if (dIsNoSpellsRequirements) { RemoveSpellsRequirements(); }
	if (dLockEquipSlots) { LockEquipSlots(); }
	if (dIsNoEquipLoadRequirements) { RemoveEquipLoad(); }
	if (dEnableDLC) {
		if (!checkIsDlcOwned()) {
			Core->Panic("You must own both the ASHES OF ARIANDEL and THE RINGED CITY DLC in order to enable the DLC option in Archipelago", "Missing DLC detected", FE_MissingDLC, 1);
		}
	}
	return bReturn;
}

VOID CGameHook::manageDeathLink() {

	
	if (lastHealthPoint == 0 && healthPoint != 0) {	//The player just respawned
		deathLinkData = false;
	} else if (deathLinkData && lastHealthPoint != 0 && healthPoint != 0 ) { //The player received a deathLink
		killThePlayer();
	} else if(lastHealthPoint != 0 && healthPoint == 0) { //The player just died, ignore the deathLink if received
		if (deathLinkData) {
			Core->Logger("The player just died, a death link has been ignored", true, false);
			deathLinkData = false;
			return;
		}
		ArchipelagoInterface->sendDeathLink();
	}
}

VOID CGameHook::killThePlayer() {
	Core->Logger("Kill the player", true, false);
	WorldChrMan::instance()->mainCharacter->container->dataModule->hp = 0;
}

VOID debugPrint(const char* prefix, void* data) {
	std::ostringstream stream;
	stream << prefix << std::hex << (ULONGLONG)data;
	Core->Logger(stream.str());
}

VOID CGameHook::updateRuntimeValues() {
	lastHealthPoint = healthPoint;
	healthPoint = WorldChrMan::instance()->mainCharacter->container->dataModule->hp;
	soulOfCinderDefeated = SprjEventFlagMan::instance()->worldFlags[0x5F67];
}

VOID CGameHook::giveItems() {
	auto fItemGib = FindPattern(
		"fItemGib",
		"8b 31 89 75 a7 8b 41 04 89 44 24 3c 89 45 ab 8b 41 08 89 44 24 38 45 32 ff"
	).as<void (*)(LPVOID mapItemMan, SItemBuffer * items, int* unknown)>();

	//Send the next item in the list
	int size = ItemRandomiser->receivedItemsQueue.size();
	if (size > 0) {
		Core->Logger("Send an item from the list of items", true, false);
		SReceivedItem item = ItemRandomiser->receivedItemsQueue.back();
		if (item.address == 0x40002346) {
			grantPathOfTheDragon();
		} else {
			SItemBuffer items = { 1, {item.address, item.count, -1} };
			int unknown = 1;
			fItemGib(*mapItemMan, &items, &unknown);
		}
	}
}

BOOL CGameHook::isSoulOfCinderDefeated() {
	constexpr std::uint8_t mask7{ 0b1000'0000 };
	return isWorldLoaded && (int)(soulOfCinderDefeated & mask7) == 128;
}

BOOL CGameHook::Hook(DWORD64 qAddress, DWORD64 qDetour, DWORD64* pReturn, DWORD dByteLen) {

	*pReturn = (qAddress + dByteLen);
	return SimpleHook((LPVOID)qAddress, (LPVOID)qDetour, 0);
}

BOOL CGameHook::SimpleHook(LPVOID pAddress, LPVOID pDetour, LPVOID* ppOriginal) {

	MH_STATUS status = MH_CreateHook(pAddress, pDetour, ppOriginal);
	if (status != MH_OK) return false;
	if (MH_EnableHook(pAddress) != MH_OK) return false;
	return true;
}

uintptr_t CGameHook::FindExecutableAddress(uintptr_t ptrOffset, std::vector<unsigned int> offsets) {
	DWORD processId = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, processId);

	uintptr_t moduleBase = GetModuleBaseAddress();
	uintptr_t dynamicPtrAddr = moduleBase + ptrOffset;
	return FindDMAAddy(hProcess, dynamicPtrAddr, offsets);
}

uintptr_t CGameHook::FindDMAAddy(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets) {

	uintptr_t addr = ptr;
	for (unsigned int i = 0; i < offsets.size(); ++i) {
		ReadProcessMemory(hProc, (BYTE*)addr, &addr, sizeof(addr), 0);
		addr += offsets[i];
	}
	return addr;
}

uintptr_t CGameHook::GetModuleBaseAddress() {
	const char* lpModuleName = "DarkSoulsIII.exe";
	DWORD procId = GetCurrentProcessId();

	MODULEENTRY32 lpModuleEntry = { 0 };
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, procId);
	if (!hSnapShot)
		return NULL;
	lpModuleEntry.dwSize = sizeof(lpModuleEntry);
	BOOL bModule = Module32First(hSnapShot, &lpModuleEntry);
	while (bModule)
	{
		if (!strcmp(lpModuleEntry.szModule, lpModuleName))
		{
			CloseHandle(hSnapShot);
			return (uintptr_t)lpModuleEntry.modBaseAddr;
		}
		bModule = Module32Next(hSnapShot, &lpModuleEntry);
	}
	CloseHandle(hSnapShot);
	return NULL;
}

VOID CGameHook::LockEquipSlots() {

	DWORD dOldProtect = 0;

	auto equip = FindPattern("OnEquip", "84 c0 0f 85 c8 00 00 00 48 8d 44 24 28");
	if (!equip) return;
	auto unequip = FindPattern(
		"OnUnequip",
		"e8 ?? ?? ?? ?? 84 c0 75 33 c7 44 24 20 58 02 00 00 c7 44 24 24 02 00 00 00 c7 44 24 28 29 00 "
		"00 00 48 8d 05",
		5);
	if (!unequip) return;

	if (!VirtualProtect(equip.as<LPVOID>(), 1, PAGE_EXECUTE_READWRITE, &dOldProtect)) return;
	if (!VirtualProtect(unequip.as<LPVOID>(), 1, PAGE_EXECUTE_READWRITE, &dOldProtect)) return;

	*equip.as<BYTE*>() = 0x30;
	*unequip.as<BYTE*>() = 0x30;

	if (!VirtualProtect(equip.as<LPVOID>(), 1, dOldProtect, &dOldProtect)) return;
	if (!VirtualProtect(unequip.as<LPVOID>(), 1, dOldProtect, &dOldProtect)) return;

	return;
}

// TODO: handle this in the offline randomizer
VOID CGameHook::RemoveSpellsRequirements() {

	DWORD processId = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, processId);

	std::vector<unsigned int> offsets = { 0x460, 0x68, 0x68, 0x00 };
	uintptr_t magicAddr = FindExecutableAddress(0x4782838, offsets); //Param + Magic
	
	uintptr_t countAddr = magicAddr + 0x0A;
	int count = 0;
	ReadProcessMemory(hProcess, (BYTE*)countAddr, &count, sizeof(char) * 2, nullptr);

	for (int i = 0; i < count; i++) {
		uintptr_t IDOAddr = magicAddr + 0x48 + 0x18 * i;
		int IDOBuffer;
		ReadProcessMemory(hProcess, (BYTE*)IDOAddr, &IDOBuffer, sizeof(IDOBuffer), nullptr);

		uintptr_t spellAddr = magicAddr + IDOBuffer + 0x1E; //Intelligence
		BYTE newValue = 0x00;
		WriteProcessMemory(hProcess, (BYTE*)spellAddr, &newValue, sizeof(newValue), nullptr);

		spellAddr = magicAddr + IDOBuffer + 0x1F;	//Faith
		WriteProcessMemory(hProcess, (BYTE*)spellAddr, &newValue, sizeof(newValue), nullptr);
	}

	return;
}

VOID CGameHook::RemoveEquipLoad() {

	DWORD processId = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, processId);

	std::vector<unsigned int> offsets = { };
	uintptr_t equipLoadAddr = FindExecutableAddress(0x581FCD, offsets); //EquipLoad 
	
	BYTE newValue[4] = {0x0F, (BYTE)0x57, 0xF6, 0x90};
	WriteProcessMemory(hProcess, (BYTE*)equipLoadAddr, &newValue, sizeof(BYTE) * 4, nullptr);

	return;
}

VOID CGameHook::showMessage(std::wstring message) {
	auto fShowBanner = FindPattern("90 48 8b 44 24 30 48 85 c0 75 11", -0x2D)
		.as<void (*)(UINT_PTR unused, DWORD unknown, ULONGLONG messageId)>;

	// The way this works is a bit hacky. DS3 looks up all its user-facing text by ID for localization
	// purposes, so we show a banner with an unused ID (0x10000000) and hook into the ID function to
	// return the value of nextMessageToSend. Ideally we'd be able to just set up a 
	nextMessageToSend = message;
	fShowBanner(NULL, 1, 0x10000000);
	nextMessageToSend = std::wstring();
}

VOID CGameHook::showMessage(std::string message) {
	std::wstring wideMessage(message.begin(), message.end());
	showMessage(wideMessage);
}

VOID CGameHook::setEventFlag(DWORD eventId, BOOL enabled) {
	static auto fSetEventFlag =
		FindPattern("fSetEventFlag", "8b da 45 84 c0 74 ?? 48 85 c9 75", -0xD)
			.as<void (*)(UINT_PTR unused, DWORD event, BOOL state)>();

	fSetEventFlag(NULL, eventId, enabled);
}

VOID CGameHook::grantPathOfTheDragon() {
	// Archipelago sets up this event flag to grant Path of the Dragon upon being set.
	setEventFlag(100001312, 1);
}

VOID CGameHook::equipItem(EquipSlot equipSlot, DWORD inventorySlot) {
	auto fEquipItem = FindPattern("fEquipItem", "84 c0 0f 84 d7 02 00 00 83 7b 3c ff 75 0e", -0x2D)
		.as<void (*)(EquipSlot dSlot, SEquipBuffer* buffer)>();

	SEquipBuffer buffer{};
	buffer.dEquipSlot = equipSlot;
	buffer.dInventorySlot = inventorySlot;
	fEquipItem(equipSlot, &buffer);
}

BOOL CGameHook::checkIsDlcOwned() {
	auto dlc = CSDlc::instance();
	return dlc->dlc1Installed && dlc->dlc2Installed;
}

LPVOID CGameHook::HookedOnWorldLoaded(ULONGLONG unknown1, ULONGLONG unknown2, DWORD unknown3,
	DWORD unknown4, DWORD unknown5) {
	auto result = OnWorldLoadedOriginal(unknown1, unknown2, unknown3, unknown4, unknown5);
	GameHook->isWorldLoaded = true;
	return result;
}

void CGameHook::HookedOnWorldUnloaded(ULONGLONG unknown1, ULONGLONG unknown2, ULONGLONG unknown3,
		ULONGLONG unknown4) {
	GameHook->isWorldLoaded = false;
	OnWorldUnloadedOriginal(unknown1, unknown2, unknown3, unknown4);
}

mem::pointer FindPattern(const char* name, const char* pattern, ptrdiff_t offset) {
	auto main_module = mem::module::main();
	mem::pattern needle(pattern);
	mem::default_scanner scanner(needle);
	mem::pointer result;
	main_module.enum_segments([&](mem::region range, mem::prot_flags prot) {
		scanner(range, [&](mem::pointer address) {
			result = address.offset(offset);
			return (bool)result;
		});

		return (bool)result;
	});

	if (!result) {
		std::ostringstream stream;
		stream << "Failed to find pattern for " << name;
		Core->Panic(stream.str().c_str(), "Missing pattern", FE_PatternFailed, true);
		return mem::pointer();
	}
	return result;
}

static mem::pointer ResolveMov(mem::pointer pointer) {
	// The pointer points to a MOV instruction whose operand (three bytes in) is relative to the
	// next instruction pointer. The resolved value of that operand is what we care about.
	return pointer.add(7).offset(*pointer.add(3).as<int32_t*>());
}

const wchar_t* CGameHook::HookedGetActionEventInfoFmg(LPVOID messages, DWORD messageId) {
	switch (messageId) {
	case 0x10000000:
		if (GameHook->nextMessageToSend.length() > 0) {
			return GameHook->nextMessageToSend.c_str();
		}
		else {
			return L"[AP mod bug: nextMessageToSend not set]";
		}
	}
	return GetActionEventInfoFmgOriginal(messages, messageId);
}
