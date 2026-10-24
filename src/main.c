#include "common.h"

/* -- shared globals ------------------------------------------ */
struct wl_display              *dpy;
struct wlr_renderer            *drw;
struct wlr_allocator           *alloc;
struct wlr_scene               *scene;
struct wlr_scene_tree          *scene_clients;
struct wlr_scene_output_layout *scene_layout;
struct wlr_output_layout       *layout;
struct wlr_box                  mon_box;
struct wlr_seat                *seat;
struct wlr_cursor              *cursor;
struct wlr_xcursor_manager     *cursor_mgr;
struct wlr_keyboard_group      *kb_group;
struct wl_list                  clients;
struct client                  *focused;
struct client                  *grab;

static struct wlr_backend *backend;

/* -- listeners ----------------------------------------------- */
static struct wl_listener l_new_output              = {.notify = output_new};
static struct wl_listener l_new_input               = {.notify = input_new};
static struct wl_listener l_new_toplevel            = {.notify = xdg_new_toplevel};
static struct wl_listener l_cursor_motion           = {.notify = pointer_motion_relative};
static struct wl_listener l_cursor_motion_abs       = {.notify = pointer_motion_absolute};
static struct wl_listener l_cursor_button           = {.notify = pointer_button};
static struct wl_listener l_cursor_axis             = {.notify = pointer_axis};
static struct wl_listener l_cursor_frame            = {.notify = pointer_frame};
static struct wl_listener l_request_cursor          = {.notify = pointer_request_cursor};
static struct wl_listener l_kb_key                  = {.notify = keyboard_key};
static struct wl_listener l_kb_mods                 = {.notify = keyboard_modifiers};
static struct wl_listener l_request_set_selection         = {.notify = seat_request_set_selection};
static struct wl_listener l_request_set_primary_selection = {.notify = seat_request_set_primary_selection};

int main(void) {
	const char *socket;
	struct wl_event_loop *loop;
	struct wlr_xdg_shell  *xdg_shell;
	struct xkb_context   *xkb_ctx;
	struct xkb_keymap    *xkb_keymap;

	if (!getenv("XDG_RUNTIME_DIR"))
		die("XDG_RUNTIME_DIR must be set");

	signal(SIGCHLD, SIG_IGN);
	wlr_log_init(WLR_ERROR, NULL);

	/* --- Setup -------------------------------------------- */
	dpy  = wl_display_create();
	loop = wl_display_get_event_loop(dpy);

	backend = wlr_backend_autocreate(loop, NULL);
	if (!backend)
		die("wlr_backend_autocreate failed");

	drw = wlr_renderer_autocreate(backend);
	if (!drw)
		die("wlr_renderer_autocreate failed");

	wlr_renderer_init_wl_display(drw, dpy);

	alloc = wlr_allocator_autocreate(backend, drw);
	if (!alloc)
		die("wlr_allocator_autocreate failed");

	wlr_compositor_create(dpy, 6, drw);
	wlr_subcompositor_create(dpy);
	wlr_data_device_manager_create(dpy);

	layout = wlr_output_layout_create(dpy);
	scene  = wlr_scene_create();

	scene_layout = wlr_scene_attach_output_layout(scene, layout);
	if (!scene_layout)
		die("wlr_scene_attach_output_layout failed");

	scene_clients = wlr_scene_tree_create(&scene->tree);

	if (wlr_renderer_get_texture_formats(drw, WLR_BUFFER_CAP_DMABUF)) {
		wlr_drm_create(dpy, drw);
		wlr_scene_set_linux_dmabuf_v1(scene,
			wlr_linux_dmabuf_v1_create_with_renderer(dpy, 5, drw));
	}

	xdg_shell = wlr_xdg_shell_create(dpy, 6);
	if (!xdg_shell)
		die("wlr_xdg_shell_create failed");
	wl_signal_add(&xdg_shell->events.new_toplevel, &l_new_toplevel);

	cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(cursor, layout);

	cursor_mgr = wlr_xcursor_manager_create(CURSOR_THEME, CURSOR_SIZE);
	if (!cursor_mgr)
		die("wlr_xcursor_manager_create failed");

	wlr_xcursor_manager_load(cursor_mgr, 1);
	wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");

	wl_signal_add(&cursor->events.motion,          &l_cursor_motion);
	wl_signal_add(&cursor->events.motion_absolute, &l_cursor_motion_abs);
	wl_signal_add(&cursor->events.button,          &l_cursor_button);
	wl_signal_add(&cursor->events.axis,            &l_cursor_axis);
	wl_signal_add(&cursor->events.frame,           &l_cursor_frame);

	seat = wlr_seat_create(dpy, "seat0");
	wl_signal_add(&seat->events.request_set_cursor, &l_request_cursor);
	wl_signal_add(&seat->events.request_set_selection, &l_request_set_selection);
	wl_signal_add(&seat->events.request_set_primary_selection, &l_request_set_primary_selection);

	kb_group = wlr_keyboard_group_create();

	xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	xkb_keymap = xkb_keymap_new_from_names(xkb_ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!xkb_keymap)
		die("xkb_keymap_new_from_names failed");

	wlr_keyboard_set_keymap(&kb_group->keyboard, xkb_keymap);
	xkb_keymap_unref(xkb_keymap);
	xkb_context_unref(xkb_ctx);
	wlr_keyboard_set_repeat_info(&kb_group->keyboard, REPEAT_RATE, REPEAT_DELAY);
	wl_signal_add(&kb_group->keyboard.events.key,       &l_kb_key);
	wl_signal_add(&kb_group->keyboard.events.modifiers, &l_kb_mods);
	wlr_seat_set_keyboard(seat, &kb_group->keyboard);

	wl_list_init(&clients);
	wl_signal_add(&backend->events.new_output, &l_new_output);
	wl_signal_add(&backend->events.new_input,  &l_new_input);

	/* --- Run ---------------------------------------------- */
	socket = wl_display_add_socket_auto(dpy);
	if (!socket)
		die("wl_display_add_socket_auto failed");

	setenv("WAYLAND_DISPLAY", socket, 1);

	if (!wlr_backend_start(backend))
		die("wlr_backend_start failed");

	wl_display_run(dpy);

	wl_display_destroy_clients(dpy);
	wl_display_destroy(dpy);
	return 0;
}