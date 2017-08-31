#include "stdio.h"
#include "malloc.h"

int next_int(char **p)
{
	int sum = 0;
	while(**p != ' ' && **p != '\n') 
		sum = sum * 10 + *((*p)++) - '0';

	while(**p == ' ' || **p == '\n')
		(*p)++;

	return sum;
}

char *next_str(char *dst, char **p)
{
	while(**p != ' ' && **p != '\n')
		*dst++ = *((*p)++);
	*dst = 0;

	while(**p == ' ' || **p == '\n')
		(*p)++;

	return dst;
}

int main()
{
	char sys_info[4096];
	memset(sys_info, 0, 4096);
	int fd = open("/dev/sys", 0);
	if(fd < 0) {
		printf("ps: open /dev/sys error\n");
		return -1;
	}
	int mt,mf,it,i_f,ut,kt,tt;
	char *p = sys_info;
	read(fd, sys_info, 4096);
	mt = next_int(&p);
	mf = next_int(&p);
	it = next_int(&p);
	i_f = next_int(&p);
	ut = next_int(&p);
	kt = next_int(&p);
	tt = next_int(&p);

	printf("Memory Total : %d kB\n", mt / 1024);
	printf("Memory Free  : %d kB\n", mf / 1024);
	printf("Inode Total  : %d\n", it);
	printf("Inode Free   : %d\n", i_f);
	printf("User Time    : %d:%02d:%02d(%d)\n", 
							ut / 1000 / 3600 % 24,
							ut / 1000 / 60 % 60,
							ut / 1000 % 60,
							ut);
	printf("Kern Time    : %d:%02d:%02d(%d)\n", 
							kt / 1000 / 3600 % 24,
							kt / 1000 / 60 % 60,
							kt / 1000 % 60,
							kt);
	printf("Total Time   : %d:%02d:%02d(%d)\n", 
							tt / 1000 / 3600 % 24,
							tt / 1000 / 60 % 60,
							tt / 1000 % 60,
							tt);
	close(fd);
	return 0;
}

