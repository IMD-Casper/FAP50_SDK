#pragma once
#include "InterfaceImdFap50Method.h"

#define MAXIMUM_PIXELERR_NUM		500
#define MAXIMUM_COLUMNERR_NUM		MAXIMUM_PIXELERR_NUM
#define MAXIMUM_ROWERR_NUM			MAXIMUM_PIXELERR_NUM
#define MAXIMUM_BLACKWHITEERR_NUM	MAXIMUM_PIXELERR_NUM
#define MSGSTRINGSIZE				512

enum E_PANEL_VERSION {
	RULE_1 = 0,		//rule 1 => WS1
	RULE_1A = 1,	//rule 1A => WS2
	RULE_1_1 = 2,	//WS1 ig1, ig2 bonding mirror
	RULE_1A_1 = 3,	//WS2 ig1, ig2 bonding mirror
};

enum E_HW_SCAN_MODE {
	HW_SCAN_MODE_FLAT,
	HW_SCAN_MODE_ROLL,
	HW_SCAN_MODE_FAST,
	HW_SCAN_MODE_RESERVED,
	HW_SCAN_MODE_SIGNATURE_2,
};

typedef union {
	struct {
		BYTE panel_ver : 4;
		BYTE ig_ver : 3;
		BYTE fix_reg1B : 1;
	};
	BYTE data;
} ModuleInfo;

typedef union {
	struct {
		DWORD is_dcmi_timeout : 1;
		DWORD is_usb_too_slow : 1;
		DWORD is_ig_chipid_err : 1;
		DWORD is_ig_crc_err : 1;
	};
	DWORD all;
} HwStatus;

typedef struct {
	WORD		SensorModel;
	BYTE		FirmwareMajor;
	BYTE		FirmwareMinor;
	BYTE		VersionType; //'G', 'B'
	ModuleInfo	module_info;
	BYTE		SerialNo[4];
} READER_INFORMATION;

typedef struct {
	WORD	wChipID;
	DWORD	nWaitTime;
	BOOL	fQETEnable;
	LPBYTE	img, raw, fast;
	LPBYTE	lpStatusBuffer;
	INT		nWOIStartX, nWOIStartY, nPixelCntX, nPixelCntY, nShiftPixelX;
	BOOL	fDetect_XShift;
	HANDLE	hEvent;
	INT     mTPN_Verify_Size;
	DWORD   raw_size;
	DWORD	dwStatusSize;

	int		hw_scan_mode;
	BOOL	enable_save_img, enable_save_raw;

	ImageStatus img_status;

} WAIT_SCAN_PARAMETER, * LPWAIT_SCAN_PARAMETER;

//Testing Summary
typedef struct {
	BYTE bOffSet;
	DWORD bPeakHistPxl;
	DWORD bPeakHist;
	BYTE bCaTimes;
	BYTE bOffSetBeforeShift;
	BYTE bOffSetShiftValue;
} stDCOFFSET_CALI_RESULT, *stDCOFFSET_CALI_RESULT_pt;

typedef struct {
	BYTE bGain;
	BYTE bOffSet;
	DWORD bPeakHistPxl;
	DWORD bPeakHist;
	BYTE bCaTimes;
}stGain_Offset_Check, *stGain_Offset_Check_pt;

//Testing Summary
typedef struct {
	BYTE sChipSN[8];
	BYTE sResult;//0:OK, Other:Bin number
	BYTE sCaliData[10];
	BYTE sCaliDataExt[8];
	BYTE sCalibData[17];
	stGain_Offset_Check OffsetCheck;
	BYTE bAFDTrim;
	WORD bContrast;
	FLOAT fTestTime;
	BYTE sEncryptKey[10];
	BYTE sModuleID[2];
} stTEST_SUMMARY, *stTEST_SUMMARY_pt;

//Golden Sample Sync
typedef struct {
	BYTE bDCOffset_RegVari[3];//gain 0xD7,E6,F5
	BYTE bDCOffset_CTimeVari;
	BYTE bAFDTrim_RegVari;
	//	BYTE bContrast_Vari;
} stGSAMPLE_SYNC, *stGSAMPLE_SYNC_pt;

//DC Offset tuning phase definition
#define COARSE_TUNNING_PHASE	1
#define FINE_TUNNING_PHASE		2

