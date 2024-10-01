/* Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */
#include <errno.h>

#ifdef __ARMCC_VERSION
/* ARMCC */
#elif __ICCARM__
/* IAR */
#else
/* GCC (newlib) */

// The default _sbrk implementation of newlib does not do heap limit checking
static char* heap_end = 0;
void* _sbrk(int incr)
{
    extern char heap_start __asm ("end");
    extern char __HeapLimit;

    char* prev_heap_end;

    if (heap_end == 0) {
        heap_end = &heap_start;
    }

    prev_heap_end = heap_end;

    if (heap_end + incr > (&__HeapLimit)) {
        errno = ENOMEM;
        return (void*)-1;
    }

    heap_end += incr;
    return prev_heap_end;
}
#endif // GCC
