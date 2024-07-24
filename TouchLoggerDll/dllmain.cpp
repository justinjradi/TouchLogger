// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "TouchLoggerDefs.h"

HHOOK hHook = NULL;
HINSTANCE hInstance = NULL;
HANDLE hPipe = INVALID_HANDLE_VALUE;
DWORD threadID = 0;

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
            OutputDebugString(L"Hook activated\n");
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
    OutputDebugString(L"Entered ThreadProc\n");
    hHook = SetWindowsHookEx(WH_CALLWNDPROC, HookProc, hInstance, threadID);
    if (hHook == NULL)
    {
        OutputDebugString(L"Failed to set hook");
        return 0;
    }
    OutputDebugString(L"Set hook successfully\n");
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        OutputDebugString(L"Attaching DLL\n");
        hPipe = TLConnectOutPipe();
        if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE) break;
        OutputDebugString(L"Connected to out pipe\n");
        while ((!ReadFile(hPipe, &threadID, TL_OUT_MSG_SIZE, NULL, NULL)));
        OutputDebugString(L"Received thread ID\n");
        if (threadID == 0)
        {
            OutputDebugString(L"Thread ID of 0 is invalid\n");
            break;
        }
        hInstance = hinstDLL;
        //hHook = SetWindowsHookEx(WH_CALLWNDPROC, HookProc, hInstance, threadID);
        //if (hHook == NULL)
        //{
        //    OutputDebugString(L"Failed to set hook");
        //    break;
        //}
        //OutputDebugString(L"Set hook successfully\n");
        if (CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL) == NULL)
        {
            OutputDebugString(L"Failed to create ThreadProc\n");
            break;
        }
        OutputDebugString(L"Created ThreadProc\n");
        break;

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


        if (lpReserved == NULL) OutputDebugString(L"Welp\n");
        break;
    }
    return TRUE;
}

