#pragma once
#include <Windows.h>
#include <tchar.h>
#include <stdarg.h>

typedef struct
{
    UINT cInputs;
    PTOUCHINPUT pInputs;
} TLTouchMessage;

#define TL_DLL_NAME			"TouchLoggerDll.dll"			// Leave as ANSI!
#define TL_PIPE_NAME		_T("\\\\.\\pipe\\TLOutPipe")

#define TL_MSG_SIZE			2 * sizeof(DWORD)
#define TL_BUFFER_SIZE		16

#define TL_STATE_RUN        0
#define TL_STATE_EXIT       1

#define TL_DEBUG
int TLDebug(const char* format, ...)
{
#ifdef TL_DEBUG
    va_list args;
    va_start(args, format);
    int r = vprintf(format, args);
    va_end(args);
    return r;
#endif
#ifndef TL_DEBUG
    return 0;
#endif
}

//C:\Users\Justin\Documents\GitHub\TouchLogger\x64\Debug