#include <stdint.h>
#include "uart.hpp"
#include "tcb.hpp"
#include "scheduler.hpp"
#include "mutex.hpp"

Mutex uart_mutex;

void task_one() {
    while(1) {
        mutex_lock(&uart_mutex);
        uart_putc('H');
        uart_putc('e');
        uart_putc('l');
        uart_putc('l');
        uart_putc('o');
        uart_putc('\n');
        mutex_unlock(&uart_mutex);
        task_sleep(200);
    }
}

void task_two() {
    while(1) {
        mutex_lock(&uart_mutex);
        uart_putc('W');
        uart_putc('o');
        uart_putc('r');
        uart_putc('l');
        uart_putc('d');
        uart_putc('\n');
        mutex_unlock(&uart_mutex);
        task_sleep(50);
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
    mutex_init(&uart_mutex);
    task_create(tcb_one,  task_one,  stack_one,  256, 1);
    task_create(tcb_two,  task_two,  stack_two,  256, 1);
    task_create(tcb_idle, idle_task, stack_idle, 256, 0);

    const char* str = "Hello World!\n";
    for(const char* p = str; *p != '\0'; ++p){
        uart_putc(*p);
    }
    
    current_task = task_list[0];

    systick_init();
  
    scheduler_start();

    while(1) {}
}

