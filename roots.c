/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

#include "mtdutils/mtdutils.h"
#include "mtdutils/mounts.h"
#include "minzip/Zip.h"
#include "roots.h"
#include "common.h"
#include "mmcutils/mmcutils.h"

#include "extracommands.h"
#include "define_roots.h"

/*
typedef struct {
    const char *name;
    const char *device;
    const char *device2;  // If the first one doesn't work (may be NULL)
    const char *partition_name;
    const char *mount_point;
    const char *filesystem;
    const char *filesystem_options;
    const char *type

} RootInfo;
*/

/* Canonical pointers.
xxx may just want to use enums
 */


static const char g_mtd_device[] = "@\0g_mtd_device";
static const char g_mmc_device[] = "@\0g_mmc_device";
static const char g_raw[] = "@\0g_raw";
static const char g_package_file[] = "@\0g_package_file";

static RootInfo g_roots_mtd[] = {
    { "BOOT:", g_mtd_device, NULL, "boot", NULL, g_raw, NULL, "mtd" },
    { "CACHE:", g_mtd_device, NULL, "cache", "/cache", "yaffs2", NULL, "mtd" },
    { "DATA:", g_mtd_device, NULL, "userdata", "/data", "yaffs2", NULL, "mtd" },
    { "MISC:", g_mtd_device, NULL, "misc", NULL, g_raw, NULL, "mtd" },
    { "PACKAGE:", NULL, NULL, NULL, NULL, g_package_file, NULL, NULL },
    { "RECOVERY:", g_mtd_device, NULL, "recovery", "/", g_raw, NULL, "mtd" },
    { "SDCARD:", "/dev/block/mmcblk0p1", "/dev/block/mmcblk0", "sdcard", "/sdcard", "vfat", NULL, NULL },
    { "SDEXT:", "/dev/block/mmcblk0p2", NULL, "sd-ext", "/sd-ext", "auto", NULL, NULL },
    { "SYSTEM:", g_mtd_device, NULL, "system", "/system", "yaffs2", NULL, "mtd" },
    { "MBM:", g_mtd_device, NULL, "mbm", NULL, g_raw, NULL, NULL },
#ifdef HAS_INTERNAL_SD 
   { "INTERNALSD:", INTERNALSDBLK, INTERNALSDBLK2, "internal_sdcard", "/internal_sdcard", "vfat", NULL, NULL },
#endif    
    { "TMP:", NULL, NULL, NULL, "/tmp", NULL, NULL, NULL },
};

static RootInfo g_roots_mmc[] = {
    { "BOOT:", BOOTBLK, NULL, "boot", NULL, g_raw, NULL, "emmc" },
    { "CACHE:", CACHEBLK, NULL, "cache", "/cache", "auto", NULL, "emmc" },
    { "DATA:", DATABLK, NULL, "userdata", "/data", "auto", NULL, "emmc" },
    { "MISC:", MISCBLK, NULL, "misc", NULL, g_raw, NULL, "emmc" },
    { "PACKAGE:", NULL, NULL, NULL, NULL, g_package_file, NULL, NULL },
    { "RECOVERY:", RECOVERYBLK, NULL, "recovery", "/", g_raw, NULL, "emmc" },
    { "SDCARD:", "/dev/block/mmcblk1p1", "/dev/block/mmcblk1", "sdcard", "/sdcard", "vfat", NULL, NULL },
    { "SDEXT:", "/dev/block/mmcblk1p2", NULL, "sd-ext", "/sd-ext", "auto", NULL, NULL },
    { "SYSTEM:", SYSTEMBLK, NULL, "system", "/system", "auto", NULL, "emmc" },
    { "MBM:", g_mmc_device, NULL, "mbm", NULL, g_raw, NULL, NULL },
    { "TMP:", NULL, NULL, NULL, "/tmp", NULL, NULL, NULL },
#ifdef HAS_INTERNAL_SD 
   { "INTERNALSD:", INTERNALSDBLK, INTERNALSDBLK2, "internal_sdcard", "/internal_sdcard", "vfat", NULL, NULL },
#endif
};

static RootInfo *g_roots = NULL;

static unsigned int NUM_ROOTS;

#define NUM_ROOTS_MMC (sizeof(g_roots_mmc) / sizeof(g_roots_mmc[0]))
#define NUM_ROOTS_MTD (sizeof(g_roots_mtd) / sizeof(g_roots_mtd[0]))

