#ifndef TIMERS_H
#define TIMERS_H

#include <sys/time.h>

#define NANO_PER_SEC 1000000000
#define NANO_PER_MILLI 1000000

typedef unsigned long long nano_t;

timespec diff(const timespec& start, const timespec& end)
{
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec) < 0)
	{
		temp.tv_sec = end.tv_sec-start.tv_sec - 1;
		temp.tv_nsec = NANO_PER_SEC + end.tv_nsec - start.tv_nsec;
	}
	else
	{
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

nano_t nano_convert(const timespec& time)
{
	nano_t result = 0;
	result = ((nano_t) time.tv_sec * NANO_PER_SEC) + (nano_t) time.tv_nsec;
	return result;
}

struct Timer
{
private:
	timespec _start;

public:
	void start()
	{
		clock_gettime(CLOCK_REALTIME, &_start);
	}

	bool timeout(unsigned int milli_timeout)
	{
		timespec end;
		clock_gettime(CLOCK_REALTIME, &end);
		nano_t nano_timeout = (nano_t)milli_timeout * NANO_PER_MILLI;

		if (nano_convert(diff(_start, end)) > nano_timeout)
			return true;

		return false;
	}
};

#endif