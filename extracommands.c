/* Getitnowmarketing
This was taken and modified from Koush's extendedcommands.c 
http://github.com/koush/android_bootable_recovery
To handle formatting non yaffs2 partitions like the ext3 /data & /cache on Incredible
*/
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "amend/commands.h"
#include "commands.h"
#include "common.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include "firmware.h"
#include "minzip/DirUtil.h"
#include "minzip/Zip.h"
#include "roots.h"

#include "extracommands.h"
#include <signal.h>

#include <ctype.h>

#include <getopt.h>

#include <linux/input.h>

#include <dirent.h>

#include <sys/reboot.h>

#include <time.h>

#include <termios.h> 

#include "bootloader.h"
#include "install.h"
#include "minui/minui.h"

#include <sys/limits.h>

#include "recovery_ui_keys.h"
#include "mmcutils/mmcutils.h"

//disable this, its optional
int signature_check_enabled = 0;

void toggle_signature_check()
{
    signature_check_enabled = !signature_check_enabled;
    ui_print("Signature Check: %s\n", signature_check_enabled ? "Enabled" : "Disabled");
    if (signature_check_enabled == 0)  ui_print("Flashing unsigned zips may corrupt your system!\n");
}

int full_ext_format_enabled = 0;

void toggle_full_ext_format()
{
    full_ext_format_enabled = !full_ext_format_enabled;
    ui_print("Full Format Ext3/4: %s\n", full_ext_format_enabled ? "Enabled" : "Disabled");
    if (full_ext_format_enabled == 1)  ui_print("Full mke2fs format of ext3-ext4 enabled!\n");
}

void key_logger_test()
{
		//finish_recovery(NULL);
    		//ui_reset_progress();
		
		ui_print("Outputting key codes.\n");
                ui_print("Go back to end debugging.\n");
		
		for (;;) {
		int key = ui_wait_key();
                int action_key = device_handle_key(key, 1);
    		if (action_key == GO_BACK) {
                   break;
               
		} else  {   
		   ui_print("Key: %d\n", key);
                }
           }               			
}

void run_script(char *str1,char *str2,char *str3,char *str4,char *str5,char *str6,char *str7)
{
	ui_print(str1);
        ui_clear_key_queue();
	ui_print("\nPress %s to confirm,", CONFIRM);
       	ui_print("\nany other key to abort.\n");
	int confirm = ui_wait_key();
	int action_confirm_rs = device_handle_key(confirm, 1);
    				if (action_confirm_rs == SELECT_ITEM) {
                	ui_print(str2);
		        pid_t pid = fork();
                	if (pid == 0) {
                		char *args[] = { "/sbin/sh", "-c", str3, "1>&2", NULL };
                	        execv("/sbin/sh", args);
                	        fprintf(stderr, str4, strerror(errno));
                	        _exit(-1);
                	}
			int status;
			while (waitpid(pid, &status, WNOHANG) == 0) {
				ui_print(".");
               		        sleep(1);
			}
                	ui_print("\n");
			if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
                		ui_print(str5);
                	} else {
                		ui_print(str6);
                	}
		} else {
	       		ui_print(str7);
       	        }
		if (!ui_text_visible()) return;
}


// This was pulled from bionic: The default system command always looks
// for shell in /system/bin/sh. This is bad.
#define _PATH_BSHELL "/sbin/sh"

extern char **environ;
int
__system(const char *command)
{
  pid_t pid;
    sig_t intsave, quitsave;
    sigset_t mask, omask;
    int pstat;
    char *argp[] = {"sh", "-c", NULL, NULL};

    if (!command)        /* just checking... */
        return(1);

    argp[2] = (char *)command;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &omask);
    switch (pid = vfork()) {
    case -1:            /* error */
        sigprocmask(SIG_SETMASK, &omask, NULL);
        return(-1);
    case 0:                /* child */
        sigprocmask(SIG_SETMASK, &omask, NULL);
        execve(_PATH_BSHELL, argp, environ);
    _exit(127);
  }

    intsave = (sig_t)  bsd_signal(SIGINT, SIG_IGN);
    quitsave = (sig_t) bsd_signal(SIGQUIT, SIG_IGN);
    pid = waitpid(pid, (int *)&pstat, 0);
    sigprocmask(SIG_SETMASK, &omask, NULL);
    (void)bsd_signal(SIGINT, intsave);
    (void)bsd_signal(SIGQUIT, quitsave);
    return (pid == -1 ? -1 : pstat);
}

int format_non_mtd_device(const char* root)
{
    // if this is SDEXT:, don't worry about it.
    if (0 == strcmp(root, "SDEXT:"))
    {
		char device_sdext[PATH_MAX];
    		get_device_index("SDEXT:", device_sdext);
	
        struct stat st;
        if (0 != stat(device_sdext, &st))
        {
            ui_print("No app2sd partition found. Skipping format of /sd-ext.\n");
            return 0;
        }
    }

    char path[PATH_MAX];
    translate_root_path(root, path, PATH_MAX);
    if (0 != ensure_root_path_mounted(root))
    {
        ui_print("Error mounting %s!\n", path);
        ui_print("Skipping format...\n");
        return 0;
    }

    static char tmp[PATH_MAX];
#ifdef HAS_DATA_MEDIA_SDCARD
    if (strcmp(root, "DATA:") == 0) {
        sprintf(tmp, "cd /data ; for f in $(ls -a | grep -v ^media$); do rm -rf $f; done");
        __system(tmp);
    }
    else {
    sprintf(tmp, "rm -rf %s/*", path);
    __system(tmp);
    sprintf(tmp, "rm -rf %s/.*", path);
    __system(tmp);
    }
#else
    sprintf(tmp, "rm -rf %s/*", path);
    __system(tmp);
    sprintf(tmp, "rm -rf %s/.*", path);
    __system(tmp);
#endif

    ensure_root_path_unmounted(root);
    return 0;
}

