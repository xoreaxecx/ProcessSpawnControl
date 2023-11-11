#pragma once
#include <string>
#include <vector>

namespace Settings
{
	//enables replacement of all unicode characters in console output
	extern bool ASCII_MODE;

	//enables suspending for all new processes
	extern bool CONTROL_ALL_MODE;

	//list of processes that cannot be suspended
	extern std::vector<std::wstring> EXCLUSIONS;
}
