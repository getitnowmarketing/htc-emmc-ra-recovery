extern int signature_check_enabled;

void
toggle_signature_check();

void
run_script(char *str1,char *str2,char *str3,char *str4,char *str5,char *str6,char *str7);

void
usb_toggle_sdcard();

#ifdef HAS_INTERNAL_SD
void
usb_toggle_internal();
#endif

int
__system(const char *command);

int
format_non_mtd_device(const char* root);

void
wipe_battery_stats();

void
wipe_rotate_settings();

void 
key_logger_test();

void
check_my_battery_level();

void
make_clockwork_path();

#ifndef USES_NAND_MTD
void
check_fs();

extern int
full_ext_format_enabled;

void
toggle_full_ext_format();
#endif

void
unpack_boot();

void
setup_mkboot();

int
check_file_exists(const char* file_path);

int
copy_file(const char* source, const char* dest);

void
do_module();

void
do_make_new_boot();

int
is_dir(const char* file_path);

int
delete_file(const char* file);

void
install_su(int eng_su);

void
rb_bootloader();

void
rb_recovery();

void
display_roots(const char *root);

#ifndef USES_NAND_MTD
int
call_format_ext(const char* root_path);

const char *
check_extfs_format(const char* root);

int
upgrade_ext3_to_ext4(const char* root);

int
force_format_ext3(const char* root);

int
force_format_ext4(const char* root);

int
format_raw_partition(const char* root);

int
format_ext_device(const char* root);
#endif

int
dump_device(const char *root);

void
create_fstab();

void
write_fstab_root(const char *root_path, FILE *file);

int
detect_ums_path();

int
symlink_toolbox();

#ifdef LGE_RESET_BOOTMODE
void
check_lge_boot_mode();
#endif

void
set_manufacturer_icon();

#ifdef HBOOT_SON_KERNEL
void
create_htcmodelid_script();

int
do_make_new_hbootbootzip();

int
do_make_new_hbootbootzip_auto();

void
delete_hboot_zip();
#endif



