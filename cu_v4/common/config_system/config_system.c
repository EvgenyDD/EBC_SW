#include "config_system.h"
#include "crc.h"
#include "platform.h"
#include <string.h>

__attribute__((weak)) void config_entry_not_found_callback(const uint8_t *key, uint32_t data_offset, uint16_t data_length) {}

#define DATA_OFFSET 4

static bool config_valid = false;

static uint8_t flush_buffer[32]; // must be multiple of 4 (cause of CRC)
static uint32_t flush_buffer_size = 0;

static uint32_t offset_write_data = 0;

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef enum
{
	PROCESS_FINISH = 0,
	PROCESS_KEY,
	PROCESS_LENGTH,
	PROCESS_VALUE,
} sts_t;

typedef struct
{
	uint8_t name_buffer[CONFIG_MAX_KEY_SIZE];
	uint16_t name_buffer_size;
	uint16_t length_data;
	uint16_t length_size;
	uint32_t offset_entry_data; // global offset from the flash start
	sts_t sts;
} parse_struct_t;

static parse_struct_t parser;

static config_sts_t flush_data_calc_crc(const uint8_t *data, uint32_t size, bool flush_all)
{
	if(offset_write_data + size > CFG_SIZE) return CONFIG_STS_STORAGE_OUT_OF_BOUNDS;
	for(uint32_t i = 0; i < size; i++)
	{
		flush_buffer[flush_buffer_size++] = data ? data[i] : 0;
		if(flush_buffer_size >= sizeof(flush_buffer) ||
		   (i == size - 1 && flush_all))
		{
			if(platform_flash_write(CFG_ORIGIN + offset_write_data, flush_buffer, flush_buffer_size)) return CONFIG_STS_STORAGE_WRITE_ERROR;
			offset_write_data += flush_buffer_size;
			crc32_end(flush_buffer, flush_buffer_size);
			flush_buffer_size = 0;
		}
	}
	return CONFIG_STS_OK;
}

/**
 * @brief Write configuration table to the Storage
 *
 * @return config_sts_t
 */
