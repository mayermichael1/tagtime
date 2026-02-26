#include "include/platform.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>

struct scratch_memory;

u64 
get_file_size(const char *filename)
{
    struct stat st;
    stat(filename, &st);

    u64 filesize = st.st_size;
    return filesize;
}
void
read_file(const char *filename, u64 file_size, u8 *buffer)
{
    s32 file = open(filename, O_RDONLY);

    if(file > 0)
    {
        read(file, buffer, file_size);
        close(file);
    }
    buffer[file_size] = 0;
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