void usb_toggle_sdcard()
{
	ui_print("\nEnabling USB-MS : ");
		        pid_t pid = fork();
                	if (pid == 0) {
                		char *args[] = { "/sbin/sh", "-c", "/sbin/ums_toggle on", "1>&2", NULL };
                	        execv("/sbin/sh", args);
                	        fprintf(stderr, "\nUnable to enable USB-MS!\n(%s)\n", strerror(errno));
                	        _exit(-1);
                	}
			int status;
			while (waitpid(pid, &status, WNOHANG) == 0) {
				ui_print(".");
               		        sleep(1);
			}
                	ui_print("\n");
			if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
                		ui_print("\nError : Run 'ums_toggle' via adb!\n\n");
                	} else {
                                ui_clear_key_queue();
                		ui_print("\nUSB-MS enabled!");
				ui_print("\nPress %s to disable,", CONFIRM);
				ui_print("\nand return to menu\n");
		       		for (;;) {
        	                        	int key = ui_wait_key();
						int action_key = device_handle_key(key, 1);
    						if (action_key == SELECT_ITEM) {
							ui_print("\nDisabling USB-MS : ");
						        pid_t pid = fork();
				                	if (pid == 0) {
				                		char *args[] = { "/sbin/sh", "-c", "/sbin/ums_toggle off", "1>&2", NULL };
                					        execv("/sbin/sh", args);
				                	        fprintf(stderr, "\nUnable to disable USB-MS!\n(%s)\n", strerror(errno));
				                	        _exit(-1);
				                	}
							int status;
							while (waitpid(pid, &status, WNOHANG) == 0) {
								ui_print(".");
				               		        sleep(1);
							}
				                	ui_print("\n");
							if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
				                		ui_print("\nError : Run 'ums_toggle' via adb!\n\n");
				                	} else {
				                		ui_print("\nUSB-MS disabled!\n\n");
							}	
							break;
					        }
				} 
                	}
		}	

#ifdef HAS_INTERNAL_SD
void usb_toggle_internal()
{
		ui_print("\nEnabling USB-MS : ");
		        pid_t pid1 = fork();
                	if (pid1 == 0) {
                		char *args[] = { "/sbin/sh", "-c", "/sbin/ums_toggle internal", "1>&2", NULL };
                	        execv("/sbin/sh", args);
                	        fprintf(stderr, "\nUnable to enable USB-MS!\n(%s)\n", strerror(errno));
                	        _exit(-1);
                	}
			int status1;
			while (waitpid(pid1, &status1, WNOHANG) == 0) {
				ui_print(".");
               		        sleep(1);
			}
                	ui_print("\n");
			if (!WIFEXITED(status1) || (WEXITSTATUS(status1) != 0)) {
                		ui_print("\nError : Run 'ums_toggle' via adb!\n\n");
                	} else {
                                ui_clear_key_queue();
                		ui_print("\nUSB-MS enabled!");
				ui_print("\nPress %s to disable,", CONFIRM);
				ui_print("\nand return to menu\n");
		       		for (;;) {
        	                        	int key = ui_wait_key();
						int action_key = device_handle_key(key, 1);
    						if (action_key == SELECT_ITEM) {
							ui_print("\nDisabling USB-MS : ");
						        pid_t pid1 = fork();
				                	if (pid1 == 0) {
				                		char *args[] = { "/sbin/sh", "-c", "/sbin/ums_toggle off", "1>&2", NULL };
                					        execv("/sbin/sh", args);
				                	        fprintf(stderr, "\nUnable to disable USB-MS!\n(%s)\n", strerror(errno));
				                	        _exit(-1);
				                	}
							int status1;
							while (waitpid(pid1, &status1, WNOHANG) == 0) {
								ui_print(".");
				               		        sleep(1);
							}
				                	ui_print("\n");
							if (!WIFEXITED(status1) || (WEXITSTATUS(status1) != 0)) {
				                		ui_print("\nError : Run 'ums_toggle' via adb!\n\n");
				                	} else {
				                		ui_print("\nUSB-MS disabled!\n\n");
							}	
							break;
					        }
				} 
             }
   }	
#endif

void wipe_battery_stats()
{
    ensure_root_path_mounted("DATA:");
    remove("/data/system/batterystats.bin");
    ensure_root_path_unmounted("DATA:");
}

void wipe_rotate_settings()
{
    ensure_root_path_mounted("DATA:");
    __system("rm -r /data/misc/akmd*");
    __system("rm -r /data/misc/rild*");    
    ensure_root_path_unmounted("DATA:");
}     

void make_clockwork_path()
{
    ensure_root_path_mounted("SDCARD:");
    __system("mkdir -p /sdcard/clockworkmod/backup");
//    ensure_root_path_unmounted("SDCARD:");
} 


void check_my_battery_level()
{
	
    char cap_s[4];
    
    FILE * cap = fopen("/sys/class/power_supply/battery/capacity","r");
    fgets(cap_s, 4, cap);
    fclose(cap);

    ui_print("\nBattery Level: %s%%\n\n", cap_s);
}

