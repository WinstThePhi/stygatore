#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

#include "time_util.h"
#include "../layer.h"

static LARGE_INTEGER performance_frequency;

double get_time()
{
        static b32 initialized = FALSE;
        if(!initialized)
        {
                QueryPerformanceFrequency(&performance_frequency);
                initialized = TRUE;
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

