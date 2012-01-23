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

#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "minui/minui.h"

#include "recovery_ui_keys.h"

#define MAX_COLS 64
#define MAX_ROWS 64

#define MENU_MAX_COLS 64
#define MENU_MAX_ROWS 250

#if defined (BOARD_LDPI_RECOVERY)
#define CHAR_WIDTH 7
#define CHAR_HEIGHT 16
#elif defined (BOARD_XDPI_RECOVERY)
#define CHAR_WIDTH 15
#define CHAR_HEIGHT 24
#else
#define CHAR_WIDTH 10
#define CHAR_HEIGHT 18
#endif

#define PROGRESSBAR_INDETERMINATE_STATES 6
#define PROGRESSBAR_INDETERMINATE_FPS 15

enum { LEFT_SIDE, CENTER_TILE, RIGHT_SIDE, NUM_SIDES };

static pthread_mutex_t gUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static gr_surface gBackgroundIcon[NUM_BACKGROUND_ICONS];
static gr_surface gProgressBarIndeterminate[PROGRESSBAR_INDETERMINATE_STATES];
static gr_surface gProgressBarEmpty[NUM_SIDES];
static gr_surface gProgressBarFill[NUM_SIDES];
static gr_surface gVirtualKeys; // surface for our virtual key buttons

static const struct { gr_surface* surface; const char *name; } BITMAPS[] = {
    
    { &gBackgroundIcon[BACKGROUND_ICON_INSTALLING], "icon_installing" },
    { &gBackgroundIcon[BACKGROUND_ICON_ERROR],      "icon_error" },
    { &gBackgroundIcon[BACKGROUND_ICON_FIRMWARE_INSTALLING],
        "icon_firmware_install" },
    { &gBackgroundIcon[BACKGROUND_ICON_FIRMWARE_ERROR],
        "icon_firmware_error" },
    { &gProgressBarIndeterminate[0],    "indeterminate1" },
    { &gProgressBarIndeterminate[1],    "indeterminate2" },
    { &gProgressBarIndeterminate[2],    "indeterminate3" },
    { &gProgressBarIndeterminate[3],    "indeterminate4" },
    { &gProgressBarIndeterminate[4],    "indeterminate5" },
    { &gProgressBarIndeterminate[5],    "indeterminate6" },
    { &gProgressBarEmpty[LEFT_SIDE],    "progress_bar_empty_left_round" },
    { &gProgressBarEmpty[CENTER_TILE],  "progress_bar_empty" },
    { &gProgressBarEmpty[RIGHT_SIDE],   "progress_bar_empty_right_round" },
    { &gProgressBarFill[LEFT_SIDE],     "progress_bar_left_round" },
    { &gProgressBarFill[CENTER_TILE],   "progress_bar_fill" },
    { &gProgressBarFill[RIGHT_SIDE],    "progress_bar_right_round" },
    { &gVirtualKeys,      "virtual_keys" },
    { NULL,                             NULL },
};

static gr_surface gCurrentIcon = NULL;

static enum ProgressBarType {
    PROGRESSBAR_TYPE_NONE,
    PROGRESSBAR_TYPE_INDETERMINATE,
    PROGRESSBAR_TYPE_NORMAL,
} gProgressBarType = PROGRESSBAR_TYPE_NONE;

// Progress bar scope of current operation
static float gProgressScopeStart = 0, gProgressScopeSize = 0, gProgress = 0;
static time_t gProgressScopeTime, gProgressScopeDuration;

// Set to 1 when both graphics pages are the same (except for the progress bar)
static int gPagesIdentical = 0;

// Log text overlay, displayed when a magic key is pressed
static char text[MAX_ROWS][MAX_COLS];
static int text_cols = 0, text_rows = 0;
static int text_col = 0, text_row = 0, text_top = 0;
static int show_text = 1;

