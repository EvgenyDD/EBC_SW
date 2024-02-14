#include "co_sdo_flasher.h"
#include "CANopen.h"
#include "crc.h"
#include "platform.h"
#include "sdo.h"

extern CO_t *CO;
extern nd_fw_cb_t *g_nd_fw_cb;

#define BLOCK_SZ (534)

uint8_t g_sdo_flasher_id = 0;

enum
{
	SDO_FL_ERR_STS_FL = 10,
	SDO_FL_ERR_WR_CMD,
	SDO_FL_ERR_RD_IDENT,
	SDO_FL_ERR_RD_IDENT2,
	SDO_FL_ERR_ID_SAME_SECOND_TIME,
	SDO_FL_ERR_SIZE_BIGGER_THAN_BUFFER,
	SDO_FL_ERR_WR_PAYLOAD,
};

static int read_sts_flash(const char *op)
{
	uint32_t err_code, flash_sts = 0;
	size_t rd = sizeof(flash_sts);
	for(int i = 0; i < 20; i++)
	{
		if((err_code = read_SDO(CO->SDOclient, g_sdo_flasher_id, 0x1F57, 1, (uint8_t *)&flash_sts, sizeof(flash_sts), &rd, 100)) == 0)
		{
			if(flash_sts == CO_SDO_FLASHER_R_OK) return 0;
			if(flash_sts != CO_SDO_FLASHER_R_BUSY) break;
		}
		delay_ms(100);
	}
	// _PRINTF("%s failed, status: x%x | SDO: x%X", op, flash_sts, err_code);
	return SDO_FL_ERR_STS_FL;
}

int cb_nd_rd_preldr(uint32_t addr, uint8_t *data, uint32_t size_buffer, uint32_t *size_read) { return 1; }
int cb_nd_wr_preldr(uint32_t addr, const uint8_t *data, uint32_t size) { return 1; }
int cb_nd_rd_ldr(uint32_t addr, uint8_t *data, uint32_t size_buffer, uint32_t *size_read) { return 1; }
int cb_nd_rd_app(uint32_t addr, uint8_t *data, uint32_t size_buffer, uint32_t *size_read) { return 1; }

struct __attribute__((packed))
{
	uint32_t block_number;
	uint32_t address;
	uint32_t size;
	uint8_t data[BLOCK_SZ];
	uint8_t data_crc[4];
} block;

int cb_nd_wr(uint32_t addr, const uint8_t *data, uint32_t size)
{
	uint32_t cmd;
	CO_SDO_abortCode_t sts;

	if(addr != 0 && size == 0)
	{
		cmd = CO_SDO_FLASHER_CHECK_SIGNATURE;
		if((sts = write_SDO(CO->SDOclient, g_sdo_flasher_id, 0x1F51, 1, (uint8_t *)&cmd, sizeof(cmd), 3000)) != 0) return SDO_FL_ERR_WR_CMD;
		if((sts = read_sts_flash("CO_SDO_FLASHER_CHECK_SIGNATURE")) != 0) return sts;
		return 0;
	}

	uint8_t str[64] = {0};
	size_t rd = sizeof(str);
	if(addr == 0)
	{
		memset(str, 0, sizeof(str));
		if((sts = read_SDO(CO->SDOclient, g_sdo_flasher_id, 0x1008, 0, str, sizeof(str), &rd, 800)) != 0) return SDO_FL_ERR_RD_IDENT;
		const char *same_fw_str = g_nd_fw_cb->name;
		if(strcmp(str, same_fw_str) == 0) // need reset
		{
			cmd = CO_SDO_FLASHER_W_START_BOOTLOADER;
			write_SDO(CO->SDOclient, g_sdo_flasher_id, 0x1F51, 1, (uint8_t *)&cmd, sizeof(cmd), 100);

			delay_ms(150);

			rd = sizeof(str);
			memset(str, 0, sizeof(str));
			if((sts = read_SDO(CO->SDOclient, g_sdo_flasher_id, 0x1008, 0, str, sizeof(str), &rd, 800)) != 0) return SDO_FL_ERR_RD_IDENT2;
			if(strcmp(str, same_fw_str) == 0) return SDO_FL_ERR_ID_SAME_SECOND_TIME;
		}

		cmd = CO_SDO_FLASHER_W_STOP;
		if((sts = write_SDO(CO->SDOclient, g_sdo_flasher_id, 0x1F51, 1, (uint8_t *)&cmd, sizeof(cmd), 3000)) != 0) return SDO_FL_ERR_WR_CMD;
		if((sts = read_sts_flash("CO_SDO_FLASHER_W_STOP")) != 0) return sts;

		cmd = CO_SDO_FLASHER_W_CLEAR;
		if((sts = write_SDO(CO->SDOclient, g_sdo_flasher_id, 0x1F51, 1, (uint8_t *)&cmd, sizeof(cmd), 3000)) != 0) return SDO_FL_ERR_WR_CMD;
		if((sts = read_sts_flash("CO_SDO_FLASHER_W_CLEAR")) != 0) return sts;
	}

	if(size > BLOCK_SZ) return SDO_FL_ERR_SIZE_BIGGER_THAN_BUFFER;

	block.block_number = addr;
	block.address = addr;
	block.size = size;

	memcpy(block.data, data, block.size);
	uint32_t block_crc = crc32((uint8_t *)&block, 12U + block.size);
	memcpy(&block.data[block.size], (uint8_t *)&block_crc, sizeof(block_crc));

	if((sts = write_SDO(CO->SDOclient, g_sdo_flasher_id, 0x1F50, 1, (uint8_t *)&block, 16 + block.size, 3000)) != 0) return SDO_FL_ERR_WR_PAYLOAD;
	if((sts = read_sts_flash("WRITE BLOCK")) != 0) return sts;

	return 0;
}
