/* Getitnowmarketing 10/29/11 
added to define recovery keys so recovery keys can easily be changed for new devices */

/* Work-around to use confirm key in ui_print */

/* useage is ui_print(" confirm is: %s", confirm_key_hack(CONFIRM) ); */

#define CONFIRM Power
#define mkstr(confirm_key) # confirm_key
#define confirm_key_hack(confirm_key) mkstr(confirm_key)

/* these are defined in device's kernel input.h */
#define SELECT KEY_POWER
#define UP KEY_VOLUMEUP
#define DN KEY_VOLUMEDOWN
#define GO_BACK KEY_BACK


