#include "error.h"

static uint8_t errors[ERROR_COUNT / 8 + ((ERROR_COUNT % 8) ? 1 : 0)] = {0};

void error_set(uint32_t error, bool value)
{
	if(value)
		errors[error / 8] |= (1 << (error % 8));
	else
		errors[error / 8] &= ~(1 << (error % 8));
}

bool error_get(uint32_t error)
{
	return errors[error / 8] & (1 << (error % 8));
}