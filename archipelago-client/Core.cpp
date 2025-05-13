#include "Core.h"

#include <conio.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/wincolor_sink.h>

#include "AutoEquip.h"
#include "GameHook.h"
#include "ItemRandomiser.h"

CCore* Core;
CGameHook* GameHook;
CItemRandomiser* ItemRandomiser;
CAutoEquip* AutoEquip;
CArchipelago* ArchipelagoInterface;

using nlohmann::json;

CCore::CCore(modengine::ModEngineExtensionConnector* connector)
		: modengine::ModEngineExtension(connector) {

	Core = this;
	GameHook = new CGameHook();
	ItemRandomiser = new CItemRandomiser();
	AutoEquip = new CAutoEquip();
}

void CCore::InitSavePath()
{
	// We keep the save path relative to the current working directory, since we want the saved
	// data to remain consistent even if the player upgrades to a new AP patch version.
	std::filesystem::path baseName(Core->pSeed + "_" + Core->pSlotName + ".json");
	if (CreateDirectoryW(WindowsLongPath("archipelago").c_str(), NULL) ||
			ERROR_ALREADY_EXISTS == GetLastError()) {
		savePath = std::filesystem::path("archipelago\\" + baseName.string());
	}
	else {
		savePath = baseName;
	}
}

void CCore::on_attach() {
	// Set up the client console
	modEngineDebug = !AllocConsole();
	SetConsoleTitleA("Dark Souls III - Archipelago Console");
	FILE* fp;
	freopen_s(&fp, "CONIN$", "r", stdin);

	// If ModEngine is running in debug mode, all our logs will automatically get printed to the
	// console. If not, we only want to print info or higher logs to the console, and we want to do
	// it without any additional prefixes.
	if (!modEngineDebug) {
		freopen_s(&fp, "CONOUT$", "w", stdout);
		auto consoleSink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>(spdlog::color_mode::always);
		consoleSink->set_pattern("%v");
		consoleSink->set_level(spdlog::level::info);
		spdlog::default_logger()->sinks().push_back(consoleSink);
	}
	else {
		// ME2 should really handle these by default, but currently it does not.
		spdlog::default_logger()->set_level(spdlog::level::trace);
		spdlog::default_logger()->flush_on(spdlog::level::trace);
	}

	spdlog::info(
		"Archipelago client v" VERSION "\n"
		"A new version may or may not be available, please check this link for updates: "
		"https://github.com/nex3/Dark-Souls-III-Archipelago-client/releases\n"
		"Type '/connect {SERVER_IP}:{SERVER_PORT} {SLOT_NAME} [password:{PASSWORD}]' to connect to the room\n"
		"Type '/help' for more information\n"
		"-----------------------------------------------------");

	// TODO: Use ModEngine2's infrastructure for registering hooks once it's more usable. See
	// https://github.com/soulsmods/ModEngine2/issues/156.
	if (!GameHook->initialize()) {
		Core->Panic("Check if the game version is 1.15.2, you must use the provided DarkSoulsIII.exe", "Cannot hook the game", FE_InitFailed, 1);
		return;
	}

	if (CheckOldApFile()) {
		spdlog::warn("The AP.json file is not supported in this version, make sure to finish your previous seed on version 1.2 or use this version on the new Archipelago server");
	}

	// Start command prompt
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Core->InputCommand, NULL, NULL, NULL);

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CCore::Start, 0, 0, 0);
}

void CCore::on_detach() {
	// Do nothing; this function is never called.
}

void CCore::WriteAtomic(const std::filesystem::path& path, const std::string& contents)
{
	// Add this silly prefix to suport long path names in the Windows API.
	std::filesystem::path realPath = WindowsLongPath(path);
	std::filesystem::path swapPath(realPath);
	swapPath += ".swap";
	std::ofstream swapFile{ swapPath };
	swapFile << contents;
	swapFile.close();

	if (ReplaceFileW(
		realPath.c_str(),
		swapPath.c_str(),
		NULL,
		REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS,
		NULL,
		NULL
	)) {
		return;
	}

	// ReplaceFileW requires the destination file to exist, so if it failed it's probably
	// because the destination file hasn't been created yet.
	if (GetLastError() == ERROR_FILE_NOT_FOUND) {
		if (MoveFileW(swapPath.c_str(), realPath.c_str())) return;
	}

	auto error = GetLastWin32ErrorText();
	if (error.has_value()) {
		std::string message = "Failed to write to " + path.string() + ": " + error.value();
		throw std::runtime_error(message);
	}
	else {
		throw std::runtime_error("Failed to write to " + path.string() + ".");
	}
}

