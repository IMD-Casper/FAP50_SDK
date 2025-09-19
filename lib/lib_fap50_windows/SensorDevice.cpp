#include "imd_framework.h"
#include "SensorDevice.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define USE_STD

#ifndef USE_STD
#ifdef _WIN32 
static HANDLE usb_mutex = CreateMutex(NULL, FALSE, NULL); // NULL 表示匿名互斥锁
#endif
#endif 

static mutex g_mutex;

class CheckBusy 
{
public:
	BOOL is_busy;

//GPT 建議使用windows Mutex 會有更穩定的表現。
#ifdef USE_STD
	CheckBusy()
	{
		g_mutex.lock();
		//dbg("");//for multi-thread
		//for (int i = 0; i < 1000; i++) NOP(); //no function
		is_busy = TRUE;
	}
#else	
	CheckBusy()
	{
		DWORD dwWaitResult = WaitForSingleObject(usb_mutex, 5000); // 5000ms timeout
		if (dwWaitResult == WAIT_TIMEOUT)
		{
			dbg("thread timeout\n");
			return;
		}

		is_busy = TRUE;
	}
#endif

	~CheckBusy()
	{
		done();
	}

#ifdef USE_STD
	void done()
	{
		if (is_busy)
		{
			is_busy = FALSE;
			g_mutex.unlock();
			//dbg(""); //for multi-thread
			//for (int i = 0; i < 1000; i++) NOP(); //no function
		}
	}
#else
	void done()
	{
		if (is_busy)
		{
			is_busy = FALSE;
			ReleaseMutex(usb_mutex);
		}
	}
#endif

};

CSensorDevice::CSensorDevice(void)
{
	//m_SensorPipe.hDeviceHandle = NULL;
	m_SensorPipe.usb_handle = NULL;
	//m_hThread = NULL;
	//m_pszDevicePath = "";
	//ZeroMemory(&m_o, sizeof(OVERLAPPED));
	//m_o.hEvent = ::CreateEvent(NULL, TRUE, TRUE, NULL);
}

CSensorDevice::~CSensorDevice(void)
{
	if (m_SensorPipe.usb_handle)
	{
		//WinUsb_Free(m_SensorPipe.usb_handle);
		//libusb_close(m_SensorPipe.usb_handle);
		m_SensorPipe.usb_handle = NULL;
	}

	//if (m_SensorPipe.hDeviceHandle)
	//{
	//	CloseHandle(m_SensorPipe.hDeviceHandle);
	//	m_SensorPipe.hDeviceHandle = NULL;
	//}

	//if (m_hThread)
	//{
	//	::CloseHandle(m_hThread);
	//	m_hThread = NULL;
	//}	

	//if (m_o.hEvent)
	//{
	//	::CloseHandle(m_o.hEvent);
	//	m_o.hEvent = NULL;
	//}
}

int CSensorDevice::SendSetupPacket(PVOID pSetupPacket, PUCHAR  Buffer, ULONG  BufferLength, PULONG  LengthTransferred)
{
	int dwResult = ERROR_SUCCESS;
	WINUSB_SETUP_PACKET* pSetup = (WINUSB_SETUP_PACKET*)pSetupPacket;

	// 将 WinUSB 的 SETUP_PACKET 转换为 libusb 的控制传输参数
	uint8_t bmRequestType = pSetup->RequestType;
	uint8_t bRequest = pSetup->Request;
	uint16_t wValue = pSetup->Value;
	uint16_t wIndex = pSetup->Index;
	uint16_t wLength = static_cast<uint16_t>(BufferLength);

	int transferred = 0;

	// 使用 libusb_control_transfer 替代 WinUsb_ControlTransfer
	int ret = libusb_control_transfer(
		m_SensorPipe.usb_handle, // libusb 设备句柄
		bmRequestType,       // 请求类型
		bRequest,            // 请求码
		wValue,              // 请求值
		wIndex,              // 请求索引
		Buffer,              // 数据缓冲区
		wLength,             // 数据长度
		5000                 // 超时时间（单位：毫秒）
	);

	if (ret >= 0)
	{
		transferred = ret;
		*LengthTransferred = static_cast<ULONG>(transferred);
	}
	else
	{
		// 转换 libusb 错误为 Windows 风格错误代码
		dwResult = ERROR_GEN_FAILURE;
		switch (ret)
		{
		case LIBUSB_ERROR_TIMEOUT:
			dwResult = ERROR_TIMEOUT;
			dbg("SendSetupPacket() ERROR_TIMEOUT\n");
			break;
		case LIBUSB_ERROR_NO_DEVICE:
			dwResult = ERROR_DEVICE_NOT_AVAILABLE;
			dbg("SendSetupPacket() ERROR_DEVICE_NOT_AVAILABLE\n");
			break;
		case LIBUSB_ERROR_ACCESS:
			dwResult = ERROR_ACCESS_DENIED;
			dbg("SendSetupPacket() ERROR_ACCESS_DENIED\n");
			break;
		case LIBUSB_ERROR_PIPE:
			dwResult = ERROR_BAD_PIPE;
			dbg("SendSetupPacket() ERROR_BAD_PIPE\n");
			break;
		default:
			dwResult = ERROR_GEN_FAILURE;
			break;
		}
	}

	return dwResult;
}

