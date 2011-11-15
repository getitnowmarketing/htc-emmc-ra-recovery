/* Getitnowmarketing 10/29/11 
added to define recovery keys so recovery keymapping can easily be changed for new devices */

extern int device_handle_key(int key, int visible);

#define NO_ACTION     		-1
#define HIGHLIGHT_UP           	-2
#define HIGHLIGHT_DOWN         	-3
#define SELECT_ITEM        	-4
#define GO_BACK       		-5


/* Begin Device Specific Here */

#define CONFIRM "Power"
#define UNCONFIRM_TXT "or press BACK to return"
#define UPDOWNTXT "Vol Up/Down"

#define RB_KEY1 	KEY_POWER
#define RB_KEY2 	KEY_VOLUMEDOWN
#define RB_KEY3 	KEY_VOLUMEUP












