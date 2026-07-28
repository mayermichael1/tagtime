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

//TODO: new argument handling structure
//      handle these arguments like getopt 
//      but write the code manually so it will eventually work on windows as well
//      implement the resulting data structure into a map and provide it as part 
//      of the platform layer as well
//
//
//

#define CLI_ARGS_CAPACITY 32

typedef enum
{
    CLI_ARGUMENT_FLAG = 0,
    CLI_ARGUMENT_ONE,
    CLI_ARGUMENT_ONE_TO_MANY
}
cli_argument_type;

typedef struct
{
    u8      option;
    u8**    argv_pointer;
    u32     count;
    cli_argument_type type;
}
cli_argument;

typedef struct
{
    string program_name;
    cli_argument args[CLI_ARGS_CAPACITY];
    u32 errors;
}
cli_arguments;

u8
_cli_args_hash(u8 option)
{
    return option / 10;
}

void
_cli_args_insert(cli_arguments *args, cli_argument arg)
{
    u8 hash = _cli_args_hash(arg.option);
    b8 vacant = true;
    for(u32 i = hash; vacant && i < CLI_ARGS_CAPACITY; i++)
    {
        if(args->args[i].option == 0)
        {
            args->args[i] = arg;
            vacant = false;
        }
    }
    ASSERT(!vacant);
}

s16
_cli_arguments_find_position(cli_arguments arguments, u8 option)
{
    u8 hash = _cli_args_hash(option);
    s16 index = -1;
    for(u8 i = hash; index == -1 && i < CLI_ARGS_CAPACITY; ++i)
    {
        if(arguments.args[i].option == option)
        {
            index = i;
        }
    }
    return(index);
}

/**
 * parses through program arguments and creates a easily handable structure
 *
 * This function is loosely based on the getopt function defined in the posix 
 * standard.
 * the options string is used to determine which characters are which type of 
 * option.
 * all characters except (. and :) in the options string represent an option.
 * - . is reserved to specify the option can have 1 to many arguments
 * - : is reserved to specify the option has exactly 1 argument
 *
 * example: option string "ab:c."
 * program invocation: program -a -b arg1 -c arg2 arg3 arg4
 *
 * NOTE: if an option expects an argument but is not provided one, an error 
 * count is increased. 
 * The argument itself contains a supposed type of the argument.
 * With these mechanisms the error might be found 
 *
 * NOTE: this API does not allow option repeats. 
 *
 * @param   argc passed directly from main
 * @param   argv passed directly from main
 * @param   options option string as specified above
 *
 * @return arguments structure containing parsed arguments
 */
cli_arguments
cli_parse(u32 argc, u8** argv, string options)
{
    cli_arguments cli_args = {};
    cli_args.program_name = create_string(argv[0]);

    for(u32 i = 1; i < argc; ++i)
    {
        string arg = create_string(argv[i]);
        if(arg.data[0] == '-' && arg.data[1] != 0)
        { 
            u8 find_option = arg.data[1];
            s64 pos = string_find_u8(options, find_option);
            if(pos != -1)
            {
                cli_argument argument = {};
                argument.option = find_option;
                
                u8 modifier = options.data[pos+1];
                if(modifier == '.') // 1 or more arguments
                {
                    ++i;
                    argument.type = CLI_ARGUMENT_ONE_TO_MANY;

                    if(i < argc)
                    {
                        argument.argv_pointer = &argv[i];

                        for(b8 next_option_found = false; !next_option_found && i < argc; ++i)
                        {
                            if(argv[i][0] == '-')
                            {
                                next_option_found = true;
                            }
                            else
                            {
                                argument.count++;
                            }
                        }
                    }

                    if(argument.count == 0)
                    {
                        cli_args.errors++;
                    }
                }
                else if(modifier == ':') // exactly one argument
                {
                    ++i;
                    argument.type = CLI_ARGUMENT_ONE;

                    if(i < argc)
                    {
                        argument.count = 1;
                        argument.argv_pointer = &argv[i];
                    }

                    if(argument.count == 0)
                    {
                        cli_args.errors++;
                    }
                }
                _cli_args_insert(&cli_args, argument);
            }
        }
    }

    return(cli_args);
}

/**
 *  does the arguments structure contain the provided option
 *
 *  @param  arguments cli_arguments structure created by cli_parse
 *  @param  option to check if contained
 *
 *  @return true if options exists
 */
b8
cli_contains(cli_arguments arguments, u8 option)
{
    return(_cli_arguments_find_position(arguments, option) != -1);
}

/**
 * retrieves the argument count for a specified option
 *
 *  @param  arguments cli_arguments structure created by cli_parse
 *  @param  option to check
 *
 *  @return count of arguments
 */
u32
cli_option_count(cli_arguments arguments, u8 option)
{
    s16 index = _cli_arguments_find_position(arguments, option);
    u32 count = 0;
    if(index != -1)
    {
        count = arguments.args[index].count;
    }
    return(count);
}

/**
 *  retrieves the n-th argument of an option
 *
 *  @param  arguments cli_arguments structure created by cli_parse
 *  @param  option to check
 *  @param  index of the argument to retrieve
 *
 *  @return string containing the argument 
 */
string
cli_get_arg(cli_arguments arguments, u8 option, u32 index)
{    
    s16 i = _cli_arguments_find_position(arguments, option);
    string arg = create_string("");
    if(i != -1)
    {
        arg = create_string(arguments.args[i].argv_pointer[index]);
    } return(arg);
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
    //

    //TODO: usage code example for new argument handling:
    /*
    cli_arguments args = cli_argument_parse(argc, argv, create_string("t.lsan"));

    if(cli_contains(args, 's')) // sum 
    if(cli_contains(args, 't')) // at least 1 argument exists for t
    {
        for(u32 i = 0; i < cli_option_count(args, 't'); ++i)
        {
            string arg = cli_get_arg(args,'t', i);
        }
    }
    */

    cli_arguments args = cli_parse(argc, argv, create_string("ab:c."));

    if(cli_contains(args, 'a'))
    {
        printf("args contains a\n");
    }

    if(cli_contains(args, 'b'))
    {
        for(u32 i = 0; i < cli_option_count(args, 'b'); ++i)
        {
            printf("option b with argument %s\n", cli_get_arg(args, 'b', i).data); 
        }
    }

    if(cli_contains(args, 'c'))
    {
        for(u32 i = 0; i < cli_option_count(args, 'c'); ++i)
        {
            printf("option c with argument %s\n", cli_get_arg(args, 'b', i).data); 
        }
    }

    printf("%d errors\n", args.errors);

    /*
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
                                datetime dt = seconds_to_timestamp(entry.timestamp);
                                printf("%d;%04d.%02d.%02d %02d:%02d:%02d;%lu\n",entry_id, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, entry.minutes);
                            }
                        }
                        if(cmd == CMD_SUM)
                        {
                            duration_minutes time = minute_to_time(sum_minutes);
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
    */

    return(0);
}
