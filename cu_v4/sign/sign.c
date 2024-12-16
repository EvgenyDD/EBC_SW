#include "../common/fw_header/fw_header.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FIELDS_OFFSET 0x190U
const uint32_t pattern[4] = {0xAAAAAAAAU, 0xBBBBBBBBU, 0xCCCCCCCCU, 0xDDDDDDDDU};

static uint32_t sw_crc32_table[256] = {
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, // 0..3
	0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005, // 4..7
	0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61, // 8..B
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, // C..F

	0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
	0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
	0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
	0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
	0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
	0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
	0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
	0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
	0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
	0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
	0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
	0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
	0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
	0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
	0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
	0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
	0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
	0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
	0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
	0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
	0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
	0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
	0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
	0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
	0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
	0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
	0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
	0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
	0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
	0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
	0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
	0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
	0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
	0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
	0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
	0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
	0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
	0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
	0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
	0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};

/*!
 * \brief Calculate STM32 compatible CRC
 * \param pBuffer Pointer to a buffer
 * \param NumOfByte Buffer size
 * \return CRC Value
 */
static uint32_t crc32_eth(const uint8_t *pBuffer, uint32_t NumOfByte)
{
	uint32_t crc32 = 0xFFFFFFFF;
	uint32_t NumOfDWord = NumOfByte >> 2;
	uint32_t i = 0;

	while(NumOfDWord--)
	{
		uint32_t data = ((uint32_t)pBuffer[i + 3] << 24U) |
						((uint32_t)pBuffer[i + 2] << 16U) |
						((uint32_t)pBuffer[i + 1] << 8U) |
						((uint32_t)pBuffer[i + 0]);
		i += 4;

		crc32 = crc32 ^ data;

		for(int k = 0; k < 4; k++)
			crc32 = (crc32 << 8) ^ sw_crc32_table[crc32 >> 24];
	}

	return crc32;
}

static void crc32_eth_start(const uint8_t *pBuffer, uint32_t NumOfByte, uint32_t *temp)
{
	*temp = 0xFFFFFFFF;
	uint32_t NumOfDWord = NumOfByte >> 2;
	uint32_t i = 0;

	while(NumOfDWord--)
	{
		uint32_t data = ((uint32_t)pBuffer[i + 3] << 24U) |
						((uint32_t)pBuffer[i + 2] << 16U) |
						((uint32_t)pBuffer[i + 1] << 8U) |
						((uint32_t)pBuffer[i + 0]);
		i += 4;

		*temp = *temp ^ data;

		for(int k = 0; k < 4; k++)
			*temp = (*temp << 8) ^ sw_crc32_table[*temp >> 24];
	}
}

static uint32_t crc32_eth_end(const uint8_t *pBuffer, uint32_t NumOfByte, uint32_t *temp)
{
	uint32_t NumOfDWord = NumOfByte >> 2;
	uint32_t i = 0;

	while(NumOfDWord--)
	{
		uint32_t data = ((uint32_t)pBuffer[i + 3] << 24U) |
						((uint32_t)pBuffer[i + 2] << 16U) |
						((uint32_t)pBuffer[i + 1] << 8U) |
						((uint32_t)pBuffer[i + 0]);
		i += 4;

		*temp = *temp ^ data;

		for(int k = 0; k < 4; k++)
			*temp = (*temp << 8) ^ sw_crc32_table[*temp >> 24];
	}

	return *temp;
}

static size_t file_len(FILE *f)
{
	fseek(f, 0, SEEK_END);
	size_t s = (size_t)ftell(f);
	rewind(f);
	return s;
}

