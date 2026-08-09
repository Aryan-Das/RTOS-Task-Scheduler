#ifndef GPIO_CS_HPP
#define GPIO_CS_HPP

#include <stdint.h>


struct CSPin {
    volatile uint32_t* moder;
    volatile uint32_t* odr;
    uint8_t pin;
};

void cs_pin_init(CSPin cs){
    *cs.moder &= ~(0x3 << (cs.pin * 2));
    *cs.moder |=  (0x1 << (cs.pin * 2)); // general purpose output
    *cs.odr   |=  (1 << cs.pin);         // idle high
}

void cs_low(CSPin cs){ *cs.odr &= ~(1 << cs.pin); }
void cs_high(CSPin cs){ *cs.odr |=  (1 << cs.pin); }

#endif