#ifndef ERROR_H__
#define ERROR_H__

#include <stdbool.h>
#include <stdint.h>

#define ERR_DESCR      \
	_F(CFG),           \
		_F(CFG_WR),    \
		_F(FRAM_SIGN), \
		_F(FRAM_WR),   \
		_F(COUNT)
enum
{
#define _F(x) ERROR_##x
	ERR_DESCR
#undef _F
};

// #define ERROR_COUNT 3

#endif // ERROR_H__