/**
 ******************************************************************************
 * @file    sysmem.c
 * @brief   _sbrk() implementation backing newlib's malloc()/free(), with a
 *          stack-collision guard. Standard CubeIDE-generated file.
 ******************************************************************************
 */
#include <sys/types.h>
#include <errno.h>

extern char _end;      /* start of heap, defined in the linker script    */
extern char _estack;   /* top of stack, defined in the linker script     */
extern uint32_t _Min_Stack_Size;

static char *current_heap_end = 0;

caddr_t _sbrk(int incr)
{
    extern char end asm("end");
    char *prev_heap_end;

    if (current_heap_end == 0)
    {
        current_heap_end = &end;
    }

    prev_heap_end = current_heap_end;

    char *stack_limit = (char *)((uint32_t)&_estack - (uint32_t)&_Min_Stack_Size);

    if ((current_heap_end + incr) > stack_limit)
    {
        errno = ENOMEM;
        return (caddr_t)-1;
    }

    current_heap_end += incr;
    return (caddr_t)prev_heap_end;
}