int findEndpoint(libusb_device_handle* handle, BYTE& in_ep, BYTE& out_ep)
{
	libusb_device* device = libusb_get_device(handle);
	libusb_config_descriptor* config = nullptr;

	// 获取设备描述符
	int ret = libusb_get_active_config_descriptor(device, &config);
	if (ret != LIBUSB_SUCCESS) 
		return -1; // 获取配置描述符失败
	
	//dbg("find all descs count: %d\n", config->bNumInterfaces);

	for (int i = 0; i < config->bNumInterfaces; ++i) 
	{
		libusb_interface_descriptor*  interface_desc = (libusb_interface_descriptor*) &config->interface[i].altsetting[0];
		ret = libusb_claim_interface(handle, i); // 0 是接口号，替换为你的实际接口号
		if (ret != LIBUSB_SUCCESS) 
		{
			dbg("libusb_claim_interface() fail: %s\n", libusb_error_name(ret));
			return ret;
		}

		//dbg("interface number: %d\n", i);

		for (int j = 0; j < interface_desc->bNumEndpoints; ++j) 
		{
			libusb_endpoint_descriptor*  endpoint_desc = (libusb_endpoint_descriptor*) &interface_desc->endpoint[j];

			if ((endpoint_desc->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK)
			{
				// 发现 OUT 端点，返回端点地址
				if ((endpoint_desc->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) 
					out_ep = endpoint_desc->bEndpointAddress;

				// 发现 OUT 端点，返回端点地址
				if ((endpoint_desc->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) 
					in_ep = endpoint_desc->bEndpointAddress;
			}
		}
	}

	return 0; 
}

DWORD CSensorDevice::ConnectSensor(
	HANDLE handle //libusb_device_handle*
)
{
	//HANDLE	hFile;
	DWORD	dwResult = ERROR_SUCCESS;

	// Open the device
	//hFile = CreateFile(
	//	pszDevicePath,							//__in     LPCWSTR lpFileName,
	//	GENERIC_READ | GENERIC_WRITE, 			//__in     DWORD dwDesiredAccess,
	//	0,//FILE_SHARE_READ | FILE_SHARE_WRITE, 	//__in     DWORD dwShareMode,
	//	NULL, 									//__in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	//	OPEN_EXISTING, 							//__in     DWORD dwCreationDisposition,
	//	FILE_FLAG_OVERLAPPED,			//__in     DWORD dwFlagsAndAttributes,
	//	NULL);									//__in_opt HANDLE hTemplateFile

 //   if (hFile == INVALID_HANDLE_VALUE)
 //       return GetLastError();

	//if (m_SensorPipe.hDeviceHandle)
	//{
	//	CloseHandle(m_SensorPipe.hDeviceHandle);
	//	m_SensorPipe.hDeviceHandle = NULL;
	//}
	//if (m_SensorPipe.usb_handle)
	//{
	//	//CloseHandle(m_SensorPipe.usb_handle);
	//	m_SensorPipe.usb_handle = NULL;
	//}
	
	//if (m_SensorPipe.usb_handle)
	//{
	//	libusb_close(m_SensorPipe.usb_handle);
	//	m_SensorPipe.usb_handle = NULL;
	//}
	
	findEndpoint((libusb_device_handle*)handle, m_SensorPipe.PipeBulkIn, m_SensorPipe.PipeBulkOut);
	m_SensorPipe.usb_handle = (libusb_device_handle*)handle;
	//dbg("USB pipe number. in:%d out:% \n", m_SensorPipe.PipeBulkIn, m_SensorPipe.PipeBulkOut);

	//if (WinUsb_Initialize(hFile, &m_SensorPipe.usb_handle))
	//{
	//	m_SensorPipe.hDeviceHandle = hFile;
	//	QueryDeviceEndpoints();

	//	//FlushReadPipe();
	//	//WinUsb_ResetPipe(m_SensorPipe.usb_handle, m_SensorPipe.PipeBulkIn);
	//	//WinUsb_ResetPipe(m_SensorPipe.usb_handle, m_SensorPipe.PipeBulkOut);

	//	//WINUSB_SETUP_PACKET SetupPacket;
	//	ULONG cbSent = 0;
	//	//READER_INFORMATION info;

	//	////Create the setup packet
	//	//SetupPacket.RequestType = 0xC0;
	//	//SetupPacket.Request = CMD_GET_USBREADER_MODEL;
	//	//SetupPacket.Value = 0;
	//	//SetupPacket.Index = 0; 
	//	//SetupPacket.Length = sizeof(READER_INFORMATION);
	//	//dwResult = SendSetupPacket(&SetupPacket, (PUCHAR)&info, SetupPacket.Length, &cbSent);
 //	//	if (dwResult != ERROR_SUCCESS || info.SensorModel != USBREADER_MODEL_ID)
	//	//{
	//	//	WinUsb_Free(m_SensorPipe.usb_handle);
	//	//}
	//	//else
	//	{
	//		m_pszDevicePath = pszDevicePath;
	//		//dbg(L"new m_pszDevicePath\r\n");//for debug
	//	}
	//}
	//else
	//{
	//	dwResult = GetLastError();
	//}

	//if (dwResult != ERROR_SUCCESS)
	//{
	//	CloseHandle(hFile);
	//	m_SensorPipe.hDeviceHandle = NULL;
	//	m_SensorPipe.usb_handle = NULL;
	//}

	return dwResult;
}

static int transfer_done = 0;
static void LIBUSB_CALL bulk_callback(struct libusb_transfer* transfer)
{
	if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
		printf("接收完成 %d bytes\n", transfer->actual_length);
	}
	else {
		printf("錯誤狀態: %d\n", transfer->status);
	}

	transfer_done = 1;
	libusb_free_transfer(transfer);
}

int CSensorDevice::CaptureImageEx(LPWAIT_SCAN_PARAMETER lpScanParam)
{
	CheckBusy check_busy;

	//auto now = timeGetTime();

	//DWORD hr = S_OK;
	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	if (lpScanParam == NULL || lpScanParam->raw == NULL)
		return ERROR_INVALID_PARAMETER;

	//OVERLAPPED overlap{};
	//DWORD dwReturn = ERROR_SUCCESS;
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;
	DWORD dwReturnLen = 0;
	int ret;
	DWORD readSize;

	static int cmd_cnt = 0;
	//Create the setup packet
	lpScanParam->nShiftPixelX = 1;
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_CAPTURE_IMAGE_ONESHOT;
	SetupPacket.Value = cmd_cnt++; //lpScanParam->nShiftPixelX;	// Shift pixel num or Ignore Num for simulated WOI function
	SetupPacket.Index = 0 | (lpScanParam->hw_scan_mode) << 8;	// Line scan delay time & hw_capture_mode
	ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
	if (ret != ERROR_SUCCESS)
	{
		dbg("CaptureImageEx(): SendSetupPacket() return is %d(%XH) fail.\n", ret, ret);
		goto return_hr;
	}

	if (lpScanParam->nShiftPixelX == 0)
		lpScanParam->nShiftPixelX = 1;
	readSize = lpScanParam->raw_size / lpScanParam->nShiftPixelX;

	{
		int transferred = 0;
#if 1

#define IMG_SIZE readSize//(1600*1024)
#define XFER_CNT 1
#define PKG_SIZE (IMG_SIZE/XFER_CNT)

		//dbg("start capture...\n");

		int cnt = 0;
		for( ;cnt< XFER_CNT; cnt++)
		{
			ret = libusb_bulk_transfer(
				m_SensorPipe.usb_handle,         // libusb 设备句柄
				m_SensorPipe.PipeBulkIn,     // Bulk IN 端点地址
				lpScanParam->raw+ cnt* PKG_SIZE, // 数据缓冲区
				PKG_SIZE,//readSize                    // 要读取的字节数
				&transferred,                // 实际传输的字节数
				750                          // 超时时间（单位：毫秒）
			);

			if (ret != LIBUSB_SUCCESS || transferred!= PKG_SIZE)
			{
				if (ret != LIBUSB_SUCCESS)
					dbg("libusb_bulk_transfer() return is %X != LIBUSB_SUCCESS\n", ret);

				if (transferred != PKG_SIZE)
					dbg("transferred(%d) != PKG_SIZE(%d)\n", transferred, PKG_SIZE);

				break;
			}
		}

		if(cnt == XFER_CNT)
			transferred = IMG_SIZE;

#else
		transfer_done = 0;
		libusb_transfer* transfer = libusb_alloc_transfer(0);
		if (!transfer) {
			dbg("無法分配 transfer\n");
			return -1;
		}

		libusb_fill_bulk_transfer(
			transfer,
			m_SensorPipe.usb_handle,
			m_SensorPipe.PipeBulkIn,
			lpScanParam->raw,   // 你的 buffer
			readSize,           // 你要讀取的大小
			bulk_callback,      // 傳輸完成後呼叫
			NULL,               // 可以放 context
			500                 // timeout 毫秒
		);

		int ret = libusb_submit_transfer(transfer);
		if (ret < 0) {
			dbg("提交失敗: %d\n", ret);
			libusb_free_transfer(transfer);
			return -1;
		}

		while (!transfer_done) {
			libusb_handle_events(NULL);  // 等 callback 被呼叫
		}

		ret = LIBUSB_SUCCESS;
#endif

		if (ret == LIBUSB_SUCCESS)
		{
			// 读取成功，返回读取的字节数
			dwReturnLen = static_cast<DWORD>(transferred);
		}
		else
		{
			// 错误处理
			dwReturnLen = 0;
			switch (ret)
			{
			case LIBUSB_ERROR_TIMEOUT:
				ret = WAIT_TIMEOUT;
				dbg("WAIT_TIMEOUT\n");

				// 如果需要，清除或复位管道
				libusb_clear_halt(m_SensorPipe.usb_handle, m_SensorPipe.PipeBulkIn);
				ResetMcuUsb(); // 自定义复位函数
				break;

			case LIBUSB_ERROR_PIPE:
				ret = ERROR_BAD_PIPE;
				dbg("Pipe error\n");

				// 复位管道
				libusb_clear_halt(m_SensorPipe.usb_handle, m_SensorPipe.PipeBulkIn);
				break;

			case LIBUSB_ERROR_NO_DEVICE:
				ret = ERROR_DEVICE_NOT_AVAILABLE;
				dbg("Device not available\n");
				break;

			default:
				ret = ERROR_GEN_FAILURE;
				dbg("General error\n");
				break;
			}
		}
	}

	//dbg("capture time is %dms\n", timeGetTime() - now);
	ret = (dwReturnLen==readSize) ? ERROR_SUCCESS:ret;

return_hr:
	return ret;
}

DWORD CSensorDevice::ResetSensor(ULONG nTime)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_RESET_SENSOR;

	DWORD ret= SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

	return ret;
}

DWORD CSensorDevice::ResetMcuUsb(void)
{
	CheckBusy check_busy;

	dbg("ResetMcuUsb() by module\n");

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

#if(1)
	{
		WINUSB_SETUP_PACKET SetupPacket{};
		ULONG cbSent = 0;
		SetupPacket.RequestType = 0x40;
		SetupPacket.Request = CMD_RESET_MCU_AND_USB;
		SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

		Sleep(550);
	}
#else
	{
		bool ret = WinUsb_ResetPipe(m_SensorPipe.hWinUSBHandle, m_SensorPipe.PipeBulkIn);
	}
#endif

	return 0;
}

DWORD CSensorDevice::GetSensorRegister(WORD wRegAddr, BYTE* lpValue)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	BYTE DevReturn[4];
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_GET_REGISTERS;
	SetupPacket.Index = wRegAddr; 
	SetupPacket.Length = 1;
	DWORD ret = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);

    if (ret == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && lpValue != NULL)
			*lpValue = DevReturn[0];

		return ERROR_SUCCESS;
	}
	else
	{
		cbSent = 0;
	}

	return ret;
}

