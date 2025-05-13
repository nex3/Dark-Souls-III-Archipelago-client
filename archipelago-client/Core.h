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

#define VERSION "3.0.4"


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
	void InitSavePath();

	std::string pSlotName;
	std::string pPassword;
	std::string pSeed;
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

	// The path where the save data for the current run lives.
	std::filesystem::path savePath;

	const char* id() override {
		return "archipelago";
	}

	void on_attach() override;
	void on_detach() override;

	// Removes items from [ItemRandomizer.ReceivedItemsQueue] that were already received according
	// to [pLastReceivedIndex].
	void SkipAlreadyReceivedItems();

	// Loads the file at [savePath] into the client's data structures. Throws an exception if the
	// save file fails to load.
	void LoadSaveFile();

	// Writes the client's updated data to the file at [savePath].
	void WriteSaveFile();

	// Removes (without deleting in case the user still wants it) the old save file. This should
	// only be called once the file is detected to be broken somehow.
	void RemoveOldSaveFile();

	// Atomically writes the given contents to the given file. Guarantees as much as possible that 
	// the file will either not be updated or will have all of the given contents.
	//
	// Throws an exception if writing fails.
	void WriteAtomic(const std::filesystem::path& path, const std::string& contents);

	// Converts a (UTF-8 encoded) string into a wide string suitable for use with win32 APIs.
	inline std::wstring Widen(const std::string& string)
	{
		if (string.empty()) return std::wstring();
		std::mbstate_t state = std::mbstate_t();
		const char* data = string.data();
		std::size_t length = 1 + std::mbsrtowcs(nullptr, &data, 0, &state);
		std::wstring result(length, L'\0');

		std::mbsrtowcs(result.data(), &data, result.size(), &state);
		return result;
	}

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
