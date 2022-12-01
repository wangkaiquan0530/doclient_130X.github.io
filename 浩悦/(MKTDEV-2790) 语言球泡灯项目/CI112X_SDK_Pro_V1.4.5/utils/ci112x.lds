/**
 ******************************************************************************
 * @file *.lds
 * @version V1.0.2
 * @date 2018-12-3
 * @brief project lds, add some export symbol for rtos used,
 ******************************************************************************
 */



/*****************************************************************************/
/*                         memory used sample chart                          */
/*****************************************************************************
                         +SRAM-----------------+ 0x1ff00000
                         |                     |
                         |       CODE          |
                         |                     |
                         +---------------------+
                         |                     |
                         |                     |
                         |       RW            |	
                         |                     |
                         |                     |
                         +---------------------+
                         |                     |
                         |       heap          |
                         |                     |
                         +---------------------+ 
                         |       stack         |
                         +---------------------+ 0x1ff70000
******************************************************************************/
ROM_SIZE      = 1024*(128+2);
RAM_SIZE      = 1024*(320-2);
STACK_SIZE    = 0x1000;/*inclued in RW*/


/*****************************************************************************/
/*                  don't modify these!!! decided by system                  */
/*****************************************************************************/
ASSERT((448*1024 == (ROM_SIZE + RAM_SIZE)), "Unsupported configuration size")

SRAM_START_ADDR            = 0x000000;
SRAM_END_ADDR              = 0x070000;
SRAM_SIZE                  = 1024*448;

__stack_size = STACK_SIZE;

OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY
{
	ilm (rxai!w) : ORIGIN = SRAM_START_ADDR,             LENGTH = ROM_SIZE
	ram (wxa!ri) : ORIGIN = SRAM_START_ADDR + ROM_SIZE,  LENGTH = RAM_SIZE
}

SECTIONS
{
    __stack_size = DEFINED(__stack_size) ? __stack_size : 2K;

    .init :
    {
        *(.vtable)
        KEEP (*(SORT_NONE(.init)))
    } >ilm AT>ilm 


    .ilalign :
    {
        . = ALIGN(4);
        PROVIDE( _ilm_lma = . );
    } >ilm AT>ilm 

    .ialign :
    {
        PROVIDE( _ilm = . );
    } >ilm AT>ilm 

    .text :
    {
        *(.text.unlikely .text.unlikely.*)
        *(.text.startup .text.startup.*)
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
    } >ilm AT>ilm 

    .fini :
    {
        KEEP (*(SORT_NONE(.fini)))
    } >ilm AT>ilm 

    . = ALIGN(4);

    PROVIDE (__etext = .);
    PROVIDE (_etext = .);
    PROVIDE (etext = .);
    PROVIDE( _eilm = . );


    .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } >ilm AT>ilm 

    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
        PROVIDE_HIDDEN (__init_array_end = .);
    } >ilm AT>ilm 

    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } >ilm AT>ilm 

    .ctors :
    {
        /* gcc uses crtbegin.o to find the start of
         * the constructors, so we make sure it is
         * first.  Because this is a wildcard, it
         * doesn't matter if the user does not
         * actually link against crtbegin.o; the
         * linker won't look for a file to match a
         * wildcard.  The wildcard also means that it
         * doesn't matter which directory crtbegin.o
         * is in.  
         */
        KEEP (*crtbegin.o(.ctors))
        KEEP (*crtbegin?.o(.ctors))
        /* We don't want to include the .ctor section from
         * the crtend.o file until after the sorted ctors.
         * The .ctor section from the crtend file contains the
         * end of ctors marker and it must be last
         */
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
    } >ilm AT>ilm 

    .dtors :
    {
        KEEP (*crtbegin.o(.dtors))
        KEEP (*crtbegin?.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
    } >ilm AT>ilm 


    .lalign :
    {
        . = ALIGN(4);
        PROVIDE( _data_lma = . );
    } >ilm AT>ilm 

    .dalign :
    {
        . = ALIGN(4);
        PROVIDE( _data = . );
    } >ram AT>ilm 

    .data :
    {
        *(.rdata)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(8);
        PROVIDE( __global_pointer$ = . + 0x800 );
        *(.sdata .sdata.* .sdata*)
        *(.gnu.linkonce.s.*)
        . = ALIGN(8);
        *(.srodata.cst16)
        *(.srodata.cst8)
        *(.srodata.cst4)
        *(.srodata.cst2)
        *(.srodata .srodata.*)
    } >ram AT>ilm 

    . = ALIGN(4);
    PROVIDE( _edata = . );
    PROVIDE( edata = . );

    PROVIDE( _fbss = . );
    PROVIDE( __bss_start = . );

    .bss :
    {
        *(.sbss*)
        *(.gnu.linkonce.sb.*)
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
    } >ram AT>ram 

    . = ALIGN(8);
    PROVIDE( _end = . );
    PROVIDE( end = . );

    .stack ORIGIN(ram) + LENGTH(ram) - __stack_size :
    {
        PROVIDE( _heap_end = . );
        . = __stack_size;
        PROVIDE( _sp = . );
    } >ram AT>ram 
}
