#include "pen_mdoule_interface.h"

#ifdef USE_CHINA_ZD03_DEMO
#define USE_CHINA_ZD03
#endif

#ifdef USE_CHINA_ZD03
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <iomanip>
#include <ctime>
#include <sstream>
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

using namespace std;
using namespace chrono;

const int DEFAULT_AXIS_MIN_X = 0;
const int DEFAULT_AXIS_MAX_X = 20100;
const int DEFAULT_AXIS_MIN_Y = 0;
const int DEFAULT_AXIS_MAX_Y = 12200;
#define SCREEN_WIDTH 1600//800
#define SCREEN_HEIGHT 1000//480

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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * 此类型表示Empen设备的抽象句柄，用于所有设备操作
     */
    typedef void* EmpenHandle;

    /**
     * 数据回调函数类型
     * @param x 当前X坐标值
     * @param y 当前Y坐标值
     * @param pressure 当前压感值
     * @param tilt_x x 軸傾斜角
     * @param tilt_y y 軸傾斜角
     */
    typedef void (*EmpenDataCallback)
        (void* context, uint16_t x, uint16_t y, uint16_t pressure, uint16_t tilt_x, uint16_t tilt_y);
    /**
     * 创建设备实例
     * @return 返回新创建的Empen设备句柄
     */
    EmpenHandle empen_create();

    /**
     * 初始化设备
     * @param handle Empen设备句柄
     * @return 成功返回0，失败返回-1
     */
    int empen_initialize(EmpenHandle handle);

    /**
     * 获取设备信息
     * @param handle Empen设备句柄
     * @param fw_version 存储固件版本的缓冲区
     * @param version_size 缓冲区大小
     * @param max_x 存储最大X坐标的指针
     * @param max_y 存储最大Y坐标的指针
     * @param max_pressure 存储最大压感值的指针
     * @return 成功返回0，失败返回-1
     */
    int empen_get_device_info(
        EmpenHandle handle,
        char* fw_version, int version_size,
        uint16_t* max_x, uint16_t* max_y, uint16_t* max_pressure
    );

    /**
     * 开始监控设备数据
     * @param handle Empen设备句柄
     * @param callback 数据回调函数指针
     */
    void empen_start_monitoring(EmpenHandle handle, EmpenDataCallback callback, void* context);


    /**
     * 停止监控设备数据
     * @param handle Empen设备句柄
     */
    void empen_stop_monitoring(EmpenHandle handle);

    /**
     * 销毁设备实例并释放资源
     * @param handle Empen设备句柄
     */
    void empen_destroy(EmpenHandle handle);

#ifdef __cplusplus
}
#endif
//////////////////////////
#include "libusb.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstring>
#include <vector>
#include <condition_variable>


const uint16_t EMPEN_VENDOR_ID = 0x248A;
const uint16_t EMPEN_PRODUCT_ID = 0x501F;
const unsigned int EMPEN_TIMEOUT_MS = 100;
uint8_t EMPEN_CONFIG_CMD[8] = { 0x05, 0xAF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };//获取设备信息
uint8_t EMPEN_DIGITIZER_CMD[8] = { 0x05, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };//切换为鼠标模式
uint8_t EMPEN_VENDOR_CMD[8] = { 0x05, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };//切换到签字模式

/**
 * Empen 电磁屏设备类
 * 封装了与Empen设备通信的所有核心功能
 */
class EmpenDigitizer {
public:
    /**
     * 构造函数 - 初始化设备状态
     */
    EmpenDigitizer() : dev_handle(nullptr), is_monitoring(false), running(true) {}

    /**
     * 析构函数 - 自动停止并清理资源
     */
    ~EmpenDigitizer() { stop(); }

    /**
     * 初始化设备
     * @return 成功返回true，失败返回false
     */
    bool initialize() {
        if (!init_libusb()) 
            return false;
        if (!open_device()) 
            return false;

        // 给设备响应时间
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        //Linux 需要解除接口绑定
#ifdef __linux__
        release_interface();
#endif

        if (!set_configuration()) {
            cleanup();
            return false;
        }
        return true;
    }

