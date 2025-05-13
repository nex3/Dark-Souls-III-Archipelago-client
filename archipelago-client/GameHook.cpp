#include <spdlog/spdlog.h>

#include "GameHook.h"
#include "ItemRandomiser.h"

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

// The underlying function that passes items to the player. We use this both to give items from
// Archipelago and to hook into to inspect items the player picks up.
auto fItemGib = FindPattern(
	"fItemGib",
	"48 8d 6c 24 d9 48 81 ec 00 01 00 00 48 c7 45 cf fe ff ff ff",
	-0x10
).as<decltype(&CItemRandomiser::HookedItemGib)>();

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
	static GameDataMan** static_address = [] {
		return ResolveMov(FindPattern(
			"GameDataMan",
			"48 8B 05 ?? ?? ?? ?? 48 85 C0 ?? ?? 48 8B 40 ?? C3"
		)).as<GameDataMan**>();
	}();
	return *static_address;
}

// Hook the functions necessary to customize game behavior.
BOOL CGameHook::initialize() {
	spdlog::debug("CGameHook::preInitialize");

	try {
		if (MH_Initialize() != MH_OK) return false;
	} catch (const std::exception&) {
		spdlog::error("Cannot initialize MinHook");
		return false;
	}

	auto onGetitemAddress = FindPattern("OnGetItem",
		"40 57 48 83 ec 40 48 c7 44 24 38 fe ff ff ff 48 89 5c 24 50 48 89 74 24 58");

	auto getActionEventInfoFmgAddress =
		FindPattern("getActionEventInfoFmg", "44 8b ca 33 d2 44 8d 42 65", -0xc);

	auto onWorldLoadedAddress = FindPattern(
		"OnWorldLoaded",
		"0f 10 00 0f 29 44 24 50 0f 10 48 10 0f 29 4c 24 60 0f 10 40 20 0f 29 44 24 70 0f 10 48 "
		"30 0f 29 8c 24 80 00 00 00",
		-0x60);
	
	auto onWorldUnloadedAddress = FindPattern(
		"OnWorldUnloaded",
		"48 8b 35 ?? ?? ?? ?? 33 ed 48 8b f9 48 85 f6 74 27",
		-0x14);

	try {
		return SimpleHook((LPVOID)fItemGib, (LPVOID)&CItemRandomiser::HookedItemGib,
				(LPVOID*)&ItemRandomiser->ItemGibOriginal)
			&& SimpleHook(onGetitemAddress.as<LPVOID>(), (LPVOID)&CItemRandomiser::HookedOnGetItem,
				(LPVOID*)&ItemRandomiser->OnGetItemOriginal)
			&& SimpleHook(getActionEventInfoFmgAddress.as<LPVOID>(), (LPVOID)&HookedGetActionEventInfoFmg,
				(LPVOID*)&GameHook->GetActionEventInfoFmgOriginal)
			&& SimpleHook(onWorldLoadedAddress.as<LPVOID>(), (LPVOID)&HookedOnWorldLoaded,
				(LPVOID*)&GameHook->OnWorldLoadedOriginal)
			&& SimpleHook(onWorldUnloadedAddress.as<LPVOID>(), (LPVOID)&HookedOnWorldUnloaded,
				(LPVOID*)&GameHook->OnWorldUnloadedOriginal);
	} catch (const std::exception&) {
		spdlog::error("Cannot hook the game");
	}
	return false;
}

BOOL CGameHook::isEverythingLoaded() {
	if (!isWorldLoaded) return false;

	auto mainCharacter = WorldChrMan::instance()->mainCharacter;
	if (!mainCharacter) return false;
	auto container = mainCharacter->container;
	if (!container || !container->dataModule) return false;

	if (!SprjEventFlagMan::instance()->worldFlags) return false;
	if (!GameDataMan::instance()->localPlayerData) return false;

	return true;
}

BOOL CGameHook::applySettings() {
	BOOL bReturn = true;

	if (dLockEquipSlots) { LockEquipSlots(); }
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
			spdlog::debug("The player just died, a death link has been ignored");
			deathLinkData = false;
			return;
		}
		ArchipelagoInterface->sendDeathLink();
	}
}

