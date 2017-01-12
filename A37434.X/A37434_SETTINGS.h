#ifndef __A37434_SETTINGS
#define __A37434_SETTINGS

// Motor Configuration
#define AFC_MOTOR_MIN_POSITION                 1000
#define AFC_MOTOR_MAX_POSITION                 34000
#define MOTOR_SPEED_FAST                       200   // Motor Speed in Full Steps per Second in "Fast Mode"
#define MOTOR_SPEED_SLOW                       200   // Motor Speed in Full Steps per Second in "Slow Mode"


// Cooldown Configuration
#define NO_PULSE_TIME_TO_INITITATE_COOLDOWN    100    // 1 second
#define LIMIT_RECORDED_OFF_TIME                120000 // 1200 seconds, 20 minutes // 240 elements


// Fast Mode Movement Configuration
#define AFC_CONTROL_WINDOW_RANGE               4000
#define FAST_MOVE_TARGET_DELTA                 64    // 2 steps
#define MAX_NO_DECISION_COUNTER                4
#define MINIMUM_POSITION_CHANGE                16

#define MINIMUM_REV_PWR_CHANGE_10K_PLUS        50
#define MINIMUM_REV_PWR_CHANGE_7K_10K          40
#define MINIMUM_REV_PWR_CHANGE_7K_MINUS        25                  

// Slow Mode Movement Configuaration
#define MOVE_SIZE_BIG          64
#define MOVE_SIZE_SMALL        32
#define SAMPLES_AT_EACH_POINT  32


// Fast to Slow mode switch configuration
#define MINIMUM_FAST_MODE_PULSES               100
#define MAXIMUM_FAST_MODE_PULSES               4000

#define MAX_REV_POWER_ERROR_FOR_SLOW_MODE_10K_PLUS 200
#define MAX_REV_POWER_ERROR_FOR_SLOW_MODE_7K_10K   160
#define MAX_REV_POWER_ERROR_FOR_SLOW_MODE_7K_MINUS 100






// Minicircuit Conversion Configuration
#define MAX_ADC_READING_100_UV                 21000
#define MIN_ADC_READING_100_UV                 5000



#endif
