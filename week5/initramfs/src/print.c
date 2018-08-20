#include <print.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <vga.h>


struct print_ctx {
	int (*out)(struct print_ctx *ctx, const char *buf, int size);
	int written;
};

static int print(struct print_ctx *ctx, const char *buf, int size)
{
	const int rc = ctx->out(ctx, buf, size);

	if (rc < 0)
		return rc;

	ctx->written += size;
	return 0;
}

enum format_type {
	FMT_INVALID,
	FMT_NONE,
	FMT_STR,
	FMT_CHAR,
	FMT_INT,
	FMT_PERCENT,
};

enum fromat_spec {
	FMT_SSHORT = (1 << 8),
	FMT_SHORT = (1 << 9),
	FMT_LONG = (1 << 10),
	FMT_LLONG = (1 << 11),
	FMT_UNSIGNED = (1 << 12),
	FMT_HEX = (1 << 13),
	FMT_OCT = (1 << 14)
};

static int decode_format(const char **fmt_ptr)
{
	const char *fmt = *fmt_ptr;
	int type = FMT_INVALID;
	int spec = 0;

	while (*fmt) {
		if (*fmt == '%' && type != FMT_INVALID)
			break;

		char c = *fmt++;

		if (c != '%') {
			type = FMT_NONE;
			continue;
		}

		switch (*fmt) {
		case '-': case '+': case ' ': case '#': case '0':
			++fmt;
			break;
		}

		while (*fmt && isdigit(*fmt))
			++fmt;

		if (*fmt == '.') {
			++fmt;
			while (*fmt && isdigit(*fmt))
				++fmt;
		}

		if (*fmt == 'l') {
			++fmt;
			if (*fmt == 'l') {
				spec = FMT_LLONG;
				++fmt;
			} else {
				spec = FMT_LONG;
			}
		} else if (*fmt == 'h') {
			++fmt;
			if (*fmt == 'h') {
				spec = FMT_SSHORT;
				++fmt;
			} else {
				spec = FMT_SHORT;
			}
		}

		switch ((c = *fmt++)) {
		case 'p':
			spec |= FMT_LONG;
			// fallthrough
		case 'x': case 'X': case 'o':
			spec |= c == 'o' ? FMT_OCT : FMT_HEX;
			// fallthrough
		case 'u':
			spec |= FMT_UNSIGNED;
			// fallthrough
		case 'd': case 'i':
			type = FMT_INT;
			break;
		case 's':
			type = FMT_STR;
			break;
		case 'c':
			type = FMT_CHAR;
			break;
		case '%':
			type = FMT_PERCENT;
			break;
		}
		break;
	}
	*fmt_ptr = fmt;

	return type | spec;
}

static int print_number(struct print_ctx *ctx, va_list args, int type)
{
	const int base = (type & FMT_HEX) ? 16 : ((type & FMT_OCT) ? 8 : 10);
	char buf[64];

	if (type & FMT_UNSIGNED) {
		unsigned long long value;

		if (type & FMT_LONG)
			value = va_arg(args, unsigned long);
		else if (type & FMT_LLONG)
			value = va_arg(args, unsigned long long);
		else
			value = va_arg(args, unsigned);
		ulltoa(value, buf, base);
	} else {
		long long value;

		if (type & FMT_LONG)
			value = va_arg(args, long);
		else if (type & FMT_LLONG)
			value = va_arg(args, long long);
		else
			value = va_arg(args, int);
		lltoa(value, buf, base);
	}

	return print(ctx, buf, strlen(buf));
}

int __vprintf(struct print_ctx *ctx, const char *fmt, va_list args)
{
	int rc = 0;

	ctx->written = 0;

	while (rc >= 0 && *fmt) {
		const char *start = fmt;
		const int type = decode_format(&fmt);

		switch (type & 0xff) {
		case FMT_STR: {
			const char *str = va_arg(args, const char *);

			rc = print(ctx, str, strlen(str));
			break;
		}
		case FMT_CHAR: {
			const char c = va_arg(args, int);

			rc = print(ctx, &c, 1);
			break;
		}
		case FMT_INT:
			rc = print_number(ctx, args, type);
			break;
		case FMT_NONE:
			rc = print(ctx, start, fmt - start);
			break;
		case FMT_PERCENT:
			rc = print(ctx, "%", 1);
			break;
		default:
			rc = -1;
			break;
		}
	}

	return rc < 0 ? rc : ctx->written;
}

static int vga_out(struct print_ctx *ctx, const char *buf, int size)
{
	(void) ctx;

	vga_write(buf, (size_t)size);

	return 0;
}

int vprintf(const char *fmt, va_list args)
{
	struct print_ctx ctx = {
		.out = &vga_out,
		.written = 0
	};

	return __vprintf(&ctx, fmt, args);
}

struct print_str_ctx {
	struct print_ctx ctx;
	char *buf;
	int size;
};

static int str_out(struct print_ctx *ctx, const char *buf, int size)
{
	struct print_str_ctx *sctx = (struct print_str_ctx *)ctx;

	if (ctx->written > sctx->size)
		return 0;

	const int remain = size < sctx->size - ctx->written
			? size : sctx->size - ctx->written;
	memcpy(sctx->buf + ctx->written, buf, remain);

	return 0;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	struct print_str_ctx ctx = {
		.ctx = {.out = &str_out},
		.buf = buf,
		.size = (int)size
	};
	const int rc = __vprintf(&ctx.ctx, fmt, args);

	if (rc >= 0)
		buf[rc < (int)size ? rc : (int)size - 1] = '\0';
	return rc;
}

int printf(const char *fmt, ...)
{
	va_list args;
	int rc;

	va_start(args, fmt);
	rc = vprintf(fmt, args);
	va_end(args);

	return rc;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int rc;

	va_start(args, fmt);
	rc = vsnprintf(buf, size, fmt, args);
	va_end(args);

	return rc;
}
