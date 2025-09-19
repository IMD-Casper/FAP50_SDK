#include "imd_framework.h"
#include "IMDFPSApi.h"
#include "SensorDevice.h"
#include "pen_mdoule_interface.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static CSensorDevice* g_pkSensorDev[USB_BDG_MAX]{nullptr};

CSensorDevice* TransferPointer(HANDLE hDevice)
{
	//I_Device* hDev = (I_Device*)hDevice;
	CSensorDevice* pSensor = (CSensorDevice*)hDevice;

	char i;
	bool bFound = false;

	for (i = 0; i < USB_BDG_MAX; i++)
	{
		if (g_pkSensorDev[i] == NULL)
			continue;

		if (g_pkSensorDev[i] == pSensor)
		{
			bFound = true;
			break;
		}
	}

	//if (!bFound)
	//	return NULL;

	return pSensor;
}

//#include <jni.h>
static int g_android_fd = 0;
void WINAPI UT_SetAndroidInfo(int fd)
{
	g_android_fd = fd;
}
static libusb_context* g_ctx = nullptr;
static libusb_device_handle* g_handle = nullptr;

static CSensorDevice g_sensor;// = CSensorDevice{};
#define VID		0x2B41
#define PID		0x0501
#define INTERFACE_NUM 0

atomic<bool> device_connected(false);

