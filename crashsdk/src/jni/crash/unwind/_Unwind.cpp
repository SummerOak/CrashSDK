#include "_Unwind.h"
#include "Log.h"
#include "EHFrame.h"


#define MAX_STACK_DEEP 200

const char* Unwind::TAG = PTAG("Unwind");

Unwind::Unwind(){
	#ifdef __i386__
		mContext = new Context_x86();
	#elif defined __arm__
		mContext = new Context_arm();
	#else
		mContext = NULL;
	#endif
}

Unwind::~Unwind(){
	if(mContext != NULL){
		SAFE_DELETE(mContext);
	}
}

void Unwind::setucontext(ucontext_t *uc){
	if(mContext != NULL){
		mContext->setucontext(uc);
	}
}

int Unwind::unwind(UnwindNode** head){
	if(mContext == NULL){
		LOGE(TAG,"context is NULL, %s not support yet.", ARCH_NAME);
		return 1;
	}

	if(head == NULL){
		return 1;
	}

	LOGD(TAG,"unwind start...");

	if(mContext->reset()){
		LOGE(TAG,"reset first frame failed.");
		return 1;
	}

	UnwindNode* tail = NULL;
	uint8_t deep = 0;
	bool skiped = false;
	while(MAX_STACK_DEEP > deep++){
		if(mContext->rootFrame()){
			LOGE(TAG,"end of call stack.");
			break;
		}

		UnwindNode* node = new UnwindNode();
		if((tail) != NULL){
			tail->next = node;
		}else{
			*head = node;
		}
		tail = node;
		node->next = NULL;

		node->ip = (void*)(mContext->getIP());
		node->pc = (uint32_t)node->ip;
		node->cfa = (void*)mContext->getCFA();
		if(mContext->restoreFrame() == 0){
			node->pc = (word_t)node->ip - (word_t)mContext->lastLibBase();
			node->libname = mContext->lastLibName();
			node->libbase = (void*)mContext->lastLibBase();

			LOGD(TAG,"\t#%02d pc(0x%08x) ip(%p) cfa(0x%p) %s", 
				deep, node->pc, mContext->getIP(), mContext->getCFA(), mContext->rootFrame()?"root":"");

			continue;
		}

		return 1;
	}

	return 0;
}

void Unwind::printNode(UnwindNode& node){
	switch(sizeof(void*)){
		case 4:
		LOGR(TAG,"\t#%02d pc 0x%08x\t%s(%s)", 0, node.pc, node.libname, node.signame);
		break;
		case 8:
		LOGR(TAG,"\t#%02d pc 0x%016lx\t%s(%s)", 0, node.pc, node.libname, node.signame);
		break;
	}
}

void Unwind::printMem(word_t s,word_t t){
	LOGD(TAG,"mem from 0x%lx - 0x%lx:", (long)s,(long)t);

	while(s <= t){
		word_t ss = (s+3)&(~3);
		if(ss == s){
			ss = s+4;
		}
		switch(ss-s){
			case 1:
			LOGD(TAG,"0x%lx: %02x%02x%02x%02x",s,*(uint8_t*)(s),0,0,0);
			break;
			case 2:
			LOGD(TAG,"0x%lx: %02x%02x%02x%02x",s,*(uint8_t*)(s+1), *(uint8_t*)(s), 0,0);
			break;
			case 3:
			LOGD(TAG,"0x%lx: %02x%02x%02x%02x",s,*(uint8_t*)(s+2),*(uint8_t*)(s+1),*(uint8_t*)(s),0);
			break;
			case 4:
			LOGD(TAG,"0x%lx: %02x%02x%02x%02x",s,*(uint8_t*)(s+3),*(uint8_t*)(s+2),*(uint8_t*)(s+1),*(uint8_t*)(s));
			break;
			default:
			break;
		}

		s = ss;
	}

}





