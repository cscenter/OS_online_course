#include <stdint.h>

#include <string.h>

size_t strlen(const char *str)
{
	const char *begin = str;

	while (*str++);
	return str - begin - 1;
}

int strcmp(const char *l, const char *r)
{
	while (*l == *r && *l) {
		++l;
		++r;
	}

	if (*l != *r)
		return *l < *r ? -1 : 1;
	return 0;
}

char *strcpy(char *dst, const char *src)
{
	char *ptr = dst;

	while ((*ptr++ = *src++));
	return dst;
}

char *strchr(const char *str, int c)
{
	while (*str && *str != c)
		++str;
	return *str ? (char *)str : 0;
}

int memcmp(const void *l, const void *r, size_t size)
{
	const char *left = l;
	const char *right = r;

	for (size_t i = 0; i != size; ++i)
		if (left[i] != right[i])
			return left[i] < right[i] ? -1 : 1;
	return 0;
}

void *memcpy(void *dst, const void *src, size_t size)
{
	char *to = dst;
	const char *from = src;

	while (size--)
		*to++ = *from++;
	return dst;
}

void *memset(void *dst, int fill, size_t size)
{
	char *to = dst;

	while (size--)
		*to++ = fill;
	return dst;
}

void *memmove(void *dst, const void *src, size_t size)
{
	if ((uintptr_t)dst < (uintptr_t)src)
		return memcpy(dst, src, size);

	if (dst != src) {
		char *to = dst;
		const char *from = src;

		while (size--)
			to[size] = from[size];
	}
	return dst;
}
