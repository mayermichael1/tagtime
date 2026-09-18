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
    struct mem_arena temp_mem = {};
    struct string inv = {};
    MEM_ARENA_SCOPE(string_local_temp_mem,temp_mem)
    {

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

        inv = string_invert(str, mem);
    }
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
    struct mem_arena temp_mem = {};
    struct string inv = {};
    MEM_ARENA_SCOPE(string_local_temp_mem,temp_mem)
    {
        struct string str = internal_u64_to_growable_string_inverted(value, &temp_mem);
        inv = string_invert(str, mem);
    }
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
    struct mem_arena temp_mem = {};
    MEM_ARENA_SCOPE(string_local_temp_mem,temp_mem)
    {

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
}

struct string
f32_to_string(f32 value, u32 precision, struct mem_arena *mem)
{
    return(f64_to_string(value, precision, mem));
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
    return(fs);
}

struct string
string_format(struct string format, struct mem_arena *mem, ...)
{
    va_list args;    
    va_start(args, mem);

    //TODO: bad: do not allocate memory on every function call
    struct mem_arena temp_mem = mem_arena_create(10*KB); 
    struct stringbuilder sb = stringbuilder_create(1024, &temp_mem);

    for(u32 i = 0; i < format.size; ++i)
    {
        if(format.data[i] == '%')
        {
            struct format_specifier fs = string_extract_format_specifier(format, &i);
            struct string to_insert = {};
            b8 negative = false;
            switch (fs.type)
            {
                case FS_INSERT_PERCENT:
                    to_insert = char_to_string('%', &temp_mem);
                    break;
                case FS_CHARACTER:
                    u8 character = (u8)va_arg(args, u32);
                    to_insert = char_to_string(character, &temp_mem);
                    break;
                case FS_STRING:
                    to_insert = va_arg(args, struct string);
                    break;
                case FS_INTEGER:
                    {
                        s32 value = va_arg(args, s32);
                        if(value<0)
                        {
                            value = -value;
                            negative = true;
                        }
                        to_insert = s32_to_string(value, &temp_mem); 
                    }
                    break;
                case FS_UNSIGNED_INTEGER:
                    {
                        u32 value = va_arg(args, u32);
                        to_insert = u32_to_string(value, &temp_mem); 
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
                        to_insert = f64_to_string(value, fs.precision, &temp_mem); 
                    }
                    break;
            }

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
            sb = stringbuilder_append(sb, string_split_to(to_insert, write_count));

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
        sb = stringbuilder_append_char(sb, format.data[i]);
    }

    va_end(args);

    //TODO: string memory is allocated "after" the temporary memory currently 
    //      therefore not the same memory pool can be used
    struct string string = stringbuilder_build(sb, mem);
    mem_arena_destroy(&temp_mem);
    return(string);
}


#endif 
