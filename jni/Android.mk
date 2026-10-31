LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := memory
LOCAL_SRC_FILES := native-lib.cpp $(wildcard $(LOCAL_PATH)/KittyMemory/*.cpp)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/KittyMemory
LOCAL_CPPFLAGS := -std=c++17 -DkNO_KEYSTONE
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)
