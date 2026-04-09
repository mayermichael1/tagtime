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
    u64 days;
    u64 hours;
    u64 minutes;

    u64 sum_minutes;
}
timestamp;

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
    u64 *ids; //NOTE: this is an containing the ids for the tags in *tags 
              //        0 means it does not exist
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
    u64 link_count;
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

/**
 * read data from data_file
 *
 * this will allocate memory to actually hold the data
 *
 * @param   filename filename to load the data from
 * @param   temp    memory arena used for temporary tasks  
 *          //TODO: remove these kind of temp mem usages
 *  
 * @return  time_data struct containing the read data
 */
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
    file_data_chunk = data.header.link_count * sizeof(tag_entry_link);
    mem = create_scratch_memory(file_data_chunk + sizeof(tag_entry_link) * MAX_TAG_LINKS);
    tag_entry_link *links = PUSH_SCRATCH_ARRAY(&mem, tag_entry_link, data.header.link_count + MAX_TAG_LINKS);
    read_file_from(filename, file_offset, file_data_chunk, (u8*)links, temp);
    pointer.links = links;

    data.data = pointer;
    return(data);
}

/**
 * write time_data to a file 
 *
 * @param   filename to be written to
 * @param   time_data struct containing the actual data
 * @param   temp_memory needed for file_writes
 *          //TODO: remove temp_memory usage
 */
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
    append_file(filename, sizeof(tag_entry_link) * data.header.link_count, (u8*)data.data.links, temp);
}

/**
 * insert a new time_entry to the data structure
 *
 * @param   data pointer
 * @param   entry to be inserted
 *
 * @return  index of the newly created entry (index in array + 1)
 */
u64
insert_time_entry(time_data *data, time_entry entry)
{
    data->data.entries[data->header.entry_count++] = entry;
    return(data->header.entry_count);
}

/**
 * insert a new tag to the data structure
 *
 * this will copy the contents of the given string into the tags store
 *
 * @param   data pointer
 * @param   tag name to be inserted as a new tag
 */
void
insert_tag(time_data *data, string tagname)
{
    u64 tag_store_size = data->header.tag_strings_size;
    u8 *newtag_data_pointer = &data->data.tag_data_store[tag_store_size];
    for(u32 i=0; i<tagname.size; ++i)
    {
        newtag_data_pointer[i] = tagname.data[i];
    }

    string tag = {};
    tag.size = tagname.size;
    tag.data = newtag_data_pointer; 

    data->header.tag_strings_size+=tagname.size;

    data->data.tags[data->header.tag_count] = tag;
    data->header.tag_count++;
}

/**
 * create a new time_entry with a duration
 *
 * @param   duration in minutes for this entry
 *
 * @return  time_entry structure containing the duration and the current timestamp
 */
time_entry
create_entry(u64 duration)
{
    time_entry entry;
    entry.minutes = duration;
    entry.timestamp = seconds_since_epoch();
    return(entry);
}

/**
 * returns the id for the given tag if exists 0 instead
 *
 * @param   data structure to get the id from
 * @param   tagname which will be searched for
 */
u64
get_tag_id(time_data data, string tag)
{
    s64 id = 0;
    for(u32 i=0; id == 0 && i<data.header.tag_count; ++i)
    {
        if(string_compare(tag, data.data.tags[i]) == 0)
        {
            id = i+1; 
        }
    }
    return(id);
}

/**
 * given an entry_id link up with given tags array
 *
 * tags_array contains an index array. If the index for one tag is 0 it is not 
 * contained in the data structure and therefore will not be linked.
 *
 *  //TODO: this expects the ids to already be set, this might be find as 
 *          tag_array contains this.
 *
 * @param   data structure
 * @param   entry_id entry to be linked
 * @param   tags array containing all arrays to be linked to
 */
void
link_entry_to_tags(time_data *data, u64 entry_id, tag_array tags)
{
    u64 last = data->header.link_count;
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
    data->header.link_count += tags.count;
}

/**
 * @param   tags array
 *
 * @return  returns false if at least one of the ids in the tags are 0 
 *          true otherwise
 */
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

/**
 * takes an array of cli_argumnts (essentially string array) and create a
 */
