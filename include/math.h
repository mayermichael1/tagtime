#ifndef MATH_H
#define MATH_H

#include "general.h"

#define MIN(a, b) (a < b) ? a : b;
#define MAX(a, b) (a > b) ? a : b;

typedef union
{
    struct 
    {
        u32 x;
        u32 y;
    };
    struct 
    {
        u32 width;
        u32 height;
    };
}
v2u32;

typedef struct
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
}
v4u8;

typedef struct
{
    f32 r;
    f32 g;
    f32 b;
    f32 a;
}
v4f32;


u64 
pow_u64(u64 value, u32 exponent)
{
    u64 pow = 1;
    for(u32 i = 0; i < exponent; ++i)
    {
        pow *= value;
    }
    return(value);
}

/// BIT OPERATIONS

#define SET_BIT(value, bit_to_set) (value | (1 << bit_to_set))
#define UNSET_BIT(value, bit_to_set) (value & ~(1 << bit_to_set))
#define CHECK_BIT(value, bit_to_check) ((value & (1 << bit_to_check)) != 0)
#define MASK(value, mask) (value & mask)

#endif
