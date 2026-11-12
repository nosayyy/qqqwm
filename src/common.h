#ifndef COMMON_H
#define COMMON_H

/* -- includes ------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <libinput.h>
#include <wayland-server-core.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>

#include <wlr/backend.h>
#include <wlr/backend/libinput.h>

#include <wlr/types/wlr_drm.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_pointer.h>

#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>

#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_keyboard_group.h>

#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>

#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>

#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>

#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_linux_dmabuf_v1.h>
#include <wlr/types/wlr_primary_selection.h>

#include <wlr/util/log.h>

#include "xdg-shell-protocol.h"

/* -- types --------------------------------------------------- */
enum cursor_mode  { CUR_NORMAL, CUR_MOVE, CUR_RESIZE };

typedef union {
	int         i;
	uint32_t    ui;
	float       f;
	const void *v;
} Arg;

struct key {
	uint32_t      mod;
	xkb_keysym_t  sym;
	void        (*func)(const Arg *);
	Arg           arg;
};

struct button {
	uint32_t      mod;
	uint32_t      button;
	void        (*func)(const Arg *);
	Arg           arg;
};

struct client {
	struct wlr_xdg_surface *xdg;
	struct wlr_scene_tree  *scene;
	struct wl_list          link;
	int                     mapped;
	int                     x, y, w, h;
	bool                    initial_configured;
	struct wl_listener      map, unmap, commit, destroy;
};

/* -- shared globals (defined in main.c) ---------------------- */
extern struct wl_display              *dpy;
extern struct wlr_renderer            *drw;
extern struct wlr_allocator           *alloc;
extern struct wlr_scene               *scene;
extern struct wlr_scene_tree          *scene_clients;
extern struct wlr_scene_output_layout *scene_layout;
extern struct wlr_output_layout       *layout;
extern struct wlr_box                  mon_box;
extern struct wlr_seat                *seat;
extern struct wlr_cursor              *cursor;
extern struct wlr_xcursor_manager     *cursor_mgr;
extern struct wlr_keyboard_group      *kb_group;
extern struct wl_list                  clients;
extern struct client                  *focused;
extern struct client                  *grab;

/* -- utils.c ------------------------------------------------- */
void  die(const char *fmt, ...);
void *ecalloc(size_t n, size_t s);
void  spawn(const Arg *arg);

/* -- client.c ------------------------------------------------ */
struct client *top_client(void);
void   client_move_resize(struct client *c, int x, int y, int w, int h);
struct client *client_at(double lx, double ly, struct wlr_surface **psurf,
                         double *sx, double *sy);
void  focus(struct client *c, int lift);
void  xdg_new_toplevel(struct wl_listener *l, void *data);

/* -- output.c ------------------------------------------------ */
void  output_new(struct wl_listener *l, void *data);

/* -- input.c ------------------------------------------------- */
void  input_new(struct wl_listener *l, void *data);
void  keyboard_key(struct wl_listener *l, void *data);
void  keyboard_modifiers(struct wl_listener *l, void *data);
void  pointer_motion_relative(struct wl_listener *l, void *data);
void  pointer_motion_absolute(struct wl_listener *l, void *data);
void  pointer_button(struct wl_listener *l, void *data);
void  pointer_axis(struct wl_listener *l, void *data);
void  pointer_frame(struct wl_listener *l, void *data);
void  pointer_request_cursor(struct wl_listener *l, void *data);
void  seat_request_set_selection(struct wl_listener *l, void *data);
void  seat_request_set_primary_selection(struct wl_listener *l, void *data);
void  cancel_grab(void);
// actions
void  killclient(const Arg *arg);
void  quit(const Arg *arg);
void  move_mouse(const Arg *arg);
void  resize_mouse(const Arg *arg);


/* -- defines --------------------------------------------------- */
#define CLEANMASK(m) ((m) & ~(WLR_MODIFIER_CAPS | WLR_MODIFIER_MOD2))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LENGTH(a) (sizeof(a) / sizeof((a)[0]))
#define UNUSED __attribute__((unused))

#include "config.h"

#endif // COMMON_H
