#include "Loader.h"
#include "resource.h"
#include <iostream>

namespace Loader
{
    bool Drop(wchar_t*& loaderPath)
    {
        bool result = true;
        wchar_t loaderName[] = L"\\DL.exe";
        wchar_t* ownPath = new wchar_t[MAX_PATH];
        GetModuleFileNameW(NULL, ownPath, MAX_PATH);
        wchar_t* pos = wcsrchr(ownPath, L'\\');

        if (pos == NULL)
        {
            std::wcout << "Can not get exe name from base path: " << ownPath << std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            result = false;
        }
        else
        {
            int baseLen = pos - ownPath;
            int bufSz = baseLen + wcslen(loaderName) + 1;
            loaderPath = new wchar_t[bufSz];
            int wcsOk = wcsncpy_s(loaderPath, bufSz, ownPath, baseLen);
            wcsOk += wcscat_s(loaderPath, bufSz, loaderName);

            if (wcsOk > 0)
            {
                std::wcout << "Can not get loader name from base path: " << ownPath << std::endl;
                std::wcout << "Press ENTER to exit..." << std::endl;
                std::wcin.ignore();
                result = false;
            }
            else
            {
                //if loader file does not exist
                struct _stat buf;
                if (_wstat(loaderPath, &buf) != 0)
                {
                    HRSRC hRes = NULL;
                    HGLOBAL hMemory = NULL;
                    PBYTE data = NULL;
                    size_t dataSz = 0;
                    HMODULE hModule = GetModuleHandleW(NULL);

                    hRes = FindResourceW(hModule, MAKEINTRESOURCEW(IDR_BIN_DL1), L"BIN_DL");
                    if (hRes != NULL)
                    {
                        hMemory = LoadResource(hModule, hRes);
                        dataSz = SizeofResource(hModule, hRes);
                        if (hMemory != NULL)
                        {
                            data = (PBYTE)LockResource(hMemory);
                        }
                    }

                    if (data == NULL || dataSz == 0)
                    {
                        std::wcout << "Can not load resource with DL.exe" << std::endl;
                        std::wcout << "Press ENTER to exit..." << std::endl;
                        std::wcin.ignore();
                        result = false;
                    }
                    else
                    {
                        FILE* file;
                        errno_t err = _wfopen_s(&file, loaderPath, L"wb");
                        if (file == NULL || err)
                        {
                            std::wcout << "Can not write DL.exe to file" << std::endl;
                            std::wcout << "Press ENTER to exit..." << std::endl;
                            std::wcin.ignore();
                            result = false;
                        }
                        else
                        {
                            fwrite(data, sizeof(data[0]), dataSz, file);
                            fclose(file);
                        }
                    }
                }
            }
        }

        delete[] ownPath;

        return result;
    }

    void RunAsOwnSub(PWSTR loaderCommand, HANDLE& hProcess, DWORD& pid)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;

        SecureZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        SecureZeroMemory(&pi, sizeof(pi));

        if (CreateProcessW(NULL, loaderCommand, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
        {
            hProcess = pi.hProcess;
            pid = pi.dwProcessId;
            CloseHandle(pi.hThread);
        }
    }

    void RunAsExplorer(PWSTR loaderCommand, HANDLE& hProcess, DWORD& pid)
    {
        DWORD shellPID = 0;
        GetWindowThreadProcessId(GetShellWindow(), &shellPID);
        HANDLE hShell = OpenProcess(PROCESS_CREATE_PROCESS, FALSE, shellPID);
        if (hShell == NULL)
        {
            std::wcout << "Can not open shell process" << std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            return;
        }

        SIZE_T size = 0;
        //get required size
        InitializeProcThreadAttributeList(NULL, 1, 0, &size);

        STARTUPINFOEX si = { sizeof(si) };
        SecureZeroMemory(&si, sizeof(si));
        si.StartupInfo.cb = sizeof(STARTUPINFOEX);
        char* siBuff = new char[size] { 0 };
        si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)siBuff;

        //allocate buffer with required size
        InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &size);

        //add parent attribute
        UpdateProcThreadAttribute(si.lpAttributeList,
            0,
            PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
            &hShell,
            sizeof(hShell),
            NULL,
            NULL);

        PROCESS_INFORMATION pi;
        RtlSecureZeroMemory(&pi, sizeof(pi));

        BOOL created = CreateProcessW(NULL,
            loaderCommand,
            NULL,
            NULL,
            FALSE,
            DETACHED_PROCESS | EXTENDED_STARTUPINFO_PRESENT,
            NULL,
            NULL,
            (STARTUPINFO*)&si,
            &pi);

