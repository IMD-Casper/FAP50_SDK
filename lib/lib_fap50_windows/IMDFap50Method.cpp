#include "imd_framework.h"
#include "IMDFPSApi.h"
#include "IMDFap50Method.h"
#include "nfiq2api.h"
#include "pen_mdoule_interface.h"

#define IMD_FAP50_LIB_MAJOR		1
#define IMD_FAP50_LIB_MINOR		7
#define IMD_FAP50_LIB_PATCH		3

#define OPENCV_LIB_MAJOR	CV_VERSION_MAJOR
#define OPENCV_LIB_MINOR	CV_VERSION_MINOR
#define OPENCV_LIB_PATCH	CV_VERSION_REVISION

#define NBIS_LIB_MAJOR		5
#define NBIS_LIB_MINOR		0
#define NBIS_LIB_PATCH		0

#define EXT_WIDTH	500
#define EXT_HEIGHT	500

//extern "C" 
//{
//#include <wsq.h>
//
////#include <img_io.h>
//extern int write_raw_from_memsize(char*, unsigned char*, const int);
//}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static mutex mu;
extern bool g_is_product_support_panel;

//InterfaceImdFap50Method* create_imd_fap50()
//{
//	if (_fap50_method)
//		delete_imd_fap50();
//
//	_fap50_method = new IMDFap50Method;
//	return _fap50_method;
//}

static string format_to_string(const char* fmt, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	return string(buffer);
}

void IMDFap50Method::thread_init_nfiq2()
{
	char* m_nfiq2_hash = nullptr;
	InitNfiq2(&m_nfiq2_hash);

	if (m_nfiq2_hash == nullptr)
		dbg("InitNfiq2 fail\n");

	//dbg("NFIQ2 hash:%s\n", m_nfiq2_hash);
	//m_nfiq2_hash = m_nfiq2_hash;
	//delete m_nfiq2_hash;
	//dbg(L"thread_init_nfiq2()\n");
}

void IMDFap50Method::thread_life_check()
{
	//return; //debug

	Sleep(1000);

	DWORD time = 0;// timeGetTime() / 1000;
	while (1)
	{
		DWORD now = timeGetTime()/1000;
		if (time != now)
		{
			time = now;
			//if (m_property.life_check_en == false)
			//	continue;

			BOOL init = (m_property.life_check_en == false);
			DWORD ret = 0;
			ret = UT_LifeCheck(device_handle, init);//life_check(init);
			//if (ret)
			//	fap50->connect_usb_device();
		}
	}
}

IMDFap50Method::IMDFap50Method()
{
	//dbg("sizeof(SystemProperty)=%d\n", sizeof(SystemProperty));
	//dbg("sizeof(ImageProperty)=%d\n", sizeof(ImageProperty));
	//dbg("sizeof(ImageStatus)=%d\n", sizeof(ImageStatus));

	//dbg("sizeof(show_mode)=%d\n", sizeof(ImageStatus::show_mode));
	//dbg("sizeof(is_roll_init)=%d\n", sizeof(ImageStatus::is_roll_init));
	//dbg("sizeof(finger_num)=%d\n", sizeof(ImageStatus::finger_num));
	//dbg("sizeof(contours)=%d\n", sizeof(ImageStatus::contours));
	//dbg("sizeof(frame_rate)=%d\n", sizeof(ImageStatus::frame_rate));
	//dbg("sizeof(img)=%d\n", sizeof(ImageStatus::img));

	m_Height = IMAGE_HEIGHT, m_Width = IMAGE_WIDTH;
	callback_event = nullptr;

	ZeroMemory(&m_property, sizeof(m_property));
	m_property.fap50_lib_ver[2] = IMD_FAP50_LIB_MAJOR;
	m_property.fap50_lib_ver[1] = IMD_FAP50_LIB_MINOR;
	m_property.fap50_lib_ver[0] = IMD_FAP50_LIB_PATCH;

	m_property.opencv_lib_ver[2] = OPENCV_LIB_MAJOR;
	m_property.opencv_lib_ver[1] = OPENCV_LIB_MINOR;
	m_property.opencv_lib_ver[0] = OPENCV_LIB_PATCH;

	m_property.nbis_lib_ver[2] = NBIS_LIB_MAJOR;
	m_property.nbis_lib_ver[1] = NBIS_LIB_MINOR;
	m_property.nbis_lib_ver[0] = NBIS_LIB_PATCH;

	m_property.image_bit_per_pix = 8;
	m_property.image_height = IMAGE_HEIGHT;
	m_property.image_width = IMAGE_WIDTH;
	m_property.image_pix_per_inch = 500;

	m_property.finger_position = FINTER_POSITION_UNKNOW_FINGER;

	m_property.png_compress_ratio = 9;

	m_property.jp2_quality = 750;
	//	m_property.jp2_pix_per_inch //The default pixel per inch: 500
	//	m_property.jp2_comment_text = nullptr; //JP2 comment text.

	m_property.wsq_bit_rate = 0.75;
	//	m_property.wsq_bit_per_pix = m_property.image_bit_per_pix; //The default bit per pixel: 8
	//	m_property.wsq_pix_per_inch = m_property.image_pix_per_inch; //The default pixel per inch: 500
	m_property.wsq_comment_text = nullptr;

	m_property.nfiq_ver = NFIQ_V1;
	m_property.nfiq_score_minimum_acceptable = 3;
	m_property.nfiq2_score_minimum_acceptable = 35;
	m_property.speech_language = 0;
	m_property.speech_volume = 1;
	m_property.scan_by_manual = false;

	int major=0, minor=0, patch=0;
	const char* opencv_ver="N/A";
	GetNfiq2Version(&major, &minor, &patch, &opencv_ver);
	m_property.nfiq2_lib_ver[2] = major;
	m_property.nfiq2_lib_ver[1] = minor;
	m_property.nfiq2_lib_ver[0] = patch;
//	dbg(L"opencv ver:%s\n", opencv_ver);

	//use thread to init NFIQ2 lib. because it need to long time to init.
	thread_member(IMDFap50Method::thread_init_nfiq2);

	m_property.speech_en = true;

	m_property.life_check_en = false;
	m_property.scan_timeout_ms = 120 * 1000;
	//thread_member(IMDFap50Method::thread_life_check);

	init_image();
	init_device();

	int clear_roi[] = {395, 830, 320, 130 };
	int confirm_roi[] = {886, 825, 320, 130 };
	memcpy(m_property.signature_clear_roi, clear_roi, sizeof(m_property.signature_clear_roi));
	memcpy(m_property.signature_confirm_roi, confirm_roi, sizeof(m_property.signature_clear_roi));
}

IMDFap50Method:: ~IMDFap50Method() 
{
	//close_device();
	//m_property.life_check_en = FALSE;
	//life_check(TRUE);
	dbg("~IMDFap50Method()\n");
	//scan_cancel();

	//if (m_nfiq2_hash)
	//{
	//	delete m_nfiq2_hash;
	//	m_nfiq2_hash = nullptr;
	//}

	//imd_contours.clear();
	//imd_contours.shrink_to_fit();
	//std::vector<ImdDiamond>().swap(imd_contours);
	//std::vector<ImdDiamond>(imd_contours).swap(imd_contours);
	
	//std::vector<int> first{ 1,2,3,4 };
	//first.push_back(1);
	//first.push_back(1);
	//first.push_back(1);
	//first.push_back(1);
	//first.clear(); //not sure this line is even necessary
	//first.shrink_to_fit();
	//{
	//	std::vector<int> second{}; //"second" points to an empty buffer
	//	std::swap(first, second); //"first" now points to an empty buffer
	//} //second goes out of scope and cleans up its 4-integer buffer

/*
Detected memory leaks!
Dumping objects ->
Object dump complete.

https://stackoverflow.com/questions/898273/memory-leak-sort-of-with-a-static-stdvector
 
	That is just a quirk of Visual Studio. The vector destructor does release the memory,
	but the memory checking module doesn't always spot it, so it complains. It is a bit 
	of a pain, but nothing to worry about.
*/

}

//void IMDFap50Method::HwBoost()
//{
//	HANDLE m_CurrentSensorHandle = device_handle;
//	
//	if (enable_hw_boost)//hw boost
//		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6115, 0x2C);
//	else
//		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6115, 0x0C);
//}
//
//void IMDFap50Method::HwIsp()
//{
//	HANDLE m_CurrentSensorHandle = device_handle;
//
//	if (enable_hw_isp)//hw isp
//	{
//		//UT_SetSensorRegister(m_CurrentSensorHandle, 0x6012, 0x8C);
//		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6108, 0x98);
//	}
//	else
//	{
//		//UT_SetSensorRegister(m_CurrentSensorHandle, 0x6012, 0x99);
//		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6108, 0x18);
//	}
//}
//
//void IMDFap50Method::HwAutoGain()
//{
//	enable_hw_auto_gain;
//}

void IMDFap50Method::recovery_power_drop()
{
	dbg("recovery_power_drop\n");

	READER_INFORMATION ver;
	for (int i = 0; i < 2; i++)
	{
		DWORD result = UT_GetUSBReaderInfo(device_handle, (BYTE*)&ver, sizeof(READER_INFORMATION));
		if (result == ERROR_SUCCESS)
			break;

		if (false == connect_usb_device())
			return;
	}

	HANDLE m_CurrentSensorHandle = device_handle;

	IMD_RESULT ret = IMD_RLT_FAIL;
	DWORD dwError;

	WORD ver_num = 0;
	ver_num = (ver.FirmwareMajor << 8) | ver.FirmwareMinor;
	if (ver_num < 0x0300)
	{
		if (ver_num <= 0x020A)
			UT_SetSensorRegister(m_CurrentSensorHandle, 0x1018, 0xC5);//special case, iG reg18 

		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6012, 0x8F);
		UT_SetDcOffsetWithLoadEn(m_CurrentSensorHandle, 0x601A, 0x60);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x601B, 0xAC);
	}
	else //if (ver_num <= 0x0302)
	{
		//UT_SetSensorRegister(m_CurrentSensorHandle, 0x1115, 0x2C);//do this will big current.
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x1116, 0xFF);
		Sleep(2);

		//UT_SetSensorRegister(m_CurrentSensorHandle, 0x6017, 0x29);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x101B, 0x8C);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x201B, 0x00);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x301B, 0x00);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x401B, 0x00);
		Sleep(2);

		UT_SetSensorRegister(m_CurrentSensorHandle, 0x1018, 0xC0); // 0xE6);//special case, iG reg18 

		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6002, 0xDB);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6003, 0x42);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6004, 0x67);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6005, 0x6C);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6006, 0x84);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6008, 0x94); //20250820:0x94, slow usb need to set 0xA4
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x600C, 0x14);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x600E, 0x50);
		//m_reg12 = 0x90;
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6012, m_reg12);// 0x9F);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6013, 0x00);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6014, 0x11);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6015, 0x91); //20250820:91, slow usb need to set 0x95
		UT_SetDcOffsetWithLoadEn(m_CurrentSensorHandle, 0x601A, 0x50);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x601B, 0x6C);
		//if (enable_runtime_judge_black == false)
		//	UT_SetSensorRegister(m_CurrentSensorHandle, 0x601B, 0xFC);

		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6107, 0x88);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6108, 0x18);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6109, 0x11);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x610A, 0xC0);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x610B, 0x00);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x610C, 0x09);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x610D, 0x80);
	}

	UT_SetSensorRegister(m_CurrentSensorHandle, 0x0004, 0x28);//special case, iB reg4

	UT_SetSensorRegister(m_CurrentSensorHandle, 0x2018, 0x80);//special case, iG reg18
	UT_SetSensorRegister(m_CurrentSensorHandle, 0x3018, 0x80);//special case, iG reg18
	UT_SetSensorRegister(m_CurrentSensorHandle, 0x4018, 0x80);//special case, iG reg18

	//load reg12, reg18, reg02
	once_for
	{
		RegsBlock rb{};
		if (UT_ModulePara_Regs(m_CurrentSensorHandle, 0, (BYTE*)&rb, MODULE_PARA_SIZE_REGS_BLOCK))
			break;

		{
			if (rb.reg12 != 0 && rb.reg12 != 0xFF)
			{
				//UT_SetSensorRegister(m_CurrentSensorHandle, 0x6012, rb.reg12); //recover power no use this.
				//m_reg12 = rb.reg12;
			}
			if (rb.reg18 != 0 && rb.reg18 != 0xFF)
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x1018, rb.reg18);
		}

		if (rb.ver >= REGS_BLOCK_VER_2)
		{
			if (rb.first_block_writen)
			{
				//UT_SetSensorRegister(m_CurrentSensorHandle, 0x6012, rb.reg12); //recover power no use this.
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x1018, rb.reg18);
				//m_reg12 = rb.reg12;
			}

			if (rb.gain_block_writen)
			{
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x1002, rb.reg02[0]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x2002, rb.reg02[1]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x3002, rb.reg02[2]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x4002, rb.reg02[3]);
			}
		}
		else if (rb.ver >= REGS_BLOCK_VER_1)
		{
			if (rb.reg02[0] != 0 && rb.reg02[0] != 0xFF)
			{
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x1002, rb.reg02[0]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x2002, rb.reg02[1]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x3002, rb.reg02[2]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x4002, rb.reg02[3]);
			}
		}
	}

	UT_SetPower(device_handle, TRUE); //power saving mode.
	//UT_set_reader(device_handle, m_is_reader);

	//ret = background_calibration_fap50(device_handle);
	for (int i = 0; i < 4; i++)
	{
		int ig_num = i + 1;
		WORD RegAddr = ((ig_num & 0xF) << 12) | ((0 & 0xF) << 8) | 0x1A;// &0xFF;
		BYTE RegValue = cali_done_reg.reg1A[i];
		dwError = UT_SetDcOffsetWithLoadEn(m_CurrentSensorHandle, RegAddr, RegValue);

		RegAddr = ((ig_num & 0xF) << 12) | ((0 & 0xF) << 8) | 0x13;// &0xFF;
		RegValue = cali_done_reg.reg13[i];
		UT_SetSensorRegister(m_CurrentSensorHandle, RegAddr, RegValue);
	}
}

//IMD_RESULT IMDFap50Method::open_device()
//{
//	return IMD_RESULT(0);
//}
//IMD_RESULT IMDFap50Method::close_device()
//{
//	return IMD_RESULT(0);
//}

unsigned long long hexstrtointeger(char* str)
{
	unsigned long long result = 0;
	const char* hex = "0123456789ABCDEF";
	while (*str)
	{
		result *= 0x10;
		char* pos = strchr((char*)hex, toupper(*str++));
		if (pos) 
			result += pos - hex;
	}

	return result;
}

/*
	20250429_box: cali reg12

	method:
		default reg12 = 0x94. with test ptn mode (reg11=0x90), capture 2 times.
		if fail. reg12 += 2; max = 0x9C.

	This issue is more likely to occur with ROSSII+. Because the panel will share the USB bandwidth.
 */
void IMDFap50Method::cali_reg12()
{
	extern unsigned char reg11_0x90_png[];
	extern unsigned int reg11_0x90_png_len;

	Mat buf(1, reg11_0x90_png_len, CV_8UC1, reg11_0x90_png);
	Mat mat_reg11_0x90_png = imdecode(buf, cv::IMREAD_UNCHANGED);
	if (mat_reg11_0x90_png.empty())
	{
		dbg("reg11_0x90_png_len image is empty.\n");
		return;
	}

	int cols = 1024;
	int rows = 1600;
	
	if (m_reg12 < 0x94)
		m_reg12 = 0x94;
	//m_reg12 = 0x90;
	//UT_SetSensorRegister(device_handle, 0x6012, m_reg12);

	//BYTE reg18 = 0xD0;
	//UT_SetSensorRegister(device_handle, 0x1018, reg18);
	//dbg("fix reg12=%X reg18=%X\n", m_reg12, reg18);
	//return;

	BYTE org_reg11; UT_GetSensorRegister(device_handle, 0x1011, &org_reg11);
	UT_SetSensorRegister(device_handle, 0x6011, 0x90);

	int success_time = 0;
	//Mat img_pattern(rows, cols, CV_8UC1);
	//BYTE* pattern = img_pattern.data;
	//for (int i = 0; i < rows * cols; i++)
	//	pattern[i] = (i % 256);

	Mat raw_data(rows, cols, CV_8UC1), img_data(rows, cols, CV_8UC1), result;
	WAIT_SCAN_PARAMETER scan{};
	scan.hw_scan_mode = HW_SCAN_MODE_FLAT;
	scan.raw = raw_data.data;
	scan.img = img_data.data;
	scan.raw_size = raw_data.total();

	while (m_reg12 <= 0x9C)
	{
		DWORD dwError = UT_CaptureImageEx(device_handle, &scan);
		BYTE* buf = raw_data.data;
		compare(raw_data, mat_reg11_0x90_png, result, CmpTypes::CMP_EQ);
		int matched = countNonZero(result);
		if (matched == cols * rows) //success
		{
			success_time++;
			if (success_time == 2)
			{
				m_reg12 ++;
				UT_SetSensorRegister(device_handle, 0x6012, m_reg12);
				break;
			}
		}
		else
		{
			m_reg12 ++;
			UT_SetSensorRegister(device_handle, 0x6012, m_reg12);
		}
	}
	//imshow("raw_data", raw_data);

	UT_SetSensorRegister(device_handle, 0x6011, org_reg11);

func_end:
	;//dbg("after cali regr12=%X\n", m_reg12);
}
void IMDFap50Method::cali_reg12_for_fw0336()
{
	int cols = 1024;
	int rows = 1600;

	//m_reg12 = 0x90;
	//UT_SetSensorRegister(device_handle, 0x6012, m_reg12);
	//BYTE reg18 = 0xD0;
	//UT_SetSensorRegister(device_handle, 0x1018, reg18);
	//dbg("fix reg12=%X reg18=%X\n", m_reg12, reg18);

	int success_time = 0;

	Mat raw_data(rows, cols, CV_8UC1), img_data(rows, cols, CV_8UC1), result;
	WAIT_SCAN_PARAMETER scan{};
	scan.hw_scan_mode = HW_SCAN_MODE_FLAT;
	scan.raw = raw_data.data;
	scan.img = img_data.data;
	scan.raw_size = raw_data.total();

	while (m_reg12 <= 0x9F)
	{
		DWORD dwError = UT_CaptureImageEx(device_handle, &scan);
		HwStatus hw_status{};
		UT_GetFingerState(device_handle, (BYTE*)&hw_status.all);
		if (hw_status.all == 0)
		{
			success_time++;
			if (success_time == 2)
				break;
		}
		else if (hw_status.is_usb_too_slow)
		{
			//dbg("reg12=%2X usb is too slow.\n", m_reg12);
			m_reg12++;
			UT_SetSensorRegister(device_handle, 0x6012, m_reg12);
			success_time = 0;
			continue;
		}
	}

func_end:
	;//dbg("after cali regr12=%X\n", m_reg12);
}

