#include "common.h"

static enum        cursor_mode cursor_mode;
static double      grab_x, grab_y;
static struct      wlr_box grab_geom;
static uint32_t    grab_button;
static bool        consumed[KEY_MAX + 1];


/* -- grab state ---------------------------------------------- */

void cancel_grab(void) {
	if (cursor_mode != CUR_NORMAL) {
		wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");
    }
    grab = NULL;
	grab_button = 0;
	cursor_mode = CUR_NORMAL;
}

static void start_grab(struct client *c, enum cursor_mode mode) {
	if (!c) {
		return;
    }

	cursor_mode = mode;
	grab = c;

	if (mode == CUR_RESIZE) {
		grab_geom = (struct wlr_box){c->x, c->y, c->w, c->h};
		grab_x = cursor->x;
		grab_y = cursor->y;
		wlr_cursor_set_xcursor(cursor, cursor_mgr, "se-resize");
	} else {
		grab_x = cursor->x - c->x;
		grab_y = cursor->y - c->y;
		wlr_cursor_set_xcursor(cursor, cursor_mgr, "grabbing");
	}
}


/* -- actions ------------------------------------------------- */

void killclient(const Arg *arg) {
	(void)arg;
	if (focused) {
		wlr_xdg_toplevel_send_close(focused->xdg->toplevel);
    }
}

void quit(const Arg *arg) {
	(void)arg;
	wl_display_terminate(dpy);
}

void move_mouse(const Arg *arg) {
	(void)arg;
	start_grab(focused, CUR_MOVE);
}

void resize_mouse(const Arg *arg) {
	(void)arg;
	start_grab(focused, CUR_RESIZE);
}


/* -- device setup -------------------------------------------- */

