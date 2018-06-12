#include "CrashMonitor.h"
#include "Defines.h"
#include "Log.h"
#include <cxxabi.h>
#include <ucontext.h>
#include <unistd.h>
#include <stdlib.h>

struct sigaction CrashMonitor::sOldSigAction;
const int CrashMonitor::SIGNALS[] = { SIGABRT, SIGILL, SIGTRAP, SIGBUS, SIGFPE, SIGSEGV, SIGTERM, 

#ifdef SIGSTKFLT
	SIGSTKFLT,
#endif
	 0 };
const char* CrashMonitor::TAG = PTAG("CrashMonitor");

int CrashMonitor::init(){

	LOGD(TAG,"arch: %s", ARCH_NAME);



	ucontext_t uc;
	LOGD(TAG,"uc %p",&uc);

	struct sigaction action;
	memset(&action, 0, sizeof(action));
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = CrashMonitor::handle;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;

    sOldSigAction.sa_sigaction = NULL;
    sOldSigAction.sa_handler = NULL;

    for(int i=0;i<sizeof(SIGNALS);i++){
    	int sig = SIGNALS[i];
    	if(sig == 0){
    		break;
    	}

    	if (sigaction(sig, &action, &sOldSigAction) != 0) {
    		LOGE(TAG,"register signal(%d) failed.",sig);
			return 1;
		}
    }

    stack_t stack;  
	memset(&stack, 0, sizeof(stack));  
	/* Reserver the system default stack size. We don't need that much by the way. */  
	stack.ss_size = SIGSTKSZ;  
	stack.ss_sp = malloc(stack.ss_size);  
	if(stack.ss_sp == NULL){
		LOGE(TAG,"alloc preserved stack space failed.");
		return 1;
	}
	stack.ss_flags = 0;  
	/* Install alternate stack size. Be sure the memory region is valid until you revert it. */  
	if (sigaltstack(&stack, NULL) != 0) {  
		LOGE(TAG,"set preserved stack space failed.");
		return 1;
	}

    LOGD(TAG,"init crash monitor successed.");
	
	return 0;
}

void CrashMonitor::handle(int sig, siginfo_t* si, void* uc){
	LOGE(TAG,"recv signal(%d), siginfo(%p), ucontext(%p)",sig,si,uc);

#ifdef UNWIND_SUPPORT
	LOGR(TAG,"dump call stack...");
	Unwind und = Unwind();
	und.setucontext((ucontext_t*)uc);
	UnwindNode* head = NULL;
	und.unwind(&head);

	LOGR(TAG,"backtrace:");
	printBacktrace(*head);

#else
	LOGR(TAG,"current arch(%s) not support unwind yet.", ARCH_NAME);
#endif

	if(sOldSigAction.sa_sigaction != NULL){
		LOGD(TAG,"relay to old sa_sigaction first.");
		sOldSigAction.sa_sigaction(sig,si,uc);
	}else if(sOldSigAction.sa_handler != NULL){
		LOGD(TAG,"relay to old sa_handler first.");
		sOldSigAction.sa_handler(sig);
	}

	LOGR(TAG,"handle finished.");

	return;
}

void CrashMonitor::printBacktrace(UnwindNode& head){
	UnwindNode* p = &head;
	int i = 0;
	while(p){

		const char* symbol = NULL;
		Dl_info info;
        if(dladdr(p->ip, &info) == 0){
        	LOGE(TAG,"dladdr (0x%p) failed.", p->ip);
        	switch(sizeof(void*)){
				case 4:
				LOGR(TAG,"\t#%02d cfa(0x%08x) pc 0x%08x\tunknow", i, p->cfa, p->pc);
				break;
				case 8:
				LOGR(TAG,"\t#%02d cfa(0x%016x) pc 0x%016lx\tunknown", i, p->cfa, p->pc);
				break;
			}
        }else{
        	switch(sizeof(void*)){
				case 4:
				LOGR(TAG,"\t#%02d cfa(0x%08x) pc 0x%08x\t%s(0x%08x)(%s)", i, p->cfa, p->pc, p->libname, p->libbase, info.dli_sname);
				break;
				case 8:
				LOGR(TAG,"\t#%02d cfa(0x%016x) pc 0x%016lx\t%s(0x%016lx)(%s)", p->cfa, i, p->pc, p->libname, p->libbase, info.dli_sname);
				break;
			}
        }
		
		++i;
		p = p->next;
	}
}

