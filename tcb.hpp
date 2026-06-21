#ifndef TCB_HPP
#define TCB_HPP

#include <stdint.h>

enum TaskState{
    Running,
    Ready,
    Blocked, 
    Inactive
};

struct TCB{
    const char* name;
    uint32_t id;
    uint32_t* stack_pointer;
    uint32_t* stack_base;
    TaskState state;
    int priority;

};



#endif