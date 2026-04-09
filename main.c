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
    set_platform_arena(create_mem_arena(KB));

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
