/**
@file SensorCommonRegister.h
@brief Header file of Sensor common registers
@arg (C)Copyright Image Match Design Inc. 2014
@author     Box Chu
@version	1.00
@date		2014/07/01
*/
#pragma once

/**
 * @defgroup groupSF104Register SF104 Registers
 * \brief SF104 register address
 @{
 */
#define REG_CLOCK_CONTROL	    0x00    /**< clock control */
#define REG_PGA_CONTROL		    0x01    /**< pga2, pga1 control */
#define REG_DC_OFFSET		    0x02    /**< dc offset */
#define REG_ADC_SETTING		    0x03    /**< adc setting */
#define REG_OTP_ADDR		    0x04    /**< otp address */
#define REG_OTP_WDAT		    0x05    /**< otp write data */
#define REG_OTP_CTRL		    0x06    /**< otp control */
#define REG_OTP_READ_DATA	    0x07    /**< otp read data */
#define REG_DET_THRESHOLD	    0x08    /**< finger detection threshold */
#define REG_DET_DIVIDER		    0x09    /**< finger detection pulse period divider */
#define REG_ENABLE_BITS 	    0x0A    /**< function is_roll_mode bits */
#define REG_MODE_CONTROL_BITS	0x0B    /**< is_roll_mode control bits */
#define REG_WINDOW_LLX          0x0C    /**< windowing LLX */
#define REG_WINDOW_LLY          0x0D    /**< windowing LLY */
#define REG_WINDOW_URX          0x0E    /**< windowing URX */
#define REG_WINDOW_URY	        0x0F    /**< windowing URY */
#define REG_CHANGE_MODE0F	    0x0F    /**< change is_roll_mode for SF301 */
#define REG_CHANGE_MODE10	    0x10    /**< change is_roll_mode for SF104, SF201 */
#define REG_INSCANLINE_FP	    0x11    /**< in scan line finger detection number */
#define REG_INDETECTLINE_FP     0x12    /**< detection phase finger detection number */
#define REG_ANALOG_OSC          0x13    /**< internal osc frequency selection */
#define REG_ANALOG_FDET         0x14    /**< analog finger detection threshold setting */
#define REG_RESERVE15           0x15    /**< internal usage */
#define REG_TDAC_SIGNED         0x10    /**< internal usage */
#define REG_RESERVE1D           0x1D    /**< internal usage */

/**
 * @defgroup groupiMDRegister_iG102 iG102 Registers
 * \brief iG102 register address
 @{
 */
#define REG_CHIPID_H    	    0x00    /**< chip ID high */
#define REG_CHIPID_L		    0x01    /**< chip ID low */
#define REG_AFE_CONTROL         0x02    /**< afe control */
#define REG_ADC_CONTROL		    0x03    /**< adc control */
#define REG_VLDO_TRM		    0x04    /**< vldo trim */
#define REG_VR_TRM              0x05    /**< vr trim */
#define REG_QP_TRM              0x06    /**< qp trim */
#define REG_CALIB_OFS   	    0x07    /**< offset calibrating */
#define REG_CLOCK_CYCLE  	    0x08    /**< operation clock control */
#define REG_SEL_NUM 		    0x09    /**< H_SEL max count */
#define REG_BIST_STATUS         0x0A    /**< Bist status and error message */
#define REG_RST_DELAY       	0x0B    /**< RST delay time from INT_VD */
#define REG_RST_WIDTH           0x0C    /**< RST pulse width*/
#define REG_INT_HD_DELAY        0x0D    /**< INT_HD delay time from RST fall edge */
#define REG_RG_RSV_0            0x0E    /**< reserved byte 0 */
#define REG_RG_RSV_1            0x0F    /**< reserved byte 1 */
#define REG_RG_DIG_RSV          0x10    /**< digital reserved byte */
#define REG_DIG_TEST            0x11    /**< option of test */
#define REG_INT_HD_WIDTH        0x12    /**< INT_HD delay time from EXVCOM */
#define REG_RESERVED            0x13    /**< */
#define REG_EXVCOM_DELAY        0x14    /**< EXVCOM delay time from INT_HD */
#define REG_EXVCOM_WIDTH        0x15    /**< EXVCOM pulse width */
#define REG_EXVCOM_NUM          0x16    /**< EXVCOM plus number */
#define REG_EXVCOM_RST          0x17    /**< EXVCOM pulse number/delay in RST */
#define REG_OSC         	    0x18    /**< oscillator */
#define REG_ANA_TEST 		    0x19    /**< analog tester */
#define REG_RG_ANA_RSV          0x1A    /**< analog reserved byte */
#define REG_LSF_TDAC        	0x1B    /**< linear shift tdac */
#define REG_RG_OTP_ADDR_BIT     0x1C    /**< OTP address & bit is_roll_mode*/
#define REG_RG_OTP_CTRL         0x1D    /**< OTP write control */
#define REG_RD_OTP_DATA         0x1E    /**< OTP read data */
#define REG_PAGE                0x1F    /**< page selection */

#define REG_AFE_CTL 		    REG_AFE_CONTROL    /**< pga2, pga1 control */
#define REG_CQP_OFFS            REG_RG_ANA_RSV    /**< dc offset */
