#include <iostream>
#include <Windows.h>

void PrintHelp()
{
    std::wcout << "===================================================================================" << std::endl;
    std::wcout << "DL.exe -tp \"target.exe\" [-ta \"-x arg\"] [-pp \"parent.exe\"] [-pa \"-x arg\"] [-pi PID]" << std::endl;
    std::wcout << std::endl;
    std::wcout << "-tp : target path. path to the target file." << std::endl;
    std::wcout << "-ta : target arguments." << std::endl;
    std::wcout << "-pp : parent path. path to the file to run. used as target's parent." << std::endl;
    std::wcout << "-pa : parent arguments." << std::endl;
    std::wcout << "-pi : parent ID. PID of the running process to attach. used as target's parent." << std::endl;
    std::wcout << "===================================================================================" << std::endl;
    std::wcout << std::endl;
    std::wcout << "Press ENTER to exit..." << std::endl;
    std::wcin.ignore();
}

bool CheckArgs(wchar_t*& tp, wchar_t*& ta, wchar_t*& pp, wchar_t*& pa, int* pi, wchar_t**& al)
{
    int nargs;
    al = CommandLineToArgvW(GetCommandLineW(), &nargs);
    if (al == NULL)
    {
        std::wcout << "Can not get arguments" << std::endl;
        std::wcout << "Press ENTER to exit..." << std::endl;
        std::wcin.ignore();
        return false;
    }

    for (int i = 1; i < nargs; i++)
    {
        if ((i + 1) < nargs)
        {
            if (wcscmp(al[i], L"-tp") == 0)
            {
                struct _stat buf;
                if (_wstat(al[i + 1], &buf) == 0)
                {
                    tp = al[++i];
                }
                else
                {
                    std::wcout << "Can not access target file: " << al[++i] << std::endl;
                    std::wcout << "Press ENTER to exit..." << std::endl;
                    std::wcin.ignore();
                    return false;
                }
            }
            else if (wcscmp(al[i], L"-ta") == 0)
            {
                ta = al[++i];
            }
            else if (wcscmp(al[i], L"-pa") == 0)
            {
                pa = al[++i];
            }
            else if (wcscmp(al[i], L"-pp") == 0)
            {
                struct _stat buf;
                if (_wstat(al[i + 1], &buf) == 0)
                {
                    pp = al[++i];
                }
                else
                {
                    std::wcout << "Can not access parent file: " << al[++i] << std::endl;
                    std::wcout << "Press ENTER to exit..." << std::endl;
                    std::wcin.ignore();
                    return false;
                }
            }
            else if (wcscmp(al[i], L"-pi") == 0)
            {
                *pi = _wtoi(al[++i]);
            }
        }
    }

    return true;
}

int CreateSingleProcess(PWSTR path, PWSTR args, bool createSuspended = false)
{
    int pid = 0;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    SecureZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    SecureZeroMemory(&pi, sizeof(pi));

    wchar_t* command = NULL;
    if (args != NULL)
    {
        int comlen = wcslen(path) + wcslen(args) + 2;
        command = new wchar_t[comlen];
        wcscpy_s(command, comlen, path);
        wcscat_s(command, comlen, L" ");
        wcscat_s(command, comlen, args);
    }
    else
    {
        command = path;
    }

    DWORD flags = DETACHED_PROCESS;
    if (createSuspended)
        flags |= CREATE_SUSPENDED;
    bool ok = CreateProcessW(NULL, command, NULL, NULL, FALSE, flags, NULL, NULL, &si, &pi);
    if (ok)
    {
        pid = pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    if (args != NULL)
        delete[] command;

    return pid;
}

int CreateProcessWithParent(DWORD parentPID, PWSTR targetPath, PWSTR targetArgs, bool createSuspended = false)
{
    int targetPID = 0;
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_PROCESS, FALSE, parentPID);
    if (!hProcess)
        return 0;

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
        &hProcess,
        sizeof(hProcess),
        NULL,
        NULL);

    PROCESS_INFORMATION pi;
    RtlSecureZeroMemory(&pi, sizeof(pi));

    //get target arguments
    wchar_t* command = NULL;
    if (targetArgs != NULL)
    {
        int comlen = wcslen(targetPath) + wcslen(targetArgs) + 2;
        command = new wchar_t[comlen];
        wcscpy_s(command, comlen, targetPath);
        wcscat_s(command, comlen, L" ");
        wcscat_s(command, comlen, targetArgs);
    }
    else
    {
        command = targetPath;
    }

    DWORD flags = DETACHED_PROCESS | EXTENDED_STARTUPINFO_PRESENT;
    if (createSuspended)
        flags |= CREATE_SUSPENDED;
    BOOL created = CreateProcessW(NULL,
        command,
        NULL,
        NULL,
        FALSE,
        flags,
        NULL,
        NULL,
        (STARTUPINFO*)&si,
        &pi);

    if (created)
    {
        targetPID = pi.dwProcessId;
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    CloseHandle(hProcess);
    DeleteProcThreadAttributeList(si.lpAttributeList);
    delete[] si.lpAttributeList;

    if (targetArgs != NULL)
        delete[] command;

    return targetPID;
}

int main()
{
    int targetPID = 0;
    int parentPID = 0;
    wchar_t* targetPath = NULL;
    wchar_t* targetArgs = NULL;
    wchar_t* parentPath = NULL;
    wchar_t* parentArgs = NULL;
    wchar_t** arglist = NULL;

    if (CheckArgs(targetPath, targetArgs, parentPath, parentArgs, &parentPID, arglist))
    {
        if (targetPath != NULL)
        {
            if (parentPath != NULL)
            {
                parentPID = CreateSingleProcess(parentPath, parentArgs);
            }

            if (parentPID > 0)
            {
                targetPID = CreateProcessWithParent(parentPID, targetPath, targetArgs, true);
            }
            else
            {
                targetPID = CreateSingleProcess(targetPath, targetArgs, true);
            }
        }
        else
        {
            PrintHelp();
        }
    }

    if (arglist != NULL)
        LocalFree(arglist);

    return targetPID;
}
