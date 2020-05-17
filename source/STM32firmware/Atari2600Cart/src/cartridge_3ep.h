#ifndef CARTRIDGE_3EP_H
#define CARTRIDGE_3EP_H

#include <stdint.h>

/* 3E+ Bankswitching by Thomas Jentzsch */
void emulate_3EPlus_cartridge(uint8_t* buffer, uint32_t image_size);


#endif // CARTRIDGE_3EP_H
