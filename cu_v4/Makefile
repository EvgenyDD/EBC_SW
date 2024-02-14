EXE_NAME=cu_v4
PRODUCT = cu_v4
VER_MAJ =   1
VER_MIN =   0
VER_PATCH = 0
MAKE_BINARY=yes

TCHAIN = arm-none-eabi-
MCPU += -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
CDIALECT = gnu99
OPT_LVL = 2
DBG_OPTS = -gdwarf-2 -ggdb -g

CFLAGS   += -fdata-sections -ffunction-sections 
CFLAGS   += -fsingle-precision-constant
CFLAGS   += -fmessage-length=0
CFLAGS   += -fno-exceptions -funsafe-math-optimizations
CFLAGS   += -fno-move-loop-invariants -ffreestanding
CFLAGS   += -Wno-pointer-sign -Wswitch-default -Wshadow -Wno-unused
CFLAGS   += -Wall -Wstrict-prototypes -Wdisabled-optimization -Wformat=2 -Winit-self  -Wmissing-include-dirs
CFLAGS   += -Wstrict-overflow=2
CFLAGS   += -Werror

# LDFLAGS  += -nostartfiles
LDFLAGS  += -specs=nano.specs
LDFLAGS  += -Wl,--gc-sections
LDFLAGS  += -Wl,--print-memory-usage

EXT_LIBS +=c m nosys

PPDEFS += USE_STDPERIPH_DRIVER STM32F40_41xxx STM32F407xx FW_TYPE=FW_APP CFG_SYSTEM_SAVES_NON_NATIVE_DATA=1 DEV=\"EBCU_APP\"

PPDEFS  += CO_USE_GLOBALS
PPDEFS  += CO_CONFIG_GFC=0
PPDEFS  += CO_CONFIG_GTW=0
PPDEFS  += CO_CONFIG_HB_CONS=0
PPDEFS  += CO_CONFIG_LEDS=0
PPDEFS  += CO_CONFIG_SRDO=0
# PPDEFS  += CO_CONFIG_SDO_CLI=0
PPDEFS  += CO_CONFIG_SDO_SRV="CO_CONFIG_SDO_SRV_SEGMENTED|CO_CONFIG_GLOBAL_FLAG_CALLBACK_PRE"
PPDEFS  += CO_CONFIG_TIME=0
PPDEFS  += CO_CONFIG_TRACE=0
PPDEFS  += CO_CONFIG_SDO_SRV_BUFFER_SIZE=534

INCDIR += app
INCDIR += app/ili9481
INCDIR += common
INCDIR += common/adc
INCDIR += common/CMSIS
INCDIR += common/canopendriver
INCDIR += common/canopennode
INCDIR += common/config_system
INCDIR += common/crc
INCDIR += common/fw_header
INCDIR += common/i2c
INCDIR += common/spi
INCDIR += common/STM32_USB_Device_Library/Core
INCDIR += common/STM32_USB_OTG_Driver
INCDIR += common/STM32F4xx_StdPeriph_Driver/inc
INCDIR += common/usb

SOURCES += $(call rwildcard, app, *.c *.S *.s)
SOURCES += $(call rwildcard, common/adc, *.c *.S *.s)
SOURCES += $(call rwildcard, common/canopendriver, *.c *.S *.s)
SOURCES += $(call rwildcard, common/canopennode, *.c *.S *.s)
SOURCES += $(call rwildcard, common/CMSIS, *.c *.S *.s)
SOURCES += $(call rwildcard, common/config_system, *.c *.S *.s)
SOURCES += $(call rwildcard, common/crc, *.c *.S *.s)
SOURCES += $(call rwildcard, common/fw_header, *.c *.S *.s)
SOURCES += $(call rwildcard, common/i2c, *.c *.S *.s)
SOURCES += $(call rwildcard, common/spi, *.c *.S *.s)
SOURCES += $(call rwildcard, common/STM32_USB_Device_Library/Core, *.c *.S *.s)
SOURCES += $(call rwildcard, common/STM32_USB_OTG_Driver, *.c *.S *.s)
SOURCES += $(call rwildcard, common/STM32F4xx_StdPeriph_Driver, *.c *.S *.s)
SOURCES += $(call rwildcard, common/usb, *.c *.S *.s)
SOURCES += $(wildcard common/*.c)

LDSCRIPT += ldscript/app.ld

BINARY_SIGNED = $(BUILDDIR)/$(EXE_NAME)_app_signed.bin
BINARY_MERGED = $(BUILDDIR)/$(EXE_NAME)_full.bin

ARTEFACTS += $(BINARY_SIGNED) $(BINARY_MERGED)

include common/core.mk

signer:
	@make -C signer_fw_tool -j

$(BINARY_SIGNED): $(BINARY) signer
	@signer_fw_tool/signer_fw_tool* $(BUILDDIR)/$(EXE_NAME).bin $(BUILDDIR)/$(EXE_NAME)_app_signed.bin 503808 \
		prod=$(PRODUCT) \
		prod_name=$(EXE_NAME)_app \
		ver_maj=$(VER_MAJ) \
		ver_min=$(VER_MIN) \
		ver_pat=$(VER_PATCH) \
		build_ts=$$(date -u +'%Y%m%d_%H%M%S')

PRELDR_SIGNED = preldr/build/$(EXE_NAME)_preldr_signed.bin
LDR_SIGNED = ldr/build/$(EXE_NAME)_ldr_signed.bin 
EXT_MAKE_TARGETS = $(PRELDR_SIGNED) $(LDR_SIGNED)

clean: clean_ext_targets

.PHONY: $(EXT_MAKE_TARGETS)
$(EXT_MAKE_TARGETS):
	$(MAKE) -C $(subst build/,,$(dir $@))

.PHONY: clean_ext_targets
clean_ext_targets:
	$(foreach var,$(EXT_MAKE_TARGETS),$(MAKE) -C $(subst build/,,$(dir $(var))) clean;)

$(BINARY_MERGED): $(EXT_MAKE_TARGETS) $(BINARY_SIGNED)
	@echo " [Merging binaries] ..."
	@cp -f $(PRELDR_SIGNED) $@
	@dd if=$(LDR_SIGNED) of=$@ bs=1 oflag=append seek=16384 status=none
	@dd if=$(BINARY_SIGNED) of=$@ bs=1 oflag=append seek=131072 status=none

#####################
### FLASH & DEBUG ###
#####################

flash: $(BINARY_SIGNED)
	openocd -f target/stm32f4xx.cfg -c "program $(BINARY_SIGNED) 0x08020000 verify reset exit" 

flash_full: $(BINARY_MERGED)
	openocd -f target/stm32f4xx.cfg -c "program $(BINARY_MERGED) 0x08000000 verify reset exit"

program: $(BINARY_SIGNED)
	@usb_dfu_flasher $< EBCU

debug:
	@echo "file $(EXECUTABLE)" > .gdbinit
	@echo "set auto-load safe-path /" >> .gdbinit
	@echo "set confirm off" >> .gdbinit
	@echo "target remote | openocd -c \"gdb_port pipe\" -f target/stm32f4xx.cfg" >> .gdbinit
	@arm-none-eabi-gdb -q -x .gdbinit