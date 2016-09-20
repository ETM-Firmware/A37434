#ifndef __MCP4725_H
#define __MCP4725_H

typedef struct {
  unsigned char address;
  unsigned char i2c_port;
  unsigned int  write_error_count;
} MCP4725;

#define MCP4725_ADDRESS_A0_0    0b11000000
#define MCP4725_ADDRESS_A0_1    0b11000010
#define MCP4725_ADDRESS_A1_0    0b11000100
#define MCP4725_ADDRESS_A1_1    0b11000110
#define MCP4725_ADDRESS_A2_0    0b11001000
#define MCP4725_ADDRESS_A2_1    0b11001010
#define MCP4725_ADDRESS_A3_0    0b11001100
#define MCP4725_ADDRESS_A3_1    0b11001110


void SetupMCP4725(MCP4725* ptr_MCP4725, unsigned char i2c_port, unsigned char address, unsigned long fcy_clk, unsigned long i2c_baud_rate);

void MCP4725UpdateFast(MCP4725* ptr_MCP4725, unsigned int value);

#endif
