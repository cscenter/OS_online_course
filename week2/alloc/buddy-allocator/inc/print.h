#ifndef __PRINT_H__
#define __PRINT_H__

#include <stddef.h>
#include <stdarg.h>

int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list args);

#endif /*__PRINT_H__*/
