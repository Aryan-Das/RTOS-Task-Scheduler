#include <stdint.h>


volatile uint32_t* RCC_APB2ENR = (volatile uint32_t*)0x40023844;
volatile uint32_t* RCC_AHB1ENR = (volatile uint32_t*)0x40023830;
volatile uint32_t* GPIOA_MODER = (volatile uint32_t*)0x40020000;
volatile uint32_t* GPIOA_AFRH = (volatile uint32_t*)0x40020024;
volatile uint32_t* USART1_BRR = (volatile uint32_t*)0x40011008;
volatile uint32_t* USART1_CR1 = (volatile uint32_t*)0x4001100C;
volatile uint32_t* USART1_SR = (volatile uint32_t*)0x40011000;
volatile uint32_t* USART1_DR = (volatile uint32_t*)0x40011004;


void configure_uart(){
    *RCC_APB2ENR |= (1 << 4);
    *RCC_AHB1ENR |= 1;
    *GPIOA_MODER |= (1 << 19);
    *GPIOA_MODER &= ~(1 << 18);
    *GPIOA_AFRH &= ~(1 << 7);
    *GPIOA_AFRH |= (1 << 6);
    *GPIOA_AFRH |= (1 << 5);
    *GPIOA_AFRH |= (1 << 4);
    *USART1_BRR = 0x683;
    *USART1_CR1 |= (1 << 3);
    *USART1_CR1 |= (1 << 13);
}
void uart_putc(char c){
    while (!(*USART1_SR & (1 << 7))) {}
    *USART1_DR = c;
}



int main(){
    const char* str = "Hello World!\n";
    for(const char* p = str; *p != '\0'; ++p){
        uart_putc(*p);
    }
}
