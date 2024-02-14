#include "fw_header.h"
#include <ctype.h>
#include <stddef.h>

extern uint32_t crc32_start(const uint8_t *data, uint32_t size_bytes);
extern uint32_t crc32_end(const uint8_t *data, uint32_t size_bytes);

static void get_uint32_number_from_str(const char *s, uint32_t *number)
{
	*number = 0;
	uint32_t count_digit = 0;
	for(uint32_t i = 0; i < 10; i++)
	{
		if(flash_check_range((uint32_t)(s + i), 2)) return;
		if(isdigit((int)s[i]) == false && s[i] != '\0') return; // not digit nor null-term
		if(s[i] == '\0') break;
		if(i == 10 - 1 &&
		   s[i + 1] != '\0') return;
		count_digit++;
	}
	uint32_t rank = 1;
	for(uint32_t i = count_digit; i > 0; i--)
	{
		uint32_t num = (uint32_t)(s[i - 1] - '0') * rank;
		rank *= 10;
		if(UINT32_MAX - *number < num)
		{
			*number = 0;
			return;
		}
		*number += num;
	}
}

/**
 * @brief Check Flash Region CRC, length & "product" field
 *
 * @return true Failed
 * @return false OK
 */
bool fw_header_check_region(fw_info_t *fw, uint32_t header_offset, uint32_t max_size)
{
	fw->locked = LOCK_NONE; // init

	fw_header_v1_t *hdr = (fw_header_v1_t *)(fw->addr + header_offset);

	fw->size = hdr->fw_size;

	if(flash_check_range(fw->addr, hdr->fw_size)) fw->locked = LOCK_BY_ADDR; // check flash range
	if(hdr->fw_size <= (header_offset + sizeof(fw_header_v1_t))) fw->locked = LOCK_BY_SIZE_SMALL;

	if(fw->locked) return true;

	crc32_start((uint8_t *)(fw->addr & 0xFFFFFFFCU), header_offset);
	const uint8_t *p_continue = (uint8_t *)((fw->addr + header_offset + sizeof(fw_header_v1_t)) & 0xFFFFFFFCU);
	if(crc32_end(p_continue, hdr->fw_size - (header_offset + sizeof(fw_header_v1_t))) != hdr->fw_crc32) fw->locked = LOCK_BY_CRC; // check FW CRC
	if(fw->locked) return true;

	fw->fields_count = fw_fields_get_count(fw->addr + hdr->fields_addr_offset,
										   max_size < hdr->fields_addr_offset ? 0 : max_size - hdr->fields_addr_offset);
	if(fw->fields_count <= 0) fw->locked = LOCK_BY_ZERO_FIELDS_COUNT;
	if(fw->locked) return true;

	fw->field_product_ptr = fw_fields_find_by_key(fw->addr + hdr->fields_addr_offset, "prod",
												  max_size < hdr->fields_addr_offset ? 0 : max_size - hdr->fields_addr_offset);
	if(fw->field_product_ptr == NULL) fw->locked = LOCK_NO_PROD_FIELD;
	fw->field_product_len = str_len_safe(fw->field_product_ptr);

	fw->field_product_name_ptr = fw_fields_find_by_key(fw->addr + hdr->fields_addr_offset, "prod_name",
													   max_size < hdr->fields_addr_offset ? 0 : max_size - hdr->fields_addr_offset);
	if(fw->field_product_name_ptr == NULL) fw->locked = LOCK_NO_PROD_NAME_FIELD;
	fw->field_product_name_len = str_len_safe(fw->field_product_name_ptr);

	const char *p_filed_ver_major = fw_fields_find_by_key(fw->addr + hdr->fields_addr_offset, "ver_maj",
														  max_size < hdr->fields_addr_offset ? 0 : max_size - hdr->fields_addr_offset);
	const char *p_filed_ver_minor = fw_fields_find_by_key(fw->addr + hdr->fields_addr_offset, "ver_min",
														  max_size < hdr->fields_addr_offset ? 0 : max_size - hdr->fields_addr_offset);
	const char *p_filed_ver_patch = fw_fields_find_by_key(fw->addr + hdr->fields_addr_offset, "ver_pat",
														  max_size < hdr->fields_addr_offset ? 0 : max_size - hdr->fields_addr_offset);

	get_uint32_number_from_str(p_filed_ver_major, &fw->ver_major);
	get_uint32_number_from_str(p_filed_ver_minor, &fw->ver_minor);
	get_uint32_number_from_str(p_filed_ver_patch, &fw->ver_patch);

	return fw->locked;
}
