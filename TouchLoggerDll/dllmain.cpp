#include "pch.h"
#include "TouchLoggerDefs.h"
#include <string>
#include <fstream>
#include <stdio.h>
#include <vector>

typedef struct
{
    UINT cInputs;
    PTOUCHINPUT pInputs;
    DWORD timestamp;
} TLLogEntry;

HHOOK hHook = NULL;
HINSTANCE hInstance = NULL;
HANDLE hPipe = INVALID_HANDLE_VALUE;
DWORD threadID = 0;
std::vector<TLLogEntry> log;

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

void TLEventType(const char* str, DWORD dwFlags)
{
    switch (dwFlags)
    {
        case 0x0001:
            str = "MOVE";
            break;
        case 0x0002:
            str = "DOWN";
            break;
        case 0x0004:
            str = "UP";
            break;
        case 0x0008:
            str = "INRANGE";
            break;
        case 0x0010:
            str = "PRIMARY";
            break;
        case 0x0020:
            str = "NOCOALESCE";
            break;
        case 0x0040:
            str = "PEN";
            break;
        case 0x0080:
            str = "PALM";
            break;
        default:
            str = "N/A";
            break;
    }
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
            UINT cInputs = (UINT)pCwp->lParam;
            TOUCHINPUT* pInputs = new TOUCHINPUT[cInputs];
            if (!GetTouchInputInfo((HTOUCHINPUT)pCwp->lParam, cInputs, pInputs, sizeof(TOUCHINPUT)))
            {
                OutputDebugString(L"Unable to get touch input info\n");
                return CallNextHookEx(hHook, nCode, wParam, lParam);
            }
            TLLogEntry tempEntry = { cInputs, pInputs, GetTickCount() };
            log.push_back(tempEntry);
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
    while (1)
    {
       DWORD data[2] = { 0 };
       BOOL readStatus = 0;
       readStatus = ReadFile(hPipe, data, TL_MSG_SIZE, NULL, NULL);
       if (readStatus && (data[0] == TL_STATE_EXIT))
       {
           CloseHandle(hPipe);
           OutputDebugString(L"Preparing log file...\n");
           FILE* file = fopen(TL_FILE_NAME, "w");
           if (file == NULL) {
               OutputDebugString(L"Error opening file\n");
               return 0;
           }
           for (int i = 0; i < log.size(); i++)
           {
               unsigned int cInputs = (unsigned int)log[i].cInputs;
               TOUCHINPUT* pInputs = (TOUCHINPUT*)log[i].pInputs;
               unsigned long mTimestamp = (unsigned long)log[i].timestamp;
               fprintf(file, "%lu ms: WM_TOUCH message with %u contacts\n", mTimestamp, cInputs);
               for (unsigned int j = 0; j < cInputs; j++)
               {
                   unsigned long contactID = (unsigned long)pInputs[j].dwID;
                   long x = (long)pInputs[j].x;
                   long y = (long)pInputs[j].y;
                   char eventType[32] = "NULL";
                   TLEventType(eventType, pInputs[j].dwFlags);
                   unsigned long scanTime = (unsigned long)pInputs[j].dwTime;
                   fprintf(file, "  Contact ID = %lu, x = %ld, y = %ld, Event Type = %s, Scan Time = %lu\n", contactID, x, y, eventType, scanTime);
               }
           }
           fclose(file);
           OutputDebugString(L"Succesfully wrote to file\n");
       }
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
            //hFile = fopen(TL_FILE_NAME, "w");
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