    /**
     * 获取设备信息
     * @param fw_version 存储固件版本的字符串引用
     * @param max_x 存储最大X坐标值的引用
     * @param max_y 存储最大Y坐标值的引用
     * @param max_pressure 存储最大压感值的引用
     * @return 成功返回true，失败返回false
     */
    bool get_device_info(std::string& fw_version, uint16_t& max_x,
        uint16_t& max_y, uint16_t& max_pressure) {

        //重置设备，windows不需要，linux X86必须，很是诡异
        // if(libusb_reset_device(dev_handle) != 0){
        //     return false;
        // }

        if (!send_config_command(EMPEN_CONFIG_CMD))
            return false;

        char fw_buf[11] = { 0 };  // 固件版本缓冲区

        // 循环接收设备响应

        std::vector<uint8_t> data(10);
        int retry = 30;
        while (retry--) {
            int transferred = 0;

            int ret = libusb_interrupt_transfer(
                dev_handle, 0x81, data.data(),
                static_cast<int>(data.size()), &transferred, EMPEN_TIMEOUT_MS);

            // 处理超时
            if (ret == LIBUSB_ERROR_TIMEOUT)// || transferred  data.size())
                continue;

            // 处理错误
            if (ret < 0) {// || transferred != data.size()) {
                dbg("Empen: Data receiving failed: %s\n", libusb_error_name(ret));
                return false;
            }

            // 解析响应数据
            if (data[0] == 0x07 && data[1] == 0xAF) {
                switch (data[2]) {
                case 0:  // 固件第一部分
                    memcpy(fw_buf, &data[3], 5);
                    break;
                case 1:  // 固件第二部分
                    memcpy(fw_buf + 5, &data[3], 5);
                    fw_version = std::string(fw_buf, 10);
                    break;
                case 2:  // 坐标和压感范围 
                    max_x = (data[4] << 8) | data[3];
                    max_y = (data[6] << 8) | data[5];
                    max_pressure = data[7] * 32;
                    return true;
                }
            }
        }

        dbg("Empen: Timeout when getting device information\n");
        return false;
    }

    //hid_device* g_hidapi_handle = NULL;
    /**
     * 开始监控设备数据
     * @param callback 数据回调函数
     */
    void start_monitoring(EmpenDataCallback callback, void* context) {
        {
            // 确保线程安全
            std::lock_guard<std::mutex> lock(monitoring_mutex);
            if (is_monitoring)
                return;
        }

        is_monitoring = true;
        running = true;

        // 启动监控线程
        monitor_thread = std::thread([this, callback, context]() {
            if (!send_config_command(EMPEN_VENDOR_CMD))
                return;

            std::vector<uint8_t> data(10);
            while (running) {
                int transferred = 0;

                // 读取设备数据
                int ret = libusb_interrupt_transfer(
                    dev_handle, 0x81, data.data(),
                    static_cast<int>(data.size()), &transferred, EMPEN_TIMEOUT_MS);

                // 处理有效数据
                if (ret == 0 && transferred == data.size() && data[0] == 0x07)
                {
                    // 解析坐标和压感数据
                    uint16_t x = (data[3] << 8) | data[2];
                    uint16_t y = (data[5] << 8) | data[4];
                    uint16_t pressure = (data[7] << 8) | data[6];
                    uint16_t tilt_x = data[8], tilt_y = data[9];

                    // 调用带上下文的回调函数
                    if (callback && pressure)
                        callback(context, x, y, pressure, tilt_x, tilt_y);
                    else
                        callback(context, 0, 0, 0, 0, 0);
                }
                else if (ret < 0 && ret != LIBUSB_ERROR_TIMEOUT) 
                {
                    dbg("Empen: Data monitoring erro: %s \n", libusb_error_name(ret));
                    break;
                }
                else
                {
                    callback(context, 0, 0, 0, 0, 0);
                }
            }

            // 更新监控状态
            {
                std::lock_guard<std::mutex> lock(monitoring_mutex);
                is_monitoring = false;
            }

            //dbg("monitor_thread is end.\n");
            cv.notify_one();

            }
        );
    }

    /**
     * 停止设备运行
     */
    void stop()
    {
        running = false;
        std::unique_lock<std::mutex> lock(cv_mutex);
        cv.wait_for(lock, std::chrono::milliseconds(200), [this] {
            std::lock_guard<std::mutex> inner(monitoring_mutex);
            return !is_monitoring;
            });

        bool need_join = false;
        {
            std::lock_guard<std::mutex> lock(monitoring_mutex);
            need_join = monitor_thread.joinable();
        }

        if (need_join) {
            monitor_thread.join();
        }

        cleanup();
    }

private:
    libusb_device_handle* dev_handle;
    std::thread monitor_thread;
    std::mutex monitoring_mutex;
    bool is_monitoring;
    std::atomic<bool> running;
    std::condition_variable cv;
    std::mutex cv_mutex;

