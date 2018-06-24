#ifndef _UNWIND_H
#define _UNWIND_H

#include "Arch.h"

typedef struct RegState{
	const char* regName;
	void* val;
}RegState;

typedef struct UnwindNode{
	uint32_t pc;
	void* ip;
	void* libbase;
	const char *libname;
	const char* signame;
	void* cfa;
	uint8_t regs_len;
	UnwindNode* next;
}UnwindNode;

#ifndef UNWIND_SUPPORT
#pragma message(ARCH_NAME "arch not support yet.")
#else
#include <link.h>
#include "DataTypes.h"
#include "Context.h"

class Unwind{

public:

	Unwind();
	~Unwind();

	void setucontext(ucontext_t *uc);
	int unwind(UnwindNode** head);

	static void printNode(UnwindNode& node);
	
private:

	static const char* TAG;

	Context* mContext;
};

#endif// ARCH_SUPPORT



#endif