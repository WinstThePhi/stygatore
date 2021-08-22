#if !defined(WIN32_PLATFORM_H)
#define WIN32_PLATFORM_H

internal f64 get_time();

internal void *request_mem(u32 size);

internal void free_mem(void *mem, u32 size);

#endif 
