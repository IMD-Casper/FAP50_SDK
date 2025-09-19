#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include <libusb.h>

//Add UsbDeviceType and Context structure
enum UsbDeviceType {
	DEVICE_SENSOR = 0,
	DEVICE_PEN_LP5,
	DEVICE_MAX
};

struct UsbDeviceContext {
	int android_fd = 0;
	libusb_context* ctx = nullptr;
	libusb_device_handle* handle = nullptr;
	uint16_t vid = 0, pid = 0;
	int Dev_Type = 0;
	int interface_num = 0;
	bool connected = false;
	std::atomic<bool> polling{ false };
	std::thread poll_thread;
	std::mutex mu_on_off;

	UsbDeviceContext() = default;
	UsbDeviceContext(int fd, libusb_context* c, libusb_device_handle* h,
		uint16_t v, uint16_t p, int type, int ifnum)
		: android_fd(fd), ctx(c), handle(h),
		vid(v), pid(p), Dev_Type(type), interface_num(ifnum),
		connected(false), polling(false) {
	}
};
