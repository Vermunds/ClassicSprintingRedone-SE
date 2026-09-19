#pragma once

namespace CSR
{
	// General
	constexpr bool MOD_ACTIVE_DEFAULT_VALUE = true;

	class Settings
	{
	public:
		static Settings* GetSingleton();

		// General
		bool modActive;

	private:
		Settings() {};
		~Settings() {};
		Settings(const Settings&) = delete;
		Settings& operator=(const Settings&) = delete;
	};

	void LoadSettings();
	void SaveSettings();
	void RestoreDefaults();
}
