#include "CrashMonitor.h"
#include "Defines.h"
#include "Log.h"
#include <cxxabi.h>
#include <ucontext.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

struct sigaction CrashMonitor::sOldSigAction;
const int CrashMonitor::SIGNALS[] = { SIGABRT, SIGILL, SIGTRAP, SIGBUS, SIGFPE, SIGSEGV, SIGTERM, SIGPIPE, 

#ifdef SIGSTKFLT
	SIGSTKFLT,
#endif
	 0 };

const char* CrashMonitor::TAG = PTAG("CrashMonitor");

int CrashMonitor::sPID = 0;
char CrashMonitor::sLogDir[128];
char CrashMonitor::sProcessName[60];
char CrashMonitor::sThreadName[60];
char CrashMonitor::sFingerprint[100];
int CrashMonitor::sVersion = 0;
char CrashMonitor::sABIs[50];
char CrashMonitor::sTEMP[512];

int CrashMonitor::init(JNIEnv* env){

	LOGD(TAG,"arch: %s", ARCH_NAME);

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

	initProcessInfo();

    LOGD(TAG,"init crash monitor successed.");
	
	return 0;
}

void CrashMonitor::setLogDir(const char* dir){
	strncpy(sLogDir, dir, sizeof(sLogDir));
	LOGR(TAG,"setLogDir: %s", sLogDir);
}

void CrashMonitor::initProcessInfo(){
	sPID = getpid();
	sprintf(sProcessName,"/proc/%d/cmdline",sPID);
	if(readline(sProcessName, sProcessName, sizeof(sProcessName)) == 0){
		LOGR(TAG,"initProcessInfo successed, pid=%d, name=%s", sPID, sProcessName);
	}  
}

int CrashMonitor::readline(const char* file, char* out, int len){

	int fd = open(file,O_RDONLY);
	if(fd < 0){
		LOGE(TAG,"open %s file failed.", file);
		return 1;
	}

	char c;
	int i = 0;
	while(i < len && read(fd,&c,1)==1){
		if(c == '\n' || c == ' ' || c == '\r'){
			break;
		}
		out[i++] = c;
	}

	out[i] = 0;
    
    if(close(fd)<0){
        LOGE(TAG,"close %s failed.", file);
        return 1;
    }

    return 0;
}

void CrashMonitor::setSystemInfo(const char* fingerprint, int version, const char* abis){
	strncpy(sFingerprint, fingerprint,sizeof(sFingerprint) - 1);
	strncpy(sABIs, abis,sizeof(sABIs) - 1);
	sFingerprint[sizeof(sFingerprint)-1] = '\0';
	sABIs[sizeof(sABIs)-1] = '\0';
	sVersion = version;

	LOGR(TAG,"setSystemInfo: fingerprint: %s\n version: %d\n abis: %s", fingerprint, version, abis);
}

