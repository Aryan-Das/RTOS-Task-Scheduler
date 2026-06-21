#include <stdint.h>
#include "uart.hpp"
#include "tcb.hpp"
#include "scheduler.hpp"

void task_one() {
    while(1) {
        uart_putc('A');
    }
}

void task_two() {
    while(1) {
        uart_putc('B');
    }
}

static uint32_t stack_one[256];
static uint32_t stack_two[256];


int main(){
    configure_uart();

    TCB tcb_one;
    TCB tcb_two;
    task_init(tcb_one, task_one, stack_one, 256, 1);
    task_init(tcb_two, task_two, stack_two, 256, 1);

    
    const char* str = "Hello World!\n";
    for(const char* p = str; *p != '\0'; ++p){
        uart_putc(*p);
    }
}

