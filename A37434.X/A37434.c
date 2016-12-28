#include "A37434.h"
#include "FIRMWARE_VERSION.h"

// This is the firmware for the AFC BOARD


//#define __USE_AFT_MODULE

unsigned int reading_array_internal[1024];
unsigned int reading_array_external[1024];
unsigned int reading_array_location = 0;


// ------------------ PROCESSOR CONFIGURATION ------------------------//
_FOSC(ECIO & CSW_FSCM_OFF);                                               // 40MHz External Osc creates 10MHz FCY
_FWDT(WDT_ON & WDTPSA_512 & WDTPSB_8);                                    // 8 Second watchdog timer 
_FBORPOR(PWRT_OFF & BORV45 & PBOR_ON & MCLR_EN);                          //
_FBS(WR_PROTECT_BOOT_OFF & NO_BOOT_CODE & NO_BOOT_EEPROM & NO_BOOT_RAM);  //
_FSS(WR_PROT_SEC_OFF & NO_SEC_CODE & NO_SEC_EEPROM & NO_SEC_RAM);         //
_FGS(GWRP_OFF & GSS_OFF);                                                 //
_FICD(PGD);                                                               //

const unsigned int PWMHighPowerTable[128] = {FULL_POWER_TABLE_VALUES}; 
 /* 
   This table defines the duty cycle for higher current mode (used when the motor is moving)
   Generated by this spreadsheet
   https://docs.google.com/spreadsheets/d/1ZgWb8tD-m0kZZ0ukkrMd0YSsf2Md83Dp7B-wVZ7SCRs
*/

const unsigned int PWMLowPowerTable[128]  = {LOW_POWER_TABLE_VALUES};   
/* 
   This table defines the duty cycle for lower current mode  (used to hold the motor when it is not moving)
   Generated by this spreadsheet
   https://docs.google.com/spreadsheets/d/1ZgWb8tD-m0kZZ0ukkrMd0YSsf2Md83Dp7B-wVZ7SCRs
*/

const unsigned int CoolDownTable[256]     = {COOL_DOWN_TABLE_VALUES};
/*
  Cooldown Process
  global_data_A37434.time_off_counter is incremented every 10mS, and reset to zero with every pulse.
  After NO_PULSE_TIME_TO_INITITATE_COOLDOWN without a pulse the cooldown process will start.
  When the cooldown process starts the current position is stored as global_data_A37434.afc_hot_position
  CoolDownTable provides a Q1.15 multiplier so that position = home_position + CoolDownTable[x] * (afc_hot_position - home_position)
  CoolDownTable starts at 1 at zero time and reaches zero after 20 minutes
  Each position in CoolDownTable corresponds to 5.12 seconds.  So the motor is moved once every 5 seconds to match the new cooldown position
  The table is generated by this spreadsheet
  https://docs.google.com/spreadsheets/d/1pyvkoiT0XYzaxereZ0c7XMhMmgaBmKexELuavmLmR8k/
*/

MCP4725 U13_MCP4725;                     // This is the external DAC

TYPE_POWER_READINGS power_readings;      // This stores the history of the position and power readings for the previous 16 pulses 
STEPPER_MOTOR afc_motor;                 // This contains the control data for the motor
AFCControlData global_data_A37434;       // Global variables



void DoStateMachine(void);
void InitializeA37434(void);
void DoPostPulseProcess(void);
void DoAFCReversePower(void);
unsigned int ConvertToDBMiniCircuit(unsigned int adc_reading_100uV);
unsigned int CalculateDirection(unsigned int current_pos, unsigned int previous_pos, unsigned int current_rev_pwr, unsigned int previous_rev_pwr);
void DoAFCCooldown(void);
void DoA37434(void);
void UpdateFaults(void);
unsigned int ShiftIndex(unsigned int index, unsigned int shift);




int main(void) {
  global_data_A37434.control_state = STATE_STARTUP;
  while (1) {
    DoStateMachine();
  }
}


