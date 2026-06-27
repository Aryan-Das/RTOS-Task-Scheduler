#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <stdint.h>
#include <stddef.h>
#include "tcb.hpp"
#include "uart.hpp"

volatile uint32_t* SYST_CSR = (volatile uint32_t*)0xE000E010;
volatile uint32_t* SYST_RVR = (volatile uint32_t*)0xE000E014;
volatile uint32_t* SYST_CVR = (volatile uint32_t*)0xE000E018;
volatile uint32_t* SCB_SHPR3 = (volatile uint32_t*)0xE000ED20;
volatile uint32_t* ICSR = (volatile uint32_t*)0xE000ED04;

uint32_t current_tick = 0;

TCB* current_task = nullptr;


const int MAX_TASKS = 8;
TCB* task_list[MAX_TASKS];
int task_count = 0;
int current_index = 0;

extern "C" void scheduler_start();



bool scheduler_running = false;


void task_register(TCB* tcb) {
    task_list[task_count++] = tcb;
}
extern "C" void task_exit_error() {
    while(1) {}
}
extern "C" void schedule() {
    
    if (!scheduler_running) {
        scheduler_running = true;
        current_task = task_list[0];
        current_task->state = Running; 
        return;
    }
    
    if (current_task->state == Running) {
        current_task->state = Ready;
    }

    int highest_priority = -1;
    int highest_priority_index = 0;
    for(int i = 0; i < task_count; i++){
        int idx = (current_index + 1 + i) % task_count;
        if(task_list[idx]->state == Ready && task_list[idx]->priority > highest_priority){
            highest_priority = task_list[idx]->priority;
            highest_priority_index = idx;
        }
    }
    current_index = highest_priority_index;
    current_task = task_list[current_index];
    current_task->state = Running;
    //uart_putc('0' + current_index);
}


extern "C" void HardFault_handler() {
    uart_putc('!');
    volatile uint32_t* msp;
    asm volatile("mrs %0, msp" : "=r"(msp));
    uint32_t pc = msp[6];
    for(int i = 28; i >= 0; i -= 4) {
        uint8_t n = (pc >> i) & 0xF;
        uart_putc(n < 10 ? '0'+n : 'A'+n-10);
    }
    uart_putc('\n');
    while(1) {}
}

extern "C" void SysTick_handler() {
    current_tick += 1;
    for(int i = 0; i < task_count; i++) {
        if(task_list[i]->state == Sleeping && 
        (int32_t)(current_tick - task_list[i]->sleep_until) >= 0) {
            task_list[i]->state = Ready;
        }
    }   
    
    *ICSR = (1 << 28);
}

void systick_init(){
    *SCB_SHPR3 |= (0xFF << 16); //set PendSV to lowest priority
    *SYST_RVR = 16000; //set timer to countdown from 16,000
    *SYST_CVR = 0;
    *SYST_CSR = (1 << 2) | (1 << 1) | (1 << 0);
}

// API Functions

void task_yield() {
    *ICSR = (1 << 28);  // pend PendSV
}
void task_sleep(uint32_t ms){
    current_task->state = Sleeping;
    current_task->sleep_until = current_tick + ms;
    task_yield();
}

void task_init(TCB& tcb, void (*func)(), uint32_t* stack, uint32_t stack_size, int priority){
    uint32_t* stack_top = stack + stack_size;

    stack_top = (uint32_t*)((uint32_t)stack_top & ~0x7);
    *(--stack_top) = 0x01000000;
    *(--stack_top) = (uint32_t)func & ~1U;    // PC - must have LSB clear
    *(--stack_top) = (uint32_t)task_exit_error;
    // r12, r3, r2, r1, r0
    for(int i = 0; i < 5; ++i) *(--stack_top) = 0;

    // r11-r4
    for(int i = 0; i < 8; ++i) *(--stack_top) = 0;


    tcb.stack_base = stack;
    tcb.stack_pointer = stack_top;
    tcb.state = Ready;
    tcb.priority = priority;
}

void task_create(TCB& tcb, void (*func)(), uint32_t* stack, uint32_t stack_size, int priority){
    task_init(tcb, func, stack, stack_size, priority);
    task_register(&tcb);
}

void task_suspend(TCB* tcb) {
    tcb->state = Blocked;
}

void task_resume(TCB* tcb) {
    tcb->state = Ready;
}






#endif