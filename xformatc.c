/**
 * @file	xformatc.c
 *
 * @brief	Printf C implementation.
 *
 * Tested with the following operating systems / compilers :
 *
 * - Visual studio 6
 * - Visual studio 2008 / Windows CE
 * - MinGw 32
 * - Linux i686
 * - Linux x86_64
 * - GCC with embedded ARM (M0+/M3/M4/M4F)
 * - Linux armel
 * - Linux armhf
 * - HCS08 with Freescale compiler.
 *
 * - SDCC (Z80 and 8051)
 * 
 * @author	Mario Viara
 * 
 * 
 * @copyright	Copyright Mario Viara 2014	- License Open Source (LGPL)
 * This is a free software and is opened for education, research and commercial
 * developments under license policy of following terms:
 * 
 * - This is a free software and there is NO WARRANTY.
 * - No restriction on use. You can use, modify and redistribute it for personal,
 *	 non-profit or commercial product UNDER YOUR RESPONSIBILITY.
 * - Redistributions of source code must retain the above copyright notice.
 *
 * To contact the author send an email to mario_at_viara.eu
 *	Modified by VasiliSk
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include  "Resconfig.h"
#include  "xformatc.h"

/**
 * Default largest int is long
 */
#ifndef	LONG
#if XCFG_FORMAT_LONG
#define	LONG	long
#else
#define LONG	int
#endif
#endif

/**
 * Define the double type if not defined
 */
#ifndef DOUBLE
#define DOUBLE	double
#endif

/**
 * Default long long type
 */
#ifndef LONGLONG
#define LONGLONG   long long
#endif

/**
 * Definition to convert integer part of floating point
 * number if supported we use the long long type
 */
#if XCFG_FORMAT_LONGLONG
#define	FLOAT_LONG		LONGLONG
#define FLOAT_VALUE		llvalue
#define FLOAT_TYPE		FLAG_TYPE_LONGLONG
#else
#define FLOAT_LONG		LONG
#define FLOAT_VALUE		lvalue
#define FLOAT_TYPE		FLAG_TYPE_LONG
#endif

/**
 * Determine the precision of floating point number
 */
#if XCFG_FORMAT_FLOAT_PREC
#ifdef DOUBLE
#undef DOUBLE
#endif
#define DOUBLE			float
#define DOUBLE_ARGS		double
#else
#define DOUBLE_ARGS		DOUBLE
#endif

/**
 * Structure with all parameter used
 */
struct param_s {
	/**
	 * Buffer for current integer value and for temporary
	 * double value defined as union to save stack.
	 */
	union {
		unsigned LONG lvalue;
#if XCFG_FORMAT_LONGLONG
		unsigned LONGLONG llvalue;
#endif
#if XCFG_FORMAT_FLOAT
		DOUBLE dvalue;
#endif
	} values;

	/**
	 * Pointer to the output buffer
	 */
	char *out;

#if XCFG_FORMAT_FLOAT
	/**
	 * Floating point argument
	 */
	DOUBLE dbl;

	/**
	 * Integer part of floating point
	 */
	unsigned FLOAT_LONG iPart;

#endif

	/**
	 * Current length of the output buffer
	 */
	int length;

	/**
	 * Field precision
	 */
	int prec;

	/**
	 * Field width
	 */
	int width;

	/**
	 * Count the number of char emitted
	 */
	unsigned count;

	/**
	 * Parser flags
	 */
	unsigned flags;

#define FLAG_TYPE_INT		0x0000	/* Argument is integer					*/
#define FLAG_TYPE_LONG		0x0001	/* Argument is long						*/
#define FLAG_TYPE_SIZEOF	0x0002	/* Argument is size_t					*/
#define FLAG_TYPE_LONGLONG	0x0003	/* Argument is long long				*/
#define	FLAG_TYPE_MASK		0x0003	/* Mask for field type					*/
#define FLAG_PREC			0x0004	/* Precision set						*/
#define FLAG_LEFT			0x0008	/* Left alignment						*/
#define	FLAG_BLANK			0x0010	/* Blank before positive integer number */
#define	FLAG_PREFIX			0x0020	/* Prefix required						*/
#define FLAG_PLUS			0x0040	/* Force a + before positive number		*/
#define	FLAG_UPPER			0x0080	/* Output in upper case letter			*/
#define FLAG_DECIMAL		0x0100	/* Decimal field						*/
#define FLAG_INTEGER		0x0200	/* Integer field						*/
#define FLAG_MINUS			0x0400	/* Field is negative					*/
#define FLAG_VALUE			0x0800	/* Value set							*/
#define FLAG_BUFFER			0x1000	/* Buffer set							*/

