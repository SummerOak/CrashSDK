//
// Created by Summer on 09/06/2018.
//

#ifndef CRASHSDKDEMO_TEST_H
#define CRASHSDKDEMO_TEST_H

#include <jni.h>
#include <string.h>
#include "Defines.h"
#include "Log.h"

#ifdef __cplusplus
extern "C" {
#endif

void __attribute__((noinline)) a(int aa){
	LOGE(PTAG("test"),"a %d", aa);

	int arr[2000];
	memset(arr,0,sizeof(arr));
	for(int i=0;i<1000;i++){
		arr[i] = i;
	}

	a(arr[aa] + 1);
}

JNIEXPORT jint JNICALL
Java_com_summer_crashsdk_CrashSDK_test(JNIEnv *env,jobject obj)
{
	LOGI(PTAG("test"),"moduleTest");


	a(32);

	LOGD(PTAG("test"),"end of moduleTest");

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif //CRASHSDKDEMO_TEST_H
