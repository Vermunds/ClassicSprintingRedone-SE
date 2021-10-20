#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "version.h"

constexpr auto MESSAGE_BOX_TYPE = 0x00001010L; // MB_OK | MB_ICONERROR | MB_SYSTEMMODAL

namespace CSR
{
	using FlagBDD = RE::PlayerCharacter::FlagBDD;

	void FlashHudMenuMeter(std::uint32_t a_meter)
	{
		using func_t = decltype(&FlashHudMenuMeter);
		REL::Relocation<func_t> func(REL::ID{ 51907 });
		return func(a_meter);
	}

	class MenuOpenCloseEventHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_dispatcher) override
		{
			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

			if (a_event && a_event->opening && a_event->menuName != "LootMenu" && (player->unkBDD & FlagBDD::kSprinting) != FlagBDD::kNone)
			{
				// Menu was opened, stop sprinting
				player->unkBDD &= static_cast<FlagBDD>(~std::underlying_type_t<FlagBDD>(FlagBDD::kSprinting));
			}

			return RE::BSEventNotifyControl::kContinue;
		}
	};
	MenuOpenCloseEventHandler g_menuOpenCloseEventHandler;

	void SprintHandler_ProcessButton_Hook(RE::SprintHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		float stamina = player->GetActorValue(RE::ActorValue::kStamina);

		if (a_event->IsDown())
		{
			if (stamina > 0.0f)
			{
				if ((player->unkBDD & FlagBDD::kSprinting) == FlagBDD::kNone)
				{
					// If not sprinting, start sprinting
					player->unkBDD |= FlagBDD::kSprinting;
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
			if ((player->unkBDD & FlagBDD::kSprinting) != FlagBDD::kNone)
			{
				// If sprinting, stop sprinting
				player->unkBDD &= static_cast<FlagBDD>(~std::underlying_type_t<FlagBDD>(FlagBDD::kSprinting));
			}
		}
		else if (a_event->IsPressed())
		{
			if (stamina > 0.0f)
			{
				if ((player->unkBDD & FlagBDD::kSprinting) == FlagBDD::kNone)
				{
					// If not sprinting, start sprinting
					player->unkBDD |= FlagBDD::kSprinting;
				}
			}
		}
	}
}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		RE::UI* ui = RE::UI::GetSingleton();
		ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(&CSR::g_menuOpenCloseEventHandler);
		SKSE::log::info("Menu open/close event handler sinked.");
		break;
	}
}

extern "C" {
	DLLEXPORT bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
	{
		assert(SKSE::log::log_directory().has_value());
		auto path = SKSE::log::log_directory().value() / std::filesystem::path("ClassicSprintingRedone.log");
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
		auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

		log->set_level(spdlog::level::trace);
		log->flush_on(spdlog::level::trace);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("%g(%#): [%^%l%$] %v", spdlog::pattern_time_type::local);

		SKSE::log::info("Classic Sprinting Redone v" + std::string(Version::NAME) + " - (" + std::string(__TIMESTAMP__) + ")");

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = Version::PROJECT.data();
		a_info->version = Version::MAJOR;

		if (a_skse->IsEditor()) {
			SKSE::log::critical("Loaded in editor, marking as incompatible!");
			return false;
		}

		if (a_skse->RuntimeVersion() < SKSE::RUNTIME_1_5_39) {
			SKSE::log::critical("Unsupported runtime version " + a_skse->RuntimeVersion().string());
			SKSE::WinAPI::MessageBox(nullptr, std::string("Unsupported runtime version " + a_skse->RuntimeVersion().string()).c_str(), "Classic Sprinting Redone - Error", MESSAGE_BOX_TYPE);
			return false;
		}

		return true;
	}

	DLLEXPORT bool SKSEPlugin_Load(SKSE::LoadInterface* a_skse)
	{
		SKSE::Init(a_skse);

		auto messaging = SKSE::GetMessagingInterface();
		if (messaging->RegisterListener("SKSE", MessageHandler)) {
			SKSE::log::info("Messaging interface registration successful.");
		}
		else {
			SKSE::log::critical("Messaging interface registration failed.");
			SKSE::WinAPI::MessageBox(nullptr, "Messaging interface registration failed.", "Classic Sprinting Redone - Error", MESSAGE_BOX_TYPE);
			return false;
		}

		SKSE::log::info("Classic Sprinting Redone loaded.");

		REL::Relocation<std::uintptr_t> vTable(RE::Offset::SprintHandler::Vtbl);
		vTable.write_vfunc(0x4, &CSR::SprintHandler_ProcessButton_Hook);

		// Force sprint state to sync in every frame
		REL::safe_write(REL::ID{ 39673 }.address() + 0x15A, std::uint16_t(0x9090));

		// Skip HUD meter flashing when out of stamina - we handle it ourselves
		REL::safe_write(REL::ID{ 41271 }.address() + 0x30C, std::uint8_t(0xEB));

		SKSE::log::info("Hooks Installed.");

		return true;
	}
};