DWORD CSensorDevice::SetSensorRegister(WORD wRegAddr, BYTE byValue)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;


	//if ((wRegAddr & 0xFF) == 0x1B)
	//	do {} while (0);

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_REGISTERS;
	SetupPacket.Index = wRegAddr; 
	SetupPacket.Value = byValue;
	DWORD ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
	return ret;
}

DWORD CSensorDevice::SetFingerDet(BOOL blEnable)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = blEnable == TRUE ? CMD_ENABLE_FIG_DET : CMD_DISABLE_FIG_DET;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::GetFingerState(BYTE *pbState)
{
	CheckBusy check_busy;
	//dbg(L"GetFingerState()\n");

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	DWORD img_status=0;
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_QUERY_FINGER;
	SetupPacket.Length = 4;
	
	DWORD dwReturn = SendSetupPacket(&SetupPacket, (BYTE*)&img_status, SetupPacket.Length, &cbSent);
	if (dwReturn == ERROR_SUCCESS)
	{
		memcpy(pbState, &img_status, sizeof(img_status));
		return ERROR_SUCCESS;
	}

	return ERROR_INVALID_FUNCTION;
}

DWORD CSensorDevice::SetSPISpeed(BYTE bySpeed)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_SPI_SPEED;
	SetupPacket.Value = bySpeed;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::GetOTPData(WORD Addr, BYTE* lpValue)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	BYTE	DevReturn[4];
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_OTP_READ;
	SetupPacket.Index = Addr;
	SetupPacket.Length = 1;
	DWORD dwReturn = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);
    if (dwReturn == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && lpValue != NULL)
			*lpValue = DevReturn[0];

		return ERROR_SUCCESS;
	}
	else
	{
		int i = dwReturn;
	}

	return dwReturn;
}