void
set_root_table()
{
    char emmc[64];
    property_get("ro.emmc", emmc, "");
    if(!strcmp(emmc, "1"))
    {
        g_roots = g_roots_mmc;
	NUM_ROOTS = NUM_ROOTS_MMC;
    }
    else
    {
        g_roots = g_roots_mtd;
	NUM_ROOTS = NUM_ROOTS_MTD;
    }
}

//#define NUM_ROOTS (sizeof(g_roots) / sizeof(g_roots[0]))

// TODO: for SDCARD:, try /dev/block/mmcblk0 if mmcblk0p1 fails

static const RootInfo *
get_root_info_for_path(const char *root_path)
{
    const char *c;

    /* Find the first colon.
     */
    c = root_path;
    while (*c != '\0' && *c != ':') {
        c++;
    }
    if (*c == '\0') {
        return NULL;
    }
    size_t len = c - root_path + 1;
    size_t i;
    for (i = 0; i < NUM_ROOTS; i++) {
        RootInfo *info = &g_roots[i];
        if (strncmp(info->name, root_path, len) == 0) {
            return info;
        }
    }
    return NULL;
}

static const ZipArchive *g_package = NULL;
static char *g_package_path = NULL;

int
register_package_root(const ZipArchive *package, const char *package_path)
{
    if (package != NULL) {
        package_path = strdup(package_path);
        if (package_path == NULL) {
            return -1;
        }
        g_package_path = (char *)package_path;
    } else {
        free(g_package_path);
        g_package_path = NULL;
    }
    g_package = package;
    return 0;
}

int
is_package_root_path(const char *root_path)
{
    const RootInfo *info = get_root_info_for_path(root_path);
    return info != NULL && info->filesystem == g_package_file;
}

const char *
translate_package_root_path(const char *root_path,
        char *out_buf, size_t out_buf_len, const ZipArchive **out_package)
{
    const RootInfo *info = get_root_info_for_path(root_path);
    if (info == NULL || info->filesystem != g_package_file) {
        return NULL;
    }

    /* Strip the package root off of the path.
     */
    size_t root_len = strlen(info->name);
    root_path += root_len;
    size_t root_path_len = strlen(root_path);

    if (out_buf_len < root_path_len + 1) {
        return NULL;
    }
    strcpy(out_buf, root_path);
    *out_package = g_package;
    return out_buf;
}

/* Takes a string like "SYSTEM:lib" and turns it into a string
 * like "/system/lib".  The translated path is put in out_buf,
 * and out_buf is returned if the translation succeeded.
 */
const char *
translate_root_path(const char *root_path, char *out_buf, size_t out_buf_len)
{
    if (out_buf_len < 1) {
        return NULL;
    }

    const RootInfo *info = get_root_info_for_path(root_path);
    if (info == NULL || info->mount_point == NULL) {
        return NULL;
    }

    /* Find the relative part of the non-root part of the path.
     */
    root_path += strlen(info->name);  // strip off the "root:"
    while (*root_path != '\0' && *root_path == '/') {
        root_path++;
    }

    size_t mp_len = strlen(info->mount_point);
    size_t rp_len = strlen(root_path);
    if (mp_len + 1 + rp_len + 1 > out_buf_len) {
        return NULL;
    }

    /* Glue the mount point to the relative part of the path.
     */
    memcpy(out_buf, info->mount_point, mp_len);
    if (out_buf[mp_len - 1] != '/') out_buf[mp_len++] = '/';

    memcpy(out_buf + mp_len, root_path, rp_len);
    out_buf[mp_len + rp_len] = '\0';

    return out_buf;
}

static int
internal_root_mounted(const RootInfo *info)
{
    if (info->mount_point == NULL) {
        return -1;
    }
//xxx if TMP: (or similar) just say "yes"

    /* See if this root is already mounted.
     */
    int ret = scan_mounted_volumes();
    if (ret < 0) {
        return ret;
    }
    const MountedVolume *volume;
    volume = find_mounted_volume_by_mount_point(info->mount_point);
    if (volume != NULL) {
        /* It's already mounted.
         */
        return 0;
    }
    return -1;
}

int
is_root_path_mounted(const char *root_path)
{
    const RootInfo *info = get_root_info_for_path(root_path);
    if (info == NULL) {
        return -1;
    }
    return internal_root_mounted(info) >= 0;
}

