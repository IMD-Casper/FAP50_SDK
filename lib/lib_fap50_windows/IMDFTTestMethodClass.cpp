#include "IMDFTTestMethodClass.h"
#include "IMDFPSApi.h"
#include "SensorCommonRegister.h"
#include "ErrCode.h"
#include "pen_mdoule_interface.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define _DEBUG_PRINTF_			1
#define _ENABLE_MESSAGELOG_		1
#define WOI_TEST 0
#define CheckDuplicatValue      1  // to check candidiate value if it calibrate DC Offset before

#define COLOR_RED		RGB(255,0,0)
#define COLOR_GREEN		RGB(0,255,0)
#define COLOR_BLUE		RGB(0,0,255)
#define COLOR_YELLOW	RGB(255,255,0)
#define COLOR_PINK		RGB(255,0,255)
#define COLOR_AQUA		RGB(0,255,255)
#define COLOR_ORANGE	RGB(255,165,0)
#define COLOR_OLIVE		RGB(128,128,0)

#define MODULE_FAP11        1
#define MODULE_FAP20        2
#define MODULE_FAP30        3
#define KEYCORE    0
#define STANDARD   1

//EXPORT_IMD_DLL() 
IMDFTTestMethodClass::IMDFTTestMethodClass(void)
{ 
	m_Width = IMAGE_WIDTH;
	m_Height = IMAGE_HEIGHT;

	enable_runtime_judge_black = true;
	enable_auto_cali_on_reset = true;
	module_info = ModuleInfo{ 0 };
	scan_done = NULL;
	mat_no_isp = Mat(m_Height, m_Width, CV_8UC1, Scalar(255));
	mat_avg = Mat(m_Height, m_Width, CV_8UC1, Scalar(255));
	sign_img = Mat(m_Height, m_Width, CV_8UC1, Scalar(255)),
	sign_img_done = Mat(m_Height, m_Width, CV_8UC1, Scalar(255));

	pen_open();	
	//open_usb_device(get_usb_device(DEVICE_PEN_LP5));
}

//EXPORT_IMD_DLL() 
IMDFTTestMethodClass::~IMDFTTestMethodClass(void)
{
	//close_usb_device(get_usb_device(DEVICE_PEN_LP5));
}

// Calculate image histogram and find maximum histogram value
void IMDFTTestMethodClass::CalculateHistogram(
	Histogram* myHistogram, LPBYTE lpImage, DWORD dwImageSize)
{
	//DWORD dwPeakH;
	UINT m_x = m_Width;
	//Histogram* myHistogram = &a_ptFTTestParams->mHistogram;
	WORD m_HistogramShrinkPixelNumber = 0;// myHistogram->HigramShrinkPixelNum;
	DWORD m_HistogramTotalPixelNumber = 0;

	// Erasing ...
	if (lpImage == NULL || dwImageSize < 1024)
		return;

	// If a_ptFTTestParams->mWindowingWOI.PixelXstep > 1, then the imagesize will be reduced.
	// This is to simulate WOI function
	//a_ptFTTestParams->mWindowingWOI.PixelXstep = 1;
	//if (a_ptFTTestParams->mWindowingWOI.PixelXstep > 1)
	//{
	//	m_x = m_x / a_ptFTTestParams->mWindowingWOI.PixelXstep;
	//	dwImageSize = dwImageSize / a_ptFTTestParams->mWindowingWOI.PixelXstep;
	//}

	UINT i, x, y;
	ZeroMemory(myHistogram->dwHistogram, sizeof(myHistogram->dwHistogram));
	m_HistogramTotalPixelNumber = 0; // amelie 1013 added
	i = dwImageSize / m_x;
	for (x = m_HistogramShrinkPixelNumber; x < (m_x - m_HistogramShrinkPixelNumber); x++)
		for (y = m_HistogramShrinkPixelNumber; y < (i - m_HistogramShrinkPixelNumber); y++)
		{
			//20131224 AlbertKe+-, disable inverse pixel value.
			//dwKPCount[255 - lpImage[y * m_x + x]] += 1;
			BYTE px = lpImage[y * m_x + x];
			myHistogram->dwHistogram[px] += 1;
			m_HistogramTotalPixelNumber++;
		}

	myHistogram->HigramTotalPixelNum = m_HistogramTotalPixelNumber;

	//Calculate maximum histogram value
	DWORD dwPeakValue = 0, dwPixCount = 0;
	for (i = 255; i > 0; i--)
	{
		dwPixCount += myHistogram->dwHistogram[i];
		if (dwPixCount >= myHistogram->HigramTotalPixelNum / 2)
		{
			myHistogram->dwHistogramPeak = i;
			dwPeakValue = myHistogram->dwHistogram[i];
			break;
		}
	}
}
	

// Calculate image image_contrast value
// old function name is CheckContrast()
DWORD IMDFTTestMethodClass::ContrastCalculation(
	FPContrast* myContrast,
	LPBYTE lpImage,
	LPDWORD a_lpdwHistogram,
	FPContrast* a_pstFPContrast)
{
	DWORD bReturn = ERR_CONTST_SUCCESS;
	DWORD dwPeakValue, dwPeakHist;//  , dwEffectHistPixelNum;
	WORD i, dwHStart = 0, dwHEnd = 0;
	LPDWORD lpdwHistogram;
	INT iHistDiff = 0;
	FLOAT iMean = 0;
	FLOAT iStd = 0;
	//CString strText;
	//FPContrast* myContrast = &a_ptFTTestParams->mFPContrast;

	//pfnMessageLog MessageLog = a_ptFTTestParams->mfnMsgLog;
#if (_DEBUG_PRINTF_ == 1)
	//CString msg;
#endif

	// 1. Search histogram minimum start value and maximum end value.
	// Find the peak
	dwPeakValue = 0;
	dwPeakHist = 0;
	lpdwHistogram = a_lpdwHistogram;

	//#if (_DEBUG_PRINTF_ == 1)
	//	for (i = 0; i < 256; i++)
	//	{
	//		msg.Format(_T("[%d]:%d \r\n"), i, lpdwHistogram[i]);
	//		//OutputDebugString(msg);
	//	}
	//#endif

		//2. Minimum start value search.
	int cnt = 0;
	for (i = 0; i < 256; i++)
	{
		cnt += lpdwHistogram[i];
		if (cnt > 300)
		{
			dwHStart = i;
			a_pstFPContrast->ContrastStart = (BYTE)dwHStart;
			break;
		}
	}

	//3. Maximum end value search.
	cnt = 0;
	for (i = 255; i > 0; i--)
	{
		cnt += lpdwHistogram[i];
		if (cnt > 300)
		{
			dwHEnd = i;
			a_pstFPContrast->ContrastEnd = (BYTE)dwHEnd;
			break;
		}
	}

	//5. Search bright peak
	a_pstFPContrast->wMaxHist = 0;
	a_pstFPContrast->dwMaxHistPixelNum = 0;
	for (i = ((dwHStart + dwHEnd) / 2) - 5; i <= dwHEnd; i++)
		if (lpdwHistogram[i] > a_pstFPContrast->dwMaxHistPixelNum)
		{
			a_pstFPContrast->wMaxHist = (BYTE)i;
			a_pstFPContrast->dwMaxHistPixelNum = lpdwHistogram[i];
		}

	if (a_pstFPContrast->wMaxHist == 0
		&& a_pstFPContrast->dwMaxHistPixelNum == 0
		&& lpdwHistogram[255] > a_pstFPContrast->dwEffectHistPixelNum
		)
	{
		a_pstFPContrast->wMaxHist = 255;
		a_pstFPContrast->dwMaxHistPixelNum = lpdwHistogram[255];
	}

	//4. Search dark peak
	a_pstFPContrast->wMinHist = 0;
	a_pstFPContrast->dwMinHistPixelNum = 0;
	int PeakWidth = (dwHStart + a_pstFPContrast->wMaxHist) / 2;
	for (i = dwHStart; i < PeakWidth + 1; i++)
		if (lpdwHistogram[i] > a_pstFPContrast->dwMinHistPixelNum)
		{
			a_pstFPContrast->wMinHist = (BYTE)i;
			a_pstFPContrast->dwMinHistPixelNum = lpdwHistogram[i];
		}


	//6. Check if the image_contrast of maximum - minimum > range
	myContrast->Contrast = dwHEnd - dwHStart;

	//if (a_pstFPContrast->wContrast < a_pstFPContrast->bCriteriaLow)
	//	bReturn = ERR_CONTST_UNDERRANGE;

	//7. Check if the image_contrast of maximum - minimum < range
	//if (a_pstFPContrast->wContrast > a_pstFPContrast->bCriteriaHigh)
	//	bReturn = ERR_CONTST_OVERRANGE;

//#if (_DEBUG_PRINTF_ == 1)
//	{
//		TRACE(_T("CheckContrast - dwHStart:%d, dwHEnd:%d, MinHist:%d, MinHistPixelNum:%d, MaxHist:%d, MaxHistPixelNum:%d\r\n"), \
//			dwHStart, dwHEnd, a_pstFPContrast->wMinHist, a_pstFPContrast->dwMinHistPixelNum, a_pstFPContrast->wMaxHist, a_pstFPContrast->dwMaxHistPixelNum);
//		//OutputDebugString(msg);
//	}
//#endif

	// Check std value. Must image will get error calibration value.
	//CalculateMeanStdbyWOI(&a_ptFTTestParams->mFPContrast.CstWOI, lpImage, &iMean, &iStd);
	myContrast->fStd = iStd;

	//if (iStd < myContrast->StdCriteria)
	//{
	//	strText.Format(_T("Contrast Std under!! Criteria:%d,Measure:%d\n"), myContrast->StdCriteria, myContrast->Std);
	//	_sntprintf_s(a_ptFTTestParams->mMegString, MSGSTRINGSIZE + 1, strText);
	//	MessageLog(strText);
	//	return ERR_CONTST_STDUNDER;
	//}

	return bReturn;
}


