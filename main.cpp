#include <stdint.h>
#include "uart.hpp"
#include "tcb.hpp"
#include "scheduler.hpp"
#include "mutex.hpp"
#include "semaphore.hpp"

Mutex uart_mutex;


/* Known limitation: first context switch has a brief race condition 
before scheduler state fully initializes. Subsequent switches are 
correct. Root cause: initial PSP setup doesn't go through full 
PendSV save/restore path. */
Semaphore data_ready;
int shared_data = 0;

void producer() {
    task_yield(); task_yield(); task_yield();
    while(1) {
        shared_data++;
        uart_putc('P');
        uart_putc('0' + (shared_data % 10));
        uart_putc('\n');
        sem_post(&data_ready);
        task_sleep(1000);
        
    }
}

void consumer() {
    task_yield(); task_yield(); task_yield();
    while(1) {
        sem_wait(&data_ready); 
        uart_putc('C'); 
        uart_putc('0' + (shared_data % 10));
        uart_putc('\n');
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
    sem_init(&data_ready, 0, 8);  
    task_create(tcb_one,  producer, stack_one,  512, 1);
    task_create(tcb_two,  consumer, stack_two,  512, 1);
    task_create(tcb_idle, idle_task, stack_idle, 512, 0);

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