static char menu[MENU_MAX_ROWS][MENU_MAX_COLS];
static int show_menu = 0;
static int menu_top = 0, menu_items = 0, menu_sel = 0;
static int menu_show_start = 0;             // this is line which menu display is starting at 

// Key event input queue
static pthread_mutex_t key_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t key_queue_cond = PTHREAD_COND_INITIALIZER;
static int key_queue[256], key_queue_len = 0;
static volatile char key_pressed[KEY_MAX + 1];

// Clear the screen and draw the currently selected background icon (if any).
// Should only be called with gUpdateMutex locked.
static void draw_background_locked(gr_surface icon)
{
    gPagesIdentical = 0;
    gr_color(0, 0, 0, 255);
    gr_fill(0, 0, gr_fb_width(), gr_fb_height());

    if (icon) {
        int iconWidth = gr_get_width(icon);
        int iconHeight = gr_get_height(icon);
        int iconX = (gr_fb_width() - iconWidth) / 2;
        int iconY = (gr_fb_height() - iconHeight) / 2;
        gr_blit(icon, 0, 0, iconWidth, iconHeight, iconX, iconY);
    }
}

// Draw the progress bar (if any) on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_progress_locked()
{
    if (gProgressBarType == PROGRESSBAR_TYPE_NONE) return;

    int iconHeight = gr_get_height(gBackgroundIcon[BACKGROUND_ICON_INSTALLING]);
    int width = gr_get_width(gProgressBarIndeterminate[0]);
    int height = gr_get_height(gProgressBarIndeterminate[0]);

    int dx = (gr_fb_width() - width)/2;
    int dy = (3*gr_fb_height() + iconHeight - 2*height)/4;

    // Erase behind the progress bar (in case this was a progress-only update)
    gr_color(0, 0, 0, 255);
    gr_fill(dx, dy, width, height);

    if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL) {
        float progress = gProgressScopeStart + gProgress * gProgressScopeSize;
        int pos = (int) (progress * width);

        gr_surface s = (pos ? gProgressBarFill : gProgressBarEmpty)[LEFT_SIDE];
        gr_blit(s, 0, 0, gr_get_width(s), gr_get_height(s), dx, dy);

        int x = gr_get_width(s);
        while (x + (int) gr_get_width(gProgressBarEmpty[RIGHT_SIDE]) < width) {
            s = (pos > x ? gProgressBarFill : gProgressBarEmpty)[CENTER_TILE];
            gr_blit(s, 0, 0, gr_get_width(s), gr_get_height(s), dx + x, dy);
            x += gr_get_width(s);
        }

        s = (pos > x ? gProgressBarFill : gProgressBarEmpty)[RIGHT_SIDE];
        gr_blit(s, 0, 0, gr_get_width(s), gr_get_height(s), dx + x, dy);
    }

    if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE) {
        static int frame = 0;
        gr_blit(gProgressBarIndeterminate[frame], 0, 0, width, height, dx, dy);
        frame = (frame + 1) % PROGRESSBAR_INDETERMINATE_STATES;
    }
}

static int vk_iconwidth = 0;
static int vk_iconheight = 0;
static int vk_iconX = 0;
static int vk_iconY = 0;

// Draw the virtual keys on the screen. Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_virtualkeys_locked()
{
    gr_surface surface = gVirtualKeys;
    int iconWidth = gr_get_width(surface);
    int iconHeight = gr_get_height(surface);
#ifdef IS_ICONIA
    int iconX = (gr_fb_width() - iconWidth);
#else
    int iconX = 0; // align left, full width on 720p displays, but moves over on tablets with > 720 pixels
#endif
    int iconY = (gr_fb_height() - iconHeight);
    //LOGW("iconWidth: %d , iconHeight: %d , iconX: %d , iconY: %d \n", iconWidth, iconHeight, iconX, iconY);
    gr_blit(surface, 0, 0, iconWidth, iconHeight, iconX, iconY);
}