VOID CGameHook::killThePlayer() {
	spdlog::debug("Kill the player");
	WorldChrMan::instance()->mainCharacter->container->dataModule->hp = 0;
}

VOID CGameHook::updateRuntimeValues() {
	lastHealthPoint = healthPoint;
	healthPoint = WorldChrMan::instance()->mainCharacter->container->dataModule->hp;
	soulOfCinderDefeated = SprjEventFlagMan::instance()->worldFlags[0x5F67];
}

VOID CGameHook::GiveNextItem() {
	if (ItemRandomiser->receivedItemsQueue.empty()) return;

	//Send the next item in the list
	spdlog::debug("Send an item from the list of items");
	SReceivedItem item = ItemRandomiser->receivedItemsQueue.back();
	ItemRandomiser->receivedItemsQueue.pop_back();
	if (item.address == 0x40002346) {
		grantPathOfTheDragon();
	} else {
		SItemBuffer items = { 1, {item.address, item.count, -1} };
		int unknown = 1;
		ItemRandomiser->HookedItemGib(*mapItemMan, &items, &unknown);
	}
	Core->writeSaveFileNextTick = true;
}

BOOL CGameHook::isSoulOfCinderDefeated() {
	constexpr std::uint8_t mask7{ 0b1000'0000 };
	return isEverythingLoaded() && (int)(soulOfCinderDefeated & mask7) == 128;
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

VOID CGameHook::showBanner(std::wstring message) {
	// The way this works is a bit hacky. DS3 looks up all its user-facing text by ID for localization
	// purposes, so we show a banner with an unused ID (0x10001312) and hook into the ID function to
	// return the value of nextMessageToSend.
	nextMessageToSend = message;
	setEventFlag(100001313, true);
}

VOID CGameHook::showBanner(std::string message) {
	std::wstring wideMessage(message.begin(), message.end());
	showBanner(wideMessage);
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

VOID CGameHook::removeFromInventory(int32_t itemCategory, int32_t itemId, uint64_t quantity) {
	auto fRemoveFromInventory = FindPattern(
		"fRemoveFromInventory",
		"48 8b 05 c9 ?? ?? 04 41 8b d9 45 8b f0 48 8b 78 10",
		-0x18
	).as<void (*)(void* unused, int32_t itemCategory, uint32_t itemId, uint64_t quantity)>();

	fRemoveFromInventory(NULL, itemCategory, itemId, quantity);
}

BOOL CGameHook::checkIsDlcOwned() {
	auto dlc = CSDlc::instance();
	return dlc->dlc1Installed && dlc->dlc2Installed;
}

LPVOID CGameHook::HookedOnWorldLoaded(ULONGLONG unknown1, ULONGLONG unknown2, DWORD unknown3,
	DWORD unknown4, DWORD unknown5) {
	auto result = GameHook->OnWorldLoadedOriginal(unknown1, unknown2, unknown3, unknown4, unknown5);
	GameHook->isWorldLoaded = true;
	return result;
}

void CGameHook::HookedOnWorldUnloaded(ULONGLONG unknown1, ULONGLONG unknown2, ULONGLONG unknown3,
		ULONGLONG unknown4) {
	GameHook->isWorldLoaded = false;
	GameHook->OnWorldUnloadedOriginal(unknown1, unknown2, unknown3, unknown4);
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
		Core->Panic(
			std::format(
				"Failed to find pattern for {}. This probably means you're using an "
				"unsupported Dark Souls III patch. 1.15.0 and 1.15.2 are known to be supported.",
				name).c_str(),
			"Missing pattern", FE_PatternFailed, true);
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
	case 100001312:
		if (GameHook->nextMessageToSend.length() > 0) {
			return GameHook->nextMessageToSend.c_str();
		}
		else {
			return L"[AP mod bug: nextMessageToSend not set]";
		}
	}
	return GameHook->GetActionEventInfoFmgOriginal(messages, messageId);
}
