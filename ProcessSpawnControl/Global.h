#pragma once
#include "Imports.h"
#include "Structs.h"

namespace Global
{
	//application termination indicator
	extern bool isRunning;

	//NtSuspendProcess and NtResumeProcess from Ntdll
	extern _Ntdll NtFuncs;

	//queue for processes from WMI event
	extern ProcessQueue processQueue;

	//used to prevent relatives from being missed
	//when an event returns processes in the wrong order
	extern std::vector<StoredProc> storedProcs;
	extern DWORD eventCount;

	//list of subprocesses of the target process
	extern std::vector<ProcInfo> relatives;

	//variables for the count of times the "CTRL + C" is pressed
	extern volatile int ctrlCcount;
	extern volatile long long lastTime;
}
