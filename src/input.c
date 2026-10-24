#include "common.h"

static enum cursor_mode cursor_mode;
static double            grab_x, grab_y;
static struct wlr_box    grab_geom;
static uint32_t          grab_button;
static bool              consumed[KEY_MAX + 1];


/* -- grab state ---------------------------------------------- */

void cancel_grab(void) {
	if (cursor_mode != CUR_NORMAL)
		wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");
	grab = NULL;
	grab_button = 0;
	cursor_mode = CUR_NORMAL;
}

static bool start_grab(struct client *c, uint32_t button) {
	if (button == BTN_RIGHT) {
		cursor_mode = CUR_RESIZE;
		grab = c;
		grab_button = button;
		grab_geom = (struct wlr_box){c->x, c->y, c->w, c->h};
		grab_x = cursor->x;
		grab_y = cursor->y;
		wlr_cursor_set_xcursor(cursor, cursor_mgr, "se-resize");
		return true;
	}

	if (button == BTN_LEFT) {
		cursor_mode = CUR_MOVE;
		grab = c;
		grab_button = button;
		grab_x = cursor->x - c->x;
		grab_y = cursor->y - c->y;
		wlr_cursor_set_xcursor(cursor, cursor_mgr, "grabbing");
		return true;
	}

	return false;
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
                libinput_device_config_tap_set_enabled(libinput_dev, LIBINPUT_CONFIG_TAP_ENABLED);
                libinput_device_config_tap_set_drag_enabled(libinput_dev, LIBINPUT_CONFIG_DRAG_ENABLED);
                libinput_device_config_tap_set_drag_lock_enabled(libinput_dev, LIBINPUT_CONFIG_DRAG_LOCK_ENABLED);
            }
        }
		wlr_cursor_attach_input_device(cursor, dev);
		break;
    }
	default:
		break;
	}

	caps = WL_SEAT_CAPABILITY_POINTER;
	if (!wl_list_empty(&kb_group->devices))
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	wlr_seat_set_capabilities(seat, caps);
}


/* -- keyboard ------------------------------------------------ */

static bool keybind(xkb_keysym_t sym) {
	switch (sym) {
	case KEYBIND_TERMINAL:
		spawn(TERMINAL);
		return true;
	case KEYBIND_CLOSE:
		if (focused)
			wlr_xdg_toplevel_send_close(focused->xdg->toplevel);
		return true;
	case KEYBIND_QUIT:
		wl_display_terminate(dpy);
		return true;
	}
	return false;
}

void keyboard_key(struct wl_listener *l, void *data) {
	struct wlr_keyboard_key_event *ev = data;
	struct wlr_keyboard *kb = &kb_group->keyboard;
	uint32_t keycode = ev->keycode + 8;
	const xkb_keysym_t *syms;
	int nsyms;
	bool handled = false;
	(void)l;

	nsyms = xkb_state_key_get_syms(kb->xkb_state, keycode, &syms);

	if (ev->state == WL_KEYBOARD_KEY_STATE_PRESSED &&
	    CLEANMASK(wlr_keyboard_get_modifiers(kb)) == MOD) {
		for (int i = 0; i < nsyms && !handled; i++)
			handled = keybind(syms[i]);
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

	if (!grab || cursor_mode == CUR_NORMAL)
		return;

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

	if (c && c != focused)
		focus(c, false);

	wlr_seat_pointer_notify_enter(seat, surf, sx, sy);
	if (time)
		wlr_seat_pointer_notify_motion(seat, time, sx, sy);
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

	if (!c)
		return false;

	focus(c, true);

	kb = wlr_seat_get_keyboard(seat);
	if (!kb || CLEANMASK(wlr_keyboard_get_modifiers(kb)) != MOD)
		return false;

	return start_grab(c, ev->button);
}

void pointer_button(struct wl_listener *l, void *data) {
	struct wlr_pointer_button_event *ev = data;
	(void)l;

	if (ev->state == WL_POINTER_BUTTON_STATE_PRESSED) {
		if (handle_press(ev))
			return;
	} else if (cursor_mode != CUR_NORMAL && ev->button == grab_button) {
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

	if (cursor_mode != CUR_NORMAL)
		return;

	if (ev->seat_client == seat->pointer_state.focused_client)
		wlr_cursor_set_surface(cursor, ev->surface, ev->hotspot_x, ev->hotspot_y);
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
