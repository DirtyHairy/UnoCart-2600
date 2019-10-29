#include <stdbool.h>

#include "tm_stm32f4_fatfs.h"

#include "cartridge_df.h"
#include "cartridge_firmware.h"

#define BUFFER_SIZE_KB 96
#define CCM_SIZE_KB 64

#define RAM_BANKS ((BUFFER_SIZE_KB / 4) - 1)
#define CCM_BANKS (32 - RAM_BANKS)

#define AVAILABLE_RAM_BASE (RAM_BANKS * 4096)

#define CCM_RAM ((uint8_t*)0x10000000)
#define CCM_SIZE (CCM_SIZE_KB * 1024)

#define CCM_IMAGE_OFFSET (RAM_BANKS * 4096)
#define CCM_IMAGE_SIZE (CCM_BANKS * 4096)

#define STARTUP_BANK_BF 1
#define STARTUP_BANK_BFSC 15

typedef struct {
    uint8_t* banks[32];
} cartridge_layout;

static bool setup_cartridge_image(const char* filename, uint32_t image_size, uint8_t* buffer, cartridge_layout* layout) {
    if (image_size != 128*1024) return false;

    FATFS fs;
	FIL fil;
	UINT bytes_read;

    if (f_mount(&fs, "", 1) != FR_OK) goto fail_unmount;
	if (f_open(&fil, filename, FA_READ) != FR_OK) goto fail_close;

    if (f_lseek(&fil, CCM_IMAGE_OFFSET) != FR_OK) goto fail_close;
    if (f_read(&fil, CCM_RAM, CCM_IMAGE_SIZE, &bytes_read) != FR_OK) goto fail_close;
    if (bytes_read != CCM_IMAGE_SIZE) goto fail_close;

    for (uint8_t i = 0; i < RAM_BANKS; i++) layout->banks[i] = buffer + i * 4096;
    for (uint8_t i = 0; i < CCM_BANKS; i++) layout->banks[RAM_BANKS + i] = CCM_RAM + i * 4096;

    f_close(&fil);
	f_mount(0, "", 1);
	return true;

	fail_close:
		f_close(&fil);
	fail_unmount:
		f_mount(0, "", 1);

	return false;
}

void emulate_dfsc_cartridge(const char* filename, uint32_t image_size, uint8_t* buffer) {
    uint8_t *ram_base = buffer + AVAILABLE_RAM_BASE;

    cartridge_layout* layout = (void*)ram_base;
    ram_base += sizeof(cartridge_layout);

    uint8_t* ram = ram_base;

    if (!setup_cartridge_image(filename, image_size, buffer, layout)) return;

    uint8_t *bank = layout->banks[STARTUP_BANK_BFSC];

    if (!reboot_into_cartridge()) return;
    __disable_irq();

    uint16_t addr, addr_prev = 0, addr_prev2 = 0, data = 0, data_prev = 0;

	while (1)
	{
		while (((addr = ADDR_IN) != addr_prev) || (addr != addr_prev2))
		{
			addr_prev2 = addr_prev;
			addr_prev = addr;
		}

        if (!(addr & 0x1000)) continue;

        uint16_t address = addr & 0x0fff;

        if (address < 0x80) {
            while (ADDR_IN == addr) { data_prev = data; data = DATA_IN; }
			data = data_prev>>8;

            ram[address] = data;
        } else {
            if (address >= 0x0fc0 && address <= 0x0fdf) bank = layout->banks[address - 0x0fc0];

            data = (address < 0x0100) ? ram[address & 0x7f] : bank[address];

            DATA_OUT = ((uint16_t)data)<<8;
			SET_DATA_MODE_OUT
			// wait for address bus to change
			while (ADDR_IN == addr) ;
			SET_DATA_MODE_IN
        }
    }
}

void emulate_df_cartridge(const char* filename, uint32_t image_size, uint8_t* buffer) {
    uint8_t *ram_base = buffer + AVAILABLE_RAM_BASE;
    cartridge_layout* layout = (void*)ram_base;

    if (!setup_cartridge_image(filename, image_size, buffer, layout)) return;

    uint8_t *bank = layout->banks[STARTUP_BANK_BF];

    if (!reboot_into_cartridge()) return;
    __disable_irq();

    uint16_t addr, addr_prev = 0, addr_prev2 = 0;

	while (1)
	{
		while (((addr = ADDR_IN) != addr_prev) || (addr != addr_prev2))
		{
			addr_prev2 = addr_prev;
			addr_prev = addr;
		}

        if (!(addr & 0x1000)) continue;

        uint16_t address = addr & 0x0fff;

        if (address >= 0x0fc0 && address <= 0x0fdf) bank = layout->banks[address - 0x0fc0];

        DATA_OUT = ((uint16_t)bank[address])<<8;
        SET_DATA_MODE_OUT
        // wait for address bus to change
        while (ADDR_IN == addr) ;
        SET_DATA_MODE_IN
    }
}
