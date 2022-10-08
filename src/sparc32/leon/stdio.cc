#include <stdio.h>
#include <unistd.h>

/* fgets() of leonbare seems to be somehow broken.. */
char* fgets(char* buf, int n, FILE* fp)
{
	int fd = fileno(fp);
	if (fd == -1) {
		return NULL;
	}

	char * p;
	for (p = buf; p < buf+n-1; p++) {
		if (read(fd, p, 1) != 1) {
			return NULL;
		}

		if (*p == '\n' || *p == '\r') {
			putc('\r', stdout);
			putc('\n', stdout);
			*p = '\n'; // replace \r by \n
			break;
		} else {
			putc(*p, stdout);
		}
	}
	*(p+1) = '\0';

	return buf;
}

