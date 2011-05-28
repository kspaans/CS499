/* printf and related functions */

#include <lib.h>
#include <string.h>

// maximum possible length of a 64-bit integer represented as a string
// this assumes that the lowest base allowable is octal (base 8)
#define INTLEN 23

#define F_MINUS   1 /* "-" flag */
#define F_PLUS    2 /* "+" flag */
#define F_SPACE   4 /* " " flag */
#define F_NUM     8 /* "#" flag */
#define F_ZERO   16 /* "0" flag */
#define F_QUOTE  32 /* "'" flag */
#define F_UPCASE 64 /* uppercase format */
#define F_UNSIGNED  128 /* unsigned format */
#define F_BINARYBASE 256 /* integer base is a power of two */

struct pf {
	printfunc_t printfunc;
	void *data;
	int flags; /* F_ constants */
	int base; /* integer base or power-of-two base */
	int width; /* width spec */
	int precision; /* precision spec */
	int longness; /* long format */
};

#define PADCHUNK 8
#define PADSPACE "        "
#define PADZERO "00000000"

static inline int _p_pad(struct pf *pf, const char *padstr, int padlen) {
	int ret = padlen;
	while(padlen >= PADCHUNK) {
		pf->printfunc(pf->data, padstr, PADCHUNK);
		padlen -= PADCHUNK;
	}
	if(padlen > 0)
		pf->printfunc(pf->data, padstr, padlen);
	return ret;
}

static int _p_fmtstr(struct pf *pf, const char *str) {
	if(str == NULL)
		str = "(null)";
	int len = 0;
	if(pf->precision < 0) len = strlen(str);
	else {
		const char *cur = str;
		while(*cur++ && len < pf->precision) len++;
	}
	int padlen = pf->width - len;
	if(padlen < 0) padlen = 0;

	if(pf->flags & F_MINUS) {
		pf->printfunc(pf->data, str, len);
		_p_pad(pf, PADSPACE, padlen);
	} else {
		_p_pad(pf, PADSPACE, padlen);
		pf->printfunc(pf->data, str, len);
	}
	return len+padlen;
}

/* take a digit and produce its character value */
static char _p_d2a(int d, int uppercase) {
	if(d < 10) return '0'+d;
	else if(uppercase) return 'A'+d-10;
	else return 'a'+d-10;
}

/* All four _p_ui2a functions return an *empty string* if the input number is zero.
 * This is by design.
 */
/* convert an unsigned int to a string */
static int _p_ui2a(unsigned int num, char *end, char **start, int uppercase, int base) {
	char *cur = end;
	while(num) {
		*cur-- = _p_d2a(num%base, uppercase);
		num /= base;
	}
	*start = cur+1;
	return end-cur;
}

/* convert an unsigned int to a string in a power-of-two base. */
static int _p_ui2ab(unsigned int num, char *end, char **start, int uppercase, int shift) {
	int mask = (1<<shift)-1;
	char *cur = end;
	while(num) {
		*cur-- = _p_d2a(num&mask, uppercase);
		num >>= shift;
	}
	*start = cur+1;
	return end-cur;
}

/* convert an unsigned long long to a string */
static int _p_ull2a(unsigned long long num, char *end, char **start, int uppercase, int base) {
	char *cur = end;
	while(num) {
		*cur-- = _p_d2a(num%base, uppercase);
		num /= base;
	}
	*start = cur+1;
	return end-cur;
}

/* convert an unsigned int to a string in a power-of-two base. */
static int _p_ull2ab(unsigned long long num, char *end, char **start, int uppercase, int shift) {
	int mask = (1<<shift)-1;
	char *cur = end;
	while(num) {
		*cur-- = _p_d2a(num&mask, uppercase);
		num >>= shift;
	}
	*start = cur+1;
	return end-cur;
}

static int _p_fmtint(struct pf *pf, long long num) {
	unsigned long long unum;
	char sign = 0;
	char *hex = NULL;

	if(pf->flags & F_UNSIGNED) {
		unum = num;
	} else if(num < 0) {
		unum = -num;
		sign = '-';
	} else {
		unum = num;
		if(pf->flags & F_PLUS)
			sign = '+';
		else if(pf->flags & F_SPACE)
			sign = ' ';
	}

	int len;
	char *start;
	char buf[INTLEN];
	buf[INTLEN-1] = '\0';
	int precision = pf->precision;
	if(precision < 0) precision = 1;
	if(pf->flags & F_BINARYBASE) {
		if(pf->longness >= 2) {
			len = _p_ull2ab(unum, buf+INTLEN-2, &start, pf->flags & F_UPCASE, pf->base);
		} else {
			len = _p_ui2ab((unsigned int)unum, buf+INTLEN-2, &start, pf->flags & F_UPCASE, pf->base);
		}
		if((pf->flags & F_NUM) && unum != 0) {
			if(pf->base == 3 && pf->precision <= len) {
				precision = len+1;
			} else if(pf->base == 4) {
				hex = (pf->flags & F_UPCASE)?"0X":"0x";
			}
		}
	} else {
		if(pf->longness >= 2) {
			len = _p_ull2a(unum, buf+INTLEN-2, &start, pf->flags & F_UPCASE, pf->base);
		} else {
			len = _p_ui2a((unsigned int)unum, buf+INTLEN-2, &start, pf->flags & F_UPCASE, pf->base);
		}
	}
	if(precision < len) precision = len;
	int zpadlen = precision - len;
	int spadlen = pf->width - precision - (sign != 0) - 2*(hex != 0);
	if(zpadlen < 0) zpadlen = 0;
	if(spadlen < 0) spadlen = 0;

	if((pf->flags & F_ZERO) && (pf->precision < 0) && !(pf->flags & F_MINUS)) {
		zpadlen += spadlen;
		spadlen = 0;
	}

	/* Start output */
	if(!(pf->flags & F_MINUS))
		_p_pad(pf, PADSPACE, spadlen);
	if(sign)
		pf->printfunc(pf->data, &sign, 1);
	if(hex)
		pf->printfunc(pf->data, hex, 2);
	_p_pad(pf, PADZERO, zpadlen);
	pf->printfunc(pf->data, start, len);
	if(pf->flags & F_MINUS)
		_p_pad(pf, PADSPACE, spadlen);
	return spadlen + zpadlen + len + (sign != 0) + 2*(hex != 0);
}

