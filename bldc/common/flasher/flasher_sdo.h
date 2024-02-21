#ifndef FLASHER_SDO_H_
#define FLASHER_SDO_H_

#include "platform.h"

enum
{
	/* The target is instructed to stop the running programme (HALT) */
	CO_SDO_FLASHER_W_STOP = 0x00,

	/* The target is instructed to start the selected programme (FAST BOOT) */
	CO_SDO_FLASHER_W_START = 0x01,

	/* The target is instructed to reset the status (index 0x1F57) */
	CO_SDO_FLASHER_W_RESET_STAT = 0x02,

	/* The target is instructed to clear that area of the flash that has been
	   selected with the appropriate sub-index (FLASH ERASE) */
	CO_SDO_FLASHER_W_CLEAR = 0x03,

	/* You can jump back from the application into the bootloader using this
	   command. This entry must therefore also be supported by the application
	   to start the bootloader (refer also to 3.2). "Reboot" */
	CO_SDO_FLASHER_W_START_BOOTLOADER = 0x80,

	/* The target is instructed to flag the selected and programmed program
	   as „valid“. In addition to a valid CRC and node number, this is the
	   requirement to start the program automatically after a power-on-reset */
	CO_SDO_FLASHER_W_SET_SIGNATURE = 0x83,

	/* The target is instructed to flag the selected and programmed program
	   as „invalid“. After a power-on RESET, the application would then not
	   be started; the bootloader remains active. */
	CO_SDO_FLASHER_W_CLR_SIGNATURE = 0x84,

	CO_SDO_FLASHER_CHECK_SIGNATURE = 0x10,
};

enum
{
	/* The last command transmitted has been run without error */
	CO_SDO_FLASHER_R_OK = 0x00000000,

	/* A command is still being run */
	CO_SDO_FLASHER_R_BUSY = 0x00000001,

	/* An attempt has been made to start an invalid application programme */
	CO_SDO_FLASHER_R_NOVALPROG = 0x00000002,

	/* The format of binary data that have been transferred to index 0x1F50
	   is incorrect */
	CO_SDO_FLASHER_R_FORMAT = 0x00000004,

	/* The CRC of the binary data is incorrect */
	CO_SDO_FLASHER_R_CRC = 0x00000006,

	/* An attempt has been made to programme although there is a valid
	   application programme */
	CO_SDO_FLASHER_R_NOTCLEARED = 0x00000008,

	/* An error occurred during the writing of the flash */
	CO_SDO_FLASHER_R_WRITE = 0x0000000A,

	/* An attempt has been made to write an invalid address into the flash */
	CO_SDO_FLASHER_R_ADDRESS = 0x0000000C,

	/* An attempt has been made to write to a protected flash area */
	CO_SDO_FLASHER_R_SECURED = 0x0000000E,

	/* An error has occurred when accessing the non-volatile memory
	   (e.g. programming the signature) */
	CO_SDO_FLASHER_R_NVDATA = 0x00000010,
};

void flasher_sdo_init(void);

#endif // FLASHER_SDO_H_