static void export_vk_info()
{
   if (vk_iconwidth == 0 && vk_iconheight == 0 && vk_iconX == 0 && vk_iconY == 0) {
   	gr_surface surface = gVirtualKeys;
   	vk_iconwidth = gr_get_width(surface);
   	vk_iconheight = gr_get_height(surface);
   	vk_iconX = (gr_fb_width() - vk_iconwidth);
   	vk_iconY = (gr_fb_height() - vk_iconheight);
	//LOGW("vk_iconwidth: %d , vk_iconheight: %d , vk_iconX: %d , vk_iconY: %d \n", vk_iconwidth, vk_iconheight, vk_iconX, vk_iconY);
  }
}

#define LEFT_ALIGN 0
#define CENTER_ALIGN 1
#define RIGHT_ALIGN 2

static void draw_text_line(int row, const char* t, int align) {
    int col = 0;
    if (t[0] != '\0') {
        int length = strnlen(t, MENU_MAX_COLS) * CHAR_WIDTH;
        switch(align)
        {
            case LEFT_ALIGN:
                col = 1;
                break;
            case CENTER_ALIGN:
                col = ((gr_fb_width() - length) / 2);
                break;
            case RIGHT_ALIGN:
                col = gr_fb_width() - length - 1;
                break;
        }

     gr_text(col, (row+1)*CHAR_HEIGHT-1, t);
    }
}

#if defined (VIGOR_RED)
// Vigor Red
#define MENU_TEXT_COLOR 224, 30, 12, 255
#define NORMAL_TEXT_COLOR 116, 116, 116, 255
#define SELECTED_TEXT_COLOR 0, 0, 0, 255

#elif defined (CM_THEME)
// CM
#define MENU_TEXT_COLOR 61, 233, 255, 255
#define NORMAL_TEXT_COLOR 193, 193, 193, 255
#define SELECTED_TEXT_COLOR 0, 0, 0, 255

#elif defined (HTC_THEME)
// HTC
#define MENU_TEXT_COLOR 120, 166, 0, 255
#define NORMAL_TEXT_COLOR 193, 193, 193, 255
#define SELECTED_TEXT_COLOR 255, 255, 255, 255

#elif defined (JF_THEME)
// JF
#define MENU_TEXT_COLOR 64, 96, 255, 255
#define NORMAL_TEXT_COLOR 255, 255, 0, 255
#define SELECTED_TEXT_COLOR 255, 255, 255, 255

#elif defined (ICONIA_DK_BLUE)
//
#define MENU_TEXT_COLOR 51, 20, 255, 255
#define NORMAL_TEXT_COLOR 193, 193, 193, 255
#define SELECTED_TEXT_COLOR 0, 0, 0, 255

#else 
// CM
#define MENU_TEXT_COLOR 61, 233, 255, 255
#define NORMAL_TEXT_COLOR 193, 193, 193, 255
#define SELECTED_TEXT_COLOR 0, 0, 0, 255

#endif

