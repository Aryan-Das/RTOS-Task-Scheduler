#ifndef UART2_HPP
#define UART2_HPP

#include <stdint.h>



volatile uint32_t* RCC_APB1ENR = (volatile uint32_t*)0x40023840;
volatile uint32_t* RCC_AHB1ENR_2 = (volatile uint32_t*)0x40023830; 
volatile uint32_t* GPIOA_MODER_2 = (volatile uint32_t*)0x40020000; 
volatile uint32_t* GPIOA_AFRL = (volatile uint32_t*)0x40020020;    
volatile uint32_t* USART2_BRR = (volatile uint32_t*)0x40004408;
volatile uint32_t* USART2_CR1 = (volatile uint32_t*)0x4000440C;
volatile uint32_t* USART2_SR  = (volatile uint32_t*)0x40004400;
volatile uint32_t* USART2_DR  = (volatile uint32_t*)0x40004404;

void configure_uart2(){
    *RCC_APB1ENR |= (1 << 17);      
    *RCC_AHB1ENR_2 |= 1;            

    
    *GPIOA_MODER_2 &= ~(1 << 4);
    *GPIOA_MODER_2 |= (1 << 5);     
    *GPIOA_MODER_2 &= ~(1 << 6);
    *GPIOA_MODER_2 |= (1 << 7);     

    *GPIOA_AFRL &= ~(0xF << 8);
    *GPIOA_AFRL |= (0x7 << 8);      
    *GPIOA_AFRL &= ~(0xF << 12);
    *GPIOA_AFRL |= (0x7 << 12);    

    *USART2_BRR = 0x683;            
                                   
    *USART2_CR1 |= (1 << 3);        
    *USART2_CR1 |= (1 << 13);       
}

void uart2_putc(char c){
    while (!(*USART2_SR & (1 << 7))) {}
    *USART2_DR = c;
}

void uart2_print_str(const char* c){
    while(*c != '\0'){
        uart2_putc(*(c++));
    }
}

void uart2_print_num(uint32_t num){
    if (num == 0)
    {
        uart2_putc('0');
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
        uart2_putc(digits[--count]);
    }
}

void uart2_print_float(float f) {
    if (f < 0) {
        uart2_putc('-');
        f = -f;
    }
    uint32_t integer = (uint32_t)f;
    uint32_t frac = (uint32_t)((f - integer) * 1000);
    uart2_print_num(integer);
    uart2_putc('.');

    if (frac < 100) uart2_putc('0');
    if (frac < 10) uart2_putc('0');
    uart2_print_num(frac);
}

#endif