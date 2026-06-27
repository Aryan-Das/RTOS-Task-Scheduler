#include "tcb.hpp"
#include "scheduler.hpp"

struct Semaphore{
    int count;
    int max_count;       
    TCB* waiters[MAX_TASKS];
    int waiter_count;
};

void sem_init(Semaphore* s, int initial_count, int max_count){
    s->count = initial_count;
    s->max_count = max_count;
    s->waiter_count = 0;
    for(int i = 0; i < MAX_TASKS; i++) s->waiters[i] = nullptr;
}

void sem_post(Semaphore* s){
    asm volatile("cpsid i" ::: "memory");
    
   
    if (s->waiter_count > 0){

        int highest = 0;
        for(int i = 1; i < s->waiter_count; i++) {
            if(s->waiters[i]->priority > s->waiters[highest]->priority) {
                highest = i;
            }
        }


        TCB* winner = s->waiters[highest];

       
        s->waiters[highest] = s->waiters[s->waiter_count - 1];
        s->waiters[s->waiter_count - 1] = nullptr;
        s->waiter_count--;

        winner->state = Ready;
        
    }else if(s->count < s->max_count){
        ++s->count;
    }
    
    asm volatile("cpsie i" ::: "memory");
        
    
    
}

void sem_wait(Semaphore* s){
    asm volatile("cpsid i" ::: "memory");
    if(s->count > 0){
        --s->count;
        asm volatile("cpsie i" ::: "memory");
    } else {
        s->waiters[s->waiter_count++] = current_task;
        current_task->state = Blocked;
        asm volatile("cpsie i" ::: "memory");
        task_yield();
    }
}
