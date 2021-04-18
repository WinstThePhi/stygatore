#ifndef WIN32_PLATFORM_H
#define WIN32_PLATFORM_H

internal f64 GetTime();

internal void *RequestMem(u32 size);

internal void FreeMem(void *mem, u32 size);

#endif 
