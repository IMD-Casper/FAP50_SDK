#pragma once
#include "IMDFPSApi.h"
#include "libusb.h"


//#include <winusb.h>
#pragma pack(1)
struct WINUSB_SETUP_PACKET {
	UCHAR   RequestType;
	UCHAR   Request;
	USHORT  Value;
	USHORT  Index;
	USHORT  Length;
};
#pragma pack()


// FINGER PRINT SENSOR COMMAND SETS
#define CMD_GET_USBREADER_MODEL		0x00
#define CMD_SET_SPI_SPEED			0x01
#define CMD_GET_REGISTERS			0x02
#define CMD_SET_REGISTERS			0x03
#define CMD_RESET_SENSOR			0x04
#define CMD_QUERY_FINGER			0x05

// ************ IMPORTANT **************
// Code 0x06 can not be used for Command code
// because of 0x06 is specified by
// Microsoft OS Feature Descriptor!!
#define CMD_ABORT_TRANSMIT			0x07
#define CMD_ENABLE_FIG_DET			0x08
#define CMD_DISABLE_FIG_DET			0x09

#define CMD_CAPTURE_IMAGE			0x10
#define CMD_CAPTURE_IMAGE_ONESHOT   0x11
#define CMD_CAPTURE_IMAGE_CONTINUE  0x12
#define CMD_OTP_READ                0x13
#define CMD_OTP_WRITE               0x14
#define CMD_USB_READ_SYNC			0x15
#define CMD_CHIPID_READ             0x16
#define	CMD_SYSTEM_STATUS			0x17
#define	CMD_SET_CHIP_NO 			0x18
#define	CMD_SET_RELAY_PIN 			0x19
#define CMD_SF125_GPIO_TEST			0x1A // boxchu 0426 added

#define	CMD_SET_GAIN    			0x20
#define	CMD_CAPTURE_IMAGE_WOI		0x21
#define	CMD_WOI_RESET       		0x22
#define	CMD_VPP_EN		       		0x23
#define	CMD_EN_INT		       		0x24
#define	CMD_READ_PORT_STATE    		0x25
#define	CMD_SW_RESET    		    0x26
#define CMD_SET_MULTICHIP_BOARD     0x27 // amelie 1026 modified
#define CMD_SET_USB_NO              0x28 // amelie 1026 added
#define CMD_VDDIO_EN                0x29 // amelie 1027 added
#define CMD_VDD_EN                  0x30 // amelie 1027 added
#define CMD_NOISE_CANCELLATION_INIT 0x31
#define CMD_GET_BUTTON_STATUS       0x32 
#define CMD_SET_LED_STATUS          0x33 
#define CMD_SET_PACKAGE_TYPE        0x34
#define CMD_SET_BPCHECK_TYPE        0x35
#define CMD_GET_ADCVALUE_ST         0x36
#define CMD_GET_ADCVALUE_OP         0x37
#define CMD_GET_ADCVALUEARRAY       0x38
#define CMD_SET_CALIB_OFS           0x39

#define CMD_BIST_TEST               0x40
#define CMD_TOUCH_EN                0x41
#define CMD_GET_DETCTL              0x42
#define	CMD_SET_SimulationFG_PIN    0x43
#define CMD_GET_FRAMETIME           0x44
#define CMD_PANEL_STATUS            0x45
#define CMD_GLASS_TYPE              0x46
#define CMD_SET_COVERSTAMP_FLAG     0x47
#define CMD_TESTPAT_VERIFY          0x48
#define CMD_SENSOR_READY            0x49 // Ask sensor initialization done
#define CMD_RESET_MCU_AND_USB		0x4A

#define CMD_SET_POWER				0x50
#define CMD_UART_TX_PTL				0x51				
#define CMD_UART_RX_PTL				0x52
#define CMD_USER_SPACE				0x53

#define CMD_CLEAR_FLAG              0x60
//#define							0x61
#define CMD_DOWNLOAD_FW				0x62
#define CMD_MODULE_PARA				0x63
#define CMD_LIFE_CHECK				0x64

#define CMD_ENTRY_ISP_MODE			0xA5 // boxchu 0427 added
#define CMD_GET_PORT                0xF0 // amelie 1104 added
#define CMD_SET_PORT                0xF1 // amelie 1104 added