/*
//new implement to support mnultiple USB device Start
static UsbDeviceContext g_usb_devs[DEVICE_MAX] = {
	{0, nullptr, nullptr, 0x2B41, 0x0501, DEVICE_SENSOR, 0}, 
	{0, nullptr, nullptr, 0x0543, 0xE601, DEVICE_PEN_LP5, 1} 
};

UsbDeviceContext& get_usb_device(UsbDeviceType type) {
	return g_usb_devs[type];
}

DWORD ConnectPen() {
#ifdef __ANDROID__		
	//LOGD("ConnectPen: start to connect pen");
#endif
	auto& penDev = get_usb_device(DEVICE_PEN_LP5);
#ifdef __ANDROID__		
	//LOGD("PEN Android fd = %d", (intptr_t)penDev.android_fd);
#endif
	int rc = pen_open((intptr_t)penDev.android_fd,
		reinterpret_cast<void**>(&penDev.ctx), 
		reinterpret_cast<void**>(&penDev.handle));
#ifdef __ANDROID__			
	LOGI("Pen ctx = %d", penDev.ctx);
	//LOGI("Pen open result = %d", rc);
#endif
	return (rc == 0) ? ERROR_SUCCESS : ERROR_GEN_FAILURE;

}
void DisconnectPen() {
	pen_close();
}

DWORD ConnectUsbDevice(UsbDeviceContext& dev) {
	DWORD res = ERROR_SUCCESS;
	if (dev.Dev_Type == DEVICE_SENSOR) {
		res = g_sensor.ConnectSensor(dev.handle);
		if (res == ERROR_SUCCESS)
		{
			g_pkSensorDev[0] = &g_sensor;

		}
			
	} else {
		res = ConnectPen();
	}
	return res;
}

bool open_usb_device(UsbDeviceContext& dev) 
{
	std::lock_guard<std::mutex> lock(dev.mu_on_off);
	bool result = false;
	if (dev.connected) 
		return true;

	if (dev.Dev_Type == DEVICE_PEN_LP5)
	{
		if (ConnectUsbDevice(dev) == ERROR_SUCCESS) {
			dev.connected = true;
			result = true;
		}
		else
		{
#ifdef __ANDROID__		
			LOGI("open_usb_dev fail to connect usb devices");
#endif			
			result = false;
		}
		return result;
	}

	//IMD_Sensor
#ifdef __ANDROID__
	libusb_set_option(dev.ctx, LIBUSB_OPTION_NO_DEVICE_DISCOVERY, NULL);
#endif
	if (!dev.ctx)
	{
		if (libusb_init(&dev.ctx) < 0)
		{
			return false;
		}
	}
#ifdef __ANDROID__
	if (dev.android_fd != 0)
	{
		if (libusb_wrap_sys_device(NULL, (intptr_t)dev.android_fd, &dev.handle) != 0)
		{
			libusb_exit(dev.ctx);
			dev.ctx = nullptr;
			return false;
		}
	}
	else
#endif
	{
		dev.handle = libusb_open_device_with_vid_pid(dev.ctx, dev.vid, dev.pid);
		if (!dev.handle) 
		{
			libusb_exit(dev.ctx); 
			dev.ctx = nullptr;
			return false;
		}
	}

#ifndef _WIN32
	if (libusb_kernel_driver_active(dev.handle, dev.interface_num) == 1) 
	{
		libusb_detach_kernel_driver(dev.handle, dev.interface_num);
	}
#endif
	if (libusb_claim_interface(dev.handle, dev.interface_num) != 0) {
		libusb_close(dev.handle); dev.handle = nullptr;
		libusb_exit(dev.ctx);    dev.ctx = nullptr;
		return false;
	}

	if (ConnectUsbDevice(dev) != ERROR_SUCCESS) {
		libusb_release_interface(dev.handle, dev.interface_num);
		libusb_close(dev.handle); dev.handle = nullptr;
		libusb_exit(dev.ctx);     dev.ctx = nullptr;
		return false;
	}

	dev.connected = true;
	return true;
	
}

void close_usb_device(UsbDeviceContext& dev) {
	std::lock_guard<std::mutex> lock(dev.mu_on_off);

	if (!dev.connected) return;

	if (dev.Dev_Type == DEVICE_PEN_LP5)
	{
		DisconnectPen();
		dev.connected = false;
		return;
	}

	if (dev.handle) {
		libusb_release_interface(dev.handle, dev.interface_num);
		libusb_close(dev.handle);
		dev.handle = nullptr;
	}
	if (dev.ctx) {
		libusb_exit(dev.ctx);
		dev.ctx = nullptr;
	}
	dev.connected = false;
}

void poll_usb(UsbDeviceContext& dev) {

	libusb_context* tmp_ctx = nullptr;
	int libusb_init_cnt = libusb_init(&tmp_ctx);
#ifdef __ANDROID__
	LOGD("Libusb_Cnt = %d", libusb_init_cnt);
#endif
	if (libusb_init_cnt < 0)
		return;

	libusb_device** list = nullptr;
	ssize_t count = libusb_get_device_list(tmp_ctx, &list);
	bool found = false;

	for (ssize_t i = 0; i < count; ++i) {
		libusb_device_descriptor desc{};
		libusb_get_device_descriptor(list[i], &desc);
		if (desc.idVendor == dev.vid && desc.idProduct == dev.pid) {
			found = true; 
			break;
		}
	}
	libusb_free_device_list(list, 1);
	libusb_exit(tmp_ctx);

	if (found && !dev.connected) {
		open_usb_device(dev);
	}
	else if (!found && dev.connected) {
		close_usb_device(dev);
	}
}

void WINAPI UT_start_polling(UsbDeviceType type) {
	auto& dev = g_usb_devs[type];
	if (dev.polling) return;
	dev.polling = true;
	dev.poll_thread = std::thread([pDev = &dev]() {
		while (pDev->polling) {
			try {
				poll_usb(*pDev);
			}
			catch (...) {
#ifdef __ANDROID__
				LOGD("Exception in polling thread VID=%04X PID=%04X", pDev->vid, pDev->pid);
#endif
			}
			
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		});
}

void WINAPI UT_stop_polling(UsbDeviceType type) {
	auto& dev = g_usb_devs[type];
	dev.polling = false;
	if (dev.poll_thread.joinable())
		dev.poll_thread.join();
}

void WINAPI UT_SetAndroidInfo(int fd, unsigned int uiVID, unsigned int uiPID)
{
	for (int i = 0; i < DEVICE_MAX; i++) 
	{
		if (uiVID == g_usb_devs[i].vid && uiPID == g_usb_devs[i].pid)
		{
			g_usb_devs[i].android_fd = fd;
			break;
		}
	}	
}

HANDLE WINAPI UT_ConnectSensor() {
	auto& dev = get_usb_device(DEVICE_SENSOR);

	if (!dev.connected) {	
		if (!open_usb_device(dev)) {
			UT_start_polling(DEVICE_SENSOR);
			return (HANDLE)-1;
		}
	}
	return (HANDLE)g_pkSensorDev[0];
}

void WINAPI UT_DisConnectSensor() {
	UT_stop_polling(DEVICE_SENSOR);
	close_usb_device(get_usb_device(DEVICE_SENSOR));
}

//new implement to support mnultiple USB device End
*/
void WINAPI UT_SetAndroidInfo(int fd, unsigned int uiVID, unsigned int uiPID)
{
	if (uiVID == 0x2B41)
	{
		g_android_fd = fd;
	}
	
}


