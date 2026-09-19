#include "Settings.h"

#include <SimpleIni.h>

namespace
{
	constexpr const char* INI_PATH = R"(.\Data\SKSE\Plugins\ClassicSprintingRedone.ini)";

	void IniSection(CSimpleIniA& a_ini, const char* a_section, const char* a_comment = nullptr)
	{
		a_ini.SetValue(a_section, nullptr, nullptr, a_comment);
		SKSE::log::info("[{}]", a_section);
	}

	bool IniGetBool(CSimpleIniA& a_ini, const char* a_section, const char* a_key, bool a_default, const char* a_comment = nullptr)
	{
		bool val = a_ini.GetBoolValue(a_section, a_key, a_default);
		a_ini.SetBoolValue(a_section, a_key, val, a_comment, true);
		SKSE::log::info("  {}: {}", a_key, val);
		return val;
	}
}

namespace CSR
{
	Settings* Settings::GetSingleton()
	{
		static Settings singleton;
		return &singleton;
	}

	void LoadSettings()
	{
		Settings* settings = Settings::GetSingleton();

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(INI_PATH);

		SKSE::log::info("Loading settings from: {}", std::filesystem::absolute(INI_PATH).string());

		IniSection(ini, "GENERAL");
		settings->modActive = IniGetBool(ini, "GENERAL", "bModActive", MOD_ACTIVE_DEFAULT_VALUE, "# Turns the mod on and off. While it is off the game handles sprinting the way it does without the mod.");

		SKSE::log::info("Settings loaded.");

		ini.SaveFile(INI_PATH);
	}

	void SaveSettings()
	{
		Settings* settings = Settings::GetSingleton();

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(INI_PATH);

		ini.SetBoolValue("GENERAL", "bModActive", settings->modActive, nullptr, true);

		ini.SaveFile(INI_PATH);

		SKSE::log::info("Settings saved.");
	}

	void RestoreDefaults()
	{
		Settings* settings = Settings::GetSingleton();

		settings->modActive = MOD_ACTIVE_DEFAULT_VALUE;

		SaveSettings();
	}
}
