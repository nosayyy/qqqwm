#include "common.h"

static struct wlr_output       *output;
static struct wlr_scene_output *scene_output;

static void output_frame(struct wl_listener *l, void *data) {
	struct timespec now;
	(void)l;
	(void)data;

	wlr_scene_output_commit(scene_output, NULL);
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(scene_output, &now);
}

static void output_destroy(struct wl_listener *l, void *data) {
	(void)l;
	(void)data;
	wl_display_terminate(dpy);
}

void output_new(struct wl_listener *l, void *data) {
	struct wlr_output *o = data;
	struct wlr_output_state st;
	struct wlr_output_mode *mode;
	struct wlr_output_layout_output *lo;

	static struct wl_listener frame   = {.notify = output_frame};
	static struct wl_listener destroy = {.notify = output_destroy};

	(void)l;

	if (output) {
		wlr_log(WLR_ERROR, "additional output ignored");
		return;
	}

	wlr_output_init_render(o, alloc, drw);

	wlr_output_state_init(&st);
	wlr_output_state_set_enabled(&st, 1);

	mode = wlr_output_preferred_mode(o);
	if (mode)
		wlr_output_state_set_mode(&st, mode);

	if (!wlr_output_commit_state(o, &st))
		die("output commit failed");

	wlr_output_state_finish(&st);
	output = o;

	scene_output = wlr_scene_output_create(scene, o);
	if (!scene_output)
		die("wlr_scene_output_create failed");

	lo = wlr_output_layout_add_auto(layout, o);
	wlr_output_layout_get_box(layout, o, &mon_box);

	wlr_scene_output_set_position(scene_output, mon_box.x, mon_box.y);
	wlr_scene_output_layout_add_output(scene_layout, lo, scene_output);

	wl_signal_add(&o->events.frame, &frame);
	wl_signal_add(&o->events.destroy, &destroy);
}