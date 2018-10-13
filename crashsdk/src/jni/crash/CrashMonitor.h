#ifndef CRASH_MONITOR_H
#define CRASH_MONITOR_H

#include <jni.h>
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
	static int init(JNIEnv* env);
	static void setLogDir(const char* dir);
	static void setSystemInfo(const char* fingerprint, int version, const char* abis);
	static void setVM(JavaVM* vm);

private:
	static void initProcessInfo();
	static int readline(const char* file, char* out, int len);
	static void handle(int sig, siginfo_t* si, void* uctx);
	static void logCrashHeader(int fd, siginfo_t *si);
	static const int parseSignalInfo(siginfo_t* siginfo, char* out, int len);

	static void printBacktrace(int fd, UnwindNode& head);

	
	static JNIEnv* getEnv();
	static void initJniBridge(JNIEnv* env);
	static uint64_t getNativeHeapAllocatedSize();
	static uint64_t getNativeHeapSize();

	static struct sigaction sOldSigAction;

	static JavaVM* sVM;
	static jclass sClzDebug;
	static jmethodID sMidGetNativeHeapSize;
	static jmethodID sMidGetNativeHeapAllocatedSize;

	static const int SIGNALS[];
	static const char* TAG;

	static int sPID;
	static char sLogDir[];
	static char sProcessName[];
	static char sThreadName[];
	static char sFingerprint[];
	static int sVersion;
	static char sABIs[];

	static char sTEMP[];

};


#endif