	/**
	 * Length of the prefix
	 */
	int prefixlen;

	/* Buffer to store the field  prefix */
	char prefix[2];

	/** Buffer to store the biggest integer number in binary */
#if XCFG_FORMAT_LONGLONG
	char buffer[sizeof(LONGLONG) * 8 + 1];
#else
	char		buffer[sizeof(LONG)*8+1];
#endif

	/* Radix for ASCII integer conversion */
	unsigned char radix;

	/* char used for padding */
	char pad;

	/**
	 * Current state
	 */
	char state;

};

/**
 * Enum for the internal state machine
 */
enum State {
	/* Normal state outputting literal */
	ST_NORMAL = 0,

	/* Just read % */
	ST_PERCENT = 1,

	/* Just read flag */
	ST_FLAG = 2,

	/* Just read the width */
	ST_WIDTH = 3,

	/* Just read '.' */
	ST_DOT = 4,

	/* Just read the precision */
	ST_PRECIS = 5,

	/* Just	 read the size */
	ST_SIZE = 6,

	/* Just read the type specifier */
	ST_TYPE = 7
};

/**
 * Enum for char class
 */
enum CharClass {
	/* Other char */
	CH_OTHER = 0,
	/* The % char */
	CH_PERCENT = 1,
	/* The . char */
	CH_DOT = 2,
	/* The * char */
	CH_STAR = 3,
	/* The 0 char */
	CH_ZERO = 4,
	/* Digit 0-9 */
	CH_DIGIT = 5,
	/* Flag chars */
	CH_FLAG = 6,
	/* Size chars */
	CH_SIZE = 7,
	/* Type chars */
	CH_TYPE = 8
};

/**
 * String used when %s is a null parameter
 */
static const char ms_null[] = "(null)";
/*
 * String for true value
 */
static const char ms_true[] = "True";

/**
 * String for false value
 */
static const char ms_false[] = "False";

#if XCFG_FORMAT_FLOAT
static DOUBLE xpow10(int e) {
	DOUBLE result = 1.0;
	static const DOUBLE table[] = { 10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0 };
	static const int size = sizeof(table) / sizeof(table[0]);

	if (e > 0) {
		if (e > size) {
			result = table[size - 1];
			for (e = e - size; e; e--)
				result *= (DOUBLE) 10.0;
		} else
			result = table[e - 1];
	}

	return result;
}

#if XCFG_FORMAT_FLOAT_SPECIAL
static const char *checkFloat(DOUBLE value)
{
	switch (fpclassify(value))
	{
		case	FP_NAN:
				return "nan";

		case	FP_INFINITE:
				return "inf";

		case	FP_SUBNORMAL:
#ifndef XCFG_FORMAT_FLOAT_SUBISOK
			return "sub";
#endif
		default:
				return 0;
	}
}
#endif

#endif

/*
 * This table contains the next state for all char and it will be
 * generate using xformattable.c
 */

static const unsigned char formatStates[] = { 0x06, 0x00, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x03, 0x06, 0x00, 0x06, 0x02, 0x10, 0x04, 0x45, 0x45, 0x45, 0x45, 0x05, 0x05, 0x05,
		0x05, 0x35, 0x30, 0x00, 0x50, 0x60, 0x00, 0x00, 0x00, 0x20, 0x28, 0x38, 0x50, 0x50, 0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x50, 0x50, 0x00, 0x00, 0x08, 0x20, 0x20, 0x28, 0x20, 0x20,
		0x20, 0x00, 0x08, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, 0x70, 0x78, 0x78, 0x78, 0x70, 0x78, 0x00, 0x07, 0x08, 0x00, 0x00, 0x07, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x08,
		0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x07 };

static const char ms_digits[] = "0123456789abcdef";

#define U2A(name,type,value) \
static void name(struct param_s * param) \
{ \
	unsigned char digit; \
	type val = param->values.value ; \
	while (param->prec -- > 0 ||  val) \
	{ \
		switch (param->radix) \
		{ \
			case 2: \
				digit = val & 0x01; \
				val >>= 1; \
				break; \
			case 8: \
				digit = val & 0x07; \
				val >>= 3; \
				break; \
			case 16: \
				digit = val & 0x0F;\
				val  >>= 4; \
				break; \
			default: \
			case 10:  \
				digit = (unsigned char)(val % 10); \
				val  /= 10; \
				break; \
		} \
		*param->out -- = ms_digits[digit]; \
		param->length ++ ;\
	} \
}

/**
 * Convert an unsigned long value in one string
 *
 * All parameter are in the passed structure
 *
 * @param prec		- Minimum precision
 * @param lvalue	- Unsigned value
 * @param radix		- Radix (Supported values 2/8/10/16)
 *
 * @param out		- Buffer with the converted value.
 */
U2A(ulong2a, unsigned LONG, lvalue)
#if XCFG_FORMAT_LONGLONG
#ifdef XCFG_FORMAT_LONG_ARE_LONGLONG
#define	ullong2a	ulong2a
#else
U2A(ullong2a, unsigned LONGLONG, llvalue)
#endif
#endif

/**
 * Printf like using variable arguments.
 * 
 * @param outchar - Pointer to the function to output one new char.
 * @param arg	- Argument for the output function.
 * @param fmt	- Format options for the list of parameters.
 * @param ...	- Arguments
 *
 * @return The number of char emitted.
 * 
 * @see xvformat
 */
unsigned xformat(void (*outchar)(void*, char), void *arg, const char *fmt, ...) {
	va_list list;
	unsigned count;

	va_start(list, fmt);
	count = xvformat(outchar, arg, fmt, list);
	va_end(list);

	(void) list;

	return count;
}

/**
 * We do not want use any library function.
 *
 * @param s - C	 string
 * @return The length of the string
 */
static unsigned xstrlen(const char *s) {
	const char *i;

	for (i = s; *i; i++) {
	}

	return (unsigned) (i - s);
}

static unsigned outBuffer(void (*myoutchar)(void *arg, char), void *arg, const char *buffer, int len, unsigned toupper) {
	unsigned count = 0;
	int i;
	char c;

	for (i = 0; i < len; i++) {
		c = buffer[i];

		if (toupper && (c >= 'a' && c <= 'z')) {
			c = (char) (c - ('a' - 'A'));
		}

		(*myoutchar)(arg, c);
		count++;
	}

	return count;
}

static unsigned outChars(void (*myoutchar)(void *arg, char), void *arg, char ch, int len) {
	unsigned count = 0;

	while (len-- > 0) {
		(*myoutchar)(arg, ch);
		count++;
	}

	return count;
}

/*
 * Lint want declare list as const but list is an obscured pointer so
 * the warning is disabled.
 */
/*lint -save -e818 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

/**
 * Printf like format function.
 *
 * General format :
 * 
 * %[width][.precision][flags]type
 *
 * - width Is the minimum size of the field.
 * 
 * - precision Is the maximum size of the field.
 * 
 * Supported flags :
 * 
 * - l	With integer number the argument will be of type long.
 * - ll With integer number the argument will be of type long long.
 * -	Space for positive integer a space will be added before.
 * - z	Compatible with C99 the argument is size_t (aka sizeof(void *))
 * - +	A + sign prefix positive number.
 * - #	A prefix will be printed (o for octal,0x for hex,0b for binary)
 * - 0	Value will be padded with zero (default is spacwe)
 * - -	Left justify as default filed have rigth justification.
 * 
 * Supported type :
 * 
 * - s	Null terminated string of char.
 * - S	Null terminated string of char in upper case.
 * - i	Integer number.
 * - d	Integer number.
 * - u	Unsigned number.
 * - x	Unsigned number in hex.
 * - X	Unsigned number in hex upper case.
 * - b	Binary number
 * - o	Octal number
 * - p	Pointer will be emitted with the prefix ->
 * - P	Pointer in upper case letter.
 * - f	Floating point number.
 * - B	Boolean value printed as True / False.
 *
 * @param outchar - Pointer to the function to output one char.
 * @param arg	- Argument for the output function.
 * @param fmt	- Format options for the list of parameters.
 * @param args	- List parameters.
 *
 * @return The number of char emitted.
 */