// Redraw everything on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_screen_locked(void)
{
    draw_background_locked(gCurrentIcon);
    draw_progress_locked();
    
    if (show_text) {
        gr_color(0, 0, 0, 160);
        gr_fill(0, 0, gr_fb_width(), gr_fb_height());

        int i = 0;
        int j = 0;
        int row = 0;
        if (show_menu) {

            gr_color(MENU_TEXT_COLOR);

            gr_fill(0, (menu_top + menu_sel - menu_show_start) * CHAR_HEIGHT,
                    gr_fb_width(), (menu_top + menu_sel - menu_show_start + 1)*CHAR_HEIGHT+1);

            for (i = 0; i < menu_top; ++i) {
                draw_text_line(i, menu[i], LEFT_ALIGN);
                row++;
            }

            if (menu_items - menu_show_start + menu_top >= MAX_ROWS)
                j = MAX_ROWS - menu_top;
            else
                j = menu_items - menu_show_start;

            gr_color(MENU_TEXT_COLOR);

            for (i = menu_show_start + menu_top; i < (menu_show_start + menu_top + j); ++i) {
                if (i == menu_top + menu_sel) {

		    gr_color(SELECTED_TEXT_COLOR);

                    draw_text_line(i - menu_show_start, menu[i], LEFT_ALIGN);
	
	            gr_color(MENU_TEXT_COLOR);

                } else {

	            gr_color(MENU_TEXT_COLOR);

                    draw_text_line(i - menu_show_start, menu[i], LEFT_ALIGN);
                }
		row++;
            }
            gr_fill(0, row*CHAR_HEIGHT+CHAR_HEIGHT/2-1,
                    gr_fb_width(), row*CHAR_HEIGHT+CHAR_HEIGHT/2+1);
        }

        gr_color(NORMAL_TEXT_COLOR);


	row++;
        for (; row < text_rows; ++row) {
            draw_text_line(row, text[(row+text_top) % text_rows], LEFT_ALIGN);
        }
    }
	draw_virtualkeys_locked(); //added to draw the virtual keys
	export_vk_info();
}

// Redraw everything on the screen and flip the screen (make it visible).
// Should only be called with gUpdateMutex locked.
static void update_screen_locked(void)
{
    draw_screen_locked();
    gr_flip();
}

// Updates only the progress bar, if possible, otherwise redraws the screen.
// Should only be called with gUpdateMutex locked.
static void update_progress_locked(void)
{
    if (show_text || !gPagesIdentical) {
        draw_screen_locked();    // Must redraw the whole screen
        gPagesIdentical = 1;
    } else {
        draw_progress_locked();  // Draw only the progress bar
    }
    gr_flip();
}

// Keeps the progress bar updated, even when the process is otherwise busy.
static void *progress_thread(void *cookie)
{
    for (;;) {
        usleep(1000000 / PROGRESSBAR_INDETERMINATE_FPS);
        pthread_mutex_lock(&gUpdateMutex);

        // update the progress bar animation, if active
        // skip this if we have a text overlay (too expensive to update)
        if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE && !show_text) {
            update_progress_locked();
        }

        // move the progress bar forward on timed intervals, if configured
        int duration = gProgressScopeDuration;
        if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && duration > 0) {
            int elapsed = time(NULL) - gProgressScopeTime;
            float progress = 1.0 * elapsed / duration;
            if (progress > 1.0) progress = 1.0;
            if (progress > gProgress) {
                gProgress = progress;
                update_progress_locked();
            }
        }

        pthread_mutex_unlock(&gUpdateMutex);
    }
    return NULL;
}

static int rel_sum = 0;
static int in_touch = 0; //1 = in a touch
static int slide_right = 0;
static int slide_left = 0;
static int touch_x = 0;
static int touch_y = 0;
static int old_x = 0;
static int old_y = 0;
static int diff_x = 0;
static int diff_y = 0;


static void reset_gestures() {
    diff_x = 0;
    diff_y = 0;
    old_x = 0;
    old_y = 0;
    touch_x = 0;
    touch_y = 0;
}