DWORD CSensorDevice::SetOTPData(WORD Addr, BYTE byValue)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_OTP_WRITE;
	SetupPacket.Index = Addr;
	SetupPacket.Value = byValue;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::GetChipID(WORD *wChipID)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	BYTE	DevReturn[2];
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_CHIPID_READ;
	SetupPacket.Length = 2;
	DWORD dwReturn = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);
    if (dwReturn == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && wChipID != NULL)
		{
			*wChipID = DevReturn[0] | DevReturn[1] << 8;

			if (*wChipID == 0)//for debug
				*wChipID = 0;
		}

		return ERROR_SUCCESS;
	}
	else//for debug
	{
		//int i = dwReturn;
		//DebugOut(L"GetChipID() return is wrong.\n");
	}

	return dwReturn;
}

DWORD CSensorDevice::SelectChipNo(BYTE byNumber)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_CHIP_NO;
	SetupPacket.Index = byNumber; 

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::SetRelayPin(BYTE byStatus)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_RELAY_PIN;
	SetupPacket.Index = byStatus; 

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::SetVPPPin(BYTE byStatus)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_VPP_EN;
	SetupPacket.Value = byStatus;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::SetPower(BOOL power_down_en)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_POWER;
	SetupPacket.Index = power_down_en;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::SetGain(BYTE byStatus)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_GAIN;
	SetupPacket.Index = byStatus; 

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::WOIReset( void )
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_WOI_RESET;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::INTEnable(BYTE bNum, BYTE bStatus)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_EN_INT;
	SetupPacket.Index = bNum; 
	SetupPacket.Value = bStatus;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::ReadPortState(BYTE bNum, BYTE *pbState)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	BYTE	DevReturn[2]{};
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_READ_PORT_STATE;
	SetupPacket.Index = bNum; 
	SetupPacket.Length = 2;
	DWORD dwReturn = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);
    if (dwReturn == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && pbState != NULL)
		{
			*pbState++ = DevReturn[0];
			*pbState++ = DevReturn[1];
		}
		return ERROR_SUCCESS;
	}

	return dwReturn;
}

DWORD CSensorDevice::SWReset( void )
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SW_RESET;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::GetUSBReaderInfo(BYTE *ReaderInfo, BYTE size)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
        return ERROR_INVALID_HANDLE;

	DWORD dwResult = ERROR_SUCCESS;
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_GET_USBREADER_MODEL;
	SetupPacket.Length = size;

	//dbg("GetUSBReaderInfo()...\n");
	dwResult = SendSetupPacket(&SetupPacket, ReaderInfo, SetupPacket.Length, &cbSent);

	return dwResult;
}

DWORD CSensorDevice::GetSystemStatus(void *err)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	DWORD dwReturn = ERROR_SUCCESS;
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;//0x40;
	//SetupPacket.Request = CMD_CAPTURE_IMAGE;
	SetupPacket.Request = CMD_SYSTEM_STATUS;
	SetupPacket.Length = 64;
	DWORD ret = SendSetupPacket(&SetupPacket, (BYTE*)err, SetupPacket.Length, &cbSent);
	if (dwReturn != ERROR_SUCCESS)
		dwReturn = ret;

	return dwReturn;
}

DWORD CSensorDevice::EntryISPMode()
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	DWORD dwReturn = ERROR_INVALID_FUNCTION;// ERROR_SUCCESS;
	ULONG cbSent = 0;
	
	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_ENTRY_ISP_MODE;
	SetupPacket.Value = 0x4444;	// Command delay time
	SetupPacket.Index = 0x5678;	// Line scan delay time
	dwReturn = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
	
	return dwReturn;
}

