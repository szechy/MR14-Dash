#if ARDUINO>=100
#include <Arduino.h> // Arduino 1.0
#else
#include <Wprogram.h> // Arduino 0022 and earlier
#endif
#include <stdint.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "pins_arduino.h"
#include <inttypes.h>

#include "Canbus.h"

CanbusClass::CanbusClass() {
}
uint8_t CanbusClass::message_rx(uint8_t *buffer, uint8_t *length) {
	tCAN message;
	if (mcp2515_check_message()) {
		// Read the message from the MCP2515 buffer
		if (mcp2515_get_message(&message)) {
			*length  = message.header.length;
			for(int i = 0; i<*length; i++) buffer[i] = message.data[i];
		}
		else return 0; //	PRINT("Can not read the message\n\n");
	}
	else return 0; //	PRINT("Can not read the message\n\n");
	return 1;
}
uint8_t CanbusClass::message_tx(uint8_t *dat, uint8_t len, uint16_t addr) {
	tCAN message;

	message.id  = addr;
	message.eid = 0x12345;  // not used so it does not matter
	message.header.extend = 0;
	message.header.rtr = 0;
	message.header.length = len;
	for(int i = 0; i<len; i++) message.data[i] = dat[i];	

	mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), 0);
		
	if (mcp2515_send_message(&message)) return 1;
	else return 0;	//	Error: could not read the message
}
uint8_t CanbusClass::init(uint8_t speed, uint16_t filt) {
	SET(MCP2515_CS);
	SET_OUTPUT(MCP2515_CS);
	
	CLEAR(P_SCK);
	CLEAR(P_MOSI);
	CLEAR(P_MISO);
	
	SET_OUTPUT(P_SCK);
	SET_OUTPUT(P_MOSI);
	SET_INPUT(P_MISO);
	
	SET_INPUT(MCP2515_INT);
	SET(MCP2515_INT);
	
	// active SPI master interface
	SPCR = (1<<SPE)|(1<<MSTR) | (0<<SPR1)|(1<<SPR0);
	SPSR = 0;
	
	// reset MCP2515 by software reset.
	// After this he is in configuration mode.
	CLEAR(MCP2515_CS);
	spi_putc(SPI_RESET);
	SET(MCP2515_CS);
	_delay_us(10);  // wait for the MCP2515 to restart
	
	// load CNF1..3 Register
	mcp2515_write_register(CNF1, speed);
	mcp2515_write_register(CNF2, (1<<BTLMODE)|(1<<PHSEG11));
	mcp2515_write_register(CNF3, (1<<PHSEG21));
  
	// activate interrupts
	mcp2515_write_register(CANINTE, (1<<RX1IE)|(1<<RX0IE));
	
	// test if we could read back the value => is the chip accessible?
	if (mcp2515_read_register(CNF1) != speed) return false;
	
	// deactivate the RXnBF Pins (High Impedance State)
	mcp2515_write_register(BFPCTRL, 0);
	
	// set TXnRTS as inputs
	mcp2515_write_register(TXRTSCTRL, 0);
	
	
	// turn on standard masks (no messages will get through until otherwise set up
	mcp2515_write_register(RXM0SIDH, 0xFF);
	mcp2515_write_register(RXM0SIDL, 0xE0);
	mcp2515_write_register(RXM1SIDH, 0xFF);
	mcp2515_write_register(RXM1SIDL, 0xE0);
	
	//uint16_t filt = 0x773;  // ID for angle
	//uint16_t filt = 0x777;  // ID for gear
	
	uint8_t fH = (uint8_t)(filt>>3);
	uint8_t fL = (uint8_t)((filt<<5) & 0xE0);
	
	//fL |= ((1<<EXIDE) | (0x03));  // for extended identifiers
	
	mcp2515_write_register(RXF0SIDH, fH);
	mcp2515_write_register(RXF0SIDL, fL);
	
	
	// Set Extended mask on
//	mcp2515_bit_modify(RXM0SIDL, 0x02, 1);
//	mcp2515_bit_modify(RXM1SIDL, 0x02, 1);
//	mcp2515_rite_register(RXM0EID0, 0xFF);
//	mcp2515_write_register(RXM1EID8, 0xFF);
//	mcp2515_write_register(RXM1EID0, 0xFF);
	
	
	
	// Set Extended filter 0
	mcp2515_write_register(RXF0EID8, 0x01);
	mcp2515_write_register(RXF0EID0, 0x2A);
	
	
	// reset device to normal mode
	mcp2515_write_register(CANCTRL, 0);
	return true;
}

