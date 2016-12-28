// Header file for new AFC Board
#ifndef __A37434_H
#define __A37434_H

#define FCY_CLK 10000000


#include <xc.h>
#include <libpic30.h>
#include <adc10.h>
#include <timer.h>
#include <pwm.h>

#include "P1395_CAN_SLAVE.h"
#include "ETM.h"
#include "A37434_SETTINGS.h"
#include "MCP4725.h"




/*
  Hardware Module Resource Usage

  CAN1   - Used/Configured by ETM CAN 
  Timer4 - Used/Configured by ETM CAN - Used to Time sending of messages (status update / logging data and such) 
  Timer5 - Used/Configured by ETM CAN - Used for detecting error on can bus

  SPI2   - Used/Configured by External ADC
  I2C    - Used/Configured by EEPROM Module, DAC Module

  Timer1 - Used for timing motor steps
  Timer3 - Used for 10ms Generation
  ADC Module - See Below For Specifics
  Motor Control PWM Module - Used to control AFC stepper motor

*/


/*
  ------------------- I/O PIN CONFIGURATION -------------------------
  Pins that need to be configured as outputs
  
  C1  PIN_INPUT_A_CS
  C3  PIN_INPUT_B_CS
  C13 PIN_MOTOR_DRV_ISET_A1
  C14 PIN_MOTOR_DRV_ISET_A0

  D0  PIN_MOTOR_DRV_ISET_B0
  D2  PIN_MOTOR_DRV_SLEEP_NOT
  D3  PIN_MOTOR_DRV_RESET_NOT
  D6  PIN_PIC_ADC_CONVERT
  D11 PIN_MOTOR_DRV_ISET_B1
  D12 PIN_MOTOR_DRV_DECAY_SELECT

  

*/

#define A37434_TRISA_VALUE 0b1111111111111111
#define A37434_TRISB_VALUE 0b1111111111111111
#define A37434_TRISC_VALUE 0b1001111111110101
#define A37434_TRISD_VALUE 0b1110011110110010
#define A37434_TRISE_VALUE 0b1111111111111111
#define A37434_TRISF_VALUE 0b1111111111111111
#define A37434_TRISG_VALUE 0b1111111111111111


// ------------- PIN DEFINITIONS ------------------- ///

// MOTOR CONTROL PINS
#define PIN_MOTOR_DRV_RESET_NOT          _LATD3
#define PIN_MOTOR_DRV_SLEEP_NOT          _LATD2
#define PIN_MOTOR_DRV_DECAY_SELECT       _LATD12

#define PIN_MOTOR_DRV_ISET_A0            _LATC14
#define PIN_MOTOR_DRV_ISET_A1            _LATC13
#define PIN_MOTOR_DRV_ISET_B0            _LATD0
#define PIN_MOTOR_DRV_ISET_B1            _LATD11

#define PIN_MOTOR_DRV_INPUT_NOT_FAULT    _RD1


// ADC Interface Pins
#define PIN_INPUT_A_CS                   _LATC1
#define PIN_INPUT_B_CS                   _LATC3
#define PIN_PIC_ADC_CONVERT              _LATD6
#define OLL_SELECT_ADC                   0


// Test Points
#define PIN_TEST_POINT_A                 _LATG1
#define PIN_TEST_POINT_B                 _LATG0
#define PIN_TEST_POINT_G                 _LATB10
#define PIN_TEST_POINT_H                 _LATB11
#define PIN_TEST_POINT_J                 _LATBF4
#define PIN_TEST_POINT_K                 _LATBF5

#define PIN_RESET_DETECT                 _RG7




// ---------------- Motor Configuration Values ------------- //
#define MOTOR_PWM_FREQ                    20000        // Motor Drive Frequency is 10KHz
#define FCY_CLK                           10000000      // 10 MHz
#define DELAY_SWITCH_TO_LOW_POWER_MODE    200


// --------------------- T1 Configuration -----
// With 1:8 prescale the minimum 1/32 step time is 52ms or a minimum speed of .6 Steps/second

#define T1CON_SETTING     (T1_ON & T1_IDLE_CON & T1_GATE_OFF & T1_PS_1_8 & T1_SYNC_EXT_OFF & T1_SOURCE_INT)
#define PR1_FAST_SETTING  (unsigned int)(FCY_CLK / 32 / 8 / MOTOR_SPEED_FAST)
#define PR1_SLOW_SETTING  (unsigned int)(FCY_CLK / 32 / 8 / MOTOR_SPEED_SLOW)


/* 
   TMR3 Configuration
   Timer3 - Used for 10msTicToc
   Period should be set to 10mS
   With 10Mhz Clock, x8 multiplier will yield max period of 17.7mS, 2.71uS per tick
*/

#define T3CON_VALUE                    (T3_ON & T3_IDLE_CON & T3_GATE_OFF & T3_PS_1_8 & T3_SOURCE_INT)
#define PR3_PERIOD_US                  10000   // 10mS
#define PR3_VALUE_10_MILLISECONDS      (unsigned int)((FCY_CLK / 1000000)*PR3_PERIOD_US/8)


