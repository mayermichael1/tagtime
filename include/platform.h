#ifndef PLATFORM_H
#define PLATFORM_H

#include "general.h"
#include "string.h"

typedef struct _mem_arena mem_arena;

void
set_platform_arena(mem_arena arena);

u64
get_file_size(string filename);

void
read_file(string filename, u64 len, u8 *buffer);

void
read_file_from(string filename, u64 from, u64 len, u8 *buffer);

void
write_file(string filename, u64 buffer_size, u8 *buffer);

void
append_file(string filename, u64 buffer_size, u8 *buffer);

umm
allocate(umm size);

void
deallocate(umm start_address, umm size);

u64
seconds_since_epoch();

string
get_data_directory();

#endif
