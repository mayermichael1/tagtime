#ifndef MEMORY_H
#define MEMORY_H

#include "platform.h"
#include "math.h"

#define KB  1024
#define MB  KB * KB
#define GB  MB * MB

enum mem_arena_flags
{
    MEM_ARENA_NO_ZERO_INIT = 0x01,
    MEM_ARENA_CURRENTLY_SCOPED = 0x10, // is this memory currently being used as 
                                       // a scoped memory if so it may not be 
                                       // used to allocate more memory 
};

/// whenever a sratch_memory is passed as a pointer the buffer is permanently 
/// changed 
/// when the struct is passed as a value it works like a temporary data store
/// because changes to current are just poped form the stack as soon as  the 
/// scope is exited the memory returns to its previous state
/// this only works as long as the scratch does not grow itself
/// the data is not deleted, only the pointer is returned to the previous position
//TODO: this might be extended for auto growing arenas and heap like implementations if needed
struct mem_arena
{
    umm start;  
    umm current; 
    umm end; 
    enum mem_arena_flags flags;
};


internal umm
mem_arena_remaining(struct mem_arena scratch)
{
    return(scratch.end - scratch.current);
}

internal umm
mem_arena_size(struct mem_arena scratch)
{
    return(scratch.end - scratch.start);
}

internal void 
mem_arena_zero(umm start, umm end)
{
    for(umm curr = start; curr < end; ++curr)
    {
        ((u8*)0)[curr] = 0;
    }
}

/// 
/// this function creates a scratch memory
///
/// @param  size    size for the whole scratch space
/// @return returns a new mem_arena
struct mem_arena
mem_arena_create_with_flags(umm size, enum mem_arena_flags flags)
{
    struct mem_arena scratch = {};
    scratch.flags = flags;
    scratch.start = allocate(size); // TODO: theoretically allocate could fail
    scratch.end = scratch.start + size;
    scratch.current = scratch.start;
    return(scratch);
}

///
/// this function creates a scratch memory with default flags
///
struct mem_arena 
mem_arena_create(umm size)
{
    struct mem_arena mem = mem_arena_create_with_flags(size, 0);
    return(mem);
}

/// scratch_push
///
/// push a struct onto a scratch memory
///
/// @param  mem_arena  which scracth memory should be used 
/// @param  size            size to reserve
///
/// @return returns the memory address of the reserved memory
//TODO: maybe create a safe function that zeroes out memory when allocating
//      currently memory is returned as is 
umm
mem_arena_push(struct mem_arena *scratch, umm size)
{
    ASSERT(mem_arena_remaining(*scratch) >= size);
    ASSERT(!MASK(scratch->flags, MEM_ARENA_CURRENTLY_SCOPED));
    umm address = scratch->current;
    //TODO: make this a setting as it may slow down mem allocation
    if(!MASK(scratch->flags, MEM_ARENA_NO_ZERO_INIT))
    {
        mem_arena_zero(address, address+size);
    }
    scratch->current += size;
    return(address);
}

#define MEM_ARENA_PUSH_STRUCT(scratch, structname) (structname*)mem_arena_push(scratch, sizeof(structname))
#define MEM_ARENA_PUSH_ARRAY(scratch, structname, entries) (structname*)mem_arena_push(scratch, sizeof(structname) * (entries))
//NOTE: this directly creates a struct containing first the count and then the array containing the elements
//TODO: following is allowed in C99
//      struct arr
//      {
//          u32 size;
//          u8 data[]; // size = 0
//      }
//      currently all arrays are not like this. data format in tagtime depends on this :(
//#define ARENA_PUSH_STRUCT_ARRAY(mem, arraystruct, arrayelement, elementcount) (arraystruct*)push_mem_arena(mem, sizeof(arraystruct) + sizeof(arrayelement) * (elementcount - 1));

/// destroy_mem_arena
///
/// this destroys a created scratch space 
///
/// @param  mem_arena  scratch space to destroy
void
mem_arena_destroy(struct mem_arena *scratch)
{
    deallocate(scratch->start, mem_arena_size(*scratch));
    scratch->start = 0;
    scratch->end = 0;
    scratch->current = 0;
}

/**
 * creates a scoped mem arena from an existing one
 *
 * @param   mem arena to scope
 *
 * @return  mem arena that can be used until it is ended
 */
struct mem_arena
mem_arena_scoped_begin(struct mem_arena *source)
{
    ASSERT(!MASK(source->flags, MEM_ARENA_CURRENTLY_SCOPED));
    struct mem_arena scoped = *source;
    source->flags |= MEM_ARENA_CURRENTLY_SCOPED;
    return(scoped);
}

void
mem_arena_scoped_begin_scoped_as_pointer(struct mem_arena *source, struct mem_arena *scoped)
{
    *scoped = mem_arena_scoped_begin(source);
}

/**
 * end a scoped arena 
 *
 * @param   mem_arena originally scoped from
 * @param   the scoped arena
 */
void
mem_arena_scoped_end(struct mem_arena *original_memory, struct mem_arena *scoped)
{
    (*scoped) = (struct mem_arena){};
    original_memory->flags &= (~MEM_ARENA_CURRENTLY_SCOPED);
}

//TODO: not quite happy with this as the scoped memory has to be provided as well
#define MEM_ARENA_SCOPE(mem, scoped) FOR_DEFER_BLOCK(mem_arena_scoped_begin_scoped_as_pointer(&mem, &scoped), mem_arena_scoped_end(&mem, &scoped))


#endif 
