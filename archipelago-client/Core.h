#pragma once
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_INTERNAL_
#define _CRT_SECURE_NO_WARNINGS
//#define WSWRAP_NO_SSL

#include <modengine/extension.h>
#include "subprojects/apclientpp/apclient.hpp"
#include <windows.h>
#include <Windows.h>
#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <string>
#include <fstream>
#include <bitset>
#include <tlhelp32.h>
#include <stdio.h>
#include <functional>
#include <nlohmann/json.hpp>
#include "ArchipelagoInterface.h"

#define int3 __debugbreak();

#define FE_InitFailed 0
#define FE_AmountTooHigh 1
#define FE_NullPtr 2
#define FE_NullArray 3
#define FE_BadFunc 4
#define FE_MemError 5
#define HE_InvalidItemType 6
#define HE_InvalidInventoryEquipID 7
#define HE_Undefined 8
#define HE_NoPlayerChar 9
#define AP_InitFailed 10
#define AP_MissingFile 11
#define AP_MissingValue 12
#define FE_MissingDLC 13
#define FE_ApplySettings 14
#define FE_PatternFailed 15

#define VERSION "3.0.8"


class CCore: public modengine::ModEngineExtension {
public:
	CCore(modengine::ModEngineExtensionConnector* connector);

	static VOID Start();
	static VOID InputCommand();
	virtual VOID Run();
	virtual VOID Panic(const char* pMessage, const char* pSort, DWORD dError, DWORD dIsFatalError);
	virtual BOOL CheckOldApFile();

	// Initializes [savePath]. May only be called after a connection has been established. Throws
	// an exception if initialization is unsuccessful.
	//
	// TODO: We now load and save last_received_index data from the native DS3 save rather than a
	// separate JSON file. Remove this once it's no longer necessary to migrate older users onto
	// the new system (probably as part of 3.1.0).
	void InitSavePath();

	/// Sets [pSeed].
	/// 
	/// We expect this to be called twice when loading into a game: once for the seed we added to
	/// the game's save data and once for the seed from the user's connection to the Archipelago
	/// server. If all goes well, these will agree. If they don't, that probably indicates that the
	/// player loaded the wrong save by acceident, so we show them an "are you sure?" message.
	/// 
	/// The [fromSave] parameter indicates whether this seed comes from the save file, as opposed
	/// to from the Archipelago server we've connected to. It's just used for error reporting.
	void SetSeed(std::string seed, bool fromSave);

	std::string pSlotName;
	std::string pPassword;

	// The seed used to generate this multiworld. We use this to double-check that the save file
	// which was loaded is in fact connected to the correct Archipelago room. Note that this isn't
	// strictly unique—multiple distinct rooms can use the same generation, and it's possible that
	// someone may manually supply the same seed to multiple generations.
	//
	// This is set to none before the seed has been loaded from the save file, or if the save file
	// didn't contain a seed.
	std::optional<std::string> pSeed;

	BOOL writeSaveFileNextTick = false;
	BOOL sendGoalStatus = true;
	int pLastReceivedIndex = 0;

	// Whether Archipelago is currently connected or not.
	BOOL connected = false;

	static const int RUN_SLEEP = 2000;

private:
	// Whether ModEngine is running in debug mode (in which case we're sharing the console with
	// its debug output).
	BOOL modEngineDebug = false;

	// The path where the config file for the current run lives.
	std::filesystem::path configPath;

	// The path where the save data for the current run lives.
	//
	// TODO: We now load and save last_received_index data from the native DS3 save rather than a
	// separate JSON file. Remove this once it's no longer necessary to migrate older users onto
	// the new system (probably as part of 3.1.0).
	std::filesystem::path savePath;

	// The raw config data currently stored in [configPath].
	nlohmann::json configData;

	// Initializes [configPath]. Throws an exception if initialization is unsuccessful.
	void InitConfigPath();

	const char* id() override {
		return "archipelago";
	}

	void on_attach() override;
	void on_detach() override;

	// Walks the user through connecting to the Archipelago server.
	void Connect();

	// Removes items from [ItemRandomizer.ReceivedItemsQueue] that were already received according
	// to [pLastReceivedIndex].
	void SkipAlreadyReceivedItems();

	// Loads the file at [configPath] into the client's data structures. Throws an exception if the
	// config file fails to load.
	void LoadConfigFile();

	// Writes the client's updated configuration to the file at [configPath].
	void WriteConfigFile();

	// Loads the file at [savePath] into the client's data structures. Throws an exception if the
	// save file fails to load.
	//
	// TODO: We now load and save last_received_index data from the native DS3 save rather than a
	// separate JSON file. Remove this once it's no longer necessary to migrate older users onto
	// the new system (probably as part of 3.1.0).
	void LoadSaveFile();

	// Removes (without deleting in case the user still wants it) the old save file. This should
	// only be called once the file is detected to be broken somehow.
	//
	// TODO: We now load and save last_received_index data from the native DS3 save rather than a
	// separate JSON file. Remove this once it's no longer necessary to migrate older users onto
	// the new system (probably as part of 3.1.0).
	void RemoveOldSaveFile();

	// Atomically writes the given contents to the given file. Guarantees as much as possible that 
	// the file will either not be updated or will have all of the given contents.
	//
	// Throws an exception if writing fails.
	void WriteAtomic(const std::filesystem::path& path, const std::string& contents);

	// Converts the given path to safely avoid MAX_PATH limits on Windows.
	inline std::filesystem::path WindowsLongPath(const std::filesystem::path& path)
	{
		// The \\?\ prefix that disables MAX_PATH limits also disables automatic resolution of
		// . and .. path components, so we have to normalize the path before conversion.
		return std::filesystem::path(
			L"\\\\?\\" + std::filesystem::absolute(path).lexically_normal().wstring()
		);
	}

	// Prompts the user for a yes or no response in the command line window. Returns true if they
	// respond "y", false if they respond "n". Returns [defaultResponse] if they press any other
	// key.
	bool PromptYN(std::string message, bool defaultResponse = true);

	// Prompts the user for a string response in the command line window.
	std::string PromptString(std::string prompt);

	// Prompts the user for a string response in the command line window, using the given default
	// value if they don't type anything.
	std::string PromptString(std::string prompt, std::string defaultValue);

	// Prompts the user for a string response in the command line window, using the given default
	// value if they don't type anything.
	//
	// Hides the default value rather than displaying it in plain text.
	std::string PromptStringHideDefault(std::string prompt, std::string defaultValue);

	// Returns a text description of the last Win32 error, or none if that fails.
	std::optional<std::string> GetLastWin32ErrorText();
};
