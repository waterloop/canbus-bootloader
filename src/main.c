#include "clock.h"

int main();

int main() {
    clock_init();

    while (1) { asm("NOP"); }

    return 0;
}