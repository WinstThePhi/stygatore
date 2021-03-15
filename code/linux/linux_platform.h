#ifndef LINUX_PLATFORM_H
#define LINUX_PLATFORM_H

internal f32 get_time();

// TODO(winston): write the linux memory functions
internal void *request_mem(u32 size);

internal void free_mem(void *mem);

#endif
