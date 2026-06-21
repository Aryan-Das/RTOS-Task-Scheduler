#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "tcb.hpp"

void task_exit_error() {
    while(1) {}
}

void task_init(TCB& tcb, void (*func)(), uint32_t* stack, uint32_t stack_size, int priority){
    uint32_t* stack_top = stack + stack_size;
    *(--stack_top) = 0x01000000;
    *(--stack_top) = (uint32_t)func;
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