#define TRIM_DEF_MAX	0x1F
#define TRIM_DEF_MIN	0x00

//Type of message log function
using pfnMessageLog = void(*)(const char*, ...);

#define PE_1SHOT			0
#define PE_2SHOT			1
#define PE_SHOT_NUMBER		PE_2SHOT+1
#define PE_1AND2			2
#define MAXIMUM_PIXELERR_ARRAY_NUM			3

//Low capacity is_roll_mode image shot number
enum {
	HIGH_CAP_MODE = 0,
	LOW_CAP_MODE,
	CAPMODE_ARRAY_NUM,
};

// Keep device information
//typedef struct {
//	CString Product_PartNo;
//	CString Product_PartNo_STR; //PRODUCT_PARTNO_STR
//	int Product_PartName; // PRODUCT_NAME
//	int Product_Chip_ID; // PRODUCT_CHIP_ID
//	int Product_Sensor_Width; // SENSOR_WIDTH_X
//	int Product_Sensor_Height; // SENSOR_HIGHT_Y
//	int Product_Sensor_Raw_Width;
//	int Product_Sensor_Raw_Height;
//	int Product_Sensor_RawData_Size;
//	CString CoatingColor;
//	CString CoatingThickness;
//	CString VDDVoltage;
//	int UseT8Process;
//	int UseCreatingCoating;
//	int UseTDACTrimData; // USE_TDAC_TRIM_DATA
//	int TDACTrimDataBit; // TDAC_TRIM_DATA_BIT
//	int EnableOTPCalibrationExt; // ENABLE_OTP_CALIBRATION_EXT
//	int EncryptKey;
//	DWORD TestItems; // one bits to denote one item
//	bool bIsRXofsCalib;
//	BOOL bImgMCUSort;
//	int	panel_type;
//} ProductInfo, *pProductInfo;

typedef struct {
	BYTE CaliVersion;
	BYTE CaliChkSum;
	BYTE CaliType; // CALITYPE_3_POINT or CALITYPE_1_POINT
	BYTE bCALI_REG[4]; // bDEF_REG01, CALI_REG01_G1, CALI_REG01_G2, CALI_REG01_G3
	BYTE bDEF_REG[32]; //  REG00, 01, 02, 03, 08, 09, 0A, 0B, 0C, 0D, 0E, 0F, 13, 1D 
	BYTE BadPixel_Gain;
	BYTE BadPixel_DCOffset;
	BYTE Contrast_Gain;
	BYTE Contrast_DCOffset;
	BYTE bCALI_Gain_Offset[4];
	BYTE bCALI_Offset_Shift[4];
	float bCALI_Gain_Step[4];
	float bDEF_Step;
}CaliDataSet, *pCaliDataSet;

//typedef struct {
//	bool m_bFTMode; // change status
//	bool m_bWriteChipSNbyTool ; // write SN or not
//	bool m_bSIPOnlyTest ; // this flag doesn't use so far.
//	bool m_bSIPFPCTest ; // for FT2Mode use
//	bool m_CfgTestItem_Contrast; // do contrast_check or not
//	bool m_CfgTestItem_OTPCaliDataWrite; // do OTP_CaliData or not
//	bool m_CfgTestItem_OTPEncryptionWrite ;  // this flag doesn't use so far.
//	bool m_CfgDebugItem_SaveErrBinImage; // save errbin image only
//	bool m_bWriteOTPDefaultRegOnDCOffset ;  // only for test write OTP
//	bool m_CfgTestItem_GoldenSampleCheck; // Golden Sample Sync Process or write SN
//	bool m_CfgDebugItem_SaveAllTestImage; // save all image
//	bool m_CfgDebugItem_SaveDebugImage; // save image after pixel error check for debugging
//	int ChangeButtonStatus; // for change different button status
//	int ReleaseProgramType; // for 1: EQC, 2: FT2, 8: MP 
//	bool m_CfgTestItem_ADC;
//	bool m_CfgTestItem_DCOffsetCali;
//	bool m_CfgTestItem_OTPApplyCheck;
//	CString m_CfgProduct_PartNo_STR; //  Product_PartNo_STR
//	int m_FT_TestType; //FT_TEST_TYPE
//}ConfigParam, *pConfigParam;

