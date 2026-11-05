#include "common.h"

/* DRM_FORMAT_XRGB8888, defined here to avoid pulling in drm_fourcc.h */
#define FMT_XRGB8888 0x34325258
#define WALL_MAX     8192

static struct wlr_output       *output;
static struct wlr_scene_output *scene_output;

static uint32_t be32(const unsigned char *p) {
	return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
	       (uint32_t)p[2] << 8  | (uint32_t)p[3];
}

static struct wlr_buffer *wallpaper_buffer(const char *rel) {
	unsigned char hdr[16], *row = NULL;
	char path[4096];
	const char *home = getenv("HOME");
	struct wlr_buffer *buf;
	uint32_t *px = NULL;
	uint32_t w, h, x, y;
	FILE *f;

	if (!rel || !home) {
		return NULL;
	}

	snprintf(path, sizeof(path), "%s/%s", home, rel);
	f = fopen(path, "rb");
	if (!f) {
		wlr_log(WLR_ERROR, "wallpaper: cannot open %s", path);
		return NULL;
	}

	if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr) || memcmp(hdr, "farbfeld", 8)) {
		wlr_log(WLR_ERROR, "wallpaper: not a farbfeld file");
		goto fail;
	}

	w = be32(hdr + 8);
	h = be32(hdr + 12);
	if (!w || !h || w > WALL_MAX || h > WALL_MAX) {
		wlr_log(WLR_ERROR, "wallpaper: bad dimensions %ux%u", w, h);
		goto fail;
	}

	px  = ecalloc((size_t)w * h, 4);
	row = ecalloc(w, 8);

	for (y = 0; y < h; y++) {
		if (fread(row, 1, (size_t)w * 8, f) != (size_t)w * 8) {
			wlr_log(WLR_ERROR, "wallpaper: truncated file");
			goto fail;
		}
		for (x = 0; x < w; x++) {
			px[(size_t)y * w + x] = (uint32_t)row[x * 8]     << 16 |
			                        (uint32_t)row[x * 8 + 2] << 8  |
			                        (uint32_t)row[x * 8 + 4];
		}
	}

	/* the buffer keeps its own copy of the pixels */
	buf = wlr_readonly_data_buffer_create(FMT_XRGB8888, (size_t)w * 4, w, h, px);
	if (!buf) {
		wlr_log(WLR_ERROR, "wallpaper: buffer creation failed");
		goto fail;
	}

	free(px);
	free(row);
	fclose(f);
	return buf;

fail:
	free(px);
	free(row);
	fclose(f);
	return NULL;
}

static void background_create(void) {
	struct wlr_buffer *buf = wallpaper_buffer(WALLPAPER);
	struct wlr_scene_buffer *sb;
	struct wlr_fbox src;
	double s;

	if (!buf) {
		return;
	}

	/* scale to cover the monitor, crop the overflow around the center */
	s = MAX((double)mon_box.width / buf->width, (double)mon_box.height / buf->height);
	src.width  = mon_box.width  / s;
	src.height = mon_box.height / s;
	src.x = (buf->width  - src.width)  / 2;
	src.y = (buf->height - src.height) / 2;

	sb = wlr_scene_buffer_create(&scene->tree, buf);
	wlr_scene_buffer_set_source_box(sb, &src);
	wlr_scene_buffer_set_dest_size(sb, mon_box.width, mon_box.height);
	wlr_scene_node_set_position(&sb->node, mon_box.x, mon_box.y);
	wlr_scene_node_lower_to_bottom(&sb->node);
	wlr_buffer_drop(buf);
}

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
	if (mode) {
		wlr_output_state_set_mode(&st, mode);
    }

	if (!wlr_output_commit_state(o, &st)) {
		die("output commit failed");
    }

	wlr_output_state_finish(&st);
	output = o;

	scene_output = wlr_scene_output_create(scene, o);
	if (!scene_output) {
		die("wlr_scene_output_create failed");
    }

	lo = wlr_output_layout_add_auto(layout, o);
	wlr_output_layout_get_box(layout, o, &mon_box);

	wlr_scene_output_set_position(scene_output, mon_box.x, mon_box.y);
	wlr_scene_output_layout_add_output(scene_layout, lo, scene_output);

	background_create();

	wl_signal_add(&o->events.frame, &frame);
	wl_signal_add(&o->events.destroy, &destroy);
}