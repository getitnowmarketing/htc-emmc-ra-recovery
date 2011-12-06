

#include <linux/input.h>
#include "recovery_ui_keys.h"

#ifdef USE_TOUCH_SCROLLING
#include "minui/minui.h"
#endif

#if defined (DEFAULT_RECOVERY_UI_KEYS) || defined (KYROS_RECOVERY_UI_KEYS)
int device_handle_key(int key_code, int visable) {
	if (visable) {
		switch (key_code) {
			case KEY_VOLUMEDOWN:
			case KEY_MENU:
#ifdef USE_TOUCH_SCROLLING
			case MT_FAKE_DN:
#endif
				return HIGHLIGHT_DOWN;
		
			case KEY_VOLUMEUP:
			case KEY_HOME:
#ifdef USE_TOUCH_SCROLLING
			case MT_FAKE_UP:
#endif
				return HIGHLIGHT_UP;

			case KEY_POWER:
			case KEY_SEARCH:
			case KEY_END:
				return SELECT_ITEM;

			case KEY_BACK:
				return GO_BACK;
		}
	}
	return NO_ACTION;
}
#endif

#ifdef DEFAULT_NAND_RECOVERY_UI_KEYS

int device_handle_key(int key_code, int visable) {
	if (visable) {
		switch (key_code) {
			case KEY_VOLUMEDOWN:
			
#ifdef USE_TOUCH_SCROLLING
			case MT_FAKE_DN:
#endif
				return HIGHLIGHT_DOWN;
		
			case KEY_VOLUMEUP:
			
#ifdef USE_TOUCH_SCROLLING
			case MT_FAKE_UP:
#endif
				return HIGHLIGHT_UP;

			case KEY_POWER:
			case KEY_MENU:
				return SELECT_ITEM;

			case KEY_BACK:
				return GO_BACK;
		}
	}
	return NO_ACTION;
}

#endif

#ifdef ALOHA_RECOVERY_UI_KEYS

int device_handle_key(int key_code, int visable) {
	if (visable) {
		switch (key_code) {
			case KEY_VOLUMEDOWN:
			
#ifdef USE_TOUCH_SCROLLING
			case MT_FAKE_DN:
#endif
				return HIGHLIGHT_DOWN;
		
			case KEY_VOLUMEUP:
			
#ifdef USE_TOUCH_SCROLLING
			case MT_FAKE_UP:
#endif
				return HIGHLIGHT_UP;

			case KEY_POWER:
			case KEY_END:
				return SELECT_ITEM;

			case KEY_BACK:
				return GO_BACK;
		}
	}
	return NO_ACTION;
}

#endif

#ifdef HTC_TRACKBALL_RECOVERY_UI_KEYS

int device_handle_key(int key_code, int visable) {
	if (visable) {
		switch (key_code) {
			case KEY_DOWN:
#ifdef USE_TOUCH_SCROLLING
			case MT_FAKE_DN:
#endif
				return HIGHLIGHT_DOWN;
		
			case KEY_UP:
#ifdef USE_TOUCH_SCROLLING
			case MT_FAKE_UP:
#endif
				return HIGHLIGHT_UP;

			case KEY_POWER:
			case BTN_MOUSE:
				return SELECT_ITEM;

			case KEY_BACK:
			case VOLUME_DOWN:
				return GO_BACK;
		}
	}
	return NO_ACTION;
}

#endif