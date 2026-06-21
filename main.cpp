#include <stdint.h>
#include "uart.hpp"
#include "tcb.hpp"
#include "scheduler.hpp"

void task_one() {
    int i = 33;
    while(1) {
        uart_putc('A');
    }
}

void task_two() {
    while(1) {
        uart_putc('B');
    }
}

static uint32_t stack_idle[256] __attribute__((aligned(8)));
TCB tcb_idle;

void idle_task() {
    while(1) {
        asm volatile("wfi");
    }
}

static uint32_t stack_one[256] __attribute__((aligned(8)));
static uint32_t stack_two[256] __attribute__((aligned(8)));


int main(){
    configure_uart();

    TCB tcb_one;
    TCB tcb_two;

    task_init(tcb_one,  task_one,  stack_one,  256, 2);  
    task_init(tcb_two,  task_two,  stack_two,  256, 2); 
    task_init(tcb_idle, idle_task, stack_idle, 256, 0);  

    task_register(&tcb_one);
    task_register(&tcb_two);
    task_register(&tcb_idle);

    const char* str = "Hello World!\n";
    for(const char* p = str; *p != '\0'; ++p){
        uart_putc(*p);
    }
 
    current_task = task_list[0];

    systick_init();
  
    scheduler_start();

    while(1) {}
}