void input_new(struct wl_listener *l, void *data) {
	struct wlr_input_device *dev = data;
	uint32_t caps;
	(void)l;

	switch (dev->type) {
	    case WLR_INPUT_DEVICE_KEYBOARD: {
		    struct wlr_keyboard *kb = wlr_keyboard_from_input_device(dev);
		    wlr_keyboard_set_keymap(kb, kb_group->keyboard.keymap);
		    wlr_keyboard_group_add_keyboard(kb_group, kb);
		    break;
	    }
	    case WLR_INPUT_DEVICE_POINTER: {
            if (wlr_input_device_is_libinput(dev)) {
                struct libinput_device *libinput_dev = wlr_libinput_get_device_handle(dev);
            
                if (libinput_device_config_tap_get_finger_count(libinput_dev) > 0) {
                    libinput_device_config_tap_set_enabled(libinput_dev,
                            LIBINPUT_CONFIG_TAP_ENABLED);

                    libinput_device_config_tap_set_drag_enabled(libinput_dev,
                            LIBINPUT_CONFIG_DRAG_ENABLED);

                    // libinput_device_config_tap_set_drag_lock_enabled(libinput_dev,
                    //        LIBINPUT_CONFIG_DRAG_LOCK_ENABLED);
                }
            }
		    wlr_cursor_attach_input_device(cursor, dev);
		    break;
        }

	    default:
		    break;
	}

	caps = WL_SEAT_CAPABILITY_POINTER;
	if (!wl_list_empty(&kb_group->devices)) {
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
	wlr_seat_set_capabilities(seat, caps);
}


/* -- keyboard ------------------------------------------------ */

static bool keybind(uint32_t mods, xkb_keysym_t sym) {
	for (size_t i = 0; i < LENGTH(keys); i++) {
		if (mods == keys[i].mod && keys[i].func &&
		    xkb_keysym_to_lower(sym) == xkb_keysym_to_lower(keys[i].sym)) {
			keys[i].func(&keys[i].arg);
			return true;
		}
	}

	return false;
}

void keyboard_key(struct wl_listener *l, void *data) {
	struct wlr_keyboard_key_event *ev = data;
	struct wlr_keyboard *kb = &kb_group->keyboard;
	uint32_t keycode = ev->keycode + 8;
	uint32_t mods = CLEANMASK(wlr_keyboard_get_modifiers(kb));
	xkb_level_index_t level;
	const xkb_keysym_t *syms;
	int nsyms;
	bool handled = false;
	(void)l;

	if (ev->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		nsyms = xkb_state_key_get_syms(kb->xkb_state, keycode, &syms);
		for (int i = 0; i < nsyms && !handled; i++)
			handled = keybind(mods, syms[i]);

		if (!handled) {
			level = xkb_state_key_get_level(kb->xkb_state, keycode, 0);
			nsyms = xkb_keymap_key_get_syms_by_level(kb->keymap, keycode, 0, level, &syms);
			for (int i = 0; i < nsyms && !handled; i++)
				handled = keybind(mods, syms[i]);
		}
	}

	if (handled || consumed[ev->keycode]) {
		consumed[ev->keycode] = ev->state == WL_KEYBOARD_KEY_STATE_PRESSED;
		return;
	}

	wlr_seat_set_keyboard(seat, kb);
	wlr_seat_keyboard_notify_key(seat, ev->time_msec, ev->keycode, ev->state);
}

void keyboard_modifiers(struct wl_listener *l, void *data) {
	(void)l;
	(void)data;
	wlr_seat_set_keyboard(seat, &kb_group->keyboard);
	wlr_seat_keyboard_notify_modifiers(seat, &kb_group->keyboard.modifiers);
}


/* -- pointer ------------------------------------------------- */

static void drag_update(void) {
	int x, y, w, h;

	if (!grab || cursor_mode == CUR_NORMAL) {
		return;
    }

	if (cursor_mode == CUR_MOVE) {
		x = (int)(cursor->x - grab_x);
		y = (int)(cursor->y - grab_y);
		w = grab->w;
		h = grab->h;
	} else {
		x = grab_geom.x;
		y = grab_geom.y;
		w = grab_geom.width  + (int)(cursor->x - grab_x);
		h = grab_geom.height + (int)(cursor->y - grab_y);
	}

	client_move_resize(grab, x, y, w, h);
}

static void hover_update(uint32_t time) {
	double sx, sy;
	struct wlr_surface *surf = NULL;
	struct client *c = client_at(cursor->x, cursor->y, &surf, &sx, &sy);

	if (!surf) {
		wlr_seat_pointer_notify_clear_focus(seat);
		wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");
		return;
	}

	if (c && c != focused) {
		focus(c, false);
    }

	wlr_seat_pointer_notify_enter(seat, surf, sx, sy);
	if (time) {
		wlr_seat_pointer_notify_motion(seat, time, sx, sy);
    }
}

static void pointer_motion(uint32_t time) {
	drag_update();
	hover_update(time);
}

void pointer_motion_relative(struct wl_listener *l, void *data) {
	struct wlr_pointer_motion_event *ev = data;
	(void)l;
	wlr_cursor_move(cursor, &ev->pointer->base, ev->delta_x, ev->delta_y);
	pointer_motion(ev->time_msec);
}

void pointer_motion_absolute(struct wl_listener *l, void *data) {
	struct wlr_pointer_motion_absolute_event *ev = data;
	(void)l;
	wlr_cursor_warp_absolute(cursor, &ev->pointer->base, ev->x, ev->y);
	pointer_motion(ev->time_msec);
}

static bool handle_press(struct wlr_pointer_button_event *ev) {
	double sx, sy;
	struct wlr_surface *surf;
	struct client *c = client_at(cursor->x, cursor->y, &surf, &sx, &sy);
	struct wlr_keyboard *kb;
	uint32_t mods;

	if (!c) {
		return false;
    }

	focus(c, true);

	kb = wlr_seat_get_keyboard(seat);
	mods = kb ? CLEANMASK(wlr_keyboard_get_modifiers(kb)) : 0;

	for (size_t i = 0; i < LENGTH(buttons); i++) {
		if (buttons[i].mod == mods && buttons[i].button == ev->button &&
		    buttons[i].func) {
			grab_button = ev->button;
			buttons[i].func(&buttons[i].arg);
			return true;
		}
	}

	return false;
}

void pointer_button(struct wl_listener *l, void *data) {
	struct wlr_pointer_button_event *ev = data;
	(void)l;

	if (ev->state == WL_POINTER_BUTTON_STATE_PRESSED) {
		if (handle_press(ev)) {
			return;
        }
	} else if (grab_button && ev->button == grab_button) {
		cancel_grab();
		return;
	}

	wlr_seat_pointer_notify_button(seat, ev->time_msec, ev->button, ev->state);
}

void pointer_axis(struct wl_listener *l, void *data) {
	struct wlr_pointer_axis_event *ev = data;
	(void)l;
	wlr_seat_pointer_notify_axis(seat, ev->time_msec, ev->orientation,
		ev->delta, ev->delta_discrete, ev->source, ev->relative_direction);
}

void pointer_frame(struct wl_listener *l, void *data) {
	(void)l;
	(void)data;
	wlr_seat_pointer_notify_frame(seat);
}

void pointer_request_cursor(struct wl_listener *l, void *data) {
	struct wlr_seat_pointer_request_set_cursor_event *ev = data;
	(void)l;

	if (cursor_mode != CUR_NORMAL) {
		return;
    }

	if (ev->seat_client == seat->pointer_state.focused_client) {
		wlr_cursor_set_surface(cursor, ev->surface, ev->hotspot_x, ev->hotspot_y);
    }
}

void seat_request_set_selection(struct wl_listener *l, void *data) {
	struct wlr_seat_request_set_selection_event *ev = data;
	(void)l;

	wlr_seat_set_selection(seat, ev->source, ev->serial);
}

void seat_request_set_primary_selection(struct wl_listener *l, void *data) {
	struct wlr_seat_request_set_primary_selection_event *ev = data;
	(void)l;

	wlr_seat_set_primary_selection(seat, ev->source, ev->serial);
}