#define USBREADER_MODEL_ID			0x1768

//Chip ID definition which define in register 0x3E(ID high) and 0x3F(ID low)
#define CHIP_ID_SF301		0x1002
#define CHIP_ID_SF302		0xA302
#define CHIP_ID_SF201		0x2001
#define CHIP_ID_SF202		0xA202
#define CHIP_ID_SF104		0x0104
#define CHIP_ID_SF105B		0x0105
#define CHIP_ID_SF105A		0xA105
#define CHIP_ID_SF107		0xA107
#define CHIP_ID_SF108		0xA108
#define CHIP_ID_SF801		0xA801
#define CHIP_ID_IF101		0xE101
#define CHIP_ID_IF501		0xE501
#define CHIP_ID_SFA01		0xEA01
#define CHIP_ID_SF519		0xA519
#define CHIP_ID_SF125TB		0xA105
#define CHIP_ID_iG101		0xAB01
#define CHIP_ID_iG102		0xF131
#define CHIP_ID_iG103		0x6000
#define CHIP_ID_FAP50		0xBD02

enum UART_CMD_INST {
	UART_CMD_INST_LED_SPEECH = 0x01,
	UART_CMD_INST_USB_SWITCH = 0x02,
	UART_CMD_INST_POWER_DET = 0x03,
	UART_CMD_INST_SPEECH_VOL = 0x04,
	UART_CMD_INST_SPEECH_LANG = 0x05,
	UART_CMD_INST_RESET_FAP50 = 0x06,
	UART_CMD_INST_GET_VER = 0x07,
	UART_CMD_INST_GET_SN = 0x08,

	UART_CMD_INST_BURN_CODE = 0x1A,
};

struct BRIDGE_DATA_PACKET
{
    unsigned char	Header;
	unsigned char	Length;
	unsigned char	Data[16];
};

#define PACKET_HEADER_STATUS	0x01
#define PACKET_HEADER_DATA		0x02

struct SENSOR_PIPE_ID
{
	//HANDLE hDeviceHandle;
	libusb_device_handle* usb_handle;
	BYTE  PipeBulkIn;
    BYTE  PipeBulkOut;
};

typedef struct tagSENSOR_REGISTER_VALUE
{
	BYTE	Name;
	BYTE	Value;
} SENSOR_REGISTER_VALUE, *PSENSOR_REGISTER_VALUE;

typedef struct tagSENSOR_REGISTER_LIST
{
	DWORD	dwCount;
	PSENSOR_REGISTER_VALUE lpRegisters;
} REGISTER_LIST, *PREGISTER_LIST;

enum E_PACKAGETYPE
{
	ePackage_TypeD = 0x01,
	ePackage_TypeT = 0x02,
	ePackage_TypeU = 0x03,
	ePackage_TypeUnknown = 0xFF,
};

//interface
//interface I_Device
//{
//	virtual DWORD CaptureImageEx(LPWAIT_SCAN_PARAMETER lpScanParam) = NULL;
//	virtual DWORD GetSensorRegister(WORD wRegAddr, BYTE* lpValue) = NULL;
//	virtual DWORD SetSensorRegister(WORD wRegAddr, BYTE byValue) = NULL;
//	virtual	DWORD SendSetupPacket(PVOID pSetupPacket, PUCHAR  Buffer, ULONG  BufferLength, PULONG  LengthTransferred) = NULL;
//	virtual	DWORD GetChipID(WORD *wChipID) = NULL;
//	virtual DWORD ResetSensor(ULONG nTime) = NULL;
//};

//real device
class CSensorDevice //: public I_Device
{
	//HANDLE			m_hThread;
	//BOOL			m_fCancelIO;
	//BOOL			m_Direction;
	WAIT_SCAN_PARAMETER	m_ScanParam;

public:
	SENSOR_PIPE_ID	m_SensorPipe;
	//CString m_pszDevicePath;
	DWORD m_dwWaitTime;

	CSensorDevice(void);
	virtual ~CSensorDevice(void);

	DWORD ConnectSensor(HANDLE handle);
	DWORD FlushReadPipe(void);

