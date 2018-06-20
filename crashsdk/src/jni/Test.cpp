//
// Created by Summer on 09/06/2018.
//

#ifndef CRASHSDKDEMO_TEST_H
#define CRASHSDKDEMO_TEST_H

#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include "Defines.h"
#include "Log.h"

#ifdef __cplusplus
extern "C" {
#endif

enum TEST_ID{
	NPE = 1,
	WPE = 2,
	VPE = 3,
	STACK_OVERFLOW = 4,
	OOM = 5,
	ABORT = 6,
	ART_CRASH = 7,
	LIBC_CRASH =8,
};

void __attribute__((noinline)) a(int aa){
	LOGR(PTAG("test"),"a %d", aa);

	int arr[2000];
	memset(arr,0,sizeof(arr));
	for(int i=0;i<1000;i++){
		arr[i] = i;
	}

	a(arr[aa] + 1);
}

static jobject sobj;

void invalidatePointer(){
	int *p = (int*)0x123;
	a(*p);
}

void stackOverflow(){
	a(1);
}

void artCrash(JNIEnv *env, jobject obj){
	if(sobj == 0){
		sobj = obj;
	}else{
		jmethodID id = env->GetStaticMethodID((jclass)sobj, "getName", "()Ljava/lang/String;");
		jstring ret = (jstring)env->CallObjectMethod((jclass)sobj, id);
		const char* jsz = env->GetStringUTFChars(ret,0);
		LOGE(PTAG("test"),"test class: %s", jsz);
		env->ReleaseStringUTFChars(ret, jsz);
	}
}

class Test{
public:
	virtual void f(){
		LOGE(PTAG("test"), "test#f %d", a);
	}

	int a;
};

void falseVirtualPointer(){
	Test *t = new Test();
	SAFE_DELETE(t);
	t->f();

}

void oom(){

}


JNIEXPORT jint JNICALL
Java_com_summer_crashsdk_CrashSDK_test(JNIEnv *env,jobject obj, jint id)
{
	switch(id){
		case NPE:{
			int *p = NULL;
			a(*p);
			break;
		}
		case WPE:{
			int *dp = (int*)0xd123;
			LOGE(PTAG("test"), "dp %d", *dp);
			break;
		}
		case VPE:{
			falseVirtualPointer();
			break;
		}
		case STACK_OVERFLOW:{
			stackOverflow();
			break;
		}
		case OOM:{
			oom();
			break;
		}
		case ABORT:{
			abort();
			break;
		}
		case ART_CRASH:{
			artCrash(env, obj);
			break;
		}
		case LIBC_CRASH:{
			char* s = (char*)333;
			strncpy(s, "abc", 12);
			break;
		}
	}

	LOGD(PTAG("test"),"end of moduleTest");

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif //CRASHSDKDEMO_TEST_H