int input_buttons()
{
    int final_code = 0;
    int start_draw = 0;
    int end_draw = 0;
    int vk_step = (vk_iconwidth / 4);
    
    if(touch_x > vk_iconX && touch_x < (vk_iconX + vk_step)) {
        //back button
        final_code = KEY_BACK;
	//LOGW("final_code input_button back\n");
        start_draw = vk_iconX;
        end_draw = ((vk_iconX + vk_step)-1);
    } else if(touch_x > vk_iconX && touch_x < (vk_iconX + (vk_step*2))) {
        //up button
        final_code = KEY_UP;
	//LOGW("final_code input_button up\n");
        start_draw = (vk_iconX + vk_step);
        end_draw = (vk_iconX + (vk_step*2)-1);
    } else if(touch_x > vk_iconX && touch_x < (vk_iconX + (vk_step*3))) {
        //down button
        final_code = KEY_DOWN;
	//LOGW("final_code input_button down\n");
        start_draw = (vk_iconX + (vk_step*2));
        end_draw = (vk_iconX + (vk_step*3)-1);
//    } else {
    } else if(touch_x > vk_iconX && touch_x < (vk_iconX + (vk_step*4))) {   
        //enter key
        final_code = KEY_ENTER;
	//LOGW("final_code input_button enter\n");
        start_draw = (vk_iconX + (vk_step*3));
        end_draw = gr_fb_width();
    } else {
	// x input is out of panel range
	//LOGW(" X %d is out of range\n", touch_x);
	return 0;
    }
		
    
    if(touch_y > (gr_fb_height() - vk_iconheight) && touch_x > vk_iconX) {
        pthread_mutex_lock(&gUpdateMutex);
        gr_color(0, 0, 0, 255); // clear old touch points
        gr_fill(vk_iconX, gr_fb_height()-(vk_iconheight+2), start_draw-1, gr_fb_height()-vk_iconheight);
        //gr_fill(0, gr_fb_height()-(vk_iconheight+2), start_draw-1, gr_fb_height()-vk_iconheight);
        gr_fill(end_draw+1, gr_fb_height()-(vk_iconheight+2), gr_fb_width(), gr_fb_height()-vk_iconheight);
        gr_color(MENU_TEXT_COLOR);
        gr_fill(start_draw, gr_fb_height()-(vk_iconheight+2), end_draw, gr_fb_height()-vk_iconheight);
        gr_flip();
        pthread_mutex_unlock(&gUpdateMutex);
    }
    
    if (in_touch == 1) {
	//LOGW("final_code input_button returned %d \n", final_code);
        return final_code;
    } else {
        return 0;
    }
}

