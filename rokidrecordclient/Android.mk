LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_CFLAGS := -std=c11 -Wall -Werror

LOCAL_SRC_FILES:= socket_client.c

LOCAL_MODULE:= micrecclient

#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils \
			liblog

include $(BUILD_EXECUTABLE)
