#include <stdio.h>

#include "include/general.h"
#include "include/platform.h"
#include "include/memory.h"
#include "include/math.h"
#include "include/string.h"
#include "include/string_memory.h"

#include "src/linux_platform.c"

#define MAX_NEW_TAG_LENGTH 60

typedef struct 
{
    u64 timestamp;
    u64 minutes;
}
time_entry;

typedef struct
{
    u64 entry_count;
    u64 tag_count;
    u64 tag_strings_size;
}
time_data_header;

typedef struct
{
    time_entry *entries;
    string *tags;
    u8* tag_data_store;
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
    read_file(filename, sizeof(time_data_header), (u8*)header, temp);

    data.header = *header; 
    
    // actual data
    time_data_pointer pointer;

    // time_entries
    u64 file_offset = sizeof(time_data_header);
    u64 file_data_chunk = data.header.entry_count * sizeof(time_entry);
    mem = create_scratch_memory(file_data_chunk + sizeof(time_entry));
    time_entry *entries = PUSH_SCRATCH_ARRAY(&mem, time_entry, data.header.entry_count + 1);
    read_file_from(filename, file_offset, file_data_chunk, (u8*)entries ,temp); 
    pointer.entries = entries;

    file_offset += file_data_chunk;
    // string lenghts 
    file_data_chunk = data.header.tag_count * sizeof(u32);
    u32 *tag_lengths = PUSH_SCRATCH_ARRAY(&temp, u32, data.header.tag_count);
    read_file_from(filename, file_offset, file_data_chunk, (u8*)tag_lengths, temp);

    file_offset += file_data_chunk;

    // string data
    file_data_chunk = data.header.tag_strings_size;
    mem = create_scratch_memory(file_data_chunk + MAX_NEW_TAG_LENGTH);
    u8 *tag_data = PUSH_SCRATCH_ARRAY(&mem, u8, data.header.tag_strings_size + MAX_NEW_TAG_LENGTH);
    read_file_from(filename, file_offset, file_data_chunk, (u8*)tag_data, temp);
    pointer.tag_data_store = tag_data;

    mem = create_scratch_memory(sizeof(string) * (data.header.tag_count + 1));
    string *tags = PUSH_SCRATCH_ARRAY(&mem, string, data.header.tag_count + 1);
    pointer.tags = tags;

    // link up strings with data
    u64 string_offset = 0;
    for(u32 i = 0; i < data.header.tag_count; ++i)
    {
        string *tag = &pointer.tags[i];
        tag->size = tag_lengths[i];
        tag->data = &tag_data[string_offset];
        string_offset += tag->size;
    }

    data.data = pointer;
    return(data);
}

void 
data_to_file(string filename, time_data data, scratch_memory temp)
{
    write_file(filename, sizeof(data.header), (u8*)&data.header, temp);
    append_file(filename, sizeof(time_entry) * data.header.entry_count, (u8*)data.data.entries, temp);

    u32 *tag_lengths = PUSH_SCRATCH_ARRAY(&temp, u32, data.header.tag_count);
    for(u32 i = 0; i < data.header.tag_count; ++i)
    {
        tag_lengths[i] = data.data.tags[i].size;
    }
    append_file(filename, sizeof(u32) * data.header.tag_count, (u8*)tag_lengths, temp);
    append_file(filename, sizeof(u8) * data.header.tag_strings_size, (u8*)data.data.tag_data_store, temp);
}

void
insert_time_entry(time_data *data, time_entry entry)
{
    data->data.entries[data->header.entry_count++] = entry;
}

void
insert_tag(time_data *data, string tag)
{
    u64 tag_store_size = data->header.tag_strings_size;
    for(u32 i=0; i<tag.size; ++i)
    {
        data->data.tag_data_store[tag_store_size+i] = tag.data[i];
    }
    tag.data = &data->data.tag_data_store[tag_store_size];
    data->header.tag_strings_size+=tag.size;

    data->data.tags[data->header.tag_count++] = tag;
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
            if(argc >= 2)
            {
                string tag = create_string(argv[2]);
                ASSERT(tag.size < MAX_NEW_TAG_LENGTH);
                insert_tag(&data, tag);
            }
        }
        else if(string_compare(command, create_string("list")) == 0)
        {
            printf("List of tracked times: \n");
            for(u32 i = 0; i < data.header.entry_count; ++i)
            {
                printf("entry at %u with a duration of %u\n", data.data.entries[i].timestamp, data.data.entries[i].minutes);
            }

            printf("List of created tags: \n");
            for(u32 i=0; i<data.header.tag_count; ++i)
            {
                printf(" - %s\n", to_c_string(data.data.tags[i], &temp_mem));
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
            string time_string = create_string(argv[1]);
            u64 duration = string_to_minutes(time_string);
            insert_time_entry(&data, create_entry(duration));
        }
        data_to_file(file, data, temp_mem);
    }

    return(0);
}