//#include "../CommonHeader/C_IMG.h"
//Catpure image function for FT test and no hardware windowing function.
BOOL IMDFTTestMethodClass::DoCaptureFT(FTTestParameter* a_ptFTTestParams, HANDLE a_hDevice)
{
	DWORD dwError = ERROR_SUCCESS;
	WAIT_SCAN_PARAMETER ScanParam{};
	WAIT_SCAN_PARAMETER *pScan= &ScanParam;
	BOOL bRet = TRUE;
	//CString strText;
	//DWORD ttStart, ttEnd;
	pfnMessageLog MessageLog = a_ptFTTestParams->mfnMsgLog;
	//WindowingWOI *woi = &a_ptFTTestParams->mFPContrast.CstWOI;
	//WindowingWOI_ptr SWWOIptr = &a_ptFTTestParams->mWindowingWOI;

	show_mode = GUI_SHOW_MODE_CAPTURE;

	//strText = L"";
	a_ptFTTestParams->mBINErrType.dwError = 0;
	a_ptFTTestParams->mBINErrType.mBin99Err.bFlag = FALSE;
	a_ptFTTestParams->mScan = pScan;
	DWORD raw_data_size = RAW_SIZE;
	pScan->wChipID = a_ptFTTestParams->mTestingChipID;
	pScan->nPixelCntX = RAW_WIDTH;
	pScan->nPixelCntY = RAW_HEIGHT;
	pScan->img = fap50_img;
	pScan->raw = fap50_raw;
	pScan->nWaitTime = 0;

	if (a_ptFTTestParams->mDebugMode.CstMsgEnable)
		if (false == enable_continue_scan)
		{
			//::SendNotifyMessage(HWND_BROADCAST, CMD_ERASE_IMG, (WPARAM)pScan, 0);
			Sleep(10); //need to add this for UI
		}

	//m_ring.write_idx = -1;

	pScan->hw_scan_mode = HW_SCAN_MODE_FLAT;
	switch (pScan->hw_scan_mode) 
	{
	default:
	case HW_SCAN_MODE_FLAT: 
		pScan->raw_size = a_ptFTTestParams->raw_data_size;
		break;
	case HW_SCAN_MODE_ROLL:
		pScan->raw_size = a_ptFTTestParams->raw_data_size / 2;
		break;
	case HW_SCAN_MODE_FAST:
		pScan->raw_size = a_ptFTTestParams->raw_data_size / 4;
		break;
	}

	if (a_ptFTTestParams->m_do_with_bmp_file)
		goto FUNC_END;

	if (a_ptFTTestParams->mDebugMode.CstMsgEnable)
	{
		//SetRxDcoffset(a_ptFTTestParams, a_hDevice);
		dwError = UT_CaptureImageEx(a_hDevice, pScan);
		//iShiftPixel = pScan->nShiftPixelX;
		//if (enable_continue_scan == FALSE)
		//	UT_GetFrameTime(a_hDevice, m_frame_time);
	}

	if (dwError != ERROR_SUCCESS)
	{
		//strText.AppendFormat(_T("\r\nCaptureImage Err = 0x%x"), dwError);
		//strText.AppendFormat(_T("\r\nPlease do power cycle!"));
		//strText.AppendFormat(_T("\r\nProgram will be terminated!\r\n"));
		a_ptFTTestParams->mBINErrType.mBin99Err.bFlag = TRUE;
		a_ptFTTestParams->mBINErrType.mBin99Err.dwError = dwError;
		a_ptFTTestParams->mBINErrType.dwError = dwError;
		a_ptFTTestParams->mBINErrType.mBinErrNum = 99;
		//_sntprintf_s(a_ptFTTestParams->mMegString, MSGSTRINGSIZE + 1, strText);
		//MessageLog(strText);
		bRet = FALSE;
		goto FUNC_END;
	}	

	//WORD* frm = m_frame_time;
	//TRACE(L"CaptureTime...MCU:%dms USB:%dms\r\n", frm[0], frm[1]);

FUNC_END:
	if (a_ptFTTestParams->mDebugMode.CstMsgEnable)
	{
		ImageStatus img_status{};
		IMD_RESULT ret = raw_to_img_fap50(pScan, &img_status);
		pScan->img_status = img_status;

		if (ret == IMD_RLT_SUCCESS)
		{
			//::SendNotifyMessage(HWND_BROADCAST, CMD_DRAW_IMG, 0, (LPARAM)pScan);
			//::SendNotifyMessage(HWND_BROADCAST, CMD_DRAW_HISTRO, (WPARAM)raw_data_size, (LPARAM)pScan);
		}
	}

	return bRet;
}

IMD_RESULT IMDFTTestMethodClass::background_calibration_fap50(HANDLE a_hDevice)
{
#define CALI_TIMEOUT 20

	typedef union {
		struct {
			BYTE iG1 : 1;
			BYTE iG2 : 1;
			BYTE iG3 : 1;
			BYTE iG4 : 1;
		};
		BYTE all;
	}iGCaliStatus;
	iGCaliStatus cali_status;
	cali_status.all = 0;

	WAIT_SCAN_PARAMETER ScanParam{};
	WAIT_SCAN_PARAMETER* pScanParam = &ScanParam;
	pScanParam->nPixelCntX = RAW_WIDTH;
	pScanParam->nPixelCntY = RAW_HEIGHT;
	pScanParam->img = fap50_img;
	pScanParam->raw = fap50_raw;
	pScanParam->nWaitTime = 0;
	pScanParam->hw_scan_mode = HW_SCAN_MODE_FLAT;// HW_SCAN_MODE_FAST;// HW_SCAN_MODE_FLAT;
	pScanParam->raw_size = RAW_SIZE;

	Histogram hist;
	FPContrast cont;
	Histogram *myHistogram = &hist;
	FPContrast *myFPContrast = &cont;
	BYTE bright_target = 230;
	BYTE ig_raw[256 * 1600];
	DWORD ig_y_length = 1600;

	myFPContrast->CstWOI.WOI_StartX = 0;
	myFPContrast->CstWOI.WOI_StartY = 0;
	myFPContrast->CstWOI.WOI_LengthX = 256;
	myFPContrast->CstWOI.WOI_LengthY = ig_y_length;
	myFPContrast->CstWOI.img_width = myFPContrast->CstWOI.WOI_LengthX;
	myFPContrast->CstWOI.img_height = myFPContrast->CstWOI.WOI_LengthY;

	int timeout;
	for (timeout = 0; timeout < CALI_TIMEOUT; timeout++)
	{
		DWORD dwError = UT_CaptureImageEx(a_hDevice, pScanParam);
		if (dwError)
			return IMD_RLT_USB_READ_IMAGE_EXCEPTION;

		for (int i = 0; i < 4; i++)
		{
			if ((cali_status.all & (1 << i)) == (1 << i))
				continue;

			for (DWORD raw_y = 0; raw_y < ig_y_length; raw_y++)
				memcpy(ig_raw + raw_y * 256, fap50_raw + raw_y * 1024 + i * 256, 256);

			CalculateHistogram(myHistogram, ig_raw, sizeof(ig_raw));
			ContrastCalculation(myFPContrast, ig_raw, myHistogram->dwHistogram, myFPContrast);

			//20230616 boxchu: bright using avg is precise. 
			int bright = myFPContrast->wMaxHist;// myFPContrast->ContrastEnd;// myFPContrast->wMaxHist;
			int ig_num = i + 1;
			int delta = (bright - bright_target);

			if (timeout < 10) //reg1A calibration
			{
				WORD RegAddr = ((ig_num & 0xF) << 12) | ((0 & 0xF) << 8) | 0x1A;// &0xFF;
				BYTE RegValue = 0;
				UT_GetSensorRegister(a_hDevice, RegAddr, &RegValue);
				BYTE dc_value = RegValue >> 2;
				if (0 <= delta && delta <= 3)
				{
					cali_status.all |= 1 << i;
					cali_done_reg.reg1A[i] = RegValue;
					continue;
				}

				BYTE step = abs(delta * 25 / 100);
				if (step == 0)
					step++;

				dc_value = (bright <= bright_target) ? dc_value + step : dc_value - step;
				BYTE new_reg_value = (RegValue & 0x3) | (dc_value << 2);
				dwError = UT_SetDcOffsetWithLoadEn(a_hDevice, RegAddr, new_reg_value);
			}
			else //reg13 calibration
			{
				WORD RegAddr = ((ig_num & 0xF) << 12) | ((0 & 0xF) << 8) | 0x13;// &0xFF;
				BYTE RegValue = 0;
				UT_GetSensorRegister(a_hDevice, RegAddr, &RegValue);
				if (0 == delta)
				{
					cali_status.all |= 1 << i;
					cali_done_reg.reg13[i] = RegValue;
					continue;
				}

				BYTE step = abs(delta);
				RegValue = (bright <= bright_target) ? RegValue + step : RegValue - step;
				UT_SetSensorRegister(a_hDevice, RegAddr, RegValue);
			}
		}

		if ((cali_status.all & 0xF) == 0xF)
		{
			if (timeout > 10)
				break;

			timeout = 10;
			cali_status.all = 0;
		}
	}

	if (timeout == CALI_TIMEOUT)
	{
		dbg("IMD_RLT_CLAI_TIMEOUT\n");
		return IMD_RLT_CLAI_TIMEOUT;
	}

	return IMD_RLT_SUCCESS;
}

