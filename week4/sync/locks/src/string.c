#include <stdint.h>

#include <string.h>

size_t strlen(const char *str)
{
	const char *begin = str;

	while (*str++);
	return str - begin - 1;
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
