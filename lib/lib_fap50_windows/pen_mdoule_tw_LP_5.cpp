// view_sign_port.cpp - Windows-only C++ version using LoadLibrary/GetProcAddress
// This version runs on Windows and loads VIEW_SIGN.dll
#include "pen_mdoule_interface.h"

#ifdef USE_TW_LP_5
#include "imd_framework.h"
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <windows.h>
#include <iomanip>
#include <ctime>
#include <sstream>

using namespace std;
using namespace chrono;


// ------------------- Structures ----------------------
struct PACKETS {
    int btn;
    int X;
    int Y;
    int Press;
};

struct PACKETS_Z {
    int btn;
    int X;
    int Y;
    int Press;
    int Z;
};

struct PACKETS_TILT {
    int btn;
    int X;
    int Y;
    int Press;
    uint16_t TILT_X;
    uint16_t TILT_Y;
};

struct AXIS {
    int32_t axMin;
    int32_t axMax;
};

struct VID_PID {
    uint32_t VID;
    uint32_t PID;
};

struct WT_PACKET_BOUND {
    int start_x;
    int start_y;
    int width;
    int height;
};

// ------------------- Constants ----------------------

const int HW_GET_AXIS_X = 0x02;
const int HW_GET_AXIS_Y = 0x03;
const int HW_GET_PRESSURE = 0x04;
const int HW_GET_VENDOR = 0x05;
const int HW_GET_PRODUCT = 0x06;
const int HW_GET_VID_PID = 0x08;
const int HW_GET_DEVICE_UID = 0x09;


// ------------------- Function Pointers ----------------------
using taggetAXIS = int(__stdcall*)(uint32_t, AXIS*);
using taggetPressure = int(__stdcall*)(uint32_t, int*);
using taggetPackets = int(__stdcall*)(PACKETS*);
using taggetVendor = int(__stdcall*)(uint32_t, char*);
using taggetVidPid = int(__stdcall*)(uint32_t, VID_PID*);
using taggetDeviceUid = int(__stdcall*)(uint32_t, uint8_t*);
using tagopenDevice = int(__stdcall*)();
using tagcloseDevice = int(__stdcall*)();
using taggetPacketsWithTilt = int(__stdcall*)(PACKETS_TILT*);
using tagEnableEmrScan = int(__stdcall*)(int);

// ------------------- Globals ----------------------
HMODULE g_hModule = nullptr;
taggetAXIS getAXIS = nullptr;
taggetPressure getPressure = nullptr;
taggetPackets getPackets = nullptr;
taggetVendor getVendor = nullptr;
taggetVidPid getVidPid = nullptr;
taggetDeviceUid getDeviceUid = nullptr;
tagopenDevice openDevice = nullptr;
tagcloseDevice closeDevice = nullptr;
tagEnableEmrScan enableEmrScan = nullptr;
taggetPacketsWithTilt getPacketsWithTilt = nullptr;
static bool g_is_pen_opened = false;

const int DEFAULT_AXIS_MIN_X = 0;
const int DEFAULT_AXIS_MAX_X = 10262;
const int DEFAULT_AXIS_MIN_Y = 0;
const int DEFAULT_AXIS_MAX_Y = 7422;

#define SCREEN_WIDTH 1600//800
#define SCREEN_HEIGHT 1000//480
AXIS axisX, axisY;

// ------------------- Dynamic Loader ----------------------

HMODULE LoadSharedLibrary(const wstring& path) {
    return LoadLibraryW(path.c_str());
}

FARPROC LoadFunction(HMODULE handle, const char* name) {
    return GetProcAddress(handle, name);
}

void UnloadSharedLibrary(HMODULE handle) {
    if (handle)
        FreeLibrary(handle);
}

// ------------------- Helpers ----------------------
int GetResolution(uint32_t dir, AXIS& axis) {
    if (!getAXIS)
        return -1;

    int ret = getAXIS(dir, &axis);
    if (ret == -1) {
        if (dir == HW_GET_AXIS_X) {
            axis.axMin = DEFAULT_AXIS_MIN_X;
            axis.axMax = DEFAULT_AXIS_MAX_X;
        }
        else {
            axis.axMin = DEFAULT_AXIS_MIN_Y;
            axis.axMax = DEFAULT_AXIS_MAX_Y;
        }
    }
    return ret;
}

string GetCurrentTimestamp() {
    auto now = system_clock::now();
    auto now_time_t = system_clock::to_time_t(now);
    auto now_ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    tm tm_buf;
    localtime_s(&tm_buf, &now_time_t);

    ostringstream oss;
    oss << put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    oss << '.' << setw(3) << setfill('0') << now_ms.count();
    return oss.str();
}

DWORD get_unix_timestamp()
{
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
    return ms; // UNIX timestamp in milliseconds
}

void PrintBasicDeviceInfo() {
    int pressure = 0;
    if (getPressure(HW_GET_PRESSURE, &pressure) != -1) {
        cout << "Pressure: " << pressure << endl;
    }

    char buffer[64] = {};
    if (getVendor(HW_GET_VENDOR, buffer) != -1) {
        cout << "Vendor: " << buffer << endl;
    }
    if (getVendor(HW_GET_PRODUCT, buffer) != -1) {
        cout << "Product: " << buffer << endl;
    }

    VID_PID vidpid;
    if (getVidPid(HW_GET_VID_PID, &vidpid) > 0) {
        cout << "VID: 0x" << hex << uppercase << setw(4) << setfill('0') << vidpid.VID
            << ", PID: 0x" << setw(4) << setfill('0') << vidpid.PID << dec << endl;
    }

    uint8_t uid[16] = {};
    if (getDeviceUid(HW_GET_DEVICE_UID, uid) != -1) {
        cout << "UID: ";
        for (int i = 0; i < 16; ++i) {
            cout << hex << uppercase << setw(2) << setfill('0') << (int)uid[i];
        }
        cout << dec << endl;
    }
}