typedef struct {
	BOOL bFlag;
} Bin4Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin5Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin6Err;

typedef struct {
	int	x;
	int y;
	BYTE Value;
	BYTE Value2;
	BYTE flag_White255;
	BYTE flag_Black0;
} BadPixel, *BadPixel_ptr;

typedef struct {
	int	x;
	int y;
} RCIgnoreBadPixel, *RCIgnoreBadPixel_ptr;

typedef struct {
	UINT  uiRowTNum; //Total number of row line
	PBYTE piRowList; //Array of sensor height
	UINT  uiColTNum; //Total number of column line
	PBYTE piColList;
} IgnoreRowColTable, *IgnoreRowColTable_ptr;

typedef struct {
	int	Column;
	BYTE Mean;
	BYTE Std;
	BYTE MeanPrev;
	BYTE StdPrev;
	BYTE MeanNext;
	BYTE StdNext;
} BadColumn, *BadColumn_ptr;

typedef struct {
	int	Row;
	BYTE Mean;
	BYTE Std;
	BYTE MeanPrev;
	BYTE StdPrev;
	BYTE MeanNext;
	BYTE StdNext;
} BadRow, *BadRow_ptr;

typedef struct {
	WORD				TotalPENumber;
	BadPixel			stBPArray[MAXIMUM_PIXELERR_NUM];
	WORD				TotalCENumber;
	BadColumn			stBCArray[MAXIMUM_COLUMNERR_NUM];
	WORD				TotalRENumber;
	BadRow				stBRArray[MAXIMUM_ROWERR_NUM];
	WORD				BCOutTargetNumber;
	BadColumn			stBCOutTargetArray[MAXIMUM_COLUMNERR_NUM];
	WORD				TotalPEBlackNumber;//black number
	BadPixel			stBPBArray[MAXIMUM_BLACKWHITEERR_NUM];
	WORD				TotalPEWhiteNumber;//white number
	BadPixel			stBPWArray[MAXIMUM_BLACKWHITEERR_NUM];
	//	RCIgnoreBadPixel	stRCIgnoreBPAry[TOTAL_RCORNER_IGNORE_BADPIXEL];
} BadPixelArray, *BadPixelArray_ptr;

typedef struct {
	int x;
	int y;
} CPoint1;

typedef struct {
	int	x;
	BYTE Value;
	BYTE IsCoverCol;
} BIN2ColNum, *BIN2ColNum_ptr;

typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin2Err;

typedef struct {
	BOOL bFlag;
	BYTE ErrorNum;
} Bin3Err;

//Bin7 PE within 12x12 area
typedef struct {
	CPoint1 LeftCorner;
	CPoint1 RightCorner;
	int TotalNumber;
	BOOL bFlag; //Set 1 when test error.
} Bin7Err;

//Bin8 PE total
typedef struct {
	int TotalNumber;
	BOOL bFlag;
} Bin8Err;

//Bin9 PE in any single row
typedef struct {
	int y;
	int TotalNumber;
	int cy;
	int TotalRNumber;
	BOOL bFlag;
} Bin9Err;

//Bin10 PE in any single column
typedef struct {
	int x;
	int TotalNumber;
	int cx;
	int TotalCNumber;
	BOOL bFlag;
	int OutTargetTotalCNNumber;
	int OutTargetCx;
	BOOL bOutTargetFlag;
} Bin10Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin11Err;

typedef struct {
	BOOL bFlag;
} Bin12Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;	// amelie added 1013
} Bin13Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin14Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin15Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin16Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin17Err;
///////////////////////////////////////////////////////////////////////////////
//SY09 Test

typedef struct {
	BOOL bFlag;
	DWORD dwError;	// boxchu added 20160505
} Bin20Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;	// boxchu added 20160505
} Bin21Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;	// boxchu added 20160505
} Bin22Err;

typedef struct {
	BOOL	bFlag;
	DWORD	dwError;	// boxchu added 20160505
} Bin23Err;

typedef struct {
	DWORD	dwError;	// boxchu added 20160713
	CPoint1 LeftCorner;
	CPoint1 RightCorner;
	int TotalNumber;
	BOOL bFlag; //Set 1 when test error.
} Bin24Err;

