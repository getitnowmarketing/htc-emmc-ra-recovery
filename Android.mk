LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

commands_recovery_local_path := $(LOCAL_PATH)

ifneq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_ARCH),arm)

LOCAL_SRC_FILES := \
	recovery.c \
	bootloader.c \
	commands.c \
	extracommands.c \
	firmware.c \
	install.c \
	roots.c \
	verifier.c \
	getprop.c \
	setprop.c

ifeq ($(ENABLE_TOUCH_UI),true)       
LOCAL_SRC_FILES += ui_touch.c 
else
LOCAL_SRC_FILES += ui.c
endif

        
LOCAL_SRC_FILES += test_roots.c

LOCAL_MODULE := recovery

LOCAL_FORCE_STATIC_EXECUTABLE := true

RECOVERY_API_VERSION := 2
LOCAL_CFLAGS += -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)

ifeq ($(RECOVERY_UI_KEYS),)
LOCAL_CFLAGS += -DDEFAULT_RECOVERY_UI_KEYS
else
RECOVERY_UI_KEYS := $(strip $(RECOVERY_UI_KEYS))
LOCAL_CFLAGS += -D$(RECOVERY_UI_KEYS)
endif

ifeq ($(TARGET_USES_MTD),true)
LOCAL_CFLAGS += -DUSES_NAND_MTD
endif

ifeq ($(TARGET_HAS_WIMAX),true)
LOCAL_CFLAGS += -DHAS_WIMAX
endif

ifeq ($(TARGET_HAS_INTERNAL_SD),true)
LOCAL_CFLAGS += -DHAS_INTERNAL_SD
endif

ifeq ($(PARTITION_LAYOUT),)
LOCAL_CFLAGS += -DPARTITION_LAYOUT_DEFAULT
else
PARTITION_LAYOUT := $(strip $(PARTITION_LAYOUT))
LOCAL_CFLAGS += -D$(PARTITION_LAYOUT)
endif

ifeq ($(ENABLE_TOUCH_SCROLLING),true)
LOCAL_CFLAGS += -DUSE_TOUCH_SCROLLING
endif

ifeq ($(BOARD_LDPI_RECOVERY),true)
LOCAL_CFLAGS += -DBOARD_LDPI_RECOVERY
endif

ifeq ($(BOARD_XDPI_RECOVERY),true)
LOCAL_CFLAGS += -DBOARD_XDPI_RECOVERY
endif

ifeq ($(TARGET_NO_EXT4),true)
LOCAL_CFLAGS += -DKERNEL_NO_EXT4
endif

ifeq ($(NEEDS_LGE_FACT_RESET_6),true)
LOCAL_CFLAGS += -DLGE_RESET_BOOTMODE
endif

ifeq ($(TARGET_USES_CAF_QCOMM_MTD_RADIO),true)
LOCAL_CFLAGS += -DUSES_QCOMM_RADIO
endif

ifeq ($(DEBUG_MMC),true)
LOCAL_CFLAGS += -DMMC_PART_DEBUG
endif 

ifeq ($(KERNEL_FLASH_SON),true)
LOCAL_CFLAGS += -DHBOOT_SON_KERNEL
endif 

ifeq ($(RECOVERY_COLOR_SCHEME),)
LOCAL_CFLAGS += -DCM_THEME
else
RECOVERY_COLOR_SCHEME := $(strip $(RECOVERY_COLOR_SCHEME))
LOCAL_CFLAGS += -D$(RECOVERY_COLOR_SCHEME)
endif

ifeq ($(ACER_ICONIA),true)
LOCAL_CFLAGS += -DIS_ICONIA
LOCAL_CFLAGS += -DHAS_DATA_MEDIA_SDCARD
endif

ifeq ($(ENABLE_TOUCH_UI),true)
LOCAL_CFLAGS += -DTOUCH_UI
endif

# This binary is in the recovery ramdisk, which is otherwise a copy of root.
# It gets copied there in config/Makefile.  LOCAL_MODULE_TAGS suppresses
# a (redundant) copy of the binary in /system/bin for user builds.
# TODO: Build the ramdisk image in a more principled way.

LOCAL_MODULE_TAGS := eng

LOCAL_SRC_FILES += default_recovery_ui.c

LOCAL_STATIC_LIBRARIES := libminzip libunz libamend libmtdutils libmmcutils libmincrypt
LOCAL_STATIC_LIBRARIES += libminui libpixelflinger_static libpng libcutils
LOCAL_STATIC_LIBRARIES += libstdc++ libc  #libdump_image liberase_image libflash_image

ifeq ($(TARGET_USES_MTD),true)
LOCAL_STATIC_LIBRARIES += libdump_image liberase_image libflash_image
endif

# Specify a C-includable file containing the OTA public keys.
# This is built in config/Makefile.
# *** THIS IS A TOTAL HACK; EXECUTABLES MUST NOT CHANGE BETWEEN DIFFERENT
#     PRODUCTS/BUILD TYPES. ***
# TODO: make recovery read the keys from an external file.
#RECOVERY_INSTALL_OTA_KEYS_INC := \
#	$(call intermediates-dir-for,PACKAGING,ota_keys_inc)/keys.inc
# Let install.c say #include "keys.inc"
#LOCAL_C_INCLUDES += $(dir $(RECOVERY_INSTALL_OTA_KEYS_INC))

include $(BUILD_EXECUTABLE)

# Depend on the generated keys.inc containing the OTA public keys.
#$(intermediates)/install.o: $(RECOVERY_INSTALL_OTA_KEYS_INC)

include $(commands_recovery_local_path)/minui/Android.mk
include $(commands_recovery_local_path)/amend/Android.mk
include $(commands_recovery_local_path)/minzip/Android.mk
include $(commands_recovery_local_path)/mtdutils/Android.mk
include $(commands_recovery_local_path)/mmcutils/Android.mk
include $(commands_recovery_local_path)/tools/Android.mk
include $(commands_recovery_local_path)/edify/Android.mk
include $(commands_recovery_local_path)/updater/Android.mk
commands_recovery_local_path :=

endif   # TARGET_ARCH == arm
endif	# !TARGET_SIMULATOR

