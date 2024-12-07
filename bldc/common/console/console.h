#ifndef CONSOLE_H__
#define CONSOLE_H__

#include <stdint.h>

#define DT_FMT_MS "%03d.%03d"
#define DT_DATA_MS(x) ((x) / 1000U), ((x) % 1000U)

void console_str(const char *str);
void _console_print(const char *fmt, ...);
void _console_print_prefix(void);

#define console_print(...)           \
	{                                \
		_console_print_prefix();     \
		_console_print(__VA_ARGS__); \
	}

void console_cb(char *data, uint32_t len);
void console_set_error_string(const char *str);

typedef enum
{
	CON_CB_OK = 0,
	CON_CB_SILENT,
	CON_CB_ERR_CUSTOM,
	CON_CB_ERR_ARGS,
	CON_CB_ERR_BAD_PARAM,
	CON_CB_ERR_UNSAFE,
	CON_CB_ERR_NO_SPACE,
} console_cmd_cb_res_t;

typedef int (*console_cmd_cb_t)(const char *, int);

typedef struct
{
	const char *name;
	console_cmd_cb_t cb;
} console_cmd_t;

extern const console_cmd_t console_cmd[];
extern const uint32_t console_cmd_sz;

#endif // CONSOLE_H__