void CrashMonitor::handle(int sig, siginfo_t* si, void* uc){
	LOGE(TAG,"recv signal(%d), siginfo(%p), ucontext(%p) si_code(%d)",sig,si,uc, si->si_code);

	int fd = -1;
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	LOGE(TAG,"sLogDir: %s", sLogDir);
	sprintf(sTEMP,"%s/%4d%02d%02d_%02d_%02d_%02d.crash", sLogDir, 
						1900+timeinfo->tm_year, 1+timeinfo->tm_mon, timeinfo->tm_mday,
						timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	LOGE(TAG,"create crash log file: %s", sTEMP);
	fd = open(sTEMP,O_CREAT|O_WRONLY);

	if(fd < 0){
		LOGE(TAG,"create log file failed.");
	}

	logCrashHeader(fd, si);

#ifdef UNWIND_SUPPORT
	LOGR(TAG,"dump call stack...");
	Unwind und = Unwind();
	und.setucontext((ucontext_t*)uc);
	UnwindNode* head = NULL;
	und.unwind(&head);

	printBacktrace(fd, *head);

#else
	LOGR(TAG,"current arch(%s) not support unwind yet.", ARCH_NAME);
#endif

	if(sOldSigAction.sa_sigaction != NULL){
		LOGD(TAG,"relay to old sa_sigaction.");
		sOldSigAction.sa_sigaction(sig,si,uc);
	}else if(sOldSigAction.sa_handler != NULL){
		LOGD(TAG,"relay to old sa_handler.");
		sOldSigAction.sa_handler(sig);
	}

	if(fd >= 0 && close(fd) < 0){
		LOGE(TAG,"close log file failed.");
	}

	LOGR(TAG,"handle finished.");

	return;
}

void CrashMonitor::logCrashHeader(int fd, siginfo* siginfo){
	int len = 0;
	int r = sizeof(sTEMP);
	int l = snprintf(sTEMP,r, 
		"*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***\n"
		"Build fingerprint: \'%s\'\n"
		"Revision: %d\n"
		"ABI: \'%s\'\n", sFingerprint, sVersion, sABIs);

	if(l > 0){
		len += l; r -= l;
	}

	int tid = gettid();
	sprintf(sThreadName,"/proc/%d/comm",tid);
	if(readline(sThreadName, sThreadName, sizeof(sThreadName)) == 0){
		l = snprintf(sTEMP+len, r, "pid: %d, tid: %d, name: %s  >>> %s <<<\n", sPID, tid, sThreadName, sProcessName);
	}else{
		l = snprintf(sTEMP+len, r, "pid: %d, >>> %s <<<\n", sPID, sProcessName);
	}

	if(l > 0){
		len += l; r -= l;
	}

	if((l = parseSignalInfo(siginfo, sTEMP + len, r)) > 0){
		len += l; r -= l;
		snprintf(sTEMP+len, r, "\n");
		++len; --r;
	}

	if(len > 0){
		LOGE(TAG,"%s",sTEMP);

		if(fd >= 0){
			write(fd, sTEMP, len);
		}
		
	}

}

void CrashMonitor::printBacktrace(int fd, UnwindNode& head){
	LOGR(TAG,"backtrace:");
	if(fd >= 0){
		write(fd, "backtrace:\n", 11);
	}

	UnwindNode* p = &head;
	int i = 0;
	int l = 0;
	while(p){
		l = 0;
		const char* symbol = NULL;
		Dl_info info;
        if(dladdr(p->ip, &info) == 0){
        	// LOGE(TAG,"dladdr (0x%p) failed.", p->ip);
        	switch(sizeof(void*)){
				case 4:
				l = snprintf(sTEMP,sizeof(sTEMP),"\t#%02d pc %08x  unknow\n", i, p->pc);
				break;
				case 8:
				l = snprintf(sTEMP,sizeof(sTEMP),"\t#%02d pc %016lx  unknown\n", i, (unsigned long)p->pc);
				break;
			}
        }else{
        	if(info.dli_sname == NULL){
				switch(sizeof(void*)){
					case 4:
					l = snprintf(sTEMP,sizeof(sTEMP),"\t#%02d pc %08x  %s\n", i, p->pc, p->libname);
					break;
					case 8:
					l = snprintf(sTEMP,sizeof(sTEMP),"\t#%02d pc %016lx  %s\n", i, (unsigned long)p->pc, p->libname);
					break;
				}
        	}else{
        		switch(sizeof(void*)){
					case 4:
					l = snprintf(sTEMP,sizeof(sTEMP),"\t#%02d pc %08x  %s (%s+%u)\n", i, p->pc, p->libname, info.dli_sname, ((uint32_t)p->ip)-((uint32_t)info.dli_saddr));
					break;
					case 8:
					l = snprintf(sTEMP,sizeof(sTEMP),"\t#%02d pc %016lx  %s (%s+%llu)\n", i, (unsigned long)p->pc, p->libname, info.dli_sname, ((uint64_t)p->ip)-((uint64_t)info.dli_saddr));
					break;
				}
        	}
        	
        }

        LOGE(TAG,"%s", sTEMP);
        if(fd >= 0 && l > 0){
        	write(fd, sTEMP, l);
        }
		
		++i;
		p = p->next;
	}
}

const int CrashMonitor::parseSignalInfo(siginfo_t* siginfo, char* out, int len){

	switch(siginfo->si_signo){
		case SIGSEGV:{

			switch(siginfo->si_code){
				case SEGV_MAPERR: return snprintf(out,len,"signal %d (%s), code %d (%s), fault addr %p", siginfo->si_signo, "SIGSEGV", siginfo->si_code, "SEGV_MAPERR", siginfo->si_addr);
				case SEGV_ACCERR: return snprintf(out,len,"signal %d (%s), code %d (%s), fault addr %p", siginfo->si_signo, "SIGSEGV", siginfo->si_code, "SEGV_ACCERR", siginfo->si_addr);
				case SEGV_BNDERR: return snprintf(out,len,"signal %d (%s), code %d (%s)", siginfo->si_signo, "SIGSEGV", siginfo->si_code, "SEGV_BNDERR");
				case SEGV_PKUERR: return snprintf(out,len,"signal %d (%s), code %d (%s)", siginfo->si_signo, "SIGSEGV", siginfo->si_code, "SEGV_PKUERR");
			}

			return snprintf(out,len,"signal %d (%s), code %d", siginfo->si_signo, "SIGSEGV", siginfo->si_code);
		}

		case SIGABRT:{
			return snprintf(out,len,"signal %d (%s), code %d", siginfo->si_signo, "SIGABRT", siginfo->si_code);
		}
		case SIGILL:{
			return snprintf(out,len,"signal %d (%s), code %d", siginfo->si_signo, "SIGILL", siginfo->si_code);
		}
		case SIGTRAP:{
			return snprintf(out,len,"signal %d (%s), code %d", siginfo->si_signo, "SIGTRAP", siginfo->si_code);
		}
		case SIGBUS:{
			return snprintf(out,len,"signal %d (%s), code %d", siginfo->si_signo, "SIGBUS", siginfo->si_code);
		}
		case SIGFPE:{
			return snprintf(out,len,"signal %d (%s), code %d", siginfo->si_signo, "SIGFPE", siginfo->si_code);
		}
		case SIGTERM:{
			return snprintf(out,len,"signal %d (%s), code %d", siginfo->si_signo, "SIGTERM", siginfo->si_code);
		}
#ifdef SIGSTKFLT
		case SIGSTKFLT:{
			return snprintf(out,len,"signal %d (%s), code %d", siginfo->si_signo, "SIGSTKFLT", siginfo->si_code);
		}
#endif

	}

	return 0;
}

