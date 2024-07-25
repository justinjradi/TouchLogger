// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "TouchLoggerDefs.h"
#include <string>


HHOOK hHook = NULL;
HINSTANCE hInstance = NULL;
HANDLE hPipe = INVALID_HANDLE_VALUE;
DWORD threadID = 0;

void output_dword(DWORD x)
{
    if (sizeof(unsigned int) != sizeof(DWORD))
    {
        return;
    }
    char str[16];
    sprintf_s(str, "%u", (unsigned int)x);
    wchar_t wstr[16];
    size_t length = NULL;
    mbstowcs_s(&length, wstr, 16, str, _TRUNCATE);
    OutputDebugString((LPCWSTR)wstr);
}

int TLConnectClient(void)
{
    hPipe = CreateFile
    (
        TL_PIPE_NAME,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE)
    {
        OutputDebugString(L"Failed to connect pipe to client, error code: ");
        output_dword(GetLastError());
        OutputDebugString(L"\n");
        return 0;
    }
    OutputDebugString(L"Connected pipe to client\n");
    return 1;
}

LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        CWPSTRUCT* pCwp = (CWPSTRUCT*)lParam;
        if (pCwp->message == WM_TOUCH)
        {
            OutputDebugString(L"Hook activated\n");
            unsigned int numInputs = static_cast<unsigned int>(pCwp->lParam);
            TOUCHINPUT* ti = new TOUCHINPUT[numInputs];
            if (!GetTouchInputInfo((HTOUCHINPUT)pCwp->lParam, numInputs, ti, sizeof(TOUCHINPUT)))
            {
                OutputDebugString(L"Unable to get touch input info\n");
                return CallNextHookEx(hHook, nCode, wParam, lParam);
            }
            // write to file
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
        {
            OutputDebugString(L"Attaching DLL\n");
            if (!TLConnectClient())
            {
                break;
            }
            DWORD data[2] = { 0 };
            OutputDebugString(L"Trying to read setup data...\n");
            if (!ReadFile(hPipe, data, TL_MSG_SIZE, NULL, NULL))
            {
                OutputDebugString(L"Setup read failed, Error Code = ");
                output_dword(GetLastError());
                OutputDebugString(L"\n");
                break;
            }
            OutputDebugString(L"Setup read successful\n");
            threadID = data[1];
            if (threadID == 0)
            {
                OutputDebugString(L"Thread ID of 0 is invalid\n");
                break;
            }
            hInstance = hinstDLL;
            if (CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL) == NULL)
            {
                OutputDebugString(L"Failed to create ThreadProc\n");
                break;
            }
            OutputDebugString(L"Created ThreadProc\n");
            break;
        }
        case DLL_PROCESS_DETACH:
        {
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
    }
    return TRUE;
}