void DoStateMachine(void) {
  switch (global_data_A37434.control_state) {

  case STATE_STARTUP:
    InitializeA37434();
    _TRISG1 = 0;  // FOR DEBUGGING
    afc_motor.min_position = 0;
    afc_motor.max_position = AFC_MOTOR_MAX_POSITION;
    afc_motor.home_position = AFC_MOTOR_MAX_POSITION;
    afc_motor.time_steps_stopped = 0;
    global_data_A37434.control_state = STATE_AUTO_ZERO;
    break;

  case STATE_AUTO_ZERO:
    afc_motor.current_position = AFC_MOTOR_MAX_POSITION;
    afc_motor.target_position  = 0;
    _CONTROL_NOT_CONFIGURED = 1;
    _STATUS_AFC_AUTO_ZERO_HOME_IN_PROGRESS = 1;
    while (global_data_A37434.control_state == STATE_AUTO_ZERO) {
      DoA37434();
      if ((afc_motor.current_position <= 100) && (_CONTROL_NOT_CONFIGURED == 0)) {
	global_data_A37434.control_state = STATE_AUTO_HOME;
      }
    }
    break;

  case STATE_AUTO_HOME:
    //global_data_A37434.aft_control_voltage.enabled = 1;
    afc_motor.min_position = AFC_MOTOR_MIN_POSITION;
    afc_motor.max_position = AFC_MOTOR_MAX_POSITION;
    afc_motor.target_position = afc_motor.home_position;
    global_data_A37434.manual_target_position = afc_motor.home_position;
    global_data_A37434.afc_hot_position = afc_motor.home_position;
    _STATUS_AFC_AUTO_ZERO_HOME_IN_PROGRESS = 1;
    while (global_data_A37434.control_state == STATE_AUTO_HOME) {
      DoA37434();
      if (afc_motor.current_position == afc_motor.home_position) {
	global_data_A37434.control_state = STATE_RUN_AFC;
      }
    }
    break;
    
  case STATE_RUN_AFC:
    _STATUS_AFC_AUTO_ZERO_HOME_IN_PROGRESS = 0;
    while (global_data_A37434.control_state == STATE_RUN_AFC) {
      DoA37434();
      global_data_A37434.manual_target_position = afc_motor.target_position;
      if (global_data_A37434.sample_complete) {
	DoPostPulseProcess();
	DoAFCReversePower();
      }

      if (_STATUS_AFC_MODE_MANUAL_MODE) {
	global_data_A37434.control_state = STATE_RUN_MANUAL;
      }
    }
    break;
    
    
  case STATE_RUN_MANUAL:
    _STATUS_AFC_AUTO_ZERO_HOME_IN_PROGRESS = 0;
    while (global_data_A37434.control_state == STATE_RUN_MANUAL) {
      DoA37434();
      afc_motor.target_position = global_data_A37434.manual_target_position;
      if (global_data_A37434.sample_complete) {
	DoPostPulseProcess();
      }
      if (!_STATUS_AFC_MODE_MANUAL_MODE) {
	global_data_A37434.control_state = STATE_RUN_AFC;
      }
    }
    break;
    

  default:
    global_data_A37434.control_state = STATE_RUN_AFC;
    break;

  }
}


