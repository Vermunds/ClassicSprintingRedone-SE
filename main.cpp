#include "RE/Skyrim.h"
#include "SKSE/API.h"
#include "REL/Relocation.h"

#include "version.h"

inline constexpr REL::ID SprintHandler_ProcessButton(static_cast<std::uint64_t>(41358));

enum FlagBDD : UInt8
{
	kNone = 0,
	kSprinting = 1 << 0
	//...
};

class MenuOpenCloseEventHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_dispatcher) override
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

		if (a_event && a_event->opening && (player->unkBDD & FlagBDD::kSprinting) != FlagBDD::kNone)
		{
			player->unkBDD &= ~FlagBDD::kSprinting;
		}

		return RE::BSEventNotifyControl::kContinue;
	}
};
MenuOpenCloseEventHandler g_menuOpenCloseEventHandler;

void SprintHandler_ProcessButton_Hook(RE::SprintHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
{
	RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

	if (a_event->IsUp() && (player->unkBDD & FlagBDD::kSprinting) != FlagBDD::kNone)
	{
		player->unkBDD &= ~FlagBDD::kSprinting;
	}

	using func_t = decltype(&RE::SprintHandler::ProcessButton);
	REL::Offset<func_t> func(SprintHandler_ProcessButton);
	return func(a_this, a_event, a_data);
}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		RE::UI* ui = RE::UI::GetSingleton();
		ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(&g_menuOpenCloseEventHandler);
		_MESSAGE("Menu open/close event handler sinked.");
		break;
	}
}

void ShowErrorMessage(std::string a_error, std::string a_desc = "")
{
	_FATALERROR(a_error.c_str());
	int result;
	if (a_desc != "")
	{
		 result = MessageBoxA(nullptr, (a_error + "\n\n" + a_desc + "\n\nPress OK to continue with the mod disabled.\nPress CANCEL to exit.").c_str(), "Classic Sprinting Redone - Error", MB_OKCANCEL | MB_ICONERROR | MB_DEFBUTTON2 | MB_SYSTEMMODAL);

	}
	else
	{
		 result = MessageBoxA(nullptr, (a_error + "\n\nPress OK to continue with the mod disabled.\nPress CANCEL to exit.").c_str(), "Classic Sprinting Redone - Error", MB_OKCANCEL | MB_ICONERROR | MB_DEFBUTTON2 | MB_SYSTEMMODAL);
	}
	if (result == IDCANCEL)
	{
		DWORD pid = GetCurrentProcessId();
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, pid);
		TerminateProcess(hProcess, EXIT_FAILURE);
		CloseHandle(hProcess);
	}
}

extern "C" {
	bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
	{
		SKSE::Logger::OpenRelative(FOLDERID_Documents, L"\\My Games\\Skyrim Special Edition\\SKSE\\ClassicSprintingRedone.log");
		SKSE::Logger::SetPrintLevel(SKSE::Logger::Level::kDebugMessage);
		SKSE::Logger::SetFlushLevel(SKSE::Logger::Level::kDebugMessage);
		SKSE::Logger::UseLogStamp(true);

		_MESSAGE("Classic Sprinting Redone v%s for Skyrim Special Edition initializing...", CSR_VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "Classic Sprinting Redone";
		a_info->version = CSR_VERSION_MAJOR;

		if (a_skse->IsEditor()) {
			_FATALERROR("Loaded in editor, marking as incompatible!");
			return false;
		}


		if (a_skse->RuntimeVersion() < SKSE::RUNTIME_1_5_3) {
			ShowErrorMessage("Unsupported runtime version " + a_skse->RuntimeVersion().GetString(), "Please update your game to a more recent version to use this mod.");
			return false;
		}

		if (SKSE::AllocTrampoline(1024 * 8))
		{
			_MESSAGE("Trampoline creation successful.");
		}
		else {
			ShowErrorMessage("Trampoline creation failed!", "This might be a result of too many SKSE mods being loaded. Try to disable other SKSE mods to confirm.");
			return false;
		}

		//Check if Address Library is available
		std::string fileName = "Data/SKSE/Plugins/version-" + std::to_string(a_skse->RuntimeVersion().GetMajor()) + "-" + std::to_string(a_skse->RuntimeVersion().GetMinor()) + "-" + std::to_string(a_skse->RuntimeVersion().GetRevision()) + "-" + std::to_string(a_skse->RuntimeVersion().GetBuild()) + ".bin";
		if (!std::filesystem::exists(fileName))
		{
			ShowErrorMessage("Address Library for SKSE Plugins not found for runtime version " + a_skse->RuntimeVersion().GetString(), "This mod requires it to function. Please install it.\nIf the game just updated please wait until a new version of the library is available.");
			return false;
		}

		return true;
	}

	bool SKSEPlugin_Load(SKSE::LoadInterface* a_skse)
	{
		if (!SKSE::Init(a_skse)) {
			ShowErrorMessage("SKSE initialization failed!");
			return false;
		}

		auto messaging = SKSE::GetMessagingInterface();
		if (messaging->RegisterListener("SKSE", MessageHandler)) {
			_MESSAGE("Messaging interface registration successful.");
		}
		else {
			ShowErrorMessage( "Messaging interface registration failed!");
			return false;
		}

		_MESSAGE("Classic Sprinting Redone loaded.");

		REL::Offset<std::uintptr_t> vTable(RE::Offset::SprintHandler::Vtbl);
		vTable.write_vfunc(0x4, &SprintHandler_ProcessButton_Hook);

		REL::SafeWrite16(SprintHandler_ProcessButton.address() + 0x16, 0x9090);
		REL::SafeWrite8(SprintHandler_ProcessButton.address() + 0x6A, 0x0A); // xor -> or

		return true;
	}

};