	DWORD SetFingerDet(BOOL blEnable);
	DWORD GetFingerState(BYTE* pbExist);
	DWORD SetSPISpeed(BYTE bySpeed);
	DWORD GetOTPData(WORD Addr, BYTE* lpValue);
	DWORD SetOTPData(WORD Addr, BYTE byValue);

	DWORD SelectChipNo(BYTE byNumber);
	DWORD SetRelayPin(BYTE byStatus);
	DWORD SetVPPPin(BYTE byStatus);
	DWORD SetGain(BYTE byStatus);
	DWORD WOIReset(void);
	DWORD INTEnable(BYTE bNum, BYTE bStatus);
	DWORD ReadPortState(BYTE bNum, BYTE* pbState);
	DWORD SWReset(void);
	DWORD GetUSBReaderInfo(BYTE* ReaderInfo, BYTE size);
	DWORD EntryISPMode();
	DWORD GetSystemStatus(void* err);
	DWORD SetMultiChipBoard(BYTE byMultichip);
	DWORD SetUsbChannel(BYTE bStatus);
	DWORD SetVDDIO_Pin(BYTE byStatus);
	DWORD SetVDD_Pin(BYTE byStatus);
	DWORD NoiseCancellationInit(BYTE* buf, int nSize);

	DWORD Set_GPIOx_Val(BYTE bGPIONo, BYTE bBit, BYTE bVal);
	DWORD Get_GPIOx_Val(BYTE bGPIONo, BYTE* pbState);

	DWORD Get_BtnStatus(BYTE* bSelectFunc);
	DWORD Set_MeasureInfo(BYTE bSelectFunc, BYTE bItemNo, BYTE bVal);
	DWORD TestGPIO(DWORD* dwResult);
	DWORD SetSensorPackageType(BYTE bType);
	DWORD Set_DiffBP_Val(BYTE bBP_Value);
	DWORD GetADCCurrentOP(BYTE* lpValue);
	DWORD GetADCCurrentST(BYTE* lpValue);
	DWORD GetADCCurrentArray(BYTE* lpValueArray);
	DWORD SetTOUCH_Pin(BYTE bStatus);
	DWORD ReadDetCTLState(BYTE* pbState);
	DWORD SetSimulationFGPin(BYTE bStatus);
	DWORD GetFrameTime(WORD* wFrameTime);
	DWORD SetPanelStatus(BYTE bStatus);
	DWORD SelectGlassType(BYTE byNumber);
	DWORD SetCoverStampFlag(BOOL bStatus);
	DWORD GetSensorReadyFlag(BOOL* pbState);
	DWORD SetPower(BOOL power_down_en);

	DWORD ResetMcuUsb(void);
	DWORD LedSpeech(BYTE led, BYTE led_status, BYTE speech);
	DWORD UsbSwtich(BYTE usb_sel);
	DWORD GetBattery(BYTE* battery);
	DWORD SetSpeechLanguage(BYTE lang);
	DWORD SetSpeechVolume(BYTE volume);
	DWORD SetBurnCode();
	DWORD LifeCheck(BOOL init);
	DWORD ClearFlag(BYTE bStatus);
	DWORD WriteFlag();
	DWORD UpdateApp(string filePath);
	int UserSpace (BOOL is_write, WORD offset, BYTE* data, int len);
	int ModulePara(BOOL is_write, WORD offset, BYTE* data, int len);
	DWORD Get_RB_Ver(BYTE* ver);
	DWORD Get_RB_SN(BYTE* sn);
	//DWORD set_reader(BOOL is_reader);
	int SuspendDevice();

	virtual int CaptureImageEx(LPWAIT_SCAN_PARAMETER lpScanParam);
	virtual DWORD GetSensorRegister(WORD wRegAddr, BYTE* lpValue);
	virtual DWORD SetSensorRegister(WORD wRegAddr, BYTE byValue);
	virtual	int SendSetupPacket(PVOID pSetupPacket, PUCHAR  Buffer, ULONG  BufferLength, PULONG  LengthTransferred);
	virtual	DWORD GetChipID(WORD *wChipID);
	virtual DWORD ResetSensor(ULONG nTime);
};

