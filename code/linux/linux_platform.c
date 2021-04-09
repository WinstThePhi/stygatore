#include <time.h>
#include <sys/mman.h>

#include "../layer.h"
#include "linux_platform.h"

internal f32 
get_time()
{
	struct timespec time_spec_thing = {0};
	clock_gettime(CLOCK_REALTIME, &time_spec_thing);
	float seconds_elapsed = 
		((float)time_spec_thing.tv_nsec / (float)1e9);
	return seconds_elapsed;
}

internal void *
request_mem(u32 size)
{
	return mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

/* 
And life is never fair or sweet. It neither spoils nor coos its children. Its scoldings pierce deep and hard, 
its teachings are always accompanied by pain. If life were ever sweet, ever fair, ever kind, what joy would
there be? Without challenge, would there ever be passion? Without fear, would there be courage? To traverse the 
mountains of life, peaks and valleys and all, would a man truly grow. Where others find pain and anguish, a true 
man finds happiness and joy. And it's the challenges and struggles that make life truly worth living. 
*/ 
internal 
void free_mem(void *mem, u32 size)
{
       munmap(mem, size); 
}
