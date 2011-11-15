

#include <linux/input.h>
#include "recovery_ui_keys.h"

#ifdef USE_TOUCH_SCROLLING
#include "minui/minui.h"
#endif

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
				return SELECT_ITEM;

			case KEY_BACK:
				return GO_BACK;
		}
	}
	return NO_ACTION;
}
