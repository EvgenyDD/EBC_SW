#include "console.h"
#include "_printf.h"
#include "platform.h"
#include "usbd_core_cdc.h"
#include <stdarg.h>
#include <string.h>

#define REQ_SIZE 256

static char print_buf[512];

void _console_print(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	int act_len = vsnprintf(print_buf, sizeof(print_buf) - 1, fmt, ap);
	va_end(ap);

	usbd_cdc_push_data((uint8_t *)print_buf, act_len);
}

void _console_print_prefix(void) { _console_print("[" DT_FMT_MS "]:", DT_DATA_MS(system_time_ms)); }

void console_str(const char *str) { usbd_cdc_push_data((const uint8_t *)str, strlen(str)); }

/////////////////////////////////////////////////////

static const char *error_str = "";

static uint8_t prev_request[256] = {0};
static int prev_request_len = 0;

void console_set_error_string(const char *str) { error_str = str; }

void console_cb(const char *_data, uint32_t len)
{
	if(len > REQ_SIZE) return;
	uint8_t data_array[REQ_SIZE + 1];
	uint8_t *data = data_array;
	memcpy(data_array, _data, len);
	data_array[len] = 0;
	if(len >= sizeof(prev_request)) return;

	uint16_t len_req = len;

	if(len_req == 1 && data[0] == '\n')
	{
		data = prev_request;
		len_req = prev_request_len;
		if(prev_request[0] != '\n' && prev_request[0] != '\0') _console_print(">");
	}
	else
	{
		memcpy(prev_request, data, len_req);
		prev_request[len_req] = 0;
		prev_request_len = len_req;
	}

	if(strncmp((char *)data, "help", 4) == 0)
	{
		_console_print("Available commands:\n");
		for(uint32_t i = 0; i < console_cmd_sz; i++)
		{
			_console_print("\t%s\n", console_cmd[i].name);
		}
	}
	else if((strcmp((char *)data, "\n") != 0) && (data[0] != '#') && data[0])
	{
		for(uint32_t i = 0; i < console_cmd_sz; i++)
		{
			if(console_cmd[i].cb)
			{
				uint16_t l = strlen(console_cmd[i].name);
				if(strncmp((char *)data, (const char *)console_cmd[i].name, l) == 0)
				{
					const char *param = (len_req - l) > 0 ? (const char *)(data + l) : 0;
					int error_code = CON_CB_SILENT;
					console_cmd[i].cb(param, len_req - l, &error_code);
					if(error_code > CON_CB_SILENT)
					{
						_console_print("Error: ");
						switch(error_code)
						{
						case CON_CB_ERR_CUSTOM: _console_print("%s\n", error_str); break;
						case CON_CB_ERR_UNSAFE: _console_print("Unsafe operation\n"); break;
						case CON_CB_ERR_NO_SPACE: _console_print("No space left\n"); break;
						case CON_CB_ERR_BAD_PARAM: _console_print("Bad parameter\n"); break;
						case CON_CB_ERR_ARGS: _console_print("To few arguments\n"); break;
						default: break;
						}
					}
					else if(!error_code)
					{
						_console_print("Ok\n");
					}
					break;
				}
			}
			if(i == console_cmd_sz - 1)
			{
				console_print("command not found\n");
			}
		}
	}
	else
	{
		console_print("\n");
	}
}