// ------------------- Entry Point ----------------------
//bool is_pen_opened()
//{
//    return g_is_pen_opened;
//}

int pen_open()
{
    //const wstring dllPath = L"VIEW_SIGN_x64.dll";
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wstring exeDir = exePath;
    size_t pos = exeDir.find_last_of(L"\\/");
    if (pos != wstring::npos) exeDir = exeDir.substr(0, pos);
    wstring dllPath = exeDir + L"\\VIEW_SIGN_x64.dll";

    g_hModule = LoadSharedLibrary(dllPath);
    if (!g_hModule) {
        cerr << "Failed to load VIEW_SIGN.dll" << endl;
        return -1;
    }

    getAXIS = reinterpret_cast<taggetAXIS>(LoadFunction(g_hModule, "getDeviceInfo"));
    getPressure = reinterpret_cast<taggetPressure>(LoadFunction(g_hModule, "getDeviceInfo"));
    getPacketsWithTilt = reinterpret_cast<taggetPacketsWithTilt>(LoadFunction(g_hModule, "getPacketsWithTilt"));
    getVendor = reinterpret_cast<taggetVendor>(LoadFunction(g_hModule, "getDeviceInfo"));
    getVidPid = reinterpret_cast<taggetVidPid>(LoadFunction(g_hModule, "getDeviceInfo"));
    getDeviceUid = reinterpret_cast<taggetDeviceUid>(LoadFunction(g_hModule, "getDeviceInfo"));
    openDevice = reinterpret_cast<tagopenDevice>(LoadFunction(g_hModule, "openDevice"));
    closeDevice = reinterpret_cast<tagcloseDevice>(LoadFunction(g_hModule, "closeDevice"));
    enableEmrScan = reinterpret_cast<tagEnableEmrScan>(LoadFunction(g_hModule, "EnableEmrScan"));

    if (!getAXIS || !getPressure || !getVendor || !getVidPid ||
        !getDeviceUid || !openDevice || !closeDevice || !enableEmrScan) {
        cerr << "Failed to load required functions" << endl;
        UnloadSharedLibrary(g_hModule);
        return -2;
    }

    if (openDevice() != 0) {
        cerr << "Failed to open device" << endl;
        g_is_pen_opened = false;
        UnloadSharedLibrary(g_hModule);
        return -3;
    }

    g_is_pen_opened = true;

    GetResolution(HW_GET_AXIS_X, axisX);
    GetResolution(HW_GET_AXIS_Y, axisY);
    cout << "Axis X: [" << axisX.axMin << " - " << axisX.axMax << "]" << endl;
    cout << "Axis Y: [" << axisY.axMin << " - " << axisY.axMax << "]" << endl;

    PrintBasicDeviceInfo();
    enable_pen(0);
    return 0;
}

int pen_close()
{
    if (g_is_pen_opened == false)
        return 0;

    enable_pen(0);
    closeDevice();
    UnloadSharedLibrary(g_hModule);
    return 0;
}

int enable_pen(int en)
{
    if (g_is_pen_opened == false)
        return -1;

    if(enableEmrScan)
        enableEmrScan(en);

    return 0;
}

int get_pen_data(int& x, int& y, int& tilt_x, int& tilt_y,
    int& press, unsigned int& timestamp)
{
    if (g_is_pen_opened == false)
        return -1;

    x = 0;
    y = 0;
    tilt_x = 0;
    tilt_y = 0;
    press = 0;

    if (getPacketsWithTilt == NULL)
        return 1;

    PACKETS_TILT pkt;
    int rc = getPacketsWithTilt(&pkt);
    if (pkt.Press < 200)
        return 0;

    //dbg("press=%d\n", pkt.Press);
    press = pkt.Press - 200;
    x = pkt.X * SCREEN_WIDTH / axisX.axMax;
    y = pkt.Y * SCREEN_HEIGHT / axisY.axMax;
    tilt_x = pkt.TILT_X;
    tilt_y = pkt.TILT_Y;
    timestamp = get_unix_timestamp();

    return 0;
}

void pen_demo_thread() 
{
    if (g_is_pen_opened == false)
        return;

    while (true) {
        int x, y, tilt_x, tilt_y, press;
        DWORD timestamp;
        get_pen_data(x, y, tilt_x, tilt_y, press, timestamp);
        if (press < 700)
            continue;

        cout << "[" << timestamp << "] "
            << "X=" << x
            << ", Y=" << y
            << ", TILT_X=" << tilt_x
            << ", TILT_y=" << tilt_y
            << ", Pressure=" << press
            << endl;

        this_thread::sleep_for(milliseconds(5));
    }
}

int main_() 
{
    pen_open();
    enable_pen(1);

    thread t(pen_demo_thread);
    t.join();

    pen_close();
    return 0;
}
#endif
