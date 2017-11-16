LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES:= system/extras/rokidrecordserver/include
LOCAL_SRC_FILES:= mixer.c pcm.c pipe.c pipe_util.c
LOCAL_MODULE := librobotalsa
LOCAL_SHARED_LIBRARIES:= libcutils libutils
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 
LOCAL_C_INCLUDES:= system/extras/rokidrecordserver/include
LOCAL_SRC_FILES:= robotcap.c main.c
LOCAL_MODULE := robotcap
LOCAL_SHARED_LIBRARIES:= libcutils libutils librobotalsa
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES:= system/extras/rokidrecordserver/include
LOCAL_SRC_FILES:= server.c
LOCAL_MODULE := mictoolservertest
LOCAL_SHARED_LIBRARIES:= libcutils libutils librobotalsa
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
