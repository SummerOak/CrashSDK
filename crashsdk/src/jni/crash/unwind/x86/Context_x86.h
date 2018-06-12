#ifndef Context_x86_h
#define Context_x86_h

#ifdef __i386__

#include "Context.h"

#define EAX	0
#define ECX	1
#define EDX	2
#define EBX	3
#define ESP	4
#define EBP	5
#define ESI	6
#define EDI	7
#define EIP	8
#define EFLAGS	9
#define TRAPNO	10
#define ST0	11

typedef enum {
	/* Note: general registers are expected to start with index 0.
	This convention facilitates architecture-independent
	implementation of the C++ exception handling ABI.  See
	_Unwind_SetGR() and _Unwind_GetGR() for details.

	The described register usage convention is based on "System V
	Application Binary Interface, Intel386 Architecture Processor
	Supplement, Fourth Edition" at

	http://www.linuxbase.org/spec/refspecs/elf/abi386-4.pdf

	It would have been nice to use the same register numbering as
	DWARF, but that doesn't work because the libunwind requires
	that the exception argument registers be consecutive, which the
	wouldn't be with the DWARF numbering.  */
	UNW_X86_EAX,	/* scratch (exception argument 1) */
	UNW_X86_EDX,	/* scratch (exception argument 2) */
	UNW_X86_ECX,	/* scratch */
	UNW_X86_EBX,	/* preserved */
	UNW_X86_ESI,	/* preserved */
	UNW_X86_EDI,	/* preserved */
	UNW_X86_EBP,	/* (optional) frame-register */
	UNW_X86_ESP,	/* (optional) frame-register */
	UNW_X86_EIP,	/* frame-register */
	UNW_X86_EFLAGS,	/* scratch (except for "direction", which is fixed */
	UNW_X86_TRAPNO,	/* scratch */

	/* MMX/stacked-fp registers */
	UNW_X86_ST0,	/* fp return value */
	UNW_X86_ST1,	/* scratch */
	UNW_X86_ST2,	/* scratch */
	UNW_X86_ST3,	/* scratch */
	UNW_X86_ST4,	/* scratch */
	UNW_X86_ST5,	/* scratch */
	UNW_X86_ST6,	/* scratch */
	UNW_X86_ST7,	/* scratch */

	UNW_X86_FCW,	/* scratch */
	UNW_X86_FSW,	/* scratch */
	UNW_X86_FTW,	/* scratch */
	UNW_X86_FOP,	/* scratch */
	UNW_X86_FCS,	/* scratch */
	UNW_X86_FIP,	/* scratch */
	UNW_X86_FEA,	/* scratch */
	UNW_X86_FDS,	/* scratch */

	/* SSE registers */
	UNW_X86_XMM0_lo,	/* scratch */
	UNW_X86_XMM0_hi,	/* scratch */
	UNW_X86_XMM1_lo,	/* scratch */
	UNW_X86_XMM1_hi,	/* scratch */
	UNW_X86_XMM2_lo,	/* scratch */
	UNW_X86_XMM2_hi,	/* scratch */
	UNW_X86_XMM3_lo,	/* scratch */
	UNW_X86_XMM3_hi,	/* scratch */
	UNW_X86_XMM4_lo,	/* scratch */
	UNW_X86_XMM4_hi,	/* scratch */
	UNW_X86_XMM5_lo,	/* scratch */
	UNW_X86_XMM5_hi,	/* scratch */
	UNW_X86_XMM6_lo,	/* scratch */
	UNW_X86_XMM6_hi,	/* scratch */
	UNW_X86_XMM7_lo,	/* scratch */
	UNW_X86_XMM7_hi,	/* scratch */

	UNW_X86_MXCSR,	/* scratch */

	/* segment registers */
	UNW_X86_GS,		/* special */
	UNW_X86_FS,		/* special */
	UNW_X86_ES,		/* special */
	UNW_X86_DS,		/* special */
	UNW_X86_SS,		/* special */
	UNW_X86_CS,		/* special */
	UNW_X86_TSS,	/* special */
	UNW_X86_LDT,	/* special */

	/* frame info (read-only) */
	UNW_X86_CFA,

	UNW_X86_XMM0,	/* scratch */
	UNW_X86_XMM1,	/* scratch */
	UNW_X86_XMM2,	/* scratch */
	UNW_X86_XMM3,	/* scratch */
	UNW_X86_XMM4,	/* scratch */
	UNW_X86_XMM5,	/* scratch */
	UNW_X86_XMM6,	/* scratch */
	UNW_X86_XMM7,	/* scratch */

	UNW_TDEP_LAST_REG = UNW_X86_XMM7,

	UNW_TDEP_IP = UNW_X86_EIP,
	UNW_TDEP_SP = UNW_X86_ESP,
	UNW_TDEP_EH = UNW_X86_EAX
}x86_regnum_t;

class Context_x86: public Context{

public:
	Context_x86(){
	}

	~Context_x86(){
		mContext = NULL;
	}

	virtual int setupFrame(CFI& cfi);
	virtual word_t getReg(uint32_t reg);
	virtual int restoreFrame();
	virtual int stepBack(CFI& cfi);
	

private:

	static const char* TAG;

};

#endif // __i386__

#endif //Context_x86_h