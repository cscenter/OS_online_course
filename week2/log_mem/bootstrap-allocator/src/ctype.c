#include <ctype.h>

#define CTYPE_SP	0x01	/* spaces */
#define CTYPE_DI	0x02	/* digits */
#define CTYPE_LO	0x04	/* lower case letters */
#define CTYPE_UP	0x08	/* upper case letters */
#define CTYPE_XD	0x10	/* hex digits */
#define CTYPE_PU	0x20	/* punctutation marks */

/* special for space, since it's the only printable space character */
#define CTYPE_XS	0x40

static const unsigned char ctype[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	CTYPE_SP,		/* tab */
	CTYPE_SP,		/* line feed */
	CTYPE_SP,		/* vertical tab */
	CTYPE_SP,		/* form feed */
	CTYPE_SP,		/* carriage return */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	CTYPE_XS | CTYPE_SP,	/* space */
	CTYPE_PU,		/* ! */
	CTYPE_PU,		/* " */
	CTYPE_PU,		/* # */
	CTYPE_PU,		/* $ */
	CTYPE_PU,		/* % */
	CTYPE_PU,		/* & */
	CTYPE_PU,		/* ' */
	CTYPE_PU,		/* ( */
	CTYPE_PU,		/* ) */
	CTYPE_PU,		/* * */
	CTYPE_PU,		/* + */
	CTYPE_PU,		/* , */
	CTYPE_PU,		/* - */
	CTYPE_PU,		/* . */
	CTYPE_PU,		/* / */
	CTYPE_DI | CTYPE_XD,	/* 0 */
	CTYPE_DI | CTYPE_XD,	/* 1 */
	CTYPE_DI | CTYPE_XD,	/* 2 */
	CTYPE_DI | CTYPE_XD,	/* 3 */
	CTYPE_DI | CTYPE_XD,	/* 4 */
	CTYPE_DI | CTYPE_XD,	/* 5 */
	CTYPE_DI | CTYPE_XD,	/* 6 */
	CTYPE_DI | CTYPE_XD,	/* 7 */
	CTYPE_DI | CTYPE_XD,	/* 8 */
	CTYPE_DI | CTYPE_XD,	/* 9 */
	CTYPE_PU,		/* : */
	CTYPE_PU,		/* ; */
	CTYPE_PU,		/* < */
	CTYPE_PU,		/* = */
	CTYPE_PU,		/* > */
	CTYPE_PU,		/* ? */
	CTYPE_PU,		/* @ */
	CTYPE_UP | CTYPE_XD,	/* A */
	CTYPE_UP | CTYPE_XD,	/* B */
	CTYPE_UP | CTYPE_XD,	/* C */
	CTYPE_UP | CTYPE_XD,	/* D */
	CTYPE_UP | CTYPE_XD,	/* E */
	CTYPE_UP | CTYPE_XD,	/* F */
	CTYPE_UP,		/* G */
	CTYPE_UP,		/* H */
	CTYPE_UP,		/* I */
	CTYPE_UP,		/* J */
	CTYPE_UP,		/* K */
	CTYPE_UP,		/* L */
	CTYPE_UP,		/* M */
	CTYPE_UP,		/* N */
	CTYPE_UP,		/* O */
	CTYPE_UP,		/* P */
	CTYPE_UP,		/* Q */
	CTYPE_UP,		/* R */
	CTYPE_UP,		/* S */
	CTYPE_UP,		/* T */
	CTYPE_UP,		/* U */
	CTYPE_UP,		/* V */
	CTYPE_UP,		/* W */
	CTYPE_UP,		/* X */
	CTYPE_UP,		/* Y */
	CTYPE_UP,		/* Z */
	CTYPE_PU,		/* [ */
	CTYPE_PU,		/* \ */
	CTYPE_PU,		/* ] */
	CTYPE_PU,		/* ^ */
	CTYPE_PU,		/* _ */
	CTYPE_PU,		/* ` */
	CTYPE_LO | CTYPE_XD,	/* a */
	CTYPE_LO | CTYPE_XD,	/* b */
	CTYPE_LO | CTYPE_XD,	/* c */
	CTYPE_LO | CTYPE_XD,	/* d */
	CTYPE_LO | CTYPE_XD,	/* e */
	CTYPE_LO | CTYPE_XD,	/* f */
	CTYPE_LO,		/* g */
	CTYPE_LO,		/* h */
	CTYPE_LO,		/* i */
	CTYPE_LO,		/* j */
	CTYPE_LO,		/* k */
	CTYPE_LO,		/* l */
	CTYPE_LO,		/* m */
	CTYPE_LO,		/* n */
	CTYPE_LO,		/* o */
	CTYPE_LO,		/* p */
	CTYPE_LO,		/* q */
	CTYPE_LO,		/* r */
	CTYPE_LO,		/* s */
	CTYPE_LO,		/* t */
	CTYPE_LO,		/* u */
	CTYPE_LO,		/* v */
	CTYPE_LO,		/* w */
	CTYPE_LO,		/* x */
	CTYPE_LO,		/* y */
	CTYPE_LO,		/* z */
	CTYPE_PU,		/* { */
	CTYPE_PU,		/* | */
	CTYPE_PU,		/* } */
	CTYPE_PU,		/* ~ */
	0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int isprint(int c)
{
	return ctype[(unsigned char)c] &
		(CTYPE_LO | CTYPE_UP | CTYPE_DI | CTYPE_PU | CTYPE_XS);
}

int isalpha(int c)
{ return ctype[(unsigned char)c] & (CTYPE_LO | CTYPE_UP); }

int isdigit(int c)
{ return ctype[(unsigned char)c] & CTYPE_DI; }

int isxdigit(int c)
{ return ctype[(unsigned char)c] & CTYPE_XD; }

int isspace(int c)
{ return ctype[(unsigned char)c] & CTYPE_SP; }

int islower(int c)
{ return ctype[(unsigned char)c] & CTYPE_LO; }

int isupper(int c)
{ return ctype[(unsigned char)c] & CTYPE_UP; }

int toupper(int c)
{ return islower(c) ? c - 0x20 : c; }

int tolower(int c)
{ return isupper(c) ? c + 0x20 : c; }