#ifndef USES_NAND_MTD
void check_fs() {
        
	ensure_root_path_mounted("SYSTEM:");
	ensure_root_path_mounted("DATA:");
	ensure_root_path_mounted("CACHE:");
#ifdef IS_ICONIA
	ensure_root_path_mounted("FLEXROM:");
#endif

	static char discard[1024];
        char device[64], name[64], type[64];
        FILE *mounts = fopen("/proc/mounts", "r");
 	
        while (fscanf(mounts, "%64s %64s %64s %1024[^\n]", device, name, type, discard) != EOF) {
                /* Enjoy the whitespace! */
                if (
                        !strcmp(name, "/data") ||
                        !strcmp(name, "/system") ||
#ifdef IS_ICONIA
			!strcmp(name, "/flexrom") ||
                        !strcmp(name, "/cache") 
#else
			!strcmp(name, "/cache") 
#endif
                 //       !strcmp(name, "/proc")
                )
                       
			 /* Only prints if filter matches */
                        ui_print("name: %s; type: %s\n", name, type);
  	}

	ensure_root_path_unmounted("SYSTEM:");
	ensure_root_path_unmounted("DATA:");
	ensure_root_path_unmounted("CACHE:");
#ifdef IS_ICONIA
	ensure_root_path_unmounted("FLEXROM:");
#endif
	fclose(mounts);
}
#endif



int dump_device(const char *root)
{

	const RootInfo* info = get_device_info(root);
		if (info == NULL || info->device == NULL) {
		return -1;
  	}

	if(!strcmp (info->type, "emmc")) {
       	    mmc_scan_partitions();
       	    const MmcPartition *partition;
            partition = mmc_find_partition_by_name(info->partition_name);
		if (partition != NULL) {
					
			static char dump[PATH_MAX];
			sprintf(dump, "dd if=%s of=/tmp/mkboot/%s.img bs=4096", partition->device_index, info->partition_name);
			__system(dump);
			LOGW("dump cmd is %s\n", dump);
			return 0;
			
			} else {

			static char dump[PATH_MAX];
			sprintf(dump, "dd if=%s of=/tmp/mkboot/%s.img bs=4096", info->device, info->partition_name);
			__system(dump);
			LOGW("dump cmd is %s\n", dump);
			return 0;
			}	
		}

	if(!strcmp (info->type, "mtd")) {
		static char dump[PATH_MAX];
		sprintf(dump, "dump_image %s /tmp/mkboot/%s.img", info->partition_name, info->partition_name);
		__system(dump);
		LOGW("dump cmd is %s\n", dump);
		return 0;
	}
	 return -1;	
}

void unpack_boot()
{
	__system("unpackbootimg -i /tmp/mkboot/boot.img -o /tmp/mkboot");
	__system("mkbootimg.sh");
#ifdef USES_NAND_MTD
	__system("flash_image boot /tmp/mkboot/newboot.img");
#else
	char boot_device[PATH_MAX];
	char fb_cmd[PATH_MAX];
	property_get("ro.boot.block", boot_device, "");
	if(!strcmp(boot_device, ""))
    		 { 
		  LOGE("Error getting boot device\n");
		  return;
		 }
	sprintf(fb_cmd, "dd if=/tmp/mkboot/newboot.img of=%s bs=4096", boot_device);
	__system(fb_cmd);
#endif		  
	sync();
}

#ifdef HBOOT_SON_KERNEL
void unpack_boot_hbootzip()
{
	__system("unpackbootimg -i /tmp/mkboot/boot.img -o /tmp/mkboot");
	__system("mkbootimg.sh");
}
#endif	

void setup_mkboot()
{
	ensure_root_path_mounted("SDCARD:");
   	__system("mkdir -p /sdcard/mkboot");
    	__system("mkdir -p /sdcard/mkboot/zImage");
    	__system("mkdir -p /sdcard/mkboot/modules");
        __system("rm -rf /tmp/mkboot");
    	__system("mkdir -p /tmp/mkboot");
    	__system("chmod 0755 /tmp/mkboot/");
}

#ifdef HBOOT_SON_KERNEL
void setup_hbootzip()
{
	ensure_root_path_mounted("SDCARD:");
	__system("mkdir -p /sdcard/mkboot");
    	__system("mkdir -p /sdcard/mkboot/zImage");
    	__system("mkdir -p /sdcard/mkboot/modules");
        __system("rm -rf /tmp/mkboot");
    	__system("mkdir -p /tmp/mkboot");
    	__system("chmod 0755 /tmp/mkboot/");
	__system("mkdir -p /sdcard/mkboot/androidinfo");
}
	
#endif

int check_file_exists(const char* file_path)
{
	struct stat st;

	if (0 != stat(file_path, &st)) {
		LOGW("Error %s doesn't exist\n", file_path);
		return -1;
	} else {
		return 0;
	}
}

int is_dir(const char* file_path)
/* dir ret 0, file ret 1, err ret -1 */
{
	if (0 == (check_file_exists(file_path))) {
		struct stat s;
		stat(file_path, &s);

	if (!(S_ISDIR(s.st_mode))) {
		return 0;

	} else if (!(S_ISREG(s.st_mode))) {
		return 1;
	} else {
		return -1;
	}

	}

	return -1;
}