static int input_callback(int fd, short revents, void *data)
{
    struct input_event ev;
    int ret;
    int fake_key = 0;
    
    ret = ev_get_input(fd, revents, &ev);
    if (ret)
        return -1;

    if (ev.type == EV_SYN) {
        return 0;
    } else if (ev.type == EV_REL) {
        if (ev.code == REL_Y) {
            // accumulate the up or down motion reported by
            // the trackball. When it exceeds a threshold
            // (positive or negative), fake an up/down
            // key event.
            rel_sum += ev.value;
            if (rel_sum > 3) {
                fake_key = 1;
                ev.type = EV_KEY;
                ev.code = KEY_DOWN;
                ev.value = 1;
                rel_sum = 0;
            } else if (rel_sum < -3) {
                fake_key = 1;
                ev.type = EV_KEY;
                ev.code = KEY_UP;
                ev.value = 1;
                rel_sum = 0;
            }
         }
	} else {
		rel_sum = 0;
	}
	//printf("ev.code: %i, ev.type: %i, ev.value: %i\n", ev.code, ev.type, ev.value);
    if(ev.type == 3 && ev.code == 57) {
        if(in_touch == 0) {
            in_touch = 1; //starting to track touch...
            reset_gestures();
        } else {
            //finger lifted! lets run with this
            ev.type = EV_KEY; //touch panel support!!!
#ifdef IS_ICONIA
	    int keywidth = (vk_iconwidth / 4);
#else
            int keywidth = gr_fb_width() / 4;
#endif
	    if(touch_y > gr_fb_height() - vk_iconheight && touch_x > vk_iconX) {
                //they lifted in the touch panel region
                if(touch_x < (keywidth + vk_iconX)) {
                    //back button
                    ev.code = KEY_BACK;
		//LOGW("touch back key @ %d\n", touch_x);
                } else if(touch_x < (keywidth*2 + vk_iconX)) {
                    //up button
                    ev.code = KEY_UP;
		//LOGW("touch up key @ %d\n", touch_x);
                } else if(touch_x < (keywidth*3 + vk_iconX)) {
                    //down button
                    ev.code = KEY_DOWN;
		//LOGW("touch down key @ %d\n", touch_x);
                } else {
                    //enter key
                    ev.code = KEY_ENTER;
			//LOGW("touch enter key @ %d\n", touch_x);
                }
                vibrate(VIBRATOR_TIME_MS);
            }
            if(slide_right == 1) {
                ev.code = KEY_ENTER;
                slide_right = 0;
            } else if(slide_left == 1) {
                ev.code = KEY_BACK;
                slide_left = 0;
            }
	ev.value = 1;
            in_touch = 0;
            reset_gestures();
        }
    } else if(ev.type == 3 && ev.code == 53) {

        old_x = touch_x;
      if (ev.value > vk_iconX) {
	 touch_x = ev.value;
      } else {
	touch_x = 0;
	in_touch = 0;
      }
//	LOGW("old_x : %d, touch_x: %d, touch_y: %d\n", old_x, touch_x, touch_y);
/*
        if(old_x != 0) {
		 diff_x = (touch_x - old_x);
		LOGW("diff x: %d\n", diff_x);
	}
    
//        if(touch_y < (gr_fb_height() - (100 + vk_iconheight))) {
	 if(touch_y < gr_fb_height() - 160) {
            if(diff_x > 100) {
                printf("Gesture forward generated\n");
                slide_right = 1;
                //ev.code = KEY_ENTER;
                //ev.type = EV_KEY;
                reset_gestures();
            } else if(diff_x < -100) {
                printf("Gesture back generated\n");
                slide_left = 1;
                //ev.code = KEY_BACK;
                //ev.type = EV_KEY;
                reset_gestures();
            }
        } else {
*/
	  //   LOGW("Calling input_buttons evcode 53\n");
 	     input_buttons();
            //reset_gestures();
//        }
    } else if(ev.type == 3 && ev.code == 54) {

        old_y = touch_y;
      if (ev.value > (gr_fb_height() - vk_iconheight)) {
	touch_y = ev.value;
      } else {
	touch_y = 0;
	in_touch = 0;
      }

/*
	LOGW("old_y : %d, touch_y: %d\n", old_y, touch_y);
        if(old_y != 0) {
		 diff_y = (touch_y - old_y);
		LOGW("diff y: %d\n", diff_y);
        }
        
//        if(touch_y < (gr_fb_height() - (100 + vk_iconheight))) {
	if(touch_y < gr_fb_height() - 160) {
            if(diff_y > 80) {
                printf("Gesture Down generated\n");
                ev.code = KEY_DOWN;
                ev.type = EV_KEY;
                reset_gestures();
            } else if(diff_y < -80) {
                printf("Gesture Up generated\n");
                ev.code = KEY_UP;
                ev.type = EV_KEY;
                reset_gestures();
            }
        } else {
*/
		//LOGW("Calling input_buttons evcode 54\n");
		input_buttons();
            //reset_gestures();
//       }
    }
    
    if (ev.type != EV_KEY || ev.code > KEY_MAX) {
        return 0;
    }

    pthread_mutex_lock(&key_queue_mutex);
    if (!fake_key) {
        // our "fake" keys only report a key-down event (no
        // key-up), so don't record them in the key_pressed
        // table.
        key_pressed[ev.code] = ev.value;
    }
    const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
    if (ev.value > 0 && key_queue_len < queue_max) {
        key_queue[key_queue_len++] = ev.code;
        pthread_cond_signal(&key_queue_cond);
    }
    pthread_mutex_unlock(&key_queue_mutex);

    // Alt+L or Home+End: toggle log display
        int alt = key_pressed[KEY_LEFTALT] || key_pressed[KEY_RIGHTALT];
        if ((alt && ev.code == KEY_L && ev.value > 0) ||
            (key_pressed[KEY_HOME] && ev.code == KEY_END && ev.value > 0)) {
        pthread_mutex_lock(&gUpdateMutex);
        show_text = !show_text;
        update_screen_locked();
        pthread_mutex_unlock(&gUpdateMutex);
    }

    // voldown+volup+power: reboot immediately
        if (ev.code == RB_KEY1 &&
            key_pressed[RB_KEY2] &&
            key_pressed[RB_KEY3]) {
            reboot(RB_AUTOBOOT);
        }

    return 0;
}

