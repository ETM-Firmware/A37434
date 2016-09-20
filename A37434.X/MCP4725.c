#include "MCP4725.h"
#include "ETM_I2C.h"


#define MCP4725_POWER_DOWN_OFF  0b00000000
#define MCP4725_POWER_DOWN_1K   0b00010000
#define MCP4725_POWER_DOWN_10K  0b00100000
#define MCP4725_POWER_DOWN_100K 0b00110000

void SetupMCP4725(MCP4725* ptr_MCP4725, unsigned char i2c_port, unsigned char address, unsigned long fcy_clk, unsigned long i2c_baud_rate) {    
  ptr_MCP4725->i2c_port = i2c_port;
  ptr_MCP4725->address  = 0;
  ConfigureI2C(ptr_MCP4725->i2c_port, I2CCON_DEFAULT_SETUP_PIC30F, i2c_baud_rate, fcy_clk, 0);
  MCP4725UpdateFast(ptr_MCP4725,0);
}


void MCP4725UpdateFast(MCP4725* ptr_MCP4725, unsigned int value) {
  unsigned int error_check;
  unsigned char dataHB;
  unsigned char dataLB;

  dataLB = (value & 0x00FF);
  dataHB = ((value >> 8) & 0x00FF);
  dataHB |= MCP4725_POWER_DOWN_OFF;
  error_check = WaitForI2CBusIdle(ptr_MCP4725->i2c_port); 
  error_check |= GenerateI2CStart(ptr_MCP4725->i2c_port); 
  error_check |= WriteByteI2C((ptr_MCP4725->address | I2C_WRITE_CONTROL_BIT),ptr_MCP4725->i2c_port);
  error_check |= WriteByteI2C(dataHB,ptr_MCP4725->i2c_port);
  error_check |= WriteByteI2C(dataLB,ptr_MCP4725->i2c_port);
  error_check |= GenerateI2CStop(ptr_MCP4725->i2c_port);
  if (error_check != 0) {
    ptr_MCP4725->write_error_count = ptr_MCP4725->write_error_count+1;
  }
}
