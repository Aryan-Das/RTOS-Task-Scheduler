MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}


SECTIONS
{
    .text : { 
        KEEP(*(.vector_table));
        *(.text*);
        *(.rodata*);
    } > FLASH
    .data : { 
        _data_load = LOADADDR(.data);
        _data_start = .;
        *(.data*);
        _data_end = .;
    } > SRAM AT > FLASH
    .bss  : { 
        _bss_start = .;
        *(.bss*);
        *(COMMON);
        _bss_end = .;
    } > SRAM
}
