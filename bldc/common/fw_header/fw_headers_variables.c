#include "fw_header.h"
#include "platform.h"

#define LDR_NAME "ebc6_ldr"
#define APP_NAME "ebc6_app"

const fw_header_v1_t g_fw_hdr __attribute__((section(".fw_header"))) = {0xAAAAAAAAU, 0xBBBBBBBBU, 0xCCCCCCCCU, 0xDDDDDDDDU}; // reserved space for fw header

fw_info_t g_fw_info[FW_COUNT] = {0};

/**
 * @brief Check range of flash region, is it valid
 *
 * @param addr_start
 * @param size
 * @return true not valid (overflow)
 * @return false valid
 */
bool flash_check_range(uint32_t addr_start, uint32_t size)
{
	if(size > FLASH_LEN) return true;

	uint32_t addr_end = addr_start + size;
	return addr_start < FLASH_START || addr_end > FLASH_FINISH;
}

void fw_header_check_all(void)
{
	// initialize fw headers addresses
	g_fw_info[FW_PRELDR].addr = (uint32_t)&__preldr_start;
	g_fw_info[FW_LDR].addr = (uint32_t)&__ldr_start;
	g_fw_info[FW_APP].addr = (uint32_t)&__app_start;

	// collect & check data from regions
	for(int i = 0; i < FW_COUNT; i++)
	{
		fw_header_check_region(&g_fw_info[i], (uint32_t)(&__header_offset), FLASH_FINISH - (uint32_t)(&__header_offset) - g_fw_info[i].addr - 1);
	}

	// logic - check "product" fields
	{
		if(g_fw_info[FW_PRELDR].locked == false && g_fw_info[FW_LDR].locked == false)
		{
			if(str_compare_equal_safe_two_arg(g_fw_info[FW_PRELDR].field_product_ptr,
											  g_fw_info[FW_LDR].field_product_ptr) == false)
			{
				g_fw_info[FW_LDR].locked = LOCK_PROD_MISMATCH;
			}
		}

		if(g_fw_info[FW_PRELDR].locked == false && g_fw_info[FW_APP].locked == false)
		{
			if(str_compare_equal_safe_two_arg(g_fw_info[FW_PRELDR].field_product_ptr,
											  g_fw_info[FW_APP].field_product_ptr) == false)
			{
				g_fw_info[FW_APP].locked = LOCK_PROD_MISMATCH;
			}
		}

		if(g_fw_info[FW_LDR].locked == false)
		{
			if(str_compare_equal_safe_two_arg(LDR_NAME, g_fw_info[FW_LDR].field_product_name_ptr) == false) g_fw_info[FW_LDR].locked = LOCK_PROD_NAME_FAULT;
		}

		if(g_fw_info[FW_APP].locked == false)
		{
			if(str_compare_equal_safe_two_arg(APP_NAME, g_fw_info[FW_APP].field_product_name_ptr) == false) g_fw_info[FW_APP].locked = LOCK_PROD_NAME_FAULT;
		}
	}
}

const char *fw_fields_find_by_key_helper(fw_info_t *fw, const char *key)
{
	if(fw->locked) return false;
	if(fw->fields_count == 0) return false;

	fw_header_v1_t *hdr = (fw_header_v1_t *)(fw->addr + (uint32_t)(&__header_offset));
	uint32_t addr_fw_start = fw->addr + hdr->fields_addr_offset;
	uint32_t max_size = FLASH_FINISH - (uint32_t)(&__header_offset) - fw->addr - 1;
	uint32_t region_size = max_size < hdr->fields_addr_offset ? 0 : max_size - hdr->fields_addr_offset;

	return fw_fields_find_by_key(addr_fw_start, key, region_size);
}

bool fw_fields_find_by_iterator_helper(fw_info_t *fw, unsigned int iterator, const char **p_key, const char **p_value)
{
	if(fw->locked) return false;
	if((int)iterator >= fw->fields_count) return false;

	fw_header_v1_t *hdr = (fw_header_v1_t *)(fw->addr + (uint32_t)(&__header_offset));
	uint32_t addr_fw_start = fw->addr + hdr->fields_addr_offset;
	uint32_t max_size = FLASH_FINISH - (uint32_t)(&__header_offset) - fw->addr - 1;
	uint32_t region_size = max_size < hdr->fields_addr_offset ? 0 : max_size - hdr->fields_addr_offset;

	return fw_fields_find_by_iterator(addr_fw_start, iterator, p_key, p_value, region_size);
}
