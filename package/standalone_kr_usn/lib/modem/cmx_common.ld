SECTIONS
{
	.text :
	{
		KEEP(*(.isr_vector))
		*(.text)
		*(.text*)

		. = ALIGN(4);
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);

		. = ALIGN(4);
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.init))
		KEEP(*(.fini))

		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

 		*crtbegin.o(.dtors)
 		*crtbegin?.o(.dtors)
 		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
 		*(SORT(.dtors.*))
 		*(.dtors)

		*(.rodata*)
		*(.rodata)
		*(.glue_7)
		*(.glue_7t)
		*(.gnu.linkonce.r.*)
		*(.gnu.linkonce.d.*);
/*		
		*(.data.impure_data);
		*(.data._impure_ptr);
*/
		*(.got);
		KEEP(*(.eh_frame*))
		. = ALIGN(4);
		_etext = .;
		
	} > FLASH

	.ARM.extab : 
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > FLASH

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASH
	__exidx_end = .;

	. = ALIGN(4);
	PROVIDE(__etext = .);

	.data : 
	{
		__data_load__ = LOADADDR(.data);
		__data_start__ = .;
		*(vtable)
		. = ALIGN(4);

		*(.sfc_code_nc*)
		. = ALIGN(4);
		__sfc_code_nc_end__ = .;
		*(.sfc_code_c*)
		. = ALIGN(4);
		
		*(.data)
		*(.data*)
		. = ALIGN(4);
		__measure_cpu_start = .;
		*(.measure_cpu)
		__measure_cpu_end = .;
		KEEP(*(.jcr*))
		. = ALIGN(4);
		KEEP(*(SORT(.nrc_cmd*)));
		. = ALIGN(4);
		*(.data.impure_data);
		*(.data._impure_ptr);
		. = ALIGN(4);
		PROVIDE(__data_end__ = .);
		_edata = .;
	} > RAM AT>FLASH

	PROVIDE(__dstart__ = __data_start__);

	.bssreloc (NOLOAD)  : 
	{
		PROVIDE(__bss_start__ = .);
		. = ALIGN(4);
		*(.bss*)
		. = ALIGN(4);
		*(COMMON)
		. = ALIGN(4);
		PROVIDE(__bss_end__ = .);
	} > RAM 

	PROVIDE(__ebss = .);
	. = ALIGN(4);

	.buffer_pool (NOLOAD) :
	{
		PROVIDE(__buffer_pool_start__ = .);
		. = ALIGN(4);
		*(.buffer)
		. = ALIGN(4);
		PROVIDE(__buffer_pool_end__ = .);
	} > RAM

	.retain :
	{
		__retention_load__ = LOADADDR(.retain);
		PROVIDE(__retention_start__ = .);
		*(.retent);
		. = ALIGN(4);
		PROVIDE(__retention_end__ = .);
	} > RETENTION AT>FLASH


	PROVIDE(end = .); 


	PROVIDE(__CODE_START = ORIGIN(FLASH));
	PROVIDE(__DATA_END = ORIGIN(FLASH) + LENGTH(FLASH));
	PROVIDE(__DATA_START = ORIGIN(RAM));
	PROVIDE(__DATA_END = ORIGIN(RAM) + LENGTH(RAM));
	PROVIDE(__CODE_START = ORIGIN(FLASH));
	PROVIDE(__DATA_END = ORIGIN(FLASH) + LENGTH(FLASH));
	PROVIDE(__DATA_START = ORIGIN(RAM));
	PROVIDE(__DATA_END = ORIGIN(RAM) + LENGTH(RAM) - STACK_SIZE);
	PROVIDE(__STACK_TOP = ORIGIN(RAM) + LENGTH(RAM));
	PROVIDE(__STACK_LIMIT = ORIGIN(RAM) + LENGTH(RAM) - STACK_SIZE);
/*
	PROVIDE(__STACK_TOP = ORIGIN(RETENTION) + LENGTH(RETENTION));
	PROVIDE(__STACK_LIMIT = ORIGIN(RETENTION) + LENGTH(RETENTION) - STACK_SIZE);
*/	
	PROVIDE(__MEASURE_CPU_START = __measure_cpu_start);
	PROVIDE(__MEASURE_CPU_END = __measure_cpu_end);
}
