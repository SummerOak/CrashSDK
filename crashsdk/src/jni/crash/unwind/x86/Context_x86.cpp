#include "Context_x86.h"
#include "Log.h"
#include "Defines.h"
#include "EHFrame.h"

#ifdef __i386__

const char* Context_x86::TAG = PTAG("Context_x86");

int Context_x86::setupFrame(CFI& cfi){

	cfi.loc[EAX] = getReg(UNW_X86_EAX);
	cfi.loc[ECX] = getReg(UNW_X86_ECX);
	cfi.loc[EDX] = getReg(UNW_X86_EDX);
	cfi.loc[EBX] = getReg(UNW_X86_EBX);
	cfi.loc[ESP] = getReg(UNW_X86_ESP);
	cfi.loc[EBP] = getReg(UNW_X86_EBP);
	cfi.loc[ESI] = getReg(UNW_X86_ESI);
	cfi.loc[EDI] = getReg(UNW_X86_EDI);
	cfi.loc[EIP] = getReg(UNW_X86_EIP);
	cfi.loc[EFLAGS] = getReg(UNW_X86_EFLAGS);
	cfi.loc[TRAPNO] = getReg(UNW_X86_TRAPNO);
	cfi.loc[ST0] = getReg(UNW_X86_ST0);

	for (int i = ST0 + 1; i < DWARF_NUM_PRESERVED_REGS; ++i){
		cfi.loc[i] = 0;
	}

	cfi.ip = cfi.loc[EIP];
	cfi.cfa = cfi.loc[ESP];

	return 0;
}

word_t Context_x86::getReg(uint32_t reg){
	void* addr;
	switch (reg){
		case UNW_X86_GS:  addr = &mContext->uc_mcontext.gregs[REG_GS]; break;
		case UNW_X86_FS:  addr = &mContext->uc_mcontext.gregs[REG_FS]; break;
		case UNW_X86_ES:  addr = &mContext->uc_mcontext.gregs[REG_ES]; break;
		case UNW_X86_DS:  addr = &mContext->uc_mcontext.gregs[REG_DS]; break;
		case UNW_X86_EAX: addr = &mContext->uc_mcontext.gregs[REG_EAX]; break;
		case UNW_X86_EBX: addr = &mContext->uc_mcontext.gregs[REG_EBX]; break;
		case UNW_X86_ECX: addr = &mContext->uc_mcontext.gregs[REG_ECX]; break;
		case UNW_X86_EDX: addr = &mContext->uc_mcontext.gregs[REG_EDX]; break;
		case UNW_X86_ESI: addr = &mContext->uc_mcontext.gregs[REG_ESI]; break;
		case UNW_X86_EDI: addr = &mContext->uc_mcontext.gregs[REG_EDI]; break;
		case UNW_X86_EBP: addr = &mContext->uc_mcontext.gregs[REG_EBP]; break;
		case UNW_X86_EIP: addr = &mContext->uc_mcontext.gregs[REG_EIP]; break;
		case UNW_X86_ESP: addr = &mContext->uc_mcontext.gregs[REG_ESP]; break;
		case UNW_X86_TRAPNO:  addr = &mContext->uc_mcontext.gregs[REG_TRAPNO]; break;
		case UNW_X86_CS:  addr = &mContext->uc_mcontext.gregs[REG_CS]; break;
		case UNW_X86_EFLAGS:  addr = &mContext->uc_mcontext.gregs[REG_EFL]; break;
		case UNW_X86_SS:  addr = &mContext->uc_mcontext.gregs[REG_SS]; break;

		default:
		LOGE(TAG,"unknown reg %d", reg);
		addr = NULL;
	}

	if(addr != NULL){
		return *(word_t*)addr;
	}

	return 0;
}

int Context_x86::restoreFrame(){
	if(rootFrame()){
		LOGE(TAG,"restoreFrame failed , it's root frame.");
		return 1;
	}
	
	int ret = 0;
	if((ret = EHFrame::restoreFrame(*this)) != 0){
		LOGE(TAG,"restoreFrame from eh_frame failed, try call routine.")
		if((ret = stepBack(mCfi)) != 0){
			LOGE(TAG,"try call routine failed.");
			return ret;
		}
	}

	--mCfi.ip;
	mCfi.loc[ESP] = mCfi.cfa;

	return ret;
}

int Context_x86::stepBack(CFI& cfi){
	int ret;
	word_t ebp = cfi.loc[EBP];
	if(ebp <= cfi.cfa || ebp - cfi.cfa > 1<<20){
		LOGE(TAG,"wrong ebp(0x%lx)", ebp);
		return 1;
	}

	cfi.loc[EBP] = *(word_t*)(ebp);
	cfi.ip = cfi.loc[EIP] = *(word_t*)(ebp+sizeof(word_t));
	cfi.cfa = cfi.loc[ESP] = ebp + 8;
	cfi.ret_addr_column = EIP;

	LOGD(TAG,"stepBack ebp(0x%lx)", ebp);

	return 0;
}

#endif