// 開啟裝置
static mutex mu_device_on_off;
bool open_device()
{
	lock_guard<mutex> lock(mu_device_on_off);
	
	//dbg("open_device()\n");
	g_handle = libusb_open_device_with_vid_pid(g_ctx, VID, PID);
	if (!g_handle) 
	{
		dbg("libusb_open_device_with_vid_pid() failed.\n");
		return false;
	}

	//// 分離 kernel driver（Linux）
	//if (libusb_kernel_driver_active(dev_handle, INTERFACE_NUM)) {
	//	libusb_detach_kernel_driver(dev_handle, INTERFACE_NUM);
	//}

	//int res = libusb_claim_interface(dev_handle, INTERFACE_NUM);
	//if (res != 0)
	//{
	//	std::cerr << "claim interface 失敗: " << libusb_error_name(res) << "\n";
	//	libusb_close(dev_handle);
	//	dev_handle = nullptr;
	//	return false;
	//}

	DWORD dwRet = g_sensor.ConnectSensor(g_handle);
	if (dwRet != ERROR_SUCCESS)
	{
		dbg(" g_sensor.ConnectSensor() fail.\n");
		return false;
	}

	//dbg("open_device() success.\n");
	g_pkSensorDev[0] = &g_sensor;
 
	//READER_INFORMATION ver;
	//DWORD dw = g_sensor.GetUSBReaderInfo((BYTE*)&ver, sizeof(READER_INFORMATION));
	//dbg("FW %.2X.%.2X \n", ver.FirmwareMajor, ver.FirmwareMinor);
	return true;
}


void close_device()
{
	lock_guard<mutex> lock(mu_device_on_off);

	//dbg("close_device()\n");

	if (g_handle)
	{
		libusb_release_interface(g_handle, INTERFACE_NUM);
		libusb_close(g_handle);
		g_handle = nullptr;
		g_pkSensorDev[0] = NULL;
	}
}

// 輪詢 USB 裝置
void poll_usb()
{
	static bool is_polling_usb = false;
	if (is_polling_usb) 
		return;
	is_polling_usb = true;

	//dbg("polling USB... \n");

	while (true)
	{
		//dbg("get usb list start.\n");
		libusb_device** list = nullptr;
		ssize_t count = libusb_get_device_list(g_ctx, &list);
		//dbg("libusb_get_device_list() g_ctx=%X count=%d\n", g_ctx, count);

		//dbg("find VID&PID ...\n");

		bool found = false;
		for (ssize_t i = 0; i < count; ++i) 
		{
			libusb_device_descriptor desc;
			libusb_get_device_descriptor(list[i], &desc);

			//dbg("find VID=%X PID=%X \n", desc.idVendor, desc.idProduct);
			if (desc.idVendor == VID && desc.idProduct == PID) 
			{
				//dbg("found device!\n");
				found = true;
				break;
			}
		}

		if(found == false)
			dbg("Not found any device!\n");

		libusb_free_device_list(list, 1);
		//dbg("get usb list done.\n");

		if (found && !device_connected)
		{
			//dbg("detect device plug-in.\n");
			if (open_device())
			{
				//dbg("open_device success.\n");
				device_connected = true;
			}
			else
			{
				//dbg("open_device() fail.\n");
			}
		}
		else if (!found && device_connected)
		{
			//dbg("detect device plug-out.\n");
			close_device();
			device_connected = false;
		}
		
		Sleep(100);
	}
}

