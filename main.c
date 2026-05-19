#include <stdio.h>

#include "include/general.h"
#include "include/platform.h"
#include "include/memory.h"
#include "include/math.h"
#include "include/string.h"
#include "include/string_memory.h"
#include "include/arrays.h"
#include "include/time_types.h"

#include "src/linux_platform.c"
#include "src/time.c"

typedef enum
{
    CMD_NONE,
    CMD_NEWTAG,
    CMD_NEWTIME,
    CMD_LIST,
    CMD_SUM,
}
command;

command
argument_to_command(string argument)
{
    command c = 0; 
    if(string_compare(argument, create_string("list")) == 0)
    {
        c = CMD_LIST;
    }
    else if(string_compare(argument, create_string("sum")) == 0)
    {
        c = CMD_SUM;
    }
    else if(string_compare(argument, create_string("newtag")) == 0)
    {
        c = CMD_NEWTAG;
    }
    else
    {
        c = CMD_NEWTIME;
    }
    return(c);
}

typedef struct
{
    u16 year;
    u8 month;
    u8 day;

    u8 hour;
    u8 minute;
    u8 second;

} 
datetime;

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

//TODO: maybe rename structure to timestamp again if current structure not to important
datetime
seconds_to_timestamp(u64 seconds_since_epoch)
{
    u64 DAYSECONDS = 24 * 60 * 60;
    u32 YEARDAYS = 365;
    u16 EPOCHYEAR = 1970; 

    u64 days_since_epoch = seconds_since_epoch / DAYSECONDS;
    u64 seconds_current_day = seconds_since_epoch - ( days_since_epoch * DAYSECONDS);

    datetime stamp = {};
    
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
    MONTH_DAY_BORDER[0] = 31; // JAN 
    MONTH_DAY_BORDER[1] = MONTH_DAY_BORDER[0] + 28 + ((is_leap_year(stamp.year)) ? 1 : 0); // FEB
    MONTH_DAY_BORDER[2] = MONTH_DAY_BORDER[1] + 31; // MAR
    MONTH_DAY_BORDER[3] = MONTH_DAY_BORDER[2] + 30; // APR
    MONTH_DAY_BORDER[4] = MONTH_DAY_BORDER[3] + 31; // MAY
    MONTH_DAY_BORDER[5] = MONTH_DAY_BORDER[4] + 30; // JUN
    MONTH_DAY_BORDER[6] = MONTH_DAY_BORDER[5] + 31; // JUL
    MONTH_DAY_BORDER[7] = MONTH_DAY_BORDER[6] + 31; // AUG
    MONTH_DAY_BORDER[8] = MONTH_DAY_BORDER[7] + 30; // SEP
    MONTH_DAY_BORDER[9] = MONTH_DAY_BORDER[8] + 31; // OCT
    MONTH_DAY_BORDER[10] = MONTH_DAY_BORDER[9] + 30; // NOV
    MONTH_DAY_BORDER[11] = MONTH_DAY_BORDER[10] + 31; // DEC

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

s32
main(u32 argc, u8 ** argv)
{
    datetime stamp = seconds_to_timestamp(seconds_since_epoch());
    printf("%d.%d.%d %d:%d:%d UTC\n", stamp.year, stamp.month, stamp.day, stamp.hour, stamp.minute, stamp.second);

    return(0);
}

/*
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

    set_platform_arena(create_mem_arena(KB));
    //TODO: most of this is not actually used as a scratch temp memory but as general 
    //      allocator
    mem_arena temp_mem = create_mem_arena(10 * MB);

    string_array args = {.count = argc - 1};
    args.data = ARENA_PUSH_ARRAY(&temp_mem, string, args.count);
    for(u32 i=0; i<argc-1; ++i)
    {
        args.data[i] = create_string(argv[i+1]);
    }

    string file = {};

    // check for alternative filename
    if(args.count > 1 && string_compare(args.data[0], create_string("-f")) == 0)
    {
        file = args.data[1];
        args.count = args.count - 2;
        args.data = &args.data[2];
    }
    else
    {
        file = string_append(get_data_directory(), create_string("tagtime.data"), &temp_mem);
    }


    if(args.count > 0) 
    {
        time_data data = data_from_file(file, temp_mem);

        command cmd = argument_to_command(args.data[0]);

        string_array tag_args = {.count = args.count-1};
        tag_args.data = &args.data[1];
        tag_array tags = tags_to_array(&data, tag_args, &temp_mem);

        switch(cmd)
        {
            case CMD_NEWTAG:
                if(args.count > 1)
                {
                    string tag = args.data[1];
                    ASSERT(tag.size < MAX_NEW_TAG_LENGTH);
                    insert_tag(&data, tag);
                }
            break;
            case CMD_LIST:
            case CMD_SUM:
                if(tag_args.count == 0) //List all available tags
                {
                    printf("List of available tags: \n");
                    for(u32 i=0; i<data.header.tag_count; ++i)
                    {
                        mem_arena temp = temp_mem;
                        printf(" - %s\n", to_c_string(data.data.tags[i], &temp));
                    }
                }
                else // list all time entries connected to tags
                {
                    if(contains_uncreated_tags(tags))
                    {
                        printf("Not all provided tags exist \n");
                    }
                    else
                    {
                        umm before = temp_mem.current; //TODO: for assertation may be removed
                        u64_array linked_entries = get_entries_linked_to_tags(data, tags, &temp_mem);

                        u64 sum_minutes = 0;
                        for(u32 i=0; i<linked_entries.count; ++i)
                        {
                            u64 entry_id = linked_entries.data[i];
                            time_entry entry = get_entry_by_id(data, entry_id);
                            sum_minutes += entry.minutes;
                            if(cmd == CMD_LIST)
                            {
                                printf("%d;%lu;%lu\n",entry_id, entry.timestamp, entry.minutes);
                            }
                        }
                        if(cmd == CMD_SUM)
                        {
                            timestamp time = minute_to_time(sum_minutes);
                            printf("Total of %lu minutes, which are %lud %luh %lum\n", time.sum_minutes, time.days, time.hours, time.minutes);
                        }
                        ASSERT(temp_mem.current == (before + data.header.entry_count * sizeof(u64)));
                    }
                }
            break;
            case CMD_NEWTIME:
                string time_string = args.data[0];

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
            break;
        }

        data_to_file(file, data, temp_mem);
    }

    return(0);
}
*/
