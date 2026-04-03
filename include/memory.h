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
typedef struct _scratch_memory //NOTE: needed for forward declaration in platform layer
{
    umm start;  
    umm current; 
    umm end; 
}
scratch_memory;

internal umm
scratch_remaining(scratch_memory scratch)
{
    return(scratch.end - scratch.current);
}

internal umm
scratch_size(scratch_memory scratch)
{
    return(scratch.end - scratch.start);
}

/// create_scratch_memory 
///
/// this function creates a scratch memory which internally operates as a 
/// stack
///
/// @param  size    size for the whole scratch space
/// @return returns a new scratch_memory
scratch_memory 
create_scratch_memory(umm size)
{
    scratch_memory scratch = {};
    scratch.start = allocate(size);
    scratch.end = scratch.start + size;
    scratch.current = scratch.start;
    return(scratch);
}


/// scratch_push
///
/// push a struct onto a scratch memory
///
/// @param  scratch_memory  which scracth memory should be used 
/// @param  size            size to reserve
///
/// @return returns the memory address of the reserved memory
//TODO: There maybe should be a MACRO to push whatever struct onto the scratch
umm
push_scratch_memory(scratch_memory *scratch, umm size)
{
    ASSERT(scratch_remaining(*scratch) >= size);
    umm address = scratch->current;
    scratch->current += size;
    return(address);
}

#define PUSH_SCRATCH_STRUCT(scratch, structname) (structname*)push_scratch_memory(scratch, sizeof(structname))
#define PUSH_SCRATCH_ARRAY(scratch, structname, entries) (structname*)push_scratch_memory(scratch, sizeof(structname) * (entries))

/// destroy_scratch_memory
///
/// this destroys a created scratch space 
///
/// @param  scratch_memory  scratch space to destroy
void
destroy_scratch_memory(scratch_memory *scratch)
{
    deallocate(scratch->start, scratch_size(*scratch));
    scratch->start = 0;
    scratch->end = 0;
    scratch->current = 0;
}


#endif 
