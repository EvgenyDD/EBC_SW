#ifndef MD5_H__
#define MD5_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	uint64_t size;		// Size of input in bytes
	uint32_t buffer[4]; // Current accumulation of hash
	uint8_t input[64];	// Input to be used in the next step
	uint8_t digest[16]; // Result of algorithm
} md5_ctx;

void md5_init(md5_ctx *ctx);
void md5_update(md5_ctx *ctx, const uint8_t *input, size_t input_len);
void md5_finalize(md5_ctx *ctx);
void md5_step(uint32_t *buffer, uint32_t *input);

void md5_string(const uint8_t *input, uint8_t *result);
void md5_data(const uint8_t *input, uint32_t size, uint8_t *result);

#endif // MD5_H__