HANDLE WINAPI UT_ConnectSensor()
{
	if (g_pkSensorDev[0] != NULL)
		return g_pkSensorDev[0];

	//dbg("UT_ConnectSensor()...\n");

	int result = 0;



#ifdef __ANDROID__
	dbg("libusb_set_option() ...\n");

	libusb_set_option(g_ctx, LIBUSB_OPTION_NO_DEVICE_DISCOVERY, NULL);
	if (g_ctx == nullptr)
	{
		result = libusb_init(&g_ctx);
		if (result < 0)
		{
			dbg("libusb_init() fail: %s\n", libusb_error_name(result));
			return (HANDLE)-1;
		}
		else
		{
			//dbg("libusb_init() ok\n");
		}

		result = libusb_wrap_sys_device(NULL, (intptr_t)g_android_fd, &g_handle);
		if (result != 0)
		{
			dbg("libusb_wrap_sys_device() fail: %s\n", libusb_error_name(result));
			libusb_exit(g_ctx);
			return 0;
		}
		else
		{
			//dbg("libusb_wrap_sys_device() ok\n");
		}

		//dbg("libusb_reset_device()\n");
		//libusb_reset_device(g_handle);

		//thread([]() {
		//	poll_usb();
		//	}).detach();

		static CSensorDevice sensor;

		DWORD dwRet = sensor.ConnectSensor(g_handle);
		if (dwRet == ERROR_SUCCESS)
			g_pkSensorDev[0] = &sensor;
	}

#else
	//if (g_ctx)
	//{
	//	libusb_exit(g_ctx);
	//	g_ctx = NULL;
	//}

	if(g_ctx == nullptr)
	{
		result = libusb_init(&g_ctx);
		if (result < 0)
		{
			dbg("libusb_init() fail: %s\n", libusb_error_name(result));
			return (HANDLE)-1;
		}

		thread([]() {
			poll_usb();
			}).detach();
	}
	
	//Sleep(10);
	UINT64 time = timeGetTime()+500;
	while((UINT64)timeGetTime() < time)
		if (g_pkSensorDev[0])
			break;

#endif

	//int timeout = 10;
	//while (timeout--)
	//open_device();
	//if (g_sensor.m_SensorPipe.usb_handle)
	//	return (HANDLE)&g_sensor;

	//dbg("g_sensor is %X\n", g_pkSensorDev[0]);
	return (HANDLE)g_pkSensorDev[0];

	//DWORD dwRet = sensor.ConnectSensor(g_handle);
	//if (dwRet == ERROR_SUCCESS)
	//	g_pkSensorDev[0] = &sensor;
	//else
	//	dbg(L"fail: sensor.ConnectSensor(pszDevicePath)\n");
	
	//SetLastError(dwRet);
	//return (HANDLE)&g_sensor;

}

DWORD WINAPI UT_CaptureImageEx(HANDLE hDevice, LPWAIT_SCAN_PARAMETER lpScanParam)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	DWORD ret = pSensor->CaptureImageEx((LPWAIT_SCAN_PARAMETER) lpScanParam);

	return ret;
}

DWORD WINAPI UT_ResetSensor(HANDLE hDevice, ULONG nTime)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->ResetSensor(nTime);
}

DWORD WINAPI UT_ResetMcuUsb(HANDLE hDevice)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	DWORD ret = pSensor->ResetMcuUsb();

	return ret;
}

DWORD WINAPI UT_GetSensorRegister(HANDLE hDevice, WORD wRegAddr, BYTE* lpValue)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	DWORD ret = pSensor->GetSensorRegister(wRegAddr, lpValue);

	return ret;
}

