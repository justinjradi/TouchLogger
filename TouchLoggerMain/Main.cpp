#pragma comment(lib, "Shlwapi.lib")
#include "TouchLoggerDefs.h"
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <shlwapi.h>
#include <thread>

HANDLE hPipe = INVALID_HANDLE_VALUE;

BOOL TLInjectDll(DWORD processID)
{
    // Get Dll path
    char dllPath[MAX_PATH];
    GetModuleFileNameA(NULL, dllPath, MAX_PATH);
    PathRemoveFileSpecA(dllPath);
    PathAppendA(dllPath, TL_DLL_NAME);
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
    FARPROC loadLibAddr = GetProcAddress(hKernel32, "LoadLibraryA");
    if (loadLibAddr == NULL) {
        printf("Could not find LoadLibrary function. Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 0;
    }
    // Create a remote thread in the target process to execute LoadLibrary
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, remoteBuf, 0, NULL);
    if (hThread == NULL) {
        printf("Could not create remote thread. Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 0;
    }
    // Cleanup
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return 1;
}

BOOL TLEjectDll(DWORD processID)
{
    // Get Dll path
    char dllPath[MAX_PATH];
    GetModuleFileNameA(NULL, dllPath, MAX_PATH);
    PathRemoveFileSpecA(dllPath);
    PathAppendA(dllPath, TL_DLL_NAME);
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
    // Get address of FreeLibrary from kernel32.dll
    FARPROC freeLibAddr = GetProcAddress(hKernel32, "FreeLibrary");
    if (freeLibAddr == NULL) {
        printf("Could not find FreeLibrary function. Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 0;
    }
    // Create a remote thread in the target process to execute FreeLibrary
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)freeLibAddr, remoteBuf, 0, NULL);
    if (hThread == NULL) {
        printf("Could not create remote thread. Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 0;
    }
    // Cleanup
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    printf("Unloaded dll\n");
    return 1;
}

void TLSetupPipe(DWORD threadID)
{
    // Create pipe
    hPipe = CreateNamedPipe(
        TL_PIPE_NAME,
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        TL_MSG_SIZE * TL_BUFFER_SIZE,
        0,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL
    );
    if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE)
    {
        unsigned long error = static_cast<unsigned long>(GetLastError());
        printf("Failed to create pipe; Error code = %lu\n", error);
        return;
    }
    printf("Created pipe\n");
    // Connect pipe
    printf("Waiting for cilent to connect...\n");
    if (!ConnectNamedPipe(hPipe, NULL))
    {
        printf("Failed to connect pipe; Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        return;
    }
    printf("Connected pipe\n");;
    // Send thread ID to dll
    DWORD data[2] = {TL_STATE_RUN, threadID};
    if (!WriteFile(hPipe, data, TL_MSG_SIZE, NULL, NULL))
    {
        printf("Failed to send thread ID; Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        return;
    }
    printf("Sent thread ID\n");
}

int main(int argc, char* argv[])
{
    DWORD processID = static_cast<DWORD>(std::strtoul(argv[1], nullptr, 16));
    DWORD threadID = static_cast<DWORD>(std::strtoul(argv[2], nullptr, 16));
    std::thread setupPipeThread(TLSetupPipe, threadID);
    if (!TLInjectDll(processID))
    {
        return 1;
    }
    setupPipeThread.join();
    printf("Successfully started logging. Press ENTER to stop\n");
    getchar();
    DWORD data[2] = { TL_STATE_EXIT, threadID };
    if (!WriteFile(hPipe, data, TL_MSG_SIZE, NULL, NULL))
    {
        printf("Failed to send exit command; Error code = %lu\n", static_cast<unsigned long>(GetLastError()));
        return 1;
    }
    /*CloseHandle(hPipe);*/
    printf("Program exit\n");
    return 0;
}