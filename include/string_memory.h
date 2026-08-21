#ifndef STRING_MEMORY_H
#define STRING_MEMORY_H

#include "memory.h"
#include "math.h"

global_variable struct mem_arena string_local_temp_mem = {};

struct string
create_mem_string(u32 size, struct mem_arena *mem)
{
    struct string str = {};
    str.size = size;
    str.data = ARENA_PUSH_ARRAY(mem, u8, str.size);
    return(str);
}

const char *
to_c_string(struct string str, struct mem_arena *scratch)
{
    u8* cstring = ARENA_PUSH_ARRAY(scratch, u8, (str.size+1));
    for(u32 i = 0; i < str.size; ++i)
    {
        cstring[i] = str.data[i];
    }
    cstring[str.size] = 0;
    return((const char*)cstring);
}

/// copy a string to a backing store
///
/// this copies a given string to a memory arena
///
/// @param  string to copy
/// @param  scratch memory space to copy onto
///
/// @return returns the string struct
//TODO: test this function
/*
string 
string_copy(string str, mem_arena *scratch)
{
    string memstr = {};
    memstr.data = ARENA_PUSH_ARRAY(scratch, u8, str.size);
    memstr.size = str.size;
    for(u32 i = 0; i < str.size; ++i)
    {
        memstr.data[i] = str.data[i];
    }
    return(memstr);
}
*/

struct string
string_invert(struct string str, struct mem_arena *mem)
{
    struct string inv = create_mem_string(str.size, mem);
    for(u32 i = 0; i < str.size; ++i)
    {
        inv.data[i]  = str.data[str.size - i - 1];
    }
    return(inv);
}


struct string
string_append(struct string str1, struct string str2, struct mem_arena *scratch)
{
    struct string appended = {};
    appended.size = str1.size+str2.size;
    appended.data = ARENA_PUSH_ARRAY(scratch, u8, appended.size);
    for(u32 i = 0; i < str1.size; ++i)
    {
        appended.data[i] = str1.data[i];
    }
    for(u32 i = 0; i < str2.size; ++i)
    {
        appended.data[str1.size+i] = str2.data[i];
    }
    return(appended);
}

//TODO: implement this using a string builder of sorts
//NOTE: internally used to convert numbers to strings
struct string
internal_u64_to_growable_string_inverted(u64 value, struct mem_arena *mem)
{
    struct string str = create_mem_string(512, mem);
    str.size = 0; // set to 0 for "string builder"

    if(value == 0)
    {
        str.size = 1;
        str.data[0] = '0';
    }
    else
    {
        while(value != 0)
        {
            u8 digit = value % 10;
            value = value / 10;
            str.data[str.size++] = digit + '0';
        }
    }
    return(str);
}

struct string
s64_to_string(s64 value, struct mem_arena *mem)
{
    ASSERT(string_local_temp_mem.start != 0);
    struct mem_arena temp_mem = create_scoped_arena(string_local_temp_mem);

    b8 negative = value < 0;
    if(negative)
    {
        value = -value;
    }

    struct string str = internal_u64_to_growable_string_inverted(value, &temp_mem);

    if(negative)
    {
        str.data[str.size++] = '-';
    }

    struct string inv = string_invert(str, mem);
    
    return(inv);
}

struct string 
s32_to_string(s32 value, struct mem_arena *mem)
{
    return(s64_to_string(value, mem));
}

struct string 
s16_to_string(s16 value, struct mem_arena *mem)
{
    return(s64_to_string(value, mem));
}

struct string 
s8_to_string(s8 value, struct mem_arena *mem)
{
    return(s64_to_string(value, mem));
}

struct string
u64_to_string(u64 value, struct mem_arena *mem)
{
    ASSERT(string_local_temp_mem.start != 0);
    struct mem_arena temp_mem = create_scoped_arena(string_local_temp_mem);
    struct string str = internal_u64_to_growable_string_inverted(value, &temp_mem);
    struct string inv = string_invert(str, mem);
    return(inv);
}

struct string 
u32_to_string(u32 value, struct mem_arena *mem)
{
    return(u64_to_string(value, mem));
}

struct string 
u16_to_string(u16 value, struct mem_arena *mem)
{
    return(u64_to_string(value, mem));
}

struct string 
u8_to_string(u8 value, struct mem_arena *mem)
{
    return(u64_to_string(value, mem));
}

struct string 
f64_to_string(f64 value, u32 precision, struct mem_arena *mem)
{
    ASSERT(string_local_temp_mem.start != 0);
    struct mem_arena temp_mem = create_scoped_arena(string_local_temp_mem);

    b8 negative = value < 0;
    if(negative)
    {
        value = -value;
    }

    value *= pow_u64(10, precision);

    struct string str = internal_u64_to_growable_string_inverted(value, &temp_mem);

    if(negative)
    {
        str.data[str.size++] = '-';
    }

    struct string inv = string_invert(str, mem);

    //insert decimal point
    if(value != 0)
    {
        if(precision >= inv.size) //NOTE: first digit should be 0
        {
            u32 offset = precision - inv.size + 2;
            for(u32 i = 0; i < inv.size; ++i)
            {
                inv.data[offset + i] = inv.data[0];
            }
            for(u32 i = 0; i < offset; ++i)
            {
                inv.data[i] = '0';
            }
            inv.data[1] = '.';
            inv.size += offset;
        }
        else
        {
            u32 digits_before_decimal = inv.size - precision;
            inv.size += 1;
            for(u32 i = inv.size-1; i > digits_before_decimal;--i)
            {
                inv.data[i] = inv.data[i-1];
            }
            inv.data[digits_before_decimal] = '.';
        }
    }
    else
    {
        inv.size += precision + 1;
        inv.data[1] = '.';
        for(u32 i = 2; i < inv.size; ++i)
        {
            inv.data[i] = '0';
        }
    }
    
    return(inv);
}


#endif 
