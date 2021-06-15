#include "utils.h"
#include "system_inc.h"

void SystemInit(void) {
	// Disable interrupts
	__disable_irq();

	// Set FLASH configurations to prepare for 72MHz
	// Enable prefetch, latency 2 wait states
	FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

	// Clock configuration begin
	// Enable GPIOF clock, for external oscillator
	RCC->AHBENR |= RCC_AHBENR_GPIOFEN;
	// Enable HSE external oscillator and turn on Clock Security System
	RCC->CR |= RCC_CR_HSEON | RCC_CR_CSSON;
	// PLLMul = 9, Select HSE as input to PLL, APB1 = HCLK / 2
	RCC->CFGR |= RCC_CFGR_PLLMUL9 | RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR_PPRE1_DIV2;
	// HSE PREDIV = 2
	RCC->CFGR2 |= RCC_CFGR2_PREDIV_DIV2;
	// Turn on PLL
	RCC->CR |= RCC_CR_PLLON;
	// Wait for HSE and PLL lock
	while((RCC->CR & (RCC_CR_PLLRDY | RCC_CR_HSERDY)) != (RCC_CR_PLLRDY | RCC_CR_HSERDY));
	// Switch SYSCLK over to PLL output
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	// Clock configuration complete

	// Enable interrupts
	__enable_irq();
}

void canbus_setup(void) {
	// Enable APB1 clock for CAN
	RCC->APB1ENR |= RCC_APB1ENR_CANEN;
	// Enable AHB clock for GPIO Port A
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	// Clear alt functions for PA11 and PA12, and set them to alt function 9
	GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(GPIO_AFRH_AFRH3 | GPIO_AFRH_AFRH4)) | REG_FIELD(GPIO_AFRH_AFRH3, 9) | REG_FIELD(GPIO_AFRH_AFRH4, 9);
	// Configure GPIO pins PA11 and PA12 to be very high speed
	GPIOA->OSPEEDR |= REG_FIELD(GPIO_OSPEEDER_OSPEEDR11, 3) | REG_FIELD(GPIO_OSPEEDER_OSPEEDR12, 3);
	// Clear port mode configuration for PA11 and PA12, and set them to Alternate function mode
	GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER11 | GPIO_MODER_MODER12)) | REG_FIELD(GPIO_MODER_MODER11, 2) | REG_FIELD(GPIO_MODER_MODER12, 2);
}

void crc_setup(void) {
	RCC->AHBENR |= RCC_AHBENR_CRCEN;
	CRC->CR |= CRC_CR_REV_OUT | REG_FIELD(CRC_CR_REV_IN, 3);
}

#define ALL_FLASH_ERR (FLASH_SR_WRPERR | FLASH_SR_PGERR)

// Page 70 of reference manual
flash_ret_t flash_erase_page(uint32_t page) {
	while(FLASH->SR & FLASH_SR_BSY); // Wait for ready
	FLASH->CR |= FLASH_CR_PER;
	FLASH->AR = page;
	FLASH->CR |= FLASH_CR_STRT;
	__asm("nop"); // Wait one CPU cycle before checking BSY bit
	while(FLASH->SR & FLASH_SR_BSY); // Wait for ready
	FLASH->CR &= ~FLASH_CR_PER;
	if(FLASH->SR & ALL_FLASH_ERR) {
		return FLASH_ERASE_FAIL;
	}
	FLASH->SR |= FLASH_SR_EOP; // Clear EOP
	return FLASH_OK;
}

flash_ret_t flash_write(void *addr, const void *data, uint32_t len) {
	while(FLASH->SR & FLASH_SR_BSY); // Wait for ready
	FLASH->CR |= FLASH_CR_PG;
	uint16_t *dst = addr;
	const uint16_t *src = data;
	uint16_t *end = (uint16_t *) ((uint8_t *) src + len);
	while(src < end) {
		*dst = *src;
		src += 1;
		dst += 1;
		while(FLASH->SR & FLASH_SR_BSY_Msk);
		// Check for success
		if(FLASH->SR & ALL_FLASH_ERR) {
			FLASH->SR |= FLASH_SR_EOP;
			FLASH->CR &= ~FLASH_CR_PG;
			return FLASH_WRITE_FAIL;
		}
		FLASH->SR |= FLASH_SR_EOP;
	}
	FLASH->CR &= ~FLASH_CR_PG;
	return FLASH_OK;
}

