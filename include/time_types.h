#ifndef TIME_TYPES_H
#define TIME_TYPES_H

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
    time_data_header header; 
    time_data_pointer data;
}
time_data;

#endif // TIME_TYPES_H
