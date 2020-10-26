#include "utils.h"

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

	// Disable interrupts
	__disable_irq();

	// Set FLASH configurations to prepare for 80MHz
	// Enable data and instruction cache, enable prefetch, latency 4 wait cycles
	FLASH->ACR = FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_4WS;

	// Clock configuration begin
	
	// Set MSI clock rate to 48MHz
	// Wait for stable MSI
	while(!(RCC->CR & RCC_CR_MSIRDY_Msk));
	// Select 48MHz and set MSI clock range to MSIRANGE
	RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE_Msk) | RCC_CR_MSIRANGE_11 | RCC_CR_MSIRGSEL;

	// Configure PLL
	// Disable PLL
	RCC->CR &= ~RCC_CR_PLLON_Msk;
	// Wait for PLL to fully stop
	while(RCC->CR & RCC_CR_PLLRDY_Msk);
	// Configure PLL: PLLR = 0 (/ 2), PLLN = 10 (* 10), PLLM = 3 (/ 3), PLLSRC = 1 (MSI)
	RCC->PLLCFGR = REG_FIELD(RCC_PLLCFGR_PLLN, 10) | REG_FIELD(RCC_PLLCFGR_PLLM, 2) | RCC_PLLCFGR_PLLSRC_MSI;

	// Start PLL
	// Wait for stable MSI
	while(!(RCC->CR & RCC_CR_MSIRDY_Msk));
	// Enable PLL
	RCC->CR |= RCC_CR_PLLON;
	// Wait for PLL lock
	while(!(RCC->CR & RCC_CR_PLLRDY_Msk));
	// Main PLL PLLCLK output enable
	RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN;

	// Switch SYSCLK over to PLL output
	RCC->CFGR |= RCC_CFGR_SW_PLL;

	// Clock configuration complete

	// Copy .data from flash to ram
	for(uint32_t *dst = &_sdata, *src = &_etext; dst < &_edata; ++dst, ++src) {
		*dst = *src;
	}
	
	// Zero-initialize .bss
	for(uint32_t *ptr = &_sbss; ptr < &_ebss; ++ptr) {
		*ptr = 0;
	}

	// Enable interrupts
	__enable_irq();

	main();

	while(1);
}

void NMI_Handler(void) {
	while(1);
}

void HardFault_Handler(void) {
	while(1);
}

void MemManage_Handler(void) {
	while(1);
}

void BusFault_Handler(void) {
	while(1);
}

void UsageFault_Handler(void) {
	while(1);
}

void Default_Handler(void) {
	while(1);
}

void __attribute__((weak, alias("Default_Handler"))) CAN1_RX0_IRQHandler();
void __attribute__((weak, alias("Default_Handler"))) CAN1_RX1_IRQHandler();

__attribute__((section(".vectors")))
const void *isr_vector[128] = {
        &_estack,
        Reset_Handler,
        NMI_Handler,
        HardFault_Handler,
        MemManage_Handler,
        BusFault_Handler,
        UsageFault_Handler,
        0,
        0,
        0,
        0,
        Default_Handler,
        Default_Handler,
        0,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        CAN1_RX0_IRQHandler,
        CAN1_RX1_IRQHandler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        0,
        0,
        Default_Handler,
        Default_Handler,
        0,
        0,
        Default_Handler,
        0,
        Default_Handler,
        Default_Handler,
        0,
        Default_Handler,
        Default_Handler,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        Default_Handler,
        0,
        0,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        0,
        0,
        0,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        Default_Handler,
        0,
        Default_Handler,
        Default_Handler,
        0,
        0,
        Default_Handler,
        Default_Handler,
        Default_Handler,
};