IMD_RESULT IMDFap50Method::device_reset()
{
	//m_property.life_check_en = TRUE;
	//static int reset_cnt = 0;
	//dbg(L"reset cnt %d\n", reset_cnt++);

	last_result = IMD_RLT_SUCCESS;
	scan_cancel();
	WORD ver_num = 0;

	img_kind = IMG_KIND_NORMAL;//IMG_KIND_RAW;// IMG_KIND_NORMAL;
	set_system_property(nullptr);
	init_image();
	IMD_RESULT ret = init_device();
	if (ret)
		return ret;

	HANDLE m_CurrentSensorHandle = device_handle;

	READER_INFORMATION ver;
	UT_GetUSBReaderInfo(device_handle, (BYTE*)&ver, sizeof(READER_INFORMATION));
	panel_type = (E_PANEL_VERSION)ver.module_info.panel_ver;
	enable_runtime_judge_black = false;

	bImgMCUSort = FALSE;
	m_property.fw_ver[1] = ver.FirmwareMajor;
	m_property.fw_ver[0] = ver.FirmwareMinor;

	ret = IMD_RLT_FAIL;
	DWORD dwError = UT_ResetSensor(device_handle, 100);
	if (dwError)
	{
		ret = IMD_RLT_RESET_DEVICE_FAIL;
		dbg("IMD_RLT_RESET_DEVICE_FAIL\n");
		goto func_end;
	}

	WORD chip_id;
	dwError = UT_GetChipID(device_handle, &chip_id);
	m_property.chip_id = chip_id;
	if (chip_id != 0xBD02) 
	{
		ret = IMD_RLT_CHIP_ID_FAIL;			
		dbg("IMD_RLT_CHIP_ID_FAIL\n");
		goto func_end;
	}

	ver_num = (ver.FirmwareMajor << 8) | ver.FirmwareMinor;
	if (ver_num < 0x0300)
	{
		if (ver_num <= 0x020A)
			UT_SetSensorRegister(m_CurrentSensorHandle, 0x1018, 0xC5);//special case, iG reg18 

		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6012, 0x8F);
		UT_SetDcOffsetWithLoadEn(m_CurrentSensorHandle, 0x601A, 0x60);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x601B, 0xAC);
	}
	else //if (ver_num <= 0x0335)
	{
		//UT_SetSensorRegister(m_CurrentSensorHandle, 0x1115, 0x2C);//do this will big current.
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x1116, 0xFF);
		Sleep(2);

		//UT_SetSensorRegister(m_CurrentSensorHandle, 0x6017, 0x29);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x101B, 0x8C);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x201B, 0x00);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x301B, 0x00);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x401B, 0x00);
		Sleep(2);

		UT_SetSensorRegister(m_CurrentSensorHandle, 0x1018, 0xC0);// 0xE6);//special case, iG reg18 

		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6002, 0xDB);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6003, 0x42);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6004, 0x67); //20240918 RD: if bg too white, v2p4 set zero.
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6005, 0x6C);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6006, 0x84);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6008, 0x94); //20250820:0x94, slow usb need to set 0xA4
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x600C, 0x14);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x600E, 0x50);
		
		m_reg12 = 0x90; // reg12 piority: ini_file > (choice bigger one between SDK and flash)
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6012, m_reg12);// 0x9F);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6013, 0x00);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6014, 0x11);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6015, 0x91); //20250820:0x91, slow usb need to set 0x95
		UT_SetDcOffsetWithLoadEn(m_CurrentSensorHandle, 0x601A, 0x20);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x601B, 0x6C);
		//if(enable_runtime_judge_black == false)
		//	UT_SetSensorRegister(m_CurrentSensorHandle, 0x601B, 0xFC);

		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6107, 0x88);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6108, 0x18);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x6109, 0x11);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x610A, 0xC0);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x610B, 0x00);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x610C, 0x09);
		UT_SetSensorRegister(m_CurrentSensorHandle, 0x610D, 0x80);
	}

	UT_SetSensorRegister(m_CurrentSensorHandle, 0x0004, 0x28);//special case, iB reg4

	UT_SetSensorRegister(m_CurrentSensorHandle, 0x2018, 0x80);//special case, iG reg18
	UT_SetSensorRegister(m_CurrentSensorHandle, 0x3018, 0x80);//special case, iG reg18
	UT_SetSensorRegister(m_CurrentSensorHandle, 0x4018, 0x80);//special case, iG reg18
	
	//load reg12, reg18, reg02
	once_for
	{
		RegsBlock rb{};
		if (UT_ModulePara_Regs(m_CurrentSensorHandle, 0, (BYTE*)&rb, MODULE_PARA_SIZE_REGS_BLOCK) != 0)
			break;

		{
			if (rb.reg12 != 0 && rb.reg12 != 0xFF)
			{
				if (rb.reg12 > m_reg12)
					m_reg12 = rb.reg12;
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x6012, m_reg12);
			}
			if (rb.reg18 != 0 && rb.reg18 != 0xFF)
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x1018, rb.reg18);
		}

		if (rb.ver >= REGS_BLOCK_VER_2)
		{
			if (rb.first_block_writen)
			{
				if (rb.reg12 > m_reg12)
					m_reg12 = rb.reg12;
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x6012, m_reg12);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x1018, rb.reg18);
			}

			if (rb.gain_block_writen)
			{
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x1002, rb.reg02[0]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x2002, rb.reg02[1]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x3002, rb.reg02[2]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x4002, rb.reg02[3]);
			}
		}
		else if (rb.ver >= REGS_BLOCK_VER_1)
		{
			if (rb.reg02[0] != 0 && rb.reg02[0] != 0xFF)
			{
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x1002, rb.reg02[0]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x2002, rb.reg02[1]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x3002, rb.reg02[2]);
				UT_SetSensorRegister(m_CurrentSensorHandle, 0x4002, rb.reg02[3]);
			}
		}
	}

	if (ver_num <= 0x0335)
		cali_reg12();
	else // (ver_num <= 0x0336)
		cali_reg12_for_fw0336();

#ifdef _WIN32
	once_for
	{
		int reg12_manual = hexstrtointeger((CT2A)ini.get_value(L"reg12_manual", L"Main"));
		if (reg12_manual)
		{
			dbg("ini file: reg12_manual=0x%X\n", reg12_manual);
			m_reg12 = reg12_manual;
			UT_SetSensorRegister(m_CurrentSensorHandle, 0x6012, m_reg12);
		}
	}
#endif

	UT_SetPower(device_handle, TRUE); //power saving mode.

	ret = IMD_RLT_SUCCESS;
	ret = background_calibration_fap50(device_handle);
	//UT_UsbSwtich(device_handle, ENABLE_USB_BOTH_OFF);
	if (ret != IMD_RLT_SUCCESS)
	{
		set_last_result(ret);
		dbg("Cali fail.\n");
		goto func_end;
	}

	ret = IMD_RLT_SUCCESS;

#if 0
	flat_img[FINGER_POSITION_BOTH_THUMBS] = imread("d:/442R_samples/20231229_183323_Both_Thumbs.png", IMREAD_GRAYSCALE);

	flat_img[FINGER_POSITION_LEFT_FOUR] = imread("d:/442R_samples/20231229_183323_Left_Four.png", IMREAD_GRAYSCALE);
	flat_img[FINGER_POSITION_LEFT_INDEX] = imread("d:/442R_samples/20231229_183323_Left_Index.png", IMREAD_GRAYSCALE);
	flat_img[FINGER_POSITION_LEFT_LITTLE] = imread("d:/442R_samples/20231229_183323_Left_Little.png", IMREAD_GRAYSCALE);
	flat_img[FINGER_POSITION_LEFT_MIDDLE] = imread("d:/442R_samples/20231229_183323_Left_Middle.png", IMREAD_GRAYSCALE);
	flat_img[FINGER_POSITION_LEFT_RING] = imread("d:/442R_samples/20231229_183323_Left_Ring.png", IMREAD_GRAYSCALE);
	flat_img[FINGER_POSITION_LEFT_THUMB] = imread("d:/442R_samples/20231229_183323_Left_Thumb.png", IMREAD_GRAYSCALE);

	flat_img[FINGER_POSITION_RIGHT_FOUR] = imread("d:/442R_samples/20231229_183323_Right_Four.png", IMREAD_GRAYSCALE);
	flat_img[FINGER_POSITION_RIGHT_INDEX] = imread("d:/442R_samples/20231229_183323_Right_Index.png", IMREAD_GRAYSCALE);
	flat_img[FINGER_POSITION_RIGHT_LITTLE] = imread("d:/442R_samples/20231229_183323_Right_Little.png", IMREAD_GRAYSCALE);
	flat_img[FINGER_POSITION_RIGHT_MIDDLE] = imread("d:/442R_samples/20231229_183323_Right_Middle.png", IMREAD_GRAYSCALE);
	flat_img[FINGER_POSITION_RIGHT_RING] = imread("d:/442R_samples/20231229_183323_Right_Ring.png", IMREAD_GRAYSCALE);
	flat_img[FINGER_POSITION_RIGHT_THUMB] = imread("d:/442R_samples/20231229_183323_Right_Thumb.png", IMREAD_GRAYSCALE);

	roll_img[FINGER_POSITION_LEFT_INDEX] = imread("d:/442R_samples/20231229_183323_Roll_Left_Index.png", IMREAD_GRAYSCALE);
	roll_img[FINGER_POSITION_LEFT_LITTLE] = imread("d:/442R_samples/20231229_183323_Roll_Left_Little.png", IMREAD_GRAYSCALE);
	roll_img[FINGER_POSITION_LEFT_MIDDLE] = imread("d:/442R_samples/20231229_183323_Roll_Left_Middle.png", IMREAD_GRAYSCALE);
	roll_img[FINGER_POSITION_LEFT_RING] = imread("d:/442R_samples/20231229_183323_Roll_Left_Ring.png", IMREAD_GRAYSCALE);
	roll_img[FINGER_POSITION_LEFT_THUMB] = imread("d:/442R_samples/20231229_183323_Roll_Left_Thumb.png", IMREAD_GRAYSCALE);
	roll_img[FINGER_POSITION_RIGHT_INDEX] = imread("d:/442R_samples/20231229_183323_Roll_Right_Index.png", IMREAD_GRAYSCALE);
	roll_img[FINGER_POSITION_RIGHT_LITTLE] = imread("d:/442R_samples/20231229_183323_Roll_Right_Little.png", IMREAD_GRAYSCALE);
	roll_img[FINGER_POSITION_RIGHT_MIDDLE] = imread("d:/442R_samples/20231229_183323_Roll_Right_Middle.png", IMREAD_GRAYSCALE);
	roll_img[FINGER_POSITION_RIGHT_RING] = imread("d:/442R_samples/20231229_183323_Roll_Right_Ring.png", IMREAD_GRAYSCALE);
	roll_img[FINGER_POSITION_RIGHT_THUMB] = imread("d:/442R_samples/20231229_183323_Roll_Right_Thumb.png", IMREAD_GRAYSCALE);
#endif

func_end:
	return ret;
}

bool IMDFap50Method::reset_FAP50()
{
	//if (_4202_mcu.m_handle==nullptr || _4202_mcu.m_handle==INVALID_HANDLE_VALUE)
	//	_4202_mcu.Open();

	//if (_4202_mcu.m_handle && _4202_mcu.m_handle!=INVALID_HANDLE_VALUE)
	//{
	//	vector<U8> v_in(9);
	//	//vector<U8> v_out{ 0xEF, 0x01, 0x01, 0x00, 0x06, 1, 1, 1, 1, 0x00, 0x00 };//put right 4
	//	vector<U8> v_out{ 0xEF, 0x01, 0x01, 0x00, 0x06, 6, 0, 0, 0, 0x00, 0x00 };

	//	U16 sum = 0;
	//	U8* p_sum = (U8*)&sum;
	//	int sum_idx = v_out.size() - 2;
	//	for (int i = 2; i < v_out.size() - 2; i++)
	//		sum += v_out[i];
	//	v_out[sum_idx] = p_sum[1];
	//	v_out[sum_idx + 1] = p_sum[0];

	//	C_Buffer out(v_out.size());
	//	memcpy(out.buf, v_out.data(), v_out.size());
	//	_4202_mcu.Phy_Out(&out);

	//	C_Buffer in(v_in.size());
	//	_4202_mcu.Phy_In(&in);
	//	dbg(L"reset FAP50 by 4202\n");
	//	 
	//	_4202_mcu.Close();
	//	Sleep(100);//delay 100ms for Reset FAP50 
	//}
	//else
	{
		DWORD dw = UT_ResetMcuUsb(device_handle);
		if (dw)
			return false;
	}

	return true;
}

bool IMDFap50Method::connect_usb_device(bool force_reset)
{
	static int is_busy = false;
	while (is_busy) Sleep(1);
	is_busy = TRUE;
	bool reset_fap50_done = false;

	if (force_reset)
		reset_fap50_done = reset_FAP50();

	bool bRet = false;
	for (int find_cnt = 0; find_cnt < 10; find_cnt++)
	{
		device_handle = nullptr;

		//LPCTSTR pszDevList[USB_BDG_MAX]{};
		//DWORD dwRet = UT_FindSensorDeviceList(pszDevList);
		//for (int i = 0; i < USB_BDG_MAX; i++)
		once_for
		{
			//if (pszDevList[i] == nullptr)
			//	continue;

			HANDLE hSensorNew = UT_ConnectSensor();// UT_ConnectSensorDevice(pszDevList[i]);
			if (hSensorNew)
			{
				bRet = TRUE;
				device_handle = hSensorNew;
				break;
			}
		}

		if (device_handle)
		{
			static READER_INFORMATION save_ver;

			READER_INFORMATION ver;
			DWORD ret = UT_GetUSBReaderInfo(device_handle, (BYTE*)&ver, sizeof(READER_INFORMATION));
			if(save_ver.FirmwareMinor != ver.FirmwareMinor)
			{
				dbg("FW: Type=%C Ver=%X.%02X\n", ver.VersionType, ver.FirmwareMajor, ver.FirmwareMinor);
				save_ver = ver;
			}

			reset_fap50_done = true;
			if (ver.VersionType != 'B' && ver.VersionType != 'G')
				reset_fap50_done = reset_FAP50();
		}

		if (reset_fap50_done)
			break;
	}

	is_busy = false;
	return bRet;
}

