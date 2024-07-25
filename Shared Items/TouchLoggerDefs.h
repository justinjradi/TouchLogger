#pragma once
#include <Windows.h>
#include <tchar.h>
#include <stdarg.h>

#define TL_DLL_NAME			"TouchLoggerDll.dll"
#define TL_FILE_NAME		"output.txt"
#define TL_PIPE_NAME		_T("\\\\.\\pipe\\TLOutPipe")

#define TL_MSG_SIZE			2 * sizeof(DWORD)	// Low DWORD = state, High DWORD = thread ID
#define TL_BUFFER_SIZE		16

#define TL_STATE_RUN        0
#define TL_STATE_EXIT       1

//C:\Users\Justin\Documents\GitHub\TouchLogger\x64\Debug