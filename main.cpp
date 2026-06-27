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

Mutex mutex_a;
Mutex mutex_b;

void task_a() {
    task_yield(); task_yield(); task_yield();
    while(1) {
        mutex_lock(&mutex_a);
        uart_putc('A'); uart_putc('1'); uart_putc('\n');
        task_sleep(10);  // small delay to let task_b acquire mutex_b
        mutex_lock(&mutex_b);  // will block forever
        uart_putc('A'); uart_putc('2'); uart_putc('\n');
        mutex_unlock(&mutex_b);
        mutex_unlock(&mutex_a);
    }
}

void task_b() {
    task_yield(); task_yield(); task_yield();
    while(1) {
        mutex_lock(&mutex_b);
        uart_putc('B'); uart_putc('1'); uart_putc('\n');
        task_sleep(10);  // small delay to let task_a acquire mutex_a
        mutex_lock(&mutex_a);  // will block forever
        uart_putc('B'); uart_putc('2'); uart_putc('\n');
        mutex_unlock(&mutex_a);
        mutex_unlock(&mutex_b);
    }
}



void deadlock_tracker() {
    task_yield(); task_yield(); task_yield();
    while(1) {
        task_sleep(500);  
       
        for(int i = 0; i < task_count; i++) {
            if(task_list[i]->state == Blocked) {
                uart_putc('D'); 
                uart_putc('!');
                uart_putc('\n');
                return; 
            }
        }
    }
}

TCB tcb_idle;
TCB tcb_one;
TCB tcb_two;
TCB tcb_deadlock_tracker;

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
static uint32_t guard_three[16];
static uint32_t stack_deadlock[512] __attribute__((aligned(8)));

int main(){
    configure_uart();
    
    mutex_init(&uart_mutex);
    sem_init(&data_ready, 0, 8);  
    task_create(tcb_one,  task_a, stack_one,  512, 1);
    task_create(tcb_two,  task_b, stack_two,  512, 1);
    task_create(tcb_idle, idle_task, stack_idle, 512, 0);
    task_create(tcb_deadlock_tracker, deadlock_tracker, stack_deadlock, 512, 1);

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

