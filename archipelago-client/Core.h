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

#define VERSION "3.1.0-alpha.16"


class CCore: public modengine::ModEngineExtension {
public:
	CCore(modengine::ModEngineExtensionConnector* connector);

	static VOID Start();
	static VOID InputCommand();
	virtual VOID Run();
	virtual VOID Panic(const char* pMessage, const char* pSort, DWORD dError, DWORD dIsFatalError);
	virtual VOID ReadConfigFiles();
	virtual VOID SaveConfigFiles();
	virtual VOID CleanReceivedItemsList();
	virtual BOOL CheckOldApFile();

	std::string pSlotName;
	std::string pPassword;
	std::string pSeed;
	BOOL saveConfigFiles = false;
	BOOL sendGoalStatus = true;
	int pLastReceivedIndex = 0;

	// Whether Archipelago is currently connected or not.
	BOOL connected = false;

	static const int RUN_SLEEP = 2000;

private:
	// Whether ModEngine is running in debug mode (in which case we're sharing the console with
	// its debug output).
	BOOL modEngineDebug = false;

	const char* id() override {
		return "archipelago";
	}

	void on_attach() override;
	void on_detach() override;

};