config_sts_t config_write_storage(void)
{
	if(g_device_config_count == 0) return CONFIG_STS_NO_DATA;

#if CFG_SYSTEM_SAVES_NON_NATIVE_DATA
	if(config_valid == false)
	{
#endif
		config_valid = false;

		// first validate entries
		uint32_t total_length = 4 + 4;
		for(uint32_t i = 0; i < g_device_config_count; i++)
		{
			int len_key = (int)strlen(g_device_config[i].key);
			if(len_key >= CONFIG_MAX_KEY_SIZE - 1 /* '\0' */) return CONFIG_STS_KEY_LONG;
			if(len_key == 0) return CONFIG_STS_KEY_SHORT;
			if(g_device_config[i].size == 0) return CONFIG_STS_LENGTH_DATA_ZERO;
			total_length += (uint32_t)((len_key + 1) + 2 + g_device_config[i].size);
		}
		if(total_length > CFG_SIZE) return CONFIG_STS_STORAGE_OUT_OF_BOUNDS;

		// next write entries
		uint32_t size_config_data = 0;

		for(uint32_t i = 0; i < g_device_config_count; i++)
		{
			size_config_data += strlen(g_device_config[i].key) + 1 /* '\0' */ + 2U /* size field - uint16 */ + g_device_config[i].size;
		}

		uint32_t count_zeros = 0;
		if(size_config_data & 0x03)
		{
			count_zeros = 4 - (size_config_data % 4);
			size_config_data += count_zeros;
		}

		// Now write data
		platform_flash_erase_flag_reset_sect_cfg();
		if(platform_flash_write(CFG_ORIGIN, (uint8_t *)&size_config_data, sizeof(uint32_t))) return CONFIG_STS_STORAGE_WRITE_ERROR;
		offset_write_data = sizeof(uint32_t);
		crc32_start((uint8_t *)&size_config_data, sizeof(uint32_t));

		flush_buffer_size = 0;
		config_sts_t sts;
		for(uint32_t i = 0; i < g_device_config_count; i++)
		{
			sts = flush_data_calc_crc((const uint8_t *)g_device_config[i].key, strlen(g_device_config[i].key) + 1, false);
			if(sts) return sts;
			sts = flush_data_calc_crc((const uint8_t *)&g_device_config[i].size, 2, false);
			if(sts) return sts;
			sts = flush_data_calc_crc((const uint8_t *)g_device_config[i].data, g_device_config[i].size, false);
			if(sts) return sts;
		}

		sts = flush_data_calc_crc(0, count_zeros, false);
		if(sts) return sts;

		uint32_t crc32_val = crc32_end(flush_buffer, flush_buffer_size);
		sts = flush_data_calc_crc((const uint8_t *)&crc32_val, sizeof(uint32_t), true);
		if(sts) return sts;

		config_valid = true;
		return CONFIG_STS_OK;
#if CFG_SYSTEM_SAVES_NON_NATIVE_DATA
	}
	else
	{
		// config is valid
		config_valid = false;
		uint32_t size_config_data = 0;
		for(uint32_t i = 0; i < g_device_config_count; i++)
		{
			int len_key = (int)strlen(g_device_config[i].key);
			if(len_key >= CONFIG_MAX_KEY_SIZE - 1 /* '\0' */) return CONFIG_STS_KEY_LONG;
			if(len_key == 0) return CONFIG_STS_KEY_SHORT;
			if(g_device_config[i].size == 0) return CONFIG_STS_LENGTH_DATA_ZERO;
			size_config_data += (uint32_t)((len_key + 1) + 2 + g_device_config[i].size);
		}

		uint32_t old_data_size;
		platform_flash_read(CFG_ORIGIN, (uint8_t *)&old_data_size, sizeof(old_data_size));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
		uint8_t buf_old_data[4 + old_data_size];
#pragma GCC diagnostic pop
		memset(buf_old_data, 0, sizeof(buf_old_data)); // erase buffer
		if(4 + old_data_size <= sizeof(buf_old_data))
		{
			platform_flash_read(CFG_ORIGIN, buf_old_data, 4 + old_data_size); // copy old config to buffer
		}

		// first calc (new) total size & check it will fit the FLASH
		for(uint32_t offset_buf = 4; offset_buf < old_data_size + 4;)
		{
			const char *entry_name = (const char *)&buf_old_data[offset_buf];
			if(*entry_name == 0)
			{
				offset_buf++;
				continue;
			}

			const uint32_t entry_name_sz = strlen(entry_name) + 1 /* '\0' */;
			uint16_t data_sz;
			memcpy(&data_sz, &buf_old_data[offset_buf + entry_name_sz], sizeof(data_sz));

			bool found = false;
			for(uint32_t i = 0; i < g_device_config_count; i++)
			{
				if(strcmp(entry_name, g_device_config[i].key) == 0)
				{
					found = true;
					break;
				}
			}
			if(found == false) size_config_data += entry_name_sz + 2 /* size field - uint16 */ + data_sz; // must be copied later
			offset_buf += entry_name_sz + 2 /* size field - uint16 */ + data_sz;
		}

		uint32_t count_zeros = 0;
		if(size_config_data & 0x03)
		{
			count_zeros = 4 - (size_config_data % 4);
			size_config_data += count_zeros;
		}
		if(4 /* size field */ + size_config_data + 4 /* CRC */ > CFG_SIZE) return CONFIG_STS_STORAGE_OUT_OF_BOUNDS;

		// Now write data
		platform_flash_erase_flag_reset_sect_cfg();
		if(platform_flash_write(CFG_ORIGIN, (uint8_t *)&size_config_data, sizeof(uint32_t))) return CONFIG_STS_STORAGE_WRITE_ERROR;
		offset_write_data = sizeof(uint32_t);
		crc32_start((uint8_t *)&size_config_data, sizeof(uint32_t));

		flush_buffer_size = 0;
		config_sts_t sts;

		// write non-native entries
		for(uint32_t offset_buf = 4; offset_buf < old_data_size + 4;)
		{
			const char *entry_name = (const char *)&buf_old_data[offset_buf];
			if(*entry_name == 0)
			{
				offset_buf++;
				continue;
			}

			const uint32_t entry_name_sz = strlen(entry_name) + 1 /* '\0' */;
			uint16_t data_sz;
			memcpy(&data_sz, &buf_old_data[offset_buf + entry_name_sz], sizeof(data_sz));

			bool found = false;
			for(uint32_t i = 0; i < g_device_config_count; i++)
			{
				if(strcmp(entry_name, g_device_config[i].key) == 0)
				{
					found = true;
					break;
				}
			}
			if(found == false)
			{
				// write entry from old config to the new config (FLASH)
				sts = flush_data_calc_crc((const uint8_t *)entry_name, entry_name_sz, false);
				if(sts) return sts;
				sts = flush_data_calc_crc((const uint8_t *)&data_sz, 2, false);
				if(sts) return sts;
				sts = flush_data_calc_crc((const uint8_t *)&buf_old_data[offset_buf + entry_name_sz + 2], data_sz, false);
				if(sts) return sts;
			}
			offset_buf += entry_name_sz + 2 + data_sz;
		}

		// write native entries
		for(uint32_t i = 0; i < g_device_config_count; i++)
		{
			sts = flush_data_calc_crc((const uint8_t *)g_device_config[i].key, strlen(g_device_config[i].key) + 1, false);
			if(sts) return sts;
			sts = flush_data_calc_crc((const uint8_t *)&g_device_config[i].size, 2, false);
			if(sts) return sts;
			sts = flush_data_calc_crc((const uint8_t *)g_device_config[i].data, g_device_config[i].size, false);
			if(sts) return sts;
		}

		sts = flush_data_calc_crc(0, count_zeros, false);
		if(sts) return sts;

		uint32_t crc32_val = crc32_end(flush_buffer, flush_buffer_size);
		sts = flush_data_calc_crc((const uint8_t *)&crc32_val, sizeof(uint32_t), true);
		if(sts) return sts;

		config_valid = true;
		return CONFIG_STS_OK;
	}
#endif
}

