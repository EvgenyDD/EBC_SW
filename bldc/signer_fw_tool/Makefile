SRC += sign.c
INCDIR += .

CFLAGS   += -fdata-sections -ffunction-sections 
CFLAGS   += -fsingle-precision-constant

CFLAGS   += -Wno-pointer-sign
CFLAGS   += -fmessage-length=0
CFLAGS   += -fno-exceptions
CFLAGS   += -fno-move-loop-invariants -ffreestanding

CFLAGS   += -Wall -Wextra -Wstrict-prototypes  -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs
CFLAGS   += -Wshadow -Wsign-conversion -Wstrict-overflow=5 -Wswitch-default -Wno-unused
CFLAGS   += -pedantic -Wcast-qual -Wcast-align

CFLAGS   += -Werror
CFLAGS   += -Wundef 
CFLAGS   += -Wmissing-declarations 
CFLAGS   += -Wredundant-decls

all: $(SRC)
	@gcc $(SRC) $(addprefix -I, $(INCDIR)) $(CFLAGS) -o signer_fw_tool

clean:
	@rm -f signer_fw_tool*