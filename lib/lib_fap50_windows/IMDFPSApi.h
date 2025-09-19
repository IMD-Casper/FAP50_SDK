#pragma once
#include "IMDFPMeasure.h"
#include "UsbDevDefine.h"
enum {
	//USB_BDG_01 = 0x00,
	//USB_BDG_02,
	//USB_BDG_03,
	//USB_BDG_04,
	USB_BDG_MAX = 4
};

DWORD WINAPI UT_CloseSensorDevice(HANDLE hDevice);
DWORD WINAPI UT_CaptureImage(HANDLE hDevice, LPBYTE lpImage, PDWORD pdwImageSize);
DWORD WINAPI UT_CaptureImageEx(HANDLE hDevice, LPWAIT_SCAN_PARAMETER lpScanParam);
DWORD WINAPI UT_CaptureImageTestPattern(HANDLE hDevice, LPWAIT_SCAN_PARAMETER lpScanParam);
DWORD WINAPI UT_CaptureImageExWOI(HANDLE hDevice, LPWAIT_SCAN_PARAMETER lpScanParam);
DWORD WINAPI UT_WOIReset(HANDLE hDevice);
DWORD WINAPI UT_ResetSensor(HANDLE hDevice, ULONG nTime);
DWORD WINAPI UT_GetSensorRegister(HANDLE hDevice, WORD wRegAddr, BYTE* lpValue);
DWORD WINAPI UT_SetSensorRegister(HANDLE hDevice, WORD wRegAddr, BYTE byValue);
DWORD WINAPI UT_GetBISTData(HANDLE hDevice, BYTE* lpValue, INT length);
DWORD WINAPI UT_SetRXChannelOfs(HANDLE hDevice, BYTE byRegAddr, BYTE byValue);
DWORD WINAPI UT_SetDcOffsetWithLoadEn(HANDLE hDevice, WORD wRegAddr, BYTE byValue);
DWORD WINAPI UT_SetFingerDet(HANDLE hDevice, BOOL blEnable);
DWORD WINAPI UT_GetFingerState(HANDLE hDevice, BYTE *pbState);
DWORD WINAPI UT_SetSPISpeed(HANDLE hDevice, BYTE bySpeed);
DWORD WINAPI UT_GetOTPData(HANDLE hDevice, WORD Addr, BYTE* lpValue);
DWORD WINAPI UT_SetOTPData(HANDLE hDevice, WORD Addr, BYTE byValue);
DWORD WINAPI UT_GetChipID(HANDLE hDevice, PWORD pwChipID);
DWORD WINAPI UT_SelectChipNo(HANDLE hDevice, BYTE bChipNo);
DWORD WINAPI UT_SelectGlassType(HANDLE hDevice, BYTE bGlassType);
DWORD WINAPI UT_SetRelayPin(HANDLE hDevice, BYTE bStatus);
DWORD WINAPI UT_SetSimulationFGPin(HANDLE hDevice, BYTE bStatus);
DWORD WINAPI UT_SetGain(HANDLE hDevice, BYTE bStatus);
DWORD WINAPI UT_SetVPPPin(HANDLE hDevice, BYTE bStatus);
DWORD WINAPI UT_SetPower(HANDLE hDevice, BOOL power_down_en);
DWORD WINAPI UT_INTEnable(HANDLE hDevice, BYTE bNum, BYTE bValue);
DWORD WINAPI UT_ReadPortState(HANDLE hDevice, BYTE bNum, BYTE* lpValue);
DWORD WINAPI UT_SWReset(HANDLE hDevice);
DWORD WINAPI UT_GetUSBReaderInfo(HANDLE hDevice, BYTE *ReaderInfo, BYTE size);
DWORD WINAPI UT_GetSystemStatus(HANDLE hDevice, BYTE* err);
DWORD WINAPI UT_EntryISPMode(HANDLE hDevice);
DWORD WINAPI UT_SetMultiChipBoard(HANDLE hDevice, BYTE bStatus);
DWORD WINAPI UT_SetUsbChannel(HANDLE hDevice, BYTE bValue);
DWORD WINAPI UT_SetVDD_Pin(HANDLE hDevice, BYTE bStatus);
DWORD WINAPI UT_SetVDDIO_Pin(HANDLE hDevice, BYTE bStatus);
DWORD WINAPI UT_NoiseCancellationInit(HANDLE hDevice, BYTE* buf, int nSize);
DWORD WINAPI UT_SetGPIOx(HANDLE hDevice, BYTE bGPIO_No, BYTE bBit, BYTE bValue);
DWORD WINAPI UT_GetGPIOx(HANDLE hDevice, BYTE bGPIO_No, BYTE* PortData);
DWORD WINAPI UT_SetSensorArea(HANDLE hDevice, WORD width, WORD length);
DWORD WINAPI UT_SetMeasureInfo(HANDLE hDevice, BYTE bSelectFunc, BYTE bItemNo, BYTE bValue);
DWORD WINAPI UT_GetBtnStatus(HANDLE hDevice, BYTE* bSelectFunc);
DWORD WINAPI UT_TestGPIO(HANDLE hDevice, DWORD* dwResult);
DWORD WINAPI UT_SetSensorPackageType(HANDLE hDevice, BYTE bType);
DWORD WINAPI UT_SetDiffBPCheck(HANDLE hDevice, BYTE bBP_Value);
DWORD WINAPI UT_GetADCCurrent(HANDLE hDevice, BYTE* lpValue, BYTE* lpValueArray, BYTE type);
DWORD WINAPI UT_SetTOUCHPin(HANDLE hDevice, BYTE bStatus);
DWORD WINAPI UT_GetDetControlStatus(HANDLE hDevice, BYTE *pbPinStatus);
DWORD WINAPI UT_GetFrameTime(HANDLE hDevice, WORD *pwFrameTime);
DWORD WINAPI UT_SetPanelStatus(HANDLE hDevice, BYTE bStatus);
DWORD WINAPI UT_SetCoverStampFlag(HANDLE hDevice, BOOL bStatus);
DWORD WINAPI UT_GetSensorReadyFlag(HANDLE hDevice, BOOL* bpSReadyFlag);

