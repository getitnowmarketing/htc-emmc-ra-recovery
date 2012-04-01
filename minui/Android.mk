LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := resources.c

ifeq ($(BOARD_USES_TWENTYFOUR_BIT_FB),true)
LOCAL_SRC_FILES += gfx.c
else
LOCAL_SRC_FILES += graphics.c
endif 

ifeq ($(ENABLE_TOUCH_UI),true)       
LOCAL_SRC_FILES += events_touch.c 
else
LOCAL_SRC_FILES += events.c
endif

LOCAL_C_INCLUDES +=\
    external/libpng\
    external/zlib

ifeq ($(BOARD_LDPI_RECOVERY),true)
LOCAL_CFLAGS += -DBOARD_LDPI_RECOVERY
endif

ifeq ($(BOARD_XDPI_RECOVERY),true)
LOCAL_CFLAGS += -DBOARD_XDPI_RECOVERY
endif

ifeq ($(ENABLE_TOUCH_SCROLLING),true)
LOCAL_CFLAGS += -DUSE_TOUCH_SCROLLING
endif

ifeq ($(ENABLE_TOUCH_UI),true)
LOCAL_CFLAGS += -DTOUCH_UI
endif

ifeq ($(BOARD_USES_THIRTYTWO_BIT_FB),true)
LOCAL_CFLAGS += -DTHIRTYTWO_BIT_FB
endif 

LOCAL_MODULE := libminui

include $(BUILD_STATIC_LIBRARY)
