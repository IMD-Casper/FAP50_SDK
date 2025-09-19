#pragma once
/*
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

int  pen_open(intptr_t android_fd = 0, void** libusb_ctx = nullptr, void** dev_handle = nullptr);
int  pen_close();
int  enable_pen(int en);
int  get_pen_data(int& x, int& y, int& tilt_x, int& tilt_y, int& press, unsigned long& timestamp);
#ifdef __cplusplus
}
#endif
*/
#ifdef __ANDROID__
static int pen_open() { return 0; }
static int pen_close() { return 0; }
static int get_pen_data(int& x, int& y, int& tilt_x, int& tilt_y, int& press, DWORD& timestamp) { return 0;}
static int enable_pen(int en) { return 0; }

#else
int pen_open();
int pen_close();
int get_pen_data(int& x, int& y, int& tilt_x, int& tilt_y,
    int& press, unsigned int& timestamp);
int enable_pen(int en);

#define USE_CHINA_ZD03
#endif
