

#ifndef TLC5940_BITBANG_H
#define TLC5940_BITBANG_H

#include <stdint.h>

#ifndef MAXBRIGHT
	#define MAXBRIGHT         4095
#endif
#ifndef NUM_TLCS
	#define NUM_TLCS          1
#endif

#define TLC_PWM_PERIOD    8192
#define TLC_GSCLK_PERIOD  3
#define TLC_CHANNEL_TYPE  uint8_t     // uint16_t if more than 16 TLCs chained together 
                                      
#define TLC_GSCLK_PIN     PORTD3      // GSCLK (TLC pin 18)
#define TLC_GSCLK_PORT    PORTD
#define TLC_GSCLK_DDR     DDRD

#define TLC_BLANK_PIN     PORTD5      // BLANK (TLC pin 23)
#define TLC_BLANK_PORT    PORTD
#define TLC_BLANK_DDR     DDRD

#define TLC_XLAT_PIN      PORTB1      // XLAT  (TLC pin 24)
#define TLC_XLAT_PORT     PORTB
#define TLC_XLAT_DDR      DDRB

#define TLC_SCLK_PIN      PORTD7      // SCLK  (TLC pin 25)
#define TLC_SCLK_PORT     PORTD
#define TLC_SCLK_DDR      DDRD

#define TLC_SIN_PIN       PORTD4      // SIN   (TLC pin 26)
#define TLC_SIN_PORT      PORTD
#define TLC_SIN_DDR       DDRD

extern uint8_t tlc_GSData[NUM_TLCS * 24];

class Tlc5940_Bitbang {
  public:
    void init(uint16_t initialValue = 0);
    void clear(void);
    uint8_t update(void);
    void set(TLC_CHANNEL_TYPE channel, uint16_t value);
    uint16_t get(TLC_CHANNEL_TYPE channel);
    void setAll(uint16_t value);
};

void tlc_shift8_init(void);
void tlc_shift8(uint8_t byte);

extern Tlc5940_Bitbang Tlc;
#endif