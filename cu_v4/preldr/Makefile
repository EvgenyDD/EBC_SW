EXE_NAME=cu_v4_preldr
PRODUCT =cu_v4
VER_MAJ = 1
VER_MIN = 0
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

LDFLAGS  += -specs=nano.specs
LDFLAGS  += -Wl,--gc-sections
LDFLAGS  += -Wl,--print-memory-usage

PPDEFS += USE_STDPERIPH_DRIVER STM32F40_41xxx STM32F407xx

INCDIR += ../common
INCDIR += ../common/CMSIS
INCDIR += ../common/crc
INCDIR += ../common/fw_header
INCDIR += ../common/STM32F4xx_StdPeriph_Driver/inc
SOURCES += $(call rwildcard, ../common/CMSIS ../common/crc ../common/fw_header ../common/STM32F4xx_StdPeriph_Driver, *.c *.S *.s)
SOURCES += $(wildcard ../common/*.c)
SOURCES += preldr.c
LDSCRIPT += ../ldscript/preldr.ld

SIGNED_BINARY = $(EXE_NAME)_signed.bin

ARTEFACTS += $(SIGNED_BINARY)

include ../common/core.mk

signer:
	@make -C ../signer_fw_tool -j

$(SIGNED_BINARY): $(BINARY) signer
	@../signer_fw_tool/signer_fw_tool* $(BINARY) $(BUILDDIR)/$(SIGNED_BINARY) 4096 \
		prod=$(PRODUCT) \
		prod_name=$(EXE_NAME) \
		ver_maj=$(VER_MAJ) \
		ver_min=$(VER_MIN) \
		ver_pat=$(VER_PATCH) \
		build_ts=$$(date -u +'%Y%m%d_%H%M%S')

flash: $(SIGNED_BINARY)
	@openocd -f ../target/stm32f4xx.cfg -c "program $(BUILDDIR)/$(SIGNED_BINARY) 0x08000000 verify reset exit" 

debug:
	@echo "file $(EXECUTABLE)" > .gdbinit
	@echo "set auto-load safe-path /" >> .gdbinit
	@echo "set confirm off" >> .gdbinit
	@echo "target remote | openocd -c \"gdb_port pipe\" -f ../target/stm32f4xx.cfg" >> .gdbinit
	@arm-none-eabi-gdb -q -x .gdbinit