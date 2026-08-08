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

u8
read_u8_stdin(){
    u8 character = 0;
    read(STDIN_FILENO, &character, 1);
    flush_stdin();
    return(character);
}

void
set_platform_arena(struct mem_arena arena)
{
    platform_local_temp_mem = arena;
}

u64 
get_file_size(struct string filename)
{
    struct stat st;
    const char *cfile = to_c_string(filename, &platform_local_temp_mem);
    u64 filesize = 0;
    if(stat(cfile, &st)==0)
    {
        filesize = st.st_size;
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
    s32 file = open(to_c_string(filename, &platform_local_temp_mem), O_RDONLY);

    if(file > 0)
    {
        lseek(file, from, SEEK_SET);
        read(file , buffer, len);
        close(file);
    }
}

void
write_file(struct string filename, u64 file_size, u8 *buffer)
{
    struct string dirname = string_split_to(filename, string_find_last(filename, '/'));
    const char *dir = to_c_string(dirname, &platform_local_temp_mem);

    if(mkdir(dir, 0777) == 0 || errno == EEXIST)
    {
        s32 file = open(to_c_string(filename, &platform_local_temp_mem), O_WRONLY | O_CREAT | O_TRUNC, 0777);

        if(file > 0)
        {
            write(file, buffer, file_size);
            close(file);
        }
    }
}

//TODO: massive code duplication from write_file
void
append_file(struct string filename, u64 file_size, u8 *buffer)
{
    struct string dirname = string_split_to(filename, string_find_last(filename, '/'));
    const char *dir = to_c_string(dirname, &platform_local_temp_mem);

    if(mkdir(dir, 0777) == 0 || errno == EEXIST)
    {
        s32 file = open(to_c_string(filename, &platform_local_temp_mem), O_WRONLY | O_CREAT | O_APPEND, 0777);

        if(file > 0)
        {
            write(file, buffer, file_size);
            close(file);
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
get_data_directory()
{
    struct string dir = create_string(getenv("XDG_DATA_HOME"));
    // TODO: determinine application name dynamically somehow
    // TODO: stringbuilder for appending strings here. right now string 
    //       is just duplicated and stored in abcking store again
    if(dir.size == 0)
    {
        dir = create_string(getenv("HOME"));
        dir = string_append(dir, create_string("/.local/share/tagtime/"), &platform_local_temp_mem);
    }
    else
    {
        dir = string_append(dir, create_string("/tagtime/"), &platform_local_temp_mem);
    }
    return(dir);
}

struct string_array
cli_get_args(struct cli_arguments arguments, u8 option, struct mem_arena *arena)
{
    struct string_array arr = {.count = cli_option_count(arguments, option)};
    arr.data = ARENA_PUSH_ARRAY(arena, struct string, arr.count);

    for(u32 i = 0; i < arr.count; ++i)
    {
        arr.data[i] = cli_get_arg(arguments, option, i);
    }

    return(arr);
}
