#include "EventSink.h"
#include "Global.h"
#include "Settings.h"
#include "Terminal.h"

ULONG EventSink::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG EventSink::Release()
{
    LONG lRef = InterlockedDecrement(&m_lRef);
    if (lRef == 0)
        delete this;
    return lRef;
}

HRESULT EventSink::QueryInterface(REFIID riid, void** ppv)
{
	if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
	{
		*ppv = (IWbemObjectSink*)this;
		AddRef();
		return WBEM_S_NO_ERROR;
	}
	else return E_NOINTERFACE;
}

bool IsNotExclusion(ProcInfo proc)
{
    for (UINT i = 0; i < Settings::EXCLUSIONS.size(); i++)
    {
        if (proc.Name == Settings::EXCLUSIONS[i])
            return false;
    }

    return true;
}

std::vector<ProcInfo> SearchRelatives(std::vector<ProcInfo>& procList)
{
    Global::eventCount++;
    //clear old entries from storage
    for (UINT i = 0; i < Global::storedProcs.size(); i++)
    {
        if ((DWORD)(Global::eventCount - Global::storedProcs[i].EventNumber) >= 20)
        {
            Global::storedProcs.erase(std::next(Global::storedProcs.begin(), i));
        }
    }

    std::vector<ProcInfo> foundRelatives;
    UINT lastSz = 0;
    while (lastSz != procList.size() && procList.size() > 0)
    {
        lastSz = procList.size();
        for (UINT i = 0; i < procList.size(); i++)
        {
            for (UINT j = 0; j < Global::relatives.size() && procList.size() > 0; j++)
            {
                if (procList[i].Parent.ProcessId == Global::relatives[j].ProcessId)
                {
                    Global::relatives.push_back(procList[i]);
                    Global::relatives.back().Parent.Name = Global::relatives[j].Name;
                    Global::relatives.back().Parent.PrintableName = Global::relatives[j].PrintableName;
                    Global::relatives.back().Parent.Command = Global::relatives[j].Command;
                    foundRelatives.push_back(Global::relatives.back());
                    procList.erase(std::next(procList.begin(), i));
                    i--;
                    break;
                }
            }
        }
    }

    bool checkStorage = !Global::storedProcs.empty();
    //store remaining procs
    for (UINT i = 0; i < procList.size(); i++)
    {
        Global::storedProcs.push_back(StoredProc(Global::eventCount, procList[i]));
    }

    if (checkStorage)
    {
        //search parents in storage
        lastSz = 0;
        while (lastSz != Global::storedProcs.size() && Global::storedProcs.size() > 0)
        {
            lastSz = Global::storedProcs.size();
            for (UINT i = 0; i < Global::storedProcs.size(); i++)
            {
                for (UINT j = 0; j < Global::relatives.size() && Global::storedProcs.size() > 0; j++)
                {
                    if (Global::storedProcs[i].Proc.Parent.ProcessId == Global::relatives[j].ProcessId)
                    {
                        Global::relatives.push_back(Global::storedProcs[i].Proc);
                        Global::relatives.back().Parent.Name = Global::relatives[j].Name;
                        Global::relatives.back().Parent.PrintableName = Global::relatives[j].PrintableName;
                        Global::relatives.back().Parent.Command = Global::relatives[j].Command;
                        foundRelatives.push_back(Global::relatives.back());
                        Global::storedProcs.erase(std::next(Global::storedProcs.begin(), i));
                        i--;
                        break;
                    }
                }
            }
        }
    }

    for (UINT i = 0; i < foundRelatives.size(); i++)
    {
        if (IsNotExclusion(foundRelatives[i]))
        {
            HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, foundRelatives[i].ProcessId);
            bool suspended = Global::NtFuncs.SuspendProcess(handle);
            if (suspended)
            {
                foundRelatives[i].Handle = handle;
                Global::processQueue.PushProc(foundRelatives[i]);
            }
            else
            {
                CloseHandle(handle);
                Terminal::PrintSuspendError(foundRelatives[i]);
            }
        }
        else
        {
            Terminal::PrintProcIgnored(foundRelatives[i], IgnoredReasons::IS_EXCLUSION);
        }
    }

    return foundRelatives;
}

HRESULT EventSink::Indicate(LONG lObjectCount, IWbemClassObject** apObjArray)
{
    HRESULT hres = S_OK;
    _variant_t vtProp;
    std::vector<ProcInfo> potentialRelatives;

    for (int i = 0; i < lObjectCount; i++)
    {
        std::wstring name;
        std::wstring command;
        DWORD pid = PID_DEFAULT;
        DWORD parentPid = PID_DEFAULT;

        hres = apObjArray[i]->Get(_bstr_t(L"TargetInstance"), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hres))
        {
            IUnknown* str = vtProp;
            hres = str->QueryInterface(IID_IWbemClassObject, reinterpret_cast<void**>(&apObjArray[i]));
            if (SUCCEEDED(hres))
            {
                _variant_t cn;
                hres = apObjArray[i]->Get(L"Name", 0, &cn, NULL, NULL);
                if (SUCCEEDED(hres))
                {
                    if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
                        name = (cn.vt == VT_NULL) ? L"NULL" : L"EMPTY";
                    else
                        name = cn.bstrVal;
                }
                VariantClear(&cn);

                hres = apObjArray[i]->Get(L"CommandLine", 0, &cn, NULL, NULL);
                if (SUCCEEDED(hres))
                {
                    if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
                        command = (cn.vt == VT_NULL) ? L"NULL" : L"EMPTY";
                    else
                        command = cn.bstrVal;
                }
                VariantClear(&cn);

                hres = apObjArray[i]->Get(L"ProcessId", 0, &cn, NULL, NULL);
                if (SUCCEEDED(hres))
                {
                    if ((cn.vt != VT_NULL) && (cn.vt != VT_EMPTY))
                        pid = cn.uintVal;
                }
                VariantClear(&cn);

                hres = apObjArray[i]->Get(L"ParentProcessId", 0, &cn, NULL, NULL);
                if (SUCCEEDED(hres))
                {
                    if ((cn.vt != VT_NULL) && (cn.vt != VT_EMPTY))
                        parentPid = cn.uintVal;
                }
                VariantClear(&cn);
            }
            str->Release();
        }
        VariantClear(&vtProp);
        
        if (pid != PID_DEFAULT && !name.empty())
        {
            ProcInfo proc = ProcInfo(pid, name, command , 0, ParentProcInfo(parentPid));
            if (Settings::CONTROL_ALL_MODE)
            {
                if (IsNotExclusion(proc))
                {
                    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, proc.ProcessId);
                    bool suspended = Global::NtFuncs.SuspendProcess(handle);
                    if (suspended)
                    {
                        proc.Handle = handle;
                        Global::processQueue.PushProc(proc);
                    }
                    else
                    {
                        CloseHandle(handle);
                        Terminal::PrintSuspendError(proc);
                    }
                }
                else
                {
                    Terminal::PrintProcIgnored(proc, IgnoredReasons::IS_EXCLUSION);
                }
            }
            else
            {
                potentialRelatives.push_back(proc);
            }
        }
    }

    if (!Settings::CONTROL_ALL_MODE)
        SearchRelatives(potentialRelatives);

    return WBEM_S_NO_ERROR;
}

HRESULT EventSink::SetStatus(
	LONG lFlags,
	HRESULT hResult,
	BSTR strParam,
	IWbemClassObject __RPC_FAR* pObjParam
)
{
	return WBEM_S_NO_ERROR;
}