static int mount_internal(const char* device, const char* mount_point, const char* filesystem, const char* filesystem_options)
{
    if (strcmp(filesystem, "auto") != 0 && filesystem_options == NULL){
        return mount(device, mount_point, filesystem, MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
    } else {
        char mount_cmd[PATH_MAX];
        const char* options = filesystem_options == NULL ? "noatime,nodiratime,nodev" : filesystem_options;
        sprintf(mount_cmd, "mount -t %s -o%s %s %s", filesystem, options, device, mount_point);
        return __system(mount_cmd);
    }

}

int
ensure_root_path_mounted(const char *root_path)
{
    const RootInfo *info = get_root_info_for_path(root_path);
    if (info == NULL) {
        return -1;
    }

    int ret = internal_root_mounted(info);
    if (ret >= 0) {
        /* It's already mounted.
         */
        return 0;
    }

    /* It's not mounted.
     */
    if (info->device == g_mtd_device) {
        if (info->partition_name == NULL) {
            return -1;
        }
//TODO: make the mtd stuff scan once when it needs to
        mtd_scan_partitions();
        const MtdPartition *partition;
        partition = mtd_find_partition_by_name(info->partition_name);
        if (partition == NULL) {
            return -1;
        }
        return mtd_mount_partition(partition, info->mount_point,
                info->filesystem, 0);
    }
	if (info->device == g_mmc_device) {
        if (info->partition_name == NULL) {
            return -1;
        }
//TODO: make the mmc stuff scan once when it needs to
        mmc_scan_partitions();
        const MmcPartition *partition;
        partition = mmc_find_partition_by_name(info->partition_name);
        if (partition == NULL) {
            return -1;
        }
	if (!strcmp(info->filesystem, "ext3")) { 
        return mmc_mount_partition(partition, info->mount_point, 0);
	} else {
	return mount_internal(partition->device_index, info->mount_point, info->filesystem, info->filesystem_options);
        } 
    }

    if (info->device == NULL || info->mount_point == NULL ||
        info->filesystem == NULL ||
        info->filesystem == g_raw ||
        info->filesystem == g_package_file) {
        return -1;
    }

    mkdir(info->mount_point, 0755);  // in case it doesn't already exist
    if (mount_internal(info->device, info->mount_point, info->filesystem, info->filesystem_options)) {
        if (info->device2 == NULL) {
            LOGE("Can't mount %s\n(%s)\n", info->device, strerror(errno));
            return -1;
        } else if (mount(info->device2, info->mount_point, info->filesystem,
                MS_NOATIME | MS_NODEV | MS_NODIRATIME, "")) {
            LOGE("Can't mount %s (or %s)\n(%s)\n",
                    info->device, info->device2, strerror(errno));
            return -1;
        }
    }
    return 0;
}

int
ensure_root_path_unmounted(const char *root_path)
{
    const RootInfo *info = get_root_info_for_path(root_path);
    if (info == NULL) {
        return -1;
    }
    if (info->mount_point == NULL) {
        /* This root can't be mounted, so by definition it isn't.
         */
        return 0;
    }
//xxx if TMP: (or similar) just return error

    /* See if this root is already mounted.
     */
    int ret = scan_mounted_volumes();
    if (ret < 0) {
        return ret;
    }
    const MountedVolume *volume;
    volume = find_mounted_volume_by_mount_point(info->mount_point);
    if (volume == NULL) {
        /* It's not mounted.
         */
        return 0;
    }

    return unmount_mounted_volume(volume);
}

const MtdPartition *
get_root_mtd_partition(const char *root_path)
{
    const RootInfo *info = get_root_info_for_path(root_path);
    if (info == NULL || info->device != g_mtd_device ||
            info->partition_name == NULL)
    {
        return NULL;
    }
    mtd_scan_partitions();
    return mtd_find_partition_by_name(info->partition_name);
}

const MmcPartition *
get_root_mmc_partition(const char *root_path)
{
	const RootInfo *info = get_root_info_for_path(root_path);
	if (info == NULL || info->device != g_mmc_device ||
		info->partition_name == NULL)
	{
		return NULL;
	}
	mmc_scan_partitions();
	return mmc_find_partition_by_name(info->partition_name);
}

int
format_root_device(const char *root)
{
    /* Be a little safer here; require that "root" is just
     * a device with no relative path after it.
     */
    const char *c = root;
    while (*c != '\0' && *c != ':') {
        c++;
    }
    /*
    if (c[0] != ':' || c[1] != '\0') {
        LOGW("format_root_device: bad root name \"%s\"\n", root);
        return -1;
    }
    */

    const RootInfo *info = get_root_info_for_path(root);
    if (info == NULL || info->device == NULL) {
        LOGW("format_root_device: can't resolve \"%s\"\n", root);
        return -1;
    }
    if (info->mount_point != NULL && (info->device == g_mtd_device || info->device == g_mmc_device)) {
        /* Don't try to format a mounted device.
         */
        int ret = ensure_root_path_unmounted(root);
        if (ret < 0) {
            LOGW("format_root_device: can't unmount \"%s\"\n", root);
            return ret;
        }
    }

    /* Format the device.
     */
    if (info->device == g_mtd_device) {
        mtd_scan_partitions();
        const MtdPartition *partition;
        partition = mtd_find_partition_by_name(info->partition_name);
        if (partition == NULL) {
            LOGW("format_root_device: can't find mtd partition \"%s\"\n",
                    info->partition_name);
            return -1;
        }
        if (info->filesystem == g_raw || !strcmp(info->filesystem, "yaffs2")) {
            MtdWriteContext *write = mtd_write_partition(partition);
            if (write == NULL) {
                LOGW("format_root_device: can't open \"%s\"\n", root);
                return -1;
            } else if (mtd_erase_blocks(write, -1) == (off_t) -1) {
                LOGW("format_root_device: can't erase \"%s\"\n", root);
                mtd_write_close(write);
                return -1;
            } else if (mtd_write_close(write)) {
                LOGW("format_root_device: can't close \"%s\"\n", root);
                return -1;
            } else {
                return 0;
            }
        }
    }
   
	if (full_ext_format_enabled) {
	//Handle MMC devices not using g_mmc_device 
	if (info->device != g_mmc_device && info->device != g_mtd_device) {
		if (!strcmp(info->filesystem, "ext3") || !strcmp(info->filesystem, "ext4") || !strcmp(info->filesystem, "auto")) { 	
		return format_ext_device(root);
		}
	}

	//Handle MMC device types
   	 if(info->device == g_mmc_device) {
       	    mmc_scan_partitions();
       	    const MmcPartition *partition;
            partition = mmc_find_partition_by_name(info->partition_name);
        if (partition == NULL) {
            LOGE("format_root_device: can't find mmc partition \"%s\"\n",
                    info->partition_name);
            return -1;
        }
        
	if (!strcmp(info->filesystem, "ext3")) {
            mmc_format_ext3(partition);
               return 0;
	}
	
	if (!strcmp(info->filesystem, "ext4")) {
            mmc_format_ext4(partition);
		return 0;
        }
	
	if (!strcmp(info->filesystem, "auto")) {
		call_format_ext(root);
		return 0;
        }
	  
    }
  }
	/* Format raw */
	if (!strcmp(info->type, "emmc")) {
	if(info->device == g_mmc_device) {
       	    mmc_scan_partitions();
       	    const MmcPartition *partition;
            partition = mmc_find_partition_by_name(info->partition_name);
        if (partition == NULL) {
          if (!strcmp(info->partition_name, "boot")) {
		  return format_raw_partition(root);
       }
	}
	if (!strcmp(partition->name, "boot")) {
		return format_raw_partition(root);
	}
}
}


    return format_non_mtd_device(root);
}


const RootInfo *get_device_info(const char *root)
{
const char *c;

    /* Find the first colon.
     */
    c = root;
    while (*c != '\0' && *c != ':') {
        c++;
    }
    if (*c == '\0') {
        return NULL;
    }
    size_t len = c - root + 1;
    size_t i;
    for (i = 0; i < NUM_ROOTS; i++) {
        RootInfo *info = &g_roots[i];
        if (strncmp(info->name, root, len) == 0) {
            return info;
        }
    }
    return NULL;
}

int get_device_index(const char *root, char *device)
{
	const RootInfo *info = get_root_info_for_path(root);
    if (info == NULL) {
		return -1;
		}
	
    if (info->device == g_mtd_device) {
        if (info->partition_name == NULL) {
            return -1;
        }
        		        
	return mtd_get_partition_device(info->partition_name, device);
    }

    if (info->device == g_mmc_device) {
        if (info->partition_name == NULL) {
            return -1;
        }

        mmc_scan_partitions();
        const MmcPartition *partition;
        partition = mmc_find_partition_by_name(info->partition_name);
        if (partition == NULL) {
            return -1;
        }

	strcpy(device, partition->device_index);
	return 0;
     }

    if (info->device != g_mtd_device && info->device != g_mmc_device && info->device != NULL) {
	strcpy(device, info->device);
	return 0;
    } else {
	return -1;
	}
}


	
	
