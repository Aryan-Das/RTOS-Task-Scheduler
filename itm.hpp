#ifndef ITM_HPP
#define ITM_HPP

// DEBUG ONLY


#include <stdint.h>

volatile uint32_t* DBGMCU_CR = (volatile uint32_t*)0xE0042004;

volatile uint32_t* TPIU_ACPR = (volatile uint32_t*)0xE0040010;
volatile uint32_t* TPIU_SPPR = (volatile uint32_t*)0xE00400F0;
volatile uint32_t* TPIU_FFCR = (volatile uint32_t*)0xE0040304;

volatile uint32_t* ITM_STIM0 = (volatile uint32_t*)0xE0000000;
volatile uint32_t* ITM_TER = (volatile uint32_t*)0xE0000E00;
volatile uint32_t* ITM_TPR = (volatile uint32_t*)0xE0000E40;
volatile uint32_t* ITM_TCR = (volatile uint32_t*)0xE0000E80;
volatile uint32_t* ITM_LAR = (volatile uint32_t*)0xE0000FB0;

uint32_t core_clock_hz = 168000000;
uint32_t swo_baud_hz = 2000000;

void configure_itm(){
    *DBGMCU_CR |= (1 << 5);

    *ITM_LAR = 0xC5ACCE55;

    *TPIU_SPPR = 2;
    *TPIU_ACPR = (core_clock_hz / swo_baud_hz) - 1;
    *TPIU_FFCR &= ~(1 << 1);

    *ITM_TCR = (1 << 0) | (1 << 3) | (1 << 22);
    *ITM_TPR = 0;
    *ITM_TER |= 1;
}

void itm_putc(char c){
    while (!(*ITM_STIM0 & 1)) {}
    *(volatile uint8_t*)ITM_STIM0 = c;
}

void itm_print_str(const char* c){
    while(*c != '\0'){
        itm_putc(*(c++));
    }
}

void itm_print_num(uint32_t num){
    if (num == 0)
    {
        itm_putc('0');
        return;
    }

    char digits[10];
    int count = 0;

    while (num > 0)
    {
        digits[count++] = '0' + (num % 10);
        num /= 10;
    }

    while (count > 0)
    {
        itm_putc(digits[--count]);
    }
}

void itm_print_float(float f) {
    if (f < 0) {
        itm_putc('-');
        f = -f;
    }
    uint32_t integer = (uint32_t)f;
    uint32_t frac = (uint32_t)((f - integer) * 1000);
    itm_print_num(integer);
    itm_putc('.');

    if (frac < 100) itm_putc('0');
    if (frac < 10) itm_putc('0');
    itm_print_num(frac);
}

#endif