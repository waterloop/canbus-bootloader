MEMORY
{
	boot_rom    (rx)  : ORIGIN = 0x08000000, LENGTH = 0x01800 /*   6K */
	boot_config (rx)  : ORIGIN = 0x08001800, LENGTH = 0x00800 /*   2K */
	app_rom     (rx)  : ORIGIN = 0x08002000, LENGTH = 0x3E000 /* 248K */

	ram         (rwx) : ORIGIN = 0x20000000, LENGTH = 0x10000 /*  64K */
}

STACK_SIZE = 0x4000;

_sapp_rom = ORIGIN(app_rom);
_eapp_rom = ORIGIN(app_rom) + LENGTH(app_rom);
_sboot_config = ORIGIN(boot_config);
_eboot_config = ORIGIN(boot_config) + LENGTH(boot_config);
_estack = ORIGIN(ram) + LENGTH(ram);

SECTIONS
{
	.text :
	{
		KEEP(*(.vectors))
		*(.text*)
		*(.rodata*)
		_etext = .;
	} > boot_rom

	.bss(NOLOAD) :
	{
		_sbss = .;
		*(.bss*)
		*(COMMON)
		_ebss = .;
	} > ram

	.data :
	{
		_sdata = .;
		*(.data*)
		_edata = .;
	} > ram AT > boot_rom

/*	.stack(NOLOAD) :
	{
		. = ALIGN(8);
		_sstack = .;
		. = . + STACK_SIZE;
		. = ALIGN(8);
		_estack = .;
	} > ram*/
}