DWORD WINAPI UT_LedSpeech(HANDLE hDevice, BYTE led, BYTE led_status, BYTE speech, BOOL is_reader=TRUE);
DWORD WINAPI UT_UsbSwtich(HANDLE hDevice, BYTE usb_sel, BOOL is_reader = TRUE);
DWORD WINAPI UT_SetSpeechLanguage(HANDLE hDevice, BYTE lang, BOOL is_reader = TRUE);
DWORD WINAPI UT_SetSpeechVolume(HANDLE hDevice, BYTE volume, BOOL is_reader = TRUE);
DWORD WINAPI UT_SetBurnCode(HANDLE hDevice, BOOL is_reader = TRUE);
DWORD WINAPI UT_Get_RB_Ver(HANDLE hDevice, BYTE* ver, BOOL is_reader = TRUE);
DWORD WINAPI UT_Get_RB_SN(HANDLE hDevice, BYTE* sn, BOOL is_reader = TRUE);

DWORD WINAPI UT_GetGuid(HANDLE hDevice, BYTE* guid, int len);
DWORD WINAPI UT_UserSpace (HANDLE hDevice, BOOL is_write, WORD offset, BYTE* data, int len);

DWORD WINAPI UT_LifeCheck(HANDLE hDevice, BOOL init);
DWORD WINAPI UT_WriteFlag(HANDLE hDevice);
DWORD WINAPI UT_ClearFlag(HANDLE hDevice);
DWORD WINAPI UT_UpdateApp(HANDLE hDevice, string filePath);
DWORD WINAPI UT_ResetMcuUsb(HANDLE hDevice);
DWORD WINAPI UT_SuspendDevice(HANDLE hDevice);

