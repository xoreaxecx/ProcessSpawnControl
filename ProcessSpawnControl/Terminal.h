#pragma once
#include "Structs.h"

namespace Terminal
{
	void PrintHelp();
	void SetColorAllowance(bool allowColors);
	void PrintProcIgnored(ProcInfo proc, int status);
	void PrintProcSuspended(ProcInfo proc, int qSz = 0);
	void PrintProcResumed(ProcInfo proc);
	void PrintSuspendError(ProcInfo proc);
	void PrintResumeError(ProcInfo proc);
}
