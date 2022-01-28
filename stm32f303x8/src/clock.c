#include "stm32f3xx.h"
#include "clock.h"

uint8_t clock_config() {
    __disable_irq();

    // set FLASH to prepare for 64MHz
    // enable prefetch, latency 2 wait states
    FLASH->ACR |= (FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2);

    // begin clock config...
    // enable HSE external oscillator, turn on clock security system
    RCC->CR |= (RCC_CR_HSEON | RCC_CR_CSSON);

    // set HSE to input to PLL, PLL multiplier is 4, APB1 prescalar is 2
    // 16MHz * 4 = 64MHz for SYSCLK
    RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR_PLLMUL4 | RCC_CFGR_PPRE1_DIV2);

    // turn on PLL
    RCC->CR |= RCC_CR_PLLON;

    // wait for PLL to lock
    while((RCC->CR & (RCC_CR_PLLRDY | RCC_CR_HSERDY)) != (RCC_CR_PLLRDY | RCC_CR_HSERDY)) { asm("NOP"); }

    // switch SYSCLK to PLL
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    __enable_irq();
}