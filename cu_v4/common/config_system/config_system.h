#ifndef CONFIG_SYSTEM_H_
#define CONFIG_SYSTEM_H_

/**
 * Dynamic config processor
 *
 * Format (total size is multiple of 4):
 *  - u32 - size of config entries + padding
 *  - entry[] - config (array of config entries)
 *  - 0 to 3 zero bytes  - padding
 *  - u32 - crc32 of all (without this field)
 *
 * Entry format:
 *  - char[] + '\0' - key (entry name)
 *  - u16 - entry data size
 *  - ... - entry data
 */

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
	const char *key;
	uint16_t size;
	uint32_t data_abs_address;
	void *data;
} config_entry_t;

typedef enum
{
	CONFIG_STS_OK = 0,
	CONFIG_STS_STORAGE_READ_ERROR,
	CONFIG_STS_STORAGE_WRITE_ERROR,
	CONFIG_STS_STORAGE_WRONG_FORMAT,
	CONFIG_STS_STORAGE_OUT_OF_BOUNDS,
	CONFIG_STS_WRONG_SIZE_CONFIG,
	CONFIG_STS_CRC_INVALID,
	CONFIG_STS_KEY_LONG,
	CONFIG_STS_KEY_SHORT,
	CONFIG_STS_LENGTH_DATA_ZERO,
	CONFIG_STS_PARSER_NOT_FINISHED,
	CONFIG_STS_NO_DATA,
} config_sts_t;

#define CONFIG_MAX_KEY_SIZE 32

#define CFG_ORIGIN ((uint32_t) & __cfg_start)
#define CFG_END ((uint32_t) & __cfg_end)
#define CFG_SIZE (CFG_END - CFG_ORIGIN)

config_sts_t config_write_storage(void);
void config_read_storage(void);
config_sts_t config_validate(void);
uint32_t config_get_size(void);

__attribute__((weak)) void config_entry_not_found_callback(const uint8_t *key, uint32_t data_offset, uint16_t data_length);

extern config_entry_t g_device_config[];
extern const uint32_t g_device_config_count;

#endif // CONFIG_SYSTEM_H_