#ifndef MUTEX_HPP
#define MUTEX_HPP

#include "tcb.hpp"
#include "scheduler.hpp"
#include <stddef.h>

struct Mutex{
    bool locked;
    TCB* lock_owner;
    TCB* waiters[MAX_TASKS];
    int waiter_count;
};

void mutex_init(Mutex* m) {
    m->locked = false;
    m->lock_owner = nullptr;
    m->waiter_count = 0;
    for(int i = 0; i < MAX_TASKS; i++) {
        m->waiters[i] = nullptr;
    }
}

void mutex_lock(Mutex* m){
    asm volatile("cpsid i" ::: "memory");
    if (m->locked){
        m->waiters[m->waiter_count++] = current_task;
        current_task->state = Blocked;
        asm volatile("cpsie i" ::: "memory");
        task_yield();
    }   
    else{
        m->locked = true;
        m->lock_owner = current_task;
        asm volatile("cpsie i" ::: "memory");
    }
   
}
void mutex_unlock(Mutex* m){
    asm volatile("cpsid i" ::: "memory");
    if(m->waiter_count == 0){
        m->locked = false;
        m->lock_owner = nullptr;
    }
    else{
        int highest = 0;
        for(int i = 1; i < m->waiter_count; i++) {
            if(m->waiters[i]->priority > m->waiters[highest]->priority) {
                highest = i;
            }
        }

        m->lock_owner = m->waiters[highest];
        

        m->waiters[highest] = m->waiters[m->waiter_count - 1];
        m->waiters[m->waiter_count - 1] = nullptr;
        m->waiter_count--;   
        
        m->lock_owner->state = Ready;
    }
    asm volatile("cpsie i" ::: "memory");
    task_yield();
}




#endif