IMD_RESULT IMDFap50Method::get_system_property(SystemProperty* p)
{
	READER_INFORMATION ver;
	static BOOL is_success = FALSE;
	static vector<BYTE> sn(64), brand(64), model(64);
	DWORD ret = 0;
	WORD ver_num = 0;
	BOOL is_reader = FALSE;

	m_property.product_sn = (char*)sn.data();
	m_property.product_brand = (char*)brand.data();
	m_property.product_model = (char*)model.data();

	struct {
		BYTE minor : 4;
		BYTE major : 4;
	} reader_board_ver{};

	ZeroMemory(m_property.fw_ver, sizeof(m_property.fw_ver));

	ret = UT_GetUSBReaderInfo(device_handle, (BYTE*)&ver, sizeof(READER_INFORMATION));
	if (ret)
	{
		connect_usb_device();
		ret = UT_GetUSBReaderInfo(device_handle, (BYTE*)&ver, sizeof(READER_INFORMATION));
		if (ret)
			goto func_end;
	}

	m_property.fw_ver[1] = ver.FirmwareMajor;
	m_property.fw_ver[0] = ver.FirmwareMinor;
	m_property.fw_ver[2] = ver.VersionType;

	if (is_success)
		goto func_end;

	ret = UT_GetChipID(device_handle, &m_property.chip_id);
	if (ret)
	{
		//dbg(L"UT_GetChipID() fail.\n");
		goto func_end;
	}

	ret = UT_GetGuid(device_handle, m_property.guid, sizeof(m_property.guid));
	if (ret)
	{
		//dbg(L"UT_GetGuid() fail.\n");
		goto func_end;
	}

	ret = UT_ModulePara_SN(device_handle, 0, sn.data(), (int)sn.size());
	if (ret)
	{
		//dbg(L"UT_ModulePara_SN() fail.\n");
		goto func_end;
	}

	ret = UT_ModulePara_Brand(device_handle, 0, brand.data(), (int)brand.size());
	if (ret)
	{
		//dbg(L"UT_ModulePara_Brand() fail.\n");
		goto func_end;
	}

	ret = UT_ModulePara_Model(device_handle, 0, model.data(), (int)model.size());
	if (ret)
	{
		//dbg(L"UT_ModulePara_Model() fail.\n");
		goto func_end;
	}
	
	for (int i = 0; i < model.size()-1; i++)
		if (model[i + 1] == 0 && model[i] == 'R')
			is_reader = TRUE;

	ver_num = (ver.FirmwareMajor << 8) | ver.FirmwareMinor;
	if (ver_num >= 0x0328 && is_reader)
	{
		//ret = UT_Get_RB_Ver(device_handle, (BYTE*)&reader_board_ver);
		//dbg(L"RB ver:%d %d\n", reader_board_ver.major, reader_board_ver.minor);
		reader_board_ver.minor = 0xff;
		reader_board_ver.major = 0xff;
		if (ret)
		{
			//dbg(L"UT_Get_RB_Ver() fail.\n");
			goto func_end;
		}
		m_property.led_ver[0] = reader_board_ver.minor;
		m_property.led_ver[1] = reader_board_ver.major;
	}

	is_success = TRUE;
	//if (reader_board_ver.major == 0)
	//	is_success = FALSE;

	//dbg(L"get_system_property success.\n");
	//memcpy(p, &m_property, sizeof(m_property));

func_end:
	memcpy(p, &m_property, sizeof(m_property));
	return (is_success) ? IMD_RLT_SUCCESS : IMD_RLT_FAIL; 
}

IMD_RESULT IMDFap50Method::set_system_property(SystemProperty* p)
{
	if (p)
	{
		m_property.finger_position = p->finger_position;

		m_property.png_compress_ratio = min(max(p->png_compress_ratio, 0), 9);

		m_property.jp2_quality = min(max(p->jp2_quality, 0), 1000);
//		m_property.jp2_pix_per_inch = property->jp2_pix_per_inch;

		m_property.wsq_bit_rate = p->wsq_bit_rate;
//		m_property.wsq_bit_per_pix = property->wsq_bit_per_pix;
//		m_property.wsq_pix_per_inch = property->wsq_pix_per_inch;
		m_property.wsq_comment_text = p->wsq_comment_text;

		m_property.nfiq_ver = p->nfiq_ver;
		m_property.nfiq_score_minimum_acceptable = min(max(p->nfiq_score_minimum_acceptable, 1), 5);
		m_property.nfiq2_score_minimum_acceptable = min(max(p->nfiq2_score_minimum_acceptable, 0), 100);

		m_property.speech_en = p->speech_en;
		m_property.speech_volume = p->speech_volume;
		m_property.speech_language = p->speech_language;

		//UT_SetSpeechLanguage(device_handle, m_property.speech_language, m_is_reader);
		//UT_SetSpeechVolume(device_handle, m_property.speech_volume, m_is_reader);

		m_property.life_check_en = p->life_check_en;
		m_property.scan_timeout_ms = p->scan_timeout_ms;
		m_property.scan_by_manual = p->scan_by_manual;
	}

	switch (img_kind) {
	default:
	case IMG_KIND_NORMAL:
		enable_repair_line = true;
		enable_sw_auto_gain = true;
		enable_filter_for_rolling = true;
		enable_moving_avg = true;
		break;
	case IMG_KIND_RAW:
		enable_repair_line = false;
		enable_sw_auto_gain = false;
		enable_filter_for_rolling = false;
		enable_moving_avg = false;
		break;
	}

	return IMD_RLT_SUCCESS;
}

IMD_RESULT IMDFap50Method::scan_cancel()
{
#define STOP_TIMEOUT 3000

	enable_continue_scan = false;
	debounce_roll_cnt = 0;
	debounce_finger_width_cnt = 0;

	UINT64 t_timeout = (UINT64)timeGetTime() + STOP_TIMEOUT;
	while ((UINT64)timeGetTime() < t_timeout)
	{
		if (!is_scan_busy())
		{
			//dbg("scan_cancel (success)\n");
			return IMD_RLT_SUCCESS;
		}

		Sleep(1);
	}
	//dbg(L"After stop, device is not busy need to wait %d ms.\n", i);

	dbg("scan_cancel (fail)\n");
	set_last_result(IMD_RLT_STOP_SCAN_TIMEOUT);
	return IMD_RLT_STOP_SCAN_TIMEOUT;
}

bool IMDFap50Method::is_scan_busy()
{
	return is_thread_scan_busy || is_make_image_status_busy;
}

//@brief 判断一个点是否在一个矩形里
//@param PointpointP 点；.
//@param cv::Point pointA, cv::Point pointB. 直线的两个点
//@output distance
float get_distance_point2line(Point pointP, Point pointA, Point pointB)
{
	int A = 0, B = 0, C = 0;
	A = pointA.y - pointB.y;
	B = pointB.x - pointA.x;
	C = pointA.x * pointB.y - pointA.y * pointB.x;

	float distance = 0;
	distance = ((float)abs(A * pointP.x + B * pointP.y + C)) / ((float)sqrtf(A * A + B * B));
	return distance;
}

// 旋转点函数
Point2f rotate_point(Point2f& pointToRotate, Point2f& centerPoint, float angle) 
{
	float radAngle = angle * CV_PI / 180.0;
	float s = sin(radAngle);
	float c = cos(radAngle);

	// 平移到原点
	float x = pointToRotate.x - centerPoint.x;
	float y = pointToRotate.y - centerPoint.y;

	// 旋转点
	float xNew = x * c - y * s;
	float yNew = x * s + y * c;

	// 平移回原位置
	xNew += centerPoint.x;
	yNew += centerPoint.y;

	return Point2f(xNew, yNew);
}

void IMDFap50Method::check_hand_direction(ImageStatus* img_status)
{
	sort_finger(FINGER_POSITION_RIGHT_FOUR);

	double angle = (imd_contours[1].angle + imd_contours[2].angle) / 2;
	Point2f
		point_1 = Point2f(imd_contours[1].cntr.x, imd_contours[1].cntr.y),
		point_2 = Point2f(imd_contours[2].cntr.x, imd_contours[2].cntr.y),
		center = (point_1 + point_2) / 2,
		point_0 = Point2f(imd_contours[0].cntr.x, imd_contours[0].cntr.y),
		point_3 = Point2f(imd_contours[3].cntr.x, imd_contours[3].cntr.y),
		point_0_r = rotate_point(point_0, center, -angle),
		point_3_r = rotate_point(point_3, center, -angle);
	//TRACE(L"before [0](%d,%d), [3](%d,%d) after [0](%d,%d) [3](%d,%d) \n",
	//	(int)point_0.x,   (int)point_0.y,   (int)point_3.x,   (int)point_3.y,
	//	(int)point_0_r.x, (int)point_0_r.y, (int)point_3_r.x, (int)point_3_r.y
	//	);
	//TRACE("angle=%d\n", (int)angle);

	E_FINGER_POSITION real_pos = (point_0_r.y < point_3_r.y) ? FINGER_POSITION_RIGHT_FOUR : FINGER_POSITION_LEFT_FOUR;
	E_FINGER_POSITION pos = m_property.finger_position;
	if (real_pos != pos)
	{
		//img_status->result = IMD_RLT_PUT_WRONG_HAND;
		is_wrong_hand = true;
	}
}

IMD_RESULT IMDFap50Method::get_image_status(ImageStatus* p_status)
{
	//*p_status = ImageStatus{};

	if(m_image_status.img == nullptr)
	{
		//dbg("IMD_RLT_NO_ANY_SCAN_DONE\n");
		return IMD_RLT_NO_ANY_SCAN_DONE;
	}

	*p_status = m_image_status;
	m_image_status = ImageStatus{};
	return IMD_RESULT(0);
}

void IMDFap50Method::make_image_status()
{
	bool is_need_init = true;
	//dbg("make_image_status() start\n");make_image_status

while(enable_continue_scan)
{
	ImageStatus img_status{};
	if (scan_done == nullptr)
		continue;

	WAIT_SCAN_PARAMETER* pScan = (WAIT_SCAN_PARAMETER*)scan_done;
	IMD_RESULT ret = IMD_RLT_NO_ANY_SCAN_DONE;
	E_FINGER_POSITION pos = m_property.finger_position;
	ret = raw_to_img_fap50(pScan, &img_status);
	img_status.finger_position = m_property.finger_position;
	scan_done = nullptr;

	if (show_mode == GUI_SHOW_MODE_ROLL)
	{
		if (ret == IMD_RLT_SUCCESS)
		{
			rolling_finger_fap50(img_status.img, &img_status, is_need_init);
			is_need_init = false;
		}
		//dbg(L"is_roll_done=%d\n", img_status.is_roll_done);

		if (img_status.is_roll_init)
			get_finger_num(&img_status, pScan);
	}
	else if (show_mode == GUI_SHOW_MODE_SIGNATURE)
	{
		if (is_sign_clear)
		{
			is_sign_clear = false;
			signature(SIGNATURE_ACTION_CLEAR);
		}

		if (ret == IMD_RLT_SUCCESS)
			signature_2_fap50(img_status.img, &img_status);

		if (is_sign_done)
		{
			is_sign_done = false;
			save_to_memory(&img_status);
			//scan_cancel();
			enable_continue_scan = false;
		}

		goto set_result;
	}
	else if (show_mode == GUI_SHOW_MODE_SIGNATURE_BY_PEN)
	{
		if (is_sign_clear)
		{
			is_sign_clear = false;
			signature(SIGNATURE_ACTION_CLEAR);
		}

		if (ret == IMD_RLT_SUCCESS)
			signature_by_LP_5(img_status.img, &img_status);

		if (is_sign_done)
		{
			is_sign_done = false;
			save_to_memory(&img_status);
			//scan_cancel();
			enable_continue_scan = false;
		}

		goto set_result;
	}
	else
	{
		if(pos!= FINGER_POSITION_RIGHT_FULL && pos != FINGER_POSITION_LEFT_FULL)
			get_finger_num(&img_status, pScan);
	}

	{
		E_GUI_SHOW_MODE show_mode = img_status.show_mode;
		int finger_num = img_status.finger_num;
		E_FINGER_POSITION pos = m_property.finger_position;

		if (finger_num > 4)
			img_status.result = IMD_RLT_FINGER_NUM_OVER_FOUR;
		else if (finger_num == 4
			&& show_mode == GUI_SHOW_MODE_FLAT
			&& (FINGER_POSITION_RIGHT_FOUR <= pos && pos <= FINGER_POSITION_LEFT_FOUR)
			&& img_status.is_flat_done
			)
			check_hand_direction(&img_status);

		if ((finger_num != 4 && (FINGER_POSITION_RIGHT_FOUR <= pos && pos <= FINGER_POSITION_LEFT_FOUR))
			|| (finger_num != 2 && pos == FINGER_POSITION_BOTH_THUMBS)
			|| (finger_num != 1 && (FINGER_POSITION_RIGHT_THUMB <= pos && pos <= FINGER_POSITION_LEFT_LITTLE))
			|| (finger_num != 1 && img_status.is_roll_init)
			|| (finger_num > 4)
			|| (finger_num != select_fingers.size() && pos == FINGER_POSITION_SOME_FINGERS)
			)
		{
			if (debounce_finger_width_cnt)
				debounce_finger_width_cnt--;
			img_status.is_flat_done = false;
		}
	}

	if ((img_status.is_roll_done)// && img_status.is_finger_on == false)
		|| img_status.is_flat_done
		)
	{
		if (is_sampling_saved == false)
		{
			is_sampling_saved = true;
			save_to_memory(&img_status);
			for (auto& contour : imd_contours)
			{
				if (contour.valid == false)
					is_finger_shape_not_good = true;
			}

			if (pos != FINGER_POSITION_RIGHT_FULL && pos != FINGER_POSITION_LEFT_FULL)
				get_score(&img_status);
		}
	}

	if (img_status.result == IMD_RLT_FINGER_TOO_TOP && is_too_top == false)
	{
		//dbg(L"SPEECH_MOVE_DOWN_YOUR_FINGERS\n");
		is_too_top = true;

		switch (show_mode) {
		case GUI_SHOW_MODE_FLAT:
			switch (pos) {
			case FINGER_POSITION_RIGHT_FOUR:
				UT_LedSpeech(device_handle, LED_RIGHT_4_FINGERS, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_MOVE_DOWN_YOUR_FINGERS, m_is_reader);
				break;
			case FINGER_POSITION_LEFT_FOUR:
				UT_LedSpeech(device_handle, LED_LEFT_4_FINGERS, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_MOVE_DOWN_YOUR_FINGERS, m_is_reader);
				break;
			case FINGER_POSITION_BOTH_THUMBS:
				UT_LedSpeech(device_handle, SPEECH_PUT_2_THUMBS_FINGERS, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_MOVE_DOWN_YOUR_FINGERS, m_is_reader);
				break;
			}
		}
	}

	ret = IMD_RLT_SUCCESS;

	if (img_status.is_flat_done || img_status.is_roll_done)
		is_done = true;

	if (is_done) //keep value
	{
		img_status.is_flat_init = false;

		if(img_status.show_mode == GUI_SHOW_MODE_FLAT)
			img_status.is_flat_done = true;
		if (img_status.show_mode == GUI_SHOW_MODE_ROLL)
			img_status.is_roll_done = true;
	}

	if (img_status.is_flat_done && m_property.scan_by_manual)
	{
		//dbg("img_status.is_flat_done && m_property.scan_by_manual\n");
		save_to_memory(&img_status);
		get_score(&img_status);
	}

	if (need_finger_off)
	{
		if (img_status.is_finger_on && is_done)
		{
			if (is_alert_finger_off == false)
			{
				is_alert_finger_off = true;
				if (show_mode == GUI_SHOW_MODE_FLAT)
					UT_LedSpeech(device_handle, LED_ALL, LED_OFF, m_is_mute ? SPEECH_OFF : SPEECH_REMOVE_YOUR_FINGERS, m_is_reader);
				else
					UT_LedSpeech(device_handle, LED_ALL, LED_OFF, m_is_mute ? SPEECH_OFF : SPEECH_REMOVE_YOUR_FINGER, m_is_reader);
			}

			img_status.result = IMD_RLT_NEED_TO_TAKE_FINGER_OFF;

			debounce_finger_width_cnt = 0;
		}
	}

set_result:
	if (img_status.result == IMD_RLT_SUCCESS)
		img_status.result = get_last_result();

	//out_img = img_status.img;

	//{
	//	static int cnt = 0;
	//	dbg(L"scan_done clear %d\n", cnt++);
	//	imshow("mat_no_isp", mat_no_isp);
	//}
	
//func_end:
	m_image_status = img_status;
	//return ret;
}

	//dbg("make_image_status() end\n");
	is_make_image_status_busy = false;
}

IMD_RESULT IMDFap50Method::init_device()
{
	static int no_any_device = 0;
	if (!connect_usb_device(no_any_device > 0))
	{
		no_any_device++;
		dbg("IMD_RLT_CANT_FIND_ANY_DEVICE\n");
		return IMD_RLT_CANT_FIND_ANY_DEVICE;
	}

	no_any_device = 0;

	vector<BYTE> sn(64);
	UT_ModulePara_SN(device_handle, 0, sn.data(), (int)sn.size());

	m_is_reader = false, m_is_display = false, m_is_security = false;

	//older S/N rule
	// GF851ER025000000
	// GF851ER-D2500000
	// GF851EM-D2500000
	m_is_reader |= (sn[5] == 'E' && sn[6] == 'R');
	m_is_display |= (sn[7] == '-' && sn[8] == 'D');

	//new S/N rule
	// GF851ERD25000000
	// GF851EMD25000000
	// GF851EMDS2500000
	// GF851EMS25000000
	m_is_display |= (sn[7] == 'D');
	m_is_security |= ((sn[7] == 'S') || (sn[8] == 'S'));

	g_is_product_support_panel = m_is_display;

	return IMD_RLT_SUCCESS;
}

