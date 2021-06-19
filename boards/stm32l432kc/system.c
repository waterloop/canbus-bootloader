#include "platform.h"
#include "crc32.h"
#include "system_inc.h"

void SystemInit(void) {
	// Disable interrupts
	__disable_irq();

	// Set FLASH configurations to prepare for 80MHz
	// Enable data and instruction cache, enable prefetch, latency 4 wait states
	FLASH->ACR = FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_4WS;

	// Enable APB1 clock for CAN1
	RCC->APB1ENR1 |= RCC_APB1ENR1_CAN1EN;
	// Enable AHB2 clock for GPIO Port A
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

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

	// Setup CAN bus peripheral
	// Clear alt functions for PA11 and PA12, and set them to alt function 9
	GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(GPIO_AFRH_AFSEL11 | GPIO_AFRH_AFSEL12)) | REG_FIELD(GPIO_AFRH_AFSEL11, 9) | REG_FIELD(GPIO_AFRH_AFSEL12, 9);
	// Configure GPIO pins PA11 and PA12 to be very high speed
	GPIOA->OSPEEDR |= REG_FIELD(GPIO_OSPEEDR_OSPEED11, 3) | REG_FIELD(GPIO_OSPEEDR_OSPEED12, 3);
	// Clear port mode configuration for PA11 and PA12, and set them to Alternate function mode
	GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODE11 | GPIO_MODER_MODE12)) | REG_FIELD(GPIO_MODER_MODE11, 2) | REG_FIELD(GPIO_MODER_MODE12, 2);

	// Enable interrupts
	__enable_irq();
}

#define ALL_FLASH_ERR (FLASH_SR_FASTERR | FLASH_SR_MISERR | FLASH_SR_PGSERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_WRPERR | FLASH_SR_PROGERR)

uint32_t flash_addr_to_page(uint32_t start) {
	return (start - 0x08000000) >> 11;
}

// Page 83 of reference manual, 3.3.6
flash_ret_t flash_erase_page(uint32_t page) {
	while(FLASH->SR & FLASH_SR_BSY); // Wait for ready
	FLASH->SR |= ALL_FLASH_ERR; // Clear all previous errors
	FLASH->CR = (FLASH->CR & ~FLASH_CR_PNB) | REG_FIELD(FLASH_CR_PNB, page) | FLASH_CR_PER;
	FLASH->CR |= FLASH_CR_STRT; // Start erase
	while(FLASH->SR & FLASH_SR_BSY); // Wait for ready
	FLASH->CR &= ~FLASH_CR_PER; // Disable page erase
	return (FLASH->SR & ALL_FLASH_ERR) ? FLASH_ERASE_FAIL : FLASH_OK;
}

// Page 84 of reference manual, 3.3.7
// Documentation states EOP should be cleared after write success.
// We ignore it here because EOP only gets set when "end of operation" interrupts are enabled (EOPIE = 1)
flash_ret_t flash_write(uint32_t addr, const void *data, uint32_t len) {
	while(FLASH->SR & FLASH_SR_BSY); // Wait for ready
	FLASH->SR |= ALL_FLASH_ERR; // Clear all previous errors
	FLASH->CR |= FLASH_CR_PG;
	uint32_t *dst = (uint32_t *) addr;
	const uint32_t *src = data;
	uint32_t *end = (uint32_t *) ((uint8_t *) src + len);
	while(src < end) {
		dst[0] = src[0];
		dst[1] = src[1];
		src += 2;
		dst += 2;
		while(FLASH->SR & FLASH_SR_BSY);
		// Check for success
		if(FLASH->SR & ALL_FLASH_ERR) {
			FLASH->CR &= ~FLASH_CR_PG;
			return FLASH_WRITE_FAIL;
		}
	}
	FLASH->CR &= ~FLASH_CR_PG;
	return FLASH_OK;
}


void flash_unlock(void) {
	if(FLASH->CR & FLASH_CR_LOCK) {
		FLASH->KEYR = 0x45670123;
		FLASH->KEYR = 0xCDEF89AB;
	}
}

void flash_lock(void) {
	FLASH->CR |= FLASH_CR_LOCK;
}

uint32_t compute_device_id() {
	return ~crc32(CRC32_INIT, (void *) UID_BASE, 12);
}

void sleep(uint32_t ms) {
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // Start CYCCNT counter
	uint32_t starting = DWT->CYCCNT;
	while(DWT->CYCCNT - starting <= ms * SYSTEM_CLOCK_KHZ);
}

extern uint32_t _sapp_rom;

void start_user_app() {
	__disable_irq();

	SCB->VTOR = (uint32_t) &_sapp_rom;

	__enable_irq();

	// Update stack pointer to the one specified in user application
	__asm volatile("mov sp, %0" : : "r" ((&_sapp_rom)[0]));
	// Jump to the user application. Does not return
	__asm volatile("bx %0" : : "r" ((&_sapp_rom)[1]));
}