int copy_file(const char* source, const char* dest)
{
/* need to add a check to see if dest dir exists and volume is mounted */

	if (0 == (is_dir(source))) {
		char copy[PATH_MAX];
		sprintf(copy, "cp -r %s %s", source, dest);
		__system(copy);
		return 0;
	}

	if (1 == (is_dir(source))) {
		char copy[PATH_MAX];
		sprintf(copy, "cp %s %s", source, dest);
		__system(copy);
		return 0;
	}

return 1;
}

void do_module()
{
	ensure_root_path_mounted("SYSTEM:");
	ensure_root_path_mounted("SDCARD:");
	__system("rm -rf /system/lib/modules");
        __system("cp -r /sdcard/mkboot/modules /system/lib/modules");
	__system("chmod 0644 /system/lib/modules/*");
	ensure_root_path_unmounted("SYSTEM:");
	//ensure_root_path_unmounted("SDCARD:");
}

#ifdef HBOOT_SON_KERNEL
void make_hboot_zip()
{
	ensure_root_path_mounted("SDCARD:");
	char htcmodelid[20];
	char zipid[5];
    	property_get("ro.htcmodelid", htcmodelid, "");
	//LOGE("htcmodelid is %s\n", htcmodelid);
	strncpy (zipid, htcmodelid, 4);
	zipid[4]='\0';
	puts (zipid);
	//LOGE("zipid is: %s\n", zipid);
	char zippkg[PATH_MAX];
	sprintf(zippkg, "/sbin/zip -j /tmp/mkboot/%sIMG /tmp/mkboot/android-info.txt /tmp/mkboot/boot.img", zipid);
	//LOGE("zip command is : %s\n", zippkg);
	__system(zippkg);
	char zipcopy[PATH_MAX];
	sprintf(zipcopy, "cp /tmp/mkboot/%sIMG.zip /sdcard/%sIMG.zip", zipid, zipid);
	__system(zipcopy);
	//LOGE("zipcopy command is: %s\n", zipcopy);
}

void delete_hboot_zip()
{
	ensure_root_path_mounted("SDCARD:");
	char htcmodelid[20];
	char zipid[5];
    	property_get("ro.htcmodelid", htcmodelid, "");
	strncpy (zipid, htcmodelid, 4);
	zipid[4]='\0';
	puts (zipid);
	char zippkg_del[PATH_MAX];
	sprintf(zippkg_del, "rm /sdcard/%sIMG.zip", zipid);
	__system(zippkg_del);
}
#endif

void do_make_new_boot()
{
	setup_mkboot();
	ui_print("\nConnect phone to pc");
	ui_print("\nand copy new zImage and");
	ui_print("\nmodules to /sdcard/mkboot");
	ui_print("\nzImage & modules folder\n\n");
	usb_toggle_sdcard();
	ensure_root_path_mounted("SDCARD:");
	dump_device("BOOT:");

	if (0 == (copy_file("/sdcard/mkboot/zImage/zImage", "/tmp/mkboot/zImage"))) {
		unpack_boot();
		do_module();
		ui_print("New boot created and flashed!!\n\n");
	} else {
		ui_print("Error missing /sdcard/mkboot/zImage/zImage\n\n");
	}

	__system("rm -rf /tmp/mkboot");
	__system("rm /sdcard/mkboot/zImage/*");
   	__system("rm /sdcard/mkboot/modules/*");
	ensure_root_path_unmounted("SDCARD:");
}

#ifdef HBOOT_SON_KERNEL
int do_make_new_hbootbootzip()
{
	setup_hbootzip();
	ui_print("\nConnect phone to pc");
	ui_print("\nand copy new zImage and");
	ui_print("\nmodules to /sdcard/mkboot");
	ui_print("\nzImage & modules folder\n");
	ui_print("\nCopy android-info.txt to");
	ui_print("\n/sdcard/mkboot/androidinfo folder\n\n");
	usb_toggle_sdcard();
	ensure_root_path_mounted("SDCARD:");
	dump_device("BOOT:");

	if (0 == (copy_file("/sdcard/mkboot/zImage/zImage", "/tmp/mkboot/zImage"))) {
		unpack_boot_hbootzip();
		do_module();
		ui_print("New boot created & Kernel modules installed!\n");
	} else {
		ui_print("Error missing /sdcard/mkboot/zImage/zImage\n\n");
		return -1;
	}
	
	if (0 == (copy_file("/sdcard/mkboot/androidinfo/android-info.txt", "/tmp/mkboot/android-info.txt"))) {
		__system("rm /tmp/mkboot/boot.img");
		__system("mv /tmp/mkboot/newboot.img /tmp/mkboot/boot.img");
		make_hboot_zip();
		ui_print("Hboot kernel zip created and copied to root of sdcard.\n");
		ui_print("Rebooting to bootloader for install!!!!\n\n");
	} else {
		ui_print("Error missing /sdcard/mkboot/androidinfo/android-info.txt\n\n");
		return -1;
	}
	__system("rm -rf /tmp/mkboot");
	__system("rm /sdcard/mkboot/zImage/*");
   	__system("rm /sdcard/mkboot/modules/*");
	sync();
	rb_bootloader();
	return 0;
}