void InitializeA37434(void) {
  unsigned char aft_control_voltage_cal;
  unsigned char aft_a_sample_cal;
  unsigned char aft_b_sample_cal;
  
  TRISA = A37434_TRISA_VALUE;
  TRISB = A37434_TRISB_VALUE;
  TRISC = A37434_TRISC_VALUE;
  TRISD = A37434_TRISD_VALUE;
  TRISE = A37434_TRISE_VALUE;
  TRISF = A37434_TRISF_VALUE;
  TRISG = A37434_TRISG_VALUE;
  
  PIN_MOTOR_DRV_RESET_NOT = 1;
  PIN_MOTOR_DRV_SLEEP_NOT = 1;
  
  PIN_MOTOR_DRV_ISET_A0   = 0;
  PIN_MOTOR_DRV_ISET_A1   = 0;
  PIN_MOTOR_DRV_ISET_B0   = 0;
  PIN_MOTOR_DRV_ISET_B1   = 0;
  
  PTPER   = PTPER_SETTING;
  PWMCON1 = PWMCON1_SETTING;
  PWMCON2 = PWMCON2_SETTING;
  DTCON1  = DTCON1_SETTING;
  DTCON2  = DTCON2_SETTING;
  FLTACON = FLTACON_SETTING;
  FLTBCON = FLTBCON_SETTING;
  OVDCON  = OVDCON_SETTING;
  PDC1    = PDC1_SETTING;
  PDC2    = PDC2_SETTING;
  PDC3    = PDC3_SETTING;
  PDC4    = PDC4_SETTING;
  PTCON   = PTCON_SETTING;
  
  PR1 = PR1_FAST_SETTING;
  _T1IF = 0;
  _T1IP = 6;
  _T1IE = 1;
  T1CON = T1CON_SETTING;
  
  PR3 = PR3_VALUE_10_MILLISECONDS;
  T3CON = T3CON_VALUE;
  _T3IF = 0;
  
  ADCON2 = ADCON2_SETTING;
  ADCON3 = ADCON3_SETTING;
  ADCHS  = ADCHS_SETTING;
  ADPCFG = ADPCFG_SETTING;
  ADCSSL = ADCSSL_SETTING;
  ADCON1 = ADCON1_SETTING;
  
  _INT1IF = 0;
  _INT1IP = 7;
  _INT1IE = 1;
  _INT1EP = 0;
  
  // Initialize the status register and load the inhibit and fault masks
  _FAULT_REGISTER = 0;
  _CONTROL_REGISTER = 0;
  _WARNING_REGISTER = 0;
  _NOT_LOGGED_REGISTER = 0;
  
  // Initialize the External EEprom
  ETMEEPromUseExternal();
  ETMEEPromConfigureExternalDevice(EEPROM_SIZE_8K_BYTES, FCY_CLK, ETM_I2C_400K_BAUD, EEPROM_I2C_ADDRESS_0, I2C_PORT_1);

  // Initialize the I2C DAC
  SetupMCP4725(&U13_MCP4725, I2C_PORT_1, MCP4725_ADDRESS_A0_0, FCY_CLK, ETM_I2C_400K_BAUD);
  
  // Initialize the SPI Module
  ConfigureSPI(ETM_SPI_PORT_2, ETM_DEFAULT_SPI_CON_VALUE, ETM_DEFAULT_SPI_CON2_VALUE, ETM_DEFAULT_SPI_STAT_VALUE, SPI_CLK_2_MBIT, FCY_CLK);
  
  if (ETMEEPromCheckOK() == 0) {
    // The eeprom is not working
    // Do not load calibration data from the EEPROM
    aft_control_voltage_cal = ANALOG_OUTPUT_NO_CALIBRATION;
    aft_a_sample_cal        = ANALOG_INPUT_NO_CALIBRATION;
    aft_b_sample_cal        = ANALOG_INPUT_NO_CALIBRATION;
  } else {
    aft_control_voltage_cal = ANALOG_OUTPUT_0;
    aft_a_sample_cal        = ANALOG_INPUT_3;
    aft_b_sample_cal        = ANALOG_INPUT_4;
  }

#ifdef __USE_AFT_MODULE
  ETMAnalogInitializeOutput(&global_data_A37434.aft_control_voltage,
			    MACRO_DEC_TO_SCALE_FACTOR_16(3.98799),
			    OFFSET_ZERO,
			    aft_control_voltage_cal,
			    AFT_CONTROL_VOLTAGE_MAX_PROGRAM,
			    AFT_CONTROL_VOLTAGE_MIN_PROGRAM,
			    0);
#endif

  ETMAnalogInitializeInput(&global_data_A37434.reverse_power_sample,
			   MACRO_DEC_TO_SCALE_FACTOR_16(.6250),
			   OFFSET_ZERO,
			   aft_a_sample_cal,
			   NO_OVER_TRIP,
			   NO_UNDER_TRIP,
			   NO_TRIP_SCALE,
			   NO_FLOOR,
			   NO_COUNTER,
			   NO_COUNTER);

  ETMAnalogInitializeInput(&global_data_A37434.forward_power_sample,
			   MACRO_DEC_TO_SCALE_FACTOR_16(.6250),
			   OFFSET_ZERO,
			   aft_b_sample_cal,
			   NO_OVER_TRIP,
			   NO_UNDER_TRIP,
			   NO_TRIP_SCALE,
			   NO_FLOOR,
			   NO_COUNTER,
			   NO_COUNTER);


  // Initialize the Can module
  ETMCanSlaveInitialize(CAN_PORT_1, FCY_CLK, ETM_CAN_ADDR_AFC_CONTROL_BOARD, _PIN_RD10, 4, _PIN_RD10, _PIN_RD9);
  ETMCanSlaveLoadConfiguration(37434, 0, FIRMWARE_AGILE_REV, FIRMWARE_BRANCH, FIRMWARE_MINOR_REV);
}



