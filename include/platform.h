#ifndef PLATFORM_H
#define PLATFORM_H

#include "general.h"
#include "string.h"

typedef struct _scratch_memory scratch_memory;

u64
get_file_size(string filename, scratch_memory scratch);

void
read_file(string filename, u64 file_size, u8 *buffer, scratch_memory scratch);

umm
allocate(umm size);

void
deallocate(umm start_address, umm size);

u64
seconds_since_epoch();

string
get_data_directory();

#endif
