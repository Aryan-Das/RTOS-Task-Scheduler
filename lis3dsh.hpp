#ifndef LIS3DSH_HPP
#define LIS3DSH_HPP

#include <stdint.h>
#include "spi.hpp"
#include "gpio_cs.hpp"

volatile uint32_t* RCC_AHB1ENR_E = (volatile uint32_t*)0x40023830;
volatile uint32_t* GPIOE_MODER   = (volatile uint32_t*)0x40021000;
volatile uint32_t* GPIOE_ODR     = (volatile uint32_t*)0x40021014;

#define LIS3DSH_WHO_AM_I 0x0F
#define LIS3DSH_READ_BIT 0x80 

CSPin lis3dsh_cs; 

void configure_lis3dsh(){
    lis3dsh_cs = { GPIOE_MODER, GPIOE_ODR, 3 }; 

    *RCC_AHB1ENR_E |= (1 << 4); // GPIOEEN
    cs_pin_init(lis3dsh_cs);

    configure_spi1(1, 1, 3);
}
uint8_t lis3dsh_read_reg(uint8_t reg){
    cs_low(lis3dsh_cs);
    spi1_transfer(reg | LIS3DSH_READ_BIT);
    uint8_t value = spi1_transfer(0x00);
    cs_high(lis3dsh_cs);
    return value;
}

#endif