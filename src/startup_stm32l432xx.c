#include <stm32l432xx.h>

// .stack end
extern uint32_t _estack;

// .text end
extern uint32_t _etext;

// .bss begin and end
extern uint32_t _sbss;
extern uint32_t _ebss;

// .data begin and end
extern uint32_t _sdata;
extern uint32_t _edata;

int main(void);

void Reset_Handler(void) {
	// Copy .data from flash to ram
	for(uint32_t *dst = &_sdata, *src = &_etext; dst < &_edata; ++dst, ++src) {
		*dst = *src;
	}
	
	// Zero-initialize .bss
	for(uint32_t *ptr = &_sbss; ptr < &_ebss; ++ptr) {
		*ptr = 0;
	}

	main();

	while(1);
}

void NMI_Handler(void) {
	while(1);
}

void HardFault_Handler(void) {
	while(1);
}

__attribute__((section(".vectors")))
const void *isr_vector[128] = {
	&_estack,
	Reset_Handler,
	NMI_Handler,
	HardFault_Handler,
};
