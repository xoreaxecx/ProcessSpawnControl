#include "Settings.h"

namespace Settings
{
	bool ASCII_MODE = false;
	bool CONTROL_ALL_MODE = false;
	std::vector<std::wstring> EXCLUSIONS =
	{
		L"dllhost.exe",
		L"searchprotocolhost.exe",
		L"searchfilterhost.exe",
		L"taskhost.exe",
		L"conhost.exe",
		L"backgroundtaskhost.exe",
		L"explorer.exe",
		L"msmpeng.exe"
	};
}