// Change Multichip or Single board setting
DWORD CSensorDevice::SetMultiChipBoard(BYTE byMultichip)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_MULTICHIP_BOARD;
	SetupPacket.Value = byMultichip;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

// Change USB Channel setting
DWORD CSensorDevice::SetUsbChannel(BYTE bStatus)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_USB_NO;
	SetupPacket.Value = bStatus;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::SetVDD_Pin(BYTE byStatus)
{
	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_VDD_EN;
	SetupPacket.Value = byStatus;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::SetVDDIO_Pin(BYTE byStatus)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_VDDIO_EN;
	SetupPacket.Value = byStatus;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::NoiseCancellationInit(BYTE* buf, int nSize)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	for (int i = 0; i < nSize; i++)
	{
		//Create the setup packet
		SetupPacket.RequestType = 0x40;
		SetupPacket.Request = CMD_NOISE_CANCELLATION_INIT;
		SetupPacket.Index = i;
		SetupPacket.Value = buf[i];

		SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
	}
	return 0;
}

DWORD CSensorDevice::Set_GPIOx_Val(BYTE bGPIONo, BYTE bBit, BYTE bVal)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_PORT;
	SetupPacket.Index = bGPIONo;
	SetupPacket.Value = (bBit << 8) | bVal; // Hi Byte: bBit, Lo Byte: bVal

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::Get_GPIOx_Val(BYTE bGPIONo, BYTE *pbState)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	BYTE	DevReturn[8];
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_GET_PORT;
	SetupPacket.Index = bGPIONo;
	SetupPacket.Length = 4;
	DWORD dwReturn = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);
	if (dwReturn == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && pbState != NULL)
		{
			pbState[3] = DevReturn[3];
			pbState[2] = DevReturn[2];
			pbState[1] = DevReturn[1];
			pbState[0] = DevReturn[0];
		}
		return ERROR_SUCCESS;
	}

	return dwReturn;
}

DWORD CSensorDevice::Get_BtnStatus(BYTE *bSelectFunc)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	BYTE	DevReturn[4]{};
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_GET_BUTTON_STATUS;
	SetupPacket.Length = 4;
	DWORD dwReturn = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);
	if (dwReturn == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && bSelectFunc != NULL)
		{
			*bSelectFunc++ = DevReturn[0];
			*bSelectFunc++ = DevReturn[1];
		}
		return ERROR_SUCCESS;
	}

	return dwReturn;
}

DWORD CSensorDevice::Set_MeasureInfo(BYTE bSelectFunc, BYTE bItemNo, BYTE bVal)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

//	uByte = SetupPacket.wValue.WB.H;  // led item (total 4)
//	uVal = SetupPacket.wValue.WB.L;  //  pass=1/fail=0, 
	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_LED_STATUS;
	SetupPacket.Index = bSelectFunc;
	SetupPacket.Value = (bItemNo << 8) | bVal; // Hi Byte: bBit, Lo Byte: bVal

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::TestGPIO(DWORD *dwResult)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	//BYTE	DevReturn[4]{};
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_SF125_GPIO_TEST;
	SetupPacket.Length = 4;

	DWORD dwReturn = SendSetupPacket(&SetupPacket, (PBYTE)dwResult, SetupPacket.Length, &cbSent);
	/*if (dwReturn == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && bSelectFunc != NULL)
		{
			*bSelectFunc++ = DevReturn[0];
			*bSelectFunc++ = DevReturn[1];
		}
		return ERROR_SUCCESS;
	}*/

	//Sleep(20);//must do.

	return dwReturn;
}

DWORD CSensorDevice::SetSensorPackageType(BYTE bType)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_PACKAGE_TYPE;
	SetupPacket.Value = bType;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::Set_DiffBP_Val(BYTE bBP_Value)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_BPCHECK_TYPE;
	SetupPacket.Value = bBP_Value;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::GetADCCurrentST(BYTE* lpValue)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	BYTE	DevReturn[4];
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_GET_ADCVALUE_ST;
	SetupPacket.Length = 2;
	DWORD dwReturn = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);
	if (dwReturn == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && lpValue != NULL)
		{
			lpValue[0] = DevReturn[1];
			lpValue[1] = DevReturn[0];
		}
		return ERROR_SUCCESS;
	}
	else
	{
		cbSent = 0;
	}

	return dwReturn;
}

DWORD CSensorDevice::GetADCCurrentOP(BYTE* lpValue)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	BYTE	DevReturn[4];
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_GET_ADCVALUE_OP;
	SetupPacket.Length = 2;
	DWORD dwReturn = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);
	if (dwReturn == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && lpValue != NULL)
		{
			lpValue[0] = DevReturn[1];
			lpValue[1] = DevReturn[0];
		}
		return ERROR_SUCCESS;
	}
	else
	{
		cbSent = 0;
	}

	return dwReturn;
}

DWORD CSensorDevice::GetADCCurrentArray(BYTE *lpValueArray)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	BYTE	DevReturn[12];
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;
	int i, j;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_GET_ADCVALUEARRAY;
	SetupPacket.Length = 12;
	DWORD dwReturn = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);
	if (dwReturn == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && lpValueArray != NULL)
		{
			for (i = 0, j = 0; i <= 10; i+=2)
			{
				lpValueArray[j++] = DevReturn[i + 1];
				lpValueArray[j++] = DevReturn[i];
			}
		}
		return ERROR_SUCCESS;
	}
	else
	{
		cbSent = 0;
	}

	return dwReturn;
}

