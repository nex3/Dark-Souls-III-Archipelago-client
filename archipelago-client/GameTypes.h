#pragma once

// Types of data structures used within Dark Souls 3 itself.

#include <fd4_singleton.h>

// Constant values used to represent different equip slots in the DS3 inventory.
enum class EquipSlot : uint32_t {
	leftHand1 = 0x00,
	rightHand1 = 0x01,
	head = 0x0C,
	body = 0x0D,
	arms = 0x0E,
	legs = 0x0F,
	ring1 = 0x11,
	ring2 = 0x12,
	ring3 = 0x13,
	ring4 = 0x14,
};

// Constant values to represent different item categories in DS3.
enum class ItemType : uint32_t {
	weapon = 0,
	protector = 1,
	accessory = 2,
	goods = 4
};

// A Dark Souls 3 struct representing a single item granted to the player.
struct SItemBufferEntry {
	// The DS3 ID of the item being granted.
	uint32_t id;

	// The number of items being granted.
	uint32_t quantity;

	// The durability of the items being granted. -1 means full durability.
	int durability;
};

// A Dark Souls 3 struct representing a set of items granted to the player.
struct SItemBuffer {
	// The number of items in this buffer.
	uint32_t length;

	// The set of items in this buffer.
	SItemBufferEntry items[];
};

// A Dark Souls 3 struct containing information about the current character's available actions.
struct SSprjChrActionFlagModule {
	uint8_t unk00[0x10];
	uint32_t chrEquipAnimFlags;
};

// A Dark Souls 3 struct containing information about the current character.
struct SSprjChrDataModule {
	uint8_t unk00[0xD8];

	// The character's HP.
	int hp;
};

// A Dark Souls 3 struct containing various SPRJ modules.
struct SChrInsComponentContainer {
	SSprjChrActionFlagModule* actionModule;
	uint8_t unk00[0x10];
	SSprjChrDataModule* dataModule;
};

// A Dark Souls 3 struct containing information about the current play session.
struct SPlayerIns {
	uint8_t unk00[0x1F90];
	SChrInsComponentContainer* container;
};

// A singleton class containing information about the current play session.
struct WorldChrMan : public FD4Singleton<WorldChrMan, "WorldChrMan"> {
	void** vftable_ptr;
	uint8_t unk00[0x78];
	SPlayerIns* mainCharacter;
};

/// A single item int he player's inventory.
struct InventorySlotItem {
	uint32_t handle;
	uint32_t itemId;
	uint32_t itemCount;
	uint32_t unk00;
};

struct EquipInventoryDataList {
	uint8_t unk00[0x10];
	uint32_t itemsCount;
	uint32_t slotIdCap;
	uint8_t unk01[0x20];
	InventorySlotItem* itemsAboveCap;
	int* pNormalItemsCount;
	InventorySlotItem* itemsBelowCap;
	int* pKeyItemsCount;
	short* itemIdMappingIndices;
	uint8_t unk02[0x18];
};

/// Information about the current player's inventory.
struct EquipInventoryData {
	uint8_t unk00[0x10];
	EquipInventoryDataList list;
	uint32_t itemEntriesCount;
};

/// Information about the current player's equipment.
struct EquipGameData {
	uint8_t unk00[0x1a8];
	EquipInventoryData equipInventoryData;
};

/// For whatever reason, DS3 saves and loads EquipGameData on the loading screen as well as in
/// the context of an individual game. This function returns whether a given instance is for the
/// synthetic loading screen character rather than a real loaded world.
inline int EquipGameData_IsFakeHomeScreenGame(EquipGameData* self) {
	// For some even stranger reason, the loading screen save actually does have a handful of
	// items. However, even a totally fresh save has more items than that, so we check for 12
	// items which is exactly how many the loading screen has. This could be tricked if a player
	// discarded all their starting equipment, so... don't do that.
	return *self->equipInventoryData.list.pKeyItemsCount == 12;
}

/// Information about the current player's state.
struct PlayerGameData {
	uint8_t unk00[0x228];
	EquipGameData equipGameData1;
};

// A singleton class containing information about the current game world.
struct GameDataMan {
	void** vftable_ptr;
	void* trophyEquipData;
	PlayerGameData* localPlayerData;

	static GameDataMan* instance();
};

/// A representation of an item in the player's inventory, used by some built-in functions that
/// modify the inventory.
struct InventoryItemId {
	uint8_t unk00[0x38];
	int inventoryId;
	int itemId;
};

struct DLMemoryOutputStream_vtable;

/// A stream that writes data to some destination.
struct DLMemoryOutputStream {
	DLMemoryOutputStream_vtable* vtable;
};

/// The vtable for DLMemoryOutputStream.
struct DLMemoryOutputStream_vtable {
	void* unk00;
	void* unk01;
	void* unk02;
	size_t(*write)(DLMemoryOutputStream* stream, const void* data, size_t size);
};

struct DLMemoryInputStream_vtable;

/// A stream that reads data from some destination.
struct DLMemoryInputStream {
	DLMemoryInputStream_vtable* vtable;
	size_t end;
	char* buf;
	size_t position;
};

/// The vtable for DLMemoryInputStream.
struct DLMemoryInputStream_vtable {
	void* unk00;
	void* unk01;
	void* unk02;
	size_t(*read)(DLMemoryInputStream* stream, void* data, size_t size);
};