tag_array
tags_to_array(time_data *data, cli_arguments tags, scratch_memory *memory)
{
    tag_array arr = {};
    arr.ids = PUSH_SCRATCH_ARRAY(memory, u64, tags.count);
    arr.tags = tags.data;
    arr.count = tags.count;

    for(u32 i=0; i<tags.count; ++i)
    {
        arr.ids[i] = get_tag_id(*data, tags.data[i]);
    }
    return(arr);
}

/**
 * allocate a new array with incrementing numbers up to count
 */
u64_array
create_incrementing_array(scratch_memory *memory, u64 count)
{
    u64_array arr = {.count = count};
    arr.data = PUSH_SCRATCH_ARRAY(memory, u64, arr.count);
    for(u32 i=0; i<arr.count; ++i)
    {
        arr.data[i] = i+1;
    }
    return(arr);
}

/**
 * this functions determines all entry_ids that are linked to a given tag
 *
 * allocates a new array which reserves the memory to contain all entries 
 * but ultimately only "contains" (count) the linked entries
 */
u64_array
entries_linked_to_tag(time_data data, u64 tagid, scratch_memory *memory)
{
    u64_array entries = {};
    //NOTE: entries will never be larger than all entries so reserve this amount of space
    entries.data = PUSH_SCRATCH_ARRAY(memory, u64, data.header.entry_count);

    for(u32 i=0; i<data.header.link_count; ++i)
    {
        tag_entry_link link = data.data.links[i];
        if(link.tag_id == tagid)
        {
            entries.data[entries.count] = link.entry_id;
            entries.count++;
        }
    }
    return(entries);
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

/**
 * returns a list of entry_ids that are linked to all given tags 
 *
 * allocates one array that could hold all created entries
 */
u64_array
get_entries_linked_to_tags(time_data data, tag_array tags, scratch_memory *memory)
{
    u64_array indices = create_incrementing_array(memory, data.header.entry_count);

    for(u32 i=0; i<tags.count; ++i) 
    {
        scratch_memory loop_local_mem = *memory;
        u64 tag_id = tags.ids[i];
        u64_array entries_for_tag = entries_linked_to_tag(data, tag_id, &loop_local_mem); 
        intersect_arrays(&indices, entries_for_tag);
    }
    return(indices);
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

/**
 * return a time_entry by id 
 *
 * if id is not found returns an empty time_entry
 */
time_entry
get_entry_by_id(time_data data, u64 entry_id)
{
    time_entry entry = {};
    if(entry_id <= data.header.entry_count)
    {
        entry = data.data.entries[entry_id-1];
    }
    return(entry);
}

/**
 * take minutes and part them to days, hours and minutes
 */
timestamp
minute_to_time(u64 minutes)
{
    timestamp t;
    u64 rest = minutes;

    t.days = rest / (60 * 24);
    rest = rest % (60 * 24);

    t.hours = rest / 60;

    t.minutes = rest % 60;

    t.sum_minutes = minutes;
    return(t);
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
                    printf("Linekd entry ids:\n");
                    umm before = temp_mem.current; //TODO: for assertation may be removed
                    u64_array linked_entries = get_entries_linked_to_tags(data, tags, &temp_mem);
                    for(u32 i=0; i<linked_entries.count; ++i)
                    {
                        u64 entry_id = linked_entries.data[i];
                        time_entry entry = get_entry_by_id(data, entry_id);
                        printf("%d;%lu;%lu\n",entry_id, entry.timestamp, entry.minutes);
                    }
                    ASSERT(temp_mem.current == (before + data.header.entry_count * sizeof(u64)));
                }
            }
        }
        else if(string_compare(command, create_string("sum")) == 0)
        {
            //NOTE: this is essentially the same as list 
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
                    u64 sum_duration = 0;
                    u64_array linked_entries = get_entries_linked_to_tags(data, tags, &temp_mem);
                    for(u32 i=0; i<linked_entries.count; ++i)
                    {
                        u64 entry_id = linked_entries.data[i];
                        time_entry entry = get_entry_by_id(data, entry_id);
                        sum_duration += entry.minutes;
                    }
                    timestamp t = minute_to_time(sum_duration);
                    printf("Sums to %lu minutes total which is %lud %luh %lum\n", t.sum_minutes, t.days, t.hours, t.minutes);
                }
            }

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
