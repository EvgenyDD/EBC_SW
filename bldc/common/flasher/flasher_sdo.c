#include "flasher_sdo.h"
#include "CANopen.h"
#include "OD.h"
#include "crc.h"
#include "fw_header.h"
#include "platform.h"

/// CANopen Bootloader | Edition August 2008 | SYS TEC electronic GmbH 2008
/// https://www.systec-electronic.com/fileadmin/Redakteur/Unternehmen/Support/Downloadbereich/Handbuecher/CANopen-BootloaderSoftware_Manual_L-1112e_05.pdf

#if FW_TYPE == FW_LDR
#define FW_TARGET FW_APP
#define ADDR_ORIGIN ((uint32_t)&__app_start)
#define ADDR_END ((uint32_t)&__app_end)
#elif FW_TYPE == FW_APP
#define FW_TARGET FW_LDR
#define ADDR_ORIGIN ((uint32_t)&__ldr_start)
#define ADDR_END ((uint32_t)&__ldr_end)
#endif

extern bool g_stay_in_boot;

static ODR_t flash_cmd_cb(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten)
{
	g_stay_in_boot = true;

	ODR_t err;
	if((err = OD_writeOriginal(stream, buf, count, countWritten)) != ODR_OK) return err;

	switch(OD_RAM.x1F51_programControl.command)
	{
	case CO_SDO_FLASHER_CHECK_SIGNATURE:
		fw_header_check_all();
		return g_fw_info[FW_TARGET].locked ? ODR_INVALID_VALUE : ODR_OK;

	/* The target is instructed to stop the running programme */
	case CO_SDO_FLASHER_W_STOP:
		g_stay_in_boot = true;
		OD_RAM.x1F57_flashStatusIdentification.error = CO_SDO_FLASHER_R_OK;
		break;

	/* The target is instructed to start the selected programme */
	case CO_SDO_FLASHER_W_START:
		fw_header_check_all();
		if(g_fw_info[FW_TARGET].locked)
		{
			OD_RAM.x1F57_flashStatusIdentification.error = CO_SDO_FLASHER_R_NOVALPROG;
		}
		else
		{
			platform_reset();
		}
		break;

	/* The target is instructed to reset the status (index 0x1F57) */
	case CO_SDO_FLASHER_W_RESET_STAT:
		g_stay_in_boot = true;
		OD_RAM.x1F57_flashStatusIdentification.error = CO_SDO_FLASHER_R_OK;
		break;

	/* The target is instructed to clear that area of the flash that has been
	   selected with the appropriate sub-index (FLASH ERASE) */
	case CO_SDO_FLASHER_W_CLEAR:
		g_stay_in_boot = true;
		platform_flash_erase_flag_reset();
		OD_RAM.x1F57_flashStatusIdentification.error = CO_SDO_FLASHER_R_OK; // no operation
		break;

	/* You can jump back from the application into the bootloader using this
	   command. This entry must therefore also be supported by the application
	   to start the bootloader (refer also to 3.2) */
	case CO_SDO_FLASHER_W_START_BOOTLOADER:
#if FW_TYPE == FW_LDR
		OD_RAM.x1F57_flashStatusIdentification.error = CO_SDO_FLASHER_R_OK; // no operation in bootloader
#else
		fw_header_check_all();
		if(g_fw_info[FW_LDR].locked)
		{
			OD_RAM.x1F57_flashStatusIdentification.error = CO_SDO_FLASHER_R_NOVALPROG;
		}
		else
		{
			platform_reset();
		}
#endif
		break;

	default: return ODR_GENERAL;
	}
	return ODR_OK;
}

static ODR_t flash_data_cb(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten /* ignore */)
{
	g_stay_in_boot = true;

	if(count < 16 + 1) return ODR_DATA_SHORT; // at least 1 block data byte

#define BLCK_OFF_NUM (0U)
#define BLCK_OFF_ADDR (4U)
#define BLCK_OFF_SIZE (8U)

	uint32_t block_num;
	uint32_t block_address;
	uint32_t block_size_data;
	uint32_t block_crc;
	memcpy(&block_num, &(((const uint8_t *)buf)[BLCK_OFF_NUM]), 4);
	memcpy(&block_address, &(((const uint8_t *)buf)[BLCK_OFF_ADDR]), 4);
	memcpy(&block_size_data, &(((const uint8_t *)buf)[BLCK_OFF_SIZE]), 4);

#define BLCK_OFF_DATA (12U)
#define BLCK_OFF_CRC (BLCK_OFF_DATA + block_size_data)

	if(count != 16 + block_size_data) return ODR_TYPE_MISMATCH; // block layout error

	memcpy((uint8_t *)&block_crc, &(((const uint8_t *)buf)[BLCK_OFF_CRC]), 4);
	uint32_t crc_calc = crc32((const uint8_t *)buf, BLCK_OFF_CRC);

	OD_RAM.x1F56_appSoftIdentification.crc = crc_calc;

	if(crc_calc != block_crc)
	{
		OD_RAM.x1F57_flashStatusIdentification.error = CO_SDO_FLASHER_R_CRC;
		return ODR_INVALID_VALUE;
	}

	uint32_t base_offset = (block_address > FLASH_SIZE) ? 0U		   // absolute addresation is used
														: ADDR_ORIGIN; // relative addresation is used

	// check that flashing type was not change enexpectedly and was not get out of the range
	if(base_offset + block_address < ADDR_ORIGIN || base_offset + block_address + block_size_data > ADDR_END)
	{
		OD_RAM.x1F57_flashStatusIdentification.error = CO_SDO_FLASHER_R_SECURED;
		return ODR_INVALID_VALUE;
	}

	if(block_num == 0) platform_flash_erase_flag_reset();
	int sts_flash = platform_flash_write(base_offset + block_address, &(((const uint8_t *)buf)[BLCK_OFF_DATA]), block_size_data);
	if(sts_flash)
	{
		OD_RAM.x1F57_flashStatusIdentification.error = CO_SDO_FLASHER_R_WRITE;
		return ODR_GENERAL;
	}

	OD_RAM.x1F57_flashStatusIdentification.error = CO_SDO_FLASHER_R_OK;

	return ODR_OK;
}

static OD_extension_t OD_1F50_extension = {.object = NULL, .read = OD_readOriginal, .write = flash_data_cb};
static OD_extension_t OD_1F51_extension = {.object = NULL, .read = OD_readOriginal, .write = flash_cmd_cb};

void flasher_sdo_init(void)
{
	OD_extension_init(OD_ENTRY_H1F50, &OD_1F50_extension);
	OD_extension_init(OD_ENTRY_H1F51, &OD_1F51_extension);
}