#include "platform.h"
#include "crc32.h"
#include "system_inc.h"

void SystemInit(void) {
	// Disable interrupts
	__disable_irq();

	// Set FLASH configurations to prepare for 168MHz
	// Enable data, instruction caches, enable prefetch, latency = 5 wait states
	FLASH->ACR = FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_5WS;

	// Clock configuration begin
	// Enable GPIOH clock for external oscillator, GPIOA for CAN Bus on AHB1
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN | RCC_AHB1ENR_GPIOAEN;
	// Enable CAN peripheral on APB1
	RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
	// Enable HSE external oscillator and turn on Clock Security System
	RCC->CR |= RCC_CR_HSEON | RCC_CR_CSSON;
	// Select HSE as input to PLL, APB1 Prescaler = 4, APB2 Prescaler = 2, SYSCLK not divided
	RCC->CFGR |= RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_HPRE_DIV1;
	// PLLSRC = HSE, PLLN = 168, PLLP = 0 (/ 2), PLLQ = 7 (/7), PLLM = 8
	RCC->PLLCFGR = (RCC->PLLCFGR & ~(RCC_PLLCFGR_PLLQ | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP)) |
			RCC_PLLCFGR_PLLSRC_HSE | REG_FIELD(RCC_PLLCFGR_PLLN, 168) | REG_FIELD(RCC_PLLCFGR_PLLP, 0) | REG_FIELD(RCC_PLLCFGR_PLLQ, 7) | REG_FIELD(RCC_PLLCFGR_PLLM, 8);
	// Turn on PLL
	RCC->CR |= RCC_CR_PLLON;
	// Wait for HSE and PLL lock
	while((RCC->CR & (RCC_CR_PLLRDY | RCC_CR_HSERDY)) != (RCC_CR_PLLRDY | RCC_CR_HSERDY));
	// Switch SYSCLK over to PLL output
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	// Clock configuration complete

	// Enable interrupts
	__enable_irq();

	// Setup CAN bus peripheral
	// Clear alt functions for PA11 and PA12, and set them to alt function 9
	GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(GPIO_AFRH_AFSEL11 | GPIO_AFRH_AFSEL12)) | REG_FIELD(GPIO_AFRH_AFSEL11, 9) | REG_FIELD(GPIO_AFRH_AFSEL12, 9);
	// Configure GPIO pins PA11 and PA12 to be very high speed
	GPIOA->OSPEEDR |= REG_FIELD(GPIO_OSPEEDR_OSPEED11, 3) | REG_FIELD(GPIO_OSPEEDR_OSPEED12, 3);
	// Clear port mode configuration for PA11 and PA12, and set them to Alternate function mode
	GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER11 | GPIO_MODER_MODER12)) | REG_FIELD(GPIO_MODER_MODER11, 2) | REG_FIELD(GPIO_MODER_MODER12, 2);
}

#define ALL_FLASH_ERR (FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR | FLASH_SR_WRPERR)

// Page 75 of reference manual, table 5
uint32_t flash_addr_to_page(uint32_t start) {
	if(start < 0x08010000) {
		return (start - 0x08000000) >> 14;
	} else if(start < 0x08020000) {
		return 4;
	} else {
		return ((start - 0x08020000) >> 17) + 5;
	}
}

// Page 85 of reference manual
flash_ret_t flash_erase_page(uint32_t page) {
	while(FLASH->SR & FLASH_SR_BSY); // Wait for ready
	FLASH->CR = (FLASH->CR & ~FLASH_CR_SNB) | REG_FIELD(FLASH_CR_SNB, page) | FLASH_CR_SER;
	FLASH->CR |= FLASH_CR_STRT;
	while(FLASH->SR & FLASH_SR_BSY); // Wait for ready
	FLASH->CR &= ~FLASH_CR_SER;
	if(FLASH->SR & ALL_FLASH_ERR) {
		return FLASH_ERASE_FAIL;
	}
	return FLASH_OK;
}

// Page 86 of reference manual
// Documentation states EOP should be cleared after write success.
// We ignore it here because EOP only gets set when "end of operation" interrupts are enabled (EOPIE = 1)
flash_ret_t flash_write(uint32_t addr, const void *data, uint32_t len) {
	while(FLASH->SR & FLASH_SR_BSY); // Wait for ready
	FLASH->CR |= FLASH_CR_PG;
	uint32_t *dst = (uint32_t *) addr;
	const uint32_t *src = data;
	uint32_t *end = (uint32_t *) ((uint8_t *) src + len);
	while(src < end) {
		*dst = *src;
		src += 1;
		dst += 1;
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
		// x32 parallelism
		FLASH->CR |= REG_FIELD(FLASH_CR_PSIZE, 2);
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
