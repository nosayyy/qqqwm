#include "common.h"

struct client *top_client(void) {
	return wl_list_empty(&clients)
		? NULL
		: wl_container_of(clients.next, (struct client *)NULL, link);
}

void client_move_resize(struct client *c, int x, int y, int w, int h) {
	c->x = x;
	c->y = y;
	c->w = MAX(w, MIN_W);
	c->h = MAX(h, MIN_H);
	wlr_scene_node_set_position(&c->scene->node, c->x, c->y);
	wlr_xdg_toplevel_set_size(c->xdg->toplevel, c->w, c->h);
}

struct client *client_at(double lx, double ly, struct wlr_surface **psurf,
                         double *sx, double *sy) {
	struct wlr_scene_node *node;
	struct wlr_scene_surface *ss;
	struct wlr_surface *root;
	struct wlr_xdg_surface *xdg;
	double nx = 0, ny = 0;

	*psurf = NULL;
	*sx = *sy = 0;

	node = wlr_scene_node_at(&scene->tree.node, lx, ly, &nx, &ny);
	if (!node || node->type != WLR_SCENE_NODE_BUFFER)
		return NULL;

	ss = wlr_scene_surface_try_from_buffer(wlr_scene_buffer_from_node(node));
	if (!ss)
		return NULL;

	root = wlr_surface_get_root_surface(ss->surface);
	xdg = wlr_xdg_surface_try_from_wlr_surface(root);
	if (!xdg || xdg->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
		return NULL;

	*psurf = ss->surface;
	*sx = nx;
	*sy = ny;
	return xdg->data;
}

void focus(struct client *c, int lift) {
	struct wlr_keyboard *kb;

	if (c == focused) {
		if (c && lift)
			wlr_scene_node_raise_to_top(&c->scene->node);
		return;
	}

	if (focused && focused->mapped)
		wlr_xdg_toplevel_set_activated(focused->xdg->toplevel, 0);
	focused = c;

	if (!c) {
		wlr_seat_keyboard_notify_clear_focus(seat);
		return;
	}

	if (lift) {
		wlr_scene_node_raise_to_top(&c->scene->node);
		wl_list_remove(&c->link);
		wl_list_insert(&clients, &c->link);
	}

	kb = wlr_seat_get_keyboard(seat);
	wlr_seat_keyboard_notify_enter(seat, c->xdg->surface,
		kb ? kb->keycodes : NULL,
		kb ? kb->num_keycodes : 0,
		kb ? &kb->modifiers : NULL);
	wlr_xdg_toplevel_set_activated(c->xdg->toplevel, 1);
}

static void xdg_destroy(struct wl_listener *l, void *data) {
	struct client *c = wl_container_of(l, c, destroy);
	(void)data;

	if (focused == c)
		focused = NULL;

	if (grab == c)
		cancel_grab();

	if (c->mapped) {
		wl_list_remove(&c->link);
		c->mapped = 0;
	}

	if (c->scene) {
		wlr_scene_node_destroy(&c->scene->node);
		c->scene = NULL;
	}

	wl_list_remove(&c->map.link);
	wl_list_remove(&c->unmap.link);
	wl_list_remove(&c->commit.link);
	wl_list_remove(&c->destroy.link);

	c->xdg->data = NULL;
	free(c);
}

static void xdg_unmap(struct wl_listener *l, void *data) {
	struct client *c = wl_container_of(l, c, unmap);
	(void)data;

	if (grab == c)
		cancel_grab();

	if (focused == c)
		focused = NULL;

	if (c->scene) {
		wlr_scene_node_destroy(&c->scene->node);
		c->scene = NULL;
	}

	if (c->mapped) {
		wl_list_remove(&c->link);
		c->mapped = 0;
	}

	focus(top_client(), 1);
}

static void xdg_map(struct wl_listener *l, void *data) {
	struct client *c = wl_container_of(l, c, map);
	(void)data;

	c->scene = wlr_scene_xdg_surface_create(scene_clients, c->xdg);
	c->scene->node.data = c;

	c->w = c->xdg->geometry.width  ? c->xdg->geometry.width  : DEF_W;
	c->h = c->xdg->geometry.height ? c->xdg->geometry.height : DEF_H;

	c->x = mon_box.x + (mon_box.width  - c->w) / 2;
	c->y = mon_box.y + (mon_box.height - c->h) / 2;

	client_move_resize(c, c->x, c->y, c->w, c->h);

	c->mapped = 1;
	wl_list_insert(&clients, &c->link);
	focus(c, 1);
}

static void xdg_commit(struct wl_listener *l, void *data) {
	struct client *c = wl_container_of(l, c, commit);
	(void)data;

	if (!c->xdg->initial_commit || c->initial_configured)
		return;

	c->initial_configured = true;
	wlr_xdg_toplevel_set_size(c->xdg->toplevel, DEF_W, DEF_H);
}

void xdg_new_toplevel(struct wl_listener *l, void *data) {
	struct wlr_xdg_toplevel *tl = data;
	struct client *c = ecalloc(1, sizeof(*c));
	(void)l;

	c->xdg = tl->base;
	tl->base->data = c;

	c->map.notify = xdg_map;
	wl_signal_add(&tl->base->surface->events.map, &c->map);

	c->unmap.notify = xdg_unmap;
	wl_signal_add(&tl->base->surface->events.unmap, &c->unmap);
	
	c->commit.notify = xdg_commit;
	wl_signal_add(&tl->base->surface->events.commit, &c->commit);
	
	c->destroy.notify = xdg_destroy;
	wl_signal_add(&tl->events.destroy, &c->destroy);
}
