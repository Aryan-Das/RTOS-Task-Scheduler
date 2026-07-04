# Bare Metal RTOS Task Scheduler for Cortex-M4

## Features
- **Round-robin and Priority Scheduler:** SysTick fires every 1ms, context switches via PendSV using the ARM-recommended deferred interrupt pattern
- **Context Switching:** saves/restores r4–r11 and FPU registers s16–s31 in ARM assembly
- **Task API Functions:** `task_create`, `task_yield`, `task_sleep`, `task_suspend`, `task_resume`
- **Mutex:** priority-ordered waiter list, atomic critical sections using `cpsid i`
- **Counting semaphore:** producer/consumer signaling with blocking and wakeup
- **Per-task stats:** context switch count, stack high-water mark, state, priority
- **Sensor pipeline demo:** simulated ADC -> low-pass filter -> telemetry over UART

## Sensor Pipeline Demo
```                                       
  sensor_task──>sem_post──> control_task
  (10ms, sine  sensor_mutex  (low-pass
   + noise)        │          filter)
                   │              │
              telemetry_task <────┘
              (500ms, UART)   sensor_mutex
 
  stats_task (2000ms) ── prints per-task metrics
  idle_task           ── wfi when nothing runnable
```

## Memory Layout
 
```
FLASH: 0x08000000  1MB   code, rodata
SRAM:  0x20000000  128KB stack arrays, global variables, TCBs
```
 
The linker script places `.text` in FLASH and `.data`/`.bss` in SRAM. The startup assembly (`startup.s`) copies `.data` from FLASH to SRAM and zeros `.bss` and then calls `main()`.

## Build Instructions
**Install Dependencies (macOS):**
```bash
brew install arm-none-eabi-gcc qemu
```

**Build:**
```bash
make
```
 
**Run on QEMU (netduinoplus2 / STM32F407):**
```bash
make qemu
```
 
Exit QEMU with `Ctrl+A` then `X`.

## File Structure
 
```
rtos/
├── Makefile          # build and QEMU targets
├── linker.ld         # FLASH/SRAM memory map, section layout
├── startup.s         # vector table, reset handler, PendSV, SysTick
├── tcb.hpp           # Task Control Block struct, TaskState enum
├── scheduler.hpp     # schedule(), task API, SysTick handler
├── mutex.hpp         # Mutex struct, mutex_lock/unlock
├── semaphore.hpp     # Semaphore struct, sem_post/wait
├── uart.hpp          # UART driver, print helpers, uart_print_float, uart_print_num, and uart_print_str
├── math_utils.hpp    # sinf() Taylor Series approximation
└── main.cpp          # task functions, virtual stack definitions, main()
```


## Known Limitations
 
- **First context switch race condition:** the initial PendSV fires before the full scheduler state is established. Tasks call `task_yield()` three times on entry at the beginning to let the scheduler settle. Root cause: `scheduler_start` doesn't go through the full PendSV save/restore path.
- **Hardcoded TCB field offset:** `stack_pointer` is accessed at byte offset 8 in `PendSV_handler`. Reordering TCB fields above it will break this.
- **QEMU limitations:** QEMU does not accurately emulate STM32 clock gating or GPIO alternate function configuration. The UART configure code is correct for real hardware but QEMU ignores it. Validated on QEMU; not yet tested on real hardware.
- **No memory protection:** tasks can read/write each other's stacks. A production RTOS would use the Cortex-M4 MPU to isolate task address spaces.

## Future Development

- Flash to a physical STM32F4 Discovery board 
- Add MPU (Memory Protection Unit) to isolate task stacks
- Port the sensor demo to read a real I2C accelerometer on the actual board
