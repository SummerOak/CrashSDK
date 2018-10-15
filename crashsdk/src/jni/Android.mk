LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_PLATFORM), android-14)
    LOCAL_MODULE    := crashsdk
else
	LOCAL_MODULE    := crashsdk21
endif

#compile with c++
LOCAL_CFLAGS := -std=c++11

NDK_APP_DST_DIR := ../../libs/$(TARGET_ARCH_ABI)

#specify where to find header files.
LOCAL_C_INCLUDES := $(LOCAL_PATH)/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/utils

LOCAL_C_INCLUDES += $(LOCAL_PATH)/crash
LOCAL_C_INCLUDES += $(LOCAL_PATH)/crash/unwind
LOCAL_C_INCLUDES += $(LOCAL_PATH)/crash/unwind/eh_frame

#specify source file need be compiled.
MY_SRC_LIST := $(wildcard $(LOCAL_PATH)/*.cpp)
MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/utils/*.cpp)

MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/*.cpp)

ifneq (,$(filter $(TARGET_ARCH_ABI),x86))
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/crash/unwind/x86
	MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/unwind/*.cpp)
	MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/unwind/x86/*.cpp)
	MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/unwind/eh_frame/*.cpp)
endif

ifneq (,$(filter $(TARGET_ARCH_ABI),armeabi-v7a))
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/crash/unwind/arm
	MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/unwind/*.cpp)
	MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/unwind/arm/*.cpp)
	
	ifneq ($(TARGET_PLATFORM), android-14)
    	MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/unwind/eh_frame/*.cpp)
	endif
	
endif


LOCAL_SRC_FILES := $(MY_SRC_LIST:$(LOCAL_PATH)/%=%)

#$(warning $(MY_SRC_LIST))

LOCAL_LDLIBS += -llog -landroid
include $(BUILD_SHARED_LIBRARY)	