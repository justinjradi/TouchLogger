#pragma once
#include <Windows.h>
#include <tchar.h>

typedef struct
{
    UINT cInputs;
    PTOUCHINPUT pInputs;
} TLTouchMessage;

#define TL_DLL_NAME				"TouchLoggerDll.dll"			// Leave as ANSI!
#define TL_OUT_PIPE_NAME			_T("\\\\.\\pipe\\TLOutPipe")
#define TL_IN_PIPE_NAME			_T("\\\\.\\pipe\\TLInPipe")

#define TL_OUT_MSG_SIZE			sizeof(DWORD)
#define TL_OUT_BUFFER_SIZE		16
#define TL_IN_MSG_SIZE			sizeof(TLTouchMessage)
#define TL_IN_BUFFER_SIZE		16

//C:\Users\Justin\Documents\GitHub\TouchLogger\x64\Debug