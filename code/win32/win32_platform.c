#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

#include "win32_platform.h"
#include "layer.h"

global LARGE_INTEGER performance_frequency;

internal f64 
get_time()
{
    static b32 initialized = FALSE;
    if(!initialized)
    {
        QueryPerformanceFrequency(&performance_frequency);
        initialized = TRUE;
    }
    
    LARGE_INTEGER time_int;
    
    QueryPerformanceCounter(&time_int);
    
    f64 time = 
    (f64)time_int.QuadPart / (f64)performance_frequency.QuadPart;
    
    return time;
}

internal void *
request_mem(u32 size) 
{
    return VirtualAlloc(0,
                        size,
                        MEM_COMMIT | MEM_RESERVE,
                        PAGE_READWRITE);
}

internal void 
free_mem(void *mem, u32 size)
{
    VirtualFree(mem,
                0,
                MEM_RELEASE);
}