int img_filter_fap50(BYTE* img, BYTE CaptureMode)
{
#define PX_CNT_TH 8
	UINT64 t_start = timeGetTime();
	BYTE px = 0;
	int px_cnt = 0;
	//continue 16 black/white point, 
	{
		DWORD repair_line[]{ 3, 27, 250, 252,253, 258, 260,276, 723,739,747,748,749, 998,999 };
		int start = 0, end = 1600;
		if (CaptureMode == HW_SCAN_MODE_ROLL)
		{
			start = (1600 / 4);
			end = 1600 - (1600 / 4);
		}

		px_cnt = 0;
		for (int i = 0; i < sizeof(repair_line) / sizeof(repair_line[0]); i++)
		{
			int line = repair_line[i];
			BYTE* img_line = &img[1600 * line];
			for (int j = start; j < end; j++)
			{
				px = *(img_line + j);
				//if (px == 0x00 || px == 0xFF)
				if (px == 0xFF)
				{
					++px_cnt;
					if (px_cnt >= PX_CNT_TH)
						goto err_handle;
				}
				else
				{
					px_cnt = 0;
				}
			}
		}
	}

	//	TRACE("img_filter_fap50() %dms\n", timeGetTime()- t_start);
	return 0;

err_handle:
//	TRACE("Rolling fail detect! over PX_CNT_TH... px=0x%02X\n", px);
	return 1;
}

void get_histogram_fap50(BYTE* img, int* black, int* white, int* middle, E_HW_SCAN_MODE hw_scan_mode, long *hist)
{
	//long hist[256]{};
	long cnt = 0;

	long sum = 0;
	if (hw_scan_mode == HW_SCAN_MODE_FLAT)
	{
		for (long i = 0; i < 1000; i += 4)
			for (long j = 0; j < 1600; j += 4)
			{
				BYTE px = img[i * 1600 + j];
				hist[px]++;
				sum += px;
				cnt++;
			}
	}
	else if (hw_scan_mode == HW_SCAN_MODE_ROLL)
	{
		for (long i = 0; i < 1000; i += 4)
			for (long j = 400; j < 1200; j += 4)
			{
				BYTE px = img[i * 1600 + j];
				hist[px]++;
				sum += px;
				cnt++;
			}
	}
	//else if (hw_scan_mode == HW_SCAN_MODE_SIGNATURE_2)
	//{
	//	for (long i = 0; i < 1000; i += 2) //y
	//		for (long j = 0; j < 400; j += 4) //x
	//		{
	//			BYTE px = img[i * 1600 + j];
	//			hist[px]++;
	//			sum += px;
	//			cnt++;
	//		}
	//}

	if (cnt && middle)
		*middle = sum / cnt;

	//if (hw_scan_mode == HW_SCAN_MODE_SIGNATURE_2)
	//{
	//	sum = 0;
	//	for (long i = 0; i < 256; i++)
	//	{
	//		sum += hist[i];
	//		if (sum > 10)
	//		{
	//			if (black)
	//				*black = i;
	//			break;
	//		}
	//	}
	//	//dbg(L"black:%d num=%d\n", *black, sum);

	//	sum = 0;
	//	for (long i = 255; i > 0; i--)
	//	{
	//		sum += hist[i];
	//		if (sum > 40)
	//		{
	//			if (white)
	//				*white = i;
	//			break;
	//		}
	//	}
	//}
	//else
	{
		if(black)
		{
			sum = 0;
			for (long i = 0; i < 256; i++)
			{
				sum += hist[i];
				if (sum > 40)
				{
					*black = i;
					break;
				}
			}
		}

		if(white)
		{
			sum = 0;
			for (long i = 255; i > 0; i--)
			{
				sum += hist[i];
				if (sum > 40)
				{
					*white = i;
					break;
				}
			}
		}
	}
}

BOOL get_line( BYTE* pImg, long finger_on_th, long* pLine, long* pLeft_x, long* pRight_x,
	long* pFinger_width, E_HW_SCAN_MODE hw_scan_mode
)
{
	long line = -1;

	long x_start = 0, x_end = 1600, y_start = 0, y_end = 1000;
	if (hw_scan_mode == HW_SCAN_MODE_ROLL)
		x_start = 400, x_end = 1200, y_start = 0, y_end = 1000;
	long left_x=0, right_x=x_end-1, cnt=0;

	cnt = 0;
	for (long x = x_start; x < x_end; x++)
		for (long y = y_start; y < y_end; y++)
		{
			BYTE px = pImg[y * 1600 + x];
			if (px == 0)
				continue;

			BYTE delta = 255 - px;
			if (delta > finger_on_th)
			{
				if (abs(left_x - x) > 1)
					cnt = 0;

				cnt++;
				left_x = x;

				if (cnt > 3)
					x = 1600;

				break;
			}
		}
	if (cnt < 3)
		left_x = -1;

	cnt = 0;
	for (long x = x_end - 1; x > x_start - 1; x--)
		for (long y = y_start; y < y_end; y++)
		{
			BYTE px = pImg[y * 1600 + x];
			if (px == 0)
				continue;

			BYTE delta = 255 - px;
			if (delta > finger_on_th)
			{
				if (abs(right_x - x) > 1)
					cnt = 0;

				cnt++;
				right_x = x;
				if (cnt > 3)
					x = 0;

				break;
			}
		}
	if (cnt < 3)
		right_x = -1;

	line = (right_x + left_x) / 2;

	int finger_width = abs(right_x - left_x);
	if (finger_width == 0)
		finger_width = 1;

	*pFinger_width = finger_width;
	*pLine = line;
	*pLeft_x = left_x;
	*pRight_x = right_x;

	return 1;
}

