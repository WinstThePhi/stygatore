#include <time.h>
#include "time_util.h"

float get_time()
{
	struct timespec time_spec_thing = {0};
	clock_gettime(CLOCK_REALTIME, &time_spec_thing);
	float seconds_elapsed = 
		((float)time_spec_thing.tv_nsec / (float)1e9);
	return seconds_elapsed;
}
