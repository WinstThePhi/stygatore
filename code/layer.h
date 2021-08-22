#ifndef __LAYER_H_
#define __LAYER_H_

#include <stdint.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

typedef u8  b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

#define FALSE 0
#define TRUE  1

#define kilobytes(count) (1024 * count)
#define megabytes(count) (1024 * kilobytes(count))
#define gigabytes(count) (1024 * megabytes(count))

#define void_check assert

#define cast(variable_identifier, type) ((type)variable_identifier)

#endif
