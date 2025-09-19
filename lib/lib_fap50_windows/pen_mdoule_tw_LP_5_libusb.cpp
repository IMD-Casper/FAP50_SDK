#ifdef USE_TW_LP_5_LIBUSB
#include "imd_framework.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <libusb.h>
#include <chrono>
#ifdef __WIN32
#include <windows.h>
#endif


unsigned long getTickCountMS() {
    using namespace std::chrono;
    auto now = steady_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
    return static_cast<unsigned long>(ms);
}
using namespace std;

// 根據你的設備修改以下參數
#define VENDOR_ID      0x0543//0x248A
#define PRODUCT_ID     0xE601//0x501F
#define INTERFACE_NUM  0
#define ENDPOINT_IN    0x81
#define ENDPOINT_OUT   0x02

#define REPORT_ID 0x02

const int DEFAULT_AXIS_MIN_X = 0;
const int DEFAULT_AXIS_MAX_X = 10262*2;
const int DEFAULT_AXIS_MIN_Y = 0;
const int DEFAULT_AXIS_MAX_Y = 7422*2;
#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 1000

struct AXIS {
    int32_t axMin;
    int32_t axMax;
};
AXIS axisX, axisY;

libusb_device_handle* dev_handle = nullptr;
static libusb_context* s_ctx = nullptr;     
static bool            s_opened = false;

bool send_report(const std::vector<uint8_t>& report)
{
    const uint8_t bmRequestType = 0x21;     // Host to Device | Class | Interface
    const uint8_t bRequest = 0x09;          // SET_REPORT
    const uint16_t wValue = (2 << 8) | report[0];  // 3 = Feature report
    const uint16_t wIndex = INTERFACE_NUM;

    int r = libusb_control_transfer(dev_handle,
        bmRequestType,
        bRequest,
        wValue,
        wIndex,
        const_cast<uint8_t*>(report.data()),
        report.size(),
        1000);

    if (r < 0) {
        std::cerr << "[send_feature_report] failed: " << libusb_error_name(r) << "\n";
        return false;
    }
    //std::cout << "[send_feature_report] sent " << r << " bytes\n";
    return true;
}

bool receive_report(std::vector<uint8_t>& buffer, int length = 14) {
    buffer.resize(length);
    int actual_length;
    int r = libusb_interrupt_transfer(dev_handle, ENDPOINT_IN, buffer.data(), length, &actual_length, 1000);
    return r == 0 && actual_length == length;
}

void print_endpoint_info()
{
    libusb_device* dev = libusb_get_device(dev_handle);
    libusb_config_descriptor* config;
    libusb_get_config_descriptor(dev, 0, &config);

    for (int i = 0; i < config->bNumInterfaces; ++i) {
        const libusb_interface& iface = config->interface[i];
        for (int j = 0; j < iface.num_altsetting; ++j) {
            const libusb_interface_descriptor& altsetting = iface.altsetting[j];

            std::cout << "Interface " << (int)altsetting.bInterfaceNumber << ":\n";
            for (int k = 0; k < altsetting.bNumEndpoints; ++k) {
                const libusb_endpoint_descriptor& ep = altsetting.endpoint[k];
                std::cout << "  Endpoint Address: 0x" << hex << (int)ep.bEndpointAddress
                    << ", Attributes: 0x" << hex << (int)ep.bmAttributes
                    << ", MaxPacketSize: " << dec << ep.wMaxPacketSize << "\n";
            }
        }
    }

    libusb_free_config_descriptor(config);
}

