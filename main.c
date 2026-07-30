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
    cli_arguments args = cli_parse(argc, argv, create_string("t.lsaf:c:"));
    //TODO: create a help page

    set_platform_arena(create_mem_arena(KB));
    //TODO: most of this is not actually used as a scratch temp memory but as general 
    //      allocator
    mem_arena temp_mem = create_mem_arena(10 * MB);

    string file = {};

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
        mem_arena temp = create_scoped_arena(temp_mem); // TODO: create function to "create" block scoped arena
        tag_array tags = tags_to_array(&data, cli_get_args(args, 't', &temp), &temp); 
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
    else if(cli_contains(args, 's') || cli_contains(args, 'l'))
    {
        if(!cli_contains(args, 't'))
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
            mem_arena temp = create_scoped_arena(temp_mem); // TODO: create function to "create" block scoped arena
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
                for(u32 i=0; i<linked_entries.count; ++i)
                {
                    u64 entry_id = linked_entries.data[i];
                    time_entry entry = get_entry_by_id(data, entry_id);
                    sum_minutes += entry.minutes;
                    if(cli_contains(args, 't'))
                    {
                        datetime dt = seconds_to_timestamp(entry.timestamp);
                        printf("%d;%04d.%02d.%02d %02d:%02d:%02d;%lu\n",entry_id, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, entry.minutes);
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
    }
    else if(cli_contains(args, 'a'))
    {
        // TODO: add more than one tag at once 
        if(cli_option_count(args, 't') >= 1)
        {
            string tag = cli_get_arg(args, 't', 0);
            ASSERT(tag.size < MAX_NEW_TAG_LENGTH);
            insert_tag(&data, tag);
        }
    }

    data_to_file(file, data, temp_mem);

    return(0);
}
