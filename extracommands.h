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

void
check_fs();

int
format_ext(const char *device, const char *type, const char *mount_pt, const char* root, int chk_for_upg_fs, int force_ext3);

int
check_fs_format(const char* root, const char* mnt_pt, int chk_for_upg_fs, int force_ext3);

extern int
full_ext_format_enabled;

void
toggle_full_ext_format();

int
upgrade_ext3(const char* device, const char* mount_pt, const char* root);

int
format_ext4(const char* device, const char* mount_pt, const char* root);

int
format_ext3(const char* device, const char* mount_pt, const char* root);

int
get_boot_device(const char *root, const char* request);

int
dump_device(const char *device);

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