#pragma pack(push, 1)
union SystemBlock { //system block
	struct {
		BYTE sn[64];
		BYTE brand[64];
		BYTE model[64];
	};
	BYTE data[512];
};

enum REGS_BLOCK_VER {
	REGS_BLOCK_VER_1 = 1,
	REGS_BLOCK_VER_2 = 2,
	REGS_BLOCK_VER_NOW = REGS_BLOCK_VER_2,
};

union RegsBlock { //regs block 
	struct {
		union {
			struct {
				//first_block 
				BYTE reg12;
				BYTE reg18;

				//gain_block 
				BYTE reg02[4];

				//block_2
				//block_3
			};
			BYTE regs[61];
		};
		struct {
			BYTE first_block_writen : 1;
			BYTE gain_block_writen : 1;
			//BYTE block2_writen : 1; //reserved
			//BYTE block3_writen : 1; //reserved
			//BYTE block4_writen : 1; //reserved
			//BYTE block5_writen : 1; //reserved
			//BYTE block6_writen : 1; //reserved
			//BYTE block7_writen : 1; //reserved
		};
		BYTE ver;
		BYTE chksum;
	};
	BYTE data[64];
};

union ModulePara {
	struct {
		SystemBlock system_block;
		RegsBlock	regs_block;
	};
	BYTE data[1024];
};
#pragma pack(pop)


#define MODULE_PARA_OFFSET_SYSTEM_BLOCK		offsetof(ModulePara, system_block)

#define MODULE_PARA_OFFSET_SN				(MODULE_PARA_OFFSET_SYSTEM_BLOCK + offsetof(SystemBlock, sn))
#define MODULE_PARA_SIZE_SN					sizeof(SystemBlock::sn)

#define MODULE_PARA_OFFSET_BRAND			(MODULE_PARA_OFFSET_SYSTEM_BLOCK + offsetof(SystemBlock, brand))
#define MODULE_PARA_SIZE_BRAND				sizeof(SystemBlock::brand)

#define MODULE_PARA_OFFSET_MODEL			(MODULE_PARA_OFFSET_SYSTEM_BLOCK + offsetof(SystemBlock, model))
#define MODULE_PARA_SIZE_MODEL				sizeof(SystemBlock::model)

#define MODULE_PARA_OFFSET_REGS_BLOCK		(MODULE_PARA_OFFSET_SYSTEM_BLOCK + offsetof(ModulePara, regs_block))
#define MODULE_PARA_SIZE_REGS_BLOCK			sizeof(RegsBlock)

DWORD WINAPI UT_ModulePara(HANDLE hDevice, BOOL is_write, WORD offset, BYTE* data, int len);
DWORD WINAPI UT_ModulePara_SN(HANDLE hDevice, BOOL is_write, BYTE* data, int len);
DWORD WINAPI UT_ModulePara_Brand(HANDLE hDevice, BOOL is_write, BYTE* data, int len);
DWORD WINAPI UT_ModulePara_Model(HANDLE hDevice, BOOL is_write, BYTE* data, int len);
DWORD WINAPI UT_ModulePara_Regs(HANDLE hDevice, BOOL is_write, BYTE* data, int len);
DWORD WINAPI UT_set_reader(HANDLE hDevice, BOOL is_reader);

//DWORD WINAPI UT_FindSensorDeviceList(LPCTSTR* list_dev_path);
//HANDLE WINAPI UT_ConnectSensorDevice(LPCTSTR dev_path);
HANDLE WINAPI UT_ConnectSensor();
void WINAPI UT_SetAndroidInfo(int fd, unsigned int uiVID, unsigned int uiPID);


DWORD ConnectPen();
void  DisconnectPen();
bool open_usb_device(UsbDeviceContext& dev);
void close_usb_device(UsbDeviceContext& dev);
void poll_usb(UsbDeviceContext& dev);
void WINAPI UT_start_polling(UsbDeviceType type);
void WINAPI UT_stop_polling(UsbDeviceType type);

UsbDeviceContext& get_usb_device(UsbDeviceType type);