void DoPostPulseProcess(void) {
  global_data_A37434.sample_complete = 0;
  
  // First - Scale and calibrate the power readings
  // Reverse Power Samples are stored in 100uV Units
  // Then convert to dB
  global_data_A37434.reverse_power_sample.filtered_adc_reading = global_data_A37434.a_adc_reading_external;
  global_data_A37434.forward_power_sample.filtered_adc_reading = global_data_A37434.b_adc_reading_internal;
  ETMAnalogScaleCalibrateADCReading(&global_data_A37434.reverse_power_sample);
  ETMAnalogScaleCalibrateADCReading(&global_data_A37434.forward_power_sample);
  global_data_A37434.reverse_power_db = ConvertToDBMiniCircuit(global_data_A37434.reverse_power_sample.reading_scaled_and_calibrated);
  global_data_A37434.forward_power_db = ConvertToDBMiniCircuit(global_data_A37434.forward_power_sample.reading_scaled_and_calibrated);
  

  ETMCanSlaveSetDebugRegister(0x0, ADCBUF1);
  ETMCanSlaveSetDebugRegister(0x1, ADCBUF0);
  ETMCanSlaveSetDebugRegister(0x2, global_data_A37434.a_adc_reading_internal);
  ETMCanSlaveSetDebugRegister(0x3, global_data_A37434.b_adc_reading_internal);
  ETMCanSlaveSetDebugRegister(0x4, global_data_A37434.a_adc_reading_external);
  ETMCanSlaveSetDebugRegister(0x5, global_data_A37434.b_adc_reading_external);
  ETMCanSlaveSetDebugRegister(0x6, global_data_A37434.reverse_power_db);
  ETMCanSlaveSetDebugRegister(0x7, global_data_A37434.forward_power_db);



  if (ETMCanSlaveGetSyncMsgHighSpeedLogging()) {
    ETMCanSlaveLogPulseData(ETM_CAN_DATA_LOG_REGISTER_AFC_FAST_LOG_0,
			    global_data_A37434.sample_index,
			    global_data_A37434.position_at_trigger,
			    afc_motor.target_position,
			    global_data_A37434.reverse_power_db);

    // DPARKER - Figure out if we need to send this message or if the ECB will be ok with the message not being sent
    ETMCanSlaveLogPulseData(ETM_CAN_DATA_LOG_REGISTER_AFC_FAST_LOG_1,
			    global_data_A37434.sample_index,
			    0x0000,
			    0x0000,
			    0x0000);
  }
}