uint8_t CanbusClass::spi_putc( uint8_t data ) {
	SPDR = data;  // put byte in send-buffer, initiates transmission
	while(!(SPSR&(1<<SPIF)));
	return SPDR;
}
void CanbusClass::mcp2515_write_register( uint8_t adress, uint8_t data ) {
	CLEAR(MCP2515_CS);
	spi_putc(SPI_WRITE);
	spi_putc(adress);
	spi_putc(data);
	SET(MCP2515_CS);
}
uint8_t CanbusClass::mcp2515_read_register(uint8_t adress) {
	uint8_t data;
	CLEAR(MCP2515_CS);
	spi_putc(SPI_READ);
	spi_putc(adress);
	data = spi_putc(0xff);	
	SET(MCP2515_CS);
	return data;
}
void CanbusClass::mcp2515_bit_modify(uint8_t adress, uint8_t mask, uint8_t data) {
	CLEAR(MCP2515_CS);
	spi_putc(SPI_BIT_MODIFY);
	spi_putc(adress);
	spi_putc(mask);
	spi_putc(data);
	SET(MCP2515_CS);
}
uint8_t CanbusClass::mcp2515_read_status(uint8_t type) {
	uint8_t data;
	CLEAR(MCP2515_CS);
	spi_putc(type);
	data = spi_putc(0xff);
	SET(MCP2515_CS);
	return data;
}
uint8_t CanbusClass::mcp2515_check_message(void) {
	return (!IS_SET(MCP2515_INT));
}
uint8_t CanbusClass::mcp2515_check_free_buffer(void) {
	if ((mcp2515_read_status(SPI_READ_STATUS) & 0x54) == 0x54)
		return false; // all buffers used
	return true;
}
uint8_t CanbusClass::mcp2515_get_message(tCAN *message) {
	// read status
	uint8_t status = mcp2515_read_status(SPI_RX_STATUS);
	uint8_t addr;
	uint8_t t;
	uint8_t temp;
	if      (bit_is_set(status,6)) addr = SPI_READ_RX;        // message in buffer 0
	else if (bit_is_set(status,7)) addr = SPI_READ_RX | 0x04; // message in buffer 1
	else return 0; // Error: no message available

	CLEAR(MCP2515_CS);
	spi_putc(addr);
	
	// read id
	message->id  = (uint16_t) spi_putc(0xff) << 3;
	temp = spi_putc(0xff);
	message->id |=  temp >> 5;
	if(temp & (1<<IDE)) { // This is an extended frame
		message->header.extend = 1;
		message->eid = ((uint32_t)temp<<16&0x30000);
	} else message->header.extend = 0;
	temp = spi_putc(0xff);
	message->eid |= (uint32_t)(temp<<8); // read extended identifier
	message->eid |= spi_putc(0xff);
	
	uint8_t length = spi_putc(0xff) & 0x0f;  // read Data Length Code
	
	message->header.length = length;
	message->header.rtr = (bit_is_set(status, 3)) ? 1 : 0;
	
	// read data
	for (t=0;t<length;t++) message->data[t] = spi_putc(0xff);
	SET(MCP2515_CS);
	
	// clear interrupt flag
	if (bit_is_set(status, 6)) mcp2515_bit_modify(CANINTF, (1<<RX0IF), 0);
	else mcp2515_bit_modify(CANINTF, (1<<RX1IF), 0);
	
	return (status & 0x07) + 1;
}
uint8_t CanbusClass::mcp2515_send_message(tCAN *message) {
	uint8_t status = mcp2515_read_status(SPI_READ_STATUS);
	
	/* Statusbyte:
	 *
	 * Bit	Function
	 *  2	TXB0CNTRL.TXREQ
	 *  4	TXB1CNTRL.TXREQ
	 *  6	TXB2CNTRL.TXREQ
	 */
	uint8_t address;
	uint8_t t;
	if      (bit_is_clear(status, 2)) address = 0x00;
	else if (bit_is_clear(status, 4)) address = 0x02;
	else if (bit_is_clear(status, 6)) address = 0x04;
	else return 0;  // all buffer used => could not send message
	
	uint8_t sidh = message->id >> 3;
	uint8_t sidl = (message->id << 5);
	
	// if the extend bit is true, then add in the extended format
	if(message->header.extend) sidl |= ( (1<<EXIDE) | ((message->eid >> 16) & 0x03));

	CLEAR(MCP2515_CS);
	spi_putc(SPI_WRITE_TX | address);
	
	spi_putc(sidh);  // standard identifier
    spi_putc(sidl);
	
	spi_putc(message->eid >> 8 & 0xFF);
	spi_putc(message->eid      & 0xFF);
	
	uint8_t length = message->header.length & 0x0f;
	
	if (message->header.rtr) // a rtr-frame has a length, but contains no data
		spi_putc((1<<RTR) | length);
	else { 
		spi_putc(length);  // set message length
		for (t=0;t<length;t++) spi_putc(message->data[t]);  // data
	}
	
	SET(MCP2515_CS);
	_delay_us(1);
	
	// send message
	CLEAR(MCP2515_CS);
	address = (address == 0) ? 1 : address;
	spi_putc(SPI_RTS | address);
	SET(MCP2515_CS);
	
	return address;
}

CanbusClass Canbus;