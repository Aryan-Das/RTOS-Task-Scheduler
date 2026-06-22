#include <stdint.h>
#include "uart.hpp"
#include "tcb.hpp"
#include "scheduler.hpp"
#include "mutex.hpp"

Mutex uart_mutex;


/* Known limitation: first context switch has a brief race condition 
before scheduler state fully initializes. Subsequent switches are 
correct. Root cause: initial PSP setup doesn't go through full 
PendSV save/restore path. */

void task_one() {
    task_yield();
    task_yield();
    task_yield();
    while(1) {
        mutex_lock(&uart_mutex);
        uart_putc('H');
        uart_putc('e');
        uart_putc('l');
        uart_putc('l');
        uart_putc('o');
        uart_putc('\n');
        mutex_unlock(&uart_mutex);

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
    
    }
}
TCB tcb_idle;
TCB tcb_one;
TCB tcb_two;

void idle_task() {
    while(1) {
        asm volatile("wfi");
    }
}

static uint32_t stack_one[512] __attribute__((aligned(8)));
static uint32_t guard_one[16]; 
static uint32_t stack_two[512] __attribute__((aligned(8)));
static uint32_t guard_two[16];
static uint32_t stack_idle[512] __attribute__((aligned(8)));


int main(){
    configure_uart();

    mutex_init(&uart_mutex);
    task_create(tcb_one,  task_one,  stack_one,  256, 1);
    task_create(tcb_two,  task_two,  stack_two,  256, 1);
    task_create(tcb_idle, idle_task, stack_idle, 256, 0);

    // print uart_mutex address
    uint32_t addr = (uint32_t)&uart_mutex;
    for(int i = 28; i >= 0; i -= 4) {
        uint8_t n = (addr >> i) & 0xF;
        uart_putc(n < 10 ? '0'+n : 'A'+n-10);
    }
    uart_putc('\n');

    // print stack_one range
    uint32_t s1 = (uint32_t)stack_one;
    uint32_t s1top = s1 + 256*4;
    for(int i = 28; i >= 0; i -= 4) {
        uint8_t n = (s1 >> i) & 0xF;
        uart_putc(n < 10 ? '0'+n : 'A'+n-10);
    }
    uart_putc('\n');
    for(int i = 28; i >= 0; i -= 4) {
        uint8_t n = (s1top >> i) & 0xF;
        uart_putc(n < 10 ? '0'+n : 'A'+n-10);
    }
    uart_putc('\n');

    // print stack_two range
    uint32_t s2 = (uint32_t)stack_two;
    uint32_t s2top = s2 + 256*4;
    for(int i = 28; i >= 0; i -= 4) {
        uint8_t n = (s2 >> i) & 0xF;
        uart_putc(n < 10 ? '0'+n : 'A'+n-10);
    }
    uart_putc('\n');
    for(int i = 28; i >= 0; i -= 4) {
        uint8_t n = (s2top >> i) & 0xF;
        uart_putc(n < 10 ? '0'+n : 'A'+n-10);
    }
    uart_putc('\n');

    current_task = task_list[0];
    tcb_one.state = Ready;
    tcb_two.state = Ready;
    tcb_idle.state = Ready;
    current_task = task_list[0];
    current_task->state = Running;
    systick_init();
    scheduler_start();

    while(1) {}
}

