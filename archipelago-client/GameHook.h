#pragma once

#include "Core.h"
#include "GameTypes.h"
#include "Params.h"
#include "mem/mem.h"
#include "./subprojects/minhook/include/MinHook.h"

class CGameHook {
public:
	virtual BOOL initialize();
	virtual BOOL applySettings();
	virtual VOID updateRuntimeValues();
	virtual VOID GiveNextItem();
	virtual BOOL isSoulOfCinderDefeated();
	virtual VOID manageDeathLink();
	int healthPoint = -1, lastHealthPoint = -1;
	char soulOfCinderDefeated;

	// Whether everything the mod needs to access is fully available.
	virtual BOOL isEverythingLoaded();

	// Displays a banner message to the player. Only works if they're in an active game, not on the
	// menu.
	virtual VOID showBanner(std::wstring message);
	virtual VOID showBanner(std::string message);

	// Sets the event flag with the given ID on or off. Works the same as the DarkScript3 function of
	// the same name.
	virtual VOID setEventFlag(DWORD eventId, BOOL enabled);

	// Grants the player the Path of the Dragon gesture.
	virtual VOID grantPathOfTheDragon();

	// Equips an item for the active player based on its index in their inventory.
	virtual VOID equipItem(EquipSlot equipSlot, DWORD inventorySlot);

	// Removes the given item from the player's inventory, if it contains any.
	virtual VOID removeFromInventory(int32_t itemCategory, int32_t itemId, uint64_t quantity = 1);

	DWORD dLockEquipSlots;
	DWORD dIsNoWeaponRequirements;
	DWORD dIsNoSpellsRequirements;
	DWORD dIsNoEquipLoadRequirements;
	DWORD dIsDeathLink;
	DWORD dEnableDLC;
	HANDLE hHeap;

	BOOL deathLinkData = false;

private:
	static uintptr_t FindExecutableAddress(uintptr_t ptrOffset, std::vector<unsigned int> offsets);
	static uintptr_t GetModuleBaseAddress();
	static uintptr_t FindDMAAddy(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets);
	static BOOL SimpleHook(LPVOID pAddress, LPVOID pDetour, LPVOID* ppOriginal);
	static VOID LockEquipSlots();
	static VOID killThePlayer();
	BOOL checkIsDlcOwned();
	
	// Whether the world is loaded. This doesn't *necessarily* mean that everything we need is
	// accessible; for that, check isEverythingLoaded.
	BOOL isWorldLoaded;

	// A hooked function that's run when the game requests a message to display.
	static const wchar_t* HookedGetActionEventInfoFmg(LPVOID messages, DWORD messageId);

	// A hooked function that's run after the data has been loaded for the current game world.
	static LPVOID HookedOnWorldLoaded(ULONGLONG unknown1, ULONGLONG unknown2, DWORD unknown3,
		DWORD unknown4, DWORD unknown5);

	// A hooked function that's run to unload data for the current game world.
	static void HookedOnWorldUnloaded(ULONGLONG unknown1, ULONGLONG unknown2, ULONGLONG unknown3,
		ULONGLONG unknown4);

	// The internal DS3 function that looks up the current localization's message for the given ID. We
	// override this to support custom messages with custom IDs.
	decltype(&HookedGetActionEventInfoFmg) GetActionEventInfoFmgOriginal;

	// The function that allocates a bunch of in-game singletons like WorldChrMan. Once this runs, it's
	// generally safe to make in-game changes.
	decltype(&HookedOnWorldLoaded) OnWorldLoadedOriginal;

	// The deallocator dual of OnWorldLoadedOriginal. Once this runs, it's no longer safe to make
	// in-game changes.
	decltype(&HookedOnWorldUnloaded) OnWorldUnloadedOriginal;
	
	uintptr_t BaseB = -1;
	uintptr_t GameFlagData = -1;
	uintptr_t Param = -1;
	uintptr_t EquipLoad = -1;

	// The next message to send when calling the internal message display function. Not remotely
	// thread-safe, but that shouldn't be an issue as long as we only display banners from the main
	// archipelago thread.
	std::wstring nextMessageToSend;
};

// Returns a pointer to the location of the given memory pattern in the current executable, or
// NULL if the pattern asn't found. If offset is passed, the pointer is adjusted by that many
// bytes if it's found.
//
// The name is used for error reporting.
static mem::pointer FindPattern(const char* name, const char* pattern, ptrdiff_t offset = 0);

// Given a pointer to the beginning of a MOV instruction whose argument is a relative offset,
// returns the address that offset is pointing to.
static mem::pointer ResolveMov(mem::pointer pointer);
