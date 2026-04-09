#ifndef ARRAYS_H
#define ARRAYS_H

#include "memory.h"

typedef struct
{
    u64 count;
    u64 *data;
}
u64_array;

typedef struct
{
    u32 count;
    string *data;
}
string_array;

/**
 * allocate a new array with incrementing numbers up to count
 */
u64_array
create_incrementing_array(mem_arena *memory, u64 count)
{
    u64_array arr = {.count = count};
    arr.data = ARENA_PUSH_ARRAY(memory, u64, arr.count);
    for(u32 i=0; i<arr.count; ++i)
    {
        arr.data[i] = i+1;
    }
    return(arr);
}

/**
 * "remove" a given index from an array be shifting the rest of the array down
 */
void
arr_remove_idx(u64_array *arr, u64 idx)
{
    u64_array newarr = *arr;
    if(idx < newarr.count)
    {
        for(u64 i = idx; i<newarr.count-1; ++i)
        {
            newarr.data[i] = newarr.data[i+1];
        }
        newarr.count--;
    }
    *arr = newarr;
}

/**
 * remove all elements matching a value from the array
 */
void
arr_remove_values(u64_array *arr, u64 element)
{
    u64_array collapsed = *arr;
    u64 i = 0;
    while(i < collapsed.count)
    {
        if(collapsed.data[i] == element)
        {
            arr_remove_idx(&collapsed, i);
        }
        else
        {
            ++i;
        }
    }
    *arr = collapsed;
}

/***
 * this will intersect array a and b with only common entries
 *
 * all 0 entries will be ignored as these are not valid entry_ids
 *
 * @param   pointer to first array, data will actually be changed
 * @param   second array, stays the sae
 */
//TODO: currently sets values to 0 which is not needed. change this so it can be 
//used generally
void
intersect_arrays(u64_array *a, u64_array b)
{
    u64_array intersect = *a;
    for(u64 i=0; i<intersect.count; ++i)
    {
        b8 element_found = false;

        for(u64 y=0; y<b.count; ++y)
        {
            if(intersect.data[i] == b.data[y])
            {
                element_found = true;
            }
        }

        if(!element_found)
        {
            intersect.data[i] = 0;
        }
    }

    arr_remove_values(&intersect, 0);

    *a = intersect;
}

#endif
