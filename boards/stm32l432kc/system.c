#include "utils.h"
#include "system_inc.h"

void SystemInit(void) {
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

	// Enable interrupts
	__enable_irq();
}

void reset(void) {

}
