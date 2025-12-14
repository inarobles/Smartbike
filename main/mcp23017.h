#ifndef MCP23017_H
#define MCP23017_H

#include "driver/i2c_master.h"
#include "esp_err.h"

// MCP23017 Registers (IOCON.BANK=0)
#define MCP_IODIRA   0x00
#define MCP_IODIRB   0x01
#define MCP_IPOLA    0x02
#define MCP_IPOLB    0x03
#define MCP_GPINTENA 0x04
#define MCP_GPINTENB 0x05
#define MCP_DEFVALA  0x06
#define MCP_DEFVALB  0x07
#define MCP_INTCONA  0x08
#define MCP_INTCONB  0x09
#define MCP_IOCON    0x0A
// Register 0x0B is mirrored IOCON
#define MCP_GPPUA    0x0C
#define MCP_GPPUB    0x0D
#define MCP_INTFA    0x0E
#define MCP_INTFB    0x0F
#define MCP_INTCAPA  0x10
#define MCP_INTCAPB  0x11
#define MCP_GPIOA    0x12
#define MCP_GPIOB    0x13
#define MCP_OLATA    0x14
#define MCP_OLATB    0x15

#define MCP_ADDR     0x20 // A0=A1=A2=0 (Grounded)

esp_err_t mcp23017_init(i2c_master_bus_handle_t bus_handle);
esp_err_t mcp23017_read_ports(uint8_t *port_a, uint8_t *port_b);

#endif
