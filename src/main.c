#include <stm32l432xx.h>

int main(void) {
	unsigned b = 0;
	for(uint64_t a = 0; a < 0xFFFFFFFFFFFFLLU; ++a) {
		b += 3;
	}
	return b;
}
