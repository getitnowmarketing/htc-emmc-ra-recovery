ifneq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mmcutils.c

ifeq ($(LGE_MMC_TYPES),true)
LOCAL_CFLAGS += -DUSE_LGE_DTYPES
endif

LOCAL_MODULE := libmmcutils

include $(BUILD_STATIC_LIBRARY)

endif	# TARGET_ARCH == arm
endif	# !TARGET_SIMULATOR
