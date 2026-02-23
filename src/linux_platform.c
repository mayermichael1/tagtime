
#include "include/platform.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

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
