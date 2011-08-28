extern int signature_check_enabled;

void
toggle_signature_check();

void
run_script(char *str1,char *str2,char *str3,char *str4,char *str5,char *str6,char *str7);

void
usb_toggle_sdcard();

void 
usb_toggle_emmc();

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

//void check_my_battery_level();