int do_make_new_hbootbootzip_auto()
{

	char manufacturer[64];
    	property_get("ro.product.manufacturer", manufacturer, "");
	if(strcmp(manufacturer, "HTC"))
    		 {
			ui_print("Error Non HTC phone detected!!!\n\n");
			return -1;
		 }

	setup_hbootzip();
	ensure_root_path_mounted("SDCARD:");
	dump_device("BOOT:");

	if (0 == (copy_file("/sdcard/mkboot/zImage/zImage", "/tmp/mkboot/zImage"))) {
		unpack_boot_hbootzip();
		do_module();
		ui_print("New boot created & kernel modules installed!\n");
	} else {
		ui_print("Error missing /sdcard/mkboot/zImage/zImage\n\n");
		return -1;
	}
	
	if (0 == (copy_file("/sdcard/mkboot/androidinfo/android-info.txt", "/tmp/mkboot/android-info.txt"))) {
		__system("rm /tmp/mkboot/boot.img");
		__system("mv /tmp/mkboot/newboot.img /tmp/mkboot/boot.img");
		make_hboot_zip();
		ui_print("Hboot kernel zip created and copied to root of sdcard.\n");
		ui_print("Rebooting to bootloader for install!!!!\n\n");
	} else {
		ui_print("Error missing /sdcard/mkboot/androidinfo/android-info.txt\n\n");
		return -1;
	}
	__system("rm -rf /tmp/mkboot");
        __system("rm /sdcard/mkboot/zImage/*");
   	__system("rm /sdcard/mkboot/modules/*");
	sync();
	rb_bootloader();
	return 0;
}

#endif

void install_su(int eng_su)
{
	ui_print("Working ......\n");
	ensure_root_path_mounted("SYSTEM:");
	ensure_root_path_mounted("DATA:");
	ensure_root_path_mounted("CACHE:");

	char device_sdext[PATH_MAX];
    	get_device_index("SDEXT:", device_sdext);	

	struct stat sd;
        	if (0 == stat(device_sdext, &sd)) {
		ensure_root_path_mounted("SDEXT:");
		__system("rm /sd-ext/dalvik-cache/*com.noshufou.android.su*classes.dex");
		__system("rm -rf /sd-ext/data/com.noshufou.android.su");
		__system("rm /sd-ext/app/com.noshufou.android.su*.apk");
		__system("rm /sd-ext/dalvik-cache/*uperuser*classes.dex");
		ensure_root_path_unmounted("SDEXT:");
	}

	__system("rm -rf /data/data/com.noshufou.android.su");
	__system("rm /data/app/com.noshufou.android.su*.apk");
	__system("rm /data/dalvik-cache/*com.noshufou.android.su*classes.dex");
	__system("rm /data/dalvik-cache/*uperuser*classes.dex");

	__system("rm /cache/dalvik-cache/*com.noshufou.android.su*classes.dex");
	__system("rm /cache/dalvik-cache/*uperuser*classes.dex");

	__system("rm /system/app/*uperuser.apk");

	if ((0 == (check_file_exists("/system/bin/su"))) || (0 == (check_file_exists("/system/xbin/su"))) ){
		ui_print("Removing old su\n");
	}
	
	delete_file("/system/bin/su");
	__system("rm /system/xbin/su");

	if (!eng_su) {
		copy_file("/extra/su", "/system/bin/su");
		copy_file("/extra/Superuser.apk", "/system/app/Superuser.apk");
		__system("chmod 0644 /system/app/Superuser.apk");
	} else {
		copy_file("/extra/suhack", "/system/bin/su");
	}

	__system("mkdir -p /system/xbin");
	__system("chmod 06755 /system/bin/su");
	__system("ln -s /system/bin/su /system/xbin/su");
	ensure_root_path_unmounted("DATA:");
	ensure_root_path_unmounted("SYSTEM:");
	ensure_root_path_unmounted("CACHE:");
	ui_print("su install complete\n\n");
}

int delete_file(const char* file)
{
/* need to add a check to see if volume is mounted */

if (0 == (is_dir(file))) {
	char del[PATH_MAX];
	sprintf(del, "rm -rf %s ", file);
	__system(del);
	return 0;
	}

if (1 == (is_dir(file))) {
	char del[PATH_MAX];
	sprintf(del, "rm %s ", file);
	__system(del);
	return 0;
	}

return 1;
}

void rb_bootloader()
{
	sync();
	ensure_root_path_unmounted("DATA:");
	ensure_root_path_unmounted("SYSTEM:");
	ensure_root_path_unmounted("CACHE:");
	ensure_root_path_unmounted("SDCARD:");
	ensure_root_path_unmounted("SDEXT:");
#ifdef HAS_INTERNAL_SD
	ensure_root_path_unmounted("INTERNALSD:");
#endif
#ifdef IS_ICONIA
	ensure_root_path_unmounted("FLEXROM:");
#endif
	__system("/sbin/reboot bootloader");
}

void rb_recovery()
{
	sync();
	ensure_root_path_unmounted("DATA:");
	ensure_root_path_unmounted("SYSTEM:");
	ensure_root_path_unmounted("CACHE:");
	ensure_root_path_unmounted("SDCARD:");
	ensure_root_path_unmounted("SDEXT:");
#ifdef HAS_INTERNAL_SD
	ensure_root_path_unmounted("INTERNALSD:");
#endif
#ifdef IS_ICONIA
	ensure_root_path_unmounted("FLEXROM:");
#endif
	__system("/sbin/reboot recovery");
}

