#pragma once
#include "imd_framework.h"
#include "IMDFPMeasure.h"

//static const UINT CMD_DRAW_IMG = ::RegisterWindowMessage(_T("CMD_DRAW_IMAGE"));
//static const UINT CMD_DRAW_HISTRO = ::RegisterWindowMessage(_T("CMD_DRAW_HISTRO"));
//static const UINT CMD_ERASE_IMG = ::RegisterWindowMessage(_T("CMD_ERASE_IMAGE"));

#define IMAGE_WIDTH					1600
#define IMAGE_HEIGHT				1000
#define RAW_WIDTH					1600
#define RAW_HEIGHT					1024
#define IMAGE_SIZE					(IMAGE_WIDTH * IMAGE_HEIGHT)
#define RAW_SIZE					(RAW_WIDTH * RAW_HEIGHT)

#define ROLL_X		400
#define ROLL_Y		150
#define ROLL_WIDTH	800
#define ROLL_HEIGHT	750

#define RING_SIZE 4 //for thread

enum MOVE_DIRECTION
{
	MOVE_DIRECTION_RIGHT = 1,
	MOVE_DIRECTION_LEFT = -1,
};

struct RingUnit {
	BYTE fast[RAW_SIZE/4];
	BYTE raw[RAW_SIZE];
	BYTE img[IMAGE_SIZE];
	BOOL valid;
} ;
struct RingScan {
	RingUnit unit[RING_SIZE];
	int write_idx;
	//int read_idx;
} ;
struct CaliDoneReg {
	BOOL cali_done;
	BYTE reg1A[4];
	BYTE reg13[4];
};

class IMDFTTestMethodClass 
	: public InterfaceImdFap50Method
{
#define CONTRAST_OF_FINGER_ON		50
#define DELTA_FINGER_WIDTH			6
#define DEBOUNCE_FINGER_WIDTH_CNT	2
#define DEBOUNCE_FLAT_CNT			6
#define DEBOUNCE_ROLL_CNT			1

public:
	SystemProperty m_property;
	INT m_Height, m_Width;// , m_Height_back, m_Width_back;
	RingScan m_ring;

	Mat mat_no_isp, 
		mat_isp,
		mat_avg,
		sign_img,//(1000, 1600, CV_8UC1, Scalar(255)),
		sign_img_done;// (1000, 1600, CV_8UC1, Scalar(255));

	BYTE reg12_bkup,
		fap50_img[IMAGE_SIZE],
		fap50_raw[RAW_SIZE];

	CaliDoneReg cali_done_reg;

	ModuleInfo module_info;
	
	bool bImgMCUSort = false,
		is_thread_scan_busy = false,
		is_make_image_status_busy = false,
		enable_repair_line = false,
		enable_sw_auto_gain = false,
		enable_moving_avg = false,
		enable_filter_for_rolling = false,
		enable_continue_scan = false,
		enable_runtime_judge_black,
		enable_auto_cali_on_reset,
		enable_check_osc_stable,
		enable_hw_auto_gain = false,
		enable_hw_isp = false,
		enable_hw_boost = false,

		is_sampling_saved = false,
		is_done = false,
		is_too_top = false,
		is_wrong_hand = false,
		is_poor_contact = false,
		is_finger_shape_not_good = false,
		is_alert_finger_off = false,
		is_sign_done = false,
		is_sign_clear = false;


	IMD_RESULT last_result;
	E_PANEL_VERSION panel_type = RULE_1;
	E_GUI_SHOW_MODE show_mode = GUI_SHOW_MODE_NONE;
	void* scan_done;

	int image_black = -1;
	int image_white = -1;
	int debounce_flat_cnt = 0, 
		debounce_finger_width_cnt = 0, 
		debounce_roll_cnt=0;
	//BOOL roll_too_fast = FALSE;
	BOOL need_finger_off;

	IMDFTTestMethodClass(void);
	~IMDFTTestMethodClass(void);
	IMD_RESULT background_calibration_fap50(HANDLE a_hDevice);
	IMD_RESULT raw_to_img_fap50(void* pPara, ImageStatus *img_status);
	IMD_RESULT rolling_finger_fap50(BYTE* img, ImageStatus* img_status, bool is_first=false);
	IMD_RESULT signature_2_fap50(BYTE* img, ImageStatus* img_status);
	IMD_RESULT signature_by_LP_5(BYTE* img, ImageStatus* img_status);

	BOOL merge_img(Mat new_img, Mat base_img, MOVE_DIRECTION direction, int finger_width, Mat no_isp);
	void sign_2_merge_img(Mat new_img, Mat base_img);// , Point& last);
	void sign_2_merge_by_LP_5(Mat base_img);// , Point& last);

	void CalculateHistogram(Histogram* myHistogram, LPBYTE lpImage, DWORD dwImageSize);
	DWORD ContrastCalculation(FPContrast* myContrast, LPBYTE lpImage, LPDWORD a_lpdwHistogram, FPContrast* a_pstFPContrast);
	BOOL DoCaptureFT(FTTestParameter* a_ptFTTestParams, HANDLE a_hDevice);

	//virtual IMD_RESULT open_device() { return IMD_RLT_NO_IMPLEMENTATION; }
	//virtual IMD_RESULT close_device() { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT device_reset() { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT set_system_property(SystemProperty cfg) { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual SystemProperty get_system_property(void) { return SystemProperty(); }
	virtual IMD_RESULT set_system_property(SystemProperty* property) { return IMD_RLT_NO_IMPLEMENTATION; }

	virtual IMD_RESULT scan_start(E_GUI_SHOW_MODE mode, E_FINGER_POSITION* pos_buf, int num) { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT scan_cancel() { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT get_image_status(ImageStatus* status) { return IMD_RESULT(); }
	virtual bool is_scan_busy() { return FALSE; }
	//virtual char* get_system_info() { return ""; }
	virtual IMD_RESULT save_file(E_GUI_SHOW_MODE mode, E_FINGER_POSITION finger_pos, const char* file_path) { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT get_last_result() { return last_result; }
	virtual IMD_RESULT usb_switch(BYTE usb_sel) { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT set_led_speech_standby_mode() { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT set_burn_code() { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT get_image(ImageProperty& img_property) { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT user_space(bool write, WORD offset, BYTE* data, int len) { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT write_flag() { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT clear_flag() { return IMD_RLT_NO_IMPLEMENTATION; }
	//virtual IMD_RESULT update_app(const char* file_name) { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT serial_number(bool is_write, BYTE* data) { return IMD_RLT_NO_IMPLEMENTATION; }
	virtual IMD_RESULT signature(E_SIGNATURE_ACTION action) { return IMD_RLT_NO_IMPLEMENTATION; }

	virtual void set_last_result(IMD_RESULT result)
	{
		if (last_result == IMD_RLT_SUCCESS)
			last_result = result;
	}

};