std::optional<std::string> CCore::GetLastWin32ErrorText()
{
	LPTSTR errorText = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&errorText,
		0,
		NULL
	);

	if (NULL == errorText) return {};
	auto result = std::string(errorText);
	LocalFree(errorText);
	return result;
}

VOID CCore::Start() {

	while (true) {
		Core->Run();
		Sleep(RUN_SLEEP);
	};
};

BOOL CCore::CheckOldApFile() {

	// read the archipelago json file
	std::ifstream i("AP.json");
	return !i.fail();
}

bool isInit = false;
int initProtectionDelay = 3;

VOID CCore::Run() {

	ArchipelagoInterface->update();

	if (GameHook->isEverythingLoaded()) {
		GameHook->updateRuntimeValues();

		if (!isInit && ArchipelagoInterface->isConnected() && initProtectionDelay <= 0) {
			LoadSaveFile();
			SkipAlreadyReceivedItems();

			//Apply player settings
			BOOL initResult = GameHook->applySettings();	
			if (!initResult) {
				Core->Panic("Failed to apply settings", "...\\Randomiser\\Core\\Core.cpp", FE_ApplySettings, 1);
				int3
			}
			spdlog::info("Mod initialized successfully");
			GameHook->showBanner(L"Archipelago connected");
			isInit = true;
		}

		if (isInit) {
			GameHook->manageDeathLink();

			if (!ItemRandomiser->receivedItemsQueue.empty()) {
				pLastReceivedIndex++;
				GameHook->GiveNextItem();
			}

			if (GameHook->isSoulOfCinderDefeated() && sendGoalStatus) {
				sendGoalStatus = false;
				ArchipelagoInterface->gameFinished();
			}
		} else if (initProtectionDelay > 0) {
			int secondsRemaining = (RUN_SLEEP / 1000) * initProtectionDelay;
			spdlog::debug("The mod will be initialized in {} seconds", secondsRemaining);
			initProtectionDelay--;
		}
	}

	WriteSaveFile();

	return;
};

/*
* Permits to remove all received item indexes lower than pLastReceivedIndex from the list.
* It has be to performed after the first connection because we now read the pLastReceivedIndex from the slot_data.
*/
VOID CCore::SkipAlreadyReceivedItems() {
	if (pLastReceivedIndex > ItemRandomiser->receivedItemsQueue.size()) {
		spdlog::warn(
			"Your last_received_index {} is greater than the number of items you've ever "
			"received ({}). This probably means that your local Archipelago save has been "
			"corrupted. The client will fix this automatically, but you'll end up receiving all "
			"your items again.",
			pLastReceivedIndex,
			ItemRandomiser->receivedItemsQueue.size()
		);
		pLastReceivedIndex = 0;
		RemoveOldSaveFile();
		return;
	}

	if (!ItemRandomiser->receivedItemsQueue.empty()) {
		spdlog::debug("Removing {0} items according to the last_received_index", pLastReceivedIndex);
		for (int i = 0; i < pLastReceivedIndex; i++) {
			if (!ItemRandomiser->receivedItemsQueue.empty()) {
				ItemRandomiser->receivedItemsQueue.pop_back();
			}
		}
	}
}

void CCore::RemoveOldSaveFile()
{
	std::filesystem::path brokenPath(savePath);
	brokenPath.replace_filename("(broken) " + savePath.filename().string());
	MoveFileW(WindowsLongPath(savePath).c_str(), WindowsLongPath(brokenPath).c_str());
}


VOID CCore::Panic(const char* pMessage, const char* pSort, DWORD dError, DWORD dIsFatalError) {

	char pOutput[MAX_PATH];
	char pTitle[MAX_PATH];

	sprintf_s(pOutput, "\n%s (%i)\n", pMessage, dError);

	spdlog::critical(pOutput);
	
	if (dIsFatalError) {
		sprintf_s(pTitle, "[Archipelago client - Fatal Error]");
	}
	else {
		sprintf_s(pTitle, "[Archipelago client - Error]");
	};

	MessageBoxA(NULL, pOutput, pTitle, MB_ICONERROR);
	
	if (dIsFatalError) *(int*)0 = 0;

	return;
};


