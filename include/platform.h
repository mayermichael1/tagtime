#ifndef PLATFORM_H
#define PLATFORM_H

#include "general.h"

u64
get_file_size(const char *filename);

void
read_file(const char *filename, u64 file_size, u8 *buffer);

umm
allocate(umm size);

void
deallocate(umm start_address, umm size);

#endif
