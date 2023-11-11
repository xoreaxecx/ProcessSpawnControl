#include "Terminal.h"
#include <iostream>

namespace Terminal
{
	struct ConsoleColorizer
	{
		CRITICAL_SECTION cs;
		bool colorsAllowed = false;
		HANDLE hConsole = NULL;
		UINT idx = 0;
		UINT lastIdx = 0;
		int sz = 0;
		const int colors[5] =
		{
			11, //azure
			6,  //yellow
			13, //purple
			10, //green
			12  //light red
		};
		int defaultColor = 0;
		int warningColor = 4;

		ConsoleColorizer()
		{
			InitializeCriticalSection(&cs);
			sz = sizeof(colors) / sizeof(colors[0]);
			hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			if (hConsole != INVALID_HANDLE_VALUE)
			{
				CONSOLE_SCREEN_BUFFER_INFO csbi;
				if (GetConsoleScreenBufferInfo(hConsole, &csbi))
				{
					defaultColor = csbi.wAttributes;
					colorsAllowed = true;
				}
			}
		}

		void DisableColors()
		{
			colorsAllowed = false;
		}

		void SetNextColor()
		{
			if (colorsAllowed)
			{
				lastIdx = idx;
				SetConsoleTextAttribute(hConsole, colors[idx++ % sz]);
			}
		}

		void SetLastColor()
		{
			if (colorsAllowed)
				SetConsoleTextAttribute(hConsole, colors[lastIdx % sz]);
		}

		void SetWarningColor()
		{
			if (colorsAllowed)
				SetConsoleTextAttribute(hConsole, warningColor);
		}

		void SetDefaultColor()
		{
			if (colorsAllowed)
				SetConsoleTextAttribute(hConsole, defaultColor);
		}

	} colorizer;

	void PrintHelp()
	{
		std::wcout << "=====================================================================================" << std::endl;
		std::wcout << "PSC.exe -tp \"target.exe\" [-ta \"-x arg\"] [-pp \"parent.exe\"] [-pa \"-x arg\"] [-pi PID] \n" \
			"[-ex \"exclusions.txt\"] [-ca] [-ae] [-nc] [-ao]" << std::endl;
		std::wcout << std::endl;
		std::wcout << "-tp : target path. path to the target file." << std::endl;
		std::wcout << "-ta : target arguments." << std::endl;
		std::wcout << "-pp : parent path. path to the file to run. used as target's parent." << std::endl;
		std::wcout << "-pa : parent arguments." << std::endl;
		std::wcout << "-pi : parent ID. PID of the running process to attach. used as target's parent." << std::endl;
		std::wcout << "-ex : exclusions. path to the list of exclusions. process names separated by newline." << std::endl;
		std::wcout << "-ca : control all. suspend all spawned processes except the list of exclusions." << std::endl;
		std::wcout << "-ae : as explorer. run processes with explorer privileges. as admin if omitted." << std::endl;
		std::wcout << "-nc : no colors. use only default text color in console output." << std::endl;
		std::wcout << "-ao : ascii output. replace all unicode chars with \"?\" in console output." << std::endl;
		std::wcout << "=====================================================================================" << std::endl;
		std::wcout << std::endl;
		std::wcout << "Press ENTER to exit..." << std::endl;
		std::wcin.ignore();
	}

	void SetColorAllowance(bool allowColors)
	{
		if (!allowColors)
			colorizer.DisableColors();
	}

	void PrintProcIgnored(ProcInfo proc, int status)
	{
		EnterCriticalSection(&colorizer.cs);
		std::wcout << std::endl;
		std::wcout << "Process \"" << proc.PrintableName << " (" << proc.ProcessId << ")\" ignored: ";
		switch (status)
		{
		case IgnoredReasons::IS_EXCLUSION:
			std::wcout << "specified in the list of exclusions" << std::endl;
			break;
		case IgnoredReasons::NOT_RELATIVE:
			std::wcout << "not a relative of the target" << std::endl;
			break;
		default:
			std::wcout << "unknown reason for ignoring" << std::endl;
			break;
		}
		std::wcout << std::endl;
		LeaveCriticalSection(&colorizer.cs);
	}