// Motor Drive Configuration
#define PTCON_SETTING     (PWM_EN & PWM_IPCLK_SCALE1 & PWM_MOD_FREE)
#define PTPER_SETTING     (unsigned int)(FCY_CLK/MOTOR_PWM_FREQ)
// Special Event Trigger - TBD
#define PWMCON1_SETTING  (PWM_MOD1_COMP & PWM_MOD2_COMP & PWM_MOD3_COMP & PWM_MOD4_COMP & PWM_PEN1H & PWM_PEN1L & PWM_PEN2H & PWM_PEN2L & PWM_PEN3H & PWM_PEN3L & PWM_PEN4H & PWM_PEN4L)
#define PWMCON2_SETTING  (PWM_SEVOPS1 & PWM_OSYNC_TCY & PWM_UEN) 
#define DTCON1_SETTING   (PWM_DTAPS1 & PWM_DTA0 & PWM_DTBPS1 & PWM_DTB0)
#define DTCON2_SETTING   (PWM_DTS1A_UA & PWM_DTS1I_UA & PWM_DTS2A_UA & PWM_DTS2I_UA  & PWM_DTS3A_UA & PWM_DTS3I_UA & PWM_DTS4A_UA & PWM_DTS4I_UA)
#define FLTACON_SETTING  (PWM_FLTA1_DIS & PWM_FLTA2_DIS & PWM_FLTA3_DIS & PWM_FLTA4_DIS)
#define FLTBCON_SETTING  (PWM_FLTB1_DIS & PWM_FLTB2_DIS & PWM_FLTB3_DIS & PWM_FLTB4_DIS)
#define OVDCON_SETTING   (PWM_GEN_1H & PWM_GEN_1L & PWM_GEN_2H & PWM_GEN_2L & PWM_GEN_3H & PWM_GEN_3L & PWM_GEN_4H & PWM_GEN_4L)

// User code needs to set PDC1, PDC2 (Duty cycle 1 and 2)
// Duty cycle is encoded at twice the resolution of the Period.
#define PDC1_SETTING     PTPER_SETTING // 50% duty cycle
#define PDC2_SETTING     PTPER_SETTING // 50% duty cycle
#define PDC3_SETTING     PTPER_SETTING // 50% duty cycle
#define PDC4_SETTING     PTPER_SETTING // 50% duty cycle



// -------------------  ADC CONFIGURATION ----------------- //

/*
  ADC is configured as following
  CHN0 - AN4(B Input), Vref-
  CHN1 - AN3(A Input), Vref-
  CHN2 - AN4(B Input), Vref-
  CHN3 - AN5(unused), Vref-
*/

#define ADCON1_SETTING   (ADC_MODULE_ON & ADC_IDLE_STOP & ADC_FORMAT_INTG & ADC_CLK_INT0 & ADC_SAMPLE_SIMULTANEOUS & ADC_AUTO_SAMPLING_ON)
#define ADCON2_SETTING   (ADC_VREF_EXT_EXT & ADC_SCAN_OFF & ADC_CONVERT_CH_0ABC & ADC_SAMPLES_PER_INT_1 & ADC_ALT_BUF_OFF & ADC_ALT_INPUT_OFF)
#define ADCON3_SETTING   (ADC_SAMPLE_TIME_10 & ADC_CONV_CLK_SYSTEM & ADC_CONV_CLK_2Tcy)
#define ADCHS_SETTING    (ADC_CHX_POS_SAMPLEA_AN3AN4AN5 & ADC_CHX_NEG_SAMPLEA_VREFN & ADC_CH0_POS_SAMPLEA_AN4 & ADC_CH0_NEG_SAMPLEA_VREFN & ADC_CHX_POS_SAMPLEB_AN3AN4AN5 & ADC_CHX_NEG_SAMPLEB_VREFN & ADC_CH0_POS_SAMPLEB_AN4 & ADC_CH0_NEG_SAMPLEB_VREFN)
#define ADPCFG_SETTING   (ENABLE_AN3_ANA & ENABLE_AN4_ANA & ENABLE_AN9_ANA & ENABLE_AN13_ANA & ENABLE_AN14_ANA)
#define ADCSSL_SETTING   (SKIP_SCAN_AN0 & SKIP_SCAN_AN1 & SKIP_SCAN_AN2 & SKIP_SCAN_AN6 & SKIP_SCAN_AN7 & SKIP_SCAN_AN9 & SKIP_SCAN_AN10 & SKIP_SCAN_AN11 & SKIP_SCAN_AN12 & SKIP_SCAN_AN13 & SKIP_SCAN_AN14 & SKIP_SCAN_AN15)