#ifdef MMC_PART_DEBUG
/* Porting Test */
void display_roots(const char *root)
{
  const RootInfo* info = get_device_info(root);
	
       	    mmc_scan_partitions();
       	    const MmcPartition *partition;
            partition = mmc_find_partition_by_name(info->partition_name);
	    if (partition == NULL) {
            LOGW("can't find mmc partition \"%s\"\n",
                    info->partition_name);
	    } else {
	     	LOGW("Begin info print!!\n");
		LOGW(" filesystem : %s device_index : %s name : %s dstatus : %d dtype : %d dfirstsec : %d dsize : %d \n\n", partition->filesystem, partition->device_index, partition->name, partition->dstatus, partition->dtype, partition->dfirstsec, partition->dsize); 	
	}
}	
#endif
  
#ifndef USES_NAND_MTD
const char* check_extfs_format(const char* root_path)
{
	const char *fstype;

	if (!strcmp(root_path, "SDEXT:")) {
	return NULL;
	}
	
	if (!strcmp(root_path, "SYSTEM:") || !strcmp(root_path, "DATA:") || !strcmp(root_path, "CACHE:") || !strcmp(root_path, "FLEXROM:")) {

	const RootInfo* info = get_device_info(root_path);
	if (info == NULL) {
        return NULL;
    	}		

	ensure_root_path_mounted(root_path);

	if (0 != ensure_root_path_mounted(root_path)) {
        ui_print("Error mounting %s!\n", info->mount_point);
        return NULL;
    	}

        static char discard1[1024];
        char device1[64], name1[64], type1[64];
        FILE *mountsf = fopen("/proc/mounts", "r");
 	
        while (fscanf(mountsf, "%64s %64s %64s %1024[^\n]", device1, name1, type1, discard1) != EOF) {
                /* Enjoy the whitespace! */
                		
		if (
                        !strcmp(name1, info->mount_point)
		   )
		LOGW("name: %s; device: %s; type: %s\n", name1, device1, type1);		
	}
	fclose(mountsf);	
		if (!strcmp(type1, "ext3") || !strcmp(type1, "ext4")) {
			
			fstype = type1;
			return fstype;
		} else {
			return NULL;
	}
	
	}
	return NULL;
}

int call_format_ext(const char* root)
{
		
	const MmcPartition *partition = get_root_mmc_partition(root);
	if (partition == NULL) {
        return -1;
    	}		
	const char *fstype = check_extfs_format(root);
	if (fstype == NULL) {
	return -1;
	}

	ensure_root_path_unmounted(root);
	
		
	if (!strcmp(fstype, "ext3")) {
	return mmc_format_ext3(partition);
		
	} else if (!strcmp(fstype, "ext4")) {
	return mmc_format_ext4(partition);

	} else {
	LOGW("Error formatting %s as %s \n" , partition->name, fstype);
	return -1;
      }

}
		
int force_format_ext3(const char* root)	
{		
	const char *fstype = check_extfs_format(root);
	if (fstype == NULL) {
		return -1;
	}

	ensure_root_path_unmounted(root);

	const MmcPartition *partition = get_root_mmc_partition(root);
		if (partition != NULL) {
		
		return mmc_format_ext3(partition);
    	}		

	const RootInfo* info = get_device_info(root);
		if (info == NULL) {
        	return -1;

    		} else {

		return format_ext3_device(info->device);
	}	
}

int force_format_ext4(const char* root)	
{	
	const char *fstype = check_extfs_format(root);
	if (fstype == NULL) {
		return -1;
	}

	ensure_root_path_unmounted(root);

	const MmcPartition *partition = get_root_mmc_partition(root);
		if (partition != NULL) {
		
		return mmc_format_ext4(partition);
    	}		

	const RootInfo* info = get_device_info(root);
		if (info == NULL) {
        	return -1;

    		} else {

		return format_ext4_device(info->device);
	}			
}

int upgrade_ext3_to_ext4(const char* root)
{
	const char *fstype = check_extfs_format(root);
		if (fstype == NULL) {
		return -1;
	}

	ensure_root_path_unmounted(root);

	const MmcPartition *partition = get_root_mmc_partition(root);
		if (partition != NULL) {
        	
			if (!strcmp(fstype, "ext3")) {
			return mmc_upgrade_ext3(partition);
		
			} else {

			ui_print("%s is already ext4\n", partition->name);
			return -1;

			}
		}

	const RootInfo* info = get_device_info(root);
		if (info == NULL) {
        	return -1;
    		}

		if (!strcmp(fstype, "ext3")) {
		return device_upgrade_ext3(info->device);
		
		} else {

		ui_print("%s is already ext4\n", info->name);
		return -1;

	     }
	
}	

int format_raw_partition(const char* root)
{
	const MmcPartition *partition = get_root_mmc_partition(root);
	if (partition == NULL) {
	//For non g_mmc emmc devices here
        	const RootInfo* info = get_device_info(root);
		if (!strcmp(info->partition_name, "boot")) {
		static char erase_raw_cmd[PATH_MAX];
		sprintf(erase_raw_cmd,"/sbin/busybox dd if=/dev/zero of=%s bs=4096", info->device);
		__system(erase_raw_cmd);
		return 0;	
	       }
    	}

	/* Lets be extra safe here */
	//if (!strcmp(partition->name, "boot") || !strcmp(partition->name, "recovery") || !strcmp(partition->name, "misc")) {
		if (!strcmp(partition->name, "boot")) {
		static char erase_raw_cmd[PATH_MAX];
		sprintf(erase_raw_cmd,"/sbin/busybox dd if=/dev/zero of=%s bs=4096", partition->device_index);
		__system(erase_raw_cmd);
		return 0;	
	} 
   
	return -1;
}
#endif
	
