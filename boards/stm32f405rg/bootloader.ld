MEMORY
{
	boot_rom    (rx)  : ORIGIN = 0x08000000, LENGTH =    16K
	boot_config (rx)  : ORIGIN = 0x08004000, LENGTH =    16K
	app_rom     (rx)  : ORIGIN = 0x08008000, LENGTH =   992K

	ram         (rwx) : ORIGIN = 0x20000000, LENGTH =   128K
}

_sapp_rom = ORIGIN(app_rom);
_eapp_rom = ORIGIN(app_rom) + LENGTH(app_rom);
_sboot_config = ORIGIN(boot_config);
_eboot_config = ORIGIN(boot_config) + LENGTH(boot_config);
_estack = ORIGIN(ram) + LENGTH(ram);

SECTIONS
{
	.text :
	{
		KEEP(*(.isr_vector))
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

	_sidata = LOADADDR(.data);

	.data :
	{
		_sdata = .;
		*(.data*)
		_edata = .;
	} > ram AT > boot_rom
}

