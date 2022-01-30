#include <stdint.h>
#include "bsp.h"
#include "timers.h"

void timers_init() {
    // Enable the TIM2 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // reset the timer
    RCC->APB1RSTR |=  (RCC_APB1RSTR_TIM2RST);
    RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM2RST);

    // divide the 64MHz clock by 64000 to get a 1kHz timer
    TIM2->PSC = 63999;

    // set auto-reload value to max (all 32 bits set to 1)
    TIM2->ARR = 0xffffffff;

    // set to up counting mode
    TIM2->ARR &= ~(TIM_CR1_DIR);

    // make timer still work in debug mode
    DBGMCU->APB1FZ &= ~(DBGMCU_APB1_FZ_DBG_TIM2_STOP);

    // generate an update event to apply newly configured settings
    TIM2->EGR  |= TIM_EGR_UG;

    // start the timer
    TIM2->CR1 |= TIM_CR1_CEN;
}

void delay_ms(uint32_t ms) {
    // set the timer counter value to 0
    TIM2->CNT = 0;

    // wait until the timer counts up to the ms
    // value (it's a 1kHz timer so we can do this)
    while (TIM2->CNT < ms) {
        uint32_t thing = TIM2->CNT;
        thing -= 1;
        asm("NOP");
    }
}
