#include "Global.h"

namespace Global
{
	bool isRunning = true;
	_Ntdll NtFuncs;
	ProcessQueue processQueue;
	std::vector<StoredProc> storedProcs;
	DWORD eventCount = 0;
	std::vector<ProcInfo> relatives;
	volatile int ctrlCcount = 0;
	volatile long long lastTime = 0;
}
