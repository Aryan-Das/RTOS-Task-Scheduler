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
        