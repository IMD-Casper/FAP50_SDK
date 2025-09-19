// FAP50Plus_SDK_Panel.cpp : Defines the exported functions for the DLL.
//
//
// 

#ifdef _WIN32 //windows
#include "pch.h"
#include "framework.h"
#include "imd_framework.h"
#endif
#include "IMDFAP50Plus_SDK_Panel.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "uv.h"
#ifdef __ANDROID__
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif
//Debug purpose
//#pragma once
//#include "imd_framework.h"


// Constants
#define TRANS_ACK_END        0x000000
#define TRANS_ACK_SUCCESS    0x55AA00
#define TRANS_ACK_FAILURE    0x55AA01
#define TRANS_RETRY          0x55AA02

#define CMD_ZERO			0
#define CMD_NEXT_KEEPGOING	1
#define CMD_NEXT_FINISHED	2
#define CMD_NEXT_SAVEJPG	3
#define CMD_SET_BRIGHTNESS  6
#define END_MARKER			0xFFFFFFFF

#define SECTION_SIZE         (60 * 1024)  // 30KB


typedef struct fap50_panel {
    uv_tcp_t socket;
    uv_connect_t connect_req;
    uv_timer_t timer_req;
} fap50_panel_t;

static fap50_panel_t* internal_panel = NULL;



#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "FAP50_PANEL_CONNECT"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif // __ANDROID__

static bool socket_opened = false;
static bool connection_done = false;
bool g_is_product_support_panel = false;
int init_device();

#ifdef __cplusplus
extern "C" {
#endif

    static void on_connect(uv_connect_t* req, int status) {

#ifdef __ANDROID__
        // Test code       
        sockaddr_storage ss; int len = sizeof(ss);
        uv_tcp_getsockname((uv_tcp_t*)req->handle, (sockaddr*)&ss, &len);
        char ip[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &((sockaddr_in*)&ss)->sin_addr, ip, sizeof(ip));
        LOGI("on_connect OK, local=%s", ip);

        uv_tcp_nodelay((uv_tcp_t*)req->handle, 1);
#endif
        if (status < 0) {
#ifdef __ANDROID__
            LOGE("Connection failed: %s", uv_strerror(status));
#endif
            fprintf(stderr, "Connection failed: %s\n", uv_strerror(status));
            socket_opened = false;
        }
        else {
            socket_opened = true;
#ifdef __ANDROID__
            LOGI("Connection successful!");
#endif
            //dbg("Connection successful!\n");

            uv_timer_stop(&internal_panel->timer_req);
            uv_close((uv_handle_t*)&internal_panel->timer_req, nullptr);
        }

        connection_done = true;
    }

    static void on_timeout(uv_timer_t* handle) {
        if (!socket_opened) {
#ifdef __ANDROID__
            LOGE("on_timeout: Connection timed out");
#else
            dbg("on_timeout: Connection timed out\n");
#endif
            connection_done = true;
        }
        connection_done = true;
        uv_timer_stop(handle);
        uv_close((uv_handle_t*)handle, nullptr);
    }

    static void on_socket_closed(uv_handle_t* handle) {
        if (internal_panel) {
            free(internal_panel);
            internal_panel = NULL;
        }
    }

    static void on_write(uv_write_t* req, int status) {
        if (status < 0) {
            fprintf(stderr, "Write failed: %s\n", uv_strerror(status));
        }
        else {
            //printf("Write completed successfully.\n");
        }

        if (req->data) {
            free(req->data);
        }
        free(req);
    }

    static bool send_package(const uint8_t* data, uint32_t length) {
        if (!socket_opened)
        {
#ifdef __ANDROID__
            LOGI("send_package socket not opened");
#endif
            return false;
        }


        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        if (!req) return false;

        // Copiamos los datos porque uv_write puede ser async y el buffer debe persistir
        uint8_t* buf_data = (uint8_t*)malloc(length);
        if (!buf_data) {
            free(req);
            return false;
        }
        memcpy(buf_data, data, length);

        uv_buf_t buf = uv_buf_init((char*)buf_data, length);
        req->data = buf_data;

        int r = uv_write(req, (uv_stream_t*)&internal_panel->socket, &buf, 1, on_write);
        if (r != 0) {
            dbg("Error sending: %s\n", uv_strerror(r));
            free(buf_data);
            free(req);
            return false;
        }
        return true;
    }

    static bool send_command(uint32_t cmd) {
        return send_package((const uint8_t*)&cmd, sizeof(cmd));

    }
#ifdef __ANDROID__

    static bool find_usb_like_ipv4(char out[INET_ADDRSTRLEN]) {
        out[0] = '\0';
        struct ifaddrs* ifaddr = nullptr;
        if (getifaddrs(&ifaddr) == -1) return false;

        for (struct ifaddrs* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
            if (!(ifa->ifa_flags & IFF_UP)) continue;
            const char* name = ifa->ifa_name ? ifa->ifa_name : "";
            bool name_ok = strcasestr(name, "usb") || strcasestr(name, "rndis") ||
                (strncmp(name, "eth", 3) == 0) || (strncmp(name, "en", 2) == 0);
            if (!name_ok) continue;

            auto* sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
            char ip[INET_ADDRSTRLEN]; inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip));
            if (strncmp(ip, "192.168.100.", 12) == 0) {
                strncpy(out, ip, INET_ADDRSTRLEN);
                freeifaddrs(ifaddr);
                return true;
            }
        }
        freeifaddrs(ifaddr);
        return false;
    }