DWORD CSensorDevice::SetTOUCH_Pin(BYTE bStatus)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_TOUCH_EN;
	SetupPacket.Value = bStatus;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::ReadDetCTLState(BYTE *pbState)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	BYTE	DevReturn;
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_GET_DETCTL;
	SetupPacket.Length = 1;
	DWORD dwReturn = SendSetupPacket(&SetupPacket, &DevReturn, SetupPacket.Length, &cbSent);
	if (dwReturn == ERROR_SUCCESS)
	{
	    *pbState = DevReturn;
		return ERROR_SUCCESS;
	}

	return dwReturn;
}

DWORD CSensorDevice::SetSimulationFGPin(BYTE bStatus)
{
	CheckBusy check_busy;
	
	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_SimulationFG_PIN;
	SetupPacket.Value = bStatus;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::GetFrameTime(WORD *wFrameTime)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	BYTE	DevReturn[4];
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_GET_FRAMETIME;
	SetupPacket.Length = 4;
	DWORD dwReturn = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);
	if (dwReturn == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && wFrameTime != NULL)
		{
			wFrameTime[0] = DevReturn[0] | DevReturn[1] << 8;
			wFrameTime[1] = DevReturn[2] | DevReturn[3] << 8;
		}

		return ERROR_SUCCESS;
	}
	else//for debug
	{
		//int i = dwReturn;
		//DebugOut(L"GetFrameTime() return is wrong.\n");
	}

	return dwReturn;
}

DWORD CSensorDevice::SetPanelStatus(BYTE bStatus)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_PANEL_STATUS;
	SetupPacket.Value = bStatus;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::SelectGlassType(BYTE byNumber)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_GLASS_TYPE;
	SetupPacket.Index = byNumber;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::SetCoverStampFlag(BOOL bStatus)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_SET_COVERSTAMP_FLAG;
	SetupPacket.Value = bStatus;

	return SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
}

DWORD CSensorDevice::GetSensorReadyFlag(BOOL* pbState)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	BYTE DevReturn;
	DWORD dwReturn = ERROR_SUCCESS;
	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_SENSOR_READY;
	SetupPacket.Length = 1;
	dwReturn = SendSetupPacket(&SetupPacket, &DevReturn, SetupPacket.Length, &cbSent);
	if (dwReturn == ERROR_SUCCESS)
		*pbState = (BOOL)DevReturn;

	return dwReturn;
}

// Suspend USB 設備
int CSensorDevice::SuspendDevice()
{
	dbg("CSensorDevice::SuspendDevice()\n");
	//libusb_release_interface(m_SensorPipe.usb_handle, interface_number);
	return 0;
#if 0
	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	libusb_device_handle* handle = m_SensorPipe.usb_handle;


	// 發送控制傳輸設置遠程喚醒
	int result = libusb_control_transfer(
		handle,
		LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
		LIBUSB_REQUEST_SET_FEATURE,
		0x0002, // FEATURE_SELECTOR_DEVICE_REMOTE_WAKEUP
		0,      // Index (set to 0 for device-level operation)
		nullptr,
		0,
		1000    // Timeout in milliseconds
	);

	if (result < 0) {
		std::cerr << "Failed to set remote wakeup feature: " << libusb_error_name(result) << std::endl;
		return false;
	}

	// 模擬總線閒置，停止所有活動（SOF 包）
	result = libusb_suspend_device(handle);
	if (result < 0) {
		std::cerr << "Failed to suspend device: " << libusb_error_name(result) << std::endl;
		return false;
	}

	return result;
#endif
}

DWORD CSensorDevice::LedSpeech(BYTE led, BYTE led_status, BYTE speech)
{
	CheckBusy check_busy;

	//static uint64 t = 0;// timeGetTime();
	//uint64 now = (uint64)timeGetTime();
	//if (abs(now - t) < 5)
	//	return 0;

	//t = (uint64)timeGetTime();

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_UART_TX_PTL;

	PBYTE data = (PBYTE)&SetupPacket.Value;
	data[0] = UART_CMD_INST_LED_SPEECH;
	data[1] = led;
	data[2] = led_status;
	data[3] = speech;
	
	DWORD ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

	return ret;
}

DWORD CSensorDevice::UsbSwtich(BYTE usb_sel)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_UART_TX_PTL;

	PBYTE data = (PBYTE)&SetupPacket.Value;
	data[0] = UART_CMD_INST_USB_SWITCH;
	data[1] = usb_sel;
	data[2] = 0;
	data[3] = 0;

	DWORD ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

	//check_busy.done();
	if(usb_sel != 0)
		Sleep(4000);

	return ret;
}

DWORD CSensorDevice::GetBattery(BYTE* battery)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_UART_TX_PTL;

	PBYTE data = (PBYTE)&SetupPacket.Value;
	data[0] = UART_CMD_INST_POWER_DET;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	SetupPacket.Length = 1;

	BYTE DevReturn[2];
	DWORD ret = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);

	if (ret == ERROR_SUCCESS)
	{
		if (cbSent >= 1 && battery != NULL)
			*battery = DevReturn[0];

		return ERROR_SUCCESS;
	}

	return ret;
}

DWORD CSensorDevice::SetSpeechLanguage(BYTE lang)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_UART_TX_PTL;

	PBYTE data = (PBYTE)&SetupPacket.Value;
	data[0] = UART_CMD_INST_SPEECH_LANG;
	data[1] = lang;
	data[2] = 0;
	data[3] = 0;

	DWORD ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

	return ret;
}

