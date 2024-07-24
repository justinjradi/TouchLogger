// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "TouchLoggerDefs.h"

HHOOK hHook = NULL;
HINSTANCE hInstance = NULL;
HANDLE hPipe = INVALID_HANDLE_VALUE;

HANDLE TLConnectOutPipe(void)
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    hPipe = CreateFile
    (
        TL_OUT_PIPE_NAME,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE)
    {
        OutputDebugString(L"Failed to connect out pipe to server\n");
        return INVALID_HANDLE_VALUE;
    }
    OutputDebugString(L"Connected out pipe to server\n");
    return hPipe;
}

HANDLE TLConnectInPipe(void)
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    hPipe = CreateFile
    (
        TL_IN_PIPE_NAME,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE)
    {
        OutputDebugString(L"Failed to connect out pipe to server\n");
        return INVALID_HANDLE_VALUE;
    }
    OutputDebugString(L"Connected out pipe to server\n");
    return hPipe;
}

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
        hPipe = TLConnectOutPipe();
        OutputDebugString(L"Connected to out pipe\n");
        DWORD threadID = 0;
        while ((!ReadFile(hPipe, &threadID, TL_OUT_MSG_SIZE, NULL, NULL)));
        OutputDebugString(L"Received thread ID\n");
        if (threadID == 0)
        {
            OutputDebugString(L"Thread ID of 0 is invalid\n");
            break;
        }
        hInstance = hinstDLL;
        hHook = SetWindowsHookEx(WH_CALLWNDPROC, HookProc, hInstance, threadID);
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