void IMDFap50Method::init_image()
{
	flat_img[FINGER_POSITION_RIGHT_FULL] = Mat(1000, 1600, CV_8UC1);
	flat_img[FINGER_POSITION_LEFT_FULL] = Mat(1000, 1600, CV_8UC1);
	flat_img[FINGER_POSITION_SOME_FINGERS] = Mat(1000, 1600, CV_8UC1);
	flat_img[FINGER_POSITION_SIGNATURE] = Mat(1000, 1600, CV_8UC1);
	flat_img[FINGER_POSITION_BOTH_THUMBS] = Mat(1000, 1600, CV_8UC1);

	flat_img[FINGER_POSITION_LEFT_FOUR] = Mat(1000, 1600, CV_8UC1);
	flat_img[FINGER_POSITION_LEFT_THUMB] = Mat(500, 400, CV_8UC1);	
	flat_img[FINGER_POSITION_LEFT_INDEX] = Mat(500, 400, CV_8UC1);
	flat_img[FINGER_POSITION_LEFT_LITTLE] = Mat(500, 400, CV_8UC1);
	flat_img[FINGER_POSITION_LEFT_MIDDLE] = Mat(500, 400, CV_8UC1);
	flat_img[FINGER_POSITION_LEFT_RING] = Mat(500, 400, CV_8UC1);

	flat_img[FINGER_POSITION_RIGHT_FOUR] = Mat(1000, 1600, CV_8UC1);
	flat_img[FINGER_POSITION_RIGHT_THUMB] = Mat(500, 400, CV_8UC1);	
	flat_img[FINGER_POSITION_RIGHT_INDEX] = Mat(500, 400, CV_8UC1);
	flat_img[FINGER_POSITION_RIGHT_LITTLE] = Mat(500, 400, CV_8UC1);
	flat_img[FINGER_POSITION_RIGHT_MIDDLE] = Mat(500, 400, CV_8UC1);
	flat_img[FINGER_POSITION_RIGHT_RING] = Mat(500, 400, CV_8UC1);


	roll_img[FINGER_POSITION_LEFT_THUMB] = Mat(750, 800, CV_8UC1);	
	roll_img[FINGER_POSITION_LEFT_INDEX] = Mat(750, 800, CV_8UC1);
	roll_img[FINGER_POSITION_LEFT_LITTLE] = Mat(750, 800, CV_8UC1);
	roll_img[FINGER_POSITION_LEFT_MIDDLE] = Mat(750, 800, CV_8UC1);
	roll_img[FINGER_POSITION_LEFT_RING] = Mat(750, 800, CV_8UC1);

	roll_img[FINGER_POSITION_RIGHT_THUMB] = Mat(750, 800, CV_8UC1);	
	roll_img[FINGER_POSITION_RIGHT_INDEX] = Mat(750, 800, CV_8UC1);
	roll_img[FINGER_POSITION_RIGHT_LITTLE] = Mat(750, 800, CV_8UC1);
	roll_img[FINGER_POSITION_RIGHT_MIDDLE] = Mat(750, 800, CV_8UC1);
	roll_img[FINGER_POSITION_RIGHT_RING] = Mat(750, 800, CV_8UC1);


	for (int i = 0; i < FINGER_POSITION_SIZE; i++)
	{
		if (i == 11 || i == 12)//no this finger pos
			continue;

		flat_img[i].setTo(Scalar(255));
		roll_img[i].setTo(Scalar(255));
	}

	ZeroMemory(roll_score, sizeof(roll_score));
	ZeroMemory(flat_score, sizeof(flat_score));
}

void IMDFap50Method::sort_finger(E_FINGER_POSITION position)
{
	//sort by finger_position (x direction)
	if (imd_contours.size())
	{
		//DWORD t_sort = timeGetTime();
		if (position == FINGER_POSITION_LEFT_FOUR) //The rightmost finger index is 0.
		{
			for (int i = 0; i < imd_contours.size() - 1; i++)
				for (int j = i + 1; j < imd_contours.size(); j++)
					if (imd_contours[i].cntr.x < imd_contours[j].cntr.x)
					{
						ImdDiamond tmp = imd_contours[i];
						imd_contours[i] = imd_contours[j];
						imd_contours[j] = tmp;
					}
		}
		else if (position == FINGER_POSITION_RIGHT_FOUR //The leftmost finger index is 0.
			|| position == FINGER_POSITION_BOTH_THUMBS
			|| position == FINGER_POSITION_SOME_FINGERS
			|| position == FINTER_POSITION_UNKNOW_FINGER
			)
		{
			for (int i = 0; i < imd_contours.size() - 1; i++)
				for (int j = i + 1; j < imd_contours.size(); j++)
					if (imd_contours[i].cntr.x > imd_contours[j].cntr.x)
					{
						ImdDiamond tmp = imd_contours[i];
						imd_contours[i] = imd_contours[j];
						imd_contours[j] = tmp;
					}
		}

		//t_sort = timeGetTime() - t_sort;
		//TRACE("sort:%d\n", t_sort);
	}
}

IMD_RESULT IMDFap50Method::get_finger_num(ImageStatus* img_status, void* pPara)
{
	lock_guard<mutex> lock(mu);

	DWORD t_start = timeGetTime();
	WAIT_SCAN_PARAMETER* scan = (WAIT_SCAN_PARAMETER*)pPara;
	E_HW_SCAN_MODE hw_scan_mode = (E_HW_SCAN_MODE)scan->hw_scan_mode;

	ImdContours _imd_contours;
	long area_max = 400 * 500L*2, area_min = 168 * 188;//168 * 226L;//IxB min area is ~150*150px < area 
	int th_white = image_white - 30;

	Rect roi(0, 0, m_Width, m_Height);
	if (hw_scan_mode == HW_SCAN_MODE_ROLL)
		roi = Rect(ROLL_X, ROLL_Y, ROLL_WIDTH, ROLL_HEIGHT);

	// 使用二值化方法將灰度圖像轉換為二值圖像
	//20230802_boxchu: threshold usage: https://shengyu7697.github.io/python-opencv-threshold/
	Mat thresh_img; //threshold(blur(roi), thresh_img, th_white, 255, THRESH_BINARY);
	
	GaussianBlur(mat_avg, thresh_img, Size(25, 25), 0, 0, BORDER_DEFAULT);
	threshold(thresh_img(roi), thresh_img, 250, 255, THRESH_BINARY);
	//morphologyEx(thresh_img, thresh_img, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT, { 3, 3 }));


//#define DEBUG_FINGER_NUM
#define DEBUG_WINDOW_NAME "Image Display"
#ifdef DEBUG_FINGER_NUM
	Mat drawing;  
	//cvtColor(mat_no_isp(roi), drawing, COLOR_GRAY2BGR);//dbg
	cvtColor(thresh_img(roi), drawing, COLOR_GRAY2BGR);//dbg
	//cvtColor(mat_isp(roi), drawing, COLOR_GRAY2BGR);//dbg
	//cvtColor(mat_avg(roi), drawing, COLOR_GRAY2BGR);//dbg

	static bool debug_image_window = true;
	if (debug_image_window)
	{
		debug_image_window = false;
		namedWindow(DEBUG_WINDOW_NAME, WINDOW_NORMAL);
		HWND hwnd = FindWindow(0, _T(DEBUG_WINDOW_NAME));
		if (hwnd != 0)
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, drawing.cols, drawing.rows, 0);
	}
#endif

	if (img_status->result != IMD_RLT_SUCCESS)
		goto func_end;

	if (debounce_finger_width_cnt == 0)
		goto func_end;

	//邊緣清成空白
	thresh_img.row(0).setTo(Scalar(255));
	thresh_img.row(thresh_img.rows - 1).setTo(Scalar(255));
	thresh_img.col(0).setTo(Scalar(255));
	thresh_img.col(thresh_img.cols - 1).setTo(Scalar(255));

	findContours(thresh_img, contours,
		RETR_LIST,				//RETR_LIST, 檢視所有輪廓，不建立父子關係
		CHAIN_APPROX_SIMPLE);	//CHAIN_APPROX_SIMPLE, 指記錄拐點訊息，不記錄值線段的。

	for(int i=0; i<contours.size(); i++)
	{
		auto& contour = contours[i];

		if (contour.size() < 300)
			continue;

		long contour_area = (long)contourArea(contour);
		if (contour_area<area_min || area_max<contour_area)
			continue;

		//cv::Rect rect = cv::boundingRect(contour);
		//float aspect = float(rect.width) / rect.height;
		//if (aspect < 0.3 || aspect > 3.0)
		//{
		//	dbg("過細或過扁不要\n");
		//	continue; 
		//}

		ImdDiamond imd_contour{};

		// 計算輪廓的主軸（PCA）
		//refer to URL: https://docs.opencv.org/3.4/d1/dee/tutorial_introduction_to_pca.html
		Mat data_pts = Mat((int)contour.size(), 2, CV_64F);
		for (int i = 0; i < data_pts.rows; i++)
		{
			data_pts.at<double>(i, 0) = contour[i].x;
			data_pts.at<double>(i, 1) = contour[i].y;
		}
		PCA pca_analysis(data_pts, noArray(), PCA::DATA_AS_ROW);

		//計算輪廓的中心
		Point2d cntr = Point((pca_analysis.mean.at<double>(0, 0)), (pca_analysis.mean.at<double>(0, 1)));

		// 主要轴向量为PCA的第一个特征向量
		Point2d majorAxis(pca_analysis.eigenvectors.at<double>(0, 0),	pca_analysis.eigenvectors.at<double>(0, 1));
		double majorAxisLength = pca_analysis.eigenvalues.at<double>(0);

		/*
			想得到輪廓與主軸LM的交點 PA, PB（PA在上方，PB在下方）。
			從質心PC與PA的方向延伸30個單位長得點PA1，
			並畫出一條垂直於LM的線LT，並相交於點PA1。LT的線長為400 並以PA1點為中心，
			故得兩點PL1, PL2。 再延伸500單位得兩點PB3, PB4。
		*/
		Point2d centroid = cntr;
		Point2d PA = centroid - (majorAxis * majorAxisLength * 0.02);
		Point2d PA_tmp = centroid + (majorAxis * majorAxisLength * 0.02);
		if (PA_tmp.y < PA.y)
			PA = PA_tmp;

#ifdef DEBUG_FINGER_NUM
		drawContours(drawing, contours, i, CV_RGB(0, 0, 255), 2);
		line(drawing, PA, cntr, CV_RGB(255, 0, 0), 2);
		//imshow(DEBUG_WINDOW_NAME, drawing);
		//waitKey(1);
#endif

		//找到确切的交点位置
		//bool find_a = false;
		//deque <float> delta_rec;
		{
			float m=0, c=0, delta_min=500000; //直線 y=mx+c
			Point2f A = cntr, B = PA;
			if (fabs(B.x - A.x) > 0.00001)
				m = (B.y - A.y) / (B.x - A.x);	// 斜率 m

			c = A.y - m * A.x;					// y 軸截距 c
			PA = cntr;

			for (auto& pt : contour)
			{
				if (pt.y > cntr.y)//-60)
					continue;

#ifdef DEBUG_FINGER_NUM
				//line(drawing, pt, pt, CV_RGB(128, 128, 128), 3);
#endif
				float delta = fabs(pt.y - (m * pt.x + c));
				//delta_rec.push_back(delta);
				if (delta_min > delta)// || pt.y < 1 )
				{
					//find_a = true;
					PA = pt;
					delta_min = delta;
#ifdef DEBUG_FINGER_NUM
					//line(drawing, pt, pt, CV_RGB(255, 255, 0), 12);
#endif
				}
			}

			//if (m)
			//	PA.x = (PA.y - c) / m;
			//else
			//	PA.x = PA.y - c;

			//if (PA.y == cntr.y)
			//	dbg("(PA.y == cntr.y)\n");
			//if (find_a == false)
			//	dbg("find_a == false\n");
		}

#ifdef DEBUG_FINGER_NUM
		//imd_contour.w = { cntr.x, cntr.y };
		//imd_contour.g = { PA.x, PA.y };
		line(drawing, cntr, cntr, CV_RGB(255, 255, 255), 16);
		line(drawing, PA, PA, CV_RGB(0, 255, 0), 16);
		//imshow(DEBUG_WINDOW_NAME, drawing);
		//waitKey(10);
#endif
		if (PA.y == cntr.y)
			continue;

		Point2d pa_vec = (centroid - PA) / norm(centroid - PA);
		//Point2d PA1 = PA - 3 * pa_vec;
		Point2d PA1 = PA - 15.0 * pa_vec;
		Point2d PL1, PL2;
		PL1 = PA1 - Point2d(majorAxis.y, -majorAxis.x) * 400.0 / 2;
		PL2 = PA1 + Point2d(majorAxis.y, -majorAxis.x) * 400.0 / 2;

		Point2d PB3 = PL1 + 500.0 * pa_vec;
		Point2d PB4 = PL2 + 500.0 * pa_vec;

		imd_contour.pa = ImdPoint2d{PL1.x, PL1.y};
		imd_contour.pb = ImdPoint2d{PL2.x, PL2.y};
		imd_contour.pc = ImdPoint2d{PB4.x, PB4.y};
		imd_contour.pd = ImdPoint2d{PB3.x, PB3.y};
		//imd_contour.rotate_box = rotate_box;
		Point2d pa = Point2d(imd_contour.pa.x, imd_contour.pa.y),
			pb = Point2d(imd_contour.pb.x, imd_contour.pb.y),
			pc = Point2d(imd_contour.pc.x, imd_contour.pc.y),
			pd = Point2d(imd_contour.pd.x, imd_contour.pd.y),
			center = (pa + pb + pc + pd) / 4;// 计算矩形的中心点
		imd_contour.cntr = ImdPoint2d{center.x, center.y};//ImdPoint2d{ cntr.x, cntr.y };

		// 计算旋转角度
		double angle = atan2(-(pa.y - pd.y), pa.x - pd.x) * 180.0 / CV_PI;
		if (angle > 90)
			angle = -(angle - 90);
		else
			angle = 90 - angle;

		imd_contour.valid = (abs(int(angle)) < 55);
		//dbg("angle > 55\n");

		imd_contour.angle = angle;
		_imd_contours.push_back(imd_contour);
	}

#ifdef DEBUG_FINGER_NUM
	imshow(DEBUG_WINDOW_NAME, drawing);
	waitKey(10);
#endif

	//{
	//	BYTE* img = thresh_img.data;
	//	for (int i = 2; i < IMAGE_WIDTH - 2 * 2; i++) //check image top line 
	//		if (img[i] < 180)
	//		{
	//			img_status->result = IMD_RLT_FINGER_TOO_TOP;
	//			//debounce_finger_width_cnt = 0;
	//			//dbg("IMD_RLT_FINGER_TOO_TOP\n");
	//			break;
	//		}
	//}

	//刪除非第一指節的指紋
	if ( _imd_contours.size() > 1)
	{
		//dbg("_imd_contours.size() = %d\n", _imd_contours.size());

		for (int i = 0; i < _imd_contours.size() - 1; i++) //sort by y, Sort from largest to smallest
			for (int j = i + 1; j < _imd_contours.size(); j++)
				if (_imd_contours[i].cntr.y < _imd_contours[j].cntr.y)
				{
					ImdDiamond tmp = _imd_contours[i];
					_imd_contours[i] = _imd_contours[j];
					_imd_contours[j] = tmp;
				}
		
		BOOL restart = FALSE;
		for (ImdContours::iterator point = _imd_contours.begin(); point != _imd_contours.end()-1; point++)
		{
			if (restart == TRUE)
			{
				point = _imd_contours.begin();
				restart = FALSE;
			}

			ImdDiamond rect_p = *point;
			Point2d pa = Point2d(rect_p.pa.x, rect_p.pa.y),
				pb = Point2d(rect_p.pb.x, rect_p.pb.y),
				pc = Point2d(rect_p.pc.x, rect_p.pc.y),
				pd = Point2d(rect_p.pd.x, rect_p.pd.y),
				center = (pa + pb + pc + pd) / 4;
			
			for (ImdContours::iterator line= point+1; line != _imd_contours.end(); line++)
			{
				ImdDiamond rect_l = *line;
				Point2d pa = Point2d(rect_l.pa.x, rect_l.pa.y),
					pb = Point2d(rect_l.pb.x, rect_l.pb.y),
					pc = Point2d(rect_l.pc.x, rect_l.pc.y),
					pd = Point2d(rect_l.pd.x, rect_l.pd.y),
					top = (pa + pb) / 2,
					bottom = (pc + pd) / 2;

				int distance = (int)get_distance_point2line(center, top, bottom);
				//TRACE("distance of point(%d, %d) to line(%d,%d) is %d\n", 
				//	(int)center.x, (int)center.y, (int)top.x, (int)top.y, distance);
				if (distance < 100)
				{
					_imd_contours.erase(point);
					point = _imd_contours.begin();//reset
					restart = TRUE;
					break;
				}
			}

			if (_imd_contours.size() < 2)
				break;
		}
	}

func_end:
	imd_contours = _imd_contours;
	img_status->contours = imd_contours.data();
	img_status->finger_num = (int)imd_contours.size();
	//	dbg(L"elapsed=%d finger_num=%d\n", timeGetTime()- t_start, imd_contours.size());
	//dbg(L"finger_num=%d\n", imd_contours.size());

	return IMD_RLT_SUCCESS;
}

