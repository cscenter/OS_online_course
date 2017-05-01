#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

unsigned long strtoul(const char *str, char **endptr, int base)
{
	const char *ptr = str;
	unsigned long ret = 0;
	int negate = 0, consumed = 0;

	while (isspace(*ptr))
		++ptr;

	if (*ptr == '-' || *ptr == '+') {
		negate = *ptr == '-';
		++ptr;
	}

	if (!base) {
		base = 10;
		if (*ptr == '0') {
			base = 8;
			if (tolower(*(++ptr)) == 'x') {
				base = 16;
				++ptr;
			}
		}
	} else {
		if (base == 8 && *ptr == '0')
			++ptr;
		if (base == 16 && *ptr == '0' && tolower(*(ptr + 1) == 'x'))
			ptr += 2;
	}

	while (*ptr) {
		const char c = toupper(*ptr);
		const int digit = isdigit(c) ? c - '0' : c - 'A' + 10;

		if (ret > (ULONG_MAX - digit) / base) {
			ret = ULONG_MAX;
			break;
		}
		ret = ret * base + digit;
		consumed = 1;
		++ptr;
	}

	if (endptr)
		*endptr = (char *)(consumed ? ptr : str);

	if (negate)
		ret = -ret;

	return ret;
}

char *ulltoa(unsigned long long value, char *str, int base)
{
	static const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	char *ptr = str;
	char *start = str;

	do {
		const int r = value % base;

		value = value / base;
		*ptr++ = digits[r];
	} while (value);

	*ptr-- = '\0';

	while (start < ptr) {
		const char c = *ptr;

		*ptr-- = *start;
		*start++ = c;
	}

	return str;
}

char *lltoa(long long value, char *str, int base)
{
	char *ptr = str;

	if (value < 0) {
		*ptr++ = '-';
		value = -value;
	}

	ulltoa(value, ptr, base);

	return str;
}

char *ultoa(unsigned long value, char *str, int base)
{
	return ulltoa(value, str, base);
}

char *ltoa(long value, char *str, int base)
{
	return lltoa(value, str, base);
}

char *utoa(unsigned value, char *str, int base)
{
	return ulltoa(value, str, base);
}

char *itoa(int value, char *str, int base)
{
	return lltoa(value, str, base);
}
