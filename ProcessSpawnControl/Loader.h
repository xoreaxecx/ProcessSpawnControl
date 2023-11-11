#pragma once
#include "Structs.h"

namespace Loader
{
	bool Drop(wchar_t*& loaderPath);
	bool Run(DWORD* targetPID, Args& args);
}
