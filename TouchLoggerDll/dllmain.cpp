// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "Windows.h"
#include "TouchLoggerDefs.h"

HHOOK hHook = NULL;
HINSTANCE hInstance = NULL;
HANDLE hPipe = INVALID_HANDLE_VALUE;

LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        CWPSTRUCT* pCwp = (CWPSTRUCT*)lParam;
        if (pCwp->message == WM_TOUCH)
        {
            unsigned int numInputs = pCwp->lParam;
            TOUCHINPUT* ti = new TOUCHINPUT[numInputs];
            if (hPipe != INVALID_HANDLE_VALUE)
            {
                OutputDebugString(L"Detected touch message");
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        OutputDebugString(L"Attaching DLL\n");
        hInstance = hinstDLL;
        hHook = SetWindowsHookEx(WH_CALLWNDPROC, HookProc, hInstance, 0);
        if (hHook == NULL)
        {
            OutputDebugString(L"Failed to set hook");
            break;
        }
        OutputDebugString(L"Set hook successfully\n");

    case DLL_PROCESS_DETACH:
        OutputDebugString(L"Detaching DLL\n");
        if (hHook == NULL)
        {
            OutputDebugString(L"Unable to unhook NULL HHOOK\n");
            break;
        }
        BOOL unhook_result = UnhookWindowsHookEx(hHook);
        if (unhook_result == 0)
        {
            OutputDebugString(L"Tried to unhook but failed\n");
            break;
        }
        hHook = NULL;
        OutputDebugString(L"Unhooked successfully\n");
        break;
    }
    return TRUE;
}