// Reads input events, handles special hot keys, and adds to the key queue.
static void *input_thread(void *cookie)
{
    for (;;) {
        if (!ev_wait(-1))
            ev_dispatch();
    }
    return NULL;
}

void ui_init(void)
{
    gr_init();
    ev_init(input_callback, NULL);

    text_col = text_row = 0;
    text_rows = gr_fb_height() / CHAR_HEIGHT;
    if (text_rows > MAX_ROWS) text_rows = MAX_ROWS;
    text_top = 1;

    text_cols = gr_fb_width() / CHAR_WIDTH;
    if (text_cols > MAX_COLS - 1) text_cols = MAX_COLS - 1;

    int i;
    for (i = 0; BITMAPS[i].name != NULL; ++i) {
        int result = res_create_surface(BITMAPS[i].name, BITMAPS[i].surface);
        if (result < 0) {
            LOGE("Missing bitmap %s\n(Code %d)\n", BITMAPS[i].name, result);
            *BITMAPS[i].surface = NULL;
        }
    }

    pthread_t t;
    pthread_create(&t, NULL, progress_thread, NULL);
    pthread_create(&t, NULL, input_thread, NULL);
}

char *ui_copy_image(int icon, int *width, int *height, int *bpp) {
    pthread_mutex_lock(&gUpdateMutex);
    draw_background_locked(gBackgroundIcon[icon]);
    *width = gr_fb_width();
    *height = gr_fb_height();
    *bpp = sizeof(gr_pixel) * 8;
    int size = *width * *height * sizeof(gr_pixel);
    char *ret = malloc(size);
    if (ret == NULL) {
        LOGE("Can't allocate %d bytes for image\n", size);
    } else {
        memcpy(ret, gr_fb_data(), size);
    }
    pthread_mutex_unlock(&gUpdateMutex);
    return ret;
}