void write_fstab_root(const char *root_path, FILE *file)
{
    const RootInfo* info = get_device_info(root_path);
    if (info == NULL) {
        LOGW("Unable to get root info for %s during fstab generation!\n", root_path);
        return;
    }
    char device[PATH_MAX];
    int ret = get_device_index(root_path, device);
    if (ret == 0)
    {
        fprintf(file, "%s ", device);
	fprintf(file, "%s ", info->mount_point);
    	fprintf(file, "%s %s\n", info->filesystem, info->filesystem_options == NULL ? "rw" : info->filesystem_options);
    }
    else
    {
        LOGW("Error in getting device for %s", root_path);
    }
    
}

void create_fstab()
{
    __system("touch /etc/mtab");
    FILE *file = fopen("/etc/fstab", "w");
    if (file == NULL) {
        LOGW("Unable to create /etc/fstab!");
        return;
    }
    write_fstab_root("CACHE:", file);
    write_fstab_root("DATA:", file);
#ifdef HAS_INTERNAL_SD 
    write_fstab_root("INTERNALSD:", file);
#endif
    write_fstab_root("SYSTEM:", file);
    write_fstab_root("SDCARD:", file);
    write_fstab_root("SDEXT:", file);
#ifdef IS_ICONIA
    write_fstab_root("FLEXROM:", file);
#endif
    fclose(file);
}

void set_nandroid_prop(const char *root_path)
{

    const RootInfo* info = get_device_info(root_path);

    if (info == NULL) {
        LOGW("Unable to get root info for %s setprop nandroid generation!\n", root_path);
        return;
    }
    char device[PATH_MAX];
    int ret = get_device_index(root_path, device);
    
    if (ret == 0)
    {
	char propset[PATH_MAX];
	char name[PATH_MAX];
	char tmp[PATH_MAX];
	strcpy(name, info->partition_name);
	sprintf(propset, "ro.%s.block", name);
	sprintf(tmp, "setprop %s %s", propset, device);
	__system(tmp);
     }
     else
     {
	LOGW("Error in getting device to setprop for %s\n", root_path);
     }	
}
		
void setprop_func()
{
	set_nandroid_prop("BOOT:");
	set_nandroid_prop("RECOVERY:");
	set_nandroid_prop("MISC:");
	set_nandroid_prop("SDCARD:");
	set_nandroid_prop("SDEXT:");
#ifdef HAS_INTERNAL_SD 
	set_nandroid_prop("INTERNALSD:");
#endif
#ifdef IS_ICONIA
	set_nandroid_prop("FLEXROM:");
#endif
	detect_ums_path();
}	

#ifndef USES_NAND_MTD
int format_ext_device(const char* root)
{
	const RootInfo* info = get_device_info(root);
	if (info == NULL) {
        return -1;
    	}

	const char *fstype = check_extfs_format(root);
		if (fstype == NULL) {
		return -1;
	}

	ensure_root_path_unmounted(root);

	if (!strcmp(fstype, "ext3")) {
		return format_ext3_device(info->device);
	}

	if (!strcmp(fstype, "ext4")) {
		return format_ext4_device(info->device);
	}

	return -1;
}
#endif

int detect_ums_path()
{
	
#define USB_FUNC_PATH "/sys/devices/platform/usb_mass_storage/lun0/file"
#define USB_GADGET_PATH "/sys/devices/platform/msm_hsusb/gadget/lun0/file"

	char tmp[PATH_MAX];
	struct stat st;
	if (0 == stat(USB_FUNC_PATH, &st)) {
		//LOGW("UMS path detected as %s\n", USB_FUNC_PATH);
		sprintf(tmp,"setprop ro.ums.path %s", USB_FUNC_PATH);
		__system(tmp);
		return 0;
		}	
	if (0 == stat(USB_GADGET_PATH, &st)) {
		//LOGW("UMS path detected as %s\n", USB_GADGET_PATH);
		sprintf(tmp,"setprop ro.ums.path %s", USB_GADGET_PATH);
		__system(tmp);
		return 0;
		}	
	
	LOGW("Unable to determine ums path\n");
	return -1;
}

int symlink_toolbox()
{
	__system("/sbin/busybox --install -s /sbin");
	__system("ln -s /sbin/recovery /sbin/getprop");
	__system("ln -s /sbin/recovery /sbin/setprop");
/*
	__system("ln -s /sbin/busybox /sbin/umount");
	__system("ln -s /sbin/busybox /sbin/mount");
*/
#ifdef USES_NAND_MTD
	__system("ln -s /sbin/recovery /sbin/flash_image");
	__system("ln -s /sbin/recovery /sbin/dump_image");
	__system("ln -s /sbin/recovery /sbin/erase_image");
#endif	
#ifdef IS_ICONIA
	__system("ln -s /sbin/recovery /sbin/itsmagic");
#endif
#ifdef HBOOT_SON_KERNEL
	__system("ln -s /sbin/recovery /sbin/misctool");
#endif

return 0;
}

#ifdef LGE_RESET_BOOTMODE
int lge_fact_reset_checked = 0;

int lge_direct_emmc_access_write(char *boot_mode)
{
	/* using lge kernel api as in lge_emmc_direct_access.c */
 	FILE * ldeaw = fopen("/sys/module/lge_emmc_direct_access/parameters/write_block","r+");

	if (ldeaw != NULL) {
		fputs (boot_mode,ldeaw);
		fclose(ldeaw);
		LOGW("LGE_FACT_RESET_6 set\n\n");
		return 0;
		
	} else {
		fclose(ldeaw);
		return -1;
	}
}


