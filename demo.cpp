#include <stdint.h>

#ifdef USE_SWO
    #include "itm.hpp"
    #define putc itm_putc
    #define print_str itm_print_str
    #define print_num itm_print_num
    #define configure_print configure_itm
    #define print_float itm_print_float
#else
    #include "uart2.hpp"
    #define putc uart2_putc
    #define print_str uart2_print_str
    #define print_num uart2_print_num
    #define configure_print configure_uart2
    #define print_float uart2_print_float
#endif

#include "clock.hpp"
#include "tcb.hpp"
#include "scheduler.hpp"
#include "mutex.hpp"
#include "semaphore.hpp"
#include "math_utils.hpp"
#include "lis3dsh.hpp"


Mutex uart_mutex;


/* Known limitation: first context switch has a brief race condition 
before scheduler state fully initializes. Subsequent switches are 
correct. Root cause: initial PSP setup doesn't go through full 
PendSV save/restore path. */

float sensor_reading = 0.0f;
float filtered_output = 0.0f;
float pid_output = 0.0f;

Mutex sensor_mutex;
Semaphore data_ready;


Mutex accel_mutex;
AccelReadingG latest_accel;

Mutex pid_mutex;


struct PIDOutput {
    float error;
    float p_term;
    float i_term;
    float d_term;
    float output;
};

PIDOutput pid_state;


void imu_task() {
    task_yield(); task_yield(); task_yield();
    while(1) {
        AccelReading raw = lis3dsh_read_accel();
        AccelReadingG g = lis3dsh_to_g(raw);

        mutex_lock(&accel_mutex);
        latest_accel = g;
        mutex_unlock(&accel_mutex);

        sem_post(&data_ready);
        task_sleep(10); // 100Hz
    }
}
void telemetry_task(){
    task_yield(); task_yield(); task_yield();
    while(1) {
        task_sleep(500);
        mutex_lock(&accel_mutex);
        AccelReadingG a = latest_accel;
        mutex_unlock(&accel_mutex);
        mutex_lock(&sensor_mutex);
        float filtered = filtered_output;
        mutex_unlock(&sensor_mutex);

        mutex_lock(&pid_mutex);
        PIDOutput p = pid_state;
        mutex_unlock(&pid_mutex);

        mutex_lock(&uart_mutex);
        print_str("accel x: ");
        print_float(a.x);
        print_str(" y: ");
        print_float(a.y);
        print_str(" z: ");
        print_float(a.z);
        print_str(" | filtered: ");
        print_float(filtered);
        print_str(" | pid_error: ");
        print_float(p.error);
        print_str(" p: ");
        print_float(p.p_term);
        print_str(" i: ");
        print_float(p.i_term);
        print_str(" d: ");
        print_float(p.d_term);
        print_str(" out: ");
        print_float(p.output);
        putc('\n');
        mutex_unlock(&uart_mutex);
    }
}

void control_task() {
    task_yield(); task_yield(); task_yield();
    static float filtered = 0.0f;
    while(1) {
        sem_wait(&data_ready);
        mutex_lock(&accel_mutex);
        float read_data = latest_accel.z; 
        mutex_unlock(&accel_mutex);
        filtered = 0.1f * read_data + 0.9f * filtered;
        mutex_lock(&sensor_mutex);
        filtered_output = filtered;
        mutex_unlock(&sensor_mutex);
    }
}
void pid_task(){
    task_yield(); task_yield(); task_yield();
    static float integral = 0.0f;
    static float previous_error = 0.0f;
    static float filtered = 0.0f;
    const float set_point = 0.0f;
    const float Kp = 1.0f;
    const float Ki = 0.0f;
    const float Kd = 0.0f;

    const float min_i = -4.0f;
    const float max_i = 4.0f;
    while(1){
 
        mutex_lock(&accel_mutex);
        float read_data = latest_accel.x; 
        mutex_unlock(&accel_mutex);
        filtered = 0.1f * read_data + 0.9f * filtered;
        float error = set_point - filtered;
        float p_term = Kp * error;
        integral += error * 0.01f;
        if(integral < min_i) integral = min_i;
        if(integral > max_i) integral = max_i;
        float i_term = Ki * integral;
        float d_term = Kd * (error - previous_error) / 0.01f;
        mutex_lock(&pid_mutex);
        pid_state.error = error;
        pid_state.p_term = p_term;
        pid_state.i_term = i_term;
        pid_state.d_term = d_term;
        pid_state.output = p_term + i_term + d_term;
        mutex_unlock(&pid_mutex);
        previous_error = error;
        task_sleep(10.0f);
    }
}

void stats_task() {
    task_yield(); task_yield(); task_yield();
    while(1) {
        task_sleep(2000);
        putc('\n');
        mutex_lock(&uart_mutex);
        for(int i = 0; i < task_count; i++) {
            TCB* t = task_list[i];
            print_str(t->name);
            print_str(" |state: ");
            print_num(t->state);
            print_str(" |priority: ");
            print_num(t->priority);
            print_str(" |switches: ");
            print_num(t->context_switches);
            print_str(" |stack: ");
            
            print_num(stack_hwm(*t));
            putc('/');
            print_num(t->stack_size);
            putc('\n');
        }
        mutex_unlock(&uart_mutex);
    }
}



TCB tcb_imu;
TCB tcb_control;
TCB tcb_telemetry;
TCB tcb_stats;
TCB tcb_idle;
TCB tcb_pid;

void idle_task() {
    while(1) {
        asm volatile("wfi");
    }
}

static uint32_t stack_imu[4096]    __attribute__((aligned(8)));
static uint32_t stack_control[4096]   __attribute__((aligned(8)));
static uint32_t stack_telemetry[512] __attribute__((aligned(8)));
static uint32_t stack_stats[1024]     __attribute__((aligned(8)));
static uint32_t stack_idle[512]       __attribute__((aligned(8)));
static uint32_t stack_pid[4096] __attribute__((aligned(8)));

int main(){
    *((volatile uint32_t*)0xE000ED88) |= (0xF << 20); // FPU enable

    configure_clock_168mhz();
    configure_print();    
    configure_lis3dsh();
    lis3dsh_enable();

    mutex_init(&uart_mutex);
    mutex_init(&sensor_mutex);
    mutex_init(&accel_mutex);
    mutex_init(&pid_mutex);
    sem_init(&data_ready, 0, 8);
    task_create(tcb_imu,       "imu",       imu_task,       stack_imu,       4096, 2);
    task_create(tcb_control,   "control",   control_task,   stack_control,   4096, 1);
    task_create(tcb_telemetry, "telemetry", telemetry_task, stack_telemetry, 512, 1);
    task_create(tcb_stats,     "stats",     stats_task,     stack_stats,     1024, 1);
    task_create(tcb_idle,      "idle",      idle_task,      stack_idle,      512,  0);
    task_create(tcb_pid, "pid", pid_task, stack_pid, 4096, 1);

    systick_init();
    scheduler_start();
}

