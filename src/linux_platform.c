#include "include/platform.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#include "include/memory.h"
#include "include/string_memory.h"

global_variable struct mem_arena platform_local_temp_mem;

void 
flush_stdin(){
    s64 read_bytes = 0;
    do
    {
        read_bytes = read(STDIN_FILENO, NULL,  1000);
    }
    while(read_bytes != -1);
}

void
write_stdout(struct string value)
{
    write(STDOUT_FILENO, value.data, value.size);
}

void
write_stdout_cstring(const char *value)
{
    for(u8 *c = (u8*)value; *c != 0; ++c)
    {
        write(STDOUT_FILENO, c, 1);
    }
}

u8
read_u8_stdin(){
    u8 character = 0;
    read(STDIN_FILENO, &character, 1);
    flush_stdin();
    return(character);
}

u64 
get_file_size(struct string filename)
{
    struct stat st;
    struct mem_arena temp = {};
    u64 filesize = 0;
    MEM_ARENA_SCOPE(&platform_local_temp_mem,&temp)
    {
        const char *cfile = to_c_string(filename, &temp);
        if(stat(cfile, &st)==0)
        {
            filesize = st.st_size;
        }
    }
    return(filesize);
}

void
read_file(struct string filename, u64 len, u8 *buffer)
{
    read_file_from(filename, 0, len, buffer);
}

void
read_file_from(struct string filename, u64 from, u64 len, u8 *buffer)
{
    struct mem_arena temp = {};
    MEM_ARENA_SCOPE(&platform_local_temp_mem,&temp)
    {
        s32 file = open(to_c_string(filename, &temp), O_RDONLY);

        if(file > 0)
        {
            lseek(file, from, SEEK_SET);
            read(file , buffer, len);
            close(file);
        }
    }
}

void
write_file(struct string filename, u64 file_size, u8 *buffer)
{
    struct string dirname = string_split_to(filename, string_find_last(filename, '/'));
    struct mem_arena temp = {};
    MEM_ARENA_SCOPE(&platform_local_temp_mem,&temp)
    {
        const char *dir = to_c_string(dirname, &temp);

        if(mkdir(dir, 0777) == 0 || errno == EEXIST)
        {
            s32 file = open(to_c_string(filename, &temp), O_WRONLY | O_CREAT | O_TRUNC, 0777);

            if(file > 0)
            {
                write(file, buffer, file_size);
                close(file);
            }
        }
    }
}

//TODO: massive code duplication from write_file
void
append_file(struct string filename, u64 file_size, u8 *buffer)
{
    struct string dirname = string_split_to(filename, string_find_last(filename, '/'));
    struct mem_arena temp = {};
    MEM_ARENA_SCOPE(&platform_local_temp_mem,&temp)
    {
        const char *dir = to_c_string(dirname, &temp);

        if(mkdir(dir, 0777) == 0 || errno == EEXIST)
        {
            s32 file = open(to_c_string(filename, &temp), O_WRONLY | O_CREAT | O_APPEND, 0777);

            if(file > 0)
            {
                write(file, buffer, file_size);
                close(file);
            }
        }
    }
}

umm
allocate(umm size)
{
    return (umm)mmap(
        0,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
        0,
        0
    );
}

void
deallocate(umm start_address, umm size)
{
    munmap((void *)start_address, size);
}

u64
seconds_since_epoch()
{
    return(time(NULL));
}

struct string
get_data_directory(struct mem_arena *mem)
{
    struct mem_arena temp = {};
    struct string dir = {};
    MEM_ARENA_SCOPE(&platform_local_temp_mem,&temp)
    {
        struct stringbuilder sb = stringbuilder_create(1024, &temp);
        sb = stringbuilder_append(sb, create_string(getenv("XDG_DATA_HOME")));
        // TODO: determinine application name dynamically somehow
        if(sb.string.size == 0)
        {

            sb = stringbuilder_append(sb, create_string(getenv("HOME")));
            sb = stringbuilder_append(sb, create_string("/.local/share/tagtime/"));
        }
        else
        {
            sb = stringbuilder_append(sb, create_string("/tagtime/"));
        }
        dir = stringbuilder_build(sb, mem);
    }
    return(dir);
}

struct string_array
cli_get_args(struct cli_arguments arguments, u8 option, struct mem_arena *arena)
{
    struct string_array arr = {.count = cli_option_count(arguments, option)};
    arr.data = MEM_ARENA_PUSH_ARRAY(arena, struct string, arr.count);

    for(u32 i = 0; i < arr.count; ++i)
    {
        arr.data[i] = cli_get_arg(arguments, option, i);
    }

    return(arr);
}
