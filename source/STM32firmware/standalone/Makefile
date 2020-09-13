# Use the official ARM Cortex GNU toolchain
#     (http//developer.arm.com/open-source/gnu-toolchain/gnu-rm)
# and the open source stlink tool suite
#     (https://github.com/texane/stlink)
# to build and flash the firmware with this Makefile.
#
# Makefile derived from https://github.com/nitsky/stm32-example

BUILDDIR = build
DEPDIR = .depend/build

CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

STFLASH = st-flash

SOURCES_MAIN = \
	src/startup_stm32f40xx.s \
	src/system_stm32f4xx.c \
	Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c \
	Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c \
	src/main.c

INCLUDES = \
	-Isrc \
	-ILibraries/CMSIS/Include \
	-ILibraries/Device/STM32F4xx/Include \
	-ILibraries/STM32F4xx_StdPeriph_Driver/inc

GARBAGE = $(DEPDIR) $(BUILDDIR)

OBJECTS_MAIN = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SOURCES_MAIN))))

DRIVERS = $(BUILDDIR)/driver_4k.bin $(BUILDDIR)/driver_3ep_32k.bin

CFLAGS = \
	-MT $@ -MMD -MP -MF $(DEPDIR)/$*.d \
	-O2 -fno-tree-vrp -g \
	-mcpu=cortex-m4 -mthumb \
	-mfpu=fpv4-sp-d16 -mhard-float \
	-DSTM32F4XX \
	-DSTM32F40XX \
	-DUSE_STM32F4_DISCOVERY \
	-DHSE_VALUE=8000000 \
	-DUSE_STDPERIPH_DRIVER \
	-ffunction-sections \
	-fdata-sections \
	$(INCLUDES)

LDSCRIPT = stm32f4_flash.ld
LDFLAGS += \
	-T$(LDSCRIPT) \
	-mthumb -mcpu=cortex-m4 \
	-mhard-float \
	--specs=nosys.specs \
	-Wl,--gc-sections

all: $(DRIVERS)

$(BUILDDIR)/%.bin: $(BUILDDIR)/%.elf
	$(OBJCOPY) -O binary $< $@
	truncate -s 16k $@

$(BUILDDIR)/%.hex: $(BUILDDIR)/%.elf
	$(OBJCOPY) -O ihex $< $@

$(BUILDDIR)/%.elf: $(OBJECTS_MAIN) $(BUILDDIR)/src/%.o $(LDSCRIPT)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS_MAIN) $(BUILDDIR)/src/$(notdir $(@:.elf=.o)) -Wl,-Map=$(@:.elf=.map)

$(BUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	mkdir -p .depend/$(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/%.o: %.s
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(GARBAGE)

.PHONY: build

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SOURCES))))
