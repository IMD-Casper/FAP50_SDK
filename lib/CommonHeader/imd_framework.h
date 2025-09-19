#pragma once
/////////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32 //windows
//include "stdafx.h"
#include <SDKDDKVer.h>
#include "afx.h"

#define WIN32_LEAN_AND_MEAN             // 從 Windows 標頭排除不常使用的成員
#include "windows.h"

/////////////////////////////////////////////////////////////////////////////////
#elif defined(__ANDROID__) //android
#include <stdint.h>
#include <inttypes.h>
using BOOL = int;

using BYTE = uint8_t;
using LPBYTE = BYTE*;
using PBYTE = BYTE*;
using UCHAR = uint8_t;
using PUCHAR = UCHAR*;

using WORD = uint16_t;
using PWORD = WORD*;
using UINT16 = uint16_t;
using USHORT = uint16_t;

using INT = int;
using UINT = unsigned int;
using DWORD = uint32_t;
using PDWORD = DWORD*;
using LPDWORD = uint32_t*;

using LONG = long;
using ULONG = unsigned long;
using PULONG = ULONG*;
using FLOAT = float;
using DOUBLE = double;

using UINT64 = uint64_t;
using HANDLE = void*;
using PVOID = void*;
using LONG_PTR = uint64_t;

#define OUT
//#define interface struct
#define WINAPI 
#define ERROR_SUCCESS 0
#define TRUE true
#define FALSE false
//#define NULL 0

#include <cstring> //for memcpy()

/////////////////////////////////////////////////////////////////////////////////
//winerror.h
#define INVALID_HANDLE_VALUE 			((HANDLE)(LONG_PTR)-1)
#define ERROR_INVALID_HANDLE             6L
#define ERROR_INVALID_DATA               13L
#define ERROR_GEN_FAILURE                31L
#define ERROR_DEVICE_NOT_AVAILABLE       4319L
#define ERROR_TIMEOUT                    1460L
#define ERROR_ACCESS_DENIED              5L
#define ERROR_BAD_PIPE                   230L
#define ERROR_INVALID_PARAMETER          87L    // dderror
#define WAIT_TIMEOUT                     258L    // dderror
#define ERROR_INVALID_FUNCTION           1L    // dderror

//#include "lib_fap50_android.h"
#include <android/log.h>
#include <jni.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "lib_fap50_android", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "lib_fap50_android", __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "lib_fap50_android", __VA_ARGS__))

#else //linux
#include <stdint.h>
#include <inttypes.h>
using BOOL = int;

using BYTE = uint8_t;
using LPBYTE = BYTE*;
using PBYTE = BYTE*;
using UCHAR = uint8_t;
using PUCHAR = UCHAR*;

using WORD = uint16_t;
using PWORD = WORD*;
using UINT16 = uint16_t;
using USHORT = uint16_t;

using INT = int;
using UINT = unsigned int;
using DWORD = uint32_t;
using PDWORD = DWORD*;
using LPDWORD = uint32_t*;

using LONG = long;
using ULONG = unsigned long;
using PULONG = ULONG*;
using FLOAT = float;
using DOUBLE = double;

using UINT64 = uint64_t;
using HANDLE = void*;
using PVOID = void*;
using LONG_PTR = uint64_t;

#define OUT
//#define interface struct
#define WINAPI 
#define ERROR_SUCCESS 0
#define TRUE true
#define FALSE false
#define NULL 0

#include <cstring> //for memcpy()

/////////////////////////////////////////////////////////////////////////////////
//winerror.h
#define INVALID_HANDLE_VALUE 			((HANDLE)(LONG_PTR)-1)
#define ERROR_INVALID_HANDLE             6L
#define ERROR_INVALID_DATA               13L
#define ERROR_GEN_FAILURE                31L
#define ERROR_DEVICE_NOT_AVAILABLE       4319L
#define ERROR_TIMEOUT                    1460L
#define ERROR_ACCESS_DENIED              5L
#define ERROR_BAD_PIPE                   230L
#define ERROR_INVALID_PARAMETER          87L    // dderror
#define WAIT_TIMEOUT                     258L    // dderror
#define ERROR_INVALID_FUNCTION           1L    // dderror

#endif

#include <future>
#include <thread>
#include <mutex>
#include <chrono>

#include <vector>
#include <deque>
#include <map>

#include <iostream>
#include <numeric>
#include <fstream>
#include <string>
#include <cstdarg>

#ifdef USE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
using namespace cv;
#endif

using namespace std;

//void dbg(const char* fmt, ...);
static mutex dbg_mutex;
static void dbg(const char* fmt, ...)
{
	lock_guard<mutex> lock(dbg_mutex);
	char buffer[1024] = "DBG: "; // buffer

	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer + 5, sizeof(buffer) - 10, fmt, args);
	va_end(args);

#ifdef _WIN32
	// output debug info on Windows.
//#include <debugapi.h>
	OutputDebugStringA(buffer);

#elif __ANDROID__
	LOGD(buffer);

#else
	// 在其他平台上输出到控制台
	cerr << buffer;// << std::endl;
#endif
}

#define NOP() 								do{}while(0)
#define once_for							for (bool _break = true; _break; _break = false)
#define thread_member(member)				{thread t(&member, this); t.detach();}
#define thread_class_member(obj, member)	{thread t(&member, obj); t.detach();}
#define thread_func(func)					{thread t(func); t.detach();}

#define sleep_ms(ms)	this_thread::sleep_for(chrono::milliseconds(ms))
//#define sleep_ms(ms)	Sleep(ms)
#define Sleep(ms)	sleep_ms(ms)

#ifdef ZeroMemory
#undef ZeroMemory
#endif
#define ZeroMemory(Destination, Length)		memset((Destination), 0, (Length))

//#include "timeapi.h"
#define timeGetTime()						(UINT64)chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count()
