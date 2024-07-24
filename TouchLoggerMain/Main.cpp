#pragma comment(lib, "Shlwapi.lib")
#include "TouchLoggerDefs.h"
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <shlwapi.h>
#include <thread>

BOOL TLInjectDll(DWORD processID, char* dllPath)
{
    // Open target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL)
    {
        printf("Unable to open target process. Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        return 0;
    }
    // Allocate memory for dll path in target process
    LPVOID remoteBuf = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (remoteBuf == NULL)
    {
        printf("Could not allocate memory in target process. Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        CloseHandle(hProcess);
        return 0;
    }
    // Write DLL path to allocated memory
    if (!WriteProcessMemory(hProcess, remoteBuf, dllPath, strlen(dllPath) + 1, NULL)) {
        printf("Could not write to process memory. Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 0;
    }
    // Load kernel32.dll in current process
    HMODULE hKernel32 = GetModuleHandle(_T("Kernel32"));
    if (hKernel32 == NULL) {
        printf("Failed to load kernel32.dll. Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 0;
    }
    // Get address of LoadLibrary from kernel32.dll
    //#ifdef _UNICODE
    //FARPROC loadLibAddr = GetProcAddress(hKernel32, "LoadLibraryW");
    //#endif
    //#ifndef _UNICODE
    FARPROC loadLibAddr = GetProcAddress(hKernel32, "LoadLibraryA");
    //#endif
    if (loadLibAddr == NULL) {
        printf("Could not find LoadLibrary function. Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 0;
    }
    // Create a remote thread in the target process to execute LoadLibrary
    DWORD injectedThreadID = 0;
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, remoteBuf, 0, &injectedThreadID);
    if (hThread == NULL) {
        printf("Could not create remote thread. Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 0;
    }
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    printf("DLL injected successfully.\n");
    return 1;
}

HANDLE TLCreateOutPipe(void)
{
    HANDLE hPipe = CreateNamedPipe(
        TL_OUT_PIPE_NAME,
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        TL_OUT_MSG_SIZE * TL_OUT_BUFFER_SIZE,
        0,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL
    );
    if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE)
    {
        unsigned long error = static_cast<unsigned long>(GetLastError());
        printf("Failed to create pipe; Error code = %lu\n", error);
        return NULL;
    }
    printf("Created out pipe\n");
    return hPipe;
}

HANDLE TLCreateInPipe(void)
{
    HANDLE hPipe = CreateNamedPipe(
        TL_IN_PIPE_NAME,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        0,
        TL_IN_MSG_SIZE * TL_IN_BUFFER_SIZE,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL
    );
    if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE)
    {
        unsigned long error = static_cast<unsigned long>(GetLastError());
        printf("Failed to create pipe; Error code = %lu\n", error);
        return NULL;
    }
    printf("Created in pipe\n");
    return hPipe;
}

void TLConnectPipe(HANDLE hPipe)
{
    printf("Waiting for cilent to connect\n");
    if (!ConnectNamedPipe(hPipe, NULL))
    {
        printf("Failed to connect server; Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        return;
    }
    printf("Connected server\n");
}

void MyThreadFunc(HANDLE hOutPipe, HANDLE hInPipe, DWORD threadID)
{
    TLConnectPipe(hOutPipe);
    TLConnectPipe(hInPipe);
    while (!(WriteFile(hOutPipe, &threadID, TL_OUT_MSG_SIZE, NULL, NULL)));
    printf("Sent thread ID to Dll\n");
}

// User provides process ID and thread ID in hex (no prefix) as command-line arguments 
int main(int argc, char* argv[])
{
    DWORD processID = static_cast<DWORD>(std::strtoul(argv[1], nullptr, 16));
    DWORD threadID = static_cast<DWORD>(std::strtoul(argv[2], nullptr, 16));
    char dllPath[MAX_PATH];
    GetModuleFileNameA(NULL, dllPath, MAX_PATH);
    PathRemoveFileSpecA(dllPath);
    PathAppendA(dllPath, TL_DLL_NAME);
    printf("%s\n", dllPath);
    HANDLE hOutPipe = TLCreateOutPipe();
    HANDLE hInPipe = TLCreateInPipe();
    std::thread myThreadObj(MyThreadFunc, hOutPipe, hInPipe, threadID);
    TLInjectDll(processID, dllPath);
    myThreadObj.join();
    TLTouchMessage touchMessage = { 0 };
    while (1)
    {
        while (!(ReadFile(hInPipe, &touchMessage, TL_IN_MSG_SIZE, NULL, NULL)));
        printf("Received WM_TOUCH message with %u contacts\n", static_cast<unsigned int>(touchMessage.cInputs));
    }
    return 0;
}