#ifndef CRASH_MONITOR_H
#define CRASH_MONITOR_H

#include <signal.h>
#include <unwind.h>
#include <dlfcn.h>
#include "_Unwind.h"

struct BacktraceState{
    void** current;
    void** end;
};

struct CrashInfo{
	uint32_t pid;
	uint32_t tid;

	int sig;

};

class CrashMonitor{

public:
	static int init();
	

private:

	static void handle(int sig, siginfo_t* si, void* uctx);
	static void printBacktrace(UnwindNode& head);

	static struct sigaction sOldSigAction;

	static const int SIGNALS[];
	static const char* TAG;

};


#endif