static void set_png_dpi (string file_path, SystemProperty* property)
{
	ifstream file(file_path, std::ios::binary);
	if (!file) {
		dbg("Error: Unable to open file\n");
		return;
	}

	file.seekg(0, std::ios::end);
	int len = static_cast<int>(file.tellg());
	file.seekg(0, std::ios::beg);
	std::vector<char> buf(len);
	file.read(buf.data(), len);  // 读取文件数据

	int new_dpi = property->image_pix_per_inch * 3937 /100; //inch to meter
	char* pResolution = (char*)&new_dpi;
	int crc = 0x75CEF095; //crc 0x75CEF095 just for 500dpi
	char* pCrc = (char*)&crc;

	//PNG图片pHYs模块可能有可能没有，如果有就改写，无就增加 70 48 59 73
	//pHYs模块存在位置0x25 0x26 0x27 0x28
	int len2 = len + 21;
	std::vector<char> buf_copy(len2);  // 插入pHYs模块

	if (len2 < 0x30) //图像内存异常
		return ;

	if (buf[0x25] != 0x70 || buf[0x26] != 0x48 || buf[0x27] != 0x59 || buf[0x28] != 0x73) //不存在pHYs块
	{
		std::copy(buf.begin(), buf.begin() + 0x21, buf_copy.begin());

		buf_copy[0x21] = 0;
		buf_copy[0x22] = 0;
		buf_copy[0x23] = 0;
		buf_copy[0x24] = 9;    //pHYs模块固定，长度位9位

		buf_copy[0x25] = 0x70;
		buf_copy[0x26] = 0x48;
		buf_copy[0x27] = 0x59;
		buf_copy[0x28] = 0x73; //pHYs块头

		//四位横向分辨率 23622像素/米 600像素/英寸换算
		buf_copy[0x29] = pResolution[3];
		buf_copy[0x2A] = pResolution[2];
		buf_copy[0x2B] = pResolution[1];
		buf_copy[0x2C] = pResolution[0];

		//四位纵向分辨率 23622像素/米 600像素/英寸换算
		buf_copy[0x2D] = pResolution[3];
		buf_copy[0x2E] = pResolution[2];
		buf_copy[0x2F] = pResolution[1];
		buf_copy[0x30] = pResolution[0];

		buf_copy[0x31] = 1;  //表示定义单位为米

		buf_copy[0x32] = pCrc[3];
		buf_copy[0x33] = pCrc[2];
		buf_copy[0x34] = pCrc[1];
		buf_copy[0x35] = pCrc[0]; //CRC值 500dpi的循环冗余校验码

		std::copy(buf.begin() + 0x21, buf.end(), buf_copy.begin() + 0x35 + 0x21 - 0x20);
		std::ofstream out_file(file_path, std::ios::binary);
		out_file.write(buf_copy.data(), buf_copy.size());
	}
	else
	{
		//四位横向分辨率 23622像素/米 600像素/英寸换算
		buf[0x29] = pResolution[3];
		buf[0x2A] = pResolution[2];
		buf[0x2B] = pResolution[1];
		buf[0x2C] = pResolution[0];

		//四位纵向分辨率 23622像素/米 600像素/英寸换算
		buf[0x2D] = pResolution[3];
		buf[0x2E] = pResolution[2];
		buf[0x2F] = pResolution[1];
		buf[0x30] = pResolution[0];

		buf[0x31] = 1;  //表示定义单位为米

		buf_copy[0x32] = pCrc[3];
		buf_copy[0x33] = pCrc[2];
		buf_copy[0x34] = pCrc[1];
		buf_copy[0x35] = pCrc[0]; //CRC值 500dpi的循环冗余校验码

		// 将文件指针移动回到文件开头并写入原始数据
		std::ofstream out_file(file_path, std::ios::binary);
		out_file.write(buf.data(), buf.size());
	}

}

static void set_bmp_dpi (string file_path, SystemProperty* property)
{
#pragma pack(push, 1)
struct lbheader {
	unsigned short identifier;      // 0x0000
	unsigned int filesize;          // 0x0002
	unsigned int reserved;          // 0x0006
	unsigned int bitmap_dataoffset; // 0x000A
	unsigned int bitmap_headersize; // 0x000E
	unsigned int width;             // 0x0012
	unsigned int height;            // 0x0016
	unsigned short planes;          // 0x001A
	unsigned short bits_perpixel;   // 0x001C
	unsigned int compression;       // 0x001E
	unsigned int bitmap_datasize;   // 0x0022
	unsigned int hresolution;       // 0x0026
	unsigned int vresolution;       // 0x002A
	unsigned int usedcolors;        // 0x002E
	unsigned int importantcolors;   // 0x0032
	unsigned int palette;           // 0x0036
} ;
#pragma pack(pop)
	lbheader hbmp;

	// 打开文件，读取头信息
	std::fstream file(file_path, std::ios::binary | std::ios::in | std::ios::out);
	if (!file) {
		dbg("Error: Unable to open file \n");
		return;
	}

	//fread(&hbmp, sizeof(BYTE), header_size, file);
	file.read(reinterpret_cast<char*>(&hbmp), sizeof(hbmp));
	if (!file){
		dbg("Error: Failed to read file header \n");
		return;
	}

	int new_dpi = property->image_pix_per_inch * 3937 / 100; //inch to meter
	hbmp.bitmap_datasize = hbmp.width * hbmp.height;
	hbmp.hresolution = new_dpi;
	hbmp.vresolution = new_dpi;
	hbmp.usedcolors = 256;

	// 将文件指针移动回文件开头，并写入更新后的头部信息
	file.seekp(0, std::ios::beg);  // 移动写入指针到文件开头
	file.write(reinterpret_cast<char*>(&hbmp), sizeof(hbmp));
	if (!file) 
		dbg("Error: Failed to write updated header \n");

	file.close();
}

//#ifdef __linux
uint16_t byteswap_ushort(uint16_t value) {
    return (value >> 8) | (value << 8);
}

uint32_t byteswap_ulong(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >>  8) & 0x0000FF00) |
           ((value <<  8) & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}
//#endif

#pragma pack(push, 1)
struct FirFileHeader { //ISO/IEC 19794-4:2011 
	DWORD format_id;		//"FIR\0" 
	DWORD version_num;		//"020\0" 
	DWORD length_record;	//image_data_size + 57 
	WORD num_of_finger_representations; //0x0001 
	BYTE certification_flag; //0 
	BYTE num_distinct_finger_positions; //1 

	DWORD representation_length; //image_data_size + 41
	struct { //date time, 9 bytes (ISO 19794-1:2011)
		WORD year;
		BYTE month;
		BYTE day;
		BYTE hour;
		BYTE min;
		BYTE sec;
		WORD ms;
	};
	BYTE capture_tech_id; //0 (UNKNOWN=0) 
	WORD capture_device_vendor_id; //0 (UNKNOWN=0)
	WORD capture_device_id; //0 (UNKNOWN=0)
	BYTE num_quality_blocks; //0 
	BYTE finger_position; //0 (UNKNOWN=0) (19794-2:2005)
	BYTE representation_num; //0
	BYTE scale_units; //(SCALE_UNITS_PPI=1, SCALE_UNITS_PPCM=2) 
	WORD scan_horiz_resolution; //500 
	WORD scan_vert_resolution; //500 
	WORD image_horiz_resolution; //500 
	WORD image_vert_resolution;//500 
	BYTE pixel_depth; //8 
	BYTE image_compression_alg; //0  (COMPRESSION_UNCOMPRESSED_NO_BIT_PACKING=0) 
	BYTE impression_type; // 	LIVE_PLAIN=0, LIVE_ROLLED=1, UNKNOWN=29 (ISO 19794-2:2005)
	WORD image_width; //width
	WORD image_height; //height
	DWORD image_data_size; //data_size = width*height if image is raw.
} ;
#pragma pack(pop)
void IMDFap50Method::save_to_fir (string file_path, Mat src)
{
	SystemProperty* property = &m_property;

	std::vector<uint8_t> a_fir_buf(sizeof(FirFileHeader)+ src.total(), 0);
	FirFileHeader* fir_buf = reinterpret_cast<FirFileHeader*>(a_fir_buf.data());

	std::copy("FIR", "FIR" + 3, &fir_buf->format_id);
	std::copy("020", "020" + 3, &fir_buf->version_num);

	DWORD length_record = (int)src.total() + 57 ;
	fir_buf->length_record = byteswap_ulong(length_record);

	WORD num_of_finger_representations = 1; //0x0001 
	fir_buf->num_of_finger_representations = byteswap_ushort(num_of_finger_representations);
	fir_buf->num_distinct_finger_positions = 1; //1 

	DWORD representation_length = (int)src.total() + 41;
	fir_buf->representation_length = byteswap_ulong(representation_length);

	auto now = chrono::system_clock::now();
	time_t current_time = chrono::system_clock::to_time_t(now);
	tm local_tm;
#ifdef _MSC_VER
	localtime_s(&local_tm, &current_time);  // Visual Studio
#else
	local_tm = *localtime(&current_time);  // GCC / Clang
#endif
	fir_buf->year =	local_tm.tm_year + 1900;  // tm_year 是从 1900 开始的
	fir_buf->month =local_tm.tm_mon + 1;       // tm_mon 从 0 开始
	fir_buf->day =	local_tm.tm_mday;
	fir_buf->hour =	local_tm.tm_hour;
	fir_buf->min = local_tm.tm_min;
	fir_buf->sec = local_tm.tm_sec;
	fir_buf->ms = 0;

	fir_buf->finger_position = finger_position_code;
	fir_buf->scale_units = 1;//ppi

	WORD dpi = property->image_pix_per_inch;
	fir_buf->scan_horiz_resolution = byteswap_ushort(dpi);
	fir_buf->scan_vert_resolution = byteswap_ushort(dpi); 
	fir_buf->image_horiz_resolution = byteswap_ushort(dpi); 
	fir_buf->image_vert_resolution = byteswap_ushort(dpi);

	fir_buf->pixel_depth = property->image_bit_per_pix;

	BYTE impression_type = 29; //29 is unknow
	//if (show_mode == GUI_SHOW_MODE_ROLL)
	//	impression_type = 1;
	fir_buf->impression_type = impression_type;

	WORD horiz_line_length = src.cols;
	fir_buf->image_width = byteswap_ushort(horiz_line_length);
	WORD vertical_line_length = src.rows;
	fir_buf->image_height = byteswap_ushort(vertical_line_length);
	DWORD image_data_length = (int)src.total();
	fir_buf->image_data_size = byteswap_ulong(image_data_length);

	std::memcpy(a_fir_buf.data() + sizeof(FirFileHeader), src.data, src.total());

	std::ofstream fir_file(file_path, std::ios::binary);
	if (!fir_file.is_open()) 
		return;

	fir_file.write((char*)a_fir_buf.data(), a_fir_buf.size());
	fir_file.close();
}

bool is_all_white(const Mat& image) 
{
	// 對於多通道，計算每個通道的總和
	cv::Scalar totalSum = cv::sum(image);
	double maxPossibleSum = 255 * image.total() * image.channels();

	// 檢查總和是否與理論最大值匹配
	return totalSum[0] + totalSum[1] + totalSum[2] == maxPossibleSum;
}

IMD_STD_API void save_wsq(void* pSrc, const char* file_path, SystemProperty* property);
void IMDFap50Method::mat_to_file (string& file_path, Mat src, E_IMG_TYPE img_type)
{
	SystemProperty* property = &m_property;
	if (src.channels() == 1)
		if (is_all_white(src))
		{
			//dbg(L"file_path=%s is all white.\n", file_path);
			return;
		}

	string file_full_path = file_path;
	size_t lastDotIndex = file_full_path.find_last_of('.');
	string fileNameWithoutExtension = file_path;
	if (lastDotIndex != string::npos)
		fileNameWithoutExtension = file_path.substr(0, lastDotIndex);

	string str_filename;
	switch (img_type) {
	default:
	case IMG_TYPE_BMP:
	{
		str_filename = format_to_string("%s.%s", fileNameWithoutExtension.c_str(), "bmp");
		imwrite(str_filename, src);
		set_bmp_dpi(str_filename, property);
		break;
	}
	case IMG_TYPE_PNG:
	{
		str_filename = format_to_string("%s.%s", fileNameWithoutExtension.c_str(), "png");

		// 指定 PNG 格式参数
		vector<int> compression_params;
		compression_params.push_back(IMWRITE_PNG_COMPRESSION);
		compression_params.push_back(property->png_compress_ratio); //0~9
		if (src.rows && src.cols)
		{
			//bool result = 
			imwrite(str_filename, src, compression_params);
			set_png_dpi(str_filename, property);
		}
		break;
	}
	case IMG_TYPE_RAW:
	{
		int width = src.cols;
		int	height = src.rows;
		int bit_per_pix = property->image_bit_per_pix;
		int ppi = property->image_pix_per_inch;

		str_filename = format_to_string("%s_%dx%dWxH_%dBit_%dPPI.%s", fileNameWithoutExtension.c_str(), width, height, bit_per_pix, ppi, "raw");
		std::ofstream raw_file(str_filename, std::ios::binary);
		if (!raw_file.is_open())
			return;

		raw_file.write((char*)src.data, src.total());
		raw_file.close();

		//if (elementsWritten != dataSize) 
		//	return ;
		
		break;
	}
	case IMG_TYPE_JP2:
	{
		vector<int> compression_params;
		compression_params.push_back(IMWRITE_JPEG2000_COMPRESSION_X1000); // 通常設為 0-1000，值越高品質越好
		compression_params.push_back(property->jp2_quality); // 在這裡設定品質 (0-1000) 

		str_filename = format_to_string("%s.%s",fileNameWithoutExtension.c_str(), "jp2");
		imwrite(str_filename, src, compression_params);
		break;
	}
	case IMG_TYPE_FIR:
	{
		str_filename = format_to_string("%s.%s", fileNameWithoutExtension.c_str(), "fir");
		save_to_fir(str_filename, src);
		break;
	}
	case IMG_TYPE_WSQ:
		str_filename = format_to_string("%s.%s", fileNameWithoutExtension.c_str(), "wsq");
		save_wsq(&src, str_filename.c_str(), property);
		break;
	case IMG_TYPE_PGM:
		str_filename = format_to_string("%s.%s", fileNameWithoutExtension.c_str(), "pgm");
		bool result = imwrite(str_filename, src);
		break;
	}
}

void IMDFap50Method::get_finger_position (int finger_num=-1)
{
	finger_position_code = m_property.finger_position;

	switch (finger_position_code) {
	case FINGER_POSITION_RIGHT_THUMB: finger_position_text = "Right_Thumb"; break;
	case FINGER_POSITION_RIGHT_INDEX: finger_position_text = "Right_Index"; break;
	case FINGER_POSITION_RIGHT_MIDDLE: finger_position_text = "Right_Middle"; break;
	case FINGER_POSITION_RIGHT_RING: finger_position_text = "Right_Ring"; break;
	case FINGER_POSITION_RIGHT_LITTLE: finger_position_text = "Right_Little"; break;

	case FINGER_POSITION_LEFT_THUMB: finger_position_text = "Left_Thumb"; break;
	case FINGER_POSITION_LEFT_INDEX: finger_position_text = "Left_Index"; break;
	case FINGER_POSITION_LEFT_MIDDLE: finger_position_text = "Left_Middle"; break;
	case FINGER_POSITION_LEFT_RING: finger_position_text = "Left_Ring"; break;
	case FINGER_POSITION_LEFT_LITTLE: finger_position_text = "Left_Little"; break;

	case FINGER_POSITION_SOME_FINGERS: 
		if (finger_num == -1)
		{
			finger_position_text = "Some_Fingers";
			break;
		}

		switch (select_fingers[finger_num]) {
		case FINGER_POSITION_RIGHT_THUMB: finger_position_text = "Right_Thumb"; finger_position_code = FINGER_POSITION_RIGHT_THUMB; break;
		case FINGER_POSITION_RIGHT_INDEX: finger_position_text = "Right_Index"; finger_position_code = FINGER_POSITION_RIGHT_INDEX; break;
		case FINGER_POSITION_RIGHT_MIDDLE: finger_position_text = "Right_Middle"; finger_position_code = FINGER_POSITION_RIGHT_MIDDLE; break;
		case FINGER_POSITION_RIGHT_RING: finger_position_text = "Right_Ring"; finger_position_code = FINGER_POSITION_RIGHT_RING; break;
		case FINGER_POSITION_RIGHT_LITTLE: finger_position_text = "Right_Little"; finger_position_code = FINGER_POSITION_RIGHT_LITTLE; break;

		case FINGER_POSITION_LEFT_THUMB: finger_position_text = "Left_Thumb"; finger_position_code = FINGER_POSITION_LEFT_THUMB; break;
		case FINGER_POSITION_LEFT_INDEX: finger_position_text = "Left_Index"; finger_position_code = FINGER_POSITION_LEFT_INDEX; break;
		case FINGER_POSITION_LEFT_MIDDLE: finger_position_text = "Left_Middle"; finger_position_code = FINGER_POSITION_LEFT_MIDDLE; break;
		case FINGER_POSITION_LEFT_RING: finger_position_text = "Left_Ring"; finger_position_code = FINGER_POSITION_LEFT_RING; break;
		case FINGER_POSITION_LEFT_LITTLE: finger_position_text = "Left_Little"; finger_position_code = FINGER_POSITION_LEFT_LITTLE; break;
		default:
			finger_position_text = format_to_string("Unknow_Finger_%d", finger_num);
			finger_position_code = FINTER_POSITION_UNKNOW_FINGER;
		}
		break;

	case FINGER_POSITION_RIGHT_FOUR:
		switch (finger_num) {
		case 0: finger_position_text = "Right_Index"; finger_position_code = FINGER_POSITION_RIGHT_INDEX; break;
		case 1: finger_position_text = "Right_Middle"; finger_position_code = FINGER_POSITION_RIGHT_MIDDLE; break;
		case 2: finger_position_text = "Right_Ring"; finger_position_code = FINGER_POSITION_RIGHT_RING; break;
		case 3: finger_position_text = "Right_Little"; finger_position_code = FINGER_POSITION_RIGHT_LITTLE; break;
		case -1: finger_position_text = "Right_Four"; break;
		default:
			finger_position_text = format_to_string("Unknow_Finger_%d", finger_num);
			finger_position_code = FINTER_POSITION_UNKNOW_FINGER;
		}
		break;

	case FINGER_POSITION_LEFT_FOUR:
		switch (finger_num) {
		case 0: finger_position_text = "Left_Index"; finger_position_code = FINGER_POSITION_LEFT_INDEX; break;
		case 1: finger_position_text = "Left_Middle"; finger_position_code = FINGER_POSITION_LEFT_MIDDLE; break;
		case 2: finger_position_text = "Left_Ring"; finger_position_code = FINGER_POSITION_LEFT_RING; break;
		case 3: finger_position_text = "Left_Little"; finger_position_code = FINGER_POSITION_LEFT_LITTLE; break;
		case -1: finger_position_text = "Left_Four"; break;
		default:
			finger_position_text = format_to_string("Unknow_Finger_%d", finger_num);
			finger_position_code = FINTER_POSITION_UNKNOW_FINGER;
		}
		break;

	case FINGER_POSITION_BOTH_THUMBS:
		switch (finger_num) {
		case 0: finger_position_text = "Left_Thumb"; finger_position_code = FINGER_POSITION_LEFT_THUMB; break;
		case 1: finger_position_text = "Right_Thumb"; finger_position_code = FINGER_POSITION_RIGHT_THUMB; break;
		case -1: finger_position_text = "Both_Thumbs"; break;
		default:
			finger_position_text =format_to_string("Unknow_Finger_%d", finger_num);
			finger_position_code = FINTER_POSITION_UNKNOW_FINGER;
		}
		break;

	default:
	case FINTER_POSITION_UNKNOW_FINGER:
		finger_position_text = "Unknow_Finger";
		if (finger_num > -1)
		{
			finger_position_text = format_to_string("Unknow_Finger_%d", finger_num);
			finger_position_code = (E_FINGER_POSITION)(finger_num+1);
		}
	}
}

