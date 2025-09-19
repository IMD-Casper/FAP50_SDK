#pragma once
#define __PRODUCT_HEADER_FILE__
#include "Product_Define.h"

// ===================================================================================================
// Define Product Part   
// ===================================================================================================
#define _FAP50_15000_STR	"_FAP50_15000"
#define PRODUCT_PARTNO_STR	_FAP50_15000_STR

#define PRODUCT_NAME		PRODUCT_FAP50_FIRST
#define IMG_MCU_SORT        0

#define IG_AFE_TPN_VERIFY_SIZE    32*256*2

#define MODULE_ID		    PRODUCT_FAP50_FIRST
#define RXOFSCALIB		    0
#define PANEL_TYPE			0

#define COATING_COLOR		COATING_BLACK
#define COATING_THICKNESS	COATING_30um
#define VDD_VOLTAGE		    VDD_3_3
#define USE_CREATING_COATING
#define USE_TDAC_TRIM_DATA  1 //Write tdac[9:0] register data in OTP.
#define ENCRYPTKEY_FUN	    0

// ===================================================================================================
// Define Sensor 
// ===================================================================================================
#define PRODUCT_CHIP_ID		    CHIP_ID_FAP50
#define IG_AFE_RX_NUM           32	
#define MODULE_RX_NUM			31  //RX channel number which be used in current module.
#define IG_AFE_SUBFRAME_NUM		2*13
#define IG_AFE_TPN_VERIFY_SIZE  32*256*2

#define SENSOR_WIDTH_X	    	1600
#define SENSOR_HIGHT_Y	    	1000
#define SENSOR_RAW_WIDTH_X		1024
#define SENSOR_RAW_HIGHT_Y		1600

//WOI parameters
#define SW_WOI_ENABLE   0   //Set 0 to non-SF20x seriess glass sensor
#define WOI_START_X		0
#define WOI_START_Y		0
#define WOI_LENGTH_X	SENSOR_WIDTH_X
#define WOI_LENGTH_Y	SENSOR_HIGHT_Y

#if ( IMG_MCU_SORT == 1)	
#define PRODUCT_SENSOR_RAWDATA_SIZE	(SENSOR_WIDTH_X * SENSOR_HIGHT_Y)
#else
#define PRODUCT_SENSOR_RAWDATA_SIZE	(SENSOR_RAW_WIDTH_X * SENSOR_RAW_HIGHT_Y)
#endif
// ===================================================================================================
// Define DC Offset Calibration Target
// ===================================================================================================   
#define DCOFFSET_CALI_TARGET						249//250
#define DCOFFSET_CALI_TARGET_RANGE                  4//5*2
#define DCOFFSET_CALI_TARGET_RANGE2                 8  
#define DCOFFSET_CALI_TARGET_STEPFINETUNE_RANGE     7  
#define PECHK_DCOFFSET_VALUE                        0x28
#define DAC_TARGET_PEAK_VALUE                       180  
#define DAC_TARGET_PEAK_THRESOLD                    5
#define DCOFFSET_CALI_STD_CRITERIA			26 // 20 // 12
#define CONTRASTCHK_STD_CRITERIA			20
#define HISTOGRAM_SHRINK_PIXEL_NUMBER		0
#define HIST2DCOFFSET_STEP					2.56
#define RXOFS_CALI_TARGET_RANGE				3
#define DEFAULT_ADCOFFSET_VALUE				0x10
#define DEFAULT_GAIN_VALUE					0x09
#define DEFAULT_CONTRASTGAIN_VALUE			0x09
#define CHECKCOATING_CONTRASTGAIN_VALUE		0x09
#define DCOFFSET_SHIFT_VALUE				0x00
#define CHECK_RX_OFS_GAIN					0x00
#define RXOFSCHANNELNUM						7
// ===================================================================================================
// Define Bad Pixel measure condition
// ===================================================================================================   
#define BIN7_PE_CRITERIA_WIDTH          12	//12*12 pixel area
#define BIN5_STD_HIGH_CRITERIA	        9
#define BIN5_MEAN_LOW_CRITERIA		    5

#define BIN7_PE_CRITERIA_NUMBER         5  
#define BIN8_PE_CRITERIA_NUMBER		    180 
#define BIN9_PE_CRITERIA_NUMBER		    30 
#define BIN10_PE_CRITERIA_NUMBER	    40

#define BIN7_PE_CRITERIA_VALUE          200
#define TOTAL_RCORNER_IGNORE_BADPIXEL	1
#define BADPIXEL_IGNORE_TOP_NUM		    0  
#define BADPIXEL_IGNORE_DOWN_NUM	    0
#define BADPIXEL_IGNORE_LEFT_NUM	    0
#define BADPIXEL_IGNORE_RIGHT_NUM	    0
#define BADPIXEL_IGNORE_MIDDLE_NUM	    0

// ===================================================================================================  
// Define ADC measure condition
// ===================================================================================================  
#define ADC_OPERATORMODE_MAX           14.7
#define ADC_OPERATORMODE_MIN           9
#define ADC_STANDBYMODE_MAX            0//TBD
#define ADC_STANDBYMODE_MIN            0//TBD

// ===================================================================================================
// Define DC Offset Calibration Condition
// ===================================================================================================
#define CALI_VERSION			0x20//iG102 :version + revision = 0x21~0x3F
#define CALI_REVISION			3

