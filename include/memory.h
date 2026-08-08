#ifndef MEMORY_H
#define MEMORY_H

#include "platform.h"

#define KB  1024
#define MB  KB * KB
#define GB  MB * MB

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
};


internal umm
arena_remaining(struct mem_arena scratch)
{
    return(scratch.end - scratch.current);
}

internal umm
arena_size(struct mem_arena scratch)
{
    return(scratch.end - scratch.start);
}

/// create_mem_arena 
///
/// this function creates a scratch memory
///
/// @param  size    size for the whole scratch space
/// @return returns a new mem_arena
struct mem_arena 
create_mem_arena(umm size)
{
    struct mem_arena scratch = {};
    scratch.start = allocate(size); // TODO: theoretically allocate could fail
    scratch.end = scratch.start + size;
    scratch.current = scratch.start;
    return(scratch);
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
push_mem_arena(struct mem_arena *scratch, umm size)
{
    ASSERT(arena_remaining(*scratch) >= size);
    umm address = scratch->current;
    scratch->current += size;
    return(address);
}

#define ARENA_PUSH_STRUCT(scratch, structname) (structname*)push_mem_arena(scratch, sizeof(structname))
#define ARENA_PUSH_ARRAY(scratch, structname, entries) (structname*)push_mem_arena(scratch, sizeof(structname) * (entries))

/// destroy_mem_arena
///
/// this destroys a created scratch space 
///
/// @param  mem_arena  scratch space to destroy
void
destroy_mem_arena(struct mem_arena *scratch)
{
    deallocate(scratch->start, arena_size(*scratch));
    scratch->start = 0;
    scratch->end = 0;
    scratch->current = 0;
}

/**
 * creates a copy of a mem_arena
 *
 * as this copy is scope local the mem arena basically resets to the previous 
 * state whenever the scope is exited.
 *
 * this function may seem useless but adds to readability.
 * maybe create this as a DEFINE if this is ever as problem.
 *
 * !Data will stay in the memory but pointers are reset.
 *
 * @param   mem_arena to copy
 *
 * @return  exact copy of the struct
 */
struct mem_arena
create_scoped_arena(struct mem_arena arena)
{
    return(arena);
}


#endif 
