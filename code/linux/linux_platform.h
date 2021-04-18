#ifndef LINUX_PLATFORM_H
#define LINUX_PLATFORM_H

internal f32 GetTime();

// TODO(winston): write the linux memory functions
internal void *RequestMem(u32 size);

internal void FreeMem(void *mem, u32 size);

#endif
