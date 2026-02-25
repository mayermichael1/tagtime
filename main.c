#include <stdio.h>

#include "include/general.h"
#include "include/platform.h"
#include "include/memory.h"
#include "include/math.h"

#include "src/linux_platform.c"

typedef struct 
{
    u64 minutes;
    u64 timestamp;
}
time_entry;

time_entry
create_entry(u64 duration)
{
    time_entry entry;
    entry.minutes = duration;
    entry.timestamp = seconds_since_epoch();
    return(entry);
}

s32 
main(void)
{
    scratch_memory mem = create_scratch_memory(500 * MB);
    printf("Hello World \n");

    // handle cli arguments here
    //
    // - track new time
    // - create new tag
    // - query tag(s)
    // - edit an existing entry
    //
        
    //NOTE: following implementation ignores tags for now and simply creates and
    //stores tags
    //
    time_entry *arr = PUSH_SCRATCH_ARRAY(&mem, time_entry, 10000);
    for(u32 i = 0; i < 10000; ++i)
    {
        arr[i] = create_entry(i);
    }

    return(0);
}
