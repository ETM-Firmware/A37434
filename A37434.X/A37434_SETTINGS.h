#ifndef __A37434_SETTINGS
#define __A37434_SETTINGS





#define AFC_MOTOR_MIN_POSITION                 1000
#define AFC_MOTOR_MAX_POSITION                 34000

#define AFC_CONTROL_WINDOW_RANGE               4000

#define FAST_MOVE_TARGET_DELTA                 128   // 4 steps
#define SLOW_MOVE_TARGET_DELTA                 64    // 2 steps  


#define MOTOR_SPEED_FAST                       200   // Motor Speed in Full Steps per Second in "Fast Mode"
#define MOTOR_SPEED_SLOW                       50    // Motor Speed in Full Steps per Second in "Slow Mode"


#define NO_PULSE_TIME_TO_INITITATE_COOLDOWN    100    // 1 second
#define LIMIT_RECORDED_OFF_TIME                120000 // 1200 seconds, 20 minutes // 240 elements


#define MAX_ADC_READING_100_UV                 21000
#define MIN_ADC_READING_100_UV                 5000


#define MINIMUM_FAST_MODE_PULSES               100
#define MAXIMUM_FAST_MODE_PULSES               4000
#define INVERSIONS_TO_REACH_SLOW_MODE          10
#define MAX_NO_DECISION_COUNTER                4

#define MINIMUM_POSITION_CHANGE                16
#define MINIMUM_REV_PWR_CHANGE                 50      //.5 dB


/*
#define AFT_CONTROL_VOLTAGE_MAX_PROGRAM        12000
#define AFT_CONTROL_VOLTAGE_MIN_PROGRAM        1000
*/

#endif
