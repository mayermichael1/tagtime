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

#include "stdio.h"

/**
 * receives an array of tags and creates arrays that are not in timedata yet
 *
 * @param time_data struct to insert tags into
 * @param tags array that are potentially inserted into the tags
 *
 * @return  true if either all tags have existed already or all uncreated tags
 *          have been created
 */
b8
create_uncreated_tags_assistant(struct time_data *data, struct tag_array tags, struct mem_arena *mem)
{
    b8 all_tags_created = true;
    if(contains_uncreated_tags(tags))
    {
        write_stdout_cstring("Uncreated tags found.\n"); 
        for(u32 i = 0; all_tags_created && i < tags.count; i++)
        {
            if(tags.ids[i] == 0)
            {
                write_stdout(string_format(create_string("Create tag \"%s\"? (y/n) : "),mem, tags.tags[i]));
                fflush(stdout);
                all_tags_created = read_u8_stdin() == 'y';
                if(all_tags_created)
                {
                    tags.ids[i] = insert_tag(data, tags.tags[i]);
                }
            }
        }
    }
    return(all_tags_created);
}

s32 
main(u32 argc, u8** argv)
{
    // CLI structure
    // t ... list of tags
    // l ... list
    // s ... sum
    // a ... add new tag(s)
    // f ... optional filename 
    // c ... timestamp for new time tracking followed by a time
    // n ... for list and sum show all entries
    // w ... filter entries for given week
    // m ... filter entries for given month
    //
    //
    //
    //

    struct cli_arguments args = cli_parse(argc, argv, create_string("t.lsaf:c:hnw:m:"));

    platform_local_temp_mem = mem_arena_create(KB);
    string_local_temp_mem = mem_arena_create(10 * KB);
    struct mem_arena scratch = mem_arena_create(10 * MB);

    struct string file = {};

    FOR_DEFER_BLOCK(
        write_stdout_cstring("before"),
        write_stdout_cstring("after")
    )
    {
        write_stdout_cstring("middle");
    }
    write_stdout_cstring("\n");

    if(cli_contains(args, 'h'))
    {
        write_stdout_cstring("tagtime usage:\n");
        write_stdout_cstring(" -h ... show this help page\n");
        write_stdout_cstring(" -c time ... create new entry (requires tag(s))\n");
        write_stdout_cstring("\t time formats: \n");
        write_stdout_cstring("\t -c HH:mm\n");
        write_stdout_cstring("\t -c minutes\n");
        write_stdout_cstring("\t -c H,Hfract \n");
        write_stdout_cstring("\t -c H.Hfract \n");
        write_stdout_cstring(" -a add new tags to the system\n");
        write_stdout_cstring(" -l list all times tracked to specified tag(s)\n");
        write_stdout_cstring(" -s sum all times tracked to specified tag(s)\n");
        write_stdout_cstring(" -t tag [tag2] [tag3] ... list of tags to be operated upon\n");
        write_stdout_cstring(" -n when no tags are given show all entries\n");
        write_stdout_cstring(" -w [offset] filter entries for given week (e.g.: -1 last week, 0 current week)\n");
        write_stdout_cstring(" -m [offset] filter entries for given month (e.g.: -1 last month, 0 current month)\n");
    }
    else
    {
        if(cli_contains(args,'f'))
        {
            file = cli_get_arg(args, 'f', 0);
        }
        else
        {
            file = string_append(get_data_directory(&scratch), create_string("tagtime.data"), &scratch);
        }

        struct time_data data = data_from_file(file, scratch);
        
        if(cli_contains(args, 'c'))
        {
            struct string time_string =  cli_get_arg(args, 'c', 0);
            struct mem_arena temp = mem_arena_create_scoped(scratch); 
            struct tag_array tags = tags_to_array(data, cli_get_args(args, 't', &temp), &temp); 
            if(tags.count != 0)
            {
                if(create_uncreated_tags_assistant(&data, tags, &temp))
                {
                    u64 duration = string_to_minutes(time_string);
                    u64 entry_id = insert_time_entry(&data, create_entry(duration));
                    link_entry_to_tags(&data, entry_id, tags);
                }
                else
                {
                    write_stdout_cstring("not all tags have been created. entry was not inserted.\n");
                }
            }
            else
            {
                write_stdout_cstring("Time needs to have at least one tag \n");
            }
        }
        else if(cli_contains(args, 's') || cli_contains(args, 'l'))
        {
            if(!cli_contains(args, 't') && !cli_contains(args, 'n'))
            {
                write_stdout_cstring("List of available tags: \n");
                for(u32 i=0; i<data.header.tag_count; ++i)
                {
                    struct mem_arena temp = mem_arena_create_scoped(scratch);
                    write_stdout(string_format(create_string(" - %s\n"),&temp, data.data.tags[i]));
                }
            }
            else if(cli_option_count(args, 't') != 0)
            {
                struct mem_arena temp = mem_arena_create_scoped(scratch); 
                struct tag_array tags = tags_to_array(data, cli_get_args(args, 't', &temp), &temp); 

                if(contains_uncreated_tags(tags))
                { 
                    write_stdout_cstring("Not all provided tags exist \n");
                }
                else
                {
                    //TODO: basically the same happens in the -n options 
                    //      maybe pull out this code in some way 
                    struct u64_array linked_entries = get_entries_linked_to_tags(data, tags, &temp);

                    u64 sum_minutes = 0;

                    struct bound_u64 filter_timestamp = {.lower = U64_MIN, .upper = U64_MAX};

                    if(cli_contains(args, 'm'))
                    {
                        s64 offset = string_to_s64(cli_get_arg(args, 'm', 0));
                        filter_timestamp = month_bounds_offset(seconds_since_epoch(), offset);
                    }

                    if(cli_contains(args, 'w'))
                    {
                        s64 week_offset = string_to_s64(cli_get_arg(args, 'w', 0));
                        filter_timestamp = week_bounds_offset(seconds_since_epoch(), week_offset);
                    }


                    for(u32 i=0; i<linked_entries.count; ++i)
                    {
                        u64 entry_id = linked_entries.data[i];
                        struct time_entry entry = get_entry_by_id(data, entry_id);
                        if(in_bound_u64_inclusive(entry.timestamp, filter_timestamp))
                        {
                            sum_minutes += entry.minutes;
                            if(cli_contains(args, 'l'))
                            {
                                struct datetime dt = seconds_to_timestamp(entry.timestamp);
                                write_stdout(string_format(create_string("%d;%04d.%02d.%02d %02d:%02d:%02d;%u\n"),&temp, entry_id, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, entry.minutes));
                            }
                        }
                    }
                    if(cli_contains(args, 's'))
                    {
                        struct duration_minutes time = minute_to_time(sum_minutes);
                        write_stdout(string_format(create_string("Total of %u minutes, which are %ud %uh %um\n"),&temp, time.sum_minutes, time.days, time.hours, time.minutes));
                    }
                }
            }
            else if(cli_contains(args, 'n'))
            {
                struct bound_u64 filter_timestamp = {.lower = U64_MIN, .upper = U64_MAX};

                if(cli_contains(args, 'm'))
                {
                    s64 offset = string_to_s64(cli_get_arg(args, 'm', 0));
                    filter_timestamp = month_bounds_offset(seconds_since_epoch(), offset);
                }

                if(cli_contains(args, 'w'))
                {
                    s64 week_offset = string_to_s64(cli_get_arg(args, 'w', 0));
                    filter_timestamp = week_bounds_offset(seconds_since_epoch(), week_offset);
                }

                u64 sum_minutes = 0;
                for(u32 i=1; i<=data.header.entry_count; ++i)
                {
                    struct time_entry entry = get_entry_by_id(data, i);
                    if(in_bound_u64_inclusive(entry.timestamp, filter_timestamp))
                    {
                        sum_minutes += entry.minutes;
                        if(cli_contains(args, 'l'))
                        {
                            struct datetime dt = seconds_to_timestamp(entry.timestamp);
                            struct mem_arena temp = mem_arena_create_scoped(scratch);
                            write_stdout(string_format(create_string("%d;%04d.%02d.%02d %02d:%02d:%02d;%u\n"),&temp, i, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, entry.minutes));
                        }
                    }
                }
                if(cli_contains(args, 's'))
                {
                    struct duration_minutes time = minute_to_time(sum_minutes);
                    struct mem_arena temp = mem_arena_create_scoped(scratch);
                    write_stdout(string_format(create_string("Total of %u minutes, which are %ud %uh %um\n"),&temp, time.sum_minutes, time.days, time.hours, time.minutes));
                }
            }
        }
        else if(cli_contains(args, 'a'))
        {
            struct mem_arena temp = mem_arena_create_scoped(scratch);
            struct tag_array tags = tags_to_array(data, cli_get_args(args, 't', &temp), &temp); 
            create_uncreated_tags_assistant(&data, tags, &temp);
        }

        data_to_file(file, data, scratch);
    }
    return(0);
}