typedef struct {
	int TotalNumber;
	INT BlankMean;
	INT BlankStd;
	INT StampMean;
	INT StampStd;
	BOOL bFlag; //Set 1 when test error.
} Bin26Err;

typedef struct {
	CPoint1 LeftCorner;
	CPoint1 RightCorner;
	int TotalBPENumber;
	int TotalWPENumber;
	BOOL bFlag; //Set 1 when test error.
} Bin27Err;

typedef struct {
	BYTE   RowMeanBG[10];
	BYTE   RowMean[10];
	BYTE   ColMeanBG[10];
	BYTE   ColMean[10];
	BYTE   RowStdBG[10];
	BYTE   RowStd[10];
	BYTE   ColStdBG[10];
	BYTE   ColStd[10];
	BYTE   IsFirst;
	BOOL   bFlag; //Set 1 when test error.
} Bin28Err;

typedef struct {
	BYTE	bRXChannelErrNum;		//Store the error RX channel number.
	FLOAT	fRXChannelErrMean;		//Store the error RX channel mean.
	FLOAT	fRXChannelErrStd;		//Store the error RX channel std.
	FLOAT	fRXChannelErrFullStd;	//Store the error RX channel full data std to Bin30.
} RXChErrData;

typedef struct {
	BYTE		RX_ChannelMeanAvgBG[2][60];
	RXChErrData stRXChErrData[60];
	INT			iRXChannelErrTotalNum;		//Total RX channel error numbers.
	BOOL		bFlag; //Set 1 when test error.
} Bin29Err;

typedef struct {
	RXChErrData stRXChErrData[60];
	INT			iRXChannelErrTotalNum;		//Total RX channel error numbers.
	BOOL		bFlag; //Set 1 when test error.
} Bin30Err;

typedef struct {
	BOOL	bFlag;
	DWORD	dwError;
} Bin31Err;

typedef struct {
	BOOL	bFlag;
	DWORD	dwError;
} Bin32Err;

typedef struct {
	BOOL	bFlag;
	DWORD	dwError;
} Bin33Err;

typedef struct {
	BOOL	bFlag;
	DWORD	dwError;
} Bin34Err;

//Bin99 to USB transfer error
typedef struct {
	BOOL	bFlag;
	DWORD	dwError;
} Bin99Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin41Err;

typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin42Err;
//20220518 AlbertKe++, temp for KeyCore project, remove after mass production.
typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin43Err;
//20220518 AlbertKe++, temp for KeyCore project, remove after mass production.
typedef struct {
	BOOL bFlag;
	DWORD dwError;
} Bin44Err;

///////////////////////////////////////////////////////////////////////////////
//Testing Summary
typedef struct {
	BYTE bOffSet;
	DWORD bPeakHistPxl;
	DWORD bPeakHist;
	BYTE bCaTimes;
} DCOffestCaliResult, *DCOffestCaliResult_pt;

//Enable windowing function during dc offset calibration 
typedef struct {
	BOOL	WinFunEnable; //ref: WOI_ENABLE
	BOOL    SWWinFunEnable; //Glass series sensor SF20x use software WOI function
	INT		img_width, img_height;
	INT		WOI_StartX, WOI_StartY;
	INT		WOI_LengthX, WOI_LengthY;
	INT     PixelXstep;
} WindowingWOI, * WindowingWOI_ptr;

typedef struct {
	BYTE	Contrast;
	FLOAT	fStd;
	FLOAT	fMean;
	BYTE	ContrastStart, ContrastEnd;

	WindowingWOI CstWOI;

	BYTE	wMinHist;
	DWORD	dwMinHistPixelNum;
	BYTE	wMaxHist;
	DWORD	dwMaxHistPixelNum;

	WORD	dwEffectHistPixelNum;
	BYTE	bCriteriaLow, bCriteriaHigh, ContrastCriteria;
	FLOAT	StdCriteria;

} FPContrast, *pFPContrast;

typedef struct {
		/*
		      Top
			 -------
			|		|
	   Left |		| Right
     middle  ------- 
			|		|
			|		|
			 -------
		      Down(Line0)
		*/
	INT		TopIgnoreNum;
	INT		DownIgnoreNum;
	INT		LeftIgnoreNum;
	INT		RightIgnoreNum;
	INT		MiddleIgnoreNum;
} BadPixelCheckWOI, *BadPixelCheckWOI_ptr;

