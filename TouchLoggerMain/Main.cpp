#pragma comment(lib, "Shlwapi.lib")
#include "Windows.h"
#include "TouchLoggerDefs.h"
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <shlwapi.h>

BOOL TLInjectDll(DWORD processID, TCHAR* dllPath)
{
    // Open target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL)
    {
        printf("Unable to open target process. Error code: %d\n", static_cast<unsigned long>(GetLastError()));
        return 0;
    }
    // Allocate memory for dll path
    size_t pathLength = _tcslen(dllPath);
    LPVOID remoteBuf = VirtualAllocEx(hProcess, NULL, pathLength, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (remoteBuf == NULL)
    {
        printf("Could not allocate memory in target process. Error code: %d\n", static_cast<unsigned long>(GetLastError()));
        CloseHandle(hProcess);
        return 0;
    }
    // Load kernel32.dll in current process
    HMODULE hKernel32 = GetModuleHandle(_T("Kernel32"));
    if (hKernel32 == NULL) {
        printf("Failed to load kernel32.dll. Error code: %d\n", static_cast<unsigned long>(GetLastError()));
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 0;
    }
    // Get address of LoadLibrary from kernel32.dll
    #ifdef _UNICODE
    FARPROC loadLibAddr = GetProcAddress(hKernel32, "LoadLibraryW");
    #endif
    #ifndef _UNICODE
    FARPROC loadLibAddr = GetProcAddress(hKernel32, "LoadLibraryA");
    #endif
    if (loadLibAddr == NULL) {
        printf("Could not find LoadLibrary function. Error code: %d\n", static_cast<unsigned long>(GetLastError()));
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 0;
    }
    // Create a remote thread in the target process to execute LoadLibrary
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, remoteBuf, 0, NULL);
    if (hThread == NULL) {
        printf("Could not create remote thread. Error code: %d\n", static_cast<unsigned long>(GetLastError()));
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 0;
    }
    // Wait for the remote thread to finish
    WaitForSingleObject(hThread, INFINITE);
    // Clean up resources
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    printf("DLL injected successfully.\n");
    return 1;
}

HANDLE TLCreatePipe(void)
{
    HANDLE hPipe = CreateNamedPipe(
        TL_IN_PIPE_NAME,
        PIPE_ACCESS_INBOUND/*PIPE_ACCESS_DUPLEX*/,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
        1,
        TL_IN_MSG_SIZE * TL_IN_BUFFER_SIZE,
        TL_IN_MSG_SIZE * TL_IN_BUFFER_SIZE,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL
    );
    if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE)
    {
        printf("Failed to create pipe; Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        return NULL;
    }
    printf("Created named pipe\n");
    return hPipe;
}

void TLConnectServer(HANDLE hPipe)
{
    printf("Waiting for cilent to connect\n");
    if (!ConnectNamedPipe(hPipe, NULL))
    {
        printf("Failed to connect server; Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        return;
    }
    printf("Connected server\n");
}

unsigned char TLGetData(HANDLE hPipe)
{
    unsigned char data[1] = { 0 };
    DWORD numberOfBytesRead = 0;
    printf("Waiting for data to arrive\n");
    if (!ReadFile(hPipe, data, TL_IN_MSG_SIZE, &numberOfBytesRead, NULL))
    {
        printf("Failed to read message; Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        return 0;
    }
    printf("Received data\n");
    return data[0];
}

// User provides process ID and thread ID in hex (no prefix) as command-line arguments 
int main(int argc, char* argv[])
{
    DWORD processID = static_cast<DWORD>(std::strtoul(argv[1], nullptr, 16));
    DWORD threadID = static_cast<DWORD>(std::strtoul(argv[2], nullptr, 16));
    TCHAR dllPath[MAX_PATH];
    GetModuleFileName(NULL, dllPath, MAX_PATH);
    PathRemoveFileSpec(dllPath);
    PathAppend(dllPath, TL_DLL_NAME);
    TLInjectDll(processID, dllPath);
}