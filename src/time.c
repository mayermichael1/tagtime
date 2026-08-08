#include "include/time_types.h"

#include "include/math.h"

#define DAYSECONDS (24 * 60 * 60)
#define YEARDAYS  365
#define EPOCHYEAR  1970 

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
struct time_data 
data_from_file(struct string filename, struct mem_arena temp)
{
    struct time_data data;

    struct mem_arena mem = create_mem_arena(sizeof(struct time_data_header));
    struct time_data_header *header = ARENA_PUSH_STRUCT(&mem, struct time_data_header);
    read_file(filename, sizeof(struct time_data_header), (u8*)header);

    data.header = *header; 
    
    // actual data
    struct time_data_pointer pointer;

    // time_entries
    u64 file_offset = sizeof(struct time_data_header);
    u64 file_data_chunk = data.header.entry_count * sizeof(struct time_entry);
    mem = create_mem_arena(file_data_chunk + sizeof(struct time_entry));
    pointer.entry_capacity = data.header.entry_count + 1;
    struct time_entry *entries = ARENA_PUSH_ARRAY(&mem, struct time_entry, pointer.entry_capacity);
    read_file_from(filename, file_offset, file_data_chunk, (u8*)entries); 
    pointer.entries = entries;

    file_offset += file_data_chunk;
    // string lenghts 
    file_data_chunk = data.header.tag_count * sizeof(u32);
    u32 *tag_lengths = ARENA_PUSH_ARRAY(&temp, u32, data.header.tag_count);
    read_file_from(filename, file_offset, file_data_chunk, (u8*)tag_lengths);

    file_offset += file_data_chunk;

    // string data
    file_data_chunk = data.header.tag_strings_size;
    mem = create_mem_arena(file_data_chunk + MAX_NEW_TAG_LENGTH * MAX_NEW_TAGS);

    pointer.tag_data_store_capacity = data.header.tag_strings_size + MAX_NEW_TAG_LENGTH * MAX_NEW_TAGS;
    u8 *tag_data = ARENA_PUSH_ARRAY(&mem, u8, pointer.tag_data_store_capacity);
    read_file_from(filename, file_offset, file_data_chunk, (u8*)tag_data);
    pointer.tag_data_store = tag_data;

    pointer.tag_capacity = data.header.tag_count + MAX_NEW_TAGS;
    mem = create_mem_arena(sizeof(struct string) * (data.header.tag_count + MAX_NEW_TAGS));
    struct string *tags = ARENA_PUSH_ARRAY(&mem, struct string, pointer.tag_capacity);
    pointer.tags = tags;

    // link up strings with data
    u64 string_offset = 0;
    for(u32 i = 0; i < data.header.tag_count; ++i)
    {
        struct string *tag = &pointer.tags[i];
        tag->size = tag_lengths[i];
        tag->data = &tag_data[string_offset];
        string_offset += tag->size;
    }

    file_offset += file_data_chunk;

    // link data
    file_data_chunk = data.header.link_count * sizeof(struct tag_entry_link);
    mem = create_mem_arena(file_data_chunk + sizeof(struct tag_entry_link) * MAX_TAG_LINKS * MAX_NEW_TAGS);
    pointer.link_capacity = data.header.link_count + MAX_TAG_LINKS * MAX_NEW_TAGS;
    struct tag_entry_link *links = ARENA_PUSH_ARRAY(&mem, struct tag_entry_link, pointer.link_capacity);
    read_file_from(filename, file_offset, file_data_chunk, (u8*)links);
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
data_to_file(struct string filename, struct time_data data, struct mem_arena temp)
{
    write_file(filename, sizeof(data.header), (u8*)&data.header);
    append_file(filename, sizeof(struct time_entry) * data.header.entry_count, (u8*)data.data.entries);

    u32 *tag_lengths = ARENA_PUSH_ARRAY(&temp, u32, data.header.tag_count);
    for(u32 i = 0; i < data.header.tag_count; ++i)
    {
        tag_lengths[i] = data.data.tags[i].size;
    }
    append_file(filename, sizeof(u32) * data.header.tag_count, (u8*)tag_lengths);
    append_file(filename, sizeof(u8) * data.header.tag_strings_size, (u8*)data.data.tag_data_store);
    append_file(filename, sizeof(struct tag_entry_link) * data.header.link_count, (u8*)data.data.links);
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
insert_time_entry(struct time_data *data, struct time_entry entry)
{
    ASSERT(data->header.entry_count < data->data.entry_capacity);
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
 * 
 * @return  tag_id of newly created tag
 */
u64
insert_tag(struct time_data *data, struct string tagname)
{
    ASSERT(data->header.tag_count < data->data.tag_capacity);
    ASSERT(tagname.size < MAX_NEW_TAG_LENGTH);

    u64 tag_store_size = data->header.tag_strings_size;
    u8 *newtag_data_pointer = &data->data.tag_data_store[tag_store_size];
    for(u32 i=0; i<tagname.size; ++i)
    {
        newtag_data_pointer[i] = tagname.data[i];
    }

    struct string tag = {};
    tag.size = tagname.size;
    tag.data = newtag_data_pointer; 

    data->header.tag_strings_size+=tagname.size;

    data->data.tags[data->header.tag_count] = tag;
    data->header.tag_count++;
    return(data->header.tag_count);
}

/**
 * create a new time_entry with a duration
 *
 * @param   duration in minutes for this entry
 *
 * @return  time_entry structure containing the duration and the current timestamp
 */
struct time_entry
create_entry(u64 duration)
{
    struct time_entry entry;
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
get_tag_id(struct time_data data, struct string tag)
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
link_entry_to_tags(struct time_data *data, u64 entry_id, struct tag_array tags)
{
    ASSERT(data->header.link_count < data->data.link_capacity);
    u64 last = data->header.link_count;
    for(u32 i=0; i<tags.count; ++i)
    {
        if(tags.ids[i] != 0)
        {
            data->data.links[last+i] = (struct tag_entry_link){
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
contains_uncreated_tags(struct tag_array tags)
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
 * this functions determines all entry_ids that are linked to a given tag
 *
 * allocates a new array which reserves the memory to contain all entries 
 * but ultimately only "contains" (count) the linked entries
 */
struct u64_array
entries_linked_to_tag(struct time_data data, u64 tagid, struct mem_arena *memory)
{
    struct u64_array entries = {};
    //NOTE: entries will never be larger than all entries so reserve this amount of space
    entries.data = ARENA_PUSH_ARRAY(memory, u64, data.header.entry_count);

    for(u32 i=0; i<data.header.link_count; ++i)
    {
        struct tag_entry_link link = data.data.links[i];
        if(link.tag_id == tagid)
        {
            entries.data[entries.count] = link.entry_id;
            entries.count++;
        }
    }
    return(entries);
}


/**
 * returns a list of entry_ids that are linked to all given tags 
 *
 * allocates one array that could hold all created entries
 */
struct u64_array
get_entries_linked_to_tags(struct time_data data, struct tag_array tags, struct mem_arena *memory)
{
    struct u64_array indices = create_incrementing_array(memory, data.header.entry_count);

    for(u32 i=0; i<tags.count; ++i) 
    {
        struct mem_arena loop_local_mem = *memory;
        u64 tag_id = tags.ids[i];
        struct u64_array entries_for_tag = entries_linked_to_tag(data, tag_id, &loop_local_mem); 
        intersect_arrays(&indices, entries_for_tag);
    }
    return(indices);
}


/// three possible formats:
/// - if : is found expect 00:00
/// - if , or . is found expect hhhh,mmmm
/// - if no special character is found excpect minutes
u64 
string_to_minutes(struct string str)
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

        struct string hour_string = string_split_to(str, index-1);
        struct string minute_string = string_split_from(str, index+1);

        u32 hours = string_to_u64(hour_string); 
        u32 decimal = string_to_u64(minute_string);
        // calculate minutes from decimal places
        minutes = 60 * decimal / pow_u64(10, minute_string.size);
        minutes += hours * 60;
    }
    else if(colon_index != -1)
    {
        ASSERT(colon_index >= 1);
        ASSERT(colon_index < (str.size-1));

        struct string hour_string = string_split_to(str, colon_index-1);
        struct string minute_string = string_split_from(str, colon_index+1);

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
struct time_entry
get_entry_by_id(struct time_data data, u64 entry_id)
{
    struct time_entry entry = {};
    if(entry_id <= data.header.entry_count)
    {
        entry = data.data.entries[entry_id-1];
    }
    return(entry);
}

/**
 * take minutes and part them to days, hours and minutes
 */
struct duration_minutes
minute_to_time(u64 minutes)
{
    struct duration_minutes t;
    u64 rest = minutes;

    t.days = rest / (60 * 24);
    rest = rest % (60 * 24);

    t.hours = rest / 60;

    t.minutes = rest % 60;

    t.sum_minutes = minutes;
    return(t);
}

/**
 * takes an array of cli_argumnts (essentially string array) and create a
 */
//TODO: time_data does not need to be a pointer here
struct tag_array
tags_to_array(struct time_data *data, struct string_array tags, struct mem_arena *memory)
{
    struct tag_array arr = {};
    arr.ids = ARENA_PUSH_ARRAY(memory, u64, tags.count);
    arr.tags = tags.data;
    arr.count = tags.count;

    for(u32 i=0; i<tags.count; ++i)
    {
        arr.ids[i] = get_tag_id(*data, tags.data[i]);
    }
    return(arr);
}

b8
is_leap_year(u16 year)
{
    return(year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

u16
number_of_leap_years(u16 year)
{
    return((year / 4) - ((year / 100) - (year/ 400)));
}

enum weekday
day_of_week(u64 timestamp)
{
    enum weekday dayow = (timestamp / (DAYSECONDS) + 3) % 7;
    return(dayow);
}

u64 
timestamp_without_hours_minutes_seconds(u64 timestamp)
{
    return((timestamp / (DAYSECONDS)) * (DAYSECONDS));
}

struct bound_u64
week_bounds(u64 timestamp)
{
    struct bound_u64 weekbound = {};
    enum weekday dayow = day_of_week(timestamp);
    u64 ts_day_midnight = timestamp_without_hours_minutes_seconds(timestamp);
    weekbound.lower = ts_day_midnight - (DAYSECONDS) * dayow;
    weekbound.upper = ts_day_midnight + (DAYSECONDS) * (7 - dayow) - 1;
    return(weekbound);
}

struct bound_u64
week_bounds_offset(u64 timestamp, s32 week_offset)
{
    ASSERT(week_offset > 0 || timestamp > -(week_offset * 7 * DAYSECONDS));
    return(week_bounds(timestamp + week_offset * 7 * DAYSECONDS));
}

u8
days_in_month(u16 year, u8 month)
{
    u8 days = 0;
    switch(month)
    {
        case 1: 
        case 3: 
        case 5: 
        case 7: 
        case 8:
        case 10:
        case 12:
            days = 31;
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            days = 30;
            break;
        case 2:
            {
                days = 28;
                if(is_leap_year(year))
                {
                    days = 29;
                }
            }
            break;
    }
    return(days);
}

struct datetime
seconds_to_timestamp(u64 seconds_since_epoch)
{

    u64 days_since_epoch = seconds_since_epoch / DAYSECONDS;
    u64 seconds_current_day = seconds_since_epoch - ( days_since_epoch * DAYSECONDS);

    struct datetime stamp = {};
    
    // time of the day
    stamp.second = seconds_current_day % 60;
    u64 minutes_current_day = seconds_current_day / 60;
    stamp.minute = minutes_current_day % 60;
    stamp.hour = minutes_current_day / 60;

    // year month day
    stamp.year = EPOCHYEAR + days_since_epoch / YEARDAYS; // NOTE: this should work until leap days result in an addiontal year

    u32 days_current_year = days_since_epoch % YEARDAYS;
    u32 leap_days = number_of_leap_years(stamp.year) - number_of_leap_years(EPOCHYEAR);
    days_current_year -= leap_days;

    // determine month and day

    //MONTH_DAY_BORDER is the day at which the current month is over
    //MONTH_DAY_BORDER[0] is JANUARY after 31 days january is over and so on
    u32 MONTH_DAY_BORDER[12];
    MONTH_DAY_BORDER[0] = days_in_month(stamp.year,1);
    for(u32 i = 1; i < 12; ++i)
    {
        MONTH_DAY_BORDER[i] = MONTH_DAY_BORDER[i-1] + days_in_month(stamp.year,i+1);
    }

    for(u8 month = 12; stamp.day == 0 && month > 0; --month)
    {
        if(days_current_year > MONTH_DAY_BORDER[month-1])
        {
            stamp.month = month+1; 
            stamp.day = days_current_year - MONTH_DAY_BORDER[month-1] + 1;
        }
    }

    return(stamp);
}

u64
timestamp_to_seconds(struct datetime dt)
{
    u64 ts = 0;
    ts += dt.second;
    ts += dt.minute * 60;
    ts += dt.hour *  60 * 60;
    ts += (dt.day-1) * DAYSECONDS;
    for(u32 i = 1; i < dt.month; ++i)
    {
        ts += days_in_month(dt.year, i) * DAYSECONDS;
    }
    u32 years_as_days_since_epoch = ((dt.year) - EPOCHYEAR) * YEARDAYS;
    u32 leap_days_since_epoch = number_of_leap_years(dt.year) - number_of_leap_years(EPOCHYEAR);
    ts += years_as_days_since_epoch * DAYSECONDS;
    ts += leap_days_since_epoch * DAYSECONDS;
    return(ts);
}

struct bound_u64
month_bounds_offset(u64 timestamp, s16 offset)
{
    struct bound_u64 bounds = {};
    struct datetime dt = seconds_to_timestamp(timestamp); 
    dt.hour = 0;
    dt.minute = 0;
    dt.second = 0;

    s16 year_offset = offset / 12;
    s16 month_offset = offset % 12;

    dt.year += year_offset;
    dt.month += month_offset;
    if(dt.month == 13)
    {
        dt.year++;
        dt.month = 1;
    }
    else if(dt.month == 0)
    {
        dt.year--;
        dt.month = 12;
    }

    dt.day = 1;
    bounds.lower = timestamp_to_seconds(dt);

    dt.month++;
    if(dt.month == 13)
    {
        dt.year++;
        dt.month = 1;
    }
    bounds.upper = timestamp_to_seconds(dt) - 1;
    return(bounds);
}

struct bound_u64
month_bounds(u64 timestamp)
{
    return month_bounds_offset(timestamp, 0);
}


