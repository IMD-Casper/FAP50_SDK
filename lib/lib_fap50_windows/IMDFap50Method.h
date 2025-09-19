#pragma once
#include "IMDFTTestMethodClass.h"
#include "INI.h"


typedef vector<ImdDiamond> ImdContours;

enum E_IMG_KIND {
	IMG_KIND_NORMAL,
	IMG_KIND_RAW,
};
enum HAND_DIRECTION {
	RIGHT_HAND,
	LEFT_HAND,
};
enum E_IMG_TYPE {
	IMG_TYPE_BMP,
	IMG_TYPE_PNG,
	IMG_TYPE_WSQ,
	IMG_TYPE_RAW,
	IMG_TYPE_JP2, //JPEG2000
	IMG_TYPE_FIR, //ISO 19794-4:2011
	IMG_TYPE_PGM, 
};
enum ENABLE_USB {
	ENABLE_USB_BOTH_OFF,
	ENABLE_USB_SPEAKER_ON_AND_MCU_OFF,
	ENABLE_USB_SPEAKER_OFF_AND_MCU_ON,
	ENABLE_USB_BOTH_ON,
};
enum LED_KIND {
	LED_ALL,	//for test mode all LED on/off
	LED_RIGHT_4_FINGERS,
	LED_LEFT_4_FINGERS,
	LED_2_THUMBS_FINGERS,
	LED_ROLLING_MODE,
};
enum LED_STATUS {
	LED_OFF,
	LED_ON,
};

class IMDFap50Method 
	: public IMDFTTestMethodClass
{
#ifdef _WIN32
	CIniReader ini;
#endif

	//C_UsbPhysical _4202_mcu;
	E_FINGER_POSITION finger_position_code;
	vector<E_FINGER_POSITION> select_fingers;
	ImageStatus m_image_status;
	Mat flat_img[FINGER_POSITION_SIZE],
		roll_img[FINGER_POSITION_SIZE];
	int roll_score[NFIQ_VER_SIZE][FINGER_POSITION_SIZE][4],
		flat_score[NFIQ_VER_SIZE][FINGER_POSITION_SIZE][4];

	bool m_is_reader, m_is_display, m_is_security, m_is_mute = false;
	string	finger_position_text;

	/** If m_img_kind is IMG_KIND_RAW, you will get the image without any image
		processing. The default is IMG_KIND_NORMAL. */
	E_IMG_KIND img_kind;
	vector<vector<Point>> contours;
	ImdContours imd_contours;
	BYTE m_reg12;

	void init_image();
	void sort_finger(E_FINGER_POSITION position);
	IMD_RESULT get_finger_num(ImageStatus* img_status, void* scan);
	void mat_to_file(string& file_path, Mat src, E_IMG_TYPE img_type);
	int get_mat_score(Mat image);
	void get_finger_position(int finger_num);
	void save_to_fir(string file_path, Mat src);
	void check_hand_direction(ImageStatus* img_status);
	IMD_RESULT save_to_memory(ImageStatus* img_status);
	IMD_RESULT get_score(ImageStatus* img_status);
	void recovery_power_drop();
	bool reset_FAP50();
	void thread_life_check();
	void thread_init_nfiq2();
	void make_image_status();
	void cali_reg12();
	void cali_reg12_for_fw0336();

public:
	char* m_nfiq2_hash = NULL;

	IMDFap50Method();
	virtual ~IMDFap50Method();
	IMD_RESULT init_device();

	void scan_loop();
	bool connect_usb_device(bool force_reset= false);

	//virtual IMD_RESULT open_device();
	//virtual IMD_RESULT close_device();
	virtual IMD_RESULT device_reset();
	virtual IMD_RESULT get_system_property(SystemProperty* p);
	virtual IMD_RESULT set_system_property(SystemProperty* p);
	virtual IMD_RESULT scan_start(E_GUI_SHOW_MODE mode, E_FINGER_POSITION* pos, int num);

	virtual bool is_scan_busy();
	virtual IMD_RESULT scan_cancel();
	virtual IMD_RESULT get_image_status(ImageStatus *status);
	virtual IMD_RESULT save_file(E_GUI_SHOW_MODE mode, E_FINGER_POSITION finger_pos, const char* file_path);
	virtual IMD_RESULT get_image(ImageProperty& img_pty);
	virtual IMD_RESULT usb_switch(BYTE usb_sel);
	virtual IMD_RESULT set_led_speech_standby_mode();
	virtual IMD_RESULT set_burn_code();
	virtual IMD_RESULT user_space(bool write, WORD offset, BYTE* data, int len);
	virtual IMD_RESULT signature(E_SIGNATURE_ACTION action);

	//fw update
	virtual IMD_RESULT clear_flag();
	virtual IMD_RESULT update_app(const char* file_name);

};

