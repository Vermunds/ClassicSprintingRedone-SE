#include "ModConfigUI.h"
#include "Settings.h"

#include "Version.h"

namespace CSR
{
	void FlashHudMenuMeter(std::uint32_t a_meter)
	{
		using func_t = decltype(&FlashHudMenuMeter);
		REL::Relocation<func_t> func(REL::ID{ 52845 });
		return func(a_meter);
	}

	class MenuOpenCloseEventHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_dispatcher) override
		{
			if (!Settings::GetSingleton()->modActive)
			{
				return RE::BSEventNotifyControl::kContinue;
			}

			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

			if (a_event && a_event->opening && a_event->menuName != "LootMenu" && player->playerFlags.isSprinting)
			{
				// Menu was opened, stop sprinting
				player->playerFlags.isSprinting = false;
			}

			return RE::BSEventNotifyControl::kContinue;
		}
	};
	MenuOpenCloseEventHandler g_menuOpenCloseEventHandler;

	using ProcessButton_t = void(RE::SprintHandler*, RE::ButtonEvent*, RE::PlayerControlsData*);
	static REL::Relocation<ProcessButton_t> _ProcessButton;

	void SprintHandler_ProcessButton_Hook(RE::SprintHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
	{
		if (!Settings::GetSingleton()->modActive)
		{
			return _ProcessButton(a_this, a_event, a_data);
		}

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		float stamina = player->GetActorValue(RE::ActorValue::kStamina);

		if (a_event->IsDown())
		{
			if (stamina > 0.0f)
			{
				if (!player->playerFlags.isSprinting)
				{
					// If not sprinting, start sprinting
					player->playerFlags.isSprinting = true;
				}
			}
			else
			{
				// Out of stamina, flash stamina meter
				FlashHudMenuMeter(26);
			}
		}
		else if (a_event->IsUp())
		{
			if (player->playerFlags.isSprinting)
			{
				// If sprinting, stop sprinting
				player->playerFlags.isSprinting = false;
			}
		}
		else if (a_event->IsPressed())
		{
			if (stamina > 0.0f)
			{
				if (!player->playerFlags.isSprinting)
				{
					// If not sprinting, start sprinting
					player->playerFlags.isSprinting = true;
				}
			}
		}
	}
}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type)
	{
	case SKSE::MessagingInterface::kPostLoad:
		CSR::InstallModConfigUI();
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		RE::UI* ui = RE::UI::GetSingleton();
		ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(&CSR::g_menuOpenCloseEventHandler);
		logger::info("Menu open/close event handler sinked.");
		break;
	}
}

extern "C"
{
	DLLEXPORT SKSE::PluginVersionData SKSEPlugin_Version = []() {
		SKSE::PluginVersionData v{};
		v.PluginVersion(REL::Version{ Version::MAJOR, Version::MINOR, Version::PATCH, 0 });
		v.PluginName(Version::NAME);
		v.AuthorName(Version::AUTHOR);
		v.UsesAddressLibrary();
		v.UsesUpdatedStructs();
		v.CompatibleVersions({ SKSE::RUNTIME_SSE_1_7_104 });
		return v;
	}();

	DLLEXPORT bool SKSEPlugin_Load(SKSE::LoadInterface* a_skse)
	{
		SKSE::InitInfo initInfo{};
		initInfo.logLevel = REX::ELogLevel::Trace;
		initInfo.logPattern = "%s(%#): [%^%l%$] %v";
		SKSE::Init(a_skse, initInfo);


		logger::info("{} v{} -({})", Version::FORMATTED_NAME, Version::STRING, __TIMESTAMP__);

		auto messaging = SKSE::GetMessagingInterface();
		if (messaging->RegisterListener("SKSE", MessageHandler))
		{
			logger::info("Messaging interface registration successful.");
		}
		else
		{
			logger::critical("Messaging interface registration failed.");
			return false;
		}

		CSR::LoadSettings();

		REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_SprintHandler[0]);
		CSR::_ProcessButton = vTable.write_vfunc(0x6, &CSR::SprintHandler_ProcessButton_Hook);

		// Force sprint state to sync in every frame
		REL::WriteSafeData(REL::ID{ 40760 }.address() + 0x159, std::uint16_t(0x9090));

		// Skip HUD meter flashing when out of stamina - we handle it ourselves
		REL::WriteSafeData(REL::ID{ 42350 }.address() + 0x350, std::uint8_t(0xEB));

		logger::info("Hooks Installed.");

		return true;
	}
};
