#include "fw_header.h"
#include <stddef.h>
#include <string.h>

int str_len_safe(const char *s)
{
	if(flash_check_range((uint32_t)s, 1))
	{
		return -1;
	}

	for(uint32_t i = 0;; i++)
	{
		if(s[i] == '\0') return (int)i;
		if(flash_check_range((uint32_t)(s + i), 0)) return -2;
	}
}

/** 
 * @brief Compare 2 strings (safe) - with absolute match
 * And checks for string ranges (first & second)
 * 
 * @param s1 
 * @param s2 
 * @return true strings are equal
 * @return false strings are not equal
 */
bool str_compare_equal_safe_two_arg(const char *s1, const char *s2)
{
	int len_s1 = str_len_safe(s1);
	int len_s2 = str_len_safe(s2);

	if(len_s1 <= 0 || len_s2 <= 0 || len_s1 != len_s2) return false;
	for(int i = 0; i < len_s1; i++)
	{
		if(s1[i] != s2[i]) return false;
	}
	return true;
}

/** 
 * @brief Compare 2 strings (safe) - with absolute match
 * And checks for string ranges (first string argument)
 * 
 * @param s1 
 * @param s2 
 * @return true strings are equal
 * @return false strings are not equal
 */
bool str_compare_equal_safe_first_arg(const char *s1, const char *s2)
{
	int len_s1 = str_len_safe(s1);
	int len_s2 = (int)strlen(s2);

	if(len_s1 <= 0 || len_s2 <= 0 || len_s1 != len_s2) return false;
	for(int i = 0; i < len_s1; i++)
	{
		if(s1[i] != s2[i]) return false;
	}
	return true;
}

int fw_fields_get_count(uint32_t addr_fields_start, uint32_t region_size)
{
	uint32_t addr_fields_end = addr_fields_start + region_size;

	// check bounds
	if(flash_check_range(addr_fields_start, region_size))
	{
		return -1;
	}

	unsigned int null_term_count = 0, element_count = 0 /* field keys or values */;

	for(uint32_t i = 0; i < addr_fields_end - addr_fields_start; i++)
	{
		if(*((uint8_t *)addr_fields_start + i) == '\0')
		{
			if(i == 0) return -2; // first element can't be zero

			null_term_count++;
			if(null_term_count == 1)
			{
				element_count++;
			}
		}
		else
		{
			null_term_count = 0;
		}

		if(null_term_count >= 2) return (element_count & 1U)
											? -3 /* not even count of field keys/values */
											: (int)(element_count >> 1U) /* key + value = 2 */;
	}

	return -4; // max_length was reached
}

/**
 * @brief Find header value by key
 * Note: not safe to use with fw_fields_get_count() <= 0 !!!
 * 
 * @param addr_fields_start 
 * @param key 
 * @param region_size 
  * @return const char* NULL if failed to find
 * @return const char* !NULL if found
 */
const char *fw_fields_find_by_key(uint32_t addr_fields_start, const char *key, uint32_t region_size)
{
	uint32_t addr_fields_end = addr_fields_start + region_size;

	// check bounds
	if(flash_check_range(addr_fields_start, region_size))
	{
		return NULL;
	}

	unsigned int null_term_count = 0, element_count = 0 /* field keys or values */;
	bool null_captured = true; /* assuming that -1 element is '\0' */

	for(uint32_t i = 0; i < addr_fields_end - addr_fields_start; i++)
	{
		if(*((uint8_t *)addr_fields_start + i) == '\0')
		{
			if(i == 0) return NULL; // first element can't be zero

			null_term_count++;
			if(null_term_count == 1)
			{
				element_count++;
			}
			null_captured = true;
		}
		else
		{
			null_term_count = 0;

			if(null_captured) // start condition of the new string
			{
				null_captured = false;
				if((element_count & 1U) == 0) // check only keys, not values
				{
					if(str_compare_equal_safe_first_arg((const char *)((uint8_t *)addr_fields_start + i), key) == true)
					{
						return (const char *)(addr_fields_start + i) /* key base */ +
							   str_len_safe((const char *)(addr_fields_start + i)) + /* key offset */
							   1 /* null separator */;
					}
				}
			}
		}

		if(null_term_count >= 2) return NULL; // end of search
	}

	return NULL;
}

/**
 * @brief Find header value by iterator
 * Note: not safe to use with fw_fields_get_count() <= 0 !!!
 * 
 * @param addr_fields_start 
 * @param max_length 
 * @param iterator 
 * @param p_key [input] const pointer to key string
 * @param p_value [input] const pointer to value string
 * @return true if found
 * @return false if failed to find
 */
bool fw_fields_find_by_iterator(uint32_t addr_fields_start, unsigned int iterator, const char **p_key, const char **p_value, uint32_t region_size)
{
	uint32_t addr_fields_end = addr_fields_start + region_size;

	// check bounds
	if(flash_check_range(addr_fields_start, region_size))
	{
		return -1;
	}

	// initialize output variables
	*p_key = NULL;
	*p_value = NULL;

	unsigned int null_term_count = 0, element_count = 0 /* field keys or values */;
	bool null_captured = true; /* assuming that -1 element is '\0' */

	for(uint32_t i = 0; i < addr_fields_end - addr_fields_start; i++)
	{
		if(*((uint8_t *)addr_fields_start + i) == '\0')
		{
			if(i == 0) return false; // first element can't be zero

			null_term_count++;
			if(null_term_count == 1)
			{
				element_count++;
			}
			null_captured = true;
		}
		else
		{
			null_term_count = 0;

			if(null_captured) // start condition of the new string
			{
				null_captured = false;
				if((element_count & 1U) == 0 &&
				   (element_count >> 1U) == iterator) // check only keys, not values
				{
					*p_key = ((const char *)addr_fields_start + i);
					*p_value = (const char *)(addr_fields_start + i) /* key base */ +
							   str_len_safe((const char *)(addr_fields_start + i)) + /* key offset */
							   1 /* null terminator */;
					return true;
				}
			}
		}

		if(null_term_count >= 2) return false; // end of search
	}

	return false;
}
