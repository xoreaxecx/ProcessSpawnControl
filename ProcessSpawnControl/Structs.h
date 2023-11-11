#pragma once
#include "Settings.h"
#include <algorithm>
#include <functional>
#include <queue>
#include <string>
#include <Windows.h>

#define PID_DEFAULT (DWORD)-1

enum IgnoredReasons
{
	NOT_IGNORED = 0,
	IS_EXCLUSION = 1,
	NOT_RELATIVE = 2
};

struct Args
{
	DWORD parentPID;		//-pi
	bool controlAll;		//-ca
	bool asExplorer;		//-ae
	bool allowColors;		//-nc
	bool asciiOutput;		//-ao
	wchar_t* targetPath;	//-tp
	wchar_t* targetArgs;	//-ta
	wchar_t* parentPath;	//-pp
	wchar_t* parentArgs;	//-pa
	wchar_t* excPath;		//-ex
	wchar_t** arglist;		//obtained by CommandLineToArgvW()
	wchar_t* loaderPath;
	std::wstring targetName;

	Args()
	{
		parentPID = 0;
		controlAll = false;
		asExplorer = false;
		allowColors = true;
		asciiOutput = false;
		targetPath = NULL;
		targetArgs = NULL;
		parentPath = NULL;
		parentArgs = NULL;
		excPath = NULL;
		arglist = NULL;
		loaderPath = NULL;
	}

	~Args()
	{
		if (loaderPath != NULL)
		{
			delete[] loaderPath;
			loaderPath = NULL;
		}

		if (arglist != NULL)
		{
			LocalFree(arglist);
			arglist = NULL;
		}

		targetPath = NULL;
		targetArgs = NULL;
		parentPath = NULL;
		parentArgs = NULL;
		excPath = NULL;
	}
};

struct ParentProcInfo
{
	DWORD ProcessId;
	std::wstring Name;
	std::wstring PrintableName;
	std::wstring Command;
	ParentProcInfo() { ProcessId = PID_DEFAULT; }
	ParentProcInfo(DWORD PID) { ProcessId = PID; }
	ParentProcInfo(DWORD PID, std::wstring name, std::wstring command)
	{
		ProcessId = PID;
		Name = name;
		PrintableName = name;
		Command = command;
	}
};

struct ProcInfo
{
	DWORD ProcessId;
	std::wstring Name;
	std::wstring PrintableName;
	std::wstring Command;
	HANDLE Handle;
	ParentProcInfo Parent;
	ProcInfo() { ProcessId = PID_DEFAULT; Handle = NULL; Parent = ParentProcInfo(); }
	ProcInfo(DWORD PID, std::wstring name, std::wstring command, HANDLE handle = NULL, ParentProcInfo parent = ParentProcInfo())
	{
		ProcessId = PID;
		Handle = handle;
		Name = name;
		std::transform(Name.begin(), Name.end(), Name.begin(), std::tolower);
		PrintableName = name;
		Command = command;
		Parent = parent;

		if (Settings::ASCII_MODE)
		{
			std::function<bool(wint_t)> notAscii = [](wint_t c) { return !iswascii(c); };
			if (!PrintableName.empty())
				std::replace_if(PrintableName.begin(), PrintableName.end(), notAscii, L'?');
			if (!Command.empty())
				std::replace_if(Command.begin(), Command.end(), notAscii, L'?');
		}
	}
};

struct StoredProc
{
	DWORD EventNumber;
	ProcInfo Proc;
	StoredProc(DWORD eventNumber, ProcInfo proc)
	{
		EventNumber = eventNumber;
		Proc = proc;
	}
};

struct ProcessQueue
{
	CRITICAL_SECTION _cs;
	std::queue<ProcInfo> _q;

	ProcessQueue()
	{
		InitializeCriticalSection(&_cs);
	}

	void PushProc(ProcInfo proc)
	{
		EnterCriticalSection(&_cs);
		_q.push(proc);
		LeaveCriticalSection(&_cs);
	}

	ProcInfo PopProc()
	{
		ProcInfo proc;
		if (!_q.empty())
		{
			EnterCriticalSection(&_cs);
			proc = _q.front();
			_q.pop();
			LeaveCriticalSection(&_cs);
		}

		return proc;
	}

	DWORD Size()
	{
		return _q.size();
	}

	ProcInfo Front()
	{
		if (_q.empty())
			return ProcInfo();
		else
			return _q.front();
	}
	
	ProcInfo Back()
	{
		if (_q.empty())
			return ProcInfo();
		else
			return _q.back();
	}

	bool Empty()
	{
		return _q.empty();
	}
};
