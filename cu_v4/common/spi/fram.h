#ifndef FRAM_H__
#define FRAM_H__

#include <stdint.h>

#define FRAM_SIZE 131072

enum
{
	FRAM_ERR_ADDR_OVF = -100,
};

#define FRAM_LOG_OFFSET 1024

typedef struct
{
	uint32_t power_on_cnt;
	uint32_t logger_start;
	uint32_t logger_end;
} fram_entries_t;

int fram_init(void);

void fram_data_read(void);
void fram_data_write(void);

int fram_read(uint32_t addr, uint8_t *data, uint32_t size);
int fram_write(uint32_t addr, const uint8_t *data, uint32_t size);

uint8_t fram_read_sts(void);
int fram_check_sn(void);

extern fram_entries_t fram_data;

#endif // FRAM_H__