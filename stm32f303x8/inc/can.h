#pragma once

#include <stdint.h>

void can_init();
void can_tx(uint16_t id, uint8_t* data, uint8_t len);
