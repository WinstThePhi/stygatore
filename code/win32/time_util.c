#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

#include "time_util.h"

static LARGE_INTEGER performance_frequency;

double get_time()
{
    static bool initialized = false;
    if(!initialized)
    {
        QueryPerformanceFrequency(&performance_frequency);
        initialized = true;
    }
    
    LARGE_INTEGER time_int;
    
    QueryPerformanceCounter(&time_int);
    
    double time = 
        (double)time_int.QuadPart / (double)performance_frequency.QuadPart;
    
    return time;
}

void sleep(uint32_t time)
{
    Sleep(time);
}

