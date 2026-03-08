#include "include/platform.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#include "include/string_memory.h"

u64 
get_file_size(string filename, scratch_memory scratch)
{
    struct stat st;
    const char *cfile = to_c_string(filename, &scratch);
    u64 filesize = 0;
    if(stat(cfile, &st)==0){
        filesize = st.st_size;
    }
    return(filesize);
}

void
read_file(string filename, u64 file_size, u8 *buffer, scratch_memory scratch)
{
    s32 file = open(to_c_string(filename, &scratch), O_RDONLY);

    if(file > 0)
    {
        read(file , buffer, file_size);
        close(file);
    }
}

void
write_file(string filename, u64 file_size, u8 *buffer, scratch_memory scratch)
{
    string dirname = string_split_to(filename, string_find_last(filename, '/'));
    const char *dir = to_c_string(dirname, &scratch);

    if(mkdir(dir, 0777) == 0 || errno == EEXIST)
    {
        s32 file = open(to_c_string(filename, &scratch), O_WRONLY | O_CREAT | O_TRUNC, 0777);

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

string
get_data_directory(scratch_memory *scratch)
{
    string dir = create_string(getenv("XDG_DATA_HOME"));
    // TODO: determinine application name dynamically somehow
    // TODO: stringbuilder for appending strings here. right now string 
    //       is just duplicated and stored in abcking store again
    if(dir.size == 0)
    {
        dir = create_string(getenv("HOME"));
        dir = string_append(dir, create_string("/.local/share/tagtime/"), scratch);
    }
    else
    {
        dir = string_append(dir, create_string("/tagtime/"), scratch);
    }
    return(dir);
}
