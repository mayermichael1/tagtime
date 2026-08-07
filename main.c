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
create_uncreated_tags_assistant(time_data *data, tag_array tags)
{
    b8 all_tags_created = true;
    if(contains_uncreated_tags(tags))
    {
        printf("Uncreated tags found.\n"); 
        for(u32 i = 0; all_tags_created && i < tags.count; i++)
        {
            if(tags.ids[i] == 0)
            {
                printf("Create tag \"%s\"? (y/n) : ", tags.tags[i].data);
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
        
    cli_arguments args = cli_parse(argc, argv, create_string("t.lsaf:c:hnw:"));

    set_platform_arena(create_mem_arena(KB));
    //TODO: most of this is not actually used as a scratch temp memory but as general 
    //      allocator
    mem_arena temp_mem = create_mem_arena(10 * MB);

    string file = {};

    if(cli_contains(args, 'h'))
    {
        printf("tagtime usage:\n");
        printf(" -h ... show this help page\n");
        printf(" -c time ... create new entry (requires tag(s))\n");
        printf("\t time formats: \n");
        printf("\t -c HH:mm\n");
        printf("\t -c minutes\n");
        printf("\t -c H,Hfract \n");
        printf("\t -c H.Hfract \n");
        printf(" -a add new tags to the system\n");
        printf(" -l list all times tracked to specified tag(s)\n");
        printf(" -s sum all times tracked to specified tag(s)\n");
        printf(" -t tag [tag2] [tag3] ... list of tags to be operated upon\n");
        printf(" -n when no tags are given show all entries\n");
    }
    else
    {
        if(cli_contains(args,'f'))
        {
            file = cli_get_arg(args, 'f', 0);
        }
        else
        {
            file = string_append(get_data_directory(), create_string("tagtime.data"), &temp_mem);
        }

        time_data data = data_from_file(file, temp_mem);
        
        if(cli_contains(args, 'c'))
        {
            string time_string =  cli_get_arg(args, 'c', 0);
            mem_arena temp = create_scoped_arena(temp_mem); 
            tag_array tags = tags_to_array(&data, cli_get_args(args, 't', &temp), &temp); 
            if(tags.count != 0)
            {
                if(create_uncreated_tags_assistant(&data, tags))
                {
                    u64 duration = string_to_minutes(time_string);
                    u64 entry_id = insert_time_entry(&data, create_entry(duration));
                    link_entry_to_tags(&data, entry_id, tags);
                }
                else
                {
                    printf("not all tags have been created. entry was not inserted.\n");
                }
            }
            else
            {
                printf("Time needs to have at least one tag \n");
            }
        }
        else if(cli_contains(args, 's') || cli_contains(args, 'l'))
        {
            if(!cli_contains(args, 't') && !cli_contains(args, 'n'))
            {
                printf("List of available tags: \n");
                for(u32 i=0; i<data.header.tag_count; ++i)
                {
                    mem_arena temp = create_scoped_arena(temp_mem);
                    printf(" - %s\n", to_c_string(data.data.tags[i], &temp));
                }
            }
            else if(cli_option_count(args, 't') != 0)
            {
                mem_arena temp = create_scoped_arena(temp_mem); 
                tag_array tags = tags_to_array(&data, cli_get_args(args, 't', &temp), &temp); 

                //TODO: dynamically create tags if they do not exist -a should be pointless then
                if(contains_uncreated_tags(tags))
                { 
                    printf("Not all provided tags exist \n");
                }
                else
                {
                    umm before = temp.current; //TODO: for assertation may be removed
                    u64_array linked_entries = get_entries_linked_to_tags(data, tags, &temp);

                    u64 sum_minutes = 0;

                    struct bound_u64 filter_timestamp = {.lower = U64_MIN, .upper = U64_MAX};

                    if(cli_contains(args, 'w'))
                    {
                        s64 week_offset = string_to_s64(cli_get_arg(args, 'w', 0));
                        filter_timestamp = week_bounds_offset(seconds_since_epoch(), week_offset);
                    }

                    for(u32 i=0; i<linked_entries.count; ++i)
                    {
                        u64 entry_id = linked_entries.data[i];
                        time_entry entry = get_entry_by_id(data, entry_id);
                        if(in_bound_u64_inclusive(entry.timestamp, filter_timestamp))
                        {
                            sum_minutes += entry.minutes;
                            if(cli_contains(args, 'l'))
                            {
                                datetime dt = seconds_to_timestamp(entry.timestamp);
                                printf("%d;%04d.%02d.%02d %02d:%02d:%02d;%lu\n",entry_id, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, entry.minutes);
                            }
                        }
                    }
                    if(cli_contains(args, 's'))
                    {
                        duration_minutes time = minute_to_time(sum_minutes);
                        printf("Total of %lu minutes, which are %lud %luh %lum\n", time.sum_minutes, time.days, time.hours, time.minutes);
                    }
                    ASSERT(temp.current == (before + data.header.entry_count * sizeof(u64)));
                }
            }
            else if(cli_contains(args, 'n'))
            {
                struct bound_u64 filter_timestamp = {.lower = U64_MIN, .upper = U64_MAX};

                if(cli_contains(args, 'w'))
                {
                    s64 week_offset = string_to_s64(cli_get_arg(args, 'w', 0));
                    filter_timestamp = week_bounds_offset(seconds_since_epoch(), week_offset);
                }

                u64 sum_minutes = 0;
                for(u32 i=1; i<=data.header.entry_count; ++i)
                {
                    time_entry entry = get_entry_by_id(data, i);
                    if(in_bound_u64_inclusive(entry.timestamp, filter_timestamp))
                    {
                        sum_minutes += entry.minutes;
                        if(cli_contains(args, 'l'))
                        {
                            datetime dt = seconds_to_timestamp(entry.timestamp);
                            printf("%d;%04d.%02d.%02d %02d:%02d:%02d;%lu\n",i, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, entry.minutes);
                        }
                    }
                }
                if(cli_contains(args, 's'))
                {
                    duration_minutes time = minute_to_time(sum_minutes);
                    printf("Total of %lu minutes, which are %lud %luh %lum\n", time.sum_minutes, time.days, time.hours, time.minutes);
                }
            }
        }
        else if(cli_contains(args, 'a'))
        {
            mem_arena temp = create_scoped_arena(temp_mem);
            tag_array tags = tags_to_array(&data, cli_get_args(args, 't', &temp), &temp); 
            create_uncreated_tags_assistant(&data, tags);
        }

        data_to_file(file, data, temp_mem);
    }
    return(0);
}