#define CALIB_IC_GAIN			0x00
#define CALIB_IC_DCOFS			0x00
#define CALIB_IC_TDAC			0x00
#define CALIB_MODULE_GAIN		DEFAULT_GAIN_VALUE
#define CALIB_MODULE_DCOFS		DEFAULT_ADCOFFSET_VALUE
#define CALIB_RX_HB             0x00
#define CALIB_RX1_LB			0x00
#define CALIB_RX2_LB			0x00
#define CALIB_RX3_LB			0x00
#define CALIB_RX4_LB			0x00
#define COATINGTHICK			COATING_THICKNESS
#define COATINGVENDOR			COATING_VENDOR_JETBEST
#define CHECK_BADPIXEL_GAIN		DEFAULT_GAIN_VALUE
#define CHECK_BADPIXEL_DCOFS	0x00//Make it as dark as possible.
#define CHECK_CONTRAST_GAIN		DEFAULT_CONTRASTGAIN_VALUE
#define CHECK_CONTRAST_DCOFS	0x14
// ===================================================================================================
// Contrast criteria
// ===================================================================================================
#define CONTRAST_RANGE_LOW				140
#define CONTRAST_RANGE_HIGH				210//240
// ===================================================================================================
// Pixel Error criteria
// ===================================================================================================
#define PIXELERROR_MEAN_DIFF			17//35
#define PIXELERROR_IG_MEAN_DIFF			30//70
#define COLUMNERROR_PEAKMEAN_DIFF		10//Mean differnece of column number N to image mean value (histogram peak) difference
#define COLUMNERROR_MEAN_DIFF			5//5//10
#define COLUMNERROR_FIRSTCOLMEAN_DIFF	5//15
#define COLUMNERROR_ENDCOLMEAN_DIFF		5//15
#define COLUMNERROR_IG_MEAN_DIFF		8//10
#define ROWERROR_MEAN_DIFF				5//10 
#define ROWERROR_FIRSTROWMEAN_DIFF		5//12
#define ROWERROR_ENDROWMEAN_DIFF		5//12
#define ROWERROR_IG_MEAN_DIFF			8 
// ===================================================================================================
// Bin29: RX Channel Error criteria
// ===================================================================================================
#define BIN29_RXCHANNEL_GAIN                0x00
#define BIN29_RXCHANNEL_DCOFFSET            0x00
#define BIN29_RXCHANNEL_MEAN_CRITERIA       10 
#define BIN29_RXCHANNEL_STD_CRITERIA        8.0
#define BIN29_RXCHANNEL_ADJMEAN_CRITERIA    5.5
// ===================================================================================================
// Bin26: On Stamp Background Check Error criteria
// ===================================================================================================
#define BIN26_ONSTAMPBG_MEANDIFF_CRITERIA   10.0
#define BIN26_ONSTAMPBG_STD_CRITERIA        5.0
// ===================================================================================================
// Image Stability criteria
// ===================================================================================================
#define STABILITY_THRESHOLD                 0.985//0.992
#define STABILITY_DIFF                      0.002//0.992
// ===================================================================================================
// Bin30: On Stamp RXChannel Check Error criteria
// ===================================================================================================
#define BIN30_ONSTAMPRX_MEANDIFF_CRITERIA     5.0
#define BIN30_ONSTAMPRX_STDDIFF_CRITERIA      1.0
#define BIN30_ONSTAMPRX_STDFULL_CRITERIA      5.0

//for EQC setting
#define PIXELERROR_MEAN_DIFF_EQC		    (PIXELERROR_MEAN_DIFF)+5
#define PIXELERROR_IG_MEAN_DIFF_EQC         (PIXELERROR_IG_MEAN_DIFF)+5
#define COLUMNERROR_PEAKMEAN_DIFF_EQC       (COLUMNERROR_PEAKMEAN_DIFF)+2
#define COLUMNERROR_MEAN_DIFF_EQC		    (COLUMNERROR_MEAN_DIFF)+2
#define COLUMNERROR_FIRSTCOLMEAN_DIFF_EQC   (COLUMNERROR_FIRSTCOLMEAN_DIFF)+2
#define COLUMNERROR_ENDCOLMEAN_DIFF_EQC     (COLUMNERROR_ENDCOLMEAN_DIFF)+2
#define COLUMNERROR_IG_MEAN_DIFF_EQC		(COLUMNERROR_IG_MEAN_DIFF)+2
#define ROWERROR_MEAN_DIFF_EQC			    (ROWERROR_MEAN_DIFF)+2
#define ROWERROR_FIRSTROWMEAN_DIFF_EQC      (ROWERROR_FIRSTROWMEAN_DIFF)+2
#define ROWERROR_ENDROWMEAN_DIFF_EQC        (ROWERROR_ENDROWMEAN_DIFF)+2
#define ROWERROR_IG_MEAN_DIFF_EQC           (ROWERROR_IG_MEAN_DIFF)+2

#define DCOFFSET_CALI_TARGET_FT2        DCOFFSET_CALI_TARGET
#define CONTRAST_RANGE_LOW_FT2			CONTRAST_RANGE_LOW
#define CONTRAST_RANGE_HIGH_FT2			CONTRAST_RANGE_HIGH

//Define ignore row\column line number array which need to use different criteria.
#define IGNORE_ROW_ARRAY       {249, 499}
//#define IGNORE_COLUMN_ARRAY    {255}