IMD_RESULT IMDFTTestMethodClass::raw_to_img_fap50(void* pPara, ImageStatus* img_status)
{
	WAIT_SCAN_PARAMETER* scan = (WAIT_SCAN_PARAMETER*)pPara;
	BYTE* raw = scan->raw,
		* img = scan->img;
	E_HW_SCAN_MODE hw_scan_mode = (E_HW_SCAN_MODE)scan->hw_scan_mode;
	img_status->show_mode = show_mode;
	img_status->img = scan->img;//scan->raw;//scan->img;
	img_status->result = IMD_RLT_SUCCESS;

	//frame rate ------------------------------------------------------
	{
#define FRAME_TIME_SIZE 2
		static deque<double> frame_times(FRAME_TIME_SIZE, 100);
		static UINT64 t_frame = (UINT64)timeGetTime() - 100;
		frame_times.push_back((UINT64)timeGetTime() - t_frame);
		t_frame = (UINT64)timeGetTime();

		if (frame_times.size() >= FRAME_TIME_SIZE)
			frame_times.pop_front();
		static float frame_rate = 0.0;
		static UINT64 t_update_frame_rate = 0;
		if ((UINT64)timeGetTime() - t_update_frame_rate > 250)
		{
			frame_rate = 1000.0 / (std::accumulate(frame_times.begin(), frame_times.end(), 0) / frame_times.size());
			t_update_frame_rate = (UINT64)timeGetTime();
			//dbg("frame_time:%.1f\n", frame_rate);
		}

		img_status->frame_rate = frame_rate;
		if (show_mode == GUI_SHOW_MODE_SIGNATURE_BY_PEN)
			return IMD_RESULT(0);
	}

	IMD_RESULT func_ret = IMD_RLT_FAIL;
	UINT64 t_start = timeGetTime(), t_repair = 0, t_sw_auto_gain = 0, t_mapping = 0, t_moving_avg = 0;
	long sel_num = 4, hcnt_num = 256, rx_num = 200, des, src,
		raw_w = 1024, raw_h = 1600, img_w = 1600, img_h = 1000,
		subframe_size = raw_w * raw_h / 8;
	vector<BYTE> vec_img_mapping(img_w * img_h);
	BYTE* img_mapping = &vec_img_mapping[0];
	BYTE* img_fast = scan->fast;

//-------------------------------------------------------------------
	switch (panel_type)  {
	case RULE_1:
		if (HW_SCAN_MODE_SIGNATURE_2 == hw_scan_mode)
		{
			//memcpy(img_fast, raw, 2 * subframe_size);
			memset(raw, 0xFF, 8 * subframe_size);
			for (int subfrm = 0; subfrm < 2; subfrm++)
				for (int ic = 0; ic < 4; ic++)
					for (int rx = 0; rx < rx_num; rx++)
						memcpy(raw + (subfrm + (rx % 4) * 2) * subframe_size + rx * 1024 + ic * 256, img_fast + subfrm * subframe_size + rx * 1024 + ic * 256, 256);
		}

		for (long sel = 0; sel < sel_num; sel++)
		{
			//outsied
			for (long rx = 0; rx < rx_num; rx++)
			{
				//IC1
				memcpy(img_mapping + (sel * 400 * 250 * 4) + (rx * img_h),
					raw + (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 0 + 6, 250);
				//IC2
				memcpy(img_mapping + (sel * 400 * 250 * 4) + ((rx + 200) * img_h),
					raw + (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 1 + 6, 250);
				//IC3
				des = (sel * 400 * 250 * 4) + (rx * img_h) + (250 * 3) + (249);
				src = (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 2 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
				//IC4
				des = (sel * 400 * 250 * 4) + ((rx + 200) * img_h) + (250 * 3) + (249);
				src = (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 3 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
			}
			//inside
			for (long rx = 0; rx < rx_num; rx++)
			{
				//IC1
				memcpy(img_mapping + (sel * 400 * 250 * 4) + (rx * img_h) + 250,
					raw + (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 0 + 6, 250);
				//IC2
				memcpy(img_mapping + (sel * 400 * 250 * 4) + ((rx + 200) * img_h) + 250,
					raw + (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 1 + 6, 250);
				//IC3
				des = (sel * 400 * 250 * 4) + (rx * img_h) + (250 * 2) + (249);
				src = (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 2 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
				//IC4
				des = (sel * 400 * 250 * 4) + ((rx + 200) * img_h) + (250 * 2) + (249);
				src = (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 3 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
			}
		}
		break;

	case RULE_1_1:
	{
		if (HW_SCAN_MODE_SIGNATURE_2 == hw_scan_mode)
		{
			//memset(raw, 0xFF, 8 * subframe_size);
			Mat mat_raw = Mat(1600, 1024, CV_8UC1, raw).setTo(Scalar(255));

			for (long subfrm = 0; subfrm < 2; subfrm++)
			{
				long ic = 0;

				ic = 0;
				for (long rx = 0; rx < rx_num; rx++)
					memcpy(raw + (subfrm + (3-rx % 4) * 2) * subframe_size + rx * 1024 + ic * 256, 
						img_fast + subfrm * subframe_size + rx * 1024 + ic * 256, 256);
				ic = 1;
				for (long rx = 0; rx < rx_num; rx++)
					memcpy(raw + (subfrm + (3-rx % 4) * 2) * subframe_size + rx * 1024 + ic * 256,
						img_fast + subfrm * subframe_size + rx * 1024 + ic * 256, 256);
				ic = 2;
				for (long rx = 0; rx < rx_num; rx++)
					memcpy(raw + (subfrm + (rx % 4) * 2) * subframe_size + rx * 1024 + ic * 256,
						img_fast + subfrm * subframe_size + rx * 1024 + ic * 256, 256);
				ic = 3;
				for (long rx = 0; rx < rx_num; rx++)
					memcpy(raw + (subfrm + (rx % 4) * 2) * subframe_size + rx * 1024 + ic * 256,
						img_fast + subfrm * subframe_size + rx * 1024 + ic * 256, 256);
			}
		}

		int sel_start = 0, sel_end = sel_num;
		if (hw_scan_mode == HW_SCAN_MODE_ROLL)
			sel_start = 1, sel_end = sel_num - 1;
		//else if (hw_scan_mode == HW_SCAN_MODE_SIGNATURE)
		//	sel_start = 0, sel_end = 1;

		for (long sel = sel_start; sel < sel_end; sel++)
		{
			//outsied
			for (long rx = 0; rx < rx_num; rx++)
			{
				//IC1
				memcpy(img_mapping + (sel * 400 * 250 * 4) + (rx * img_h),
					raw + (sel * 400 * 256 * 4) + ((199 - rx) * 256 * 4) + 256 * 0 + 6, 250);
				//IC2
				memcpy(img_mapping + (sel * 400 * 250 * 4) + ((rx + 200) * img_h),
					raw + (sel * 400 * 256 * 4) + ((199 - rx) * 256 * 4) + 256 * 1 + 6, 250);
				//IC3
				des = (sel * 400 * 250 * 4) + (rx * img_h) + (250 * 3) + (249);
				src = (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 2 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
				//IC4
				des = (sel * 400 * 250 * 4) + ((rx + 200) * img_h) + (250 * 3) + (249);
				src = (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 3 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
			}
			//inside
			for (long rx = 0; rx < rx_num; rx++)
			{
				//IC1
				memcpy(img_mapping + (sel * 400 * 250 * 4) + (rx * img_h) + 250,
					raw + (sel * 400 * 256 * 4) + subframe_size + ((199 - rx) * 256 * 4) + 256 * 0 + 6, 250);
				//IC2
				memcpy(img_mapping + (sel * 400 * 250 * 4) + ((rx + 200) * img_h) + 250,
					raw + (sel * 400 * 256 * 4) + subframe_size + ((199 - rx) * 256 * 4) + 256 * 1 + 6, 250);
				//IC3
				des = (sel * 400 * 250 * 4) + (rx * img_h) + (250 * 2) + (249);
				src = (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 2 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
				//IC4
				des = (sel * 400 * 250 * 4) + ((rx + 200) * img_h) + (250 * 2) + (249);
				src = (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 3 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
			}
		}

		//Mat dbg = Mat(1600, 1000, CV_8UC1, img_mapping), r_dbg;
		//rotate(dbg, r_dbg, ROTATE_90_COUNTERCLOCKWISE);
		////imshow("dbg", dbg.clone());
		//imshow("r_dbg", r_dbg.clone());

		break;
	}

	case RULE_1A:
	{
		if (HW_SCAN_MODE_SIGNATURE_2 == hw_scan_mode)
		{
			//memcpy(img_fast, raw, 2 * subframe_size);
			memset(raw, 0xFF, 8 * subframe_size);
			for (long subfrm = 0; subfrm < 2; subfrm++)
				for (long ic = 0; ic < 4; ic++)
					for (long rx = 0; rx < rx_num; rx++)
						memcpy(raw + (subfrm + (rx % 2) * 4 + (ic % 2) * 2) * subframe_size + rx * 1024 + ic * 256, img_fast + subfrm * subframe_size + rx * 1024 + ic * 256, 256);
		}

		for (long sel = 0; sel < sel_num; sel++)
		{
			//outsied
			for (long rx = 0; rx < rx_num; rx++)
			{
				//IC1
				memcpy(img_mapping + (sel * 400 * 250 * 4) + (rx * 2 * img_h),
					raw + (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 0 + 6, 250);
				//IC2
				memcpy(img_mapping + (sel * 400 * 250 * 4) + ((rx * 2 + 1) * img_h),
					raw + (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 1 + 6, 250);
				//IC3
				des = (sel * 400 * 250 * 4) + (rx * 2 * img_h) + (250 * 3) + (249);
				src = (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 2 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
				//IC4
				des = (sel * 400 * 250 * 4) + ((rx * 2 + 1) * img_h) + (250 * 3) + (249);
				src = (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 3 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
			}
			//inside
			for (long rx = 0; rx < rx_num; rx++)
			{
				//IC1
				memcpy(img_mapping + (sel * 400 * 250 * 4) + (rx * 2 * img_h) + 250,
					raw + (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 0 + 6, 250);
				//IC2
				memcpy(img_mapping + (sel * 400 * 250 * 4) + ((rx * 2 + 1) * img_h) + 250,
					raw + (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 1 + 6, 250);
				//IC3
				des = (sel * 400 * 250 * 4) + (rx * 2 * img_h) + (250 * 2) + (249);
				src = (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 2 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
				//IC4
				des = (sel * 400 * 250 * 4) + ((rx * 2 + 1) * img_h) + (250 * 2) + (249);
				src = (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 3 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
			}
		}
		break;
	}
	case RULE_1A_1:
	{
		if (HW_SCAN_MODE_SIGNATURE_2 == hw_scan_mode)
		{
			//memset(raw, 0xFF, 8 * subframe_size);
			Mat mat_raw = Mat(1600, 1024, CV_8UC1, raw).setTo(Scalar(255));

			for (long subfrm = 0; subfrm < 2; subfrm++)
			{
				long ic = 0;

				ic = 0;
				for (long rx = 0; rx < rx_num; rx++)
					memcpy(raw + (subfrm + (3 - rx % 4) * 2) * subframe_size + rx * 1024 + ic * 256,
						img_fast + subfrm * subframe_size + rx * 1024 + ic * 256, 256);
				ic = 1;
				for (long rx = 0; rx < rx_num; rx++)
					memcpy(raw + (subfrm + (3 - rx % 4) * 2) * subframe_size + rx * 1024 + ic * 256,
						img_fast + subfrm * subframe_size + rx * 1024 + ic * 256, 256);
				ic = 2;
				for (long rx = 0; rx < rx_num; rx++)
					memcpy(raw + (subfrm + (rx % 4) * 2) * subframe_size + rx * 1024 + ic * 256,
						img_fast + subfrm * subframe_size + rx * 1024 + ic * 256, 256);
				ic = 3;
				for (long rx = 0; rx < rx_num; rx++)
					memcpy(raw + (subfrm + (rx % 4) * 2) * subframe_size + rx * 1024 + ic * 256,
						img_fast + subfrm * subframe_size + rx * 1024 + ic * 256, 256);
			}
		}
		int sel_start = 0, sel_end = sel_num;
		if (hw_scan_mode == HW_SCAN_MODE_ROLL)
			sel_start = 1, sel_end = sel_num - 1;
		//else if (hw_scan_mode == HW_SCAN_MODE_SIGNATURE)
		//	sel_start = 0, sel_end = 1;

		for (long sel = sel_start; sel < sel_end; sel++)
		{
			//outsied
			for (long rx = 0; rx < rx_num; rx++)
			{
				//IC1
				memcpy(img_mapping + (sel * 400 * 250 * 4) + (rx * 2 * img_h),
					raw + (sel * 400 * 256 * 4) + ((199 - rx) * 256 * 4) + 256 * 0 + 6, 250);
				//IC2
				memcpy(img_mapping + (sel * 400 * 250 * 4) + ((rx * 2 + 1) * img_h),
					raw + (sel * 400 * 256 * 4) + ((199 - rx) * 256 * 4) + 256 * 1 + 6, 250);
				//IC3
				des = (sel * 400 * 250 * 4) + (rx * 2 * img_h) + (250 * 3) + (249);
				src = (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 2 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
				//IC4
				des = (sel * 400 * 250 * 4) + ((rx * 2 + 1) * img_h) + (250 * 3) + (249);
				src = (sel * 400 * 256 * 4) + (rx * 256 * 4) + 256 * 3 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
			}
			//inside
			for (long rx = 0; rx < rx_num; rx++)
			{
				//IC1
				memcpy(img_mapping + (sel * 400 * 250 * 4) + (rx * 2 * img_h) + 250,
					raw + (sel * 400 * 256 * 4) + subframe_size + ((199 - rx) * 256 * 4) + 256 * 0 + 6, 250);
				//IC2
				memcpy(img_mapping + (sel * 400 * 250 * 4) + ((rx * 2 + 1) * img_h) + 250,
					raw + (sel * 400 * 256 * 4) + subframe_size + ((199 - rx) * 256 * 4) + 256 * 1 + 6, 250);
				//IC3
				des = (sel * 400 * 250 * 4) + (rx * 2 * img_h) + (250 * 2) + (249);
				src = (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 2 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
				//IC4
				des = (sel * 400 * 250 * 4) + ((rx * 2 + 1) * img_h) + (250 * 2) + (249);
				src = (sel * 400 * 256 * 4) + subframe_size + (rx * 256 * 4) + 256 * 3 + 6;
				for (long i = 0; i < 250; i++)
					img_mapping[des - i] = raw[src + i];
			}
		}
		break;
	}
	default:
		;
	}

//	return IMD_RLT_SUCCESS;
//	dbg("mapping time is %d\n", timeGetTime() - now);

	//Mat dbg_img = Mat(m_Width, m_Height, CV_8UC1, img_mapping);
	//imshow("dbg_img", dbg_img);

	long raw_y_start = 0, raw_y_end = img_w;
	if (hw_scan_mode == HW_SCAN_MODE_ROLL)
		raw_y_start = 400, raw_y_end = 1200;
	//else if (hw_scan_mode == HW_SCAN_MODE_SIGNATURE)
	//	raw_y_start = 0, raw_y_end = 400;

	for (long y = raw_y_start; y < raw_y_end; y++)
		for (long x = 0; x < img_h; x++)
			img[y + (img_h - 1 - x) * img_w] = img_mapping[y * img_h + x];

//	t_mapping = (UINT64)timeGetTime() - t_start;
//	dbg(L"t_mapping=%d\n", t_mapping);//2~3ms

//#define DEBUG_IMG "d:/finger_num_err/num_err_05.png" 
//#define DEBUG_IMG "d:/finger_num_err/num_err_10.png" 
//#define DEBUG_IMG "d:/finger_num_err/num_err_11.png"  
//#define DEBUG_IMG "d:/finger_num_err/num_err_13.png"  
//#define DEBUG_IMG "d:/finger_num_err/num_err_16.png"   
//#define DEBUG_IMG "d:/fingers_split_err/split_err_07.png"
#ifdef DEBUG_IMG
	Mat cv_img = imread(DEBUG_IMG, IMREAD_GRAYSCALE);
	memcpy(img, cv_img.data, IMAGE_SIZE);
	need_finger_off = FALSE;
#endif

	Mat mat_img = Mat(m_Height, m_Width, CV_8UC1, img);

//get histogram -----------------------------------------------------
	int black = 0, white = 0, middle = 0, image_contrast=0;
	//auto t_get_hist = timeGetTime();
	vector<long> hist(256, 0);
	get_histogram_fap50(img, &black, &white, &middle, hw_scan_mode, &hist[0]);
	//int black_peak = 0, peak_cnt = 0;// *max_element(hist.begin(), hist.end());
	//for (int i = 0; i < 185; i++)
	//	if (peak_cnt < hist[i])
	//	{
	//		peak_cnt = hist[i];
	//		black_peak = i;
	//	}
	//if (peak_cnt == 0)
	//	NOP();

	//	dbg(L"t_get_hist=%d\n", timeGetTime()-t_get_hist);//0~1ms

	image_black = black;
	image_white = white;
	image_contrast = white - black;
	//dbg(L"b=%d w=%d c=%d\n", image_black, image_white, image_contrast);//0~1ms

	int idx = m_ring.write_idx + RING_SIZE;
	int idx_a = (idx - 1) % RING_SIZE;
	int idx_b = (idx - 2) % RING_SIZE;
	int idx_c = (idx - 3) % RING_SIZE;
	BYTE* img_a = m_ring.unit[idx_a].img;
	BYTE* img_b = m_ring.unit[idx_b].img;
	BYTE* img_c = m_ring.unit[idx_c].img;


// -----------------------------------------------------
//filter abnomal image 
	{
		m_ring.unit[idx_a].valid = TRUE;
		if (hw_scan_mode == HW_SCAN_MODE_FLAT)
			if (img_filter_fap50(img, hw_scan_mode) != 0)
				m_ring.unit[idx_a].valid = FALSE; //goto err_handle;
	}

// -----------------------------------------------------
//repair line 
	{
		t_repair = (UINT64)timeGetTime();
		if (enable_repair_line
			&& (hw_scan_mode == HW_SCAN_MODE_FLAT
				|| hw_scan_mode == HW_SCAN_MODE_ROLL)
			)
		{
			int unit = 200;
			//top-----------------------------
			long repair_line[]{ 0,1,9,25, 250,251,259,275 };
			for (long r = 0; r < sizeof(repair_line) / sizeof(long); r++)
				for (long sub = 0; sub < 1600; sub += unit)
				{
					long ref_sum = 0, repair_sum = 0;
					long ref_line = (repair_line[r] % 250 == 0) ? repair_line[r] + 2 : repair_line[r] + 1;

					{
						long y = repair_line[r];
						long* sum = &repair_sum;
						long base = sub + y * 1600;
						for (long x = base; x < base + unit; x += 2)
							*sum += img[x];
						repair_sum /= (unit / 2);
					}
					{
						long y = ref_line;
						long* sum = &ref_sum;
						long base = sub + y * 1600;
						for (long x = base; x < base + unit; x += 2)
							*sum += img[x];
						ref_sum /= (unit / 2);
					}

					long base = sub + repair_line[r] * 1600;
					long delta = ref_sum - repair_sum;
					for (long x = base; x < base + unit; x++)
					{
						int px = img[x] + delta;
						if (px > 255) px = 255;
						if (px < 0) px = 0;
						img[x] = px;
					}
				}

			//bottom-----------------------------
			long repair_line2[]{ 724,740,748,749, 974,990,998,999 };
			//if(0)
			for (long r = 0; r < sizeof(repair_line2) / sizeof(long); r++)
				for (long sub = 0; sub < 1600; sub += unit)
				{
					long ref_sum = 0, repair_sum = 0;
					long ref_line = (repair_line2[r] % 250 == 249) ? repair_line2[r] - 2 : repair_line2[r] - 1;

					{
						long y = repair_line2[r];
						long* sum = &repair_sum;
						long base = sub + y * 1600;
						for (long x = base; x < base + unit; x += 2)
							*sum += img[x];
						repair_sum /= (unit / 2);
					}
					{
						long y = ref_line;
						long* sum = &ref_sum;
						long base = sub + y * 1600;
						for (long x = base; x < base + unit; x += 2)
							*sum += img[x];
						ref_sum /= (unit / 2);
					}

					long base = sub + repair_line2[r] * 1600;
					long delta = ref_sum - repair_sum;
					for (long x = base; x < base + unit; x++)
					{
						int px = img[x] + delta;
						if (px > 255) px = 255;
						if (px < 0) px = 0;
						img[x] = px;
					}
				}
		}
		//t_repair = (UINT64)timeGetTime() - t_repair;

		//Mat mat_no_isp(m_Height, m_Width, CV_8UC1, fap50_img_no_isp);
		if (hw_scan_mode == HW_SCAN_MODE_ROLL)
		{
			//long x_start = 400, x_end = 1200, y_start = 0, y_end = 1000;
			//for (long y = y_start; y < y_end; y++)
			//{
			//	long i = x_start + y * 1600;
			//	memcpy(fap50_img_no_isp+i, img+i , 800);
			//}
			Rect roi(400, 0, 800, 1000);
			mat_img(roi).copyTo(mat_no_isp(roi));
		}
		//else if (hw_scan_mode == HW_SCAN_MODE_SIGNATURE)
		//{
		//	Rect roi(0, 0, 400, 1000); 
		//	mat_img(roi).copyTo(mat_no_isp(roi));
		//}
		//else if (hw_scan_mode == HW_SCAN_MODE_SIGNATURE_2)
		//{
		//	Rect roi(0, 0, 1600, 1000);
		//	mat_img(roi).copyTo(mat_no_isp(roi));
		//}
		else
			mat_img.copyTo(mat_no_isp);
	}

	//imshow("mat_no_isp", mat_no_isp);
	//waitKey(1);

//sw auto gain -----------------------------------------------------
	//enable_sw_auto_gain = false;
	t_sw_auto_gain = (UINT64)timeGetTime();
	once_for
	{
		if ( !enable_sw_auto_gain
		||
			(hw_scan_mode == HW_SCAN_MODE_SIGNATURE_2)
		) 
			break;

		int image_contrast = abs(black - white);
		//dbg(L"image_contrast=%d\n", image_contrast);

		if(1)//image_contrast<100)
		{
			//if(image_contrast>50)
			//	dbg(L"image_contrast=%d\n", image_contrast);

			void sw_auto_gain(BYTE * img, int white, int black, int hw_scan_mode);
			sw_auto_gain(img, white, black, hw_scan_mode);
		}
		else
		{
		//BYTE* img, int white, int black, int hw_scan_mode
			int gain = 0, bg = 0, bg1 = 0;

			//bg = black + image_contrast * 50 / 100; //40
			//bg = black_peak + 30;
			bg = black + 55;//60
			if (bg > 255)
				bg = 255;
			//bg = (bg > 185) ? 185 : bg;

			//bg1 = black + image_contrast * 70 / 100; //55
			//bg1 = black_peak + 45;
			//bg1 = black + 70;//75
			bg1 = bg + 15;
			if (bg1 > 255)
				bg1 = 255;
			bg1 = (bg1 > 185) ? 185 : bg1;
			//bg = (bg > bg1) ? bg1 : bg;
			
			int div = abs(bg - black);
			if (div == 0) div = 1;
			gain = 255 * 10 / div;
			if (abs(black - white) < 50)
				gain = 10;
			if (gain > 50) gain = 50;

			//dbg(L"black_peak=%d gain=%d bg:%d bg1=%d\n", black_peak, gain, bg, bg1);

			long x_start = 0, x_end = 1600, y_start = 0, y_end = 1000;
			if (hw_scan_mode == 1)//roll mode
				x_start = 400, x_end = 1200, y_start = 0, y_end = 1000;

			DWORD rand_num = 0;
			for (long x = x_start; x < x_end; x++)
				for (long y = y_start; y < y_end; y++)
				{
					long i = x + y * 1600;
					long px = img[i];
					rand_num += px;

					if (px >= bg1)//bg
					{
						img[i] = 255;
						continue;
					}
					else if (px > bg)
					{
						px = px + (255 - bg1) + (rand_num % ((bg1 - px) * 3));
						if (px > 255)
							px = 255;
						img[i] = (BYTE)px;
						continue;
					}

					px = ((bg - px) * 50L + rand_num % 50L) * gain / 500L;

					//integer domain
					if (px > 255)
						px = rand_num % (px / 30) + 10;
					else
						px = 255 - px;

					img[i] = (BYTE)px;
				}
		}
	}
	t_sw_auto_gain = (UINT64)timeGetTime() - t_sw_auto_gain;

	mat_isp = mat_img;
	//namedWindow("mat_isp", WINDOW_NORMAL);
	//imshow("mat_isp", mat_isp);

	{
		//img_status->is_finger_on = image_contrast > CONTRAST_OF_FINGER_ON;
	/*
		20250901 boxchu: If there is a broken wire, the finger detection method will 
		fail and the pressing area will be used instead.
	*/
		int th = 200;
		Mat mask = (mat_isp < th);
		int count = countNonZero(mask);
		img_status->is_finger_on = count > 512 * 8;
		//dbg("Pixels < %d count is %d \n", th, count);
	}

//de-bounce processing -----------------------------------------------
	if (show_mode == GUI_SHOW_MODE_CAPTURE 
	 || show_mode == GUI_SHOW_MODE_SIGNATURE )
	{
		func_ret = IMD_RLT_SUCCESS;
		goto func_end;
	}

	{
		static long old_line = 0, old_finger_width = 0;
		long finger_width = 0, line = 0, left, right;

		get_line(img_a, CONTRAST_OF_FINGER_ON, &line, &left, &right, &finger_width, hw_scan_mode);
		img_status->is_flat_init = TRUE;
		img_status->is_flat_done = FALSE;

		if (img_status->result != IMD_RLT_SUCCESS)
			debounce_finger_width_cnt = 0;

		//	dbg(L"delta finger position:%d \n", abs(old_line - line));
		//	if (image_contrast < CONTRAST_OF_FINGER_ON || abs(old_line - line) > 5)
			//	dbg(L"delta finger width:%d \n", abs(old_finger_width - finger_width));

		if (image_contrast < CONTRAST_OF_FINGER_ON || abs(old_finger_width - finger_width) > DELTA_FINGER_WIDTH)
		{
			debounce_finger_width_cnt = 0;
			old_finger_width = finger_width;
		}
		else
		{
		}

		if (debounce_finger_width_cnt < DEBOUNCE_FINGER_WIDTH_CNT)
			debounce_finger_width_cnt++;

		if (debounce_finger_width_cnt == DEBOUNCE_FINGER_WIDTH_CNT)
		{
			if (debounce_flat_cnt < DEBOUNCE_FLAT_CNT)
				debounce_flat_cnt++;

			if (debounce_flat_cnt == DEBOUNCE_FLAT_CNT)
			{
				img_status->is_flat_init = FALSE;
				img_status->is_flat_done = TRUE;
			}
		}
		else
			debounce_flat_cnt = 0;
	}

//moving avg -----------------------------------------------------
	//t_moving_avg = (UINT64)timeGetTime();
	{
		int valid_num = m_ring.unit[idx_a].valid + m_ring.unit[idx_b].valid + m_ring.unit[idx_c].valid;
		if (enable_moving_avg)
		{
			if (valid_num == 0)
			{
				img_status->img = img_a;
				if (hw_scan_mode == HW_SCAN_MODE_FLAT)
				{
					func_ret = IMD_RLT_NO_AVAILABLE_IMAGE;
					goto err_handle;
				}

				func_ret = IMD_RLT_SUCCESS;
				goto func_end;
			}

			img_status->img = fap50_img;//m_AvgImg;// m_ImageData;
			if (valid_num == 3)
			{
				if (hw_scan_mode == HW_SCAN_MODE_ROLL)
				{
					for (long x = 400; x < 1200; x++)
						for (long y = 0; y < 1000; y++)
						{
							//long i = x + y * 1600;
							//BYTE a = img_a[i], b = img_b[i];
							////if (a != b)//這方式會讓指紋一直保持住
							//	fap50_img[i] = (a + b + img_c[i]) / 3;

							long i = x + y * 1600;
							BYTE a = img_a[i], b = img_b[i];
							//if (a != b)//這方式會讓指紋一直保持住
							fap50_img[i] = (a + b) / 2;// +img_c[i]) / 3;

						}

					//img_status->img = img_a;
				}
				else
				{
					for (long i = 0; i < IMAGE_SIZE; i++)
					{
						//fap50_img[i] = (img_a[i] + img_b[i] + img_c[i]) / 3;
						//fap50_img[i] = (img_a[i]*2 + img_b[i]) / 3; 

						BYTE a = img_a[i], b = img_b[i];
						//if (a != b) //這方式會讓指紋一直保持住
						fap50_img[i] = (a + b + img_c[i]) / 3;
					}
				}
			}
			else if (valid_num == 2)
			{
				if (m_ring.unit[idx_a].valid)
				{
					img_a = m_ring.unit[idx_a].img;

					if (m_ring.unit[idx_b].valid)
						img_b = m_ring.unit[idx_b].img;
					else
						img_b = m_ring.unit[idx_c].img;
				}
				else
				{
					img_a = m_ring.unit[idx_b].img;
					img_b = m_ring.unit[idx_c].img;
				}

				if (hw_scan_mode == HW_SCAN_MODE_ROLL)
				{
					//for (long x = 400; x < 1200; x++)
					//	for (long y = 0; y < 1000; y++)
					//	{
					//		long i = x + y * 1600;
					//		fap50_img[idx] = (img_a[idx] * 2 + img_b[idx]) / 3;
					//	}

					for (long x = 400; x < 1200; x++)
						for (long y = 0; y < 1000; y++)
						{
							long i = x + y * 1600;
							BYTE a = img_a[i], b = img_b[i];
							//if (a != b)//這方式會讓指紋一直保持住
							fap50_img[i] = (a + b) / 2;// +img_c[i]) / 3;
						}


					//img_status->img = img_a;
				}
				else
				{
					for (long i = 0; i < IMAGE_SIZE; i++)
						fap50_img[i] = (img_a[i] * 2 + img_b[i]) / 3;
				}
			}
			else// if (div == 1)
			{
				if (m_ring.unit[idx_a].valid)
					img_a = m_ring.unit[idx_a].img;
				else if (m_ring.unit[idx_b].valid)
					img_a = m_ring.unit[idx_b].img;
				else
					img_a = m_ring.unit[idx_c].img;

				img_status->img = img_a;
			}
		}
	}

	if(img_status->img)
		mat_avg = Mat(m_Height, m_Width, CV_8UC1, img_status->img);

	//t_moving_avg = (UINT64)timeGetTime() - t_moving_avg; //~6ms
	//	dbg(L"t_moving_avg=%d valid_num=%d\n", t_moving_avg, valid_num);

//	dbg(L"black:%d white:%d image_contrast:%d gain:%d bg:%d bg1:%d \n", black, white, white-black, gain, bg, bg1);
	func_ret = IMD_RLT_SUCCESS;
	goto func_end;

err_handle:
	//func_ret = IMD_RLT_FAIL;

func_end:
	//dbg("func_end:\n");
//	img_status->is_capture_done = TRUE;

//	if (img_fast != NULL) delete[] img_fast;

//	UINT64 t_total = (UINT64)timeGetTime() - t_start;
//	dbg(L"t_total:%d t_mapping:%d t_repair:%d t_sw_auto_gain:%d t_moving_avg=%d \n\n", t_total, t_mapping, t_repair, t_sw_auto_gain, t_moving_avg);
		//t_total:60 t_mapping:14 t_repair:0 t_sw_auto_gain:9
	return func_ret;
}

//===================================================================
#define LEFT_SIDE	400
#define RIGHT_SIDE	1200
//void moving_avg(BYTE* avg, BYTE* src, BOOL is_init) //just for rolling
//{
//	if (is_init)
//	{
//		memcpy(avg, src, IMAGE_SIZE);
//		return;
//	}
//
//	//	TRACE("moving_avg() init ... done\n");
//	for (long y = 0; y < IMAGE_HEIGHT; y++)
//		for (long x = LEFT_SIDE; x < RIGHT_SIDE; x++)
//		{
//			BYTE avg_px = avg[y * IMAGE_WIDTH + x];
//			BYTE new_px = src[y * IMAGE_WIDTH + x];
//			avg[y * IMAGE_WIDTH + x] = (avg_px + new_px * 3L) / 4; //new_px
//		}
//}

//int sn = 0;

BOOL IMDFTTestMethodClass::merge_img(Mat new_img, Mat base_img, MOVE_DIRECTION direction, int finger_width, Mat no_isp )
{
	//static BOOL debug_image_window = 1;
	//if (debug_image_window == 1)
	//{
	//	debug_image_window = -1;
	//	namedWindow("Image Display", cv::WINDOW_NORMAL);
	//	HWND hwnd = FindWindow(0, L"Image Display");
	//	if (hwnd != 0)
	//		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, new_img.cols / 3, new_img.rows / 3, 0);
	//}

	UINT64 start_time = (UINT64)timeGetTime();

	// base ----------------------------------------------------------------------
	vector<vector<Point>> contours, contours_deal;
	Mat blur, thresh_img;
	//UINT64 t_blur = (UINT64)timeGetTime();
	GaussianBlur(base_img, blur, Size(13, 13), 0, 0, BORDER_DEFAULT);
	threshold(blur, thresh_img, 254, 255, THRESH_BINARY);
	//	dbg(L"t_blur=%d\n", timeGetTime()- t_blur);//1~2ms

	// 将上边界的像素设置为255
	thresh_img.row(0).setTo(Scalar(255));
	thresh_img.row(thresh_img.rows - 1).setTo(Scalar(255));
	thresh_img.col(0).setTo(Scalar(255));
	thresh_img.col(thresh_img.cols - 1).setTo(Scalar(255));

	findContours(thresh_img, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
	//finger_width = 17;// 4;
	finger_width /= 5;//3, 4, 5, 6;

	UINT64 t_area = (UINT64)timeGetTime();
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
		return FALSE;
	contours_deal.push_back(contours[maxAreaContourIndex]);
	//	dbg(L"t_area=%d\n", timeGetTime()- t_area);//0ms
	for (auto& contour : contours_deal) //shift contour
		for (auto& point : contour)
			point.x -= finger_width * direction;

	Mat base_mask = Mat::zeros(new_img.size(), new_img.type());
	drawContours(base_mask, contours_deal, 0, Scalar(255), FILLED);
	//imshow("Image Display", thresh_img);// 显示图片

	Mat base_mask_inv;
	bitwise_not(base_mask, base_mask_inv);

	// new ----------------------------------------------------------------------
	Mat new_mask_inv, new_mask;
	//GaussianBlur(new_img, blur, cv::Size(11, 11), 0, 0, cv::BORDER_DEFAULT);
	//threshold(blur, new_mask_inv, 254, 255, cv::THRESH_BINARY);
	threshold(no_isp, new_mask_inv, image_white - 35, 255, THRESH_BINARY);

	Mat union_new_base_mask, fg_mask, fg_mask_inv, fg, bg, final_img;
	UINT64 t_or_and_not = (UINT64)timeGetTime();
	bitwise_not(new_mask_inv, new_mask);
	bitwise_or(base_mask, new_mask, union_new_base_mask);
	bitwise_and(union_new_base_mask, base_mask_inv, fg_mask);
	bitwise_not(fg_mask, fg_mask_inv);
	bitwise_and(new_img, fg_mask, fg);
	bitwise_and(base_img, fg_mask_inv, bg);
	bitwise_or(fg, bg, final_img);
	//	dbg(L"t_or_and_not=%d\n", timeGetTime() -t_or_and_not); //1~2ms

	vector<Point> contour_x;
	for (auto& pt : contours_deal[0])
	{
		BOOL is_find = FALSE;
		for (auto& exist : contour_x)
		{
			if (direction == MOVE_DIRECTION_RIGHT)
			{
				if (exist.y == pt.y && exist.x < pt.x)
				{
					exist.x = pt.x;
					is_find = TRUE;
					break;
				}
			}
			else
			{
				if (exist.y == pt.y && exist.x > pt.x)
				{
					exist.x = pt.x;
					is_find = TRUE;
					break;
				}
			}
		}

		if (is_find)
			continue;

		contour_x.push_back(pt);
	}

	long x_start = 0, x_width = new_img.cols;
	for (auto& pt : contour_x)
	{
		int y = pt.y;
		BYTE* final_ptr = final_img.ptr<BYTE>(y),
			* new_ptr = new_img.ptr<BYTE>(y),
			* base_ptr = base_img.ptr<BYTE>(y);

		for (int i=-1; i <=1; i++)
		{
			long x = pt.x + i;
			if (x >= x_start && x < x_width)
				final_ptr[x] = min(new_ptr[x], base_ptr[x]);
		}
	}

	final_img.copyTo(base_img);
	//dbg(dbg_x + L"\n");

	//if (debug_image_window == -1)
	//	imshow("Image Display", fg_mask_inv);// 显示图片

	UINT64 elapsed_time = (UINT64)timeGetTime() - start_time;
	//	dbg(L"merge_img elapsed_time=%d \n",
	//		elapsed_time);

	return TRUE;
}

IMD_RESULT IMDFTTestMethodClass::rolling_finger_fap50(BYTE* pImg, ImageStatus* img_status, bool is_first)
{
#define ROLLING_DONE_RATIO	40

	typedef struct {
		BOOL is_poor_contact_iron;
		BOOL is_done;
	}RollStatus;
	typedef struct {
		RollStatus this_time;
		RollStatus this_roll;
	}AllStatus;
	static AllStatus roll_status{};
	if (is_first)
		roll_status = AllStatus{};

	static Mat
		roll_img(1000, 1600, CV_8UC1),
		roll_img_done(1000, 1600, CV_8UC1);

	int contrast = abs(image_black - image_white);
	static int finger_width_old = 0, finger_width_max = 0,
		old_line = -1, left = -1, right = -1;
	static MOVE_DIRECTION direction = MOVE_DIRECTION_LEFT;
	static UINT64 t_roll_done = (UINT64)timeGetTime();// , t_change_direction = 0;
	Rect roi(ROLL_X, ROLL_Y, ROLL_WIDTH, ROLL_HEIGHT); // 定义要复制到的ROI

	if (img_status == NULL || pImg == NULL)
		return IMD_RLT_PARAM_WRONG;

	//check finger on/off ------------------------------------------------------
	img_status->img = roll_img.data;
	img_status->is_roll_init = false;
	long line = 0, finger_width = 0, left_x, right_x;

	if (img_status->is_finger_on == false)
	{
		left = -1;
		right = -1;
		line = -1;
		old_line = -1;
		finger_width_old = 0;
		debounce_roll_cnt = 0;
		finger_width_max = 0;

		if(roll_status.this_roll.is_done == false)
		{
			img_status->is_roll_init = true;
			img_status->is_roll_done = false;
			roll_img.setTo(Scalar(255));
		}
		else
		{
			img_status->is_roll_done = roll_status.this_roll.is_done;
			roll_img_done(roi).copyTo(roll_img(roi));
			roll_img_done(roi).setTo(Scalar(255));
			if (roll_status.this_roll.is_poor_contact_iron)
			{
				img_status->result = IMD_RLT_POOR_CONTACT_IRON;
				is_poor_contact = true;
			}
		}

		roll_status = { false };

		return IMD_RLT_SUCCESS;
	}

	//img_status init ---------------------------------------------------
	//moving_avg(fap50_img_avg, pImg, (debounce_roll_cnt == 0));
	Mat src_img(1000, 1600, CV_8UC1, pImg);

	//UINT64 t_get_line = (UINT64)timeGetTime();
	get_line(src_img.data, CONTRAST_OF_FINGER_ON, &line, &left_x, &right_x, &finger_width, HW_SCAN_MODE_ROLL);
	finger_width_old = (finger_width * 1 + finger_width_old) / 2;

	if (debounce_roll_cnt < DEBOUNCE_ROLL_CNT)
	{
		img_status->is_roll_init = TRUE;
		roll_img.setTo(Scalar(255));

		if (line < LEFT_SIDE || RIGHT_SIDE <= line)
			goto func_end;

		if (debounce_finger_width_cnt == DEBOUNCE_FINGER_WIDTH_CNT)
		{
			if (debounce_roll_cnt < DEBOUNCE_ROLL_CNT)
				debounce_roll_cnt++;
		}
		else
		{
			debounce_roll_cnt = 0;
			goto func_end;
		}

		if (debounce_roll_cnt < DEBOUNCE_ROLL_CNT)
			goto func_end;

		//debounce end ---
		old_line = line;
		left = line;
		right = line;
		img_status->is_roll_init = FALSE;
		src_img(roi).copyTo(roll_img(roi));
		goto func_end;
	}

	finger_width_max = MAX(finger_width_max, finger_width);

#define PROPORTION 4
	line = (line + old_line * (PROPORTION - 1)) / PROPORTION;

	//img_status->is_roll_too_fast = FALSE;
	//if (abs(line - old_line) > 15)
	//{
	//	roll_too_fast = TRUE;
	//	img_status->is_roll_too_fast = TRUE;
	//}

	//merge image ---------------------------------------------------
	{
		BOOL is_finger_width_valid = (finger_width > finger_width_max * 75 / 100);
		if (contrast < 80 && is_finger_width_valid)
		{
			if (img_status->result == IMD_RLT_SUCCESS)
				img_status->result = IMD_RLT_POOR_CONTACT_IRON;
			roll_status.this_time.is_poor_contact_iron = TRUE;
		}

		Rect finger_roi = roi;
		if (direction == 1)//right dirction
		{
			if (line > right && is_finger_width_valid && contrast > 100)
			{
				right = line;
				merge_img(src_img(finger_roi), roll_img(finger_roi), direction, finger_width, mat_no_isp(finger_roi));
			}
			else if (line < right)//direction change
			{
				left = right;
				direction = MOVE_DIRECTION_LEFT;
				mat_isp(roi).copyTo(roll_img(roi));
				finger_width_max = finger_width;
				roll_status.this_time.is_done = FALSE;
				roll_status.this_time.is_poor_contact_iron = FALSE;
			}
		}
		else//left dirction
		{
			if (line < left && is_finger_width_valid && contrast>100)
			{
				left = line;
				merge_img(src_img(finger_roi), roll_img(finger_roi), direction, finger_width, mat_no_isp(finger_roi));
			}
			else if (line > left)//direction change
			{
				right = left;
				direction = MOVE_DIRECTION_RIGHT;
				mat_isp(roi).copyTo(roll_img(roi));
				finger_width_max = finger_width;
				roll_status.this_time.is_done = FALSE;
				roll_status.this_time.is_poor_contact_iron = FALSE;
			}
		}
	}

	{
		//CString str;
		//check rolling done ------------------------------------------------------
		if (1)//(UINT64)timeGetTime() > t_roll_done)
		{
			if (roll_status.this_time.is_done == FALSE)
			{
				long rolling_width = abs(left - right);
				roll_status.this_time.is_done = (rolling_width * 100 / finger_width_max) > ROLLING_DONE_RATIO;
				roll_status.this_time.is_done &= (finger_width < finger_width_max * 96 / 100);
				roll_status.this_time.is_done &= rolling_width > 125;
				//str.AppendFormat(L"roll_width=%d, finger_w_max=%d, ROLLING_DONE_RATIO=%d\n",
				//	rolling_width, finger_width_max, ROLLING_DONE_RATIO);
			}

			if (roll_status.this_time.is_done)
			{
				t_roll_done = (UINT64)timeGetTime() + 200;
				if (finger_width > finger_width_max * 70 / 100)//防止最後手指抬起時出現較白的圖塊
					roll_img(roi).copyTo(roll_img_done(roi));

				roll_status.this_roll.is_poor_contact_iron = roll_status.this_time.is_poor_contact_iron;
				roll_status.this_roll.is_done = TRUE;
			}
		}
		//else
		//	str.AppendFormat(L"timeGetTime() < t_roll_done\n");

	}


func_end:
	old_line = line;
	img_status->is_roll_done = roll_status.this_time.is_done;

	return IMD_RLT_SUCCESS;
}

void IMDFTTestMethodClass::sign_2_merge_img(Mat new_img, Mat base_img)// , Point& last_point)
{
	static Point last_point;
	UINT64 start_time = timeGetTime();

	// base ----------------------------------------------------------------------
	vector<vector<Point>> contours, contours_deal;
	Mat blur, thresh_img;
	//auto t_blur = timeGetTime();
	GaussianBlur(new_img, blur, Size(3, 3), 0, 0, BORDER_DEFAULT);
	threshold(blur, thresh_img, 220, 255, THRESH_BINARY);

	// 将 ROI 以外的区域设置为白色
	thresh_img(cv::Rect(0, 0, thresh_img.cols, 30)).setTo(Scalar(255));       // 顶部
	thresh_img(cv::Rect(0, 970, thresh_img.cols, thresh_img.rows - 970)).setTo(Scalar(255)); // 底部
	
	//dbg(L"pen_down_cnt=%d\n", pen_down_cnt);
	
	//imshow("thresh_img", thresh_img);
	//imshow("new_img", new_img);
	//imshow("base_img", base_img);
	//imshow("mat_no_isp", mat_no_isp);
	//bitwise_and(new_img, base_img, base_img);
	int pen_down_cnt = thresh_img.rows * thresh_img.cols - countNonZero(thresh_img);
	static int dbg_pen_down = 0;
	if(pen_down_cnt == 0)
	{
		last_point.x = 0;
		last_point.y = 0;

		if(dbg_pen_down)
		{
			//dbg(L"pen leave, dbg_pen_down=%d\n", dbg_pen_down);
			dbg_pen_down = 0;
		}
	}
	
	findContours(thresh_img, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
	if (contours.size() <= 1)
	{
		//dbg(L"no point. contours=%d \n", contours.size());
		return;
	}

	dbg_pen_down++;
	
	//auto t_area = timeGetTime();
	double maxArea = 0.0;
	int maxAreaContourIndex = -1;
	deque<int> log_size, log_area;
	for (size_t i = 0; i < contours.size(); i++)
	{
		if (contours[i].size() < 2 || contours[i][0].x == 0 || contours[i][0].y == 0)
		{
			//dbg(L"size() < 2\n");
			continue;
		}

		double area = contourArea(contours[i]);
		log_area.push_back((int)area);
		log_size.push_back(contours[i].size());

		if (area < maxArea)// || area <= 13)
		{
			//dbg(L"area < maxArea\n");
			continue;
		}

		maxArea = area;
		maxAreaContourIndex = i;
	}

	if (maxAreaContourIndex == -1) // 将最大的轮廓添加到 contours_deal
	{
		//CString str_log = L"(Area,Size)= ";
		//for (int i = 0; i < log_area.size(); i++)
		//	str_log.AppendFormat(L"(%d,%d) ", log_area[i], log_size[i]);
		//dbg(L"no find. contours=%d %s \n", contours.size(), str_log);

		return;
	}

	//for(int i=0; i< contours.size(); i++)
	//	drawContours(thresh_img, contours, i, CV_RGB(255, 0, 0), 2);

	//drawContours(thresh_img, contours, maxAreaContourIndex, CV_RGB(255, 0, 0), 1);

	vector<Point> contour = contours[maxAreaContourIndex];
	Point start= contour[0], end = contour[contour.size()-1];
	//Moments mu = moments(contours[maxAreaContourIndex], false);
	Point new_point = (start+ end)/2;

	if (new_point.y > 875)
	{
		bool is_first_pen_on = false;
		if (last_point.x == 0 && last_point.y == 0)
			is_first_pen_on = true;

		if(is_first_pen_on == false)
			return;

		//last_point = new_point;

		SystemProperty& p = m_property;
		if (new_point.x < (p.signature_clear_roi[0] + p.signature_clear_roi[2])
			&& new_point.x > p.signature_clear_roi[0])
			is_sign_clear = true;
		else if(new_point.x < (p.signature_confirm_roi[0] + p.signature_confirm_roi[2])
			&& new_point.x > p.signature_confirm_roi[0])
			is_sign_done = true;

		//goto func_end;
		return;
	}

	//ignore point.
	static vector<int> ignore_point{
		148,149,
		248,249,
		250,251,252,253,254,255,256,257,258,259,
		737,738,739,
		740,741,742,743,744,745,746,747,748,749,
		987,988,989
	};
	if (find(ignore_point.begin(), ignore_point.end(), new_point.y) != ignore_point.end()
		&& maxArea <= 25)
	{
		//dbg(L"point(%d, %d) y is wrong. \n", new_point.x, new_point.y);
		return;
	}

	//this way not good.
	//if (abs(new_point.x - last_point.x) < 2 || abs(new_point.y - last_point.y) < 2)
	//	return;

	//Mat draw = Mat(new_img.size(), new_img.type(), Scalar(255));
	int line_thickness = 5;

	//circle(draw, new_point, 4, Scalar(0), thickness, -1, 0);
	if (last_point.x != 0 && last_point.y != 0)
		line(base_img, new_point, last_point, Scalar(0), line_thickness);

func_end:
	last_point = new_point;
	//dbg(L"find point (%d, %d) area=%d\n", new_point.x, new_point.y, (int)maxArea);

	//bitwise_and(new_img, base_img, base_img);
	//imshow("base_img", base_img);
}

void IMDFTTestMethodClass::sign_2_merge_by_LP_5(Mat base_img)// , Point& last_point)
{
	static Point last_point;
	UINT64 start_time = timeGetTime();

	int x, y, tilt_x, tilt_y, press;
	unsigned int timestamp; //unsigned long timestamp;
	get_pen_data(x, y, tilt_x, tilt_y, press, timestamp);

	int pen_down_cnt = press;
	static int dbg_pen_down = 0;
	if (pen_down_cnt == 0)
	{
		last_point.x = 0;
		last_point.y = 0;
		return;
	}

	Point new_point = Point(x, y);
	//dbg("new point: %d, %d\n", x, y);

	if (new_point.y > 875)
	{
		bool is_first_pen_on = false;
		if (last_point.x == 0 && last_point.y == 0)
			is_first_pen_on = true;

		if (is_first_pen_on == false)
			return;

		SystemProperty& p = m_property;
		if (new_point.x < (p.signature_clear_roi[0] + p.signature_clear_roi[2])
			&& new_point.x > p.signature_clear_roi[0])
			is_sign_clear = true;
		else if (new_point.x < (p.signature_confirm_roi[0] + p.signature_confirm_roi[2])
			&& new_point.x > p.signature_confirm_roi[0])
			is_sign_done = true;

		return;
	}

	int line_thickness = press;
	if (last_point.x != 0 && last_point.y != 0)
		line(base_img, new_point, last_point, Scalar(0), line_thickness);

func_end:
	last_point = new_point;
	//dbg(L"find point (%d, %d) area=%d\n", new_point.x, new_point.y, (int)maxArea);

	//bitwise_and(new_img, base_img, base_img);
	//imshow("base_img", base_img);
}

IMD_RESULT IMDFTTestMethodClass::signature_2_fap50 (BYTE* pImg, ImageStatus* img_status)
{
	typedef struct {
		bool is_poor_contact_iron;
		bool is_done;
	}SignStatus;
	typedef struct {
		SignStatus this_time;
		SignStatus this_sign;
	}AllStatus;
	static AllStatus sign_status = { 0 };
	//static Point last_point;

	int contrast = abs(image_black - image_white);
	static UINT64 t_sign_done = (UINT64)timeGetTime();// , t_change_direction = 0;
	Rect roi(0, 0, 1600, 1000);

	if (img_status == NULL || pImg == NULL)
		return IMD_RLT_PARAM_WRONG;

	img_status->img = sign_img.data;

	Mat src_img(1000, 1600, CV_8UC1, pImg);

	sign_2_merge_img(src_img, sign_img);// , last_point);
	sign_img_done = sign_img;

func_end:
	return IMD_RLT_SUCCESS;
}

IMD_RESULT IMDFTTestMethodClass::signature_by_LP_5(BYTE* pImg, ImageStatus* img_status)
{
	if (img_status == NULL)
		return IMD_RLT_PARAM_WRONG;

	img_status->img = sign_img.data;

	sign_2_merge_by_LP_5(sign_img);// , last_point);
	sign_img_done = sign_img;

func_end:
	return IMD_RLT_SUCCESS;
}

void sw_auto_gain(BYTE* img, int white, int black, int hw_scan_mode)
{
	int gain = 0, bg = 0, bg1 = 0;
	int image_contrast = abs(black - white);

	bg = black + image_contrast * 50 / 100; //40
	bg1 = black + image_contrast * 70 / 100; //55
	//TRACE("bg:%d\n", bg);
	bg1 = (bg1 < 185) ? bg1 : 185;

	int div = abs(bg - black);
	if (div == 0) div = 1;
	gain = 255 * 10 / div;
	if (abs(black - white) < 50)
		gain = 10;
	if (gain > 50) gain = 50;

	long x_start = 0, x_end = 1600, y_start = 0, y_end = 1000;
	if (hw_scan_mode == 1)//roll mode
		x_start = 400, x_end = 1200, y_start = 0, y_end = 1000;

	DWORD rand_num = 0;
	for (long x = x_start; x < x_end; x++)
		for (long y = y_start; y < y_end; y++)
		{
			long i = x + y * 1600;
			long px = img[i];
			rand_num += px;

			if (px >= bg1)//bg
			{
				img[i] = 255;
				continue;
			}
			else if (px > bg)
			{
				px = px + (255 - bg1) + (rand_num % ((bg1 - px) * 3));
				if (px > 255)
					px = 255;
				img[i] = (BYTE)px;
				continue;
			}

			px = ((bg - px) * 50L + rand_num % 50L) * gain / 500L;

			//integer domain
			if (px > 255)
				px = rand_num % (px / 30) + 10;
			else
				px = 255 - px;

			img[i] = (BYTE)px;
		}

}