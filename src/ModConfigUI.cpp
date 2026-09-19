#include "ModConfigUI.h"

#include "Settings.h"
#include "Version.h"

#include <ModConfigUI/Localization.h>

namespace CSR
{
	const char* Translate(const char* a_key)
	{
		return ModConfigUI::Localization::Get(a_key);
	}

	// Pages
	void DrawSettingsPage(ModConfigUI::Renderer& a_renderer)
	{
		Settings* settings = Settings::GetSingleton();

		a_renderer.SeparatorText(Translate("$CSR_Section_General"));

		if (a_renderer.Checkbox(Translate("$CSR_ModActive"), &settings->modActive, MOD_ACTIVE_DEFAULT_VALUE, Translate("$CSR_ModActive_Tooltip")))
		{
			SaveSettings();
		}
	}

	void InstallModConfigUI()
	{
		static constexpr ModConfigUI::ModInfo MOD_INFO{
			.pluginName = Version::NAME.data(),
			.displayName = Version::FORMATTED_NAME.data(),
			.version = Version::STRING.data(),
			.author = Version::AUTHOR.data(),
			.description = "$CSR_Description",
			.nexusUrl = "https://www.nexusmods.com/skyrimspecialedition/mods/20166",
			.sourceUrl = "https://github.com/Vermunds/ClassicSprintingRedone-SE"
		};

		static constexpr ModConfigUI::Page PAGES[] = {
			{ "$CSR_Page_Settings", &DrawSettingsPage }
		};

		ModConfigUI::Install(MOD_INFO, PAGES, &RestoreDefaults);
	}
}
