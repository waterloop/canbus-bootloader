#include "bsp.h"

int main();

int main() {
    clock_init();
    timers_init();

    while (1) {
        delay_ms(10000);
        asm("NOP");
    }

    return 0;
}