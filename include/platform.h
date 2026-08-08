#ifndef PLATFORM_H
#define PLATFORM_H

#include "general.h"
#include "string.h"

struct mem_arena;
struct string_array;

///==========================================================================///
///                             GENERAL                                      ///
///==========================================================================///

/**
 * this sets the memory arena the platform code uses for various tasks
 *
 * this memory arena has to be set or the platform layer will crash
 * currently most functions need this memory for temporaray allocations while 
 * performing the task, only data directory actually allocates "permanent" 
 * memory
 *
 * //TODO: this should later assure that the memory arena is growable. 
 *          in this case the memory could be created in the platform layer and 
 *          will never run out. 
 *          platform layer will never need much memory anyways
 */
void
set_platform_arena(struct mem_arena arena);

u64
get_file_size(struct string filename);

void
read_file(struct string filename, u64 len, u8 *buffer);

void
read_file_from(struct string filename, u64 from, u64 len, u8 *buffer);

void
write_file(struct string filename, u64 buffer_size, u8 *buffer);

void
append_file(struct string filename, u64 buffer_size, u8 *buffer);

umm
allocate(umm size);

void
deallocate(umm start_address, umm size);

u64
seconds_since_epoch();

struct string
get_data_directory();

u8 
read_u8_stdin();

///==========================================================================///
///                             CLI ARGUMENTS HADNLING                       ///
///==========================================================================///

#define CLI_ARGS_CAPACITY 32

enum cli_argument_type
{
    CLI_ARGUMENT_FLAG = 0,
    CLI_ARGUMENT_ONE,
    CLI_ARGUMENT_ONE_TO_MANY
};

struct cli_argument
{
    u8      option;
    u8**    argv_pointer;
    u32     count;
    enum cli_argument_type type;
};

struct cli_arguments
{
    struct string program_name;
    struct cli_argument args[CLI_ARGS_CAPACITY];
    u32 errors;
};

u8
_cli_args_hash(u8 option)
{
    return option / 10;
}

void
_cli_args_insert(struct cli_arguments *args, struct cli_argument arg)
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
_cli_arguments_find_position(struct cli_arguments arguments, u8 option)
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
struct cli_arguments
cli_parse(u32 argc, u8** argv, struct string options)
{
    struct cli_arguments cli_args = {};
    cli_args.program_name = create_string(argv[0]);

    for(u32 i = 1; i < argc; ++i)
    {
        struct string arg = create_string(argv[i]);
        if(arg.data[0] == '-' && arg.data[1] != 0)
        { 
            u8 find_option = arg.data[1];
            s64 pos = string_find_u8(options, find_option);
            if(pos != -1)
            {
                struct cli_argument argument = {};
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
cli_contains(struct cli_arguments arguments, u8 option)
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
cli_option_count(struct cli_arguments arguments, u8 option)
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
struct string
cli_get_arg(struct cli_arguments arguments, u8 option, u32 index)
{    
    s16 i = _cli_arguments_find_position(arguments, option);
    struct string arg = create_string("");
    if(i != -1 && index < cli_option_count(arguments, option))
    {
        arg = create_string(arguments.args[i].argv_pointer[index]);
    } 
    return(arg);
}

/**
 *  returns a string array containing all arguments to a option
 *
 *  @param  arguments cli_arguments structure created by cli_parse
 *  @param  option to check
 *  @param  memory arena where array will be created in
 *
 *  @return string array containing all arguments
 */
struct string_array
cli_get_args(struct cli_arguments arguments, u8 option, struct mem_arena *arena);

#endif
