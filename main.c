#include <stdio.h>

#include "include/general.h"
#include "include/platform.h"
#include "include/memory.h"
#include "include/math.h"
#include "include/string.h"
#include "include/string_memory.h"

#include "src/linux_platform.c"

#define MAX_NEW_TAG_LENGTH  60
#define MAX_TAG_LINKS       10

typedef struct 
{
    u64 timestamp;
    u64 minutes;
}
time_entry;

typedef struct
{
    u64 count;
    string *tags;
    u64 *ids;
}
tag_array;

typedef struct
{
    u64 count;
    u64 *data;
}
u64_array;

typedef struct
{
    u64 entry_id;
    u64 tag_id;
}
tag_entry_link;

typedef struct
{
    u64 entry_count;
    u64 tag_count;
    u64 tag_strings_size;
    u64 tag_entry_table_size;
}
time_data_header;

typedef struct
{
    time_entry *entries;
    string *tags;
    u8* tag_data_store;
    tag_entry_link *links;
}
time_data_pointer;

typedef struct
{
    u32 count;
    string *data;
}
cli_arguments;

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

    file_offset += file_data_chunk;

    // link data
    file_data_chunk = data.header.tag_entry_table_size * sizeof(tag_entry_link);
    mem = create_scratch_memory(file_data_chunk + sizeof(tag_entry_link) * MAX_TAG_LINKS);
    tag_entry_link *links = PUSH_SCRATCH_ARRAY(&mem, tag_entry_link, data.header.tag_entry_table_size + MAX_TAG_LINKS);
    read_file_from(filename, file_offset, file_data_chunk, (u8*)links, temp);
    pointer.links = links;

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
    append_file(filename, sizeof(tag_entry_link) * data.header.tag_entry_table_size, (u8*)data.data.links, temp);
}

u64
insert_time_entry(time_data *data, time_entry entry)
{
    data->data.entries[data->header.entry_count++] = entry;
    return(data->header.entry_count);
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

/**
 * returns the id for the given tag if exists
 * 0 instead
 */
u64
get_tag_id(time_data *data, string tag)
{
    s64 id = 0;
    for(u32 i=0; id == 0 && i<data->header.tag_count; ++i)
    {
        if(string_compare(tag, data->data.tags[i]) == 0)
        {
            id = i+1; 
        }
    }
    return(id);
}

void
link_entry_to_tags(time_data *data, u64 entry_id, tag_array tags)
{
    u64 last = data->header.tag_entry_table_size;
    for(u32 i=0; i<tags.count; ++i)
    {
        if(tags.ids[i] != 0)
        {
            data->data.links[last+i] = (tag_entry_link){
                .entry_id = entry_id,
                .tag_id = tags.ids[i]
            };
        }
    }
    data->header.tag_entry_table_size += tags.count;
}

b8
contains_uncreated_tags(tag_array tags)
{
    b8 tag_not_existing = false;

    for(u32 i=0; !tag_not_existing && i<tags.count; ++i)
    {
        if(tags.ids[i] == 0)
        {
            tag_not_existing = true;
        }
    }

    return(tag_not_existing);
}

tag_array
tags_to_array(time_data *data, cli_arguments tags, scratch_memory *memory)
{
    tag_array arr = {};
    arr.ids = PUSH_SCRATCH_ARRAY(memory, u64, tags.count);
    arr.tags = tags.data;
    arr.count = tags.count;

    for(u32 i=0; i<tags.count; ++i)
    {
        arr.ids[i] = get_tag_id(data, tags.data[i]);
    }
    return(arr);
}

void
entries_minus_tag_linked(time_data data, u64_array *entries, u64 tag_id)
{
    for(u32 i=0; i<data.header.tag_entry_table_size; ++i)
    {
        if(tag_id == data.data.links[i].tag_id)
        {
           //TODO: continue here 
        }
    }
}

u64_array
get_entries_linked_to_tags(time_data data, tag_array tags, scratch_memory *memory)
{
    /*
    u64_array ids = {.count = data->header.entry_count};
    ids.data = PUSH_STRUCT_ARRAY(memory, u64, ids.count);
    for(u32 i=0; i<ids.count; ++i)
    {
        ids[i] = i+1;
    }

    for(u32 i=0; i<tags.count; ++i) 
    {
        
    }
    */
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

    //TODO: most of this is not actually used as a scratch temp memory but as general 
    //      allocator
    scratch_memory temp_mem = create_scratch_memory(10 * MB);

    cli_arguments args = {.count = argc - 1};
    args.data = PUSH_SCRATCH_ARRAY(&temp_mem, string, args.count);
    for(u32 i=0; i<argc-1; ++i)
    {
        args.data[i] = create_string(argv[i+1]);
    }

    if(args.count > 0) 
    {
        string command = args.data[0];

        string file = string_append(get_data_directory(&temp_mem), create_string("tagtime.data"), &temp_mem);
        time_data data = data_from_file(file, temp_mem);

        if(string_compare(command, create_string("newtag")) == 0)
        {
            // TODO: check if tag already exists
            if(args.count > 1)
            {
                string tag = args.data[1];
                ASSERT(tag.size < MAX_NEW_TAG_LENGTH);
                insert_tag(&data, tag);
            }
        }
        else if(string_compare(command, create_string("list")) == 0)
        {
            cli_arguments tag_args = {.count = args.count-1};
            tag_args.data = &args.data[1];

            if(tag_args.count == 0) //List all available tags
            {
                printf("List of available tags: \n");
                for(u32 i=0; i<data.header.tag_count; ++i)
                {
                    printf(" - %s\n", to_c_string(data.data.tags[i], &temp_mem));
                }
            }
            else // list all time entries connected to tags
            {
                tag_array tags = tags_to_array(&data, tag_args, &temp_mem);
                if(contains_uncreated_tags(tags))
                {
                    printf("Not all provided tags exist \n");
                }
                else
                {
                    printf("List should appear here\n");
                }
                //TODO: get_linked_entries_by_tags();
                //TODO: print all entries returned in the list
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
            string time_string = args.data[0];
            cli_arguments tag_args = {.count = args.count-1};
            tag_args.data = &args.data[1];

            tag_array tags = tags_to_array(&data, tag_args, &temp_mem);

            if(tags.count != 0)
            {
                if(!contains_uncreated_tags(tags))
                {
                    u64 duration = string_to_minutes(time_string);
                    u64 entry_id = insert_time_entry(&data, create_entry(duration));
                    link_entry_to_tags(&data, entry_id, tags);
                }
                else
                {
                    printf("Not all provided tags exist in the system \n");
                }
            }
            else
            {
                printf("Time needs to have at least one tag \n");
            }

        }
        data_to_file(file, data, temp_mem);
    }

    return(0);
}
