#ifndef canbus__h
#define canbus__h

#define CANSPEED_125 	7		// CAN speed at 125 kbps
#define CANSPEED_250  	3		// CAN speed at 250 kbps
#define CANSPEED_500	1		// CAN speed at 500 kbps
#define CANSPEED_1000	0		// CAN speed at 1000 kbps

#define ENGINE_COOLANT_TEMP 0x05
#define ENGINE_RPM          0x0C
#define VEHICLE_SPEED       0x0D
#define MAF_SENSOR          0x10
#define O2_VOLTAGE          0x14
#define THROTTLE			0x11

#define PID_REQUEST         0x7DF
#define PID_REPLY			0x7E8

#define	P_MOSI		B,3
#define	P_MISO		B,4
#define	P_SCK		B,5
#define	MCP2515_CS	B,2
#define	MCP2515_INT	D,2

#include "global.h"
#include "mcp2515_defs.h"

typedef struct {
	uint16_t id;
	uint32_t eid;
	struct {
		uint8_t extend : 1;
		uint8_t rtr : 1;
		uint8_t length : 4;
	} header;
	uint8_t data[8];
} tCAN;


class CanbusClass {
  public:
	CanbusClass();
    uint8_t init(uint8_t  speed, uint16_t filt);
	uint8_t message_tx(uint8_t *dat, uint8_t len, uint16_t addr);
	uint8_t message_rx(uint8_t *buffer, uint8_t *length);
  private:
	uint8_t spi_putc( uint8_t data );
	void mcp2515_write_register( uint8_t adress, uint8_t data );
	uint8_t mcp2515_read_register(uint8_t adress);
	void mcp2515_bit_modify(uint8_t adress, uint8_t mask, uint8_t data);
	uint8_t mcp2515_read_status(uint8_t type);
	uint8_t mcp2515_check_message(void);
	uint8_t mcp2515_check_free_buffer(void);
	uint8_t mcp2515_get_message(tCAN *message);
	uint8_t mcp2515_send_message(tCAN *message);
};
extern CanbusClass Canbus;

#endif