void DoAFCReversePower(void) {
  unsigned int relative_index;
  unsigned int calculated_move;
  unsigned int previous_direction;
  unsigned int new_direction;
  unsigned int target_delta;
  unsigned int n;

  if (global_data_A37434.position_at_trigger > power_readings.position[power_readings.active_index]) {
    previous_direction = MOVE_UP;
  } else {
    previous_direction = MOVE_DOWN;
  }
  
  power_readings.active_index++;
  power_readings.active_index &= 0x000F;
  
  power_readings.reverse_power[power_readings.active_index] = global_data_A37434.reverse_power_db;
  power_readings.forward_power[power_readings.active_index] = global_data_A37434.forward_power_db;
  power_readings.position[power_readings.active_index]      = global_data_A37434.position_at_trigger;


  relative_index = power_readings.active_index;
  calculated_move = 0;
  for (n=0; n<15; n++) {
    // Need to compare the current data with the 15 prevoius samples
    relative_index = ((relative_index - 1) & 0x000F);
    calculated_move += CalculateDirection(global_data_A37434.position_at_trigger,
					  power_readings.position[relative_index],
					  global_data_A37434.reverse_power_db,
					  power_readings.reverse_power[relative_index]);
  }
  
  if (calculated_move > 15) {
    new_direction = MOVE_DOWN;
    global_data_A37434.no_decision_counter = 0;
  } else if (calculated_move < 15) {
    new_direction = MOVE_UP;
    global_data_A37434.no_decision_counter = 0;
  } else {
    if (global_data_A37434.no_decision_counter < MAX_NO_DECISION_COUNTER) {
      new_direction = previous_direction;  
      global_data_A37434.no_decision_counter++;
    } else {
      global_data_A37434.no_decision_counter = 0;
      if (previous_direction == MOVE_UP) {
	new_direction = MOVE_DOWN;
      } else {
	new_direction = MOVE_UP;
      }
    }
  }
  
  if (new_direction != previous_direction) {
    global_data_A37434.inversion_counter++;
  }

  // We know what direction we are going to move next.
  // Figure out how far and how fast we are going to move

  // Figure out if we need to exit fast afc mode
  if (global_data_A37434.fast_afc_done == 0) {
    if (global_data_A37434.pulses_on_this_run >= MAXIMUM_FAST_MODE_PULSES) {
      global_data_A37434.fast_afc_done = 1;
    }
    
    if ((global_data_A37434.pulses_on_this_run >= MINIMUM_FAST_MODE_PULSES) && (global_data_A37434.inversion_counter >= INVERSIONS_TO_REACH_SLOW_MODE)) {
      global_data_A37434.fast_afc_done = 1;
    }
  }    



  if (global_data_A37434.fast_afc_done == 0) {
    target_delta = FAST_MOVE_TARGET_DELTA;
    PR1 = PR1_FAST_SETTING;
  } else {
    target_delta = SLOW_MOVE_TARGET_DELTA;
    PR1 = PR1_SLOW_SETTING;
  }

  if (new_direction == MOVE_UP) {
    afc_motor.target_position = afc_motor.current_position + target_delta; 
  } else {
    afc_motor.target_position = afc_motor.current_position - target_delta;
  }
}


unsigned int ConvertToDBMiniCircuit(unsigned int adc_reading_100uV) {
  unsigned int value;
  if (adc_reading_100uV > MAX_ADC_READING_100_UV) {
    adc_reading_100uV = MAX_ADC_READING_100_UV;
  }

  if (adc_reading_100uV < MIN_ADC_READING_100_UV) {
    adc_reading_100uV = MIN_ADC_READING_100_UV;
  }
  
  value = MAX_ADC_READING_100_UV - adc_reading_100uV;
  // Divide by 2.5 to get .01 dB units
  value = ETMScaleFactor2(value, MACRO_DEC_TO_CAL_FACTOR_2(.4), 0);

  return value;
}



unsigned int CalculateDirection(unsigned int current_pos, unsigned int previous_pos, unsigned int current_rev_pwr, unsigned int previous_rev_pwr) {
  // we want to count this as a change direction, unless the data strongly says to keep going in the same direction
  
  if ((previous_pos == 0) || (previous_rev_pwr == 0)) {
    // The buffer is empty (or bad reading) and has no data in it so skip this comp.
    return MOVE_NO_DATA;
  }

  if (((current_pos + MINIMUM_POSITION_CHANGE) > previous_pos) && ((current_pos - MINIMUM_POSITION_CHANGE) < previous_pos)) {
    // The positions are very close
    return MOVE_NO_DATA;
  } 
  
  if (((current_rev_pwr + MINIMUM_REV_PWR_CHANGE) > previous_rev_pwr) && ((current_rev_pwr - MINIMUM_REV_PWR_CHANGE) < previous_rev_pwr)) {
    // The reverse power readings are very close
    return MOVE_NO_DATA;
  }
  
  if (current_pos > previous_pos) {
    if (current_rev_pwr < previous_rev_pwr) {
      // We need to go up more
      return MOVE_UP;
    } else {
      // We need to go down
      return MOVE_DOWN;
    }
  } else {
    if (current_rev_pwr < previous_rev_pwr) {
      // We need to go down more
      return MOVE_DOWN;
    } else {
      // We need to go up
      return MOVE_UP;
    }
  }
}




