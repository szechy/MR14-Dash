#include <avr/io.h>
#include <avr/interrupt.h>
#include "Tlc5940_Bitbang.h"

#define pulse_pin(port, pin)   port |= _BV(pin); port &= ~_BV(pin)

uint8_t tlc_GSData[NUM_TLCS * 24];

void Tlc5940_Bitbang::clear(void) {
    setAll(0);
}
void Tlc5940_Bitbang::set(TLC_CHANNEL_TYPE channel, uint16_t value) {
    TLC_CHANNEL_TYPE index8 = (NUM_TLCS * 16 - 1) - channel;
    uint8_t *index12p = tlc_GSData + ((((uint16_t)index8) * 3) >> 1);
    if (index8 & 1) { // starts in the middle
                      // first 4 bits intact | 4 top bits of value
        *index12p = (*index12p & 0xF0) | (value >> 8);
                      // 8 lower bits of value
        *(++index12p) = value & 0xFF;
    } else { // starts clean
                      // 8 upper bits of value
        *(index12p++) = value >> 4;
                      // 4 lower bits of value | last 4 bits intact
        *index12p = ((uint8_t)(value << 4)) | (*index12p & 0xF);
    }
}
void Tlc5940_Bitbang::setAll(uint16_t value) {
    uint8_t firstByte = value >> 4;
    uint8_t secondByte = (value << 4) | (value >> 8);
    uint8_t *p = tlc_GSData;
    while (p < tlc_GSData + NUM_TLCS * 24) {
        *p++ = firstByte;
        *p++ = secondByte;
        *p++ = (uint8_t)value;
    }
}
uint16_t Tlc5940_Bitbang::get(TLC_CHANNEL_TYPE channel) {
    TLC_CHANNEL_TYPE index8 = (NUM_TLCS * 16 - 1) - channel;
    uint8_t *index12p = tlc_GSData + ((((uint16_t)index8) * 3) >> 1);
    return (index8 & 1)? // starts in the middle
            (((uint16_t)(*index12p & 15)) << 8) | // upper 4 bits
            *(index12p + 1)                       // lower 8 bits
        : // starts clean
            (((uint16_t)(*index12p)) << 4) | // upper 8 bits
            ((*(index12p + 1) & 0xF0) >> 4); // lower 4 bits
    // that's probably the ugliest ternary operator I've ever created.
}
uint8_t Tlc5940_Bitbang::update(void) {
    pulse_pin(TLC_SCLK_PORT, TLC_SCLK_PIN);
    uint8_t *p = tlc_GSData;
    while (p < tlc_GSData + NUM_TLCS * 24) {
        tlc_shift8(*p++);
        tlc_shift8(*p++);
        tlc_shift8(*p++);
    }
    TLC_BLANK_PORT |=  _BV(TLC_BLANK_PIN); 
    pulse_pin(TLC_XLAT_PORT, TLC_XLAT_PIN);
    TLC_BLANK_PORT &= ~_BV(TLC_BLANK_PIN);

    return 0;
}
void Tlc5940_Bitbang::init(uint16_t initialValue) {
    TLC_XLAT_DDR    |= _BV(TLC_XLAT_PIN);
    TLC_BLANK_PORT  |= _BV(TLC_BLANK_PIN);
    TLC_BLANK_DDR   |= _BV(TLC_BLANK_PIN);
    TLC_GSCLK_DDR   |= _BV(TLC_GSCLK_PIN);
    
    tlc_shift8_init();

    setAll(initialValue);
    update();
    
    TLC_BLANK_PORT |=  _BV(TLC_BLANK_PIN); 
    pulse_pin(TLC_XLAT_PORT, TLC_XLAT_PIN);
    TLC_BLANK_PORT &= ~_BV(TLC_BLANK_PIN);

    /* Timer 2 - GSCLK */
    TCCR2A = _BV(COM2B1)      // set on BOTTOM, clear on OCR2A (non-inverting), output on OC2B
           | _BV(WGM21)       // Fast pwm with OCR2A top
           | _BV(WGM20);      // Fast pwm with OCR2A top
    TCCR2B = _BV(WGM22);      // Fast pwm with OCR2A top
    OCR2B = 0;                // duty factor (as short a pulse as possible)
    OCR2A = TLC_GSCLK_PERIOD; // see tlc_config.h
    
    TCCR2B |= _BV(CS20);      // no prescale, (start pwm output)
    
    /* Timer 1 - BLANK pulses */
    TCNT1  = 0;
    TCCR1A = 0;
    TCCR1B = _BV(WGM12)       // CTC
           | _BV(CS10);       // clk/1 (No prescaling)
    OCR1A = TLC_PWM_PERIOD;

    TIMSK1 |= _BV(OCIE1A);    // Timer/Counter1, Output Compare A Match Interrupt Enable
}
void tlc_shift8_init(void) {
    TLC_SIN_DDR   |=  _BV(TLC_SIN_PIN);   // SIN  as output
    TLC_SCLK_DDR  |=  _BV(TLC_SCLK_PIN);  // SCLK as output
    TLC_SCLK_PORT &= ~_BV(TLC_SCLK_PIN);
}
void tlc_shift8(uint8_t byte) { /** Shifts a byte out, MSB first */
    for (uint8_t bit = 0x80; bit; bit >>= 1) {
        if (bit & byte) TLC_SIN_PORT |=  _BV(TLC_SIN_PIN);
        else            TLC_SIN_PORT &= ~_BV(TLC_SIN_PIN);
        pulse_pin(TLC_SCLK_PORT, TLC_SCLK_PIN);
    }
}
/** Preinstantiated Tlc variable. */
Tlc5940_Bitbang Tlc;

ISR(TIMER1_COMPA_vect){
    pulse_pin(TLC_BLANK_PORT, TLC_BLANK_PIN);
}