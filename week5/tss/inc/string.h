#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

size_t strlen(const char *str);
int strcmp(const char *l, const char *r);
char *strcpy(char *dst, const char *src);
char *strchr(const char *str, int c);
int memcmp(const void *l, const void *r, size_t size);
void *memcpy(void *dst, const void *src, size_t size);
void *memset(void *dst, int fill, size_t size);
void *memmove(void *dst, const void *src, size_t size);

#endif /*__STRING_H__*/
