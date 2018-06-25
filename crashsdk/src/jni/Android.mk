LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := crashsdk

#compile with c++
LOCAL_CFLAGS := -std=c++11

NDK_APP_DST_DIR := ../../libs/$(TARGET_ARCH_ABI)

#specify where to find header files.
LOCAL_C_INCLUDES := $(LOCAL_PATH)/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/utils

LOCAL_C_INCLUDES += $(LOCAL_PATH)/crash
LOCAL_C_INCLUDES += $(LOCAL_PATH)/crash/unwind

#specify source file need be compiled.
MY_SRC_LIST := $(wildcard $(LOCAL_PATH)/*.cpp)
MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/utils/*.cpp)

MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/*.cpp)

ifneq (,$(filter $(TARGET_ARCH_ABI),x86))
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/crash/unwind/x86
	MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/unwind/*.cpp)
	MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/unwind/x86/*.cpp)
endif

ifneq (,$(filter $(TARGET_ARCH_ABI),armeabi-v7a))
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/crash/unwind/arm
	MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/unwind/*.cpp)
	MY_SRC_LIST += $(wildcard $(LOCAL_PATH)/crash/unwind/arm/*.cpp)
endif


LOCAL_SRC_FILES := $(MY_SRC_LIST:$(LOCAL_PATH)/%=%)

#$(warning $(MY_SRC_LIST))

LOCAL_LDLIBS += -llog -ldl -landroid
include $(BUILD_SHARED_LIBRARY)	