static bool find_entry(parse_struct_t *prs, uint32_t data_offset)
{
	for(uint32_t i = 0; i < g_device_config_count; i++)
	{
		if(strcmp((char *)prs->name_buffer, g_device_config[i].key) == 0 &&
		   prs->length_data == g_device_config[i].size)
		{
			g_device_config[i].data_abs_address = data_offset;
			return true;
		}
	}
	return false;
}

static config_sts_t parse_data(const uint32_t offset_data, const uint8_t *data, uint16_t length)
{
	for(uint16_t i = 0; i < length; i++)
	{
		switch(parser.sts)
		{
		case PROCESS_FINISH:
			if(data[i] == '\0') continue; // padding is made of '\0'
										  // fall through
		case PROCESS_KEY:
			parser.sts = PROCESS_KEY;
			parser.name_buffer[parser.name_buffer_size++] = data[i];
			if(parser.name_buffer_size >= CONFIG_MAX_KEY_SIZE && data[i] != '\0') return CONFIG_STS_KEY_LONG; // no end zero
			if(data[i] == '\0' && parser.name_buffer_size <= 1) return CONFIG_STS_KEY_SHORT;
			if(data[i] == '\0') parser.sts = PROCESS_LENGTH;
			break;

		case PROCESS_LENGTH:
			parser.length_data |= data[i] << (8 * parser.length_size);
			if(++parser.length_size >= 2)
			{
				if(parser.length_data == 0) return CONFIG_STS_LENGTH_DATA_ZERO;
				parser.sts = PROCESS_VALUE;
				parser.offset_entry_data = offset_data + (i + 1U) /* next byte*/;
			}
			break;

		case PROCESS_VALUE:
			if(parser.offset_entry_data + parser.length_data == offset_data + i + 1) // entry is ready!
			{
				parser.sts = PROCESS_FINISH;
				if(find_entry(&parser, parser.offset_entry_data) == false) config_entry_not_found_callback(parser.name_buffer, parser.offset_entry_data, parser.length_size);
				memset(&parser, 0, sizeof(parser));
			}
			break;

		default: break;
		}
	}
	return CONFIG_STS_OK;
}

/**
 * @brief Checks the config in Storage & fills data addresses (::data_abs_address) to configuration table
 *
 * @return config_sts_t
 */
config_sts_t config_validate(void)
{
	config_valid = false;

#define BUFFER_SIZE 32
	uint8_t buffer_array[BUFFER_SIZE];

	uint32_t size_config;
	platform_flash_read(CFG_ORIGIN, (uint8_t *)&size_config, sizeof(size_config));
	crc32_start((uint8_t *)&size_config, sizeof(uint32_t));

	const uint32_t end_data = DATA_OFFSET + size_config;

	if(size_config < (8) /* minimal data */ ||
	   size_config > DATA_OFFSET + CFG_SIZE + 4 ||
	   (size_config & 0x03U) /* not the multiple of 4 */) return CONFIG_STS_WRONG_SIZE_CONFIG;

	uint32_t crc_calc = 0;
	for(uint32_t i = 0, word; i < size_config; i += 4)
	{
		platform_flash_read(CFG_ORIGIN + DATA_OFFSET + i, (uint8_t *)&word, sizeof(uint32_t));
		crc_calc = crc32_end((uint8_t *)&word, sizeof(uint32_t));
	}

	uint32_t crc_end;
	platform_flash_read(CFG_ORIGIN + end_data, (uint8_t *)&crc_end, sizeof(uint32_t));
	if(crc_end != crc_calc) return CONFIG_STS_CRC_INVALID;

	memset(&parser, 0, sizeof(parser));

	uint32_t offset_read = DATA_OFFSET;
	for(;;)
	{
		uint32_t size = BUFFER_SIZE;
		if(size > end_data - offset_read) size = end_data - offset_read;

		platform_flash_read(CFG_ORIGIN + offset_read, buffer_array, size);

		config_sts_t sts = parse_data(CFG_ORIGIN + offset_read, buffer_array, size);
		if(sts) return sts;

		offset_read += size;

		// last
		if(offset_read == end_data)
		{
			if(parser.sts != PROCESS_FINISH) return CONFIG_STS_PARSER_NOT_FINISHED;
			config_valid = true;
			return CONFIG_STS_OK;
		}
	}
}

/**
 * @brief Copy Storage config fields to configuration table variable references
 *
 */
void config_read_storage(void)
{
	for(uint32_t i = 0; i < g_device_config_count; i++)
	{
		if(g_device_config[i].data_abs_address &&
		   g_device_config[i].data &&
		   g_device_config[i].size)
		{
			platform_flash_read(g_device_config[i].data_abs_address, g_device_config[i].data, g_device_config[i].size);
		}
	}
}

/**
 * @brief Get config section size
 *
 */
uint32_t config_get_size(void)
{
	uint32_t size_config;
	platform_flash_read(CFG_ORIGIN, (uint8_t *)&size_config, sizeof(size_config));
	return size_config + 8;
}