int func_vprintf(printfunc_t printfunc, void *data, const char *fmt, va_list va) {
	struct pf pf;
	pf.printfunc = printfunc;
	pf.data = data;
	char c;
	int len = 0;
	const char *runstart = NULL;

	while(1) {
		c = *fmt++;
		if(c && c != '%') {
			if(!runstart)
				runstart = fmt-1;
			continue;
		}
		if(runstart) {
			printfunc(data, runstart, fmt-runstart-1);
			runstart = NULL;
		}
		if(c == 0)
			break;
		pf.flags = 0;
		pf.base = 10;
		pf.width = 0;
		pf.precision = -1;
		pf.longness = 0;

		c = *(fmt++);
		while(1) {
			switch(c) {
			case '-':
				pf.flags |= F_MINUS;
				c = *fmt++;
				continue;
			case '+':
				pf.flags |= F_PLUS;
				c = *fmt++;
				continue;
			case ' ':
				pf.flags |= F_SPACE;
				c = *fmt++;
				continue;
			case '#':
				pf.flags |= F_NUM;
				c = *fmt++;
				continue;
			case '0':
				pf.flags |= F_ZERO;
				c = *fmt++;
				continue;
			case '\'':
				pf.flags |= F_QUOTE;
				c = *fmt++;
				continue;
			}
			break;
		}
		switch(c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			pf.width = strtol(fmt-1, &fmt, 10);
			c = *fmt++;
			break;
		case '*':
			pf.width = va_arg(va, int);
			c = *fmt++;
			break;
		}
		if(c=='.') {
			pf.precision = 0;
			c = *fmt++;
			switch(c) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				pf.precision = strtol(fmt-1, &fmt, 10);
				c = *fmt++;
				break;
			case '*':
				pf.precision = va_arg(va, int);
				c = *fmt++;
				break;
			}
		}

		do {
			switch(c) {
			case '\0':
				goto end;
			case 'l':
				pf.longness++;
				c = *(fmt++);
				continue;
			case 'h':
				pf.longness--;
				c = *(fmt++);
				continue;
			case 'u':
				pf.flags |= F_UNSIGNED;
				/* fall through */
			case 'i':
			case 'd':
				if(pf.longness >= 2)
					len += _p_fmtint(&pf, va_arg(va, long long));
				else
					len += _p_fmtint(&pf, va_arg(va, int));
				break;
			case 'o':
			case 'X':
			case 'x':
				switch(c) {
				case 'o':
					pf.base = 3;
					break;
				case 'X':
					pf.flags |= F_UPCASE;
					/* fall through */
				case 'x':
					pf.base = 4;
					break;
				}
				pf.flags |= F_UNSIGNED | F_BINARYBASE;
				if(pf.longness >= 2)
					len += _p_fmtint(&pf, va_arg(va, long long));
				else
					len += _p_fmtint(&pf, va_arg(va, int));
				break;
			case 'p':
				pf.flags |= F_UNSIGNED | F_BINARYBASE | F_NUM;
				pf.base = 4;
				pf.precision = 8;
				len += _p_fmtint(&pf, (unsigned long)va_arg(va, void*));
				break;
			case 's':
				len += _p_fmtstr(&pf, va_arg(va, char*));
				break;
			case 'c':
				c = va_arg(va, int);
				/* fall through */
			default:
				len++;
				printfunc(data, &c, 1);
				break;
			}
			break;
		} while(1);
	}
end:
	return len;
}

int func_printf(printfunc_t printfunc, void *data, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int ret = func_vprintf(printfunc, data, fmt, va);
	va_end(va);
	return ret;
}

static void printfunc_sprintf(void *data, const char *buf, size_t len) {
	char *ptr = *(char **)data;
	while(len--) {
		*ptr++ = *buf++;
	}
	*ptr = '\0';
	*(char **)data = ptr;
}

int vsprintf(char *buf, const char *fmt, va_list va) {
	return func_vprintf(printfunc_sprintf, &buf, fmt, va);
}

int sprintf(char *buf, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int ret = vsprintf(buf, fmt, va);
	va_end(va);
	return ret;
}
