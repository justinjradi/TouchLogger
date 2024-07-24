#pragma once
#include <tchar.h>

#define TL_DLL_NAME				"TouchLoggerDll.dll"			// Leave as ANSI
#define TL_IN_PIPE_NAME			_T("\\\\.\\pipe\\TLInPipe")

#define TL_IN_MSG_SIZE				4
#define TL_IN_BUFFER_SIZE			64