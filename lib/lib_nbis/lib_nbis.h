#pragma once

#ifdef BUILD_IMD_DLL
	#ifdef _WIN32
		#define IMD_STD_API  	extern "C" __declspec(dllexport) 
		#define EXPORT_IMD_DLL 	__declspec(dllexport)
	#else
		#define IMD_STD_API  	extern "C" __attribute__((visibility("default")))
		#define EXPORT_IMD_DLL 	__attribute__((visibility("default"))) 
	#endif
#else
	#ifdef _WIN32
		#define IMD_STD_API  	extern "C" __declspec(dllimport)
		#define EXPORT_IMD_DLL 	__declspec(dllimport)
	#else
		#define IMD_STD_API  	 extern "C" __attribute__((visibility("default"))) 
		#define EXPORT_IMD_DLL 	__attribute__((visibility("default"))) 
	#endif
#endif

IMD_STD_API int get_nfiq_fap20_fap30(void* pSrc, int* score, int bitrate, int ppi, int width, int height);
