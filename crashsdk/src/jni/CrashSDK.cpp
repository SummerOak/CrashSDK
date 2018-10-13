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
	return CrashMonitor::init(env);
}

JNIEXPORT void JNICALL
Java_com_summer_crashsdk_CrashSDK_setSystemInfo(JNIEnv *env,jobject obj, jstring fingerprint, jint version, jstring abis)
{
	const char* jni_fingerprint = env->GetStringUTFChars(fingerprint,0);
	const char* jni_abis = env->GetStringUTFChars(abis,0);
	CrashMonitor::setSystemInfo(jni_fingerprint, version, jni_abis);

	env->ReleaseStringUTFChars(abis,jni_abis);
	env->ReleaseStringUTFChars(fingerprint,jni_fingerprint);
}

JNIEXPORT void JNICALL
Java_com_summer_crashsdk_CrashSDK_setLogDir(JNIEnv *env,jobject obj, jstring dir)
{
	const char* szDir = env->GetStringUTFChars(dir,0);
	CrashMonitor::setLogDir(szDir);
	env->ReleaseStringUTFChars(dir,szDir);
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) 
{
	LOGR(PTAG(),"jni onload...");
	CrashMonitor::setVM(vm);
	return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif