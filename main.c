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
    u64 timestamp;
    u64 minutes;
}
time_entry;

typedef struct
{
    u64 entry_count;
}
state_header;

typedef struct
{
    time_entry *entries;
}
state_data;

typedef struct
{
    state_header header; 
    state_data data;
}
state;

void 
state_set_data_pointers(state *store)
{
    store->data.entries = (time_entry*) store + sizeof(store->header);
    // next will be :
    // store->data.whatever = store->data.entries + store->header.entries * sizeof(time_entry);
}

u64 
state_size(state store)
{
    return(sizeof(state_header) + store.header.entry_count * sizeof(time_entry));
}

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

        scratch_memory temp_mem = create_scratch_memory(MB);
            
        string file = string_append(get_data_directory(&temp_mem), create_string("tagtime.data"), &temp_mem);
        u64 file_size = get_file_size(file, temp_mem);
        u64 buffer_size = file_size + KB; // safety buffer should most definitely 
                                           // be enough

        scratch_memory data_mem = create_scratch_memory(buffer_size);
        state *store = (state*)push_scratch_memory(&data_mem, buffer_size);
        read_file(file, file_size, (u8*)store, temp_mem);
        state_set_data_pointers(store);

        store->data.entries[store->header.entry_count++] = create_entry(duration);
        write_file(file, state_size(*store), (u8*)store, temp_mem);
        
    }
    else
    {
        printf("argument required!\n");
        //TODO: properly handle arguments before going to logic
    }


    return(0);
}