//去除臨近的指紋
void erase_other_finger(Mat& base_img)
{
	DWORD t = timeGetTime();

	// base ----------------------------------------------------------------------
	vector<vector<Point>> contours, contours_deal;
	Mat blur, thresh_img;
	GaussianBlur(base_img, blur, Size(11, 11), 0, 0, BORDER_DEFAULT);
	threshold(blur, thresh_img, 254, 255, THRESH_BINARY);

	// 将上边界的像素设置为255
	thresh_img.row(0).setTo(Scalar(255));
	thresh_img.row(thresh_img.rows - 1).setTo(Scalar(255));
	thresh_img.col(0).setTo(Scalar(255));
	thresh_img.col(thresh_img.cols - 1).setTo(Scalar(255));

	findContours(thresh_img, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

	double maxArea = 0.0;
	int maxAreaContourIndex = -1;
	for (int i = 0; i < (int)contours.size(); i++)
	{
		if (contours[i].size() < 300)
			continue;

		double area = contourArea(contours[i]);
		if (area < maxArea)
			continue;

		maxArea = area;
		maxAreaContourIndex = i;
	}

	if (maxAreaContourIndex == -1) // 将最大的轮廓添加到 contours_deal
		return;
	contours_deal.push_back(contours[maxAreaContourIndex]);

	Mat base_mask = Mat::zeros(base_img.size(), base_img.type());
	drawContours(base_mask, contours_deal, 0, Scalar(255), FILLED);

	Mat base_mask_inv;
	bitwise_not(base_mask, base_mask_inv);

	bitwise_and(base_img, base_mask, base_img);
	bitwise_or(base_img, base_mask_inv, base_img);

	//dbg("time=%d\n", timeGetTime()-t); //1~2ms
}

IMD_RESULT IMDFap50Method::save_to_memory(ImageStatus* img_status)
{
	if (show_mode == GUI_SHOW_MODE_SIGNATURE )
	{
		Rect roi(7, 26, 1583, 847);
		sign_img(roi).copyTo(flat_img[FINGER_POSITION_SIGNATURE](roi));
		return IMD_RLT_SUCCESS;
	}

	//if (out_img == nullptr)
	//	return IMD_RLT_NO_AVAILABLE_IMAGE;

	int finger_num = 0;
	sort_finger(m_property.finger_position);

	Mat src(m_Height, m_Width, CV_8UC1, img_status->img);// 假设您有原始图像 
	get_finger_position();

	if (show_mode == GUI_SHOW_MODE_ROLL)
	{
		Rect roi(ROLL_X, ROLL_Y, ROLL_WIDTH, ROLL_HEIGHT);
		src(roi).copyTo(roll_img[finger_position_code]);// = src(roi).clone();
		return IMD_RLT_SUCCESS;
	}
	else if (show_mode == GUI_SHOW_MODE_CAPTURE)
	{
		src.copyTo(flat_img[finger_position_code]);// = src.clone();
		return IMD_RLT_SUCCESS;
	}

	if(finger_position_code == FINGER_POSITION_RIGHT_FULL || finger_position_code== FINGER_POSITION_LEFT_FULL)
	{
		src.copyTo(flat_img[FINGER_POSITION_SOME_FINGERS]);
		return IMD_RLT_SUCCESS;
	}

	src.copyTo(flat_img[finger_position_code]);
	if (imd_contours.size() == 0)
		return IMD_RLT_NO_ANY_FINGER;

	// 旋转图像
	Mat rotatedImage;
	// 扩展图像并填充空白
	Mat expanded_img;
	copyMakeBorder(src, expanded_img, EXT_WIDTH, EXT_WIDTH, EXT_WIDTH, EXT_WIDTH, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

	//dbg("\n");

	for (const auto& contour : imd_contours)
	{
		//save to bmp file.
		//if (1)
		{
			// 计算矩形的中心点
			Point2d
				PA = Point2d(contour.pa.x, contour.pa.y),
				PB = Point2d(contour.pb.x, contour.pb.y),
				PC = Point2d(contour.pc.x, contour.pc.y),
				PD = Point2d(contour.pd.x, contour.pd.y),
				center = (PA + PB + PC + PD) / 4;

			// 计算矩形的宽度和高度
			double width = norm(PB - PA);
			double height = norm(PD - PA);

			// 计算旋转角度
			double angle = atan2(-(PA.y - PD.y), PA.x - PD.x) * 180.0 / CV_PI;
			if (angle > 90)
				angle = -(angle - 90);
			else
				angle = 90 - angle;

			// 创建旋转矩阵
			center += Point2d(EXT_WIDTH, EXT_HEIGHT);//extend width and height.
			Mat rotationMatrix = getRotationMatrix2D(center, angle, 1.0);
			warpAffine(expanded_img, rotatedImage, rotationMatrix, expanded_img.size(), cv::INTER_CUBIC, cv::BORDER_REPLICATE);

			// 获取旋转后的矩形区域
			Rect new_rect((int)(center.x - width / 2), (int)(center.y - height / 2), 400, 500);
			//dbg("new_rect: point(%d, %d)\n", (int)new_rect.x, (int)new_rect.y);

			if (
				(int)rotatedImage.cols < (int)new_rect.x
				||
				abs((int)rotatedImage.cols - (int)new_rect.x) < EXT_WIDTH
				|| 
				(int)rotatedImage.rows < (int)new_rect.y
				|| 
				abs((int)rotatedImage.rows - (int)new_rect.y) < EXT_HEIGHT
			)
			{
				//dbg("(diff < EXT_WIDTH)\n");
				continue;
			}

			int now_finger_num = finger_num;
			finger_num++;
			//check new_rect valid
			if ((0 <= new_rect.x && 0 <= new_rect.width && 0 <= new_rect.y && 0 <= new_rect.height) == FALSE)
			{
				//dbg("The sub finger width or height not invalid.\n");
				continue;
			}

			Mat finalImage = rotatedImage(new_rect).clone();
			erase_other_finger(finalImage);//去除臨近的指紋

			//save finger to memory
			get_finger_position(now_finger_num);
			flat_img[finger_position_code] = finalImage;// .clone();
		}
	}

	return IMD_RLT_SUCCESS;
}

static bool no_case_compare(const string str1, const string str2) 
{
	string lowerStr1 = str1;
	string lowerStr2 = str2;
	transform(lowerStr1.begin(), lowerStr1.end(), lowerStr1.begin(), ::tolower);
	transform(lowerStr2.begin(), lowerStr2.end(), lowerStr2.begin(), ::tolower);
	return lowerStr1 == lowerStr2;
}

IMD_RESULT IMDFap50Method::save_file (E_GUI_SHOW_MODE mode, E_FINGER_POSITION finger_pos, const char* _file_path)
{
#if 1 
	string file_path = _file_path;
	size_t lastDotIndex = file_path.find_last_of('.');
	string extension = (lastDotIndex != string::npos) ? file_path.substr(lastDotIndex + 1) : "";

	E_IMG_TYPE img_type = IMG_TYPE_BMP;
	if (no_case_compare(extension, "png"))
		img_type = IMG_TYPE_PNG;
	else if (no_case_compare(extension, "wsq"))
		img_type = IMG_TYPE_WSQ;
	else if (no_case_compare(extension, "jp2"))
		img_type = IMG_TYPE_JP2;
	else if (no_case_compare(extension, "raw"))
		img_type = IMG_TYPE_RAW;
	else if (no_case_compare(extension, "fir"))
		img_type = IMG_TYPE_FIR;
	else if (no_case_compare(extension, "pgm"))
		img_type = IMG_TYPE_PGM;

	int finger_num = 0;
	string fileNameWithoutExtension = file_path;
	if (lastDotIndex != string::npos)
		fileNameWithoutExtension = file_path.substr(0, lastDotIndex);

	show_mode = mode;
	m_property.finger_position = finger_pos;

	string file_full_path = file_path;
	if (show_mode == GUI_SHOW_MODE_SIGNATURE)
	{
		file_full_path = format_to_string("%s_Signature_%s.%s",
			fileNameWithoutExtension.c_str(),
			finger_position_text.c_str(),
			"xxx"
		);
		mat_to_file(file_full_path, flat_img[m_property.finger_position], img_type);
		return IMD_RLT_SUCCESS;
	}

	get_finger_position();

	if (show_mode == GUI_SHOW_MODE_ROLL)
	{
		file_full_path = format_to_string("%s_Roll_%s.%s",
			fileNameWithoutExtension.c_str(),
			finger_position_text.c_str(),
			"xxx"
		);
		mat_to_file(file_full_path, roll_img[finger_position_code], img_type);
		return IMD_RLT_SUCCESS;
	}
	else if (show_mode == GUI_SHOW_MODE_CAPTURE)
	{
		file_full_path = format_to_string("%s_%s.%s",
			fileNameWithoutExtension.c_str(),
			finger_position_text.c_str(),
			"xxx"
		);
		mat_to_file(file_full_path, flat_img[finger_position_code], img_type);
		return IMD_RLT_SUCCESS;
	}

//flat mode
	file_full_path = format_to_string("%s_%s.%s",
		fileNameWithoutExtension.c_str(),
		finger_position_text.c_str(),
		"xxx"
	);
	mat_to_file(file_full_path, flat_img[finger_position_code], img_type);
	 
#endif
	return IMD_RLT_SUCCESS;
}

IMD_RESULT IMDFap50Method::scan_start(
	E_GUI_SHOW_MODE mode, E_FINGER_POSITION* pos_buf, int num)
{
	E_FINGER_POSITION pos = FINTER_POSITION_UNKNOW_FINGER;
	//dbg("scan_start() mode=%d pos=%d\n", mode, pos_buf[0]);

	if (mode == GUI_SHOW_MODE_SIGNATURE && m_is_display == false)
		return IMD_RLT_NOT_SUPPORT;

	if (num == 1)
	{
		pos = pos_buf[0];
		m_property.finger_position = pos;
	}
	else if (num > 1)
	{
		vector<E_FINGER_POSITION> ids = {
			FINGER_POSITION_LEFT_LITTLE, FINGER_POSITION_LEFT_RING, FINGER_POSITION_LEFT_MIDDLE, FINGER_POSITION_LEFT_INDEX, FINGER_POSITION_LEFT_THUMB,
			FINGER_POSITION_RIGHT_THUMB, FINGER_POSITION_RIGHT_INDEX, FINGER_POSITION_RIGHT_MIDDLE, FINGER_POSITION_RIGHT_RING, FINGER_POSITION_RIGHT_LITTLE,
		};

		select_fingers.clear();
		for (auto& id : ids)
			for (int i = 0; i < num; i++)
				if (id == pos_buf[i])
					select_fingers.push_back(pos_buf[i]);

		pos = FINGER_POSITION_SOME_FINGERS;
		m_property.finger_position = pos;
	}

	show_mode = mode;
	last_result = IMD_RLT_SUCCESS;
	is_sampling_saved = false;
	is_too_top = false;
	is_done = false;
	is_alert_finger_off = m_property.scan_by_manual;//false;
	is_finger_shape_not_good = false;

	if (show_mode == GUI_SHOW_MODE_CAPTURE)
	{
		static WAIT_SCAN_PARAMETER ScanParam{};
		WAIT_SCAN_PARAMETER* pScan = &ScanParam;

		DWORD raw_data_size = RAW_SIZE;
		pScan->nPixelCntX = RAW_WIDTH;
		pScan->nPixelCntY = RAW_HEIGHT;
		pScan->img = fap50_img;
		pScan->raw = fap50_raw;
		pScan->nWaitTime = 0;
		pScan->hw_scan_mode = HW_SCAN_MODE_FLAT;
		pScan->raw_size = RAW_SIZE;
		DWORD dwError = UT_CaptureImageEx(device_handle, pScan);

		scan_done = pScan;
		return IMD_RLT_SUCCESS;
	}
	//else if (show_mode == GUI_SHOW_MODE_FLAT)
	//else if (show_mode == GUI_SHOW_MODE_ROLL)
	//	;// roll_too_fast = false;
	else if (show_mode == GUI_SHOW_MODE_SIGNATURE || show_mode == GUI_SHOW_MODE_SIGNATURE_BY_PEN
)
	{
		//sign_img = Mat(1000, 1600, CV_8UC1, Scalar(255)),
		//sign_img_done = Mat(1000, 1600, CV_8UC1, Scalar(255));
		signature(SIGNATURE_ACTION_CLEAR);
	}

	need_finger_off = true;

	int force_reader = 0;

#ifdef _WIN32
	force_reader = atoi((CT2A)ini.get_value(L"force_reader", L"Main"));
#endif

	if (force_reader == 1)
		m_is_reader = true;

	m_is_reader = m_is_reader & m_property.speech_en;
	m_is_mute = (m_property.speech_volume == 3);

	UT_SetSpeechLanguage(device_handle, m_property.speech_language, m_is_reader);
	UT_SetSpeechVolume(device_handle, m_property.speech_volume, m_is_reader);

	switch (show_mode) {
	case GUI_SHOW_MODE_FLAT:
		switch (pos) {
		case FINGER_POSITION_RIGHT_FOUR:
			UT_LedSpeech(device_handle, LED_RIGHT_4_FINGERS, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_PUT_RIGHT_4_FINGERS, m_is_reader);
			break;
		case FINGER_POSITION_LEFT_FOUR:
			UT_LedSpeech(device_handle, LED_LEFT_4_FINGERS, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_PUT_LEFT_4_FINGERS, m_is_reader);
			break;
		case FINGER_POSITION_BOTH_THUMBS:
			UT_LedSpeech(device_handle, SPEECH_PUT_2_THUMBS_FINGERS, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_PUT_2_THUMBS_FINGERS, m_is_reader);
			break;
		}
		break;
	case GUI_SHOW_MODE_ROLL:
		switch (pos) {
		case FINGER_POSITION_RIGHT_THUMB:
			UT_LedSpeech(device_handle, LED_ROLLING_MODE, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_ROLLING_RIGHT_THUMB_FINGER, m_is_reader);
			break;
		case FINGER_POSITION_RIGHT_INDEX:
			UT_LedSpeech(device_handle, LED_ROLLING_MODE, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_ROLLING_RIGHT_INDEX_FINGER, m_is_reader);
			break;
		case FINGER_POSITION_RIGHT_MIDDLE:
			UT_LedSpeech(device_handle, LED_ROLLING_MODE, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_ROLLING_RIGHT_MIDDLE_FINGER, m_is_reader);
			break;
		case FINGER_POSITION_RIGHT_RING:
			UT_LedSpeech(device_handle, LED_ROLLING_MODE, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_ROLLING_RIGHT_RING_FINGER, m_is_reader);
			break;
		case FINGER_POSITION_RIGHT_LITTLE:
			UT_LedSpeech(device_handle, LED_ROLLING_MODE, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_ROLLING_RIGHT_LITTLE_FINGER, m_is_reader);
			break;
		case FINGER_POSITION_LEFT_THUMB:
			UT_LedSpeech(device_handle, LED_ROLLING_MODE, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_ROLLING_LIFT_THUMB_FINGER, m_is_reader);
			break;
		case FINGER_POSITION_LEFT_INDEX:
			UT_LedSpeech(device_handle, LED_ROLLING_MODE, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_ROLLING_LIFT_INDEX_FINGER, m_is_reader);
			break;
		case FINGER_POSITION_LEFT_MIDDLE:
			UT_LedSpeech(device_handle, LED_ROLLING_MODE, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_ROLLING_LIFT_MIDDLE_FINGER, m_is_reader);
			break;
		case FINGER_POSITION_LEFT_RING:
			UT_LedSpeech(device_handle, LED_ROLLING_MODE, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_ROLLING_LIFT_RING_FINGER, m_is_reader);
			break;
		case FINGER_POSITION_LEFT_LITTLE:
			UT_LedSpeech(device_handle, LED_ROLLING_MODE, LED_ON, m_is_mute ? SPEECH_OFF : SPEECH_ROLLING_LIFT_LITTLE_FINGER, m_is_reader);
			break;
		}
		break;
	}

//wait thread end -------------------------------------------
	DWORD timeout_wait_for_thread_end = timeGetTime() + 500;
	while (is_thread_scan_busy)
	{
		Sleep(1);
		if (timeGetTime() > timeout_wait_for_thread_end)
		{
			set_last_result(IMD_RLT_SCAN_THREAD_IS_BUSY);
			dbg("is_thread_scan_busy\n");
			return IMD_RLT_SCAN_THREAD_IS_BUSY;
			//while (is_thread_scan_busy);
		}
	}

	enable_continue_scan = true;
	is_thread_scan_busy = true;
	is_make_image_status_busy = true;
	scan_done = nullptr;

	m_image_status = ImageStatus{}; //this need to set.
	//while(is_thread_scan_busy == false) Sleep(1); //wait to active.
	thread_member(IMDFap50Method::scan_loop);
	thread_member(IMDFap50Method::make_image_status);
	//dbg("scan_start() end\n");

	return IMD_RLT_SUCCESS;
}

void IMDFap50Method::scan_loop()
{
	if (callback_event) callback_event(IMD_RLT_SCAN_THREAD_START);

	static WAIT_SCAN_PARAMETER scan_end{}, scan{};
	HwStatus hw_status{};
	IMD_RESULT res = IMD_RLT_FAIL;

	//ring buffer init.
	RingScan* ring = &m_ring;
	ring->write_idx = 0;
	for (int i = 0; i < RING_SIZE; i++)
	{
		memset(ring->unit[i].raw, 0xFF, sizeof(ring->unit[i].raw));
		memset(ring->unit[i].img, 0xFF, sizeof(ring->unit[i].img));
	}
	debounce_flat_cnt = 0;

	scan.raw = ring->unit[ring->write_idx].raw;
	scan.img = ring->unit[ring->write_idx].img;
	scan.raw_size = 1600 * 1024;
	scan.hw_scan_mode = HW_SCAN_MODE_FLAT;

	BYTE reg12 = m_reg12;
	//dbg("scan_loop(): reg12=%.2X\n", m_reg12);

	if (show_mode == GUI_SHOW_MODE_ROLL)
	{
		scan.raw_size /= 2;
		scan.hw_scan_mode = HW_SCAN_MODE_ROLL;
	}
	//else if (show_mode == GUI_SHOW_MODE_SIGNATURE)
	//{
	//	reg12 = 0x88;
	//	scan.raw_size /= 4;
	//	scan.hw_scan_mode = HW_SCAN_MODE_SIGNATURE;
	//}
	else if (show_mode == GUI_SHOW_MODE_SIGNATURE)
	{
		reg12 = 0x88;// 0x8C;
		scan.raw_size /= 4;
		scan.hw_scan_mode = HW_SCAN_MODE_SIGNATURE_2;
	}
	else if (show_mode == GUI_SHOW_MODE_SIGNATURE_BY_PEN)
	{
		enable_pen(1);
	}

	UT_SetSensorRegister(device_handle, 0x6012, reg12);
	//UT_SetSensorRegister(device_handle, 0x6011, 0x81);//for debug
	//UT_SetSensorRegister(device_handle, 0x6012, 0x95);//for debug

	UINT64 timeout_finger_off = (UINT64)timeGetTime() + m_property.scan_timeout_ms;
	int err_cnt = 0;// , capture_cnt = 10;

	//auto now = timeGetTime();

	while (enable_continue_scan)
	{
		//dbg("while (enable_continue_scan)\n");

		int dark = image_black;
		int white = image_white;
		
//idle 30s, exit loop------------------------------------------------------------
		//if (capture_cnt-- == 0)
		//	goto exit_loop;

		//if (0)//debug
		{
			if ((UINT64)timeGetTime() > timeout_finger_off)
			{
				if (callback_event) callback_event(IMD_RLT_SCAN_THREAD_IDLE_TIMEOUT);
				set_last_result(IMD_RLT_SCAN_THREAD_IDLE_TIMEOUT);
				break;
			}

			if (white - dark < 50//finger off
				&& (show_mode==GUI_SHOW_MODE_ROLL || show_mode== GUI_SHOW_MODE_FLAT))
				Sleep(300); //when finger off, delay for prevent the sensor to get hot.
			else
			{
				//(show_mode == GUI_SHOW_MODE_ROLL) ? Sleep(30) : Sleep(15);
				timeout_finger_off = (UINT64)timeGetTime() + m_property.scan_timeout_ms;
			}

			//if (show_mode == GUI_SHOW_MODE_ROLL)
			//	Sleep(10);
		}

		//auto gain ----------------------------------------------------------------------
		//if (m_pTestMethod->module_info.fix_reg1B)
		BYTE RegValue = 0;
		WORD RegAddr = ((1 & 0xF) << 12) | ((0 & 0xF) << 8) | (0x1B & 0xFF);//iG 0
		if (enable_runtime_judge_black == false)
		{
//			TRACE("m_pFTParam->module.fix_reg1B\n");
//			UT_GetSensorRegister(device_handle, RegAddr, &RegValue);
//			RegValue = (RegValue & 0xF) | 0xF0;
//			RegAddr = ((1 & 0xF) << 12) | ((0 & 0xF) << 8) | 0x1B & 0xFF; //iG 1
//			UT_SetSensorRegister(device_handle, RegAddr, RegValue);
//
//			RegValue &= 0xF3;
//			RegAddr = ((2 & 0xF) << 12) | ((0 & 0xF) << 8) | 0x1B & 0xFF; //iG 2
//			UT_SetSensorRegister(device_handle, RegAddr, RegValue);
//			RegAddr = ((3 & 0xF) << 12) | ((0 & 0xF) << 8) | 0x1B & 0xFF; //iG 3
//			UT_SetSensorRegister(device_handle, RegAddr, RegValue);
//			RegAddr = ((4 & 0xF) << 12) | ((0 & 0xF) << 8) | 0x1B & 0xFF; //iG 4
//			UT_SetSensorRegister(device_handle, RegAddr, RegValue);
		}
		else if (dark != -1)
		{
			int dark_target = 20;
			while (dark < 15 || dark>30)
			{
				int step = (dark < dark_target) ? -1 : 1;
				UT_GetSensorRegister(device_handle, RegAddr, &RegValue);

				if (RegValue == 0)
				{
					UT_GetSensorRegister(device_handle, RegAddr, &RegValue);
					if (RegValue == 0)
						RegValue = 0xFC;
					//for (int i = 0; i < 32; i++)
					//{
					//	WORD addr = ((1 & 0xF) << 12) | ((0 & 0xF) << 8) | (i & 0xFF);//iG 0
					//	BYTE reg;
					//	UT_GetSensorRegister(device_handle, addr, &reg);
					//	CString str; str.Format(L"reg%04X=%02X\n", addr, reg);
					//	OutputDebugString(str);
					//}
					//RegValue = 0xFC;
				}
				//RegValue = (RegValue & 0xF0) | 0x0C;

				if ((RegValue & 0xF0) == 0xF0 && step > 0)
					break;
				if ((RegValue & 0xF0) == 0x00 && step < 0)
					break;

				RegValue += (step << 4) | 0x0C;
				RegAddr = ((1 & 0xF) << 12) | ((0 & 0xF) << 8) | 0x1B;// &0xFF; //iG 1
				UT_SetSensorRegister(device_handle, RegAddr, RegValue);

				RegValue &= 0xF3;
				RegAddr = ((2 & 0xF) << 12) | ((0 & 0xF) << 8) | 0x1B;// &0xFF; //iG 2
				UT_SetSensorRegister(device_handle, RegAddr, RegValue);
				RegAddr = ((3 & 0xF) << 12) | ((0 & 0xF) << 8) | 0x1B;// &0xFF; //iG 3
				UT_SetSensorRegister(device_handle, RegAddr, RegValue);
				RegAddr = ((4 & 0xF) << 12) | ((0 & 0xF) << 8) | 0x1B;// &0xFF; //iG 4
				UT_SetSensorRegister(device_handle, RegAddr, RegValue);
				break;
			}

			dbg("dark:%d white:%d reg1B=%02X\n", dark, white, RegValue);
		}
		
		//Sleep(50);

		//check image status ------------------------------------------------------------------
		DWORD dwError = ERROR_SUCCESS;
		//long subframe_size = 1024 * 1600 / 8;
		BYTE* img_raw = nullptr;
		if (scan.hw_scan_mode == HW_SCAN_MODE_ROLL)
		{
			long raw_w = 1024, raw_h = 1600;
			long subframe_size = raw_w * raw_h / 8;
			scan.raw = ring->unit[ring->write_idx].raw + 2*subframe_size;
			dwError = UT_CaptureImageEx(device_handle, &scan);
			img_raw = scan.raw;
			scan.raw = ring->unit[ring->write_idx].raw;
		}
		else if (scan.hw_scan_mode == HW_SCAN_MODE_SIGNATURE_2)
		{
			scan.raw = ring->unit[ring->write_idx].fast;
			dwError = UT_CaptureImageEx(device_handle, &scan);
			img_raw = scan.raw;
			scan.raw = ring->unit[ring->write_idx].raw;
			scan.fast = ring->unit[ring->write_idx].fast;
		}
		else if (show_mode == GUI_SHOW_MODE_SIGNATURE_BY_PEN)
		{
			goto capture_done;
		}
		else
		{
			dwError = UT_CaptureImageEx(device_handle, &scan);
			img_raw = scan.raw;

			//BYTE* buf = img_raw;
			//for (int i = 0; i < 210; i++)
			//{
			//	int* log = (int*)(buf + i*1024);
			//	dbg("i=%d dcmi_w=%d pkg=%d seq=%d ring_w=%d ring_r=%d abs=%d \n", i, log[0], log[1], log[2]%100000, log[4], log[5], abs(log[4]- log[5]));
			//}
			
		}

		//dbg("capture time is %dms\n", timeGetTime() - now);
		//now = timeGetTime();
		//scan_done = nullptr;

		UT_GetFingerState(device_handle, (BYTE*)&hw_status.all);
		if (hw_status.is_usb_too_slow)
		{
			res = IMD_RLT_USB_TOO_SLOW;
			dbg("IMD_RLT_USB_TOO_SLOW reg12=%2X\n", m_reg12);
			m_reg12++;
			UT_SetSensorRegister(device_handle, 0x6012, m_reg12);
			continue;
		}
		else if (hw_status.is_dcmi_timeout)
		{
			static int dcmi_err_cnt = 0;
			res = IMD_RLT_DCMI_IS_STUCK;
			dbg("cnt:%d IMD_RLT_DCMI_IS_STUCK %d\n", err_cnt, dcmi_err_cnt++);
		}
		else if (dwError == WAIT_TIMEOUT)
		{
			res = IMD_RLT_USB_READ_IMAGE_TIMEOUT;
			dbg("cnt:%d IMD_RLT_USB_READ_IMAGE_TIMEOUT\n", err_cnt);
		}

		if (hw_status.is_dcmi_timeout 
			//|| hw_status.is_usb_too_slow 
			|| dwError)
		{
			int ERR_CNT = (scan.hw_scan_mode == HW_SCAN_MODE_SIGNATURE_2) ? 10 : 3;

			if (err_cnt++ > ERR_CNT)
			{
				if (callback_event) callback_event(res);
				set_last_result(res);
				dbg("(err_cnt++ > %d)\n", ERR_CNT);
				break;
			}
			else if (dwError == ERROR_GEN_FAILURE) 
			{
				res = IMD_RLT_USB_READ_IMAGE_EXCEPTION;
				if (callback_event) callback_event(res);
				set_last_result(res);
				dbg("cnt:%d ERROR_GEN_FAILURE err:0x%X\n", err_cnt, dwError);
				Sleep(1500);
				break;
			}
			else if (dwError == ERROR_BAD_PIPE) 
			{
				res = IMD_RLT_USB_READ_IMAGE_EXCEPTION;
				if (callback_event) callback_event(res);
				set_last_result(res);
				dbg("cnt:%d ERROR_BAD_PIPE err:0x%X\n", err_cnt, dwError);
				Sleep(1500);
				break;
			}
			else if (dwError)
			{
				res = IMD_RLT_USB_READ_IMAGE_EXCEPTION;
				dbg("cnt:%d IMD_RLT_USB_READ_IMAGE_EXCEPTION err:0x%X\n", err_cnt, dwError);
				Sleep(1500);
				recovery_power_drop();
			}

			continue;
		}

capture_done:
		err_cnt = 0;
		//int processing_cnt = 0; long
		UINT64 t_now = (UINT64)timeGetTime();
		UINT64 timeout_wait_for_show_img_done = t_now + 300;
		bool show_worning = true;
		while ((UINT64)timeGetTime() < timeout_wait_for_show_img_done)
		{
			if (scan_done == nullptr)
				break;

			if (show_worning)
				show_worning = false;
		}

		if (scan_done == nullptr)
		{
			scan_end = scan;
			ring->write_idx = (ring->write_idx + 1) % RING_SIZE;
			scan.raw = ring->unit[ring->write_idx].raw;
			scan.img = ring->unit[ring->write_idx].img;
			scan_done = &scan_end;
		}
	}

	enable_continue_scan = false;
	is_thread_scan_busy = false;
	//m_property.life_check_en = org_life_check_en;
	enable_pen(0);

	if (callback_event) callback_event(IMD_RLT_SCAN_THREAD_END);
	//dbg("IMD_RLT_SCAN_THREAD_END\n");
	//dbg("scan_loop() end\n");
}

//bool run_cmd(LPCTSTR cmd, string& outOutput)
//{
//	HANDLE g_hChildStd_OUT_Rd = NULL;
//	HANDLE g_hChildStd_OUT_Wr = NULL;
//	HANDLE g_hChildStd_ERR_Rd = NULL;
//	HANDLE g_hChildStd_ERR_Wr = NULL;
//
//	SECURITY_ATTRIBUTES sa;
//	// Set the bInheritHandle flag so pipe handles are inherited.
//	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
//	sa.bInheritHandle = TRUE;
//	sa.lpSecurityDescriptor = NULL;
//	if (!CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &sa, 0)) { return 1; } // Create a pipe for the child process's STDERR.
//	if (!SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0)) { return 1; } // Ensure the read handle to the pipe for STDERR is not inherited.
//	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &sa, 0)) { return 1; } // Create a pipe for the child process's STDOUT.
//	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) { return 1; } // Ensure the read handle to the pipe for STDOUT is not inherited
//
//	PROCESS_INFORMATION piProcInfo;
//	STARTUPINFO siStartInfo;
//	bool bSuccess = FALSE;
//
//	// Set up members of the PROCESS_INFORMATION structure.
//	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
//
//	// Set up members of the STARTUPINFO structure.
//	// This structure specifies the STDERR and STDOUT handles for redirection.
//	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
//	siStartInfo.cb = sizeof(STARTUPINFO);
//	siStartInfo.hStdError = g_hChildStd_ERR_Wr;
//	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
//	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
//
//	// Create the child process.
//	bSuccess = CreateProcess(
//		NULL,             // program name
//		(LPTSTR)cmd,	  // command line
//		NULL,             // process security attributes
//		NULL,             // primary thread security attributes
//		TRUE,             // handles are inherited
//		CREATE_NO_WINDOW, // creation flags (this is what hides the window)
//		NULL,             // use parent's environment
//		NULL,             // use parent's current directory
//		&siStartInfo,     // STARTUPINFO pointer
//		&piProcInfo       // receives PROCESS_INFORMATION
//	);
//
//	CloseHandle(g_hChildStd_ERR_Wr);
//	CloseHandle(g_hChildStd_OUT_Wr);
//
//	// read output
//#define BUFSIZE 4096
//	DWORD dwRead;
//	CHAR chBuf[BUFSIZE];
//	bool bSuccess2 = FALSE;
//	for (;;) { // read stdout
//		bSuccess2 = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
//		if (!bSuccess2 || dwRead == 0) break;
//		std::string s(chBuf, dwRead);
//		outOutput += s;
//	}
//	dwRead = 0;
//	for (;;) { // read stderr
//		bSuccess2 = ReadFile(g_hChildStd_ERR_Rd, chBuf, BUFSIZE, &dwRead, NULL);
//		if (!bSuccess2 || dwRead == 0) break;
//		std::string s(chBuf, dwRead);
//		outOutput += s;
//	}
//
//	// The remaining open handles are cleaned up when this process terminates.
//	// To avoid resource leaks in a larger application,
//	// close handles explicitly.
//	return 0;
//}

IMD_STD_API int get_nfiq(void* pSrc, int* score, SystemProperty* property);

int IMDFap50Method::get_mat_score(Mat image)
{
	int score = 0;
	get_nfiq(&image, &score, &m_property);
	return score;

	//TCHAR buffer[MAX_PATH] = { 0 };
	//GetModuleFileName(NULL, buffer, MAX_PATH);
	//std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
	//std::wstring(buffer).substr(0, pos);

	//CString module_file = std::wstring(buffer).substr(0, pos + 1).c_str();
	//CString tmp_image_file_name = module_file + L"tmp.png";

	//int bkup_ratio = m_property.png_compress_ratio;
	//m_property.png_compress_ratio = 0;
	//mat_to_file(tmp_image_file_name, image, IMG_TYPE_PNG);
	//m_property.png_compress_ratio = bkup_ratio;

	//CString nfiq_exe = module_file + L"nfiq.exe";
	//CString cmd = nfiq_exe + L" " + tmp_image_file_name;
	//std::string result;
	//run_cmd(cmd, result);
	//DeleteFile(tmp_image_file_name);

	//return atoi(result.c_str());
}

IMD_RESULT IMDFap50Method::get_score(ImageStatus* img_status)//, int** score_array, int* array_size)
{
	//if (out_img == nullptr)
	//	return IMD_RLT_NO_AVAILABLE_IMAGE;

	NFIQ_VERSION score_ver = m_property.nfiq_ver;
	auto finger_code = 0; //ISO finger code of the fingerprint in the image.
	int score = 0;
	int finger_num = 0;
	Mat src(m_Height, m_Width, CV_8UC1, img_status->img);// 假设您有原始图像 

//#define SAVE_NO_ISP
#ifdef SAVE_NO_ISP
	static int save_cnt = 0;
	Mat img_no_isp(m_Height, m_Width, CV_8UC1, fap50_img_no_isp);
	CString filename_no_isp; filename_no_isp.Format(L"d:/img_no_isp_%d.png", save_cnt++ % 100);
	mat_to_file(filename_no_isp, img_no_isp, IMG_TYPE_PNG);//dbg
#endif

	get_finger_position();
	if (show_mode == GUI_SHOW_MODE_ROLL)
	{
		Mat roi_src = roll_img[finger_position_code];
		if (score_ver == NFIQ_V1)
			score = get_mat_score(roi_src);
		else if (score_ver == NFIQ_V2)
			score = ComputeNfiq2Score(finger_code, roi_src.data, (int)roi_src.total(), roi_src.cols, roi_src.rows, 500);

		if (score > 100)
			score = 0;
		if (img_status->result == IMD_RLT_SUCCESS)
		{
			if (   (score_ver == NFIQ_V1 && score > m_property.nfiq_score_minimum_acceptable)
				|| (score_ver == NFIQ_V2 && score < m_property.nfiq2_score_minimum_acceptable))
				img_status->result = IMD_RLT_POOR_NFIQ_QUALITY;
		}

		roll_score[score_ver][finger_position_code][0] = score;
		return IMD_RLT_SUCCESS;
	}
	else if (show_mode == GUI_SHOW_MODE_CAPTURE)
	{
		if (score > 100)
			score = 0;
		return IMD_RLT_NO_IMPLEMENTATION;
	}

	//flat mode
	if (imd_contours.size() == 0)
		return IMD_RLT_NO_ANY_FINGER;

	int four_finger_pos = finger_position_code;
	//if(imd_contours.size() == 4)

	int idx = 0;
	for (const auto& contour : imd_contours)
	{
		int now_finger_num = finger_num;
		finger_num++;

		get_finger_position(now_finger_num);
		Mat finalImage = flat_img[finger_position_code];
		//imshow("finalImage", finalImage);
		if (score_ver == NFIQ_V1)
			score = get_mat_score(finalImage);
		else if (score_ver == NFIQ_V2)
			score = ComputeNfiq2Score(finger_code, finalImage.data, (int)finalImage.total(), finalImage.cols, finalImage.rows, 500);

		if (score > 100)
			score = 0;
		if (img_status->result == IMD_RLT_SUCCESS)
		{
			if (   (score_ver == NFIQ_V1 && score > m_property.nfiq_score_minimum_acceptable)
				|| (score_ver == NFIQ_V2 && score < m_property.nfiq2_score_minimum_acceptable))
				img_status->result = IMD_RLT_POOR_NFIQ_QUALITY;
		}

		flat_score[score_ver][finger_position_code][0] = score;
		flat_score[score_ver][four_finger_pos][idx] = score;
		//dbg(L"idx=%d, score=%d\n", idx, score);

		idx++;
		if (idx > SCORE_ARRAY_SIZE)
			return IMD_RLT_FAIL;
	}

	return IMD_RLT_SUCCESS;
}

IMD_RESULT IMDFap50Method::get_image(ImageProperty& pty)
{
	Mat mat;
	IMD_RESULT ret {};
	pty.score_ver = m_property.nfiq_ver;

	switch (pty.mode) {
	default:
	case GUI_SHOW_MODE_CAPTURE:
		break;

	case GUI_SHOW_MODE_FLAT:
		mat = flat_img[pty.pos];

		if (pty.pos == FINGER_POSITION_RIGHT_FOUR || pty.pos == FINGER_POSITION_LEFT_FOUR)
			pty.score_size = 4;
		else if (pty.pos == FINGER_POSITION_BOTH_THUMBS)
			pty.score_size = 2;
		else if (pty.pos == FINGER_POSITION_SOME_FINGERS)
			pty.score_size = (int)select_fingers.size();
		else
			pty.score_size = 1;

		for (int i = 0; i < pty.score_size; i++)
		{
			int score = flat_score[m_property.nfiq_ver][pty.pos][i];
			pty.score_array[i] = score;

			if (m_property.nfiq_ver == NFIQ_V1)
			{
				if (score > m_property.nfiq_score_minimum_acceptable)
					ret = IMD_RLT_POOR_NFIQ_QUALITY;
			}
			else if (m_property.nfiq_ver == NFIQ_V2)
			{
				if (score < m_property.nfiq2_score_minimum_acceptable)
					ret = IMD_RLT_POOR_NFIQ_QUALITY;
			}
		}
		break;

	case GUI_SHOW_MODE_ROLL:
		mat = roll_img[pty.pos];

		if (pty.pos == FINGER_POSITION_RIGHT_FOUR || pty.pos == FINGER_POSITION_LEFT_FOUR)
			pty.score_size = 4;
		else if (pty.pos == FINGER_POSITION_BOTH_THUMBS)
			pty.score_size = 2;
		else
			pty.score_size = 1;

		for (int i = 0; i < pty.score_size; i++)
		{
			int score = roll_score[m_property.nfiq_ver][pty.pos][i];
			pty.score_array[i] = score;

			if (m_property.nfiq_ver == NFIQ_V1)
			{
				if (score > m_property.nfiq_score_minimum_acceptable)
					ret = IMD_RLT_POOR_NFIQ_QUALITY;
			}
			else if (m_property.nfiq_ver == NFIQ_V2)
			{
				if (score < m_property.nfiq2_score_minimum_acceptable)
					ret = IMD_RLT_POOR_NFIQ_QUALITY;
			}
		}
		break;
	}

	pty.img = mat.data;
	pty.width = mat.cols;
	pty.height = mat.rows;

	if (m_property.nfiq_ver == NFIQ_V1)
		pty.score_min = m_property.nfiq_score_minimum_acceptable;
	else
		pty.score_min = m_property.nfiq2_score_minimum_acceptable;

	if (pty.this_scan)
	{
		if (is_wrong_hand)
		{
			is_wrong_hand = false;
			if (ret == IMD_RLT_POOR_NFIQ_QUALITY)
				ret = IMD_RLT_POOR_QUALITY_AND_WRONG_HAND;
			else if (ret == IMD_RLT_SUCCESS)
				ret = IMD_RLT_PUT_WRONG_HAND;
		}
		else if(is_poor_contact)
		{
			is_poor_contact = false;
			if (ret == IMD_RLT_POOR_NFIQ_QUALITY)
				ret = IMD_RLT_POOR_QUALITY_AND_CANTACT_IRON;
			else if (ret == IMD_RLT_SUCCESS)
				ret = IMD_RLT_POOR_CONTACT_IRON;
		}
		else if (is_finger_shape_not_good)
		{
			is_finger_shape_not_good = false;
			ret = IMD_RLT_FINGER_SHAPE_NOT_GOOD;
		}
	}

	return ret;
}

IMD_RESULT IMDFap50Method::usb_switch(BYTE usb_sel)
{
	UT_UsbSwtich(device_handle, usb_sel, m_is_reader);
	connect_usb_device();
	return IMD_RESULT(0);
}

IMD_RESULT IMDFap50Method::set_led_speech_standby_mode()
{
	//m_property.life_check_en = FALSE;

	UT_LedSpeech(device_handle, LED_ALL, LED_ON, SPEECH_OFF, m_is_reader);
	//UT_LifeCheck(device_handle, 1);
	//dbg("set_led_speech_standby_mode\n");

	//UT_SuspendDevice(device_handle);

	return IMD_RESULT(0);
}

IMD_RESULT IMDFap50Method::set_burn_code()
{
	UT_SetBurnCode(device_handle, m_is_reader);
	connect_usb_device();
	return IMD_RESULT(0);
}

IMD_RESULT IMDFap50Method::user_space(bool is_write, WORD offset, BYTE* data, int len)
{
	if (is_write)
	{
		for (int i = 0; i < 3; i++)
		{
			DWORD res = UT_UserSpace(device_handle, is_write, offset, data, len);			
			if (res == 0)
				break;

			if (res && i==2)
				return IMD_RLT_FLASH_WRITE_FAIL;
		}
	}
	else
	{
		UT_UserSpace(device_handle, is_write, offset, data, len);
	}

	return IMD_RESULT(0);
}

// 画由线组成的虚线
void draw_dotted_line2(Mat img, Point pt1, Point pt2, Scalar color, int thickness)
{
	int dashLength = 15;
	int gapLength = 15;
	float lineLength = cv::norm(pt1 - pt2);
	float dx = (pt2.x - pt1.x) / lineLength;
	float dy = (pt2.y - pt1.y) / lineLength;

	int numDashes = static_cast<int>(lineLength / (dashLength + gapLength));
	for (int i = 0; i < numDashes; i++) 
	{
		Point start(pt1.x + i * (dashLength + gapLength) * dx, pt1.y + i * (dashLength + gapLength) * dy);
		Point end(start.x + dashLength * dx, start.y + dashLength * dy);
		line(img, start, end, color, thickness);
	}
}

void drawButton(Mat& img, Rect buttonRect, const string& buttonText) 
{
	// 畫按鈕背景
	Scalar buttonColor(200, 200, 200); // 灰色
	rectangle(img, buttonRect, buttonColor, cv::FILLED);

	// 畫按鈕邊框
	Scalar borderColor(0, 0, 0); // 黑色
	rectangle(img, buttonRect, borderColor, 2); // 線寬 2 像素

	// 在按鈕上標示文字
	int fontFace = cv::FONT_HERSHEY_SIMPLEX;
	double fontScale = 2.0;
	int thickness = 5;
	Scalar textColor(0, 0, 0); // 黑色文字

	// 計算文字的大小及位置，使其居中
	int baseline = 0;
	Size textSize = cv::getTextSize(buttonText, fontFace, fontScale, thickness, &baseline);
	Point textOrg(
		buttonRect.x + (buttonRect.width - textSize.width) / 2, // X 居中
		buttonRect.y + (buttonRect.height + textSize.height) / 2 // Y 居中
	);

	// 繪製文字
	putText(img, buttonText, textOrg, fontFace, fontScale, textColor, thickness);
}

IMD_RESULT IMDFap50Method::signature(E_SIGNATURE_ACTION action)
{
	switch (action) {
	case SIGNATURE_ACTION_CLEAR:
		sign_img.setTo(Scalar(255));
		//sign_img_done.setTo(Scalar(255));
		
//#define BUTTON_H 120
//#define BUTTON_W 300
//		draw_dotted_line2(sign_img, Point2f(0, 20), Point2f(sign_img.cols - 1, 20), Scalar(0), 5);
//		draw_dotted_line2(sign_img, Point2f(0, 20), Point2f(0, sign_img.rows - 1 - BUTTON_H), Scalar(0), 5);
//		draw_dotted_line2(sign_img, Point2f(sign_img.cols - 1, sign_img.rows - 1 - BUTTON_H), Point2f(sign_img.cols - 1, 20), Scalar(0), 5);
//		draw_dotted_line2(sign_img, Point2f(sign_img.cols - 1, sign_img.rows - 1 - BUTTON_H), Point2f(0, sign_img.rows - 1- BUTTON_H), Scalar(0), 5);
//		drawButton(sign_img, Rect(0, sign_img.rows - 1 - (BUTTON_H-5), BUTTON_W, (BUTTON_H - 5)), "Clear");
//		drawButton(sign_img, Rect(sign_img.cols- 1 - BUTTON_W, sign_img.rows - 1 - (BUTTON_H - 5), BUTTON_W, (BUTTON_H - 5)), "OK");
		break;

	case SIGNATURE_ACTION_OK:
		//scan_cancel();
		is_sign_done = true;
		break;
	}

	return IMD_RESULT(0);
}

IMD_RESULT IMDFap50Method::clear_flag()
{
	UT_ClearFlag(device_handle);

	return IMD_RESULT(0);
}

IMD_RESULT IMDFap50Method::update_app(const char* filePath)
{
	DWORD res = UT_UpdateApp(device_handle, filePath);
	if (res)
		return IMD_RLT_UPDATE_FW_FAIL;

	return IMD_RESULT(0);
}

//-------------------------------------------------------------------------------------
// Implementation interface of iMD FAP50 libaray.
IMD_STD_API InterfaceImdFap50Method* get_imd_fap50();

//static Fap50CallbackEvent callback_event=nullptr;

//static IMDFap50Method _fap50_method; //this method is not good. will be detect memory leak. 
//InterfaceImdFap50Method* fap50_method = &_fap50_method;
static InterfaceImdFap50Method* fap50_method = nullptr;
IMD_STD_API InterfaceImdFap50Method* get_imd_fap50()
{
	if (fap50_method == nullptr)
	{
		fap50_method = new IMDFap50Method();
		//dbg("new IMDFap50Method\n");
	}

	return fap50_method;
}

int init_device()
{
	if (fap50_method == nullptr)
		return IMD_RLT_FAIL;

	IMDFap50Method *fap50 = dynamic_cast<IMDFap50Method*>(fap50_method);
	if (fap50 == nullptr)
		return IMD_RLT_FAIL;

	return fap50->init_device();
}

IMD_STD_API IMD_RESULT set_event(Fap50CallbackEvent e)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	fap50_method->callback_event = e;

	return IMD_RESULT(0);
}

//IMD_STD_API(IMD_RESULT) open_device() 
//{
//	if (get_imd_fap50() == nullptr)
//		return IMD_RLT_NULL_POINTER;
//
//	return fap50_method->open_device();
//}
//
//IMD_STD_API(IMD_RESULT) close_device()
//{
//	if (get_imd_fap50() == nullptr)
//		return IMD_RLT_NULL_POINTER;
//
//	return fap50_method->close_device();
//}

IMD_STD_API IMD_RESULT device_reset(void)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;
	
	return fap50_method->device_reset();
}

