#ifndef STRING_MEMORY_H
#define STRING_MEMORY_H

#include "memory.h"
#include "math.h"
#include "stdarg.h"

global_variable struct mem_arena string_local_temp_mem = {};

/// ======================================================================== ///
/// STRING 
/// ======================================================================== ///

struct string
create_mem_string(u32 size, struct mem_arena *mem)
{
    struct string str = {};
    str.size = size;
    str.data = MEM_ARENA_PUSH_ARRAY(mem, u8, str.size);
    return(str);
}

const char *
to_c_string(struct string str, struct mem_arena *scratch)
{
    u8* cstring = MEM_ARENA_PUSH_ARRAY(scratch, u8, (str.size+1));
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
struct string 
string_copy(struct string str, struct mem_arena *scratch)
{
    struct string memstr = {};
    memstr.data = MEM_ARENA_PUSH_ARRAY(scratch, u8, str.size);
    memstr.size = str.size;
    for(u32 i = 0; i < str.size; ++i)
    {
        memstr.data[i] = str.data[i];
    }
    return(memstr);
}

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
    appended.data = MEM_ARENA_PUSH_ARRAY(scratch, u8, appended.size);
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


struct string
char_to_string(u8 value, struct mem_arena *mem)
{
    struct string charstr = {};
    charstr.size = 1;
    charstr.data = MEM_ARENA_PUSH_ARRAY(mem, u8, 1);
    charstr.data[0] = value;
    return(charstr);
}

/// ======================================================================== ///
/// STRING BUILDER
/// ======================================================================== ///

struct stringbuilder
stringbuilder_create(u32 capacity, struct mem_arena *mem)
{
    //TODO: for string_local_temp_mem a general purpose allocater with a free list 
    //would be nice, in that case the user would not need to provide its own arena
    //to a string builder_function
    struct stringbuilder sb = {};
    sb.string.data = MEM_ARENA_PUSH_ARRAY(mem, u8, capacity);
    sb.capacity = capacity;
    return(sb);
}

struct stringbuilder
stringbuilder_append(struct stringbuilder sb, struct string string)
{
    ASSERT((sb.string.size + string.size) <= sb.capacity);
    for(u32 i = 0; i < string.size; ++i)
    {
        sb.string.data[sb.string.size+i] = string.data[i];
    }
    sb.string.size += string.size;
    return(sb);
}

struct stringbuilder
stringbuilder_append_char(struct stringbuilder sb, u8 character)
{
    ASSERT((sb.string.size + 1) <= sb.capacity);
    sb.string.data[sb.string.size++] = character;
    return(sb);
}

struct string
stringbuilder_build(struct stringbuilder sb, struct mem_arena *mem)
{
    struct string string = string_copy(sb.string, mem);
    return(string);
}

/// 
/// STRING FORMAT
///

internal struct stringbuilder
stringbuilder_u64_append_inverted(struct stringbuilder sb, u64 value)
{
    if(value == 0)
    { 
        sb = stringbuilder_append_char(sb, '0');
    }
    else
    {
        while(value != 0)
        {
            u8 digit = value % 10;
            value = value / 10;
            sb = stringbuilder_append_char(sb, digit + '0');
        }
    }
    return(sb);
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

struct stringbuilder
stringbuilder_invert_in_range(struct stringbuilder sb, u32 from_index, u32 range)
{
    ASSERT(sb.string.size >= (from_index + range));
    u32 to_position = from_index + (range - 1);

    for(u32 i = 0; i < range / 2; ++i)
    {
        u8 swap = sb.string.data[from_index + i];
        sb.string.data[from_index + i] = sb.string.data[to_position - i];
        sb.string.data[to_position - i] = swap; 
    }
    return(sb);
}

/// number to string actual implementations for maximum size possible

struct stringbuilder
stringbuilder_append_s64(struct stringbuilder sb, s64 value)
{
    b8 negative = value < 0;
    if(negative)
    {
        value = -value;
    }

    u32 size_before = sb.string.size;
    sb = stringbuilder_u64_append_inverted(sb, value);

    if(negative)
    {
        sb = stringbuilder_append_char(sb, '-');
    }

    sb = stringbuilder_invert_in_range(sb, size_before, (sb.string.size - size_before));

    return(sb);
}

struct string
s64_to_string(s64 value, struct mem_arena *mem)
{
    struct mem_arena temp_mem = {};
    struct string inv = {};
    MEM_ARENA_SCOPE(&string_local_temp_mem,&temp_mem)
    {
        struct stringbuilder sb = stringbuilder_create(100, &temp_mem);
        sb = stringbuilder_append_s64(sb, value);
        inv = stringbuilder_build(sb, mem);
    }
    return(inv);
}

struct stringbuilder
stringbuilder_append_u64(struct stringbuilder sb, u64 value)
{
    u32 size_before = sb.string.size;
    sb = stringbuilder_u64_append_inverted(sb, value);
    sb = stringbuilder_invert_in_range(sb, size_before, (sb.string.size - size_before));
    return(sb);
}

struct string
u64_to_string(u64 value, struct mem_arena *mem)
{
    struct mem_arena temp_mem = {};
    struct string inv = {};
    MEM_ARENA_SCOPE(&string_local_temp_mem,&temp_mem)
    {
        struct stringbuilder sb = stringbuilder_create(100, &temp_mem);
        sb = stringbuilder_append_u64(sb, value);
        inv = stringbuilder_build(sb, mem);
    }
    return(inv);
}

struct stringbuilder
stringbuilder_append_f64(struct stringbuilder sb, f64 value, u32 precision)
{
    struct mem_arena temp_mem = {};
    MEM_ARENA_SCOPE(&string_local_temp_mem,&temp_mem)
    {
        b8 negative = value < 0;
        if(negative)
        {
            value = -value;
        }

        value *= pow_u64(10, precision);

        u32 size_before = sb.string.size;
        sb = stringbuilder_u64_append_inverted(sb, value);

        if(negative)
        {
            sb = stringbuilder_append_char(sb, '-');
        }

        sb = stringbuilder_invert_in_range(sb, size_before, (sb.string.size - size_before));

        //insert decimal point
        //NOTE: assume the decimal point has to have space
        if(value != 0)
        {
            if(precision >= sb.string.size) //NOTE: first digit should be 0
            {
                u32 offset = precision - sb.string.size + 2;
                for(u32 i = 0; i < sb.string.size; ++i)
                {
                    sb.string.data[offset + i] = sb.string.data[0];
                }
                for(u32 i = 0; i < offset; ++i)
                {
                    sb.string.data[i] = '0';
                }
                sb.string.data[1] = '.';
                sb.string.size += offset;
            }
            else
            {
                u32 digits_before_decimal = sb.string.size - precision;
                sb.string.size += 1;
                for(u32 i = sb.string.size-1; i > digits_before_decimal;--i)
                {
                    sb.string.data[i] = sb.string.data[i-1];
                }
                sb.string.data[digits_before_decimal] = '.';
            }
        }
        else
        {
            sb.string.size += precision + 1;
            sb.string.data[1] = '.';
            for(u32 i = 2; i < sb.string.size; ++i)
            {
                sb.string.data[i] = '0';
            }
        }
    }
    return(sb);

}

struct string 
f64_to_string(f64 value, u32 precision, struct mem_arena *mem)
{
    struct mem_arena temp_mem = {};
    struct string inv = {};
    MEM_ARENA_SCOPE(&string_local_temp_mem,&temp_mem)
    {
        struct stringbuilder sb = stringbuilder_create(100, &temp_mem);
        sb = stringbuilder_append_f64(sb, value, precision);
        inv = stringbuilder_build(sb, mem);
    }
    return(inv);
}


/// number to string smaller sizes
///
struct stringbuilder
stringbuilder_append_u32(struct stringbuilder sb, u32 value)
{
    return(stringbuilder_append_u64(sb, value));
}

struct string 
u32_to_string(u32 value, struct mem_arena *mem)
{
    return(u64_to_string(value, mem));
}

struct stringbuilder
stringbuilder_append_u16(struct stringbuilder sb, u16 value)
{
    return(stringbuilder_append_u64(sb, value));
}

struct string 
u16_to_string(u16 value, struct mem_arena *mem)
{
    return(u64_to_string(value, mem));
}

struct stringbuilder
stringbuilder_append_u8(struct stringbuilder sb, u8 value)
{
    return(stringbuilder_append_u64(sb, value));
}

struct string 
u8_to_string(u8 value, struct mem_arena *mem)
{
    return(u64_to_string(value, mem));
}

struct stringbuilder
stringbuilder_append_s32(struct stringbuilder sb, s32 value)
{
    return(stringbuilder_append_s64(sb, value));
}

struct string 
s32_to_string(s32 value, struct mem_arena *mem)
{
    return(s64_to_string(value, mem));
}

struct stringbuilder
stringbuilder_append_s16(struct stringbuilder sb, s16 value)
{
    return(stringbuilder_append_s64(sb, value));
}

struct string 
s16_to_string(s16 value, struct mem_arena *mem)
{
    return(s64_to_string(value, mem));
}

struct stringbuilder
stringbuilder_append_s8(struct stringbuilder sb, s8 value)
{
    return(stringbuilder_append_s64(sb, value));
}

struct string 
s8_to_string(s8 value, struct mem_arena *mem)
{
    return(s64_to_string(value, mem));
}

struct stringbuilder
stringbuilder_append_f32(struct stringbuilder sb, f32 value, u32 precision)
{
    return(stringbuilder_append_f64(sb, value, precision));
}

struct string
f32_to_string(f32 value, u32 precision, struct mem_arena *mem)
{
    return(f64_to_string(value, precision, mem));
}

enum fs_align_flag
{
    FS_LEFT_JUSTIFY = 0b0001,
    FS_SIGN         = 0b0010,
    FS_ZERO_LEAD    = 0b0100,
    FS_ALL          = 0b1111,
    FS_NONE         = 0b0000,
};

enum fs_format_type
{
    FS_INSERT_PERCENT,
    FS_CHARACTER,
    FS_STRING,
    FS_INTEGER,
    FS_UNSIGNED_INTEGER,
    FS_FLOAT,
    FS_COUNT,
};
struct format_specifier
{
    u32 width;
    u32 precision;
    enum fs_align_flag align;
    enum fs_format_type type;
};

struct format_specifier
string_extract_format_specifier(struct string format, u32 *index)
{
    struct format_specifier fs = {}; 
    // get alignment flags etc
    b8 alignment_flag_found = true;

    // skip percent sign
    (*index)++;

    // extract all align specifiers
    for(;format.size > *index & alignment_flag_found;)
    {
        alignment_flag_found = false;
        switch (format.data[*index])
        {
            case '+':
                fs.align |= FS_SIGN;
                alignment_flag_found = true;
            break;
            case '-':
                fs.align |= FS_LEFT_JUSTIFY;
                alignment_flag_found = true;
            break;
            case '0':
                fs.align |= FS_ZERO_LEAD;
                alignment_flag_found = true;
            break;
        }

        if(alignment_flag_found)
        {
            (*index)++;
        }
    }

    // extract width string
    struct string widthstr = {};
    widthstr.data = &format.data[*index];
    for(;format.size > *index && in_bound_u64_inclusive(format.data[*index], (struct bound_u64){'0', '9'});(*index)++)
    {
        widthstr.size++;
    }
    fs.width = string_to_u64(widthstr);

    // extract precision
    if(format.data[*index] == '.')
    {
        (*index)++;
        struct string precision_str = {};
        precision_str.data = &format.data[*index];
        for(;format.size > *index && in_bound_u64_inclusive(format.data[*index], (struct bound_u64){'0', '9'});(*index)++)
        {
            precision_str.size++;
        }
        fs.precision = string_to_u64(precision_str);
    }

    // get type of argument
    if(format.size > *index)
    {
        switch(format.data[*index])
        {
            case '%':
                fs.type = FS_INSERT_PERCENT; 
            break;
            case 'c':
                fs.type = FS_CHARACTER; 
                fs.precision = 0;
                fs.align &= ~FS_SIGN;
                fs.align &= ~FS_ZERO_LEAD;
            break;
            case 's':
                fs.type = FS_STRING;
                fs.align &= ~FS_SIGN;
                fs.align &= ~FS_ZERO_LEAD;
            break;
            case 'f': case 'F':
                fs.type = FS_FLOAT;
                if(fs.precision == 0)
                {
                    fs.precision = 6;
                }
            break;
            case 'i': case 'd':
                fs.type = FS_INTEGER;
            break;
            case 'u':
                fs.type = FS_INTEGER;
            break;
        }
        (*index)++;
    }

    (*index)--; // go back once so the for loop can be advanced normally on the outside

    return(fs);
}

struct string
string_format_valist(struct string format, struct mem_arena *mem, va_list args) 
{
    struct string string = {};

    //TODO: bad: do not allocate memory on every function call
    struct mem_arena temp_mem = {}; 
    MEM_ARENA_SCOPE(&string_local_temp_mem,&temp_mem)
    {
        struct stringbuilder sb = stringbuilder_create(1024, &temp_mem);

        for(u32 i = 0; i < format.size; ++i)
        {
            if(format.data[i] == '%')
            {
                struct mem_arena if_local_mem = {};
                MEM_ARENA_SCOPE(&temp_mem,&if_local_mem)
                {
                    struct format_specifier fs = string_extract_format_specifier(format, &i);
                    struct stringbuilder to_insert_sb = stringbuilder_create(100, &if_local_mem);
                    b8 negative = false;
                    switch (fs.type)
                    {
                        case FS_INSERT_PERCENT:
                            to_insert_sb = stringbuilder_append_char(to_insert_sb, '%');
                            break;
                        case FS_CHARACTER:
                            u8 character = (u8)va_arg(args, u32);
                            to_insert_sb = stringbuilder_append_char(to_insert_sb, character);
                            break;
                        case FS_STRING:
                            to_insert_sb = stringbuilder_append(to_insert_sb, va_arg(args, struct string));
                            break;
                        case FS_INTEGER:
                            {
                                s32 value = va_arg(args, s32);
                                if(value<0)
                                {
                                    value = -value;
                                    negative = true;
                                }
                                to_insert_sb = stringbuilder_append_s32(to_insert_sb, value);
                            }
                            break;
                        case FS_UNSIGNED_INTEGER:
                            {
                                u32 value = va_arg(args, u32);
                                to_insert_sb = stringbuilder_append_u32(to_insert_sb, value);
                            }
                            break;
                        case FS_FLOAT:
                            {
                                f64 value = va_arg(args, f64);
                                if(value<0)
                                {
                                    value = -value;
                                    negative = true;
                                }
                                to_insert_sb = stringbuilder_append_f64(to_insert_sb, value, fs.precision);
                            }
                            break;
                    }

                    struct string to_insert = stringbuilder_build(to_insert_sb, &if_local_mem);
                    // pad to width
                    //
                    if(MASK(fs.align, FS_ZERO_LEAD))
                    {
                        if(negative)
                        {
                            sb = stringbuilder_append_char(sb, '-');
                        }
                        else if(MASK(fs.align, FS_SIGN))
                        {
                            sb = stringbuilder_append_char(sb, '+');
                        }
                    }
                    if(!MASK(fs.align, FS_LEFT_JUSTIFY) && fs.width != 0)
                    {
                        u8 padding_char = ' ';
                        if(MASK(fs.align, FS_ZERO_LEAD))
                        {
                            padding_char = '0';
                        }

                        u32 pad_size = fs.width - to_insert.size;
                        if(to_insert.size > fs.width)
                        {
                            pad_size = 0;
                        }
                        if(fs.type == FS_STRING && fs.precision != 0)
                        {
                            pad_size = fs.width - fs.precision;
                        }
                        for(u32 i = 0; i < pad_size; ++i)
                        {
                            sb = stringbuilder_append_char(sb, padding_char);
                        }
                    }
                    if(!MASK(fs.align, FS_ZERO_LEAD))
                    {
                        if(negative)
                        {
                            sb = stringbuilder_append_char(sb, '-');
                        }
                        else if(MASK(fs.align, FS_SIGN))
                        {
                            sb = stringbuilder_append_char(sb, '-');
                        }
                    }

                    // print the actual string
                    u32 write_count = to_insert.size;
                    if(fs.type == FS_STRING && fs.precision != 0)
                    {
                        write_count = MIN(to_insert.size, fs.precision);
                    }
                    sb = stringbuilder_append(sb, string_split_to(to_insert, write_count - 1));

                    // pad to width left
                    if(MASK(fs.align, FS_LEFT_JUSTIFY) && fs.width != 0)
                    {
                        u32 pad_size = fs.width - to_insert.size;
                        if(to_insert.size > fs.width)
                        {
                            pad_size = 0;
                        }
                        if(fs.type == FS_STRING && fs.precision != 0)
                        {
                            pad_size = fs.width - fs.precision;
                        }
                        for(u32 i = 0; i < pad_size; ++i)
                        {
                            sb = stringbuilder_append_char(sb, ' ');
                        }
                    }
                }
            }
            else
            {
                sb = stringbuilder_append_char(sb, format.data[i]);
            }
        }

        string = stringbuilder_build(sb, mem);

    }

    return(string);
}

struct string
string_format(struct string format, struct mem_arena *mem, ...)
{
    va_list args;
    va_start(args, mem);
    struct string str = string_format_valist(format, mem, args);
    va_end(args);
    return(str);
}


#endif 