DWORD WINAPI UT_SetDcOffsetWithLoadEn(HANDLE hDevice, WORD wRegAddr, BYTE byValue)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	BYTE tmp = 0;
	pSensor->GetSensorRegister((wRegAddr&0xFF00)|0x19, &tmp);
	pSensor->SetSensorRegister((wRegAddr&0xFF00)|0x19, tmp|0x80);
	pSensor->SetSensorRegister(wRegAddr, byValue);
	Sleep(10);//10

	return pSensor->SetSensorRegister((wRegAddr&0xFF00)|0x19, tmp);
}

DWORD WINAPI UT_SetSensorRegister(HANDLE hDevice, WORD wRegAddr, BYTE byValue)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	if (wRegAddr >> 12 == 6)
	{
		wRegAddr = wRegAddr & 0x0FFF;
		DWORD result = 0;
		for (int dev = 1; dev < 5; dev++)
		{
			result = pSensor->SetSensorRegister(wRegAddr | dev << 12, byValue);
			if (result)
				return result;
		}

		return 0;
	}
	else
	{
		return pSensor->SetSensorRegister(wRegAddr, byValue);
	}
}

DWORD WINAPI UT_GetFingerState(HANDLE hDevice, BYTE *pbState)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->GetFingerState(pbState);
}

DWORD WINAPI UT_GetOTPData(HANDLE hDevice, WORD Addr, BYTE* lpValue)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->GetOTPData(Addr, lpValue);
}

DWORD WINAPI UT_SetOTPData(HANDLE hDevice, WORD Addr, BYTE byValue)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->SetOTPData(Addr, byValue);
}

DWORD WINAPI UT_GetChipID(HANDLE hDevice, PWORD pwChipID)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->GetChipID(pwChipID);
}

DWORD WINAPI UT_SetPower(HANDLE hDevice, BOOL power_down_en)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->SetPower(power_down_en);
}

DWORD WINAPI UT_GetUSBReaderInfo(HANDLE hDevice, BYTE *ReaderInfo, BYTE size)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->GetUSBReaderInfo(ReaderInfo, size);
}

DWORD WINAPI UT_SetBurnCode(HANDLE hDevice, BOOL is_reader)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->SetBurnCode();
}

DWORD WINAPI UT_GetGuid(HANDLE hDevice, BYTE* guid, int len)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	//BYTE guid[16];
	for (int i = 0; i < len; i++)
	{
		UT_GetOTPData(hDevice, (i/4+1)<<8 | (i%4), guid); 
		guid++;
	}

	return 0;
}

DWORD WINAPI UT_UserSpace(HANDLE hDevice, BOOL write, WORD offset, BYTE* data, int len)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	if (offset + len > 5 * 1024)
		return 1;

	return pSensor->UserSpace(write, offset, data, len);
}

DWORD WINAPI UT_ModulePara(HANDLE hDevice, BOOL is_write, WORD offset, BYTE* data, int len)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->ModulePara(is_write, offset, data, len);
}

DWORD WINAPI UT_LifeCheck(HANDLE hDevice, BOOL init)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->LifeCheck(init);
}

DWORD WINAPI UT_ClearFlag(HANDLE hDevice)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->ClearFlag(0);
}

DWORD WINAPI UT_WriteFlag(HANDLE hDevice)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->WriteFlag();
}

DWORD WINAPI UT_UpdateApp(HANDLE hDevice, string filePath)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->UpdateApp(filePath);
}

DWORD WINAPI UT_ModulePara_SN(HANDLE hDevice, BOOL is_write, BYTE* data, int len)
{
	int offset = MODULE_PARA_OFFSET_SN;
	if (len > MODULE_PARA_SIZE_SN)
		return ERROR_INVALID_DATA;

	return UT_ModulePara(hDevice, is_write, offset, data, len);
}