IMD_STD_API IMD_RESULT get_system_property(SystemProperty* p)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	return fap50_method->get_system_property(p);
}

IMD_STD_API IMD_RESULT set_system_property(SystemProperty* p)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	return fap50_method->set_system_property(p);
}

IMD_STD_API IMD_RESULT scan_start(E_GUI_SHOW_MODE mode, E_FINGER_POSITION* pos_buf, int num)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER; 

	return fap50_method->scan_start(mode, pos_buf, num);
}

//IMD_STD_API IMD_RESULT scan_fingers_start(E_GUI_SHOW_MODE mode, E_FINGER_POSITION* pos_buf, int num)
//{
//	if (get_imd_fap50() == nullptr)
//		return IMD_RLT_NULL_POINTER;
//
//	return fap50_method->scan_fingers_start(mode, pos_buf, num);
//}

IMD_STD_API bool is_scan_busy(void)
{
	if (get_imd_fap50() == nullptr)
		return false;// IMD_RLT_NULL_POINTER;

	return fap50_method->is_scan_busy();
}

IMD_STD_API IMD_RESULT scan_cancel(void)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	return fap50_method->scan_cancel();
}

IMD_STD_API IMD_RESULT get_image_status(ImageStatus* status)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	return fap50_method->get_image_status(status);
}

