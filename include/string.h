#ifndef STRING_H
#define STRING_H

typedef struct 
{
    u8 *data;
    u32 size;
}
string;

string
create_string(const char* value)
{
    string str = {};
    if(value != NULL)
    {
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
    }

    return(str);
}

/// finds first occurance of a certain character in a string
///
/// @param  string to be searched
/// @param  character to search for
///
/// @return returns either the index or -1 if the character can not be found
s64
string_find_u8(string str, u8 character)
{
    s64 index = -1;
    for(u32 i = 0; i < str.size && index == -1; ++i)
    {
        if(str.data[i] == character)
        {
            index = i;
        }
    }
    return(index);
}

s64 
string_find_last(string str, u8 character)
{    
    s64 index = -1;
    for(u32 i = str.size-1; i >= 0 && index == -1; --i)
    {
        if(str.data[i] == character)
        {
            index = i;
        }
    }
    return(index);
}

/// try to convert the string to a u64 value
///
/// for now this function completely ignores any other characters in the 
/// middle of the string.
/// e.g.: 
/// 1asdf3 will still be converted to 13
///
/// @param string to be converted
/// @return return the converted value or 0 if not successfull
u64
string_to_u64(string str)
{
    u64 value = 0;
    for(u32 i = 0; i < str.size; ++i)
    {
        u8 ch = str.data[i];
        if(ch >= '0' && ch <= '9')
        {
            u32 digit = ch - '0';
            value *= 10;
            value += digit;
        }
    }

    return(value);
}

string 
string_split(string str, u32 from, u32 to)
{
    string split = str;
    split.data = &str.data[from];
    split.size = to - from + 1;
    return(split);
}

string 
string_split_from(string str, u32 from)
{
    return(string_split(str, from, str.size));
}

string
string_split_to(string str, u32 to)
{
    return(string_split(str, 0, to));
}

#endif
