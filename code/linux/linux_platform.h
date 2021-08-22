#if !defined(LINUX_PLATFORM_H)
#define LINUX_PLATFORM_H

static f32 get_time();

static void *request_mem(u32 size);

static void free_mem(void *mem, u32 size);

#endif