void DoAFCCooldown(void) {
  unsigned int position_difference;
  unsigned int shift_position;

  if (afc_motor.home_position > global_data_A37434.afc_hot_position) {
    position_difference = afc_motor.home_position - global_data_A37434.afc_hot_position;
    shift_position = ETMScaleFactor2(position_difference, CoolDownTable[global_data_A37434.time_off_counter >> 9], 0);
    afc_motor.target_position = afc_motor.home_position - shift_position;
  } else {
    position_difference = global_data_A37434.afc_hot_position - afc_motor.home_position; 
    shift_position = ETMScaleFactor2(position_difference, CoolDownTable[global_data_A37434.time_off_counter >> 9], 0);
    afc_motor.target_position = afc_motor.home_position + shift_position;
  }
}




void DoA37434(void) {
  ETMCanSlaveDoCan();

  if (_T3IF) {
    _T3IF = 0;

    // -------------- Update Logging Data ---------------- //
    slave_board_data.log_data[0] = 0;
    slave_board_data.log_data[1] = afc_motor.target_position;
    slave_board_data.log_data[2] = afc_motor.current_position;
    
    //slave_board_data.log_data[4] = global_data_A37434.aft_filtered_error_for_client;
    //slave_board_data.log_data[5] = global_data_A37434.aft_B_sample_filtered;
    //slave_board_data.log_data[6] = global_data_A37434.aft_A_sample_filtered;
    
    //slave_board_data.log_data[8] = global_data_A37434.aft_control_voltage.set_point;
    slave_board_data.log_data[11] = afc_motor.home_position;
    
    UpdateFaults();

    // Update the "Hot Position" - This is where the motor ended when we stopped pulsing
    if (global_data_A37434.fast_afc_done == 1) {
      global_data_A37434.afc_hot_position = afc_motor.current_position;
    }

    // Update the time_off_counter and run the cooldown if needed
    if (global_data_A37434.time_off_counter < LIMIT_RECORDED_OFF_TIME) {
      global_data_A37434.time_off_counter++;
    }

    if (global_data_A37434.time_off_counter >= NO_PULSE_TIME_TO_INITITATE_COOLDOWN) {
      global_data_A37434.fast_afc_done = 0;
      global_data_A37434.pulses_on_this_run = 0;
      global_data_A37434.inversion_counter = 0;
      // Do not perform the cooldown in manual mode
      if (global_data_A37434.control_state == STATE_RUN_AFC) {
	DoAFCCooldown();	
      }  
    }
    
    /*
    ETMCanSlaveSetDebugRegister(0x0, ADCBUF1);
    ETMCanSlaveSetDebugRegister(0x1, ADCBUF2);
    ETMCanSlaveSetDebugRegister(0x2, ADCBUF9);
    ETMCanSlaveSetDebugRegister(0x3, ADCBUFA);
    ETMCanSlaveSetDebugRegister(0x4, global_data_A37434.aft_A_sample.filtered_adc_reading);
    ETMCanSlaveSetDebugRegister(0x5, global_data_A37434.aft_B_sample.filtered_adc_reading);
    ETMCanSlaveSetDebugRegister(0x6, global_data_A37434.aft_A_sample.reading_scaled_and_calibrated);
    ETMCanSlaveSetDebugRegister(0x7, global_data_A37434.aft_B_sample.reading_scaled_and_calibrated);  
    ETMCanSlaveSetDebugRegister(0x8, global_data_A37434.aft_A_sample.reading_scaled_and_calibrated);
    ETMCanSlaveSetDebugRegister(0x9, global_data_A37434.aft_B_sample.reading_scaled_and_calibrated);
    ETMCanSlaveSetDebugRegister(0xA, global_data_A37434.aft_A_sample_filtered);
    ETMCanSlaveSetDebugRegister(0xB, global_data_A37434.aft_B_sample_filtered);
    */
    
#ifdef __USE_AFT_MODULE
    // update the AFT control voltage
    // DPARKER consider timing this with Magnetron pulses
    /*    
    if (ETMCanSlaveIsNextPulseLevelHigh()) {
      ETMAnalogSetOutput(&global_data_A37434.aft_control_voltage, global_data_A37434.aft_control_voltage_high_energy);
    } else {
      ETMAnalogSetOutput(&global_data_A37434.aft_control_voltage, global_data_A37434.aft_control_voltage_low_energy);
    }
    ETMAnalogScaleCalibrateDACSetting(&global_data_A37434.aft_control_voltage);
    WriteLTC265X(&U23_LTC2654, LTC265X_WRITE_AND_UPDATE_DAC_A, global_data_A37434.aft_control_voltage.dac_setting_scaled_and_calibrated);
    */
#endif

  }
}