    /**
     * 初始化libusb库
     * @return 成功返回true，失败返回false
     */
    bool init_libusb() 
    {
        int ret = libusb_init(nullptr);
        if (ret < 0) 
        {
            dbg("Empen: LibUsb initialization failed: %s\n", libusb_error_name(ret));
            return false;
        }
        return true;
    }

    /**
     * 打开Empen设备
     * @return 成功返回true，失败返回false
     */
    bool open_device() 
    {
        dev_handle = libusb_open_device_with_vid_pid(nullptr, EMPEN_VENDOR_ID, EMPEN_PRODUCT_ID);
        if (!dev_handle) 
        {
            dbg("Empen: Device not found \n");
            return false;
        }
        return true;
    }

    /**
     * 接触接口的内核绑定，
     * liux下配置USB设备之前需要调用
     */
    void release_interface() {

        if (libusb_kernel_driver_active(dev_handle, 0)) {
            std::cerr << "Interface 0 detected as occupied. Releasing it now" << std::endl;
            libusb_detach_kernel_driver(dev_handle, 0);
        }
        if (libusb_kernel_driver_active(dev_handle, 1)) {
            std::cerr << "Interface 1 detected as occupied. Releasing it now" << std::endl;
            libusb_detach_kernel_driver(dev_handle, 1);
        }
        if (libusb_kernel_driver_active(dev_handle, 2)) {
            std::cerr << "Interface 2 detected as occupied. Releasing it now" << std::endl;
            libusb_detach_kernel_driver(dev_handle, 2);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    /**
     * 恢复接口的内核绑定
     * liux签字完成时需要调用，切换为鼠标模式
     */
    void restore_interface() {
        int ret = -1;
        ret = libusb_attach_kernel_driver(dev_handle, 0);
        if (ret == LIBUSB_SUCCESS) {
            std::cerr << "Successfully restored kernel binding for Interface 0" << std::endl;
        }
        ret = libusb_attach_kernel_driver(dev_handle, 1);
        if (ret == LIBUSB_SUCCESS) {
            std::cerr << "Successfully restored kernel binding for Interface 1" << std::endl;
        }
        ret = libusb_attach_kernel_driver(dev_handle, 2);
        if (ret == LIBUSB_SUCCESS) {
            std::cerr << "Successfully restored kernel binding for Interface 2" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    /**
     * 配置USB设备
     * @return 成功返回true，失败返回false
     */
    bool set_configuration() {
        // 设置设备配置1
        if (!set_device_configuration())
            return false;

        // 声明接口2
        if (!claim_interface())
            return false;

        // 切换到签字模式
        if (!send_config_command(EMPEN_VENDOR_CMD))
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return true;
    }

    /**
     * 设置设备配置
     * @return 成功返回true，失败返回false
     */
    bool set_device_configuration() {
        int ret = libusb_set_configuration(dev_handle, 1);
        if (ret < 0) {
            std::cerr << "Empen: Configuration 1 setup failed: " << libusb_error_name(ret) << std::endl;
            return false;
        }
        return true;
    }

    /**
     * 声明USB接口
     * @return 成功返回true，失败返回false
     */
    bool claim_interface() {
        int ret = libusb_claim_interface(dev_handle, 2);//2
        if (ret < 0) {
            dbg("Empen: Interface 2 declaration failed: %s\n", libusb_error_name(ret));
            return false;
        }
        return true;
    }

    /**
     * 发送配置命令到设备，用于获取电磁屏信息
     * @return 成功返回true，失败返回false
     */
    bool send_config_command(uint8_t* CMD) {

        if (dev_handle == NULL)
            return false;

        if (0)//debug
        {
            libusb_device* dev = libusb_get_device(dev_handle);
            libusb_config_descriptor* config;
            libusb_get_active_config_descriptor(dev, &config);

            for (int i = 0; i < config->bNumInterfaces; ++i) {
                const libusb_interface& intf = config->interface[i];
                for (int j = 0; j < intf.num_altsetting; ++j) {
                    const libusb_interface_descriptor& desc = intf.altsetting[j];
                    dbg("Interface %d: class=0x%02X, subclass=0x%02X, protocol=0x%02X\n",
                        desc.bInterfaceNumber,
                        desc.bInterfaceClass,
                        desc.bInterfaceSubClass,
                        desc.bInterfaceProtocol);
                }
            }
            libusb_free_config_descriptor(config);

            ////////////////////////////////////////////
            uint8_t report_desc[1024] = { 0 };
            int ret = libusb_control_transfer(
                dev_handle,
                0x81,                 // Device-to-host | Standard | Interface
                0x06,                 // GET_DESCRIPTOR
                (0x22 << 8) | 0x00,   // wValue: HID Report Descriptor (0x22)
                2,                    // wIndex: interface number (你的 HID interface 是 2)
                report_desc,
                sizeof(report_desc),
                1000
            );

            if (ret < 0) {
                dbg("Failed to get HID report descriptor: %s\n", libusb_error_name(ret));
            }
            else {
                dbg("Report Descriptor (%d bytes):\n", ret);
                for (int i = 0; i < ret; ++i)
                    dbg("%02X \n", report_desc[i]);
            }
        }

        // 尝试发送最多3次，第一次通常失败
        for (int i = 0; i < 3; i++) {
            int ret = libusb_control_transfer(
                dev_handle,
                0x21,
                0x09,
                0x0305,// 0x03 << 8 | CMD[0],
                0,//0
                CMD,//const_cast<uint8_t*>(CMD),  // 配置命令
                sizeof(CMD),
                EMPEN_TIMEOUT_MS
            );

            if (ret < 0)
                continue;

            // 检查发送长度
            if (ret == 8)
                return true;
        }

        dbg("Empen: Failed to get device information\n");
        return false;
    }

    /**
     * 清理USB资源
     */
    void cleanup() {
        return;

        if (dev_handle) {
            //切换为鼠标模式
            if (send_config_command(EMPEN_DIGITIZER_CMD))
                std::cerr << "Digitizer mode switch succeeded" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            //Linux 需要恢复接口绑定
#ifdef __linux__
            restore_interface();
#endif

            // 释放USB接口
            libusb_release_interface(dev_handle, 2);
            // 关闭设备
            libusb_close(dev_handle);
            dev_handle = nullptr;
        }

        // 退出libusb
        libusb_exit(nullptr);
    }
};

/**********************************************************
 * Empen C 接口实现
 **********************************************************/

 /**
  * 创建设备实例
  * @return 新创建的Empen设备句柄
  */
EmpenHandle empen_create() {
    return new EmpenDigitizer();
}

/**
 * 初始化设备
 * @param handle Empen设备句柄
 * @return 成功返回0，失败返回-1
 */
int empen_initialize(EmpenHandle handle) {
    EmpenDigitizer* digitizer = static_cast<EmpenDigitizer*>(handle);
    return digitizer->initialize() ? 0 : -1;
}

/**
 * 获取设备信息
 * @param handle Empen设备句柄
 * @param fw_version 固件版本缓冲区
 * @param version_size 缓冲区大小
 * @param max_x 最大X坐标指针
 * @param max_y 最大Y坐标指针
 * @param max_pressure 最大压感值指针
 * @return 成功返回0，失败返回-1
 */
int empen_get_device_info(EmpenHandle handle, char* fw_version, int version_size, uint16_t* max_x, uint16_t* max_y, uint16_t* max_pressure) {
    EmpenDigitizer* digitizer = static_cast<EmpenDigitizer*>(handle);
    std::string versionStr;
    uint16_t x = 0, y = 0, pressure = 0;

    // 获取设备信息
    if (!digitizer->get_device_info(versionStr, x, y, pressure)) {
        return -1;
    }

    // 安全复制固件版本
    if (fw_version && version_size > 0) {
        strncpy(fw_version, versionStr.c_str(), version_size - 1);
        fw_version[version_size - 1] = '\0';  // 确保终止
    }

    // 返回坐标和压感范围
    *max_x = x;
    *max_y = y;
    *max_pressure = pressure;
    return 0;
}

/**
 * 开始监控设备数据
 * @param handle Empen设备句柄
 * @param callback 数据回调函数
 */
void empen_start_monitoring(EmpenHandle handle, EmpenDataCallback callback, void* context) {
    EmpenDigitizer* digitizer = static_cast<EmpenDigitizer*>(handle);
    digitizer->start_monitoring(callback, context);
}

/**
 * 停止设备运行
 * @param handle Empen设备句柄
 */
void empen_stop_monitoring(EmpenHandle handle) {
    EmpenDigitizer* digitizer = static_cast<EmpenDigitizer*>(handle);
    digitizer->stop();
}

/**
 * 销毁设备实例
 * @param handle Empen设备句柄
 */
void empen_destroy(EmpenHandle handle) {
    delete static_cast<EmpenDigitizer*>(handle);
}

///////////////////////////////////////////////////
static EmpenHandle g_hanel = NULL;
struct Pen {
    uint16_t x, y, pressure, tilt_x, tilt_y;
    uint16_t max_x, max_y, max_p;
};
static Pen pen;

//EmpenDataCallback 
void callback_get_pen_data(void* context, uint16_t x, uint16_t y, uint16_t pressure, uint16_t tilt_x, uint16_t tilt_y)
{
    pen.x = x;
    pen.y = y;
    pen.tilt_x = tilt_x;
    pen.tilt_y = tilt_y;
    pen.pressure = pressure;
}

int enable_pen(int en)
{
    if (en)
    {
        empen_start_monitoring(g_hanel, callback_get_pen_data, NULL);
        //dbg("enable_pen(1)\n");
    }
    else
    {
        empen_stop_monitoring(g_hanel);
        //dbg("enable_pen(0)\n");
    }

    return 0;
}

int pen_open()
{
    g_hanel = new EmpenDigitizer();
    if (g_hanel == NULL)
        return -1;

    int res = empen_initialize(g_hanel);
    if (res)
        return -1;

    //get dev info
    char fw_version[15] = { 0 };
    if (empen_get_device_info(g_hanel, fw_version, sizeof(fw_version), &pen.max_x, &pen.max_y, &pen.max_p) != 0)
        return -1;

    if (pen.max_x==0 || pen.max_y==0 || pen.max_p==0)
    {
        pen.max_x = DEFAULT_AXIS_MAX_X;
        pen.max_y = DEFAULT_AXIS_MAX_Y;
        pen.max_p = 256*32;
    }

    dbg("pen fw_version: %s\n", fw_version);
    dbg("max_x=%d max_y=%d, max_p=%d\n", pen.max_x, pen.max_y, pen.max_p);

    enable_pen(0);

    return 0;
}

int pen_close()
{
    empen_stop_monitoring(g_hanel);

    Sleep(100);

    if (g_hanel)
    {
        empen_destroy(g_hanel);
        g_hanel = NULL;
    }

    return 0;
}

int get_pen_data(int& x, int& y, int& tilt_x, int& tilt_y, int& pressure, unsigned int& timestamp)
{
    static int old_x = 0, old_y = 0;

    x = 0, y = 0, tilt_x = 0, tilt_y = 0, pressure = 0;

    if (pen.max_x == 0 || pen.max_y == 0 || pen.max_p == 0)
        return 0;

    auto now = timeGetTime();
    while(timeGetTime()-now < 100)
        if (pen.x != old_x || pen.y != old_y)
            break;
    
    if (timeGetTime() - now >= 100)
        return 0;

    old_x = pen.x;
    old_y = pen.y;

    pressure = (DWORD)pen.pressure * 10 / pen.max_p;
    if (pressure<2 || pen.x==0 || pen.y==0)
        return 0;

    UINT64 pen_x = pen.x, pen_y = pen.y;
    x = pen_x * SCREEN_WIDTH / pen.max_x;
    y = pen_y * SCREEN_HEIGHT / pen.max_y;

    tilt_x = pen.tilt_x;
    tilt_y = pen.tilt_y;

    //if (x && y)
    //    dbg("x=%d y=%d tilt_x=%d tilt_y=%d pressure=%d\n", x, y, tilt_x, tilt_y, pressure);

    return 0;
}

#ifdef USE_CHINA_ZD03_DEMO
int main()
#else
static int demo()
#endif
{
    pen_open();
    enable_pen(1);

    while (1)
    {
        int x, y, press, tilt_x, tilt_y;
        unsigned int timestamp;
        get_pen_data(x, y, tilt_x, tilt_y, press, timestamp);
    }
    pen_close();
    return 0;
}

#endif

