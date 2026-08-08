#ifndef STRING_MEMORY_H
#define STRING_MEMORY_H

#include "memory.h"

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


#endif 
