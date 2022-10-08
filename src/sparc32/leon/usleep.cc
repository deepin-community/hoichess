#include <sys/time.h>

int usleep(int usec)
{
	struct timeval tv;
	unsigned long long start, now;

	if (gettimeofday(&tv, NULL) == -1) {
		return -1;
	}
	start = (unsigned long long) tv.tv_sec * 1000000 +
		(unsigned long long) tv.tv_usec;
	now = start;

	while (now < start+usec) {
		if (gettimeofday(&tv, NULL) == -1) {
			return -1;
		}
		now = (unsigned long long) tv.tv_sec * 1000000 +
			(unsigned long long) tv.tv_usec;
	}

	return 0;	
}