        if (created)
        {
            hProcess = pi.hProcess;
            pid = pi.dwProcessId;
            CloseHandle(pi.hThread);
        }
        CloseHandle(hShell);
        DeleteProcThreadAttributeList(si.lpAttributeList);
        delete[] si.lpAttributeList;
    }

    void GetCommand(wchar_t*& command, Args& args)
    {
        int strKeyLen = 7; //e.g. [ -tp ""]
        int intKeyLen = 5; //e.g. [ -pi ]
        int pidLen = 0;
        int pidPos = 0;
        int commandLen = wcslen(args.loaderPath) + wcslen(args.targetPath) + strKeyLen;
        if (args.targetArgs != NULL)
            commandLen += wcslen(args.targetArgs) + strKeyLen;
        if (args.parentPath != NULL)
            commandLen += wcslen(args.parentPath) + strKeyLen;
        if (args.parentArgs != NULL)
            commandLen += wcslen(args.parentArgs) + strKeyLen;
        if (args.parentPID > 0)
        {
            pidPos = commandLen + intKeyLen;
            pidLen = _scwprintf(L"%d", args.parentPID);
            commandLen += pidLen + intKeyLen;
        }
        commandLen += 1; //for termination zero
        command = new wchar_t[commandLen];

        wcscpy_s(command, commandLen, args.loaderPath);

        wcscat_s(command, commandLen, L" -tp \"");
        wcscat_s(command, commandLen, args.targetPath);
        wcscat_s(command, commandLen, L"\"");

        if (args.targetArgs != NULL)
        {
            wcscat_s(command, commandLen, L" -ta \"");
            wcscat_s(command, commandLen, args.targetArgs);
            wcscat_s(command, commandLen, L"\"");
        }
        if (args.parentPath != NULL)
        {
            wcscat_s(command, commandLen, L" -pp \"");
            wcscat_s(command, commandLen, args.parentPath);
            wcscat_s(command, commandLen, L"\"");
        }
        if (args.parentArgs != NULL)
        {
            wcscat_s(command, commandLen, L" -pa \"");
            wcscat_s(command, commandLen, args.parentArgs);
            wcscat_s(command, commandLen, L"\"");
        }
        if (args.parentPID > 0)
        {
            wcscat_s(command, commandLen, L" -pi ");
            swprintf_s(command + pidPos, pidLen + 1, L"%d", args.parentPID);
        }
    }

    bool Run(DWORD* targetPID, Args& args)
    {
        bool result = true;
        HANDLE hLoader = NULL;
        DWORD pidLoader = PID_DEFAULT;
        wchar_t* command = NULL;
        GetCommand(command, args);

        if (command == NULL)
        {
            std::wcout << "Can not get loader command" << std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            result = false;
        }
        else
        {
            //check loader still exists
            struct _stat buf;
            if (_wstat(args.loaderPath, &buf) != 0)
            {
                std::wcout << "Can not access loader from path: " << args.loaderPath << std::endl;
                std::wcout << "Press ENTER to exit..." << std::endl;
                std::wcin.ignore();
                result = false;
            }
            else
            {
                if (args.asExplorer)
                {
                    RunAsExplorer(command, hLoader, pidLoader);
                }
                else
                {
                    RunAsOwnSub(command, hLoader, pidLoader);
                }

                if (hLoader == NULL || pidLoader == PID_DEFAULT)
                {
                    std::wcout << "Can not run Loader" << std::endl;
                    std::wcout << "Press ENTER to exit..." << std::endl;
                    std::wcin.ignore();
                    result = false;
                }
                else
                {
                    WaitForSingleObject(hLoader, 10000);
                    if (GetExitCodeProcess(hLoader, targetPID) == FALSE)
                    {
                        CloseHandle(hLoader);
                        std::wcout << "Can not receive Loader exit code" << std::endl;
                        std::wcout << "Press ENTER to exit..." << std::endl;
                        std::wcin.ignore();
                        result = false;
                    }
                    else if (*targetPID == STILL_ACTIVE)
                    {
                        HANDLE hStillRunning = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pidLoader);
                        if (hStillRunning)
                        {
                            CloseHandle(hStillRunning);
                            CloseHandle(hLoader);
                            std::wcout << "Loader timeout expired" << std::endl;
                            std::wcout << "Press ENTER to exit..." << std::endl;
                            std::wcin.ignore();
                            result = false;
                        }
                    }
                }
            }
        }

        if (command != NULL)
            delete[] command;

        if (*targetPID == 0 || *targetPID == PID_DEFAULT)
        {
            std::wcout << "Invalid target PID received" << std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            result = false;
        }

        return result;
    }
}
