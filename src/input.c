#include "common.h"

static bool keybind(uint32_t mods, xkb_keysym_t sym) {
	if (mods != cfg.modkey)
		return false;

	sym = xkb_keysym_to_lower(sym);
	if (cfg.terminal_key != XKB_KEY_NoSymbol &&
		sym == xkb_keysym_to_lower(cfg.terminal_key)) {
		char *const argv[] = {(char *)cfg.terminal, NULL};
		spawn(&(Arg){.v = argv});
		return true;
	}
	if (cfg.kill_key != XKB_KEY_NoSymbol &&
		sym == xkb_keysym_to_lower(cfg.kill_key)) {
		killclient(&(Arg){0});
		return true;
	}
	if (cfg.quit_key != XKB_KEY_NoSymbol &&
		sym == xkb_keysym_to_lower(cfg.quit_key)) {
		quit(&(Arg){0});
		return true;
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
			nsyms = xkb_keymap_key_get_syms_by_level(kb->keymap, keycode, 0,
				level, &syms);
			for (int i = 0; i < nsyms && !handled; i++)
				handled = keybind(mods, syms[i]);
		}
	}

	if (ev->keycode <= KEY_MAX && (handled || consumed[ev->keycode])) {
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
		w = grab_geom.width + (int)(cursor->x - grab_x);
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
	uint32_t mods;

	if (!c)
		return false;

	focus(c, true);
	kb = wlr_seat_get_keyboard(seat);
	mods = kb ? CLEANMASK(wlr_keyboard_get_modifiers(kb)) : 0;
	if (mods != cfg.modkey)
		return false;

	if (ev->button == cfg.move_button) {
		grab_button = ev->button;
		move_mouse(&(Arg){0});
		return true;
	}
	if (ev->button == cfg.resize_button) {
		grab_button = ev->button;
		resize_mouse(&(Arg){0});
		return true;
	}
	return false;
}

void pointer_button(struct wl_listener *l, void *data) {
	struct wlr_pointer_button_event *ev = data;
	(void)l;

	if (ev->state == WL_POINTER_BUTTON_STATE_PRESSED) {
		if (handle_press(ev))
			return;
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