typedef struct {
	Bin2Err	    mBin2Err;
	Bin4Err		mBin4Err;
	Bin5Err		mBin5Err;
	Bin6Err		mBin6Err;
	Bin7Err		mBin7Err;
	Bin8Err		mBin8Err;
	Bin9Err		mBin9Err;
	Bin10Err	mBin10Err;
	Bin11Err	mBin11Err;
	Bin12Err	mBin12Err;
	Bin13Err	mBin13Err;
	Bin14Err	mBin14Err;
	Bin15Err	mBin15Err;
	Bin16Err	mBin16Err;
	Bin17Err	mBin17Err;
	/////////////////////////////////
	// SY09 Testing
	Bin20Err	mBin20Err;
	Bin21Err	mBin21Err;
	Bin22Err	mBin22Err;
	Bin23Err	mBin23Err;
	Bin24Err	mBin24Err;
	Bin26Err	mBin26Err;
	Bin27Err	mBin27Err;
	Bin28Err	mBin28Err;
	Bin29Err	mBin29Err;
	Bin30Err	mBin30Err;
	Bin31Err	mBin31Err;
	Bin32Err	mBin32Err;
	Bin33Err	mBin33Err;
	Bin34Err	mBin34Err;
	/////////////////////////////////
	// USB transfer error
	Bin99Err	mBin99Err;
	Bin41Err	mBin41Err;
	Bin42Err	mBin42Err;
	Bin43Err	mBin43Err;
	Bin44Err	mBin44Err;
	UINT		mBinErrNum;
	UINT		mbPEBinNum;
	LONG        dwError;
} BINErrType;

typedef struct {
	INT					Bin7PECtaWidth;
	INT					Bin7CtaPETNum;
	INT					Bin7CtaPETValue;
	INT					Bin8CtaPETNum;
	INT					Bin9CtaPETNum;
	INT					Bin10CtaPETNum;
	UINT				RCornIgnorePETNum;
	//FPContrast			mFPContrast;
	BadPixelCheckWOI	mBPChkWOI;
	INT					PEMeanDiff;
    INT					PEIGMeanDiff;
	INT					ColMeanDiff;
	INT					ColFirstMeanDiff;
	INT					ColEndMeanDiff;
	INT					ColIGMeanDiff;
	INT					ColHistPeakMeanDiff; //Column mean difference to image histogram peak value (Should be average gray scale if it is background image).
	INT					RowMeanDiff;
	INT					RowFirstMeanDiff;
	INT					RowEndMeanDiff;
	INT					RowIGMeanDiff;
	INT	                RowPixelStep;	// use Software to count WOI
	INT	                ColPixelStep;	// use Software to count WOI
	RCIgnoreBadPixel_ptr	mptRCIgnoreBPArray;
	DWORD				mRCIgnoreBPANum;
	BadPixelArray		mBPArray[MAXIMUM_PIXELERR_ARRAY_NUM];
	WORD				TotalRCornerIgnoreBPSize;
	IgnoreRowColTable	m_IgnoreRowColTable;
} PixelErrCriteria;

typedef struct {
	INT		StdHigh;
	INT		MeanLow;
} Bin5ErrCriteria;

typedef struct {
	INT		StartX;
	INT		StartY;
	INT		WidthX;
	INT		HightY;
	INT		Raw_WidthX;
	INT		Raw_HightY;
} SensorSize;

typedef struct {
	DWORD	dwHistogram[256];
	DWORD	dwHistogramPeak;
	WORD	HigramShrinkPixelNum;
	DWORD	HigramTotalPixelNum;
} Histogram;

typedef struct {
	BYTE	CaliVersion;		// Calibration version selection  
	BYTE	CaliType;		    // Calibration type selection  
	BYTE	StdMaxCriteria;
	BYTE    TuningPhase;
	INT		iAdcOffSetValue;
	INT		iPreAdcOffSetValue;
	INT     iAdcOffsetShiftValue; //A register shift value after calibration
	INT		CaliTarget;
	INT		CaliTargetApplyChk;
	INT		CaliTargetRange;
	INT		CaliTargetRange2;
	BYTE	CaliTargetStepFinetuneRange;
	INT		CaliTargetGain;
	INT		CaliDefaultDCOffset;
	BYTE	PEChkDCOffsetValue;
	DWORD	DCOffsetHistHigh;
	DWORD	DCOffsetHistLow;
	FLOAT	Hist2DCOffsetStep;
	BYTE	bDCOffsetFinalValue;
	BYTE    bDCOffsetValueBeforeShift; //DCOffset calibration data without add iAdcOffsetShiftValue
	BOOL	bDCOffsetCaliDone;
} DCOffset;

