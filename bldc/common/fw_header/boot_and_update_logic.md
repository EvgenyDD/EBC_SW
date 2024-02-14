Preldr Algorithm:
    1. Preldr checks CRC & validates the fields of bootloader & application
    2. Preldr determines the load source (LD_SRC) by the #0 cell in retention memory
    3. If LD_SRC == BOOT and application is valid then preldr
        a. sets LD_SRC to NONE
        b. runs application
    4. If bootloader is valid then 
        a. sets LD_SRC to NONE
        b. preldr runs bootloader
    5. If application is valid then
        a. sets LD_SRC to NONE
        b. preldr runs application


Boot Algorithm:
    1. Sets LD_SRC to BOOT
    2. Validates application
    3. If application is not valid than sets Timeout to INFINITY else sets Timeout to DEFAULT_TIMEOUT
    4. If timeout is elapsed than resets the micricontroller (to go to preldr)


Boot Update Algorithm:
    1. Determine the execution code by reading 3rd string if SII_EEPROM ["name" field]
    2. Ife execution code is application
        a. Upload new "boot" file with firmware
        b. (Optional) reboot
    3. If execution code is bootloader
        a. Reboot
        b. Wait execution code readout
        c. If execution code is application
            1) Upload new "boot" file with bootloader firmware
            2) (Optional) reboot
        d. If execution code is bootloader ("everything-is-bad variant")
            1) Upload new "app" file with application firmware
            2) Reboot
            3) Upload new "boot" file with bootloader firmware
            4) (Optional) reboot


Appliction Update Algorithm:
    1. Determine the execution code by reading 3rd string if SII_EEPROM ["name" field]
    2. Ife execution code is bootloader
        a. Upload new "app" file with firmware
        b. (Optional) reboot
    3. If execution code is application
        a. Reboot
        b. Wait execution code readout
        c. If execution code is bootloader
            1) Upload new "app" file with application firmware
            2) (Optional) reboot
        d. If execution code is application ("everything-is-bad variant")
            1) Upload new "boot" file with bootloader firmware
            2) Reboot
            3) Upload new "app" file with application firmware
            4) (Optional) reboot


Risks:
    1. Upload firmware which sets LD_SRC to BOOT => than boot will be never execute while the power is active


Firmware structure:
    1 Section: Vectors
    2 Section: Header: (see fw_header_v1_t) 0x300U offset from the firmware entry point
        a. size
        b. crc32 (without Header)
        c. fields offset from the firmware entry point
    3 Section: firmware
    4 Section: fields


Fields section format:
    {
        string key
        null terminator
        string value
        null terminator
    } x N times
    null terminator