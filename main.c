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
time_data_header;

typedef struct
{
    time_entry *entries;
}
time_data_pointer;

typedef struct
{
    time_data_header header; 
    time_data_pointer data;
}
time_data;

time_data 
data_from_file(string filename, scratch_memory temp)
{
    time_data data;

    scratch_memory mem = create_scratch_memory(sizeof(time_data_header));
    time_data_header *header = PUSH_SCRATCH_STRUCT(&mem, time_data_header);
    read_file(filename, sizeof(header), (u8*)header, temp);

    data.header = *header; 
    
    // actual data
    //
    u64 offset = sizeof(header);
    u64 chunk_size = data.header.entry_count * sizeof(time_entry);

    time_data_pointer pointer;
    mem = create_scratch_memory(chunk_size + sizeof(time_entry));
    time_entry *entries = PUSH_SCRATCH_ARRAY(&mem, time_entry, data.header.entry_count + 1);
    read_file_from(filename, sizeof(header), chunk_size, (u8*)entries ,temp); 
    offset+=chunk_size;
    pointer.entries = entries;

    data.data = pointer;

    return(data);
}

void 
data_to_file(string filename, time_data data, scratch_memory temp)
{
    write_file(filename, sizeof(data.header), (u8*)&data.header, temp);
    append_file(filename, sizeof(time_entry) * data.header.entry_count, (u8*)data.data.entries, temp);
}

void
insert_time_entry(time_data *data, time_entry entry)
{
    data->data.entries[data->header.entry_count++] = entry;
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

    if(argc >= 1) 
    {
        string command = create_string(argv[1]);

        scratch_memory temp_mem = create_scratch_memory(MB);
        string file = string_append(get_data_directory(&temp_mem), create_string("tagtime.data"), &temp_mem);
        time_data data = data_from_file(file, temp_mem);

        if(string_compare(command, create_string("newtag")) == 0)
        {
            printf("newtag\n");
        }
        else if(string_compare(command, create_string("list")) == 0)
        {
            printf("List of tracked times:!\n");
            for(u32 i = 0; i < data.header.entry_count; ++i)
            {
                printf("entry at %u with a duration of %u\n", data.data.entries[i].timestamp, data.data.entries[i].minutes);
            }
        }
        else if(string_compare(command, create_string("sum")) == 0)
        {
            printf("sum\n");
        }
        else if(string_compare(command, create_string("delete")) == 0)
        {
            printf("delete\n");
        }
        else if(string_compare(command, create_string("addtag")) == 0)
        {
            printf("addtag\n");
        }
        else if(string_compare(command, create_string("deltag")) == 0)
        {
            printf("deltag\n");
        }
        else
        {
            u64 duration = string_to_minutes(create_string(argv[1]));
            insert_time_entry(&data, create_entry(duration));
            data_to_file(file, data, temp_mem);
        }
    }

    return(0);
}
