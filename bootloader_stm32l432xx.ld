MEMORY
{
	boot_rom    (rx)  : ORIGIN = 0x8000000, LENGTH = 0x00800 /*   2K */
	boot_config (rx)  : ORIGIN = 0x8000800, LENGTH = 0x00800 /*   2K */
	app_rom     (rx)  : ORIGIN = 0x8001000, LENGTH = 0x3F000 /* 252K */

	vector_reloc(rwx) : ORIGIN = 0x20000000, LENGTH = 0x0200 /* 512  */
	boot_ram    (rwx) : ORIGIN = 0x20000200, LENGTH = 0x0200 /* 512  */
	app_ram     (rwx) : ORIGIN = 0x20000400, LENGTH = 0xFC00 /*  63K */
}

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
	} > boot_ram

	.data :
	{
		_sdata = .;
		*(.data*)
		_edata = .;
	} > boot_ram AT > boot_rom

	.stack(NOLOAD) :
	{
		. = ALIGN(8);
		_sstack = .;
		. = . + LENGTH(app_ram);
		. = ALIGN(8);
		_estack = .;
	} > app_ram
}