int main(int argc, char **argv)
{
	if(argc < 4)
	{
		fprintf(stderr, "Usage %s [fw_id] [.bin input firmware file] [.bin output firmware file] [size limit]\n", argv[0]);
		return 1;
	}

	FILE *fw_in_file = fopen(argv[1], "rb");
	if(!fw_in_file)
	{
		perror("[ERROR]\topen input file");
		return 2;
	}

	FILE *fw_out_file = fopen(argv[2], "wb");
	if(!fw_out_file)
	{
		perror("[ERROR]\topen output file");
		return 2;
	}

	int length_limit = atoi(argv[3]);

	size_t content_length = file_len(fw_in_file);
	uint8_t *content = malloc(content_length);

	int read = (int)fread(content, 1, content_length, fw_in_file);
	if(read != (int)content_length)
	{
		perror("[ERROR]\tread input file");
		return 2;
	}

	// 4-byte align
	for(uint32_t i = 0; i < 1 || (content_length % 4); i++)
	{
		content = realloc(content, content_length + 1);
		if(content == NULL)
		{
			perror("[ERROR]\trealloc failed");
			return 6;
		}
		content[content_length] = '\0'; // fields END
		content_length += 1;
	}

	if(content_length < FIELDS_OFFSET + 4)
	{
		perror("[ERROR]\ttoo small input file");
		return 7;
	}

	uint32_t fields_addr_offset = content_length;
	for(int i = 4; i < argc; i++)
	{
		printf("\tField [%d]: %s\n", i - 4, argv[i]);

		char *p_delim = strchr(argv[i], '=');
		size_t field_len_total = strlen(argv[i]);

		if(p_delim == NULL ||
		   (int)(p_delim - argv[i]) <= 0 ||
		   (int)field_len_total <= (int)(p_delim - argv[i]) + 1)
		{
			printf("\n[ERROR]\tInvalid entry [%s] format\n", argv[i]);
			return 3;
		}

		int size = (int)strlen(argv[i]) + 1;
		if(size <= 0)
		{
			perror("[ERROR]\tstrlen failed");
			return 4;
		}

		content = realloc(content, content_length + (size_t)size);
		if(content == NULL)
		{
			perror("[ERROR]\trealloc failed");
			return 5;
		}

		memcpy(content + content_length, argv[i], (size_t)size);
		content[content_length + (size_t)(p_delim - argv[i])] = '\0'; // field name & data delimeter
		content_length += (size_t)size;
	}

	// null - terminator + 4-byte align
	for(uint32_t i = 0; i < 1 || (content_length % 4); i++)
	{
		content = realloc(content, content_length + 1);
		if(content == NULL)
		{
			perror("[ERROR]\trealloc failed");
			return 6;
		}
		content[content_length] = '\0'; // fields END
		content_length += 1;
	}

	fw_header_v1_t *const p_hdr = (fw_header_v1_t *)(content + FIELDS_OFFSET);
	if(memcmp(p_hdr, pattern, sizeof(pattern)) != 0)
	{
		printf("[Error]\tPattern mismatch!\n\tPattern: 0x");
		for(uint8_t i = 0; i < sizeof(pattern); i++)
		{
			printf("%02X", ((const uint8_t *)pattern)[i]);
		}
		printf("\n\tActual:  0x");
		for(uint8_t i = 0; i < sizeof(pattern); i++)
		{
			printf("%02X", ((const uint8_t *)p_hdr)[i]);
		}
		printf("\nPut the pattern at FIELDS OFFSET(0x%04X), for example:\n\t\""
			   "const volatile fw_header_v1_t g_fw_hdr __attribute__((section(\".fw_header\"))) = "
			   "{0xAAAAAAAAU, 0xBBBBBBBBU, 0xCCCCCCCCU, 0xDDDDDDDDU};\"\n",
			   FIELDS_OFFSET);
		return 7;
	}

	p_hdr->fw_size = content_length;
	p_hdr->fields_addr_offset = fields_addr_offset;
	uint32_t crc_temp;
	crc32_eth_start(content,
					FIELDS_OFFSET,
					&crc_temp);
	p_hdr->fw_crc32 = crc32_eth_end(content + FIELDS_OFFSET + sizeof(fw_header_v1_t),
									content_length - (FIELDS_OFFSET + sizeof(fw_header_v1_t)),
									&crc_temp);

	printf("FW length: %d | CRC: 0x%08X | Offset: %d | Space: %.3f %%\n", (int)p_hdr->fw_size, p_hdr->fw_crc32, p_hdr->fields_addr_offset, 100.0f * (float)p_hdr->fw_size / (float)length_limit);

	if(length_limit < (int)p_hdr->fw_size)
	{
		printf("[Error]\tLength limit! Target: %d, actual: %d\n", length_limit, p_hdr->fw_size);
		return 8;
	}

	// write 'signed' firmware
	if(fwrite(content, 1, content_length, fw_out_file) != content_length)
	{
		perror("[Error]\tWrite output file failed!");
		return 9;
	}

	free(content);

	fclose(fw_in_file);
	fclose(fw_out_file);

	return 0;
}
