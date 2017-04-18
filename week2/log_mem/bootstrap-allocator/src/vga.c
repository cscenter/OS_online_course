#include <stddef.h>
#include <memory.h>
#include <vga.h>


static char * const VBUF = VA(0xB8000);
static const int ROWS = 25;
static const int COLS = 80;
static const int ATTR = 15;


static char *__vga_at(int r, int c)
{
	return VBUF + (r * COLS * 2 + c);
}

static void vga_set(int r, int c, char x)
{
	*__vga_at(r, 2 * c) = x;
	*__vga_at(r, 2 * c + 1) = ATTR;
}

static char vga_get(int r, int c)
{
	return *__vga_at(r, 2 * c);
}

static void scroll(void)
{
	for (int r = 1; r != ROWS; ++r)
		for (int c = 0; c != COLS; ++c)
			vga_set(r - 1, c, vga_get(r, c));
}

static void putchar(char x)
{
	static int r = 0;
	static int c = 0;

	if (x == '\n') {
		for (; c != COLS; ++c)
			vga_set(r, c, ' ');
		c = 0;
		++r;
	} else {
		vga_set(r, c, x);
		if (++c == COLS) {
			c = 0;
			++r;
		}
	}

	if (r == ROWS) {
		scroll();
		--r;
	}
}

void vga_write(const char *data, size_t size)
{
	for (size_t i = 0; i != size; ++i)
		putchar(data[i]);
}

void vga_clr(void)
{
	for (int r = 0; r != ROWS; ++r)
		for (int c = 0; c != COLS; ++c)
			vga_set(r, c, ' ');
}