void ui_set_background(int icon)
{
    pthread_mutex_lock(&gUpdateMutex);
    gCurrentIcon = gBackgroundIcon[icon];
    update_screen_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_show_indeterminate_progress()
{
    pthread_mutex_lock(&gUpdateMutex);
    if (gProgressBarType != PROGRESSBAR_TYPE_INDETERMINATE) {
        gProgressBarType = PROGRESSBAR_TYPE_INDETERMINATE;
        update_progress_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_show_progress(float portion, int seconds)
{
    pthread_mutex_lock(&gUpdateMutex);
    gProgressBarType = PROGRESSBAR_TYPE_NORMAL;
    gProgressScopeStart += gProgressScopeSize;
    gProgressScopeSize = portion;
    gProgressScopeTime = time(NULL);
    gProgressScopeDuration = seconds;
    gProgress = 0;
    update_progress_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_set_progress(float fraction)
{
    pthread_mutex_lock(&gUpdateMutex);
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && fraction > gProgress) {
        // Skip updates that aren't visibly different.
        int width = gr_get_width(gProgressBarIndeterminate[0]);
        float scale = width * gProgressScopeSize;
        if ((int) (gProgress * scale) != (int) (fraction * scale)) {
            gProgress = fraction;
            update_progress_locked();
        }
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_reset_progress()
{
    pthread_mutex_lock(&gUpdateMutex);
    gProgressBarType = PROGRESSBAR_TYPE_NONE;
    gProgressScopeStart = gProgressScopeSize = 0;
    gProgressScopeTime = gProgressScopeDuration = 0;
    gProgress = 0;
    update_screen_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_print(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 256, fmt, ap);
    va_end(ap);

    fputs(buf, stderr);

    // This can get called before ui_init(), so be careful.
    pthread_mutex_lock(&gUpdateMutex);
    if (text_rows > 0 && text_cols > 0) {
        char *ptr;
        for (ptr = buf; *ptr != '\0'; ++ptr) {
            if (*ptr == '\n' || text_col >= text_cols) {
                text[text_row][text_col] = '\0';
                text_col = 0;
                text_row = (text_row + 1) % text_rows;
                if (text_row == text_top) text_top = (text_top + 1) % text_rows;
            }
// \r support 
            if (*ptr == '\r') {
                text_col = 0;
            }
// \r support 


            if (*ptr != '\n') text[text_row][text_col++] = *ptr;
        }
        text[text_row][text_col] = '\0';
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_start_menu(char** headers, char** items) {
    int i;
    pthread_mutex_lock(&gUpdateMutex);
    if (text_rows > 0 && text_cols > 0) {
        for (i = 0; i < text_rows; ++i) {
            if (headers[i] == NULL) break;
            strncpy(menu[i], headers[i], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }
        menu_top = i;
        for (; i < MENU_MAX_ROWS; ++i) {
            if (items[i-menu_top] == NULL) break;
            strncpy(menu[i], items[i-menu_top], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }

//	strcpy(menu[i], " ");
//        ++i;

        menu_items = i - menu_top;
        show_menu = 1;
        menu_sel = menu_show_start = 0;
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

int ui_menu_select(int sel) {
    int old_sel;
    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0) {
        old_sel = menu_sel;
        menu_sel = sel;
/*        if (menu_sel < 0) menu_sel = 0;
          if (menu_sel >= menu_items) menu_sel = menu_items-1;
//        if (menu_sel < 0) menu_sel = menu_items-1;
//        if (menu_sel >= menu_items) menu_sel = 0;

        if (menu_sel < menu_show_start && menu_show_start > 0) {
            menu_show_start--;
        }

        if (menu_sel - menu_show_start + menu_top >= text_rows) {
            menu_show_start++;
        }

        sel = menu_sel;
        if (menu_sel != old_sel) update_screen_locked();
*/

        if (menu_sel < 0) menu_sel = menu_items + menu_sel;
        if (menu_sel >= menu_items) menu_sel = menu_sel - menu_items;


        if (menu_sel < menu_show_start && menu_show_start > 0) {
            menu_show_start = menu_sel;
        }

        if (menu_sel - menu_show_start + menu_top >= text_rows) {
            menu_show_start = menu_sel + menu_top - text_rows + 1;
        }

        sel = menu_sel;

        if (menu_sel != old_sel) update_screen_locked();

    }
    pthread_mutex_unlock(&gUpdateMutex);
    return sel;
}

void ui_end_menu() {
    int i;
    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0 && text_rows > 0 && text_cols > 0) {
        show_menu = 0;
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

int ui_text_visible()
{
    pthread_mutex_lock(&gUpdateMutex);
    int visible = show_text;
    pthread_mutex_unlock(&gUpdateMutex);
    return visible;
}

int ui_wait_key()
{
    pthread_mutex_lock(&key_queue_mutex);
    while (key_queue_len == 0) {
        pthread_cond_wait(&key_queue_cond, &key_queue_mutex);
    }

    int key = key_queue[0];
    memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
    pthread_mutex_unlock(&key_queue_mutex);
    return key;
}

int ui_key_pressed(int key)
{
    // This is a volatile static array, don't bother locking
    return key_pressed[key];
}

void ui_clear_key_queue() {
    pthread_mutex_lock(&key_queue_mutex);
    key_queue_len = 0;
    pthread_mutex_unlock(&key_queue_mutex);
}