typedef struct {
	INT iMeanValue;
	INT iStdValue;
	INT	BoundHigh;
	INT	BoundLow;
} BadPixelCalculateValue;

typedef struct {
	INT	iMeanValue;
	INT	iStdValue;
} LowCapData;

typedef struct {
	WORD	MeanDiffCriteria;
	LowCapData	mLowCapData[CAPMODE_ARRAY_NUM];
} LowCapDataAry;

typedef struct {
	BYTE bFDTrimMax;
	BYTE bFDTrimMin;
	BYTE bTrimValue[2];  // for return 
} FDTrim, *FDTrim_ptr;

typedef struct {
	BOOL	CstMsgEnable;
} DebugMode;

typedef struct {
	LONG	x, y;
	DWORD	ColorRef;
	int	    CircleWidth;
} DrawPixel, *DrawPixel_ptr;

typedef struct {
	LONG	lx, ly, rx,ry;
	DWORD	ColorRef;
} DrawRXArea, *DrawRXArea_ptr;

typedef struct {
	LONG	lx, ly, rx, ry;
	DWORD	ColorRef;
} DrawBadLineArea, *DrawBadLineArea_ptr;

typedef struct {
	BYTE  PeakValue;		// For roughly setting DAC
	BYTE  PeakThresold;		// thresold for adjusting DAC value
	WORD  DACValueConst;	// store default DAC value setting which should not be change after assigned value.
	INT  DACValue;			// default DAC value
	WORD  DACStepValue;		// DAC value adjust step value when image gray scale always at 0x00 or 0xFF.
	INT  SetRegValUpper;	// TDAC register value high boundary.
	INT  SetRegValLower;	// TDAC register value low boundary.
} TargetPeak, *TargetPeak_ptr;

//Bin 29 RX Channel Check Parameters, include must parameters and criterias.
typedef struct {
	INT		miGain;	//Gain setting, use maximum gain
	INT		miDCOffset; //Offset setting, to glass sensor, move to as dark as possible
	FLOAT	mfRXCh_MeanDiffCTA; //Average of gray scale
	FLOAT	mfRXCh_StdDiffCTA; //Average of standard deviation
	FLOAT	mfRXCh_AdjMeanDiffCTA; //Adjcent RX channel mean difference
} Bin29RXChkParam, * Bin29RXChkParam_ptr;

//Bin 26 On Stamp Background Check Error Criteria
typedef struct {
	FLOAT fMeanDiff;
	FLOAT fStd;
} Bin26ErrCriteria;

//Bin 30 On Stamp Foreground Check Error Criteria
typedef struct {
	FLOAT fMeanDiff;
	FLOAT fStdDiff;
	FLOAT fFullStd;
} Bin30ErrCriteria;

typedef struct {
	FLOAT	fSimCriteria;
	FLOAT	fDiffCriteria;
	FLOAT	fSimilarity0_11;
	FLOAT	fSimilarity0_6;
} StableChk, * StableChk_ptr;

