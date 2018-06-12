//
// Created by Summer on 09/06/2018.
//

#include <jni.h>
#include "Defines.h"
#include "Log.h"
#include "CrashMonitor.h"

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL
Java_com_summer_crashsdk_CrashSDK_initNative(JNIEnv *env,jobject obj)
{
	LOGR(PTAG(),"init...");
	return CrashMonitor::init();
}

#ifdef __cplusplus
}
#endif