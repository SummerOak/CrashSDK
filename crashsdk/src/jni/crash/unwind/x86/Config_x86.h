#ifndef CONFIG_x86_H
#define CONFIG_x86_H

#ifdef __i386__

#include<stdint.h>

/* This matches the value used by GCC (see
   gcc/config/i386.h:DWARF_FRAME_REGISTERS), which leaves plenty of
   room for expansion.  */
#define DWARF_NUM_PRESERVED_REGS	17

typedef uint32_t word_t;
typedef int32_t sword_t;

#endif

#endif