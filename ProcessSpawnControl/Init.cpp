#include "Global.h"
#include "Init.h"
#include "Settings.h"
#include "Terminal.h"
#include <fstream>
#include <iostream>

namespace Init
{
    bool CheckArgs(Args& args)
    {
        int nargs;
        args.arglist = CommandLineToArgvW(GetCommandLineW(), &nargs);
        if (args.arglist == NULL)
        {
            std::wcout << "Can not get arguments" << std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            return false;
        }

        for (int i = 1; i < nargs; i++)
        {
            if (wcscmp(args.arglist[i], L"-ca") == 0)
            {
                args.controlAll = true;
            }
            else if (wcscmp(args.arglist[i], L"-ae") == 0)
            {
                args.asExplorer = true;
            }
            else if (wcscmp(args.arglist[i], L"-nc") == 0)
            {
                args.allowColors = false;
            }
            else if (wcscmp(args.arglist[i], L"-ao") == 0)
            {
                args.asciiOutput = true;
            }
            else if ((i + 1) < nargs)
            {
                if (wcscmp(args.arglist[i], L"-tp") == 0)
                {
                    //check target file exists
                    struct _stat buf;
                    if (_wstat(args.arglist[i + 1], &buf) == 0)
                    {
                        args.targetPath = args.arglist[++i];
                    }
                    else
                    {
                        std::wcout << "Can not access target file: " << args.arglist[++i] << std::endl;
                        std::wcout << "Press ENTER to exit..." << std::endl;
                        std::wcin.ignore();
                        return false;
                    }
                }
                else if (wcscmp(args.arglist[i], L"-ta") == 0)
                {
                    args.targetArgs = args.arglist[++i];
                }
                else if (wcscmp(args.arglist[i], L"-pa") == 0)
                {
                    args.parentArgs = args.arglist[++i];
                }
                else if (wcscmp(args.arglist[i], L"-pp") == 0)
                {
                    //check parent file exists
                    struct _stat buf;
                    if (_wstat(args.arglist[i + 1], &buf) == 0)
                    {
                        args.parentPath = args.arglist[++i];
                    }
                    else
                    {
                        std::wcout << "Can not access parent file: " << args.arglist[++i] << std::endl;
                        std::wcout << "Press ENTER to exit..." << std::endl;
                        std::wcin.ignore();
                        return false;
                    }
                }
                else if (wcscmp(args.arglist[i], L"-pi") == 0)
                {
                    args.parentPID = _wtoi(args.arglist[++i]);
                }
                else if (wcscmp(args.arglist[i], L"-ex") == 0)
                {
                    //check exclusions file exists
                    struct _stat buf;
                    if (_wstat(args.arglist[i + 1], &buf) == 0)
                    {
                        args.excPath = args.arglist[++i];
                    }
                    else
                    {
                        std::wcout << "Can not access exclusions file: " << args.arglist[++i] << std::endl;
                        std::wcout << "Press ENTER to exit..." << std::endl;
                        std::wcin.ignore();
                        return false;
                    }
                }
            }
        }

        if (args.targetPath == NULL)
        {
            std::wcout << "Required argument \"-tp\" not specified" << std::endl;
            std::wcout << std::endl;
            Terminal::PrintHelp();
            return false;
        }
        else
        {
            wchar_t* sub = wcsrchr(args.targetPath, L'\\');
            if (sub == NULL)
                args.targetName = std::wstring(args.targetPath);
            else
                args.targetName = std::wstring(++sub);
        }

        if (!Global::NtFuncs.Loaded)
        {
            std::wcout << "Can not load ntdll functions"<< std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            return false;
        }

        if (args.excPath != NULL)
        {
            std::wifstream inFile;
            inFile.open(args.excPath);
            if (inFile.fail())
            {
                inFile.close();
                std::wcout << "Can not read exclusions file: " << args.excPath << std::endl;
                std::wcout << "Press ENTER to exit..." << std::endl;
                std::wcin.ignore();
                return false;
            }

            std::wstring line;
            while (getline(inFile, line))
            {
                if (!line.empty())
                {
                    std::transform(line.begin(), line.end(), line.begin(), std::tolower);
                    Settings::EXCLUSIONS.push_back(line);
                }
            }
        }

        Settings::CONTROL_ALL_MODE = args.controlAll;
        Settings::ASCII_MODE = args.asciiOutput;
        Terminal::SetColorAllowance(args.allowColors);

        return true;
    }
}
