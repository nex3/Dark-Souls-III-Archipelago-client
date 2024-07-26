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
	uint8_t unk00[0x14];
	uint32_t slotIdCap;
	uint8_t unk01[0x20];
	InventorySlotItem* itemsAboveCap;
	uint64_t unk02;
	InventorySlotItem* itemsBelowCap;
};

/// Information about the current player's inventory.
struct EquipInventoryData {
	uint8_t unk00[0x10];
	EquipInventoryDataList list;
};

/// Information about the current player's equipment.
struct EquipGameData {
	uint8_t unk00[0x1a8];
	EquipInventoryData equipInventoryData;
};

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
