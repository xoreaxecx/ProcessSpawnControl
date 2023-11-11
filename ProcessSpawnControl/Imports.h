#pragma once
#include <Windows.h>

typedef LONG(NTAPI* NtSuspendProcess)(IN HANDLE ProcessHandle);
typedef LONG(NTAPI* NtResumeProcess)(IN HANDLE ProcessHandle);

struct _Ntdll
{
	NtSuspendProcess _suspend = NULL;
	NtResumeProcess _resume = NULL;
	bool Loaded = false;

	_Ntdll();
	bool SuspendProcess(HANDLE hProcess);
	bool ResumeProcess(HANDLE hProcess);
};
