/**
 * @file stm32f429_linker.ld
 * @brief STM32F429 linker script for QEMU ARM Cortex-M4 target
 * 
 * STM32F429 memory map:
 *  - Flash: 0x08000000 - 0x080FFFFF (1MB)
 *  - SRAM1: 0x20000000 - 0x2002FFFF (192KB)
 *  - SRAM2: 0x20030000 - 0x2003FFFF (64KB)
 *  - CCRAM: 0x10000000 - 0x1000FFFF (64KB, core-coupled, for performance-critical code)
 */

MEMORY
{
    /* Flash memory - program code and constants */
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    
    /* Main SRAM - data, BSS, heap, stack */
    SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 192K
    
    /* CCRAM - performance-critical data (optional, not used by default) */
    CCRAM (rwx) : ORIGIN = 0x10000000, LENGTH = 64K
}

/* Define output sections */
SECTIONS
{
    /* ==================== FLASH SECTIONS ==================== */
    
    /* Interrupt vector table must be first in flash at 0x08000000 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH
    
    /* Program code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH
    
    /* Initialization array for constructors */
    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
    } > FLASH
    
    /* Data initialization (in flash, loaded to SRAM at boot) */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH
    
    /* Starting address of data section in flash (for copying) */
    _sidata = LOADADDR(.data);
    
    /* ==================== SRAM SECTIONS ==================== */
    
    /* Zero-initialized data (BSS) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM
    
    /* Heap - grows upward from end of BSS */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(_heap_start = .);
        . = . + 16K;  /* Reserve 16KB for heap */
        PROVIDE(_heap_end = .);
    } > SRAM
    
    /* Stack - grows downward from end of SRAM */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        . = . + 16K;  /* Reserve 16KB for stack */
        PROVIDE(_estack = .);
    } > SRAM
    
    /* Remove unused sections */
    /DISCARD/ :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }
}

/* Provide symbols for use in C code */
PROVIDE(__estack = _estack);