VOID CCore::InputCommand() {
	while (true) {
		std::string line;
		std::getline(std::cin, line);

		if (line == "/help") {
			spdlog::info(
				"List of available commands : \n"
				"/help : Prints this help message.\n"
				"!help : Prints the help message related to Archipelago.\n"
				"/connect {SERVER_IP}:{SERVER_PORT} {SLOT_NAME} [password:{PASSWORD}] : Connect to the specified server.\n"
				"/debug on|off : Prints additional debug info");
		}

#ifdef DEBUG
		if (line.find("/itemGib ") == 0) {
			std::string param = line.substr(9);
			std::cout << "/itemGib executed with " << param << "\n";
			GameHook->itemGib(std::stoi(param));
		}

		if (line.find("/give ") == 0) {
			std::string param = line.substr(6);
			std::cout << "/give executed with " << param << "\n";
			ItemRandomiser->receivedItemsQueue.push_front(std::stoi(param));
		}

		if (line.find("/save") == 0) {
			std::cout << "/save\n";
			Core->saveConfigFiles = true;
		}
			
#endif

		if (line.find("/debug ") == 0) {
			std::string param = line.substr(7);
			BOOL res = (param.find("on") == 0);
			if (res) {
				spdlog::info("Debug logs activated");
				spdlog::default_logger()->set_level(spdlog::level::trace);
			}
			else {
				spdlog::info("Debug logs deactivated");
				spdlog::default_logger()->set_level(
					Core->modEngineDebug ? spdlog::level::debug : spdlog::level::info);
			}

			
		}
		else if (line.find("/connect ") == 0) {
			std::string param = line.substr(9);
			int spaceIndex = param.find(" ");
			if (spaceIndex == std::string::npos) {
				spdlog::warn("Missing parameter: make sure to type '/connect {SERVER_IP}:{SERVER_PORT} {SLOT_NAME} [password:{PASSWORD}]'");
			} else {
				int passwordIndex = param.find("password:");
				std::string address = param.substr(0, spaceIndex);
				std::string slotName = param.substr(spaceIndex + 1, passwordIndex - spaceIndex - 2);
				std::string password = "";
				std::cout << address << " - " << slotName << "\n";
				Core->pSlotName = slotName;
				if (passwordIndex != std::string::npos)
				{
					password = param.substr(passwordIndex + 9);
				}
				Core->pPassword = password;
				if (!ArchipelagoInterface->Initialise(address)) {
					Core->Panic("Failed to initialise Archipelago", "", AP_InitFailed, 1);
					int3
				}
			}
		}
		else if (line.find("!") == 0) {
			ArchipelagoInterface->say(line);
		}
	}
};

void CCore::LoadSaveFile() {
	std::ifstream gameFile{ WindowsLongPath(savePath) };
	if (!gameFile.good()) {
		spdlog::debug("No save file found, starting a new game");
		return;
	}

	spdlog::debug("Reading {}", savePath.string());
	json k;

	try {
		gameFile >> k;
		k.at("last_received_index").get_to(pLastReceivedIndex);
	}
	catch (const std::exception& error) {
		RemoveOldSaveFile();
		spdlog::warn(
			"Failed to read {}: {}\n"
			"You will receive all foreign items again.",
			savePath.string(),
			error.what()
		);
	}
};

VOID CCore::WriteSaveFile() {
	if (!writeSaveFileNextTick) return;
	writeSaveFileNextTick = false;

	json j;
	j["last_received_index"] = pLastReceivedIndex;

	try {
		spdlog::debug("Writing to {}", savePath.string());
		WriteAtomic(savePath, j.dump());
	}
	catch (const std::exception& error) {
		spdlog::warn("Failed to save config: {}", error.what());
	}
}

// Entrypoint called by ModEngine2 to initialize this extension.
bool modengine_ext_init(modengine::ModEngineExtensionConnector* connector,
		modengine::ModEngineExtension** extension) {
	*extension = new CCore(connector);
	return true;
}