IMD_STD_API IMD_RESULT save_file(
	E_GUI_SHOW_MODE mode, E_FINGER_POSITION finger_pos, const char* file_path)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	return fap50_method->save_file(mode, finger_pos, file_path);
}

IMD_STD_API IMD_RESULT get_image(ImageProperty& img_property)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	return fap50_method->get_image(img_property);
}

IMD_STD_API IMD_RESULT usb_switch(BYTE usb_sel)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	return fap50_method->usb_switch(usb_sel);
}

IMD_STD_API IMD_RESULT set_led_speech_standby_mode()
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	return fap50_method->set_led_speech_standby_mode();
}

IMD_STD_API IMD_RESULT set_burn_code()
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	return fap50_method->set_burn_code();
}

IMD_STD_API IMD_RESULT user_space(bool is_write, WORD offset, BYTE* data, int len)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	return fap50_method->user_space(is_write, offset, data, len);
}

IMD_STD_API IMD_RESULT signature(E_SIGNATURE_ACTION action)
{
	if (get_imd_fap50() == nullptr)
		return IMD_RLT_NULL_POINTER;

	return fap50_method->signature(action);
}

IMD_STD_API IMD_RESULT set_android_info(int fd, unsigned int uiVID, unsigned int uiPID)
{
	UT_SetAndroidInfo(fd, uiVID, uiPID);
	return IMD_RESULT(0);
}
/* New pen feature pending
IMD_STD_API IMD_RESULT start_polling(int DevType)
{
	UT_start_polling(static_cast<UsbDeviceType>(DevType));
	return IMD_RESULT(0);
}

IMD_STD_API IMD_RESULT stop_polling(int DevType)
{
	UT_stop_polling(static_cast<UsbDeviceType>(DevType));
	return IMD_RESULT(0);
}
*/
//IMD_STD_API IMD_RESULT serial_number(BOOL is_write, BYTE* data)
//{
//	if (get_imd_fap50() == nullptr)
//		return IMD_RLT_NULL_POINTER;
//
//	return fap50_method->serial_number(is_write, data);
//}