int lge_direct_emmc_access_read()
{
	char read_lge[2];

	FILE * ldear = fopen("/sys/module/lge_emmc_direct_access/parameters/read_block","r");

	if (ldear != NULL) {
		fgets(read_lge, 2, ldear);
    		fclose(ldear);
	} else {
		fclose(ldear);
		return -1;
	}
	if (!strcmp(read_lge, "3")) {
		LOGW("LGE FACT_RESET_%s detected\n", read_lge);
		lge_direct_emmc_access_write("6");
		ui_print("LGE_FACT_RESET_6 set\n\n");
	} else {
		LOGW("LGE FACT_RESET_%s detected ...ignoring\n", read_lge);
	}
	
	lge_fact_reset_checked = !lge_fact_reset_checked;
	return 0;
}

int lge_direct_mtd_access_write(char *boot_mode)
{
	/* using lge kernel api as in lge_mtd_direct_access.c */
 	FILE * ldmaw = fopen("/sys/module/lge_mtd_direct_access/parameters/write_block","r+");

	if (ldmaw != NULL) {
		fputs (boot_mode,ldmaw);
		fclose(ldmaw);
		LOGW("LGE_FACT_RESET_6 set\n\n");
		return 0;
	} else {
		fclose(ldmaw);
		return -1;
	}
}


int lge_direct_mtd_access_read()
{
	char read_lge[2];

	FILE * ldmar = fopen("/sys/module/lge_mtd_direct_access/parameters/read_block","r");

	if (ldmar != NULL) {
		fgets(read_lge, 2, ldmar);
    		fclose(ldmar);
	} else {
		return -1;
	}
	
	if (!strcmp(read_lge, "3")) {
		LOGW("LGE FACT_RESET_%s detected\n", read_lge);
		lge_direct_mtd_access_write("6");
		ui_print("LGE_FACT_RESET_6 set\n\n");
	} else {
		LOGW("LGE FACT_RESET_%s detected ...ignoring\n", read_lge);
	}
		
	lge_fact_reset_checked = !lge_fact_reset_checked;
	return 0;
}

void check_lge_boot_mode()
{
	if (!lge_fact_reset_checked) {
		char emmc[64];
    		property_get("ro.emmc", emmc, "");

    		if(!strcmp(emmc, "1"))
    		 {
		    lge_direct_emmc_access_read();
		 }
		else
		 {
		    lge_direct_mtd_access_read();
		 }
	}
}
#endif

int manufacturer_icon_set = 0;

void set_manufacturer_icon()
{
     if (!manufacturer_icon_set) {
	char manufacturer[64];
    	property_get("ro.product.manufacturer", manufacturer, "");
	if(!strcmp(manufacturer, "HTC"))
    		 {
		    __system("rm /res/images/icon_error.png");
		    __system("cp /res/images/icon_error_htc.png /res/images/icon_error.png");
		 }
		else if (!strcmp(manufacturer, "LGE"))
		 {
		    __system("rm /res/images/icon_error.png");
		    __system("cp /res/images/icon_error_lg.png /res/images/icon_error.png");
		 }
		manufacturer_icon_set = !manufacturer_icon_set;
	}
}


#ifdef HBOOT_SON_KERNEL
void write_script(const char *txt, FILE *file)
{
	fprintf(file, "%s ", txt);
}

void create_htcmodelid_script()
{
	FILE *script = fopen("/sbin/htcmodelid.sh", "w");
    if (script == NULL) {
        LOGW("Unable to create /sbin/htcmodelid.sh");
        return;
    }
	write_script("#!/sbin/sh\n", script);
	write_script("MODELID=`cat /proc/cmdline | sed \"s/.*mid=//\" | cut -d\" \" -f1`\n", script);
	write_script("/sbin/setprop ro.htcmodelid $MODELID\n", script);
	write_script("exit 0\n", script);
	fclose(script);
	__system("chmod 0755 /sbin/htcmodelid.sh");
	__system("/sbin/htcmodelid.sh");
}
#endif

#ifdef IS_ICONIA
void exec_itsmagic()
{
	ui_print("\n");
	ui_print("Executing itsmagic by sc2k!\n");
	__system("/sbin/itsmagic");
	ui_print("Done!\n");
}
#endif

void symlink_data_media()
{
	__system("setprop ro.datamedia.device 1");
	__system("mkdir /data/media");
	__system("ln -s /data/media/ /internal_sdcard");
}

void preinit_setup()
{
/* Pre recovery setup items GNM */ 
    symlink_toolbox();
    set_root_table();
    create_fstab();
    setprop_func();
    set_manufacturer_icon();
#ifdef HBOOT_SON_KERNEL
    create_htcmodelid_script();
#endif

#ifdef LGE_RESET_BOOTMODE
    check_lge_boot_mode();
#endif

#ifdef HAS_DATA_MEDIA_SDCARD
    symlink_data_media();
#endif
}

void source_and_credits()
{
	ui_print("Built by Getitnowmarketing\n\n");
	ui_print("Credits:\n");
	ui_print("Amon Ra for original source\n");
	ui_print("Koush & the rest of Cyanogenmod team\n");
#ifdef TOUCH_UI
	ui_print("Gweedo767 & CEnnis91 for touch ui\n");
#endif
	ui_print("Source Code for both GPL and Apache Licensed code:\n");
	ui_print("https://github.com/getitnowmarketing\n");
}
	