DWORD CSensorDevice::SetSpeechVolume(BYTE volume)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_UART_TX_PTL;

	PBYTE data = (PBYTE)&SetupPacket.Value;
	data[0] = UART_CMD_INST_SPEECH_VOL;
	data[1] = volume;
	data[2] = 0;
	data[3] = 0;

	DWORD ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

	return ret;
}

DWORD CSensorDevice::Get_RB_Ver(BYTE *ver)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_UART_TX_PTL;

	PBYTE data = (PBYTE)&SetupPacket.Value;
	data[0] = UART_CMD_INST_GET_VER;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	SetupPacket.Length = 1;
	BYTE DevReturn[1];
	DWORD ret = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);
	if (ret == 0)
		*ver = DevReturn[0];

	return ret;
}

DWORD CSensorDevice::Get_RB_SN(BYTE* sn)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0xC0;
	SetupPacket.Request = CMD_UART_TX_PTL;

	PBYTE data = (PBYTE)&SetupPacket.Value;
	data[0] = UART_CMD_INST_GET_SN;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	SetupPacket.Length = 32;
	BYTE DevReturn[32]{};
	DWORD ret = SendSetupPacket(&SetupPacket, DevReturn, SetupPacket.Length, &cbSent);
	if (ret == 0)
		memcpy(sn, DevReturn, 32);

	return ret;
}

DWORD CSensorDevice::SetBurnCode()
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_UART_TX_PTL;

	PBYTE data = (PBYTE)&SetupPacket.Value;
	data[0] = UART_CMD_INST_BURN_CODE;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;

	DWORD ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

	//check_busy.done();
	//Sleep(4000);

	return ret;
}

int CSensorDevice::UserSpace(BOOL is_write, WORD offset, BYTE* data, int len)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

//#define PACKAGE_BASE 64
//	BYTE data[5 * 1024];
//	int len = _len / PACKAGE_BASE;
//	if (_len % PACKAGE_BASE)
//		len++;// = len + 1;
//	len *= PACKAGE_BASE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_USER_SPACE;
	SetupPacket.Index = is_write <<15 | offset;
	SetupPacket.Value = len;

	int ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

	{
		int transferred = 0;

		if (is_write)
		{
			ret = libusb_bulk_transfer(
				m_SensorPipe.usb_handle,         // libusb 设备句柄
				m_SensorPipe.PipeBulkOut,     // Bulk out 端点地址
				data, // 数据缓冲区
				len,                    // 要读取的字节数
				&transferred,                // 实际传输的字节数
				1500                          // 超时时间（单位：毫秒）
			);

			Sleep(10); //delay for mcu program flash.
		}
		else
		{
			ret = libusb_bulk_transfer(
				m_SensorPipe.usb_handle,         // libusb 设备句柄
				m_SensorPipe.PipeBulkIn,     // Bulk out 端点地址
				data, // 数据缓冲区
				len,                    // 要读取的字节数
				&transferred,                // 实际传输的字节数
				1500                          // 超时时间（单位：毫秒）
			);

		}

		if (ret != LIBUSB_SUCCESS)
		{
			// 错误处理
			switch (ret)
			{
			case LIBUSB_ERROR_TIMEOUT:
				ret = WAIT_TIMEOUT;
				dbg("WAIT_TIMEOUT\n");

				// 如果需要，清除或复位管道
				libusb_clear_halt(m_SensorPipe.usb_handle, m_SensorPipe.PipeBulkIn);
				//ResetMcuUsb(); // 自定义复位函数
				break;

			case LIBUSB_ERROR_PIPE:
				ret = ERROR_BAD_PIPE;
				dbg("Pipe error\n");

				// 复位管道
				libusb_clear_halt(m_SensorPipe.usb_handle, m_SensorPipe.PipeBulkIn);
				break;

			case LIBUSB_ERROR_NO_DEVICE:
				ret = ERROR_DEVICE_NOT_AVAILABLE;
				dbg("Device not available\n");
				break;

			default:
				ret = ERROR_GEN_FAILURE;
				dbg("General error\n");
				break;
			}
		}
	}

//return_hr:
	check_busy.done();
	vector<WORD> status(32);
	GetSystemStatus(status.data());
	if (ret == 0 && status[0])
		ret = status[0];

	return ret;
}

int CSensorDevice::ModulePara(BOOL is_write, WORD offset, BYTE* data, int len)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;
	WORD check_sum = 0;
	for (int i = 0; i < len; i++)
		check_sum += data[i];

	//Create the setup packet
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_MODULE_PARA;
	SetupPacket.Index = (is_write <<15) | offset;
	SetupPacket.Value = len;
	int ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);
	if (ret)
		return ret;

	{
		int really = 0;

		if (is_write)
		{
			ret = libusb_bulk_transfer(
				m_SensorPipe.usb_handle,         // libusb 设备句柄
				m_SensorPipe.PipeBulkOut,     // Bulk IN 端点地址
				data, // 数据缓冲区
				len,                    // 要读取的字节数
				&really,                // 实际传输的字节数
				3000                          // 超时时间（单位：毫秒）
			);

			Sleep(400); //delay for mcu program flash.
		}
		else
		{
			ret = libusb_bulk_transfer(
				m_SensorPipe.usb_handle,         // libusb 设备句柄
				m_SensorPipe.PipeBulkIn,     // Bulk IN 端点地址
				data, // 数据缓冲区
				len,                    // 要读取的字节数
				&really,                // 实际传输的字节数
				3000                          // 超时时间（单位：毫秒）
			);
		}

		if (ret == LIBUSB_SUCCESS)
		{
			// 读取成功，返回读取的字节数
		}
		else
		{
			// 错误处理
			switch (ret)
			{
			case LIBUSB_ERROR_TIMEOUT:
				ret = WAIT_TIMEOUT;
				dbg("WAIT_TIMEOUT\n");

				// 如果需要，清除或复位管道
				libusb_clear_halt(m_SensorPipe.usb_handle, m_SensorPipe.PipeBulkIn);
				//ResetMcuUsb(); // 自定义复位函数
				break;

			case LIBUSB_ERROR_PIPE:
				ret = ERROR_BAD_PIPE;
				dbg("Pipe error\n");

				// 复位管道
				libusb_clear_halt(m_SensorPipe.usb_handle, m_SensorPipe.PipeBulkIn);
				break;

			case LIBUSB_ERROR_NOT_FOUND:
			case LIBUSB_ERROR_NO_DEVICE:
				ret = ERROR_DEVICE_NOT_AVAILABLE;
				dbg("Device not available (ModulePara)\n");
				break;

			default:
				ret = ERROR_GEN_FAILURE;
				dbg("General error\n");
				break;
			}
		}
	}

