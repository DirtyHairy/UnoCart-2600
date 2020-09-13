#include <stdbool.h>
#include <stdint.h>

#include "cartridge_io.h"

void emulate_cartridge(uint8_t* buffer, uint8_t* cart_ram) {
    __disable_irq();	// Disable interrupts

	uint16_t addr, addr_prev = 0;
	while (1)
	{
		while ((addr = ADDR_IN) != addr_prev)
			addr_prev = addr;
		// got a stable address
		if (addr & 0x1000)
		{ // A12 high
			DATA_OUT = ((uint16_t)buffer[addr&0xFFF])<<8;
			SET_DATA_MODE_OUT
			// wait for address bus to change
			while (ADDR_IN == addr) ;
			SET_DATA_MODE_IN
		}
	}

	__enable_irq();
}