unsigned xvformat(void (*outchar)(void*, char), void *arg, const char *fmt, va_list _args) {
	XCFG_FORMAT_STATIC struct param_s param;
	int i;
	char c;

#if XCFG_FORMAT_VA_COPY
	va_list args;

	va_copy(args,_args);
#else
#define args	_args
#endif

	param = (struct param_s){0};
	param.state = ST_NORMAL;

	while (*fmt) {
		c = *fmt++;

		if (c < ' ' || c > 'z')
			i = (int) CH_OTHER;
		else
			i = formatStates[c - ' '] & 0x0F;

		param.state = (char) (formatStates[(i << 3) + param.state] >> 4);

		switch (param.state) {
		default:
		case ST_NORMAL:
			(*outchar)(arg, c);
			param.count++;
			break;

		case ST_PERCENT:
			param.flags = param.length = param.prefixlen = param.width = param.prec = 0;
			param.pad = ' ';
			break;

		case ST_WIDTH:
			if (c == '*')
				param.width = (int) va_arg(args, int);
			else
				param.width = param.width * 10 + (c - '0');
			break;

		case ST_DOT:
			break;

		case ST_PRECIS:
			param.flags |= FLAG_PREC;
			if (c == '*')
				param.prec = (int) va_arg(args, int);
			else
				param.prec = param.prec * 10 + (c - '0');
			break;

		case ST_SIZE:
			switch (c) {
			default:
				break;
			case 'z':
				param.flags &= (unsigned) ~FLAG_TYPE_MASK;
				param.flags |= FLAG_TYPE_SIZEOF;
				break;

#if XCFG_FORMAT_LONG
			case 'l':
#if XCFG_FORMAT_LONGLONG
				if ((param.flags & FLAG_TYPE_MASK) == FLAG_TYPE_LONG) {
					param.flags &= (unsigned) ~FLAG_TYPE_MASK;
					param.flags |= FLAG_TYPE_LONGLONG;
				} else {
					param.flags &= (unsigned) ~FLAG_TYPE_MASK;
					param.flags |= FLAG_TYPE_LONG;

				}
#else
						param.flags &= ~FLAG_TYPE_MASK;
						param.flags |= FLAG_TYPE_LONG;
#endif
				break;
#endif

			}
			break;

		case ST_FLAG:
			switch (c) {
			default:
				break;
			case '-':
				param.flags |= FLAG_LEFT;
				break;
			case '0':
				param.pad = '0';
				break;
			case ' ':
				param.flags |= FLAG_BLANK;
				break;
			case '#':
				param.flags |= FLAG_PREFIX;
				break;
			case '+':
				param.flags |= FLAG_PLUS;
				break;
			}
			break;

		case ST_TYPE:
			switch (c) {
			default:
				param.length = 0;
				break;

				/*
				 * Pointer upper case
				 */
			case 'P':
				param.flags |= FLAG_UPPER;
				/*
				 * Pointer
				 */
				[[fallthrough]];
				//no break
			case 'p':
				param.flags &= (unsigned) ~FLAG_TYPE_MASK;
				param.flags |= FLAG_INTEGER | FLAG_TYPE_SIZEOF;
				param.radix = 16;
				param.prec = sizeof(void*) * 2;
				param.prefix[0] = '-';
				param.prefix[1] = '>';
				param.prefixlen = 2;
				break;

				/*
				 * Binary number
				 */
			case 'b':
				param.flags |= FLAG_INTEGER;
				param.radix = 2;
				if (param.flags & FLAG_PREFIX) {
					param.prefix[0] = '0';
					param.prefix[1] = 'b';
					param.prefixlen = 2;
				}
				break;

				/*
				 * Octal number
				 */
			case 'o':
				param.flags |= FLAG_INTEGER;
				param.radix = 8;
				if (param.flags & FLAG_PREFIX) {
					param.prefix[0] = '0';
					param.prefixlen = 1;
				}
				break;

				/*
				 * Hex number upper case letter.
				 */
			case 'X':
				/* no break */
				param.flags |= FLAG_UPPER;
				/*
				 * Hex number lower case
				 */
				[[fallthrough]];
				//no break
			case 'x':
				param.flags |= FLAG_INTEGER;
				param.radix = 16;
				if (param.flags & FLAG_PREFIX) {
					param.prefix[0] = '0';
					param.prefix[1] = 'x';
					param.prefixlen = 2;
				}
				break;

				/*
				 * Integer number radix 10
				 */
			case 'd':
			case 'i':
				param.flags |= FLAG_DECIMAL;
				/* no break */

				/* lint -fallthrough */

				/*
				 * Unsigned number
				 */
				[[fallthrough]];
				//no break
			case 'u':
				param.flags |= FLAG_INTEGER;
				param.radix = 10;
				break;

				/*
				 * Upper case string
				 */
			case 'S':
				param.flags |= FLAG_UPPER;
				/* no break */
				/*lint -fallthrough */

				/*
				 * Normal string
				 */

				[[fallthrough]];
				//no break
			case 's':
				param.out = va_arg(args, char*);
				if (param.out == 0)
					param.out = (char*) ms_null;
				param.length = (int) xstrlen(param.out);
				break;

				/*
				 * Upper case char
				 */
			case 'C':
				param.flags |= FLAG_UPPER;
				/* no break */
				/* lint -fallthrough */

				/*
				 * Char
				 */
				[[fallthrough]];
				//no break
			case 'c':
				param.out = param.buffer;
				param.buffer[0] = (char) va_arg(args, int);
				param.length = 1;
				break;

#if XCFG_FORMAT_FLOAT
				/**
				 * Floating point number
				 */
			case 'f':
				if (!(param.flags & FLAG_PREC)) {
					param.prec = 6;
				}

				param.values.dvalue = xpow10(param.prec);
#ifndef XCFG_FORMAT_FLOAT_HACK
				param.dbl = (DOUBLE) va_arg(args, DOUBLE_ARGS);
#else
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
				int fint = va_arg(args, int);
				param.dbl =  *((float*)(&fint));
#pragma GCC diagnostic pop
#endif

#if XCFG_FORMAT_FLOAT_SPECIAL
				param.out = (char *)checkFloat(param.dbl);
				if (param.out != 0)
				{
					param.length = (int)xstrlen(param.out);
				}
				else
				{
#endif
				if (param.dbl < 0) {
					param.flags |= FLAG_MINUS;
					param.dbl -= (DOUBLE) 0.5 / param.values.dvalue;
					param.iPart = (FLOAT_LONG) param.dbl;
					param.dbl -= (DOUBLE) (FLOAT_LONG) param.iPart;
					param.dbl = -param.dbl;
				} else {
					param.dbl += (DOUBLE) 0.5 / param.values.dvalue;
					param.iPart = (FLOAT_LONG) param.dbl;
					param.dbl -= (DOUBLE) param.iPart;
				}

				param.dbl *= param.values.dvalue;

				param.values.lvalue = (unsigned LONG) param.dbl;

				param.out = param.buffer + sizeof(param.buffer) - 1;
				param.radix = 10;
				if (param.prec) {
					ulong2a(&param);
					*param.out-- = '.';
					param.length++;
				}
				param.flags |= FLAG_INTEGER | FLAG_BUFFER |
				FLAG_DECIMAL | FLAG_VALUE | FLOAT_TYPE;

				param.prec = 0;
				param.values.FLOAT_VALUE = (unsigned FLOAT_LONG) param.iPart;

#if XCFG_FORMAT_FLOAT_SPECIAL
						}
#endif
				break;
#endif

				/**
				 * Boolean value
				 */
			case 'B':
				if (va_arg(args,int) != 0)
					param.out = (char*) ms_true;
				else
					param.out = (char*) ms_false;

				param.length = (int) xstrlen(param.out);
				break;

			}

			/*
			 * Process integer number
			 */
			if (param.flags & FLAG_INTEGER) {
				if (param.prec == 0)
					param.prec = 1;

				if (!(param.flags & FLAG_VALUE)) {
					switch (param.flags & FLAG_TYPE_MASK) {
					case FLAG_TYPE_SIZEOF:
						param.values.lvalue = (unsigned LONG) va_arg(args, void*);
						break;
					case FLAG_TYPE_LONG:
						if (param.flags & FLAG_DECIMAL)
							param.values.lvalue = (LONG) va_arg(args, long);
						else
							param.values.lvalue = (unsigned LONG) va_arg(args, unsigned long);
						break;

					case FLAG_TYPE_INT:
						if (param.flags & FLAG_DECIMAL)
							param.values.lvalue = (LONG) va_arg(args, int);
						else
							param.values.lvalue = (unsigned LONG) va_arg(args, unsigned int);
						break;
#if XCFG_FORMAT_LONGLONG
					case FLAG_TYPE_LONGLONG:
						param.values.llvalue = (LONGLONG) va_arg(args, long long);
						break;
#endif
					}

				}

				if ((param.flags & FLAG_PREFIX) && param.values.lvalue == 0) {
					param.prefixlen = 0;
				}

				/*
				 * Manage signed integer
				 */
				if (param.flags & FLAG_DECIMAL) {
#if XCFG_FORMAT_LONGLONG
					if ((param.flags & FLAG_TYPE_MASK) == FLAG_TYPE_LONGLONG) {
						if ((LONGLONG) param.values.llvalue < 0) {
							param.values.llvalue = ~param.values.llvalue + 1;
							param.flags |= FLAG_MINUS;
						}
					} else {
#endif
						if ((LONG) param.values.lvalue < 0) {
							param.values.lvalue = ~param.values.lvalue + 1;
							param.flags |= FLAG_MINUS;

						}
#if XCFG_FORMAT_LONGLONG
					}
#endif
					if (!(param.flags & FLAG_MINUS) && (param.flags & FLAG_BLANK)) {
						param.prefix[0] = ' ';
						param.prefixlen = 1;
					}
				}

				if ((param.flags & FLAG_BUFFER) == 0) {
					param.out = param.buffer + sizeof(param.buffer) - 1;
				}

#if XCFG_FORMAT_LONGLONG
				if ((param.flags & FLAG_TYPE_MASK) == FLAG_TYPE_LONGLONG)
					ullong2a(&param);
				else
					ulong2a(&param);
#else

					ulong2a(&param);
#endif
				param.out++;

				/*
				 * Check if a sign is required
				 */
				if (param.flags & (FLAG_MINUS | FLAG_PLUS)) {
					c = param.flags & FLAG_MINUS ? '-' : '+';

					if (param.pad == '0') {
						param.prefixlen = 1;
						param.prefix[0] = c;
					} else {
						*--param.out = c;
						param.length++;
					}
				}

			} else {
				if (param.width && param.length > param.width) {
					param.length = param.width;
				}

			}

			/*
			 * Now width contain the size of the pad
			 */
			param.width -= (param.length + param.prefixlen);

			param.count += outBuffer(outchar, arg, param.prefix, param.prefixlen, param.flags & FLAG_UPPER);
			if (!(param.flags & FLAG_LEFT))
				param.count += outChars(outchar, arg, param.pad, param.width);
			param.count += outBuffer(outchar, arg, param.out, param.length, param.flags & FLAG_UPPER);
			if (param.flags & FLAG_LEFT)
				param.count += outChars(outchar, arg, param.pad, param.width);

		}
	}

#if XCFG_FORMAT_VA_COPY
	va_end(args);
#endif

	return param.count;
}

#pragma GCC diagnostic pop
/*lint -restore */