//return_hr:
	check_busy.done();
	vector<WORD> status(32);
	GetSystemStatus(status.data());
	if (ret == 0 && status[0])
		ret = status[0];

	return ret;
}

DWORD CSensorDevice::LifeCheck(BOOL init)
{
	//return 0;//debug
	CheckBusy check_busy;

	static int live_code = 0xFF;
	live_code++;
	//dbg("LifeCheck init=%d live_code=%d\n", init, live_code);
	DWORD ret = 0;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	ULONG cbSent = 0;
	WINUSB_SETUP_PACKET SetupPacket{};
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_LIFE_CHECK;
	SetupPacket.Index = init;
	ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

	return ret;
}

DWORD CSensorDevice::UpdateApp(string filePath)
{
	CheckBusy check_busy;

	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

    std::ifstream file(filePath, std::ios::binary | std::ios::ate); // 打开为二进制模式，定位到文件末尾以获取长度
    if (!file.is_open()) {
        return IMD_RLT_OPEN_FILE_FAIL;
    }

    // 获取文件大小
    std::streamsize fileSize = file.tellg();
    if (fileSize > 128 * 1024) {
        return IMD_RLT_INVALID_BIN_FILE;
    }

	  // 读取文件数据
    file.seekg(0, std::ios::beg);
	vector<BYTE> file_data(fileSize);
    if (!file.read(reinterpret_cast<char*>(file_data.data()), fileSize)) {
        return IMD_RLT_OPEN_FILE_FAIL;
    }

	DWORD* bin = (DWORD*)file_data.data();
	if (bin[1] < 0x8020000 || bin[1] > 0x8040000)
		return IMD_RLT_INVALID_BIN_FILE;

	BYTE* data = file_data.data();
	int len = (int)file_data.size();
	WORD check_sum = 0;
	for (int i = 0; i < len; i++)
		check_sum += data[i];

	WINUSB_SETUP_PACKET SetupPacket{};
	ULONG cbSent = 0;
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_DOWNLOAD_FW;
	SetupPacket.Index = check_sum;
	SetupPacket.Value = len;
	int ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

	{
		int really = 0;

		ret = libusb_bulk_transfer(
			m_SensorPipe.usb_handle,         // libusb 设备句柄
			m_SensorPipe.PipeBulkOut,     // Bulk IN 端点地址
			data,					// 数据缓冲区
			len,                    // 要读取的字节数
			&really,                // 实际传输的字节数
			1500                          // 超时时间（单位：毫秒）
		);

		Sleep(3000); //delay for mcu program flash.

		if (ret == LIBUSB_SUCCESS)
		{
			// 读取成功，返回读取的字节数
		}
		else
		{
			// 错误处理
			switch (ret)
			{
			case LIBUSB_ERROR_TIMEOUT:
				ret = WAIT_TIMEOUT;
				dbg("WAIT_TIMEOUT\n");

				// 如果需要，清除或复位管道
				libusb_clear_halt(m_SensorPipe.usb_handle, m_SensorPipe.PipeBulkIn);
				break;

			case LIBUSB_ERROR_PIPE:
				ret = ERROR_BAD_PIPE;
				dbg("Pipe error\n");

				// 复位管道
				libusb_clear_halt(m_SensorPipe.usb_handle, m_SensorPipe.PipeBulkIn);
				break;

			case LIBUSB_ERROR_NO_DEVICE:
				ret = ERROR_DEVICE_NOT_AVAILABLE;
				dbg("Device not available\n");
				break;

			default:
				ret = ERROR_GEN_FAILURE;
				dbg("General error\n");
				break;
			}
		}
	}

//return_hr:
	return ret;
}

DWORD CSensorDevice::ClearFlag(BYTE bStatus)
{
	CheckBusy check_busy;
	
	if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
		return ERROR_INVALID_HANDLE;

	ULONG cbSent = 0;
	WINUSB_SETUP_PACKET SetupPacket{};
	SetupPacket.RequestType = 0x40;
	SetupPacket.Request = CMD_CLEAR_FLAG;
	SetupPacket.Index = bStatus;
	DWORD ret = SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

	//check_busy.done();
	Sleep(1500);

	return ret;
}

DWORD CSensorDevice::WriteFlag()
{
	CheckBusy check_busy;

	//if (m_SensorPipe.usb_handle == INVALID_HANDLE_VALUE || m_SensorPipe.usb_handle == NULL)
	//	return ERROR_INVALID_HANDLE;

	//ULONG cbSent = 0;
	//WINUSB_SETUP_PACKET SetupPacket{};
	//SetupPacket.RequestType = 0x40;
	//SetupPacket.Request = CMD_WRITE_FLAG;
	//SendSetupPacket(&SetupPacket, NULL, 0, &cbSent);

	return 0;
}

