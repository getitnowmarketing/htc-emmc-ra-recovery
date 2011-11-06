LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := graphics.c events.c resources.c

LOCAL_C_INCLUDES +=\
    external/libpng\
    external/zlib

ifeq ($(BOARD_LDPI_RECOVERY),true)
LOCAL_CFLAGS += -DBOARD_LDPI_RECOVERY
endif

ifeq ($(BOARD_USES_THIRTYTWO_BIT_FB),true)
LOCAL_CFLAGS += -DTHIRTYTWO_BIT_FB
endif 

LOCAL_MODULE := libminui

include $(BUILD_STATIC_LIBRARY)
