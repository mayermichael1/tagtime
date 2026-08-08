#ifndef TIME_TYPES_H
#define TIME_TYPES_H

#define MAX_NEW_TAG_LENGTH  60
#define MAX_TAG_LINKS       10
#define MAX_NEW_TAGS        10

enum weekday
{
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    SUNDAY
};

struct datetime
{
    u16 year;
    u8 month;
    u8 day;

    u8 hour;
    u8 minute;
    u8 second;

};
struct duration_minutes
{
    u64 days;
    u64 hours;
    u64 minutes;

    u64 sum_minutes;
};

struct time_entry
{
    u64 timestamp;
    u64 minutes;
};

struct tag_array
{
    u64 count;
    struct string *tags;
    u64 *ids; //NOTE: this is an containing the ids for the tags in *tags 
              //        0 means it does not exist
};
struct tag_entry_link
{
    u64 entry_id;
    u64 tag_id;
};

struct time_data_header
{
    u64 entry_count;
    u64 tag_count;
    u64 tag_strings_size;
    u64 link_count;
};

struct time_data_pointer
{
    struct time_entry *entries;
    u64 entry_capacity;

    struct string *tags;
    u64 tag_capacity;

    u8* tag_data_store;
    u64 tag_data_store_capacity;

    struct tag_entry_link *links;
    u64 link_capacity;
};

struct time_data
{
    struct time_data_header header; 
    struct time_data_pointer data;
};

#endif // TIME_TYPES_H
