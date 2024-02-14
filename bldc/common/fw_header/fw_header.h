#ifndef FW_HEADER_H
#define FW_HEADER_H

#include <stdbool.h>
#include <stdint.h>

#define FW_PRELDR (0)
#define FW_LDR (1)
#define FW_APP (2)
#define FW_COUNT (3)

typedef enum
{
	LOCK_NONE = 0,
	LOCK_BY_CRC,
	LOCK_BY_ADDR,
	LOCK_BY_SIZE_SMALL,
	LOCK_BY_ZERO_FIELDS_COUNT,
	LOCK_NO_PROD_FIELD,
	LOCK_NO_PROD_NAME_FIELD,
	LOCK_PROD_MISMATCH,
	LOCK_PROD_NAME_FAULT,
} FW_HDR_LOCK_t;

typedef struct
{
	uint32_t fw_size;
	uint32_t fw_crc32; // with fields, without fw_header_v1_t
	uint32_t fields_addr_offset;
	uint32_t reserved2;
} fw_header_v1_t;

typedef struct
{
	int locked;							// locked (::FW_HDR_LOCK_t)
	uintptr_t addr;						// pointer to fw
	uint32_t size;						// firmware size
	int fields_count;					// count of fields
	const char *field_product_ptr;		// pointer to value of the "product" field
	int field_product_len;				// "product" field length
	const char *field_product_name_ptr; // pointer to value of the "product_name" field
	int field_product_name_len;			// "product_name" field length
	uint32_t ver_major;					// parsed major version
	uint32_t ver_minor;					// parsed minor version
	uint32_t ver_patch;					// parsed patch version
} fw_info_t;

int fw_fields_get_count(uint32_t addr_fw_start, uint32_t region_size);
const char *fw_fields_find_by_key(uint32_t addr_fw_start, const char *key, uint32_t region_size);
const char *fw_fields_find_by_key_helper(fw_info_t *fw, const char *key);
bool fw_fields_find_by_iterator(uint32_t addr_fw_start, unsigned int iterator, const char **p_key, const char **p_value, uint32_t region_size);
bool fw_fields_find_by_iterator_helper(fw_info_t *fw, unsigned int iterator, const char **p_key, const char **p_value);

int str_len_safe(const char *s);
bool str_compare_equal_safe_two_arg(const char *s1, const char *s2);
bool str_compare_equal_safe_first_arg(const char *s1, const char *s2); // hack
bool flash_check_range(uint32_t addr_start, uint32_t size);

bool fw_header_check_region(fw_info_t *fw, uint32_t header_offset, uint32_t max_size);
void fw_header_check_all(void);

extern fw_info_t g_fw_info[FW_COUNT];

#endif // FW_HEADER_H
