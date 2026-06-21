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

TCB* current_task = nullptr;


const int MAX_TASKS = 8;
TCB* task_list[MAX_TASKS];
int task_count = 0;
int current_index = 0;

extern "C" void scheduler_start();

void task_register(TCB* tcb) {
    task_list[task_count++] = tcb;
}

bool scheduler_running = false;

extern "C" void schedule() {
    if (!scheduler_running) {
        scheduler_running = true;
        current_task = task_list[0];
        return;
    }
    current_index = (current_index + 1) % task_count;
    current_task = task_list[current_index];
}


extern "C" void HardFault_handler() {
    uart_putc('!');
    
    while(1) {}
}

extern "C" void SysTick_handler() {

    *ICSR = (1 << 28);
}

void systick_init(){
    *SCB_SHPR3 |= (0xFF << 16); //set PendSV to lowest priority
    *SYST_RVR = 16000; //set timer to countdown from 16,000
    *SYST_CVR = 0;
    *SYST_CSR = (1 << 2) | (1 << 1) | (1 << 0);
}


extern "C" void task_exit_error() {
    while(1) {}
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
#endif