	void PrintProcSuspended(ProcInfo proc, int qsz)
	{
		EnterCriticalSection(&colorizer.cs);
		colorizer.SetNextColor();
		std::wcout << std::endl;
		std::wcout << std::endl;
		std::wcout << "================================================" << std::endl;
		std::wcout << "New process suspended. Queue size: " << qsz << std::endl;
		std::wcout << "------------------------------------------------" << std::endl;
		std::wcout << "Process info:" << std::endl;
		std::wcout << "-------------" << std::endl;
		std::wcout << "Name: " << proc.PrintableName << std::endl;
		std::wcout << "PID : " << proc.ProcessId << std::endl;
		std::wcout << "CMD : " << proc.Command << std::endl;
		std::wcout << "------------------------------------------------" << std::endl;
		if (proc.Parent.ProcessId != PID_DEFAULT)
		{
			std::wcout << "Parent info:" << std::endl;
			std::wcout << "-------------" << std::endl;
			if (!proc.Parent.PrintableName.empty())
				std::wcout << "Name: " << proc.Parent.PrintableName << std::endl;
			std::wcout << "PID : " << proc.Parent.ProcessId << std::endl;
			if (!proc.Parent.Command.empty())
				std::wcout << "CMD : " << proc.Parent.Command << std::endl;
			std::wcout << "------------------------------------------------" << std::endl;
		}
		std::wcout << "         PRESS ENTER TO RESUME PROCESS          " << std::endl;
		std::wcout << "================================================" << std::endl;
		std::wcout << std::endl;
		colorizer.SetDefaultColor();
		LeaveCriticalSection(&colorizer.cs);
		std::wcin.ignore();
	}

	void PrintProcResumed(ProcInfo proc)
	{
		EnterCriticalSection(&colorizer.cs);
		colorizer.SetLastColor();
		std::wcout << std::endl;
		std::wcout << "Process \"" << proc.PrintableName << " (" << proc.ProcessId << ")\" resumed" << std::endl;
		std::wcout << std::endl;
		colorizer.SetDefaultColor();
		LeaveCriticalSection(&colorizer.cs);
	}

	void PrintSuspendError(ProcInfo proc)
	{
		EnterCriticalSection(&colorizer.cs);
		colorizer.SetWarningColor();
		std::wcout << std::endl;
		std::wcout << L"------------ SUSPEND ERROR ------------" << std::endl;
		std::wcout << L"------- can not suspend process -------" << std::endl;
		std::wcout << L"Name: " << proc.PrintableName << std::endl;
		std::wcout << L"PID : " << proc.ProcessId << std::endl;
		std::wcout << L"CMD : " << proc.Command << std::endl;
		if (proc.Parent.ProcessId != PID_DEFAULT)
		{
			std::wcout << L"-----" << std::endl;
			if (!proc.Parent.PrintableName.empty())
				std::wcout << L"Parent Name: " << proc.Parent.PrintableName << std::endl;
			std::wcout << L"Parent PID : " << proc.Parent.ProcessId << std::endl;
			if (!proc.Parent.Command.empty())
				std::wcout << L"Parent CMD : " << proc.Parent.Command << std::endl;
		}
		std::wcout << L"-----------------------------------------" << std::endl;
		std::wcout << std::endl;
		colorizer.SetDefaultColor();
		LeaveCriticalSection(&colorizer.cs);
	}

	void PrintResumeError(ProcInfo proc)
	{
		EnterCriticalSection(&colorizer.cs);
		colorizer.SetWarningColor();
		std::wcout << std::endl;
		std::wcout << L"------------ RESUME ERROR ------------" << std::endl;
		std::wcout << L"------- can not resume process -------" << std::endl;
		std::wcout << L"Name: " << proc.PrintableName << std::endl;
		std::wcout << L"PID : " << proc.ProcessId << std::endl;
		std::wcout << L"CMD : " << proc.Command << std::endl;
		if (proc.Parent.ProcessId != PID_DEFAULT)
		{
			std::wcout << L"-----" << std::endl;
			if (!proc.Parent.PrintableName.empty())
				std::wcout << L"Parent Name: " << proc.Parent.PrintableName << std::endl;
			std::wcout << L"Parent PID : " << proc.Parent.ProcessId << std::endl;
			if (!proc.Parent.Command.empty())
				std::wcout << L"Parent CMD : " << proc.Parent.Command << std::endl;
		}
		std::wcout << L"----------------------------------------" << std::endl;
		std::wcout << std::endl;
		colorizer.SetDefaultColor();
		LeaveCriticalSection(&colorizer.cs);
	}
}
