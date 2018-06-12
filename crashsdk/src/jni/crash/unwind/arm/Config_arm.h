#ifndef CONFIG_arm_H
#define CONFIG_arm_H

#ifdef __arm__

#include<stdint.h>

/* This matches the value used by GCC (see
   gcc/config/i386.h:DWARF_FRAME_REGISTERS), which leaves plenty of
   room for expansion.  */
#define DWARF_NUM_PRESERVED_REGS	128

typedef uint32_t word_t;
typedef int32_t sword_t;

#endif

#endif