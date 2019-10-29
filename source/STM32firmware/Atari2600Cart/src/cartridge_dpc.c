#include <string.h>

#include "cartridge_dpc.h"
#include "cartridge_firmware.h"

#define UPDATE_MUSIC_FETCHERS { \
    uint32_t sysTick = SysTick->VAL; \
    if (sysTick > lastSysTick) \
    { \
        if (++DpcClocks1 > DpcTops[5]) DpcClocks1 = 0; \
        if (++DpcClocks2 > DpcTops[6]) DpcClocks2 = 0; \
        if (++DpcClocks3 > DpcTops[7]) DpcClocks3 = 0; \
    } else {\
        DpcMusicFlags[0] = DpcClocks1 > DpcBottoms[5] ? 1 : 0; \
        DpcMusicFlags[1] = DpcClocks2 > DpcBottoms[6] ? 2 : 0; \
        DpcMusicFlags[2] = DpcClocks3 > DpcBottoms[7] ? 4 : 0; \
    } \
    lastSysTick = sysTick; \
}

#define CCM_RAM ((uint8_t*)0x10000000)

bool emulate_dpc_cartridge(uint8_t* buffer, uint32_t image_size)
{
	SysTick_Config(SystemCoreClock / 21000);	// 21KHz

    uint8_t* ccm = CCM_RAM;

	int soundAmplitudeIndex = 0;

    uint8_t* soundAmplitudes = ccm;
    soundAmplitudes[0] = 0x00;
    soundAmplitudes[1] = 0x04;
    soundAmplitudes[2] = 0x05;
    soundAmplitudes[3] = 0x09;
    soundAmplitudes[4] = 0x06;
    soundAmplitudes[5] = 0x0a;
    soundAmplitudes[6] = 0x0b;
    soundAmplitudes[7] = 0x0f;
    ccm += 8;

    uint8_t* DpcTops = ccm;
    ccm += 8;

    uint8_t* DpcBottoms = ccm;
    ccm += 8;

    uint8_t* DpcFlags = ccm;
    ccm += 8;

    uint16_t* DpcCounters = (void*)ccm;
    ccm += 16;

    int* DpcMusicModes = (void*)ccm;
    ccm += 12;

    int* DpcMusicFlags = (void*)ccm;
    ccm += 12;

    memcpy(ccm, buffer, image_size);
    buffer = ccm;
    ccm += image_size;

	uint16_t addr, addr_prev = 0, data = 0, data_prev = 0;
	unsigned char *bankPtr = buffer, *DpcDisplayPtr = buffer + 8*1024;

	// Initialise the DPC's random number generator register (must be non-zero)
	int DpcRandom = 1;

	// Initialise the DPC registers
	for(int i = 0; i < 8; ++i)
		DpcTops[i] = DpcBottoms[i] = DpcCounters[i] = DpcFlags[i] = 0;

	DpcMusicModes[0] = DpcMusicModes[1] = DpcMusicModes[2] = 0;
	DpcMusicFlags[0] = DpcMusicFlags[1] = DpcMusicFlags[2] = 0;


	uint32_t lastSysTick = SysTick->VAL;
	uint32_t DpcClocks1 = 0, DpcClocks2 = 0, DpcClocks3 = 0;

    if (!reboot_into_cartridge()) {
        return false;
    }

    __disable_irq();	// Disable interrupts

	while (1)
	{
		while ((addr = ADDR_IN) != addr_prev)
			addr_prev = addr;

		// got a stable address
		if (addr & 0x1000)
		{ // A12 high

			if (addr < 0x1040)
			{	// DPC read
				unsigned char index = addr & 0x07;
				unsigned char function = (addr >> 3) & 0x07;

				unsigned char result = 0;
				switch (function)
				{
					case 0x00:
					{
						if(index < 4)
						{	// random number read
							DpcRandom ^= DpcRandom << 3;
							DpcRandom ^= DpcRandom >> 5;
							result = (unsigned char)DpcRandom;
						}
						else
						{	// sound
							soundAmplitudeIndex = (DpcMusicModes[0] & DpcMusicFlags[0]);
							soundAmplitudeIndex |=  (DpcMusicModes[1] & DpcMusicFlags[1]);
							soundAmplitudeIndex |=  (DpcMusicModes[2] & DpcMusicFlags[2]);
							result = soundAmplitudes[soundAmplitudeIndex];;
						}
						break;
					}

					case 0x01:
					{	// DFx display data read
						result = DpcDisplayPtr[2047 - DpcCounters[index]];
						break;
					}

					case 0x02:
					{	// DFx display data read AND'd w/flag
						result = DpcDisplayPtr[2047 - DpcCounters[index]] & DpcFlags[index];
						break;
					}

					case 0x07:
					{	// DFx flag
						result = DpcFlags[index];
						break;
					}
				}

				DATA_OUT = ((uint16_t)result)<<8;
				SET_DATA_MODE_OUT
				// wait for address bus to change

				// Clock the selected data fetcher's counter if needed
				if ((index < 5) || ((index >= 5) && (!DpcMusicModes[index - 5]))) {
					DpcCounters[index] = (DpcCounters[index] - 1) & 0x07ff;

					// Update flag register for selected data fetcher
					if((DpcCounters[index] & 0x00ff) == DpcTops[index])
						DpcFlags[index] = 0xff;
					else if((DpcCounters[index] & 0x00ff) == DpcBottoms[index])
						DpcFlags[index] = 0x00;
				}

                UPDATE_MUSIC_FETCHERS;

				while (ADDR_IN == addr) ;
				SET_DATA_MODE_IN
			}
			else if (addr < 0x1080)
			{	// DPC write
				unsigned char index = addr & 0x07;
				unsigned char function = (addr >> 3) & 0x07;
				unsigned char ctr = DpcCounters[index] & 0xff;

                UPDATE_MUSIC_FETCHERS;

				while (ADDR_IN == addr) { data_prev = data; data = DATA_IN; }
				unsigned char value = data_prev>>8;
				switch (function)
				{
					case 0x00:
					{	// DFx top count
						DpcTops[index] = value;
						DpcFlags[index] = 0x00;

						if(ctr == value)
							DpcFlags[index] = 0xff;

						break;
					}

					case 0x01:
					{	// DFx bottom count
						DpcBottoms[index] = value;

						if(ctr == value)
							DpcFlags[index] = 0x00;

						break;
					}

					case 0x02:
					{	// DFx counter low
						DpcCounters[index] = (DpcCounters[index] & 0x0700) | value;

						if (value == DpcTops[index])
							DpcFlags[index] = 0xff;
						else if(value == DpcBottoms[index])
							DpcFlags[index] = 0x00;

						break;
					}

					case 0x03:
					{	// DFx counter high
						DpcCounters[index] = (((uint16_t)(value & 0x07)) << 8) | ctr;
						if(index >= 5)
							DpcMusicModes[index - 5] = (value & 0x10) ? 0x7 : 0;

						break;
					}

					case 0x06:
					{	// Random Number Generator Reset
						DpcRandom = 1;
						break;
					}
				}
			}
			else
			{	// check bank-switch
				if (addr == 0x1FF8)
					bankPtr = buffer;
				else if (addr == 0x1FF9)
					bankPtr = buffer + 4*1024;

				// normal rom access
				DATA_OUT = ((uint16_t)bankPtr[addr&0xFFF])<<8;
				SET_DATA_MODE_OUT

				UPDATE_MUSIC_FETCHERS;

				while (ADDR_IN == addr) ;
				SET_DATA_MODE_IN
			}
		}
	}

	__enable_irq();
    return true;
}
