#include <stdio.h>

#include "include/general.h"
#include "include/platform.h"
#include "include/memory.h"
#include "include/math.h"
#include "include/string.h"
#include "include/string_memory.h"

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

/// three possible formats:
/// - if : is found expect 00:00
/// - if , or . is found expect hhhh,mmmm
/// - if no special character is found excpect minutes
u64 
string_to_minutes(string str)
{
    s64 colon_index = string_find_u8(str,':');
    s64 komma_index = string_find_u8(str,',');
    s64 point_index = string_find_u8(str,'.');

    u64 minutes = 0;

    // need functions to split strings

    if(komma_index != -1 || point_index != -1)
    {
        u32 index = MAX(komma_index, point_index)
        ASSERT(index >= 1);
        ASSERT(index < (str.size-1));

        string hour_string = string_split_to(str, index-1);
        string minute_string = string_split_from(str, index+1);

        u32 hours = string_to_u64(hour_string); 
        u32 decimal = string_to_u64(minute_string);
        // calculate minutes from decimal places
        minutes = 60 * decimal / pow_u64(10, (minute_string.size + 1));
        minutes += hours * 60;
    }
    else if(colon_index != -1)
    {
        ASSERT(colon_index >= 1);
        ASSERT(colon_index < (str.size-1));

        string hour_string = string_split_to(str, colon_index-1);
        string minute_string = string_split_from(str, colon_index+1);

        u32 hours = string_to_u64(hour_string); 
        minutes = string_to_u64(minute_string);
        ASSERT(minutes < 60);
        minutes += hours * 60;
    }
    else
    {
        minutes = string_to_u64(str);
    }

    return(minutes);
}

s32 
main(u32 argc, u8** argv)
{

    // cli strutcture
    // tagtime <time> <tag>* -- record an entry
    // tagtime newtag <name> -- create new tag with name <name>
    // tagtime list <tag>* -- list all entries connected to all given tags
    // tagtime sum <tag>* -- sum all entries connected to all given tags
    // tagtime delete entryid|tagname  delete either a tag or entry
    // tagtime addtag <tag> <entry> add tag to an entry
    // tagtiem deltag <tag> <entry> remove tag from an entry
    //
    // global options:
    //
    // -f filename for the backing store. If this is not given a 
    // location is determined automatically
    //
    //
    // handle cli arguments here
    //
    // - track new time
    // - create new tag
    // - query tag(s)
    // - edit an existing entry
    
    //NOTE: following implementation ignores tags for now and simply creates and
    //stores tags
    //
    
    u64 duration = 0;
    if(argc >= 2)
    {
        duration = string_to_minutes(create_string(argv[1]));
    }
    else
    {
        printf("argument required!\n");
        //TODO: properly handle arguments before going to logic
    }

    time_entry entry = create_entry(duration);
    printf("Entry created at %lu with a duration of %lu minutes\n", entry.timestamp, entry.minutes);

    scratch_memory mem = create_scratch_memory(MB);
    printf("data dir %s\n", get_data_directory(&mem).data);

    return(0);
}
