#include "EventSink.h"
#include "Global.h"
#include "SpawnControl.h"
#include "Terminal.h"
#include <signal.h>

namespace SpawnControl
{
    volatile sig_atomic_t ctrlCcount = 0;
    volatile std::time_t lastTime = 0;
    volatile bool isRunning = true;

    //prevents accidental closing when pressing "CTRL + C"
    //to exit the program, press "CTRL + C" 3 times with an interval of less than 2 seconds
    BOOL WINAPI CatchSignal(DWORD sig)
    {
        if (sig == CTRL_C_EVENT)
        {
            if (lastTime == 0)
            {
                lastTime = std::time(NULL);
                ctrlCcount++;
            }
            else
            {
                std::time_t tmp = std::time(NULL);
                if (tmp - lastTime >= 2)
                    ctrlCcount = 0;
                else
                    ctrlCcount++;
                lastTime = tmp;
            }

            if (ctrlCcount >= 3)
                isRunning = false;
        }

        return TRUE;
    }

    bool Start(ProcInfo target)
    {
        HRESULT hres;
        hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hres))
        {
            std::wcout << "Can not initialize COM library" << std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            return false;
        }

        hres = CoInitializeSecurity(
            NULL,
            -1,
            NULL,
            NULL,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE,
            NULL);

        if (FAILED(hres))
        {
            std::wcout << "Can not initialize security" << std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            CoUninitialize();
            return false;
        }

        IWbemLocator* pLoc = NULL;
        hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
        if (FAILED(hres))
        {
            std::wcout << "Can not create IWbemLocator object" << std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            CoUninitialize();
            return false;
        }

        IWbemServices* pSvc = NULL;
        hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
        if (FAILED(hres))
        {
            std::wcout << "Can not connect to ROOT\\CIMV2 WMI namespace" << std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            pLoc->Release();
            CoUninitialize();
            return false;
        }

        hres = CoSetProxyBlanket(
            pSvc,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE);

        if (FAILED(hres))
        {
            std::wcout << "Can not set proxy blanket" << std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return false;
        }

        IUnsecuredApartment* pUnsecApp = NULL;
        hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL, CLSCTX_LOCAL_SERVER,
            IID_IUnsecuredApartment, (void**)&pUnsecApp);

        EventSink* pSink = new EventSink;
        pSink->AddRef();

        IUnknown* pStubUnk = NULL;
        pUnsecApp->CreateObjectStub(pSink, &pStubUnk);

        IWbemObjectSink* pStubSink = NULL;
        pStubUnk->QueryInterface(IID_IWbemObjectSink,
            (void**)&pStubSink);

        Global::relatives.push_back(target);
        SetConsoleCtrlHandler(CatchSignal, TRUE);
        Terminal::PrintProcSuspended(target);

        hres = pSvc->ExecNotificationQueryAsync(
            _bstr_t("WQL"),
            _bstr_t("SELECT * FROM __InstanceCreationEvent WITHIN 1 WHERE TargetInstance ISA 'Win32_Process'"),
            WBEM_FLAG_SEND_STATUS,
            NULL,
            pStubSink);

        if (FAILED(hres))
        {
            std::wcout << "ExecNotificationQueryAsync failed" << std::endl;
            std::wcout << "Press ENTER to exit..." << std::endl;
            std::wcin.ignore();
            pSvc->Release();
            pLoc->Release();
            pUnsecApp->Release();
            pStubUnk->Release();
            pSink->Release();
            pStubSink->Release();
            CoUninitialize();
            return false;
        }

        Global::NtFuncs.ResumeProcess(target.Handle);
        CloseHandle(target.Handle);

        while (isRunning)
        {
            if (!Global::processQueue.Empty())
            {
                ProcInfo proc = Global::processQueue.PopProc();
                Terminal::PrintProcSuspended(proc, Global::processQueue.Size());
                bool resumed = Global::NtFuncs.ResumeProcess(proc.Handle);
                CloseHandle(proc.Handle);

                if (!resumed)
                    Terminal::PrintResumeError(proc);
                else
                    Terminal::PrintProcResumed(proc);

                if (Global::processQueue.Empty())
                    std::wcout << "Waiting for events..." << std::endl;
            }

            Sleep(200);
        }

        hres = pSvc->CancelAsyncCall(pStubSink);
        pSvc->Release();
        pLoc->Release();
        pUnsecApp->Release();
        pStubUnk->Release();
        pSink->Release();
        pStubSink->Release();
        CoUninitialize();

        //clean queue
        while (!Global::processQueue.Empty())
        {
            ProcInfo proc = Global::processQueue.PopProc();
            bool resumed = Global::NtFuncs.ResumeProcess(proc.Handle);
            CloseHandle(proc.Handle);

            if (!resumed)
                Terminal::PrintResumeError(proc);
        }

        return true;
    }
}
