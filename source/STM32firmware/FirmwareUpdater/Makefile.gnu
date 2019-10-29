# Use the official ARM Cortex GNU toolchain
#     (http//developer.arm.com/open-source/gnu-toolchain/gnu-rm)
# to build the updater binary with this Makefile. Be sure to build the
# actual firmware first.
#
# Makefile derived from https://github.com/nitsky/stm32-example

BUILDDIR = build
DEPDIR = .depend/build

CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
LDLD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

SOURCES = \
	src/startup_stm32f40xx.s \
	src/system_stm32f4xx.c \
	Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_flash.c \
	src/flash.c \
	src/main.c \
	src/stm32f4xx_it.c \
	src/tiny_printf.c

INCLUDES = \
	-Isrc \
	-ILibraries/CMSIS/Include \
	-ILibraries/STM32F4xx_StdPeriph_Driver/inc \
	-ILibraries/Device/STM32F4xx/Include

GARBAGE = $(DEPDIR) $(BUILDDIR)

OBJECTS = \
	$(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SOURCES)))) \
	$(BUILDDIR)/firmware.o $(BUILDDIR)/UpgradeComplete.o

ELF = $(BUILDDIR)/update.elf
BIN = $(BUILDDIR)/update.bin
MAP = $(BUILDDIR)/update.map

CFLAGS = \
	-MT $@ -MMD -MP -MF $(DEPDIR)/$*.d \
	-O2 -Wall -g \
	-mcpu=cortex-m4 -mthumb \
	-mfpu=fpv4-sp-d16 -mhard-float \
	-DSTM32F4XX \
	-DSTM32F40XX \
	-DUSE_STM32F4_DISCOVERY \
	-DHSE_VALUE=8000000 \
	-DUSE_STDPERIPH_DRIVER \
	-DFIRMWARE_SYMBOL_START=_binary_build_firmware_bin_start \
	-DFIRMWARE_SYMBOL_END=_binary_build_firmware_bin_end \
	-DFIRMWARE_SYMBOL_SIZE=_binary_build_firmware_bin_size \
	-ffunction-sections \
	-fdata-sections \
	$(INCLUDES)

LDSCRIPT = stm32f4_flash.ld
LDFLAGS += \
	-T$(LDSCRIPT) \
	-mhard-float \
	-mthumb -mcpu=cortex-m4 \
	--specs=nosys.specs \
	-Wl,--gc-sections \
	-Wl,-Map=$(MAP)

bin: $(BIN)
elf: $(ELF)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(ELF): $(OBJECTS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

$(BUILDDIR)/firmware.o: ../Atari2600Cart/build/firmware.elf
	$(OBJCOPY) -O binary $< $(@:.o=.bin)
	$(OBJCOPY) -I binary -O elf32-littlearm -B arm $(@:.o=.bin) $@

$(BUILDDIR)/UpgradeComplete.o: src/UpgradeComplete.bin
	$(LDLD) -r -b binary $< -o $@

$(BUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	mkdir -p .depend/$(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/%.o: %.s
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(GARBAGE)

.PHONY: bin elf hex flash clean

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SOURCES))))
