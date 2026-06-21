.syntax unified        
.cpu cortex-m4
.thumb                 

.section .vector_table, "a", %progbits   
.word 0x20020000    
.word reset_handler 
.word NMI_handler
.word HardFault_handler
.word MemManage_handler
.word BusFault_handler
.word UsageFault_handler

.word Default_handler
.word Default_handler
.word Default_handler
.word Default_handler
.word Default_handler
.word Default_handler
.word Default_handler

.word PendSV_handler

.word SysTick_handler 


.section .text, "ax", %progbits          

.global Default_handler
.type Default_handler, %function

Default_handler:
1:
    b 1b


.weak NMI_handler
.thumb_set NMI_handler, Default_handler

.weak HardFault_handler
.thumb_set HardFault_handler, Default_handler

.weak MemManage_handler
.thumb_set MemManage_handler, Default_handler

.weak BusFault_handler
.thumb_set BusFault_handler, Default_handler

.weak UsageFault_handler
.thumb_set UsageFault_handler, Default_handler




.weak SysTick_handler
.thumb_set SysTick_handler, Default_handler


.global scheduler_start
.type scheduler_start, %function
scheduler_start:
    @ Move stack pointer of current task into Process Stack Pointer. 
    ldr r0, =current_task 
    ldr r0, [r0]           
    ldr r0, [r0, #8]        
    msr psp, r0 

    @ Set bit 1 of control to use PSP in thread mode 
    mrs r0, control
    orr r0, r0, #2
    msr control, r0
    isb

    @ enable interrupts
    cpsie i

    @ trigger PendSV and hang (by sending back to the infinite loop in main())
    ldr r0, =0xE000ED04
    ldr r1, =(1 << 28)     
    str r1, [r0]

    bx lr               @ return to main, PendSV fires after

.global PendSV_handler
.type PendSV_handler, %function
PendSV_handler:
    @don't save current task unless scheduler is running already
    ldr r2, =scheduler_running
    ldrb r2, [r2]
    cmp r2, #0
    beq skip_save


    @ push current registers into r0
    mrs r0, psp
    stmdb r0!, {r4-r11}

    @ store contents of r0 (our stack) at the address of our current task's stack pointer
    ldr r1, =current_task
    ldr r1, [r1]
    str r0, [r1, #8]

    skip_save:
    @ call the schedule function, which moves to the next task 
    push {lr}         
    bl schedule
    pop {lr} 

    @ get the new current task's stack pointer and save it to the PSP
    ldr r1, =current_task
    ldr r1, [r1]
    ldr r0, [r1, #8]
    ldmia r0!, {r4-r11}
    msr psp, r0

    @ return to main
    bx lr


.global reset_handler  
.type reset_handler, %function  

reset_handler:
    ldr r0, =_data_load
    ldr r1, =_data_start
    ldr r2, =_data_end

    @copies data from FLASH to SRAM on startup.
    copy_data:
        cmp r1, r2
        bcs setup_bss @if compare operation finds that our current destination is > _data_end, break and move to next loop.
        ldr r3, [r0], #4  @load the word at the memory address in r0. Then add 4 to r0 (4 bytes = 1 word.)  Basicaly, r3 = *r0, r0 += 4;   
        str r3, [r1], #4 @store the word we just loaded into our destination, then increase the destination.
        b copy_data

    setup_bss:
        mov r0, #0x0
        ldr r1, =_bss_start
        ldr r2, =_bss_end
    zero_bss:
        cmp r1, r2
        bcs call_main
        str r0, [r1], #4 
        b zero_bss
    call_main:
        bl main
    hang:
        b hang
