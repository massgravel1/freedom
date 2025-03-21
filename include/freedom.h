#pragma once

#include <windows.h>

#include <thread>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#ifndef GIT_COMMIT_HASH  // Correctly handle the case where GIT_COMMIT_HASH isn't defined
#define GIT_COMMIT_HASH Unknown  // Provide a default if GIT_COMMIT_HASH is not available
#endif

#define FR_VERSION "KKK [" STR(GIT_COMMIT_HASH) "]"

extern HWND g_hwnd;
extern HMODULE g_module;
extern HANDLE g_process;

void unload_dll();
