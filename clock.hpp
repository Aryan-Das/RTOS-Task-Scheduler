#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <stdint.h>

volatile uint32_t* RCC_CR    = (volatile uint32_t*)0x40023800;
volatile uint32_t* RCC_PLLCFGR = (volatile uint32_t*)0x40023804;
volatile uint32_t* RCC_CFGR  = (volatile uint32_t*)0x40023808;
volatile uint32_t* FLASH_ACR = (volatile uint32_t*)0x40023C00;

void configure_clock_168mhz() {
    *RCC_CR |= (1 << 16); // turn on hse (HSEON bit)
    while (!(*RCC_CR & (1 << 17))) {} // wait for HSERDY

    // set flash wait states
    *FLASH_ACR = (1 << 8) | (1 << 9) | (1 << 10) | 5; 

    // configure PLL M, N, P, Q, and source value
    *RCC_PLLCFGR = (8)                // PLLM  [5:0]
                 | (336 << 6)         // PLLN  [14:6]
                 | (0 << 16)          // PLLP  [17:16] = 00 → /2
                 | (1 << 22)          // PLLSRC = HSE
                 | (7 << 24);

    // enable PLL and wait for PLLREADY
    *RCC_CR |= (1 << 24); 
    while (!(*RCC_CR & (1 << 25))) {} 

    // Set prescaler values
    //    AHB = /1 (168MHz), APB1 max 42MHz so /4, APB2 max 84MHz so /2
    *RCC_CFGR &= ~(0xF << 4);   // clear HPRE
    *RCC_CFGR |= (0 << 4);      // AHB 

    *RCC_CFGR &= ~(0x7 << 10);  // clear PPRE1
    *RCC_CFGR |= (5 << 10);     // APB1

    *RCC_CFGR &= ~(0x7 << 13);  // clear PPRE2
    *RCC_CFGR |= (4 << 13);     // APB2


    // switch SYSCLK source
    *RCC_CFGR &= ~(0x3);        // clear SW bits
    *RCC_CFGR |= 0x2;           // SW = PLL (0b10)

    // wait for SWS to confirm PLL is actually driving SYSCLK
    while (((*RCC_CFGR >> 2) & 0x3) != 0x2) {}
}


#endif