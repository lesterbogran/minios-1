#include "kernel.h"

void *memmove(void *dst, void *src, int len) 
{
	char *d = dst;
	char *s = src;;
	while(len-- > 0)
		*d++ = *s++;
	return dst;
}

void *memset(void *dst, int data, int len) 
{
	stosb(dst, data, len);
	return dst;
}

void cprintfint(int data, int base, bool sign) {
	static char digitals[] = "0123456789ABCDEF";
	char buf[32] = {0};

	uint abs_data = 0;
	if(sign && (sign = data < 0))
		abs_data = -data;
	else
		abs_data = data;

	int i = 0;
	do {
		buf[i++] = digitals[abs_data % base];
	}while((abs_data /= base) != 0);

	if(base == 16) {
		buf[i++] = 'x';
		buf[i++] = '0';
	}
	if(sign)
		buf[i++] = '-';

	while(--i >= 0)
		cputc(buf[i]);
}

void vprintf(char *fmt, uint *argp) 
{
	int c;
	//uint *argp;
	char *s;
	int i = 0;
	for(;(c = fmt[i]&0xff) != 0; i++) {
		if(c != '%'){
			cputc(c);
			continue;
		}
		c = fmt[++i]&0xff;
		if(c == 0)
			break;
		switch(c) {
			case 's':
				if((s = (char *)*argp++) == 0){
					s= "(null)";
				}
				for(; *s != 0; s++) {
					cputc(*s);
				}
			break;
			case 'd':
				cprintfint(*argp++, 10, true);
			break;
			case 'x':
			case 'p':
				cprintfint(*argp++, 16, false);
			break;
			case '%':
				cputc('%');
			break;
			default:
				cputc('%');
				cputc(c);
			break;
		}
	}
}

void cprintf(char *fmt, ...) {
	uint *argp = (uint*)(&fmt + 1);
	vprintf(fmt, argp);
}

void panic(char *fmt, ...)
{
	uint *argp = (uint*)(&fmt + 1);
	cprintf("MINIOS PANIC: ");
	vprintf(fmt, argp);
	for(;;);
}

void cputc(int c) 
{
	cga_putc(c);
	uart_putc(c);
}

