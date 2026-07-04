#include <stdint.h>

#include "uart.hpp"
#include "tcb.hpp"
#include "scheduler.hpp"
#include "mutex.hpp"
#include "semaphore.hpp"
#include "math_utils.hpp"



Mutex uart_mutex;


/* Known limitation: first context switch has a brief race condition 
before scheduler state fully initializes. Subsequent switches are 
correct. Root cause: initial PSP setup doesn't go through full 
PendSV save/restore path. */

float sensor_reading = 0.0f;
float filtered_output = 0.0f;

Mutex sensor_mutex;
Semaphore data_ready;

// for math library to call since I am using -nostdlib
extern "C" void* _sbrk(int incr) {
    return (void*)-1; 
}



void sensor_task() {
    task_yield(); task_yield(); task_yield();
    static float angle = 0.0f;
    while(1) {
        float noise = ((current_tick * 1664525 + 1013904223) & 0xFF) / 255.0f * 0.1f - 0.05f; // simple pseudo random hash to simulate noise
        float new_reading = sinf(angle) + noise;
        
        mutex_lock(&sensor_mutex);
        sensor_reading = new_reading;
        mutex_unlock(&sensor_mutex);
        
        sem_post(&data_ready);
        angle += (PI / 50.0f);
        task_sleep(10);
    }
}

void telemetry_task(){
    task_yield(); task_yield(); task_yield();
    
    while(1) {

        task_sleep(500);
        mutex_lock(&sensor_mutex);
        float sensor = sensor_reading;
        float filtered = filtered_output;
        mutex_unlock(&sensor_mutex);
        mutex_lock(&uart_mutex);
        uart_print_str("sensor: ");
        uart_print_float(sensor);
        uart_print_str("\nfiltered: ");
        uart_print_float(filtered);
        uart_putc('\n');
        mutex_unlock(&uart_mutex);
    }
}


void control_task() {
    task_yield(); task_yield(); task_yield();
    static float filtered = 0.0f;
    while(1) {
        sem_wait(&data_ready);
        mutex_lock(&sensor_mutex);
        float read_data = sensor_reading;
        mutex_unlock(&sensor_mutex);
        filtered = 0.1f * read_data + 0.9f * filtered;
        mutex_lock(&sensor_mutex);
        filtered_output = filtered;
        mutex_unlock(&sensor_mutex);
    }
}


void stats_task() {
    task_yield(); task_yield(); task_yield();
    while(1) {
        task_sleep(2000);
        uart_putc('\n');
        mutex_lock(&uart_mutex);
        for(int i = 0; i < task_count; i++) {
            TCB* t = task_list[i];
            uart_print_str(t->name);
            uart_print_str(" |state: ");
            uart_print_num(t->state);
            uart_print_str(" |priority: ");
            uart_print_num(t->priority);
            uart_print_str(" |switches: ");
            uart_print_num(t->context_switches);
            uart_print_str(" |stack: ");
            
            uart_print_num(stack_hwm(*t));
            uart_putc('/');
            uart_print_num(t->stack_size);
            uart_putc('\n');
        }
        mutex_unlock(&uart_mutex);
    }
}



TCB tcb_sensor;
TCB tcb_control;
TCB tcb_telemetry;
TCB tcb_stats;
TCB tcb_idle;

void idle_task() {
    while(1) {
        asm volatile("wfi");
    }
}

static uint32_t stack_sensor[4096]    __attribute__((aligned(8)));
static uint32_t stack_control[4096]   __attribute__((aligned(8)));
static uint32_t stack_telemetry[4096] __attribute__((aligned(8)));
static uint32_t stack_stats[1024]     __attribute__((aligned(8)));
static uint32_t stack_idle[512]       __attribute__((aligned(8)));

int main(){
    configure_uart();
    

    mutex_init(&uart_mutex);
    mutex_init(&sensor_mutex);
    sem_init(&data_ready, 0, 8);  
    task_create(tcb_sensor,   "sensor task",   sensor_task,    stack_sensor,   4096, 1);
    task_create(tcb_control,   "control task",   control_task,  stack_control,   4096, 1);
    task_create(tcb_telemetry, "telemetry task", telemetry_task,  stack_telemetry, 4096, 1);
    task_create(tcb_stats, "stats",    stats_task, stack_stats, 1024, 1);
    task_create(tcb_idle,  "idle",     idle_task, stack_idle,  512, 0);
    

    current_task = task_list[0];
    tcb_sensor.state = Ready;
    tcb_control.state = Ready;
    tcb_telemetry.state = Ready;
    tcb_stats.state = Ready;
    tcb_idle.state = Ready;
    current_task = task_list[0];
    current_task->state = Running;
    systick_init();
    scheduler_start();

    while(1) {}
}

