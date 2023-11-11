#include "Init.h"
#include "Loader.h"
#include "SpawnControl.h"

int main()
{
    Args args;
    bool ok = false;
    ok = Init::CheckArgs(args);
    if (!ok)
        return 1;

    ok = Loader::Drop(args.loaderPath);
    if (!ok)
        return 2;

    DWORD targetPID = 0;
    ok = Loader::Run(&targetPID, args);
    if (!ok)
        return 3;

    HANDLE hTarget = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
    std::wstring command;
    if (args.targetArgs)
        command = args.targetArgs;
    ProcInfo target = ProcInfo(targetPID, args.targetName, command, hTarget);
    ok = SpawnControl::Start(target);
    if (!ok)
        return 4;
    
    return 0;
}

