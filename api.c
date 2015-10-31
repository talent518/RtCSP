#include "api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

inline double microtime()
{
	struct timeval tp = {0};

	if (gettimeofday(&tp, NULL)) {
		return 0;
	}

	return tp.tv_sec + ((double)tp.tv_usec / 1000000);
}

inline char *fsize(int size)
{
	char units[5][3]={"B","KB","MB","GB","TB"};
	char buf[10];
	int unit=(int)(log(size)/log(1024));

	if (unit>4)
	{
		unit=4;
	}

	sprintf(buf, "%.3f%s", size/pow(1024,unit), units[unit]);

	return strdup(buf);
}

inline int execi(const char *cmd)
{
	FILE *fp;
	int ret=0;
	fp=popen(cmd,"r");
	if (fp!=NULL)
	{
		fscanf(fp,"%d",&ret);
		fclose(fp);
	}
	return ret;
}

inline char *str_repeat(const char *str,size_t str_len,size_t repeat)
{
	char *ret=(char *)malloc(str_len*repeat);
	size_t i;
	for (i=0;i<repeat;i++)
	{
		strncpy(ret+i*str_len,str,str_len);
	}
	return ret;
}

inline void strnprint(const char *str,size_t repeat)
{
	size_t i;
	for (i=0;i<repeat;i++)
	{
		printf(str);
	}
}