DWORD WINAPI UT_ModulePara_Brand(HANDLE hDevice, BOOL is_write, BYTE* data, int len)
{
	int offset = MODULE_PARA_OFFSET_BRAND;
	if (len > MODULE_PARA_SIZE_BRAND)
		return ERROR_INVALID_DATA;

	return UT_ModulePara(hDevice, is_write, offset, data, len);
}

DWORD WINAPI UT_ModulePara_Model(HANDLE hDevice, BOOL is_write, BYTE* data, int len)
{
	int offset = MODULE_PARA_OFFSET_MODEL;
	if (len > MODULE_PARA_SIZE_MODEL)
		return ERROR_INVALID_DATA;

	return UT_ModulePara(hDevice, is_write, offset, data, len);
}

DWORD WINAPI UT_ModulePara_Regs(HANDLE hDevice, BOOL is_write, BYTE* data, int len)
{
	int offset = MODULE_PARA_OFFSET_REGS_BLOCK;
	if (len > MODULE_PARA_SIZE_REGS_BLOCK)
		return ERROR_INVALID_DATA;

	BYTE chk_sum = 0;
	RegsBlock* block = (RegsBlock*)data;

	if (is_write)
	{
		block->ver = REGS_BLOCK_VER_NOW;

		for (int i = 0; i < MODULE_PARA_SIZE_REGS_BLOCK - 1; i++)
			chk_sum += data[i];

		data[MODULE_PARA_SIZE_REGS_BLOCK - 1] = ~chk_sum;
		return UT_ModulePara(hDevice, is_write, offset, data, len);
	}
	else
	{
		UT_ModulePara(hDevice, is_write, offset, data, len);
		for (int i = 0; i < MODULE_PARA_SIZE_REGS_BLOCK - 1; i++)
			chk_sum += data[i];

		if (data[MODULE_PARA_SIZE_REGS_BLOCK - 1] != (BYTE)~chk_sum)
		{
			ZeroMemory(data, len);
			return -1;
		}

		//if (block->ver != REGS_BLOCK_VER)
		//{
		//	ZeroMemory(data, len);
		//	return -1;
		//}
	}

	return 0;
}

DWORD WINAPI UT_SuspendDevice(HANDLE hDevice)
{
	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->SuspendDevice();
}
 
//>>> UART CMD
DWORD WINAPI UT_LedSpeech(HANDLE hDevice, BYTE led, BYTE led_status, BYTE speech, BOOL is_reader)
{
	if (is_reader == FALSE)
		return 0;

	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->LedSpeech(led, led_status, speech);
}

DWORD WINAPI UT_UsbSwtich(HANDLE hDevice, BYTE usb_sel, BOOL is_reader)
{
	if (is_reader == FALSE)
		return 0;

	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->UsbSwtich(usb_sel);
}

DWORD WINAPI UT_GetBattery(HANDLE hDevice, BYTE* battery, BOOL is_reader)
{
	if (is_reader == FALSE)
		return 0;

	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->GetBattery(battery);
}

DWORD WINAPI UT_SetSpeechLanguage(HANDLE hDevice, BYTE lang, BOOL is_reader)
{
	if (is_reader == FALSE)
		return 0;

	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->SetSpeechLanguage(lang);
}

DWORD WINAPI UT_SetSpeechVolume(HANDLE hDevice, BYTE vol, BOOL is_reader)
{
	if (is_reader == FALSE)
		return 0;

	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->SetSpeechVolume(vol);
}

DWORD WINAPI UT_Get_RB_Ver(HANDLE hDevice, BYTE* ver, BOOL is_reader)
{
	if (is_reader == FALSE)
		return 0;

	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->Get_RB_Ver(ver);
}

DWORD WINAPI UT_Get_RB_SN(HANDLE hDevice, BYTE* sn, BOOL is_reader)
{
	if (is_reader == FALSE)
		return 0;

	CSensorDevice* pSensor = TransferPointer(hDevice);
	if (!pSensor)
		return ERROR_INVALID_HANDLE;

	return pSensor->Get_RB_SN(sn);
}
//<<< UART CMD