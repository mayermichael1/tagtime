#ifndef STRING_H
#define STRING_H

#include "memory.h"

typedef struct 
{
    u8 *data;
    u32 size;
}
string;

string
create_string(const char* value)
{
    string str;
    str.data = (u8*)value;

    u32 len = 0;
    u8 c = 0;
    do
    {
        c = value[len];
        len++;
    }
    while(c != 0);
    str.size = len-1; 

    return(str);
}

const char *
to_c_string(string str, scratch_memory *scratch)
{
    u8* cstring = PUSH_SCRATCH_ARRAY(scratch, u8, (str.size+1));
    for(u32 i = 0; i < str.size; ++i)
    {
        cstring[i] = str.data[i];
    }
    cstring[str.size] = 0;
    return((const char*)cstring);
}

#endif
