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
#define LIS3DSH_CTRL_REG4 0x20
#define LIS3DSH_OUT_X_L   0x28

#define LIS3DSH_AUTO_INCREMENT 0x40
#define LIS3DSH_SENSITIVITY_2G 0.061f

CSPin lis3dsh_cs; 

struct AccelReading {
    int16_t x;
    int16_t y;
    int16_t z;
};

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

void lis3dsh_write_reg(uint8_t reg, uint8_t value){
    cs_low(lis3dsh_cs);
    spi1_transfer(reg); // bit7=0 for write
    spi1_transfer(value);
    cs_high(lis3dsh_cs);
}

void lis3dsh_enable(){
    // ODR=100Hz, all axes enabled, BDU on 
    // ODR[3:0] | BDU | Zen | Yen | Xen
    lis3dsh_write_reg(LIS3DSH_CTRL_REG4, 0x67); // 0110 0111
}

AccelReading lis3dsh_read_accel(){
    AccelReading result;
    uint8_t xl = lis3dsh_read_reg(0x28);
    uint8_t xh = lis3dsh_read_reg(0x29);
    uint8_t yl = lis3dsh_read_reg(0x2A);
    uint8_t yh = lis3dsh_read_reg(0x2B);
    uint8_t zl = lis3dsh_read_reg(0x2C);
    uint8_t zh = lis3dsh_read_reg(0x2D);

    result.x = (int16_t)((xh << 8) | xl);
    result.y = (int16_t)((yh << 8) | yl);
    result.z = (int16_t)((zh << 8) | zl);
    return result;
}

struct AccelReadingG {
    float x;
    float y;
    float z;
};

AccelReadingG lis3dsh_to_g(AccelReading raw){
    AccelReadingG g;
    g.x = (raw.x * LIS3DSH_SENSITIVITY_2G) / 1000.0f;
    g.y = (raw.y * LIS3DSH_SENSITIVITY_2G) / 1000.0f;
    g.z = (raw.z * LIS3DSH_SENSITIVITY_2G) / 1000.0f;
    return g;
}

#endif