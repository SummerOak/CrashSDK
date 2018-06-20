#ifndef LOG_C
#define LOG_C

#include <android/log.h>

#ifdef DEBUG
#define LOGE(TAG,fmt...)	Log(0,ANDROID_LOG_ERROR,TAG,fmt);
#define LOGR(TAG,fmt...)	Log(1,ANDROID_LOG_INFO,TAG,fmt);
#define LOGW(TAG,fmt...) 	Log(2,ANDROID_LOG_WARN,TAG,fmt);
#define LOGI(TAG,fmt...)	Log(3,ANDROID_LOG_INFO,TAG,fmt);
#define LOGD(TAG,fmt...)	Log(4,ANDROID_LOG_DEBUG,TAG,fmt);
#define LOGT(TAG,fmt...)	Log(4,ANDROID_LOG_INFO,TAG,fmt);
#else
#define LOGE(TAG,fmt...)	Log(0,ANDROID_LOG_ERROR,TAG,fmt);
#define LOGR(TAG,fmt...)	Log(1,ANDROID_LOG_INFO,TAG,fmt);
#define LOGW(TAG,fmt...) 	//Log(2,ANDROID_LOG_WARN,TAG,fmt);
#define LOGI(TAG,fmt...)	//Log(3,ANDROID_LOG_INFO,TAG,fmt);
#define LOGD(TAG,fmt...)	//Log(4,ANDROID_LOG_DEBUG,TAG,fmt);
#define LOGT(TAG,fmt...)	//Log(4,ANDROID_LOG_INFO,TAG,fmt);
#endif

static void Log(int levl,int androidLevl,const char* TAG,const char* fmt, ...){

	va_list argptr;
    va_start(argptr, fmt);
    __android_log_vprint(androidLevl, TAG, fmt, argptr);
    va_end(argptr);
	
}


#endif