void UpdateFaults(void) {
  // Check for Can Faults
  if (ETMCanSlaveGetComFaultStatus()) {
    _FAULT_CAN_COMMUNICATION_LATCHED = 1;
  } else {
    if (ETMCanSlaveGetSyncMsgResetEnable()) {
      _FAULT_CAN_COMMUNICATION_LATCHED = 0;
    }
  }
  
  if (_FAULT_CAN_COMMUNICATION_LATCHED || _STATUS_AFC_AUTO_ZERO_HOME_IN_PROGRESS) {
    _CONTROL_NOT_READY = 1;
  } else {
    _CONTROL_NOT_READY = 0;
  }
}





void __attribute__((interrupt, no_auto_psv)) _INT1Interrupt(void) {
  unsigned long adc_read;

  /* 
     The INT1 Interrupt is read back data from the ADCs (they should have already been sampled)
  */
  
  PIN_TEST_POINT_A = 1;

  __delay32(80);  // wait 8us to pulse to terminate

  global_data_A37434.position_at_trigger = afc_motor.current_position;


  // Wait for completion of the internal ADC
  // while (!_DONE); we don't need to wait because of the 10us delay above
  global_data_A37434.a_adc_reading_internal = ADCBUF1 << 6;
  global_data_A37434.b_adc_reading_internal = ADCBUF0 << 6;
  
  // Read the data from the external ADCs
    
  // Read back the "A" input ADC
  PIN_INPUT_A_CS = OLL_SELECT_ADC; 
  adc_read = SendAndReceiveSPI(0, ETM_SPI_PORT_2);
  if (adc_read == 0x11110000) {
    global_data_A37434.a_adc_reading_external = 0;
  } else {
    global_data_A37434.a_adc_reading_external = adc_read & 0xFFFF;
  }
  PIN_INPUT_A_CS = !OLL_SELECT_ADC;
  
  Nop();
  Nop();

  // Read back the "B" input ADC
  PIN_INPUT_B_CS = OLL_SELECT_ADC;
  adc_read = SendAndReceiveSPI(0, ETM_SPI_PORT_2);
  if (adc_read == 0x11110000) {
    global_data_A37434.b_adc_reading_external = 0;
  } else {
    global_data_A37434.b_adc_reading_external = adc_read & 0xFFFF;
  }
  PIN_INPUT_B_CS = !OLL_SELECT_ADC;  
  
  
  global_data_A37434.pulses_on_this_run++;
  global_data_A37434.time_off_counter = 0;
  global_data_A37434.sample_index = ETMCanSlaveGetPulseCount();
  global_data_A37434.sample_complete = 1;
  PIN_TEST_POINT_A = 0;

  reading_array_external[reading_array_location] = global_data_A37434.b_adc_reading_external;
  reading_array_internal[reading_array_location] = global_data_A37434.b_adc_reading_internal;
  reading_array_location++;
  reading_array_location &= 0b0000001111111111;


  _INT1IF = 0;
}


