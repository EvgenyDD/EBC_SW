#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int sprintf(char *buffer, const char *format, ...);
int snprintf(char *buffer, size_t count, const char *format, ...);
int vsnprintf(char *buffer, size_t count, const char *format, va_list va);
int fctprintf(void (*out)(char character, void *arg), void *arg, const char *format, ...);

#endif // PRINTF_H
