#include <stdio.h>

#include "include/general.h"
#include "include/platform.h"
#include "include/memory.h"
#include "include/math.h"

#include "src/linux_platform.c"


s32 
main (void)
{
    scratch_memory mem = create_scratch_memory(500 * MB);
    while(true);
    printf("Hello World \n");
    return 0;
}