void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
  /*
    The T1 interrupt controls the motor movent
    The maximum speed of the motor is 1/32 step per _T1 interrupt
    The maximum speed of the motor is set by setting the time of the _T1 interrupt 
  */

  _T1IF = 0;

  // Ensure that the target position is a valid value
  if (afc_motor.target_position > afc_motor.max_position) {
    afc_motor.target_position = afc_motor.max_position;
  }
  if (afc_motor.target_position < afc_motor.min_position) {
    afc_motor.target_position = afc_motor.min_position;
  }
    
  if (afc_motor.current_position > afc_motor.target_position) {
    // Move the motor one position
    afc_motor.time_steps_stopped = 0;
    afc_motor.current_position--;
  } else if (afc_motor.current_position < afc_motor.target_position) {
    // Move the motor one position the other direction
    afc_motor.time_steps_stopped = 0;
    afc_motor.current_position++;
  } else {
    // We are at our target position
    afc_motor.time_steps_stopped++;
  }
  
  if (afc_motor.time_steps_stopped >= DELAY_SWITCH_TO_LOW_POWER_MODE) {
    // use the low power look up table
    afc_motor.time_steps_stopped = DELAY_SWITCH_TO_LOW_POWER_MODE;
    PDC1 = PWMLowPowerTable[ShiftIndex(afc_motor.current_position,0)];
    PDC2 = PWMLowPowerTable[ShiftIndex(afc_motor.current_position,64)];
    PDC3 = PWMLowPowerTable[ShiftIndex(afc_motor.current_position,32)];
    PDC4 = PWMLowPowerTable[ShiftIndex(afc_motor.current_position,96)];        
  } else {
    // use the high power lookup table
    PDC1 = PWMHighPowerTable[ShiftIndex(afc_motor.current_position,0)];
    PDC2 = PWMHighPowerTable[ShiftIndex(afc_motor.current_position,64)];
    PDC3 = PWMHighPowerTable[ShiftIndex(afc_motor.current_position,32)];
    PDC4 = PWMHighPowerTable[ShiftIndex(afc_motor.current_position,96)];        
  }
}

unsigned int ShiftIndex(unsigned int index, unsigned int shift) {
  unsigned int value;
  value = index;
  value &= 0x007F;
  value += shift;
  value &= 0x007F;
  return value;
}





void __attribute__((interrupt, no_auto_psv)) _DefaultInterrupt(void) {
  // Clearly should not get here without a major problem occuring
  // DPARKER do something to save the state into a RAM location that is not re-initialized and then reset
  Nop();
  Nop();
  __asm__ ("Reset");
}




void ETMCanSlaveExecuteCMDBoardSpecific(ETMCanMessage* message_ptr) {
  unsigned int index_word;

  index_word = message_ptr->word3;
  switch (index_word)
    {
      /*
	Place all board specific commands here
      */
    case ETM_CAN_REGISTER_AFC_SET_1_HOME_POSITION_AND_OFFSET:
      afc_motor.home_position = message_ptr->word0;
      /*
      global_data_A37434.aft_control_voltage_low_energy = message_ptr->word1;
      global_data_A37434.aft_control_voltage_high_energy = message_ptr->word2;
      */
      _CONTROL_NOT_CONFIGURED = 0;
      break;

    case ETM_CAN_REGISTER_AFC_CMD_SELECT_AFC_MODE:
      _STATUS_AFC_MODE_MANUAL_MODE = 0;
      break;

    case ETM_CAN_REGISTER_AFC_CMD_SELECT_MANUAL_MODE:
      _STATUS_AFC_MODE_MANUAL_MODE = 1;
      break;

    case ETM_CAN_REGISTER_AFC_CMD_SET_MANUAL_TARGET_POSITION:
      global_data_A37434.manual_target_position = message_ptr->word0;
      break;

    case ETM_CAN_REGISTER_AFC_CMD_RELATIVE_MOVE_MANUAL_TARGET:
      if (message_ptr->word1) {
	// decrease the target position;
	if (global_data_A37434.manual_target_position > message_ptr->word0) {
	  global_data_A37434.manual_target_position -= message_ptr->word0;
	} else {
	  global_data_A37434.manual_target_position = 0;
	}
      } else {
	// increase the target position;
	if ((0xFFFF - message_ptr->word0) > global_data_A37434.manual_target_position) {
	  global_data_A37434.manual_target_position += message_ptr->word0;
	} else {
	  global_data_A37434.manual_target_position = 0xFFFF;
	}
      }
      break;

    default:
      //local_can_errors.invalid_index++;
      break;
    }
}



