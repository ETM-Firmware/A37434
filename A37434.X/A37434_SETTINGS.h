#ifndef __A37434_SETTINGS
#define __A37434_SETTINGS




//#define __NJRC_MAGNETRON





// Motor Configuration
#define AFC_MOTOR_MIN_POSITION                 1000
#define AFC_MOTOR_MAX_POSITION                 28000
#define MOTOR_SPEED                            200   // Motor Speed in Full Steps per Second

#define AFC_MOTOR_MAX_POSITION_STARTUP         45000

// Cooldown Configuration
#define NO_PULSE_TIME_TO_INITITATE_COOLDOWN    100    // 1 second
#define LIMIT_RECORDED_OFF_TIME                120000 // 1200 seconds, 20 minutes // 240 elements


// Fast Mode Movement Configuration
#define AFC_CONTROL_WINDOW_RANGE               4000  // IF the Motor is more than this far away from the home position, it will just move to home position instead
#define FAST_MOVE_TARGET_DELTA                 64    // 2 steps
#define MAX_NO_DECISION_COUNTER                4
#define MINIMUM_POSITION_CHANGE                16

#define MINIMUM_REV_PWR_CHANGE_16K_PLUS         7    
#define MINIMUM_REV_PWR_CHANGE_11K_16K          5    
#define MINIMUM_REV_PWR_CHANGE_11K_MINUS        3


// Slow Mode Movement Configuaration

#ifndef __NJRC_MAGNETRON

#define MOVE_SIZE_BIG          64
#define MOVE_SIZE_SMALL        32
#define SAMPLES_AT_EACH_POINT  32

#else

#define MOVE_SIZE_BIG          16
#define MOVE_SIZE_SMALL        8
#define SAMPLES_AT_EACH_POINT  32

#endif



// Fast to Slow mode switch configuration
#define MAXIMUM_FAST_MODE_PULSES               400
#define MAXIMUM_FAST_MODE_TIME                 80     // 800 milliseconds


#endif
