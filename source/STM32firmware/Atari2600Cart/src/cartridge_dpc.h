#ifndef CARTRIDGE_DPC_H
#define CARTRIDGE_DPC_H

#include <stdint.h>
#include <stdbool.h>

bool emulate_dpc_cartridge(uint8_t* buffer, uint32_t image_size);

#endif // CARTRIDGE_DPC_H
