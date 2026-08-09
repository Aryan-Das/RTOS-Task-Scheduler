#ifndef SPI_HPP
#define SPI_HPP

#include <stdint.h>

volatile uint32_t* RCC_APB2ENR_SPI = (volatile uint32_t*)0x40023844;
volatile uint32_t* RCC_AHB1ENR_SPI = (volatile uint32_t*)0x40023830;

volatile uint32_t* GPIOA_MODER_SPI = (volatile uint32_t*)0x40020000;
volatile uint32_t* GPIOA_AFRL_SPI  = (volatile uint32_t*)0x40020020;

volatile uint32_t* SPI1_CR1 = (volatile uint32_t*)0x40013000;
volatile uint32_t* SPI1_CR2 = (volatile uint32_t*)0x40013004;
volatile uint32_t* SPI1_SR  = (volatile uint32_t*)0x40013008;
volatile uint32_t* SPI1_DR  = (volatile uint32_t*)0x4001300C;


void configure_spi1(uint8_t cpol, uint8_t cpha, uint8_t baud_prescaler_bits){
    *RCC_APB2ENR_SPI |= (1 << 12); // SPI1 CLOCK EN
    *RCC_AHB1ENR_SPI |= (1 << 0);  // GPIO CLOCK AEN

    // PA5 (SCK), PA6 (MISO), PA7 (MOSI) -> alternate function mode (10)
    *GPIOA_MODER_SPI &= ~(0x3 << 10);
    *GPIOA_MODER_SPI |=  (0x2 << 10); // PA5
    *GPIOA_MODER_SPI &= ~(0x3 << 12);
    *GPIOA_MODER_SPI |=  (0x2 << 12); // PA6
    *GPIOA_MODER_SPI &= ~(0x3 << 14);
    *GPIOA_MODER_SPI |=  (0x2 << 14); // PA7

    // AF5 = SPI1, on AFRL bits for pins 5,6,7
    *GPIOA_AFRL_SPI &= ~(0xF << 20);
    *GPIOA_AFRL_SPI |=  (0x5 << 20); // PA5
    *GPIOA_AFRL_SPI &= ~(0xF << 24);
    *GPIOA_AFRL_SPI |=  (0x5 << 24); // PA6
    *GPIOA_AFRL_SPI &= ~(0xF << 28);
    *GPIOA_AFRL_SPI |=  (0x5 << 28); // PA7

    *SPI1_CR1 = (1 << 2)                        // MSTR = master
              | (baud_prescaler_bits << 3)       // BR[2:0]
              | (cpol << 1)                      // CPOL
              | (cpha << 0)                      // CPHA
              | (1 << 8)                         // SSI = 1
              | (1 << 9);                        // SSM = software slave management

    *SPI1_CR1 |= (1 << 6); // SPE = SPI enable
}

uint8_t spi1_transfer(uint8_t data){
    while (!(*SPI1_SR & (1 << 1))) {} // wait TXE
    *(volatile uint8_t*)SPI1_DR = data;
    while (!(*SPI1_SR & (1 << 0))) {} // wait RXNE
    return *(volatile uint8_t*)SPI1_DR;
}

#endif