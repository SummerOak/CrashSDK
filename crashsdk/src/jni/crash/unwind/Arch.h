#ifndef Unwind_arch_h
#define Unwind_arch_h

#if defined __aarch64__
#define ARCH_NAME "__aarch64__"
#elif defined __arm__
#define ARCH_NAME "__arm__"
#define UNWIND_SUPPORT 1
#include "Config_arm.h"
#elif defined __hppa__
#define ARCH_NAME "__hppa__"
#elif defined __ia64__
#define ARCARCH_NAMEH "__ia64__"
#elif defined __mips__
#define ARCH_NAME "__mips__"
#elif defined __i386__
#define ARCH_NAME "__i386__"
#define UNWIND_SUPPORT 1
#include "Config_x86.h"
#elif defined __x86_64__
#define ARCH_NAME "__x86_64__"
#else
#define ARCH_NAME "unknown"
#endif

#endif