typedef struct {
  //int          frequency_error_filtered;
  //unsigned long pulse_off_counter;
  //AnalogOutput aft_control_voltage;
  //unsigned int aft_A_sample_filtered;
  //unsigned int aft_B_sample_filtered;
  //unsigned int sample_index;
  //unsigned int aft_A_sample_history[16];
  //unsigned int aft_B_sample_history[16];
  //unsigned int aft_filtered_error_for_client;

  //unsigned int aft_control_voltage_low_energy;
  //unsigned int aft_control_voltage_high_energy;

  unsigned int sample_index;


  unsigned int control_state;
  unsigned int manual_target_position;
  unsigned int sample_complete;

  unsigned int fast_afc_done;
  unsigned int pulses_on_this_run;
  unsigned int afc_hot_position;

  AnalogInput  reverse_power_sample;
  AnalogInput  forward_power_sample;

  unsigned int time_off_counter;
  unsigned int inversion_counter;
  unsigned int no_decision_counter;
  unsigned int position_at_trigger;

  unsigned int a_adc_reading_internal;
  unsigned int b_adc_reading_internal;

  unsigned int a_adc_reading_external;
  unsigned int b_adc_reading_external;

  unsigned int reverse_power_db;
  unsigned int forward_power_db;


} AFCControlData;

extern AFCControlData global_data_A37434;



#define MOVE_UP       0
#define MOVE_NO_DATA  1
#define MOVE_DOWN     2


#define STATE_STARTUP       0x10
#define STATE_AUTO_ZERO     0x20
#define STATE_AUTO_HOME     0x30
#define STATE_RUN_AFC       0x40
#define STATE_RUN_MANUAL    0x50






typedef struct {
  unsigned int current_position;
  unsigned int target_position;
  unsigned int home_position;
  unsigned int max_position;
  unsigned int min_position;
  //unsigned int pwm_table_index;
  unsigned int time_steps_stopped;
} STEPPER_MOTOR;


typedef struct {
  unsigned int position[16];
  unsigned int reverse_power[16];
  unsigned int forward_power[16];
  unsigned int active_index;
} TYPE_POWER_READINGS;


#define _STATUS_AFC_MODE_MANUAL_MODE                    _LOGGED_STATUS_0
#define _STATUS_AFC_AUTO_ZERO_HOME_IN_PROGRESS          _LOGGED_STATUS_1
// DPARKER - REALLY NEED TO UPDATE THE DOCUMENTATION

#define _FAULT_CAN_COMMUNICATION_LATCHED                _LOGGED_FAULT_0




#define FULL_POWER_TABLE_VALUES 50,50,50,50,50,50,50,50,253,253,253,253,253,253,253,253,425,425,425,425,425,425,425,425,540,540,540,540,540,540,540,540,580,580,580,580,580,580,580,580,540,540,540,540,540,540,540,540,425,425,425,425,425,425,425,425,253,253,253,253,253,253,253,253,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50 

#define LOW_POWER_TABLE_VALUES 50,50,50,50,50,50,50,50,165,165,165,165,165,165,165,165,262,262,262,262,262,262,262,262,327,327,327,327,327,327,327,327,350,350,350,350,350,350,350,350,327,327,327,327,327,327,327,327,262,262,262,262,262,262,262,262,165,165,165,165,165,165,165,165,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50

#define COOL_DOWN_TABLE_VALUES 31130,31260,28985,27246,25880,24768,23833,23023,22303,21651,21052,20495,19972,19479,19012,18568,18146,17742,17357,16988,16635,16296,15972,15660,15361,15074,14798,14532,14277,14031,13794,13565,13345,13132,12927,12728,12536,12351,12171,11997,11828,11665,11506,11352,11202,11057,10916,10778,10644,10514,10387,10263,10142,10024,9908,9796,9686,9578,9473,9370,9269,9170,9073,8978,8884,8793,8703,8614,8528,8442,8359,8276,8195,8115,8037,7959,7883,7808,7734,7661,7589,7518,7448,7379,7311,7243,7177,7111,7047,6983,6919,6857,6795,6734,6674,6614,6555,6497,6439,6382,6326,6270,6215,6160,6106,6053,6000,5947,5895,5844,5793,5742,5693,5643,5594,5546,5498,5450,5403,5356,5310,5264,5219,5174,5130,5085,5042,4998,4956,4913,4871,4829,4788,4747,4706,4666,4626,4586,4547,4508,4470,4431,4394,4356,4319,4282,4245,4209,4173,4138,4102,4067,4033,3998,3964,3931,3897,3864,3831,3798,3766,3734,3702,3671,3639,3608,3578,3547,3517,3487,3457,3428,3399,3370,3341,3313,3285,3257,3229,3202,3174,3147,3121,3094,3068,3042,3016,2990,2965,2940,2915,2890,2865,2841,2817,2793,2769,2745,2722,2699,2676,2653,2631,2608,2586,2564,2542,2521,2499,2478,2457,2436,2416,2395,2375,2354,2334,2315,2295,2275,2256,2237,2218,2199,2180,2162,2143,2125,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

#endif