typedef struct {
	WORD					mTestingChipID;
	SensorSize				mSensorSize;
	Histogram				mHistogram;
	DCOffset				mDCOffset;
	BadPixelCalculateValue	mBadPixel;
	Bin5ErrCriteria			mBin5ErrCta;
	BINErrType				mBINErrType;
	PixelErrCriteria		mPECrteria;

	FPContrast				mFPContrast;

	LowCapDataAry			mLowCapDataAry;
	pfnMessageLog			mfnMsgLog;
	//TCHAR					mMegString[MSGSTRINGSIZE + 1];
	WindowingWOI            mWindowingWOI;	// For WOI
	FDTrim					mFDTrim;		// For checking trim 
	DCOffestCaliResult      mCaliResult;	// For keeping the testing result
	TargetPeak              mTargetPeak;	// For setting DAC value 
	DebugMode				mDebugMode;
	void					*mScan;			// for WAIT_SCAN_PARAMETER
	//DWORD					mAppSign;		// use timestamp
	WindowingWOI            mReCapWOI;	// For Re-Capture image
	WORD                    mCoatingPxlCriteria;
	WORD                    mCoatingPxlNum;
	//BOOL                    mBIN2CoverPixelCheck;
	//BOOL                    mBIN2BlackPixelCheck;

	DOUBLE                  mADC_OperatorMode_Min;
	DOUBLE                  mADC_OperatorMode_Max;
	DOUBLE                  mADC_StandbyMode_Min;
	DOUBLE                  mADC_StandbyMode_Max;
	//WORD                    mDCOffsetCrteriaRange;
	//BOOL                    mModuleFAPx;
	BYTE                    bModuleID;

	INT                     raw_data_size;
	//INT                     mGlassType;
	INT                     mGlass_RX_TotalNum;
	INT                     mGlass_RX_UseNum;
	INT                     mGlass_SubFrameNum;
	//INT                     miTestGain;
	//INT                     miDCOffset;
	//FLOAT                   mfRXCh_MeanDiffCTA;
	//FLOAT                   mfRXCh_StdDiffCTA;
	Bin29RXChkParam			mBin29RXChkParam;
	BYTE                    mRXCaliOFSValue[4];
	BYTE                    bCheckCoating_ContrastGain;
	//INT                     mCustomer;
	//BYTE                    bCheckRXOfsGain;
	BYTE                    bRXOfsChannelNum;
	WORD                    mRXOfsCrteriaRange;
	BOOL					mbRxCaliDone;

	INT                     mTPN_Verify_Size;
	BYTE                    m_FT_TestType;

	BYTE					mStampPosition; //Keep the position of stamp which need to know when execute stamp on sensor test items.
	StableChk				mStableCheck;
	Bin26ErrCriteria		mBin26ErrCta; //Bin26 on stamp background check criteria
	Bin30ErrCriteria		mBin30ErrCta; //Bin30 on stamp foreground check criteria
	FLOAT					mfStabilityThreshold; //Image stability threshold
	FLOAT					mfStabilityDiff; //Image stability diff of double two frame check

	BOOL					m_is_FT_running;
	BOOL					m_do_with_bmp_file;

} FTTestParameter, *FTTestParameter_ptr;

typedef struct {
	short		   BinErr2, BinErr3, BinErr4, BinErr5, BinErr6, BinErr7, BinErr8, BinErr9, BinErr10;
	short BinErr11,BinErr12,BinErr13,BinErr14,BinErr15,BinErr16,BinErr17,BinErr18,BinErr19,BinErr20;
	short BinErr21,BinErr22,BinErr23,BinErr24,BinErr25,BinErr26,BinErr27,BinErr28,BinErr29,BinErr30;
	short BinErr31,BinErr32,BinErr33,BinErr34;
	short BinErr41,BinErr42;
	short BinErr99;
}FTTestBINErrorNum;

typedef struct {
	int total, pass, fail;
	float yield;
}FTTestResultTotal;

//typedef struct {
//	CString wcON, wcRCN, wcPON, wcLN, wcTR;
//	CString wcPF, m_strProgramName;
//	CString m_PartNo, m_StartDate, m_EndDate;
//
//	map<int, int> bin_nums;
//	FTTestResultTotal resultTotal;
//	FTTestBINErrorNum binErrNum;
//}FTTestLotInfo, *FTTestLotInfo_ptr;

//typedef struct {
//	CImage  mImgDrawBP;
//	BYTE    gain;
//	BYTE    offset;
//}FTTestBPDraw;

enum {
	STAMP_NONE,
	STAMP_TOP,
	STAMP_DOWN,
	STAMP_LEFT,
	STAMP_RIGHT,
	STAMP_UNKNOWN,
};

typedef struct {
	DOUBLE	fHistogram[2][256];
	DOUBLE  fSimilarHist[256];
	DOUBLE	flTotalSimilar;
	UINT16	iHistogramPxlNum[2];
} st_subFrameSimilarityChk, *st_subFrameSimilarityChk_ptr;
