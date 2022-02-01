#include "bsp.h"

int main();

int main() {
    clock_init();
    timers_init();
    can_init();

    uint16_t id = 0x069;
    uint8_t data[8] = {0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69};
    uint8_t len = 8;

    while (1) {
        can_tx(id, data, len);
        delay_ms(300);
    }

    return 0;
}