#ifdef __cplusplus
extern "C" {
#endif
    int pen_open(intptr_t android_fd, void** libusb_ctx, void** DevHandle)
    {
        if (s_opened) return 0;
#ifdef __ANDROID__
        int ret = 0;        
        if (!s_ctx) {
            libusb_set_option(s_ctx, LIBUSB_OPTION_NO_DEVICE_DISCOVERY);
            ret = libusb_init(&s_ctx);
            if (ret != 0) {
                LOGD("libusb_init failed, code = %d\n", ret);
                return -1;
            }
            //LOGI("libusb_init OK, s_ctx=%p", s_ctx);
        }
        else {
            s_ctx = reinterpret_cast<libusb_context*>(libusb_ctx);
        }     
        *libusb_ctx = s_ctx;

        if (android_fd != 0) 
        {            
            ret = libusb_wrap_sys_device(s_ctx, android_fd, &dev_handle);           
            if ( ret != 0) 
                return -2;           
        }
        else {
            dev_handle = libusb_open_device_with_vid_pid(s_ctx, VENDOR_ID, PRODUCT_ID);
            if (!dev_handle) return -3;
        }
        *DevHandle = dev_handle;
#else

        if (!s_ctx && libusb_init(&s_ctx) != 0) return -1;
        dev_handle = libusb_open_device_with_vid_pid(s_ctx, VENDOR_ID, PRODUCT_ID);
        if (!dev_handle) return -3;
#endif

#ifndef _WIN32
        if (libusb_kernel_driver_active(dev_handle, INTERFACE_NUM) == 1) {
            libusb_detach_kernel_driver(dev_handle, INTERFACE_NUM);
        }
#endif
        if (libusb_claim_interface(dev_handle, INTERFACE_NUM) != 0) {
            libusb_close(dev_handle); dev_handle = nullptr;
            if (libusb_ctx == nullptr) { libusb_exit(s_ctx); s_ctx = nullptr; }
            return -4;
        }

        // TODO: LP-5 初始命令（原 send_report/receive_report 流程）
        print_endpoint_info();

        // Step 1: switch to ID2
        send_report({ 0x02, 0x10, 0x01, 0x00, 0x00 });
        this_thread::sleep_for(chrono::milliseconds(10));

        vector<uint8_t> resp;
        //// Step 2: Get X max
        //send_report({ 0x02, 0x01, 0x00, 0x00, 0x00 });
        //receive_report(resp, 5);
        //uint16_t x_max = resp[2] << 8 | resp[1];
        //cout << "X Max: " << x_max << endl;

        //// Step 3: Get Y max
        //send_report({ 0x02, 0x01, 0x01, 0x00, 0x00 });
        //receive_report(resp, 5);
        //uint16_t y_max = resp[2] << 8 | resp[1];
        //cout << "Y Max: " << y_max << endl;

        axisX.axMin = DEFAULT_AXIS_MIN_X;
        axisX.axMax = DEFAULT_AXIS_MAX_X;
        axisY.axMin = DEFAULT_AXIS_MIN_Y;
        axisY.axMax = DEFAULT_AXIS_MAX_Y;

        // Step 4: Get Pressure
        send_report({ 0x02, 0x08, 0x00, 0x00, 0x00 });
        receive_report(resp, 5);
        uint16_t pressure = resp[2] << 8 | resp[1];
        cout << "Pressure Level: " << pressure << endl;

        s_opened = true;
        return 0;
    }

    int pen_close() {
        if (!s_opened) return 0;

        libusb_release_interface(dev_handle, INTERFACE_NUM);
        libusb_close(dev_handle); dev_handle = nullptr;

        if (s_ctx) {
            libusb_exit(s_ctx);
            s_ctx = nullptr;
        }
        s_opened = false;
        return 0;
    }

    /*
    int pen_open() {

        if (libusb_init(nullptr) != 0) {
            dbg("libusb init failed\n");
            return -1;
        }

        dev_handle = libusb_open_device_with_vid_pid(nullptr, VENDOR_ID, PRODUCT_ID);
        if (!dev_handle) {
            dbg("Device not found\n");
            return -2;
        }

        if (libusb_claim_interface(dev_handle, INTERFACE_NUM) != 0) {
            dbg("Cannot claim interface\n");
            return -3;
        }

        print_endpoint_info();

        // Step 1: switch to ID2
        send_report({ 0x02, 0x10, 0x01, 0x00, 0x00 });
        this_thread::sleep_for(chrono::milliseconds(10));

        vector<uint8_t> resp;
        //// Step 2: Get X max
        //send_report({ 0x02, 0x01, 0x00, 0x00, 0x00 });
        //receive_report(resp, 5);
        //uint16_t x_max = resp[2] << 8 | resp[1];
        //cout << "X Max: " << x_max << endl;

        //// Step 3: Get Y max
        //send_report({ 0x02, 0x01, 0x01, 0x00, 0x00 });
        //receive_report(resp, 5);
        //uint16_t y_max = resp[2] << 8 | resp[1];
        //cout << "Y Max: " << y_max << endl;

        axisX.axMin = DEFAULT_AXIS_MIN_X;
        axisX.axMax = DEFAULT_AXIS_MAX_X;
        axisY.axMin = DEFAULT_AXIS_MIN_Y;
        axisY.axMax = DEFAULT_AXIS_MAX_Y;

        // Step 4: Get Pressure
        send_report({ 0x02, 0x08, 0x00, 0x00, 0x00 });
        receive_report(resp, 5);
        uint16_t pressure = resp[2] << 8 | resp[1];
        cout << "Pressure Level: " << pressure << endl;

        return 0;
    }

    int pen_close() {
        if (dev_handle) {
            libusb_release_interface(dev_handle, INTERFACE_NUM);
            libusb_close(dev_handle);
            libusb_exit(nullptr);
            dev_handle = nullptr;
        }

        return 0;
    }
    */
    int enable_pen(int enable) {
        // 可選：依需要加入其他 enable 控制命令
        (void)enable;

        return 0;
    }

    // 接收一筆筆 14-byte 筆資料（report ID = 2）
    int get_pen_data(int& x, int& y, int& tilt_x, int& tilt_y, int& press, unsigned long& timestamp) {
        vector<uint8_t> buf;
        if (!receive_report(buf, 14))
        {
            timestamp = 0;
            return 0;
        }

        press = buf[10] | (buf[11] << 8);
        press -= 200;
        press = press / 120 + 1;
        if (press <= 2)
        {
            press = 0;
            x = 0;
            y = 0;
            return 0;
        }

        // 解析筆資料
        x = buf[1] | (buf[2] << 8);
        y = buf[5] | (buf[6] << 8);
        x = x * SCREEN_WIDTH / axisX.axMax;
        y = y * SCREEN_HEIGHT / axisY.axMax;
        tilt_x = buf[12];
        tilt_y = buf[13];
        timestamp = getTickCountMS();     //GetTickCount();

        return 0;
    }
#ifdef __cplusplus
}  
#endif

int demo()
{

    if (!pen_open(0, nullptr, nullptr)) {
        return -1;
    }
   
    enable_pen(1);

    while (1)
    {
        int x = 0, y = 0, press = 0, tilt_x = 0, tilt_y = 0;
        unsigned long timestamp = 0;

        if (get_pen_data(x, y, tilt_x, tilt_y, press, timestamp)) {
            if (timestamp == 0)
                continue;

            cout << "[" << timestamp << "] "
                << "X=" << x
                << ", Y=" << y
                << ", TILT_X=" << tilt_x
                << ", TILT_Y=" << tilt_y
                << ", Pressure=" << press
                << endl;
        }
        else {
            cerr << "Read failed or timeout\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // optional: 控制頻率
    }

    pen_close();

    return 0;
}
#endif