#endif



    bool connect_fap50_panel(const char* host, uint16_t port) {
#ifdef __ANDROID__
        LOGI("connect_fap50_panel CALLED host=%s port=%u", host, port);
#endif
        if (!g_is_product_support_panel)
        {
            init_device();
            if (!g_is_product_support_panel)
                return false;
        }

        if (socket_opened) {
            dbg("Socket is already opened\n");
            return true;
        }

        if (!internal_panel) {
            internal_panel = (fap50_panel_t*)malloc(sizeof(fap50_panel_t));
            if (!internal_panel)
                return false;
        }

        uv_loop_t* loop = uv_default_loop();
        int r = uv_tcp_init(loop, &internal_panel->socket);
#ifdef __ANDROID__
        LOGI("uv_tcp_init r=%d", r);
#endif
        if (r != 0) {
#ifdef __ANDROID__
            LOGI("Error when init the tcp socket: %d\n", r);
#else
            printf("Error when init the tcp socket: %d\n", r);
#endif
            return false;
        }
#ifdef __ANDROID__
        char local_ip[INET_ADDRSTRLEN];
        if (find_usb_like_ipv4(local_ip)) {
            struct sockaddr_in src {};
            uv_ip4_addr(local_ip, 0, &src);
            int br = uv_tcp_bind(&internal_panel->socket, (const struct sockaddr*)&src, 0);
            LOGI("uv_tcp_bind src=%s ret=%d\n", local_ip, br);
        }
        else {
            LOGE("Cannot find src=%s\n", local_ip);

        }
#endif


        struct sockaddr_in dest {};
        r = uv_ip4_addr(host, port, &dest);
#ifdef __ANDROID__
        LOGI("uv_ip4_addr r=%d", r);
#endif
        if (r != 0) {
#ifdef __ANDROID__
            LOGI("Error converting IP address : % d\n", r);
#else
            dbg("Error converting IP address: %d\n", r);
#endif
            return false;
        }
#ifdef __ANDROID__
        //Test
        uv_tcp_nodelay(&internal_panel->socket, 1);

#endif

        connection_done = false;

        r = uv_tcp_connect(&internal_panel->connect_req, &internal_panel->socket, (const struct sockaddr*)&dest, on_connect);
#ifdef __ANDROID__
        LOGI("uv_tcp_connect r=%d", r);
#endif
        if (r != 0) {
#ifdef __ANDROID__
            LOGE("Error during connection: %d\n", r);
#else
            dbg("Error during connection: %d\n", r);
#endif
            return false;
        }


        g_is_product_support_panel = true;

        uv_timer_init(loop, &internal_panel->timer_req);
        uv_timer_start(&internal_panel->timer_req, on_timeout, 200, 0);

        while (!connection_done && uv_loop_alive(loop)) {
            uv_run(loop, UV_RUN_ONCE);
        }


        if (!socket_opened) {
#ifdef __ANDROID__
            LOGE("connect_fap50_panel: connection failed or timed out");
#else
            dbg("connect_fap50_panel: connection failed or timed out\n");
#endif

            return false;
        }
        /*
        uint64_t start_time = uv_hrtime();
        const uint64_t timeout_ns = 1000000000ULL;

        bool connected = false;
        while (uv_loop_alive(loop) && (uv_hrtime() - start_time) < timeout_ns) {
            uv_run(loop, UV_RUN_NOWAIT);

            if (socket_opened) {
                connected = true;
                break;
            }

            uv_sleep(10);
        }

        if (!connected) {
            printf("Connection timed out.\n");
            return false;
        }
        */
#ifdef __ANDROID__
        LOGI("Connected.\n");
#else
        //dbg("Connected.\n");
#endif

        return true;
    }

    bool send_jpg_fap50_panel(const uint8_t* buffer, uint32_t size) {
        if (!g_is_product_support_panel)
            return false;

        if (!internal_panel || !socket_opened)
            return false;

        uint32_t file_size_net = size;
        uint32_t section_size = SECTION_SIZE;

        if (!send_package((const uint8_t*)&file_size_net, sizeof(file_size_net))) {
            dbg("Error sending size\n");
            return false;
        }

        if (!send_package((const uint8_t*)&section_size, sizeof(section_size))) {
            dbg("Error sending section\n");
            return false;
        }

        uint32_t offset = 0;
        uv_loop_t* loop = uv_default_loop();
        while (offset < size) {
            uint32_t chunk_size = (size - offset > SECTION_SIZE) ? SECTION_SIZE : (size - offset);
            if (!send_package(&buffer[offset], chunk_size)) {
                dbg("Error sending chunkk\n");
                return false;
            }
            offset += chunk_size;
            uv_run(loop, UV_RUN_NOWAIT);
        }
        uv_run(loop, UV_RUN_NOWAIT);

        if (!send_command(0x000001)) {   //0x55AA01)) {
            printf("Error sending CMD_NEXT_KEEPGOING\n");
            return false;
        }
        return true;
    }


    void disconnect_fap50_panel() {
        //dbg("disconnect_fap50_panel\n");

        if (!g_is_product_support_panel)
            return;

        if (!internal_panel || !socket_opened) return;

        send_command(END_MARKER);
        send_command(CMD_NEXT_FINISHED);

        uv_close((uv_handle_t*)&internal_panel->socket, on_socket_closed);
        uv_run(uv_default_loop(), UV_RUN_ONCE);

        socket_opened = false;
    }

    void set_brightness(int i)
    {
        if (!g_is_product_support_panel)
            return;

        if (!socket_opened) {
            dbg("Socket is not opened, cannot set brightness\n");
            return;
        }

        struct { uint32_t cmd; uint32_t value; } pkg = { CMD_SET_BRIGHTNESS, (uint32_t)i };

        send_command(CMD_ZERO);
        send_command(CMD_ZERO);
        if (!send_package((uint8_t*)&pkg, sizeof(pkg)))
        {
            dbg("send brightness failed\n");
            return;
        }

        while (uv_loop_alive(uv_default_loop())) {
            uv_run(uv_default_loop(), UV_RUN_NOWAIT);
            uv_sleep(1);
        }

        return;

    }
#ifdef __cplusplus
}
#endif

//#endif




