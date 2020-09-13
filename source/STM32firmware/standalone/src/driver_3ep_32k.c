#include "driver_3ep.h"

void emulate_cartridge(uint8_t* buffer, uint8_t* cart_ram) {
	emulate_cartridge_generic(buffer